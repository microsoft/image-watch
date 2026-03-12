#include "stdafx.h"

#include <memory>
#include "NativeImageHelpers.h"
#include "LocalNativeImage.h"

using namespace System;

BEGIN_NI_NAMESPACE;

HRESULT LocalNativeImage::CreateCacheFromImageFile(const wchar_t* imagePath)
{
	VT_HR_BEGIN();

	try
	{
		SetImage(new vt::CImgInCache());
	}
	catch (const std::bad_alloc&)
	{
		throw gcnew OutOfMemoryException();
	}

	vt::IMG_CACHE_SOURCE_PROPERTIES props;
	props.bAutoLevelGenerate = false;

	if (imagePath_->ToLower()->EndsWith(".jxr"))
	{
		vt::CImg img;
		VT_HR_EXIT(vt::VtLoadHDPhoto(imagePath, img));
		VT_HR_EXIT(static_cast<vt::CImgInCache*>(GetImage())
			->Create(img.GetImgInfo(), &props));
		VT_HR_EXIT(GetImage()->WriteImg(img));
	}
	else
	{
#if 0   // TODO: remove this old code, which directly call the CWicReader?
		vt::CWicReader file;
		VT_HR_EXIT(file.OpenFile(imagePath));
		VT_HR_EXIT(static_cast<vt::CImgInCache*>(GetImage())
			->Create(file.GetImgInfo(), &props));
		VT_HR_EXIT(file.GetImage(GetImage()));
#else
        // Rick's new code (7/9/2105) that call VtLoadImage for additional PPM support
        // TODO: once the old WIC code above is removed, we can move the redundant calls below
        //  out of the if statement and just select between VtLoadImage or VtLoadHDPhoto based on the extension
        vt::CImg img;
        VT_HR_EXIT(vt::VtLoadImage(imagePath, img));
        VT_HR_EXIT(static_cast<vt::CImgInCache*>(GetImage())
            ->Create(img.GetImgInfo(), &props));
        VT_HR_EXIT(GetImage()->WriteImg(img));
#endif
	}
	
	VT_HR_EXIT(BuildMipMap(0));

	VT_HR_END(;);
}

HRESULT LocalNativeImage::CreateCacheFromBlob(const wchar_t* blobPath)
{
	VT_HR_BEGIN();

	try
	{
		vtiBaseCache_.Reset(new vt::CImgInCache());
	}
	catch (const std::bad_alloc&)
	{
		throw gcnew OutOfMemoryException();
	}		

	VT_HR_EXIT(static_cast<vt::CImgInCache*>(vtiBaseCache_.Get())
		->Create(blobPath, true));

	vt::IMG_CACHE_SOURCE_PROPERTIES vtiProps;
	vtiBaseCache_->GetSourceProperties(vtiProps);
	VT_ASSERT(vtiProps.iMaxLevel != -1);

	int numRequiredLevels = vt::ComputePyramidLevelCount(
		vtiBaseCache_->GetImgInfo().width, vtiBaseCache_->GetImgInfo().height);

	if (vtiProps.iMaxLevel >= numRequiredLevels - 1)
	{
		SetImage(vtiBaseCache_.Get());
		vtiBaseCache_.ReleaseNoDelete();
	}
	else
	{
		try
		{
			mipMapCache_.Reset(new vt::CImgInCache());
			SetImage(new vt::CLevelMapReaderWriter());	
		}
		catch (const std::bad_alloc&)
		{
			throw gcnew OutOfMemoryException();
		}	

		vt::CLayerImgInfo vtiLastLevelInfo = 
			vtiBaseCache_->GetImgInfo(vtiProps.iMaxLevel);

		vt::CLayerImgInfo firstMipMapInfo = vtiLastLevelInfo;
		vt::CRect firstMipMapRect = 
			vt::LayerRectAtLevel(vtiLastLevelInfo.LayerRect(), 1);
		firstMipMapInfo.origin = firstMipMapRect.TopLeft();
		firstMipMapInfo.compositeWidth = firstMipMapInfo.width = 
			firstMipMapRect.Width();
		firstMipMapInfo.compositeHeight = firstMipMapInfo.height = 
			firstMipMapRect.Height();

		vt::IMG_CACHE_SOURCE_PROPERTIES props;
		props.bAutoLevelGenerate = false;
		VT_HR_EXIT(static_cast<vt::CImgInCache*>(mipMapCache_.Get())
			->Create(firstMipMapInfo, &props));

		for (int i=0; i<numRequiredLevels; ++i)
		{
			if (i <= vtiProps.iMaxLevel)
			{
				VT_HR_EXIT(
					static_cast<vt::CLevelMapReaderWriter*>(GetImage())
					->SetLevel(vtiBaseCache_.Get(), i, i));
			}
			else 
			{
				VT_HR_EXIT(
					static_cast<vt::CLevelMapReaderWriter*>(GetImage())
					->SetLevel(mipMapCache_.Get(), i - (vtiProps.iMaxLevel + 1), 
					i));
			}
		}

		VT_HR_EXIT(BuildMipMap(vtiProps.iMaxLevel));
	}

	VT_HR_END(;);
}

bool LocalNativeImage::Reload()
{
	ClearMinMaxValue();

	VT_HR_BEGIN();

	msclr::interop::marshal_context mc;	
	const wchar_t* wcpath = mc.marshal_as<const wchar_t*>(imagePath_);

	if (!imagePath_->ToLower()->EndsWith(".vti"))
	{
		VT_HR_EXIT(CreateCacheFromImageFile(wcpath));
	}
	else
	{
		VT_HR_EXIT(CreateCacheFromBlob(wcpath));
	}

	VT_HR_EXIT_LABEL();

	NativeImageHelpers::ThrowIfHRFailed(hr, true);

	return true;
}

LocalNativeImage::LocalNativeImage(String^ imagePath)
	: imagePath_(imagePath)
{	
	Reload();
}

LocalNativeImage::~LocalNativeImage()
{
	this->!LocalNativeImage();
}

LocalNativeImage::!LocalNativeImage()
{
	mipMapCache_.Release();	
	vtiBaseCache_.Release();
}

int LocalNativeMultiFrameImage::NumFrames::get()
{
	return numFrames_;
}

int LocalNativeMultiFrameImage::FrameNumber::get()
{
	return currentFrame_;
}

END_NI_NAMESPACE