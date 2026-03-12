#include "vtcore.h"
#include "WatchedImageUserImage.h"
#include "WatchedImageHelpers.h"

#include <vector>

using namespace System::Collections::Generic;
using namespace Microsoft::ImageWatch;

Microsoft::ImageWatch::WatchedImageInfo^ 
	Microsoft::ImageWatch::WatchedImageUserImage::MakeInfo()
{
	auto info = gcnew WatchedImageInfo();

	bool allPlanesInit = NumPlanes() > 0;
	for (UInt32 i = 0; i < NumPlanes(); ++i)
	{
		if (planeInfo_[i].PixelAddress == 0)
		{
			allPlanesInit = false;
			break;
		}
	}
	
	info->IsInitialized = allPlanesInit;
	
	info->Width = Width();
	info->Height = Height();
	
	info->NumBytesPerPixel = VT_IMG_ELSIZE(elementFormat_) * NumBands();

	info->PixelFormat = pixelFormat_ == UserPixelFormat::Generic ? 
		nullptr : GetPixelFormatName(pixelFormat_);

	info->ViewerPixelFormat = VT_IMG_MAKE_TYPE(elementFormat_, NumBands());

	info->HasNativePixelValues = true;

	if (NumPlanes() == 1 && pixelFormat_ != UserPixelFormat::YUY2)
	{
		info->HasNativePixelAddress = true;
		info->NumStrideBytes = NumStrideBytes();
		info->PixelAddress = PixelAddress();
	}
	else
	{
		info->HasNativePixelAddress = false;
		info->NumStrideBytes = 0;
		info->PixelAddress = 0;
	}

	info->SpecialColorMapName = specialColormapName_;
	
	if (customRange_ != nullptr)
	{
		System::Diagnostics::Debug::Assert(customRange_->Length == 2);
		if (customRange_->Length == 2)
		{
			info->HasCustomValueRange = true;
			info->CustomValueRangeStart = customRange_[0];
			info->CustomValueRangeEnd = customRange_[1];
		}
	}

	return info;
}

bool Microsoft::ImageWatch::WatchedImageUserImage::TryParseUInt32Array(
	String^ str, cli::array<UInt32>^% arr)
{
	cli::array<UInt64>^ tmp = nullptr;
	if (!TryParseUInt64Array(str, tmp))
		return false;

	if (tmp == nullptr)
		return false;
	
	arr = gcnew array<UInt32>(tmp->Length);
	for (int i = 0; i < tmp->Length; ++i)
	{
		if (tmp[i] > UInt32::MaxValue)
			return false;

		arr[i] = Convert::ToUInt32(tmp[i]);
	}

	return true;
}

bool Microsoft::ImageWatch::WatchedImageUserImage::TryParseUInt64Array(
	String^ str, cli::array<UInt64>^% arr)
{
	if (str == nullptr)
		return false;

	auto vlist = str->Split((gcnew String(",;|"))->ToCharArray(), 
		StringSplitOptions::RemoveEmptyEntries);
	
	if (vlist == nullptr || vlist->Length < 1)
		return false;

	arr = gcnew array<UInt64>(vlist->Length);
	
	for (int i = 0; i < arr->Length; ++i)
	{
		if (!WatchedImageHelpers::TryParseUInt64(vlist[i], arr[i]))
			return false;
	}

	return true;
}

bool Microsoft::ImageWatch::WatchedImageUserImage::TryParseDoubleArray(
	String^ str, cli::array<double>^% arr)
{
	if (str == nullptr)
		return false;

	auto vlist = str->Split((gcnew String(",;|"))->ToCharArray(), 
		StringSplitOptions::RemoveEmptyEntries);
	
	if (vlist == nullptr || vlist->Length < 1)
		return false;

	arr = gcnew array<double>(vlist->Length);
	
	for (int i = 0; i < arr->Length; ++i)
	{
		if (!WatchedImageHelpers::TryParseDouble(vlist[i], arr[i]))
			return false;
	}

	return true;
}

void Microsoft::ImageWatch::WatchedImageUserImage::DoReloadInfo(
	Microsoft::ImageWatch::WatchedImageInfo^% info)
{
	info = nullptr;

	auto expr = String::Format("({0})0x{1:x}", ObjectPointerType, 
		ObjectAddress);
	
	auto plist = WatchedImageHelpers::GetEvaluatedExpressionChildrenInfo(
		InspectionContext, Frame, expr);
	if (plist == nullptr)
		return;

#define EVALERROR(msg_) \
	{ \
	LogError(String::Format("ERROR: cannot evaluate {0}: {1}", expr, msg_)); \
	return; \
	}

	// NOTE: don't log errors in GETVALUESTRING! 
	// our contract with the .natvis author: if property shows 
	// up and it's ill-formatted, then we log an error. but if
	// the property is missing, we simple stop image evaluation.
	// this is important for allowing images to be in invalid or 
	// non-watchable states (weird pixel types, for example)
#define GETVALUESTRING(name_) \
	{ \
	String^ err = nullptr; \
	if (!WatchedImageHelpers::GetPropertyChildValue(plist, name_, \
	valueString, err)) \
	return;\
	} 

	String^ valueString = nullptr;

	GETVALUESTRING("[type]");
	if (!WatchedImageHelpers::TryParseNeutralElFormatString(valueString, 
		elementFormat_))
		EVALERROR("invalid channel type");

	GETVALUESTRING("[channels]");
	UInt32 numBandsPerPlane = 0;
	if (!ParsePixelFormat(valueString, pixelFormat_, numBandsPerPlane,
		specialColormapName_))
	{
		// if parse error and NOT a generic number format, natvis author 
		// has probably made a mistake -> log error
		if (pixelFormat_ != UserPixelFormat::Generic)
			EVALERROR("invalid channel format");

		return;
	}

	// don't log the following parse errors, because TryParseUInt* returns 
	// false if number is actually there, but is negative (that's probably 
	// uninitialized/bad memory and not a reason to log an error)
	
	GETVALUESTRING("[width]");
	UInt32 width = 0;
	if (!WatchedImageHelpers::TryParseUInt32(valueString, width))
		return;
	
	GETVALUESTRING("[height]");
	UInt32 height = 0;
	if (!WatchedImageHelpers::TryParseUInt32(valueString, height))
		return;

	// [planes] property is optional (if absent, single plane is assumed)
	UInt32 numPlanes = 1;
	String^ errString = "";
	if (WatchedImageHelpers::GetPropertyChildValue(plist, "[planes]", 
		valueString, errString))
	{
		if (!WatchedImageHelpers::TryParseUInt32(valueString, numPlanes))
			return;
		if (numPlanes > VT_IMG_BANDS_MAX)
			return;
	}

	// data has one or numPlanes entries. if one, we assume each plane starts 
	// on the next row after the previous plane (see MakePlaneInfos)
	GETVALUESTRING("[data]");	
	array<UInt64>^ pixelAddressArr = nullptr;
	if (!TryParseUInt64Array(valueString, pixelAddressArr))
		return;	
	System::Diagnostics::Debug::Assert(pixelAddressArr != nullptr && 
		pixelAddressArr->Length > 0);
	if (!(pixelAddressArr->Length == 1 || 
		pixelAddressArr->Length == (int)numPlanes))
		EVALERROR("number of data pointers must be one or #planes");
	
	// data has one or numPlanes entries. if one, we assume all strides are 
	// the same (see MakePlaneInfos)
	GETVALUESTRING("[stride]");
	array<UInt32>^ numStrideBytesArr = nullptr;
	if (!TryParseUInt32Array(valueString, numStrideBytesArr))
		return;
	System::Diagnostics::Debug::Assert(numStrideBytesArr != nullptr && 
		numStrideBytesArr ->Length > 0);
	if (!(numStrideBytesArr->Length == 1 || 
		numStrideBytesArr->Length == (int)numPlanes))
		EVALERROR("number of strides must be one or #planes");

	planeInfo_ = MakePlaneInfos(numPlanes, numBandsPerPlane, pixelFormat_, width,
		height, pixelAddressArr, numStrideBytesArr);

	if (planeInfo_ == nullptr)
		return;

	if (NumBands() != numBandsPerPlane * numPlanes)
		EVALERROR("inconsistent number of channels/planes");

	// [range] is optional
	customRange_ = nullptr;			
	errString = "";
	if (WatchedImageHelpers::GetPropertyChildValue(plist, "[range]", 
		valueString, errString))
	{
		array<double>^ rtmp = nullptr;
		if (!TryParseDoubleArray(valueString, rtmp))
			return;	
		if (rtmp->Length != 2)
			return;
		for (int i = 0; i < 2; ++i)
		{
			if (double::IsNaN(rtmp[i]) || double::IsInfinity(rtmp[i]))
				return;
		}
		if (rtmp[0] > rtmp[1])
			return;

		customRange_ = rtmp;
	}
			
#undef GETVALUESTRING
#undef EVALERROR

	for (int i = 0; i < planeInfo_->Length; ++i)
	{
		auto pi = planeInfo_[i];
		
		// make sure test succeed for yuy2, which is stored as 2 band ..
		if (pixelFormat_ == UserPixelFormat::YUY2)
			pi.NumBands = 2;

		if (!WatchedImageHelpers::ValidateVTCImgInfo(pi.Width, pi.Height,
			VT_IMG_MAKE_TYPE(elementFormat_, pi.NumBands), pi.NumStrideBytes, 
			pi.PixelAddress))
		{
			return;
		}
	}

	info = MakeInfo();
}

template <typename ElementType, unsigned int NumBands>
void UpSampleRowBy2(ElementType* dstRow, const ElementType* srcRow, 
					unsigned int numPixels)
{
	if (dstRow == nullptr || srcRow == nullptr)
	{
		System::Diagnostics::Debug::Assert(false);
		return;
	}

	for (unsigned int i = 0; i < numPixels; ++i)
	{
		const unsigned int isrc = i >> 1;

		for (unsigned int b = 0; b < NumBands; ++b)
			dstRow[i * NumBands + b] = srcRow[isrc * NumBands + b];
	}
}

void UpSampleRow(void* dstRow, const void* srcRow, 
				 unsigned int numPixels, int type, int factor)
{
	const int numBands = VT_IMG_BANDS(type);

	if (factor == 1)
	{
		memcpy(dstRow, srcRow, 
			numPixels * VT_IMG_ELSIZE(type) * numBands);
		return;
	}
	
	if (factor != 2)
	{
		System::Diagnostics::Debug::Assert(false);
		return;
	}

#define SAMPLEROWCASE(typeId_, type_) \
	case typeId_: \
	{ \
	switch (numBands) \
	{ \
	case 1: UpSampleRowBy2<type_, 1>((type_*)dstRow, (type_*)srcRow, numPixels); break; \
	case 2: UpSampleRowBy2<type_, 2>((type_*)dstRow, (type_*)srcRow, numPixels); break; \
	default: System::Diagnostics::Debug::Assert(false); return; \
	} \
	break; \
	} 
		
	switch (EL_FORMAT(type))
	{
		SAMPLEROWCASE(EL_FORMAT_BYTE, uint8_t);
		SAMPLEROWCASE(EL_FORMAT_SBYTE, int8_t);
		SAMPLEROWCASE(EL_FORMAT_SHORT, uint16_t);
		SAMPLEROWCASE(EL_FORMAT_SSHORT, int16_t);
		SAMPLEROWCASE(EL_FORMAT_INT, int);
		SAMPLEROWCASE(EL_FORMAT_FLOAT, float);
		SAMPLEROWCASE(EL_FORMAT_DOUBLE, double);
		SAMPLEROWCASE(EL_FORMAT_HALF_FLOAT, vt::HALF_FLOAT);

	default:
		System::Diagnostics::Debug::Assert(false);
		return;
	}

#undef SAMPLEROWCASE
}

unsigned int GetUpSampleFactor(int dstWidth, int srcWidth)
{
	if (dstWidth == srcWidth)
		return 1;
	else if (dstWidth / 2 == srcWidth)
		return 2;
	else
		return 0;	
}

bool UpSampleByUpTo2(vt::CImg& dst, const vt::CImg& src)
{
	if (!dst.IsValid() || !src.IsValid())
	{
		System::Diagnostics::Debug::Assert(false);
		return false;
	}

	const int xfactor = GetUpSampleFactor(dst.Width(), src.Width());
	if (xfactor == 0 || xfactor > 2)
		return false;
	const int yfactor = GetUpSampleFactor(dst.Height(), src.Height());
	if (yfactor == 0 || yfactor > 2)
		return false;
	
	for (int y = 0; y < dst.Height(); ++y)
	{
		int srcy = yfactor == 1 ? y : y >> 1;
		
		UpSampleRow(dst.BytePtr(y), src.BytePtr(srcy), dst.Width(), 
			dst.GetType(), xfactor);
	}

	return true;
}

template <typename T>
bool UpSampleYUY2(vt::CTypedImg<T>& dst, const vt::CTypedImg<T>& src)
{
	if (!dst.IsValid() || !src.IsValid())
	{
		System::Diagnostics::Debug::Assert(false);
		return false;
	}
	
	if (dst.Width() != src.Width() || dst.Height() != src.Height())
		return false;

	if (dst.Bands() != 3 || src.Bands() != 2)
		return false;

	if (dst.Width() % 2 != 0)
		return false;
	
	for (int y = 0; y < dst.Height(); ++y)
	{
		auto dstPtr = dst.Ptr(y);
		auto srcPtr = src.Ptr(y);
		for (int x = 0; x < dst.Width(); x += 2)
		{
			dstPtr[0] = srcPtr[0];
			dstPtr[1] = srcPtr[1];				
			dstPtr[2] = srcPtr[3];
			dstPtr[3] = srcPtr[2];
			dstPtr[4] = srcPtr[1];				
			dstPtr[5] = srcPtr[3];

			dstPtr += 6;
			srcPtr += 4;
		}
	}

	return true;
}

bool UpSampleYUY2(vt::CImg& dst, const vt::CImg& src)
{
	if (EL_FORMAT(src.GetType()) != EL_FORMAT(dst.GetType()))
		return false;

#define SAMPLEYUY2CASE(typeId_, type_) \
	case typeId_: \
	return UpSampleYUY2(static_cast<vt::CTypedImg<type_>&>(dst), \
		static_cast<const vt::CTypedImg<type_>&>(src));
	
	switch (EL_FORMAT(src.GetType()))
	{
	default:
		return false;

		SAMPLEYUY2CASE(EL_FORMAT_BYTE, uint8_t);
		SAMPLEYUY2CASE(EL_FORMAT_SBYTE, int8_t);
		SAMPLEYUY2CASE(EL_FORMAT_SHORT, uint16_t);
		SAMPLEYUY2CASE(EL_FORMAT_SSHORT, int16_t);
		SAMPLEYUY2CASE(EL_FORMAT_INT, int);
		SAMPLEYUY2CASE(EL_FORMAT_FLOAT, float);
		SAMPLEYUY2CASE(EL_FORMAT_DOUBLE, double);
		SAMPLEYUY2CASE(EL_FORMAT_HALF_FLOAT, vt::HALF_FLOAT);
	}
}

void Microsoft::ImageWatch::WatchedImageUserImage::DoReloadPixels(
	bool% hasPixelsLoaded)
{	
	hasPixelsLoaded = false;

	if (!HasValidInfo())
		return;

	if (planeInfo_ == nullptr)
	{
		System::Diagnostics::Debug::Assert(false);
		return;
	}

	if (planeInfo_->Length == 1)
	{
		if (pixelFormat_ == UserPixelFormat::YUY2)
		{
			vt::CImg yuy2;
			auto pi = planeInfo_[0];
			if (FAILED(yuy2.Create(pi.Width, pi.Height, 
				VT_IMG_MAKE_TYPE(elementFormat_, 2))))
				return;
			vt::CImgReaderWriter<vt::CImg> yuy2rw;
			if (FAILED(yuy2.Share(yuy2rw)))
				return;

			if (!WatchedImageHelpers::LoadRemoteVTCImg(&yuy2rw, 
				pi.PixelAddress, pi.NumStrideBytes, Process))
				return;

			vt::CImg yuy2unpacked;
			if (FAILED(yuy2unpacked.Create(pi.Width, pi.Height, 
				VT_IMG_MAKE_TYPE(elementFormat_, 3))))
				return;
			
			if (!UpSampleYUY2(yuy2unpacked, yuy2))
				return;

			hasPixelsLoaded = 
				SUCCEEDED(GetReaderWriter()->WriteImg(yuy2unpacked));
		}
		else
		{
			hasPixelsLoaded =
				WatchedImageHelpers::LoadRemoteVTCImg(GetReaderWriter(), 
				planeInfo_[0].PixelAddress, planeInfo_[0].NumStrideBytes, 
				Process);
		}
	}
	else
	{
		vt::CImg packed;
		if (FAILED(packed.Create(Width(), Height(), 
			VT_IMG_MAKE_TYPE(elementFormat_, NumBands()))))
			return;

		int planeId = 0;
		vt::CImg plane;
		for (UInt32 i = 0; i < NumPlanes(); ++i)
		{
			auto pi = planeInfo_[i];
			if (FAILED(plane.Create(pi.Width, pi.Height, 
				VT_IMG_MAKE_TYPE(elementFormat_, pi.NumBands))))
				return;

			vt::CImgReaderWriter<vt::CImg> planerw;
			if (FAILED(plane.Share(planerw)))
				return;

			if (!WatchedImageHelpers::LoadRemoteVTCImg(&planerw, 
				pi.PixelAddress, pi.NumStrideBytes, Process))
				return;

			// upsample if necessary
			vt::CImg planeUpsampled;
			if (packed.Width() == plane.Width() && 
				packed.Height() == plane.Height())
			{
				if (FAILED(plane.Share(planeUpsampled)))
					return;
			}
			else
			{
				if (FAILED(planeUpsampled.Create(packed.Width(), 
					packed.Height(), 
					VT_IMG_MAKE_TYPE(elementFormat_, pi.NumBands))))
					return;
			
				if (!UpSampleByUpTo2(planeUpsampled, plane))
					return;
			}

			std::vector<vt::BandIndexType> bi(NumBands(), vt::BandIndexIgnore);
			static_assert(0 == (int)vt::BandIndex0, "");
			for (UInt32 k = 0; k < pi.NumBands; ++k)
			{
				if (planeId + k > bi.size() - 1)
				{
					System::Diagnostics::Debug::Assert(false);				
					return;
				}
				bi[planeId + k] = (vt::BandIndexType)((int)vt::BandIndex0 + k);
			}

			if (FAILED(vt::VtConvertBands(packed, planeUpsampled, 
				packed.Bands(), &bi[0])))
				return;

			planeId += pi.NumBands;
		}

		hasPixelsLoaded = SUCCEEDED(GetReaderWriter()->WriteImg(packed));
	}
}

static WatchedImageUserImage::WatchedImageUserImage()
{
	supportedFormats_ = gcnew Dictionary<UserPixelFormat, 
		UserPixelFormatInfo^>();

	supportedFormats_->Add(UserPixelFormat::LUMA, 
		gcnew UserPixelFormatInfo("VisionTools Default", 1));
	supportedFormats_->Add(UserPixelFormat::UV, 
		gcnew UserPixelFormatInfo("UV", 2));
	supportedFormats_->Add(UserPixelFormat::RG, 
		gcnew UserPixelFormatInfo("Red/Green", 2));
	supportedFormats_->Add(UserPixelFormat::BGR, 
		gcnew UserPixelFormatInfo("VisionTools Default", 3));
	supportedFormats_->Add(UserPixelFormat::BGRA, 
		gcnew UserPixelFormatInfo("VisionTools Default", 4));
	supportedFormats_->Add(UserPixelFormat::RGB, 
		gcnew UserPixelFormatInfo("RGB Flipped", 3));
	supportedFormats_->Add(UserPixelFormat::RGBA, 
		gcnew UserPixelFormatInfo("RGBA Flipped", 4));
	supportedFormats_->Add(UserPixelFormat::YUV, 
		gcnew UserPixelFormatInfo("YUV", 3));
	supportedFormats_->Add(UserPixelFormat::NV12, 
		gcnew UserPixelFormatInfo("YUV", 3));
	supportedFormats_->Add(UserPixelFormat::YV12, 
		gcnew UserPixelFormatInfo("YVU", 3));
	supportedFormats_->Add(UserPixelFormat::IYUV, 
		gcnew UserPixelFormatInfo("YUV", 3));
	supportedFormats_->Add(UserPixelFormat::YUY2, 
		gcnew UserPixelFormatInfo("YUV", 3));
};

bool Microsoft::ImageWatch::WatchedImageUserImage::ParsePixelFormat(
	String^ pfstring, UserPixelFormat% format, UInt32% numBands, 
	String^% cmapname)
{
	numBands = 0;
	cmapname = nullptr;
	format = UserPixelFormat::Generic;
	
	if (pfstring == nullptr)
		return false;

	String^ ns = pfstring->Trim()->ToUpper();

	Int64 nbtmp = 0;
	if (WatchedImageHelpers::TryParseInt64(ns, nbtmp))
	{
		format = UserPixelFormat::Generic;
		cmapname = nullptr;

		if (nbtmp < 0 || nbtmp >  VT_IMG_BANDS_MAX)
			return false;

		numBands = (UInt32)nbtmp;

		return true;
	}
	else
	{
		if (!Enum::TryParse<UserPixelFormat>(ns, format))
			return false;

		if (!supportedFormats_->ContainsKey(format))
			return false;

		cmapname = supportedFormats_[format]->ColorMapName;
		numBands = supportedFormats_[format]->NumBands;
		return true;			
	}
}

cli::array<PlaneInfo>^ 
	Microsoft::ImageWatch::WatchedImageUserImage::MakePlaneInfos(
	UInt32 numPlanes, UInt32 numBands, UserPixelFormat pixelFormat, 
	UInt32 width, UInt32 height, 
	cli::array<UInt64>^ addressArr, cli::array<UInt32>^ strideArr)
{
	// these cases should be handled outside this function, but just in case
	if (addressArr == nullptr || strideArr == nullptr)
		return nullptr;
	if (addressArr->Length < 1)
		return nullptr;
	if (!(addressArr->Length == 1 || addressArr->Length == (int)numPlanes))
		return nullptr;
	if (!(strideArr->Length == 1 || strideArr->Length == (int)numPlanes))
		return nullptr;
	
	// set base width/height
	auto pi = gcnew array<PlaneInfo>(numPlanes);
	for (UInt32 i = 0; i < numPlanes; ++i)
	{
		pi[i].Width = width;
		pi[i].Height = height;
	}		

	// set real width/height, set bands
	if (pixelFormat == UserPixelFormat::NV12)	
	{
		if (numPlanes != 2)
			return nullptr;
		
		pi[0].NumBands = 1;

		pi[1].NumBands = 2;
		pi[1].Width /= 2;
		pi[1].Height /= 2;
	}
	else if (pixelFormat == UserPixelFormat::YV12 || 
		pixelFormat == UserPixelFormat::IYUV)	
	{
		if (numPlanes != 3)
			return nullptr;
		
		pi[0].NumBands = 1;
		
		pi[1].NumBands = 1;
		pi[1].Width /= 2;
		pi[1].Height /= 2;

		pi[2].NumBands = 1;
		pi[2].Width /= 2;
		pi[2].Height /= 2;
	}	
	else if (pixelFormat == UserPixelFormat::YUY2)	
	{
		if (numPlanes != 1)
			return nullptr;
		
		if (pi[0].Width % 2 != 0)
			return nullptr;

		pi[0].NumBands = 3;
	}
	else 
	{
		if (pi->Length == 1) 
		{
			// single plane (includes standard packed formats)
			pi[0].NumBands = numBands;			
		}
		else 
		{	
			// default planar: 1-channel per plane, no subsampling
			for (int i = 0; i < pi->Length; ++i)
				pi[i].NumBands = 1;
		}		
	}

	// set pixel address and stride
	for (UInt32 i = 0; i < numPlanes; ++i)
	{
		if (addressArr->Length == (int)numPlanes)
		{
			pi[i].PixelAddress = addressArr[i];
		}
		else
		{
			if (i == 0)
			{
				pi[i].PixelAddress = addressArr[0];
			}
			else
			{
				auto prev = pi[i - 1];
				pi[i].PixelAddress = prev.PixelAddress 
					+ prev.NumStrideBytes * prev.Height;
			}
		}

		pi[i].NumStrideBytes = strideArr->Length == 1 ? 
			strideArr[0] : strideArr[i];	
	}

	return pi;
}
