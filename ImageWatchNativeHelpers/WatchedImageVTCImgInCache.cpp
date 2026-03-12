#include "vtcore.h"
#include "WatchedImageVTCImgInCache.h"
#include "WatchedImageHelpers.h"

using namespace Microsoft::ImageWatch;

Microsoft::ImageWatch::WatchedImageInfo^ 
	Microsoft::ImageWatch::WatchedImageVTCImgInCache::MakeInfo()
{
	auto info = gcnew WatchedImageInfo();

	info->IsInitialized = type_ != OBJ_UNDEFINED;

	info->Width = width_;
	info->Height = height_;
	
	info->NumBytesPerPixel = info->IsInitialized ? 
		VT_IMG_ELSIZE(type_) * VT_IMG_BANDS(type_) : 0;

	auto pf = gcnew String(vt::VtPixFormatStringFromType(type_, true));
	info->PixelFormat = !info->IsInitialized || pf == "NONE" ? 
		 nullptr : pf;

	info->ViewerPixelFormat = type_;

	info->HasNativePixelValues = true;
	info->HasNativePixelAddress = false;

	return info;
}

void Microsoft::ImageWatch::WatchedImageVTCImgInCache::DoReloadInfo(
	Microsoft::ImageWatch::WatchedImageInfo^% info)
{
	info = nullptr;

    int width  = width_;
    int height = height_;
    int type   = type_;
    UInt32 blockSizeX    = blockSizeX_;
    UInt32 blockSizeY    = blockSizeY_;
    UInt64 blocksAddress = blocksAddress_;

	if (!WatchedImageHelpers::LoadRemoteVTCImgInCacheInfo(width_, height_, 
		type_, blockSizeX_, blockSizeY_, blocksAddress_, blocksSize_,
        ObjectAddress, InspectionContext, Frame))
		return;

	if (!WatchedImageHelpers::ValidateVTCImgInCacheInfo(
		type_, blockSizeX_, blockSizeY_, blocksAddress_, blocksSize_))
		return;

    // Has debuggee VTCImgInCache changed?
    if (width != width_ || height != height_ || type != type_ ||
        blockSizeX != blockSizeX_ || blockSizeY != blockSizeY_ ||
        blocksAddress != blocksAddress_)
    {
        delete[] uniqueness_;
        UInt32 blocksX = (width_  + blockSizeX_ - 1) / blockSizeX_;
        UInt32 blocksY = (height_ + blockSizeY_ - 1) / blockSizeY_;
        if ((uniqueness_ = VT_NOTHROWNEW UInt32[blocksX * blocksY]) == NULL)
            return;
        ZeroMemory(uniqueness_, blocksX * blocksY * sizeof(UInt32));
    }

	info = MakeInfo();
}

void Microsoft::ImageWatch::WatchedImageVTCImgInCache::DoReloadPixels(
	bool% hasPixelsLoaded)
{	
	hasPixelsLoaded = false;

	if (!HasValidInfo())
		return;

	hasPixelsLoaded = WatchedImageHelpers::LoadRemoteVTCImgInCache(
		GetReaderWriter(), blockSizeX_, blockSizeY_,
        blocksAddress_, blocksSize_, uniqueness_, Process);
}

