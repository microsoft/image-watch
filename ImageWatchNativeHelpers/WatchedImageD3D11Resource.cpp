#include "vtcore.h"
#include "WatchedImageD3D11Resource.h"
#include "WatchedImageHelpers.h"

using namespace Microsoft::ImageWatch;
using namespace System::Text::RegularExpressions;

Microsoft::ImageWatch::WatchedImageInfo^ 
	Microsoft::ImageWatch::WatchedImageD3D11Resource::MakeInfo()
{
	auto info = gcnew WatchedImageInfo();

	info->IsInitialized = true;

	info->Width = width_;
	info->Height = height_;
	
	info->ViewerPixelFormat = vtType_;
	info->PixelFormat = pixelFormatString_;
	info->SpecialColorMapName = specialColorMapName_;

	info->HasNativePixelValues = true;
	
	info->HasNativePixelAddress = false;
	info->NumBytesPerPixel = 0;
	
	return info;
}

bool Microsoft::ImageWatch::WatchedImageD3D11Resource::
	DXToVTFormat(UInt32 dxFormat, int% vtFormat, String^% fmtString, 
	String^% clrMapName)
{
#define DXFORMATCASE(dxfmt_, vtfmt_, fmtstring_, clrmap_) \
	case (dxfmt_): vtFormat = (vtfmt_); fmtString = (fmtstring_); \
	clrMapName = (clrmap_); return true;

	switch (dxFormat)
	{
		DXFORMATCASE(DXGI_FORMAT_B8G8R8A8_UNORM, OBJ_RGBAIMG, "BGRA", "");
		DXFORMATCASE(DXGI_FORMAT_R8G8B8A8_UNORM, 
			VT_IMG_MAKE_TYPE(EL_FORMAT_BYTE, 4), "RGBA", "RGBA Flipped");
		DXFORMATCASE(DXGI_FORMAT_D32_FLOAT, 
			VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, 1), nullptr, nullptr);
		DXFORMATCASE(DXGI_FORMAT_R32_FLOAT, 
			VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, 1), nullptr, nullptr);

	default:
		vtFormat = VT_IMG_MAKE_TYPE(EL_FORMAT_BYTE, 4);
		fmtString = "???";
		clrMapName = nullptr;
		return true;
		//return false;
	}

#undef DXFORMATCASE
}

void Microsoft::ImageWatch::WatchedImageD3D11Resource::DoReloadInfo(
	Microsoft::ImageWatch::WatchedImageInfo^% info)
{
	info = nullptr;

	bool is64bit = false;
	if (!WatchedImageHelpers::Is64BitProcess(InspectionContext, Frame, is64bit)
		|| is64bit)
		return;

	// TODO: find out which type of resource this is:
	// -> if (ObjectPointerType->Contains("ID3D11Texture2D")) ...
	
	// !!! For now, assume Texture2D !!!
		
	// 1) check if warp device	
	// for CTexture1/2/3D in d3d11.dll
	const unsigned int D3D11CTextureXDToCDevicePtr = 0x10;
	const unsigned int D3D11CDeviceToWarpAPIPtr = 0x5e0;

	UInt64 warpObjAddr = 0;
	if (!WatchedImageHelpers::FollowTwoPointers(InspectionContext, Frame,
		ObjectAddress, D3D11CTextureXDToCDevicePtr, D3D11CDeviceToWarpAPIPtr, 
		warpObjAddr))
		return;

	// not on a warp device?
	if (warpObjAddr == 0)
		return;

	// sizeof(CTexture2D) + sizeof(CTexture2D::CLS)
	const unsigned int D3D11Texture2DToWarpResourceContainer= 0x6c + 0x30; 
	const unsigned int WarpResourceContainerToWarpResource = 0x0; 
	const unsigned int WarpResourceToResourceShape = 0x210;

	const unsigned int ResourceShapeToDataFormatUInt32 = 0x30;
	const unsigned int ResourceShapeToSubResourceList = 0x0;
	const unsigned int SubResourceToBitsPtr = 0x0;
	const unsigned int SubResourceToPitchUInt32 = 0x0c;
	const unsigned int SubResourceToWidthUInt32 = 0x014;
	const unsigned int SubResourceToHeightUInt32 = 0x018;
	unsigned char srbuffer[0x24];

	UInt64 resourceShape;
	if (!WatchedImageHelpers::FollowTwoPointers(InspectionContext, Frame,
		ObjectAddress, D3D11Texture2DToWarpResourceContainer + WarpResourceContainerToWarpResource, 
		WarpResourceToResourceShape, resourceShape))
		return;
	
	UInt32 dataFormat;
	if (!WatchedImageHelpers::EvaluateUInt32(InspectionContext, Frame,
		resourceShape + ResourceShapeToDataFormatUInt32, dataFormat))
		return;

	if (!DXToVTFormat(dataFormat, vtType_, pixelFormatString_, specialColorMapName_))
		return;
	
	UInt64 subResource;
	if (!WatchedImageHelpers::FollowPointer(InspectionContext, Frame,
		resourceShape + ResourceShapeToSubResourceList, 0, subResource))
		return;
		
	if (!WatchedImageHelpers::ReadMemory(Process, subResource, srbuffer, 
		sizeof(srbuffer)))
		return;

	System::Diagnostics::Debug::Assert(!is64bit);
	pixelAddress_ = *(UInt32*)(&srbuffer[SubResourceToBitsPtr]);
	numStrideBytes_ = *(UInt32*)(&srbuffer[SubResourceToPitchUInt32]);
	width_ = *(UInt32*)(&srbuffer[SubResourceToWidthUInt32]);
	height_ = *(UInt32*)(&srbuffer[SubResourceToHeightUInt32]);

	if (!WatchedImageHelpers::ValidateVTCImgInfo(width_, height_, vtType_, 
		numStrideBytes_, pixelAddress_))
		return;

	info = MakeInfo();
}

void Microsoft::ImageWatch::WatchedImageD3D11Resource::DoReloadPixels(
	bool% hasPixelsLoaded)
{	
	hasPixelsLoaded = false;

	if (!HasValidInfo())
		return;

	hasPixelsLoaded = WatchedImageHelpers::LoadRemoteVTCImg(
		GetReaderWriter(), pixelAddress_, numStrideBytes_, Process);
}

