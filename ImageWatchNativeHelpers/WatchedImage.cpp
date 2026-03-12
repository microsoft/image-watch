#include "vtcore.h"
#include "WatchedImage.h"
#include "WatchedImageHelpers.h"

using namespace Microsoft::ImageWatch;
using namespace Microsoft::Research::NativeImage;

Microsoft::ImageWatch::WatchedImageInfo::WatchedImageInfo()
	: IsInitialized(false), 
	Width(0), Height(0), PixelFormat(nullptr),
	ViewerPixelFormat(0), 	
	HasNativePixelValues(false),
	HasNativePixelAddress(false), PixelAddress(0),
	NumBytesPerPixel(0), NumStrideBytes(0),
	HasCustomValueRange(false), 
	CustomValueRangeStart(0), CustomValueRangeEnd(0),
	SpecialColorMapName(nullptr)
{
}

UInt32 Microsoft::ImageWatch::WatchedImageInfo::NumBands::get()
{
	return VT_IMG_BANDS(ViewerPixelFormat);
}
		
String^ Microsoft::ImageWatch::WatchedImageInfo::ElementFormat::get()
{
	return WatchedImageHelpers::MakeNeutralElFormatString(ViewerPixelFormat);
}

Microsoft::ImageWatch::WatchedImage::WatchedImage()
	: hasPixelsLoaded_(false)
{
}

void Microsoft::ImageWatch::WatchedImage::InvalidateInfo()
{
	info_ = nullptr;
	InvalidatePixels();
}

void Microsoft::ImageWatch::WatchedImage::ValidateInfo()
{
	if (info_ == nullptr)
		return;
	
	if (!info_->IsInitialized)
	{
		// not initialized? OK. But to be safe, construct new one to make sure 
		// all fields have default values now ..
		info_ = gcnew WatchedImageInfo();
		return;
	}
	
#define ENFORCE(expr) if (!(expr)) { info_ = nullptr; \
	return; }

	ENFORCE(info_->Width <= MAX_IMAGE_WIDTH);
	ENFORCE(info_->Height <= MAX_IMAGE_HEIGHT);
	
	const auto bands = VT_IMG_BANDS(info_->ViewerPixelFormat);	
	ENFORCE(bands <= VT_IMG_BANDS_MAX);
	ENFORCE(bands == info_->NumBands);
	
	const auto viewFmt = EL_FORMAT(info_->ViewerPixelFormat);
	ENFORCE(viewFmt == EL_FORMAT_BYTE ||
		viewFmt == EL_FORMAT_SBYTE ||
		viewFmt == EL_FORMAT_SHORT ||
		viewFmt == EL_FORMAT_SSHORT ||
		viewFmt == EL_FORMAT_HALF_FLOAT ||
		viewFmt == EL_FORMAT_FLOAT ||
		viewFmt == EL_FORMAT_INT ||
		viewFmt == EL_FORMAT_DOUBLE);

	// TODO: validate colormap here?

	if (info_->HasNativePixelAddress)
	{
		if (info_->Width * info_->Height * info_->NumBands > 0)
			ENFORCE(info_->PixelAddress != 0);

		ENFORCE(info_->NumBytesPerPixel > 0);
		ENFORCE(info_->NumStrideBytes >= info_->Width * 
			info_->NumBytesPerPixel);
	}

#undef ENFORCE
}

void Microsoft::ImageWatch::WatchedImage::InvalidatePixels()
{	
	hasPixelsLoaded_ = false;
}

bool Microsoft::ImageWatch::WatchedImage::HasValidInfo()
{
	return info_ != nullptr;
}

bool Microsoft::ImageWatch::WatchedImage::HasPixelsLoaded()
{
	return HasValidInfo() && hasPixelsLoaded_;
}

bool Microsoft::ImageWatch::WatchedImage::GetViewFormat(
	int% width, int% height, int% vttype)
{
	if (!HasValidInfo())
		return false;

	width = info_->Width;
	height = info_->Height;
	vttype = info_->ViewerPixelFormat;

	return true;
}

vt::IImageReaderWriter* 
	Microsoft::ImageWatch::WatchedImage::GetReaderWriter()
{
	if (!HasValidInfo())
		return nullptr;

	vt::IImageReaderWriter* img = 
		reinterpret_cast<vt::IImageReaderWriter*>(
		GetImageIntPtr(false).ToPointer());

	if (img == nullptr)
	{
		try
		{
			// creating CImgInCache object in NativeImage.dll because 
			// that's where it will be destroyed (~NativeImageBase())
			auto newImgInCache = NativeImageHelpers::CreateCImgInCache();
			if (newImgInCache.ToPointer() == nullptr)
				return nullptr;
			SetImageIntPtr(newImgInCache);
			img = reinterpret_cast<vt::IImageReaderWriter*>(
				GetImageIntPtr(true).ToPointer());
		}
		catch (...)
		{
			return nullptr;
		}
	}
	
	if ((UInt32)img->GetImgInfo().width != info_->Width || 
		(UInt32)img->GetImgInfo().height != info_->Height || 
		(UInt32)img->GetImgInfo().type != info_->ViewerPixelFormat)
	{
		if (FAILED(NativeImageHelpers::CreateCImgInCacheImage(
			IntPtr(img), info_->Width, info_->Height, 
			info_->ViewerPixelFormat)))
		{
			return nullptr;
		}
	}

	return img;
}

void Microsoft::ImageWatch::WatchedImage::ReloadInfo()
{
	InvalidateInfo();

	ClearMinMaxValue();
	
	DoReloadInfo(info_);

	ValidateInfo();
}

Microsoft::ImageWatch::WatchedImageInfo^ 
	Microsoft::ImageWatch::WatchedImage::GetInfo()
{
	return HasValidInfo() ? info_ : nullptr;
}

void Microsoft::ImageWatch::WatchedImage::ReloadPixels()
{
	InvalidatePixels();

	if (!HasValidInfo())
		return;

	DoReloadPixels(hasPixelsLoaded_);

	if (hasPixelsLoaded_)
	{
		if (FAILED(BuildMipMap(0)))
			hasPixelsLoaded_ = false;
	}
}

bool Microsoft::ImageWatch::WatchedImage::Reload()
{
	throw gcnew NotImplementedException();
}
