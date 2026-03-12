#include "Stdafx.h"

#include "NativeImageBase.h"
#include "NativeImageHelpers.h"

using namespace System;

BEGIN_NI_NAMESPACE;

#ifdef COUNT_NI_OBJECTS
int g_ni_objects = 0;
#endif

HRESULT ComputeMinMaxElement(vt::IImageReader& reader, int selectedBand,
	float& minVal, float& maxVal);

NativeImageBase::NativeImageBase()
	: minMaxComputed_(false), minElementValue_(0.0), maxElementValue_(1.0),
	imagePtrManagedExternally_(false)
{
	try
	{
		vtrm_.Reset(new vt::CVTResourceManagerInit());		
	}
	catch (const std::bad_alloc&)
	{
		throw gcnew OutOfMemoryException();
	}

#ifdef COUNT_NI_OBJECTS
	System::Diagnostics::Debug::WriteLine("NI: {0}", ++g_ni_objects);	
#endif
}

NativeImageBase::~NativeImageBase()
{
	this->!NativeImageBase();
}

NativeImageBase::!NativeImageBase()
{
	if (imagePtrManagedExternally_)
	{
		image_.ReleaseNoDelete();
		imagePtrManagedExternally_ = false;
	}
	image_.Release();
	vtrm_.Release();

#ifdef COUNT_NI_OBJECTS
	System::Diagnostics::Debug::WriteLine("NI: {0}", --g_ni_objects);	
#endif
}

void NativeImageBase::SetImage(vt::IImageReaderWriter* image)
{
	if (imagePtrManagedExternally_)
	{
		image_.ReleaseNoDelete();
		imagePtrManagedExternally_ = false;
	}
  	image_.Reset(image);
}

void NativeImageBase::SetExternallyManagedImage(vt::IImageReaderWriter* image)
{
	SetImage(image);
	// set flag to avoid deleting Release on this image pointer
	imagePtrManagedExternally_ = true;
}

vt::IImageReaderWriter* NativeImageBase::GetImage()
{
	if (image_.IsNull())
		return nullptr;

	return image_.Get();
}

void NativeImageBase::SetImageIntPtr(IntPtr img)
{
	SetImage(reinterpret_cast<vt::IImageReaderWriter*>(img.ToPointer()));
}

IntPtr NativeImageBase::GetImageIntPtr(bool throwIfNull)
{
	if (!throwIfNull && image_.IsNull())
		return IntPtr(nullptr);
		
	return IntPtr(GetImage());
}

HRESULT NativeImageBase::BuildMipMap(int baseLevel)
{
	if (image_.IsNull())
		return E_NOINIT;

	VT_HR_BEGIN();

	int maxLevel = 
		vt::ComputePyramidLevelCount(image_->GetImgInfo().width,
		image_->GetImgInfo().height) - 1;

	if (maxLevel > baseLevel)
	{
		vt::C14641Kernel kernel;		
		VT_HR_EXIT(vt::GeneratePyramid(image_.Get(), baseLevel + 1, maxLevel, 
			kernel.AsKernel()));
	}

	VT_HR_END(;);
}

Int32Rect NativeImageBase::Rectangle::get()
{	
	if (image_.IsNull())
		return Int32Rect::Empty;

	const vt::CRect r = image_->GetImgInfo().LayerRect();

	return Int32Rect(r.left, r.top, r.Width(), r.Height());
}

IntPtr NativeImageBase::IImageReader::get()
{ 
	if (image_.IsNull())
		return IntPtr::Zero;

	return IntPtr(static_cast<vt::IImageReader*>(image_.Get()));
}

NativePixelFormat^ NativeImageBase::PixelFormat::get()
{
	if (image_.IsNull())
		return nullptr;

	auto info = image_->GetImgInfo();

	NativePixelFormat^ res = gcnew NativePixelFormat();

	res->ElementFormat = gcnew String(vt::VtElFormatStringFromType(info.type));
	res->NumBytesPerElement = info.ElSize();
	res->NumBands = info.Bands();
	res->PixelFormat = 
		gcnew String(vt::VtPixFormatStringFromType(info.type, true));

	return res;
}

bool NativeImageBase::HasMinMaxElementValue::get()
{
	return minMaxComputed_;
}

void NativeImageBase::ComputeMinMaxElementValue(int selectedBand)
{
	if (image_.IsNull())
		return;

	VT_HR_BEGIN();

	float mn, mx;
	VT_HR_EXIT(ComputeMinMaxElement(*image_.Get(), selectedBand, mn, mx));
	minElementValue_ = mn;
	maxElementValue_ = mx;

	minMaxComputed_ = true;

	VT_HR_EXIT_LABEL();

	NativeImageHelpers::ThrowIfHRFailed(hr);
}

void NativeImageBase::ClearMinMaxValue()
{
	minElementValue_ = 0.0;
	maxElementValue_ = 1.0;
	minMaxComputed_ = false;
}

double NativeImageBase::MinElementValue::get()
{ 
	if (double::IsNaN(minElementValue_) || 
		double::IsInfinity(minElementValue_))
		return double::MinValue;

	return minElementValue_;
}

double NativeImageBase::MaxElementValue::get()
{ 
	if (double::IsNaN(maxElementValue_) || 
		double::IsInfinity(maxElementValue_))
		return double::MaxValue;

	return maxElementValue_;
}

cli::array<Byte>^ NativeImageBase::ReadPixel(int x, int y)
{
	if (image_.IsNull())
		return nullptr;

	vt::CRect r(image_->GetImgInfo().LayerRect());

	if (x < r.left || 
		y < r.top || 
		x >= r.left + r.Width() || 
		y >= r.top + r.Height())
	{
		throw gcnew ArgumentOutOfRangeException();
	}

	vt::CImg dst;
	// roi is NOT in layer coordinates for ReadRegion();
	vt::CRect roi(x - r.left, y - r.top, x + 1 - r.left, y + 1 - r.top);

	NativeImageHelpers::ThrowIfHRFailed(image_->ReadRegion(roi, dst, 0));

	cli::array<System::Byte>^ res = 
		gcnew cli::array<System::Byte>(dst.PixSize());

	System::Runtime::InteropServices::Marshal::Copy(IntPtr(dst.BytePtr()),
		res, 0, dst.PixSize());

	return res;
}

END_NI_NAMESPACE;