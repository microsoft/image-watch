#include "vtcore.h"
#include "vtfileio.h"

#include <vcclr.h>

#include "WatchedImageImage.h"
#include "WatchedImagePtrOp.h"
#include "WatchedImageHelpers.h"
#include "ElFormatStringArg.h"

using namespace Microsoft::ImageWatch;
using namespace System::IO;

Microsoft::ImageWatch::WatchedImagePtrOp::WatchedImagePtrOp()
	: image_(nullptr), irw_(nullptr), transformNode_(nullptr), 
	transform_(nullptr)
{
	image_ = new vt::CImg();
	irw_ = new vt::CImgReaderWriter<vt::CImg>();
	
	transformNode_ = new vt::CTransformGraphUnaryNode();
}

Microsoft::ImageWatch::WatchedImagePtrOp::~WatchedImagePtrOp()
{
	this->!WatchedImagePtrOp();
}

Microsoft::ImageWatch::WatchedImagePtrOp::!WatchedImagePtrOp()
{
	delete transformNode_;

	delete irw_;
	delete image_;
}

array<Type^>^ Microsoft::ImageWatch::WatchedImagePtrOp::DoGetArgumentTypes() 
{
	auto res = gcnew array<Type^>(6);
	res[0] = UInt64::typeid; // addr
	res[1] = ElFormatStringArg::typeid; // eltype
	res[2] = UInt32::typeid; // bands
	res[3] = UInt32::typeid; // width
	res[4] = UInt32::typeid; // height
	res[5] = UInt32::typeid; // stride
	return res;
}

Microsoft::ImageWatch::WatchedImageInfo^ 
	Microsoft::ImageWatch::WatchedImagePtrOp::MakeInfo()
{	
	if (!CheckArguments())
		return nullptr;
	
	auto info = gcnew WatchedImageInfo();

	info->IsInitialized = true;

	info->NumBytesPerPixel = VT_IMG_ELSIZE(vtType_) * VT_IMG_BANDS(vtType_);
	info->NumStrideBytes = stride_;
	info->PixelAddress = addr_;
	info->PixelFormat = nullptr;

	info->HasNativePixelValues = true;
	info->HasNativePixelAddress = true;

	info->Width = width_;
	info->Height = height_;
	info->ViewerPixelFormat = vtType_;

	return info;
}

void Microsoft::ImageWatch::WatchedImagePtrOp::DoReloadInfo(
	Microsoft::ImageWatch::WatchedImageInfo^% info)
{
	info = nullptr;

	if (!CheckArguments())
		return;

	addr_ = (UInt64)Args[0];
	ElFormatStringArg^ elType = (ElFormatStringArg^)Args[1];
	UInt32 numBands = (UInt32)Args[2];
	vtType_ = VT_IMG_MAKE_TYPE(elType->VtElType, numBands);
	width_ = (UInt32)Args[3];
	height_ = (UInt32)Args[4];
	stride_ = (UInt32)Args[5];

	if (!WatchedImageHelpers::ValidateVTCImgInfo(width_, height_, vtType_,
		stride_, addr_))
		return;

	if (FAILED(image_->Create(width_, height_, vtType_)))
		return;

	if (FAILED(image_->Share(*irw_)))
		return;

	info = MakeInfo();
}

void Microsoft::ImageWatch::WatchedImagePtrOp::DoReloadPixels(
	bool% hasPixelsLoaded)
{	
	hasPixelsLoaded = false;

	if (!CheckArguments() || !HasValidInfo())
		return;

	if (!WatchedImageHelpers::LoadRemoteVTCImg(
		irw_, addr_, stride_, Process))
		return;

	auto dst = GetReaderWriter();
	if (dst == nullptr)
		return;

	hasPixelsLoaded = SUCCEEDED(dst->WriteImg(*image_));
}

vt::CTransformGraphNode* Microsoft::ImageWatch::WatchedImagePtrOp
	::GetTransformGraphHead()
{
	if (!CheckArguments() || !HasValidInfo())
		return nullptr;

	if (!WatchedImageHelpers::LoadRemoteVTCImg(irw_,
		addr_, stride_, Process))
		return nullptr;
	
	vt::IMAGE_EXTEND ex(vt::Extend);
	if (FAILED(transformNode_->BindSourceToReader(irw_, &ex)))
		return nullptr;

	delete transform_;
	transform_ = nullptr;
	transform_ = new vt::CConvertTransform(
		VT_IMG_FIXED(irw_->GetImgInfo().type));
	
	transformNode_->SetTransform(transform_);

	return transformNode_;
}
