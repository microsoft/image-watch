#include "vtcore.h"
#include "vtfileio.h"
#include "vthdpio.h"

#include <vcclr.h>

#include "WatchedImageImage.h"
#include "WatchedImageFileOp.h"
#include "WatchedImageHelpers.h"

using namespace Microsoft::ImageWatch;
using namespace System::IO;

Microsoft::ImageWatch::WatchedImageFileOp::WatchedImageFileOp()
	: image_(nullptr), irw_(nullptr), transformNode_(nullptr), 
	transform_(nullptr)
{
	image_ = new vt::CImg();
	irw_ = new vt::CImgReaderWriter<vt::CImg>();
	
	transformNode_ = new vt::CTransformGraphUnaryNode();		
	
	lastWriteTime_ = DateTime::MinValue;
	lastFileName_ = nullptr;
}

Microsoft::ImageWatch::WatchedImageFileOp::~WatchedImageFileOp()
{
	this->!WatchedImageFileOp();
}

Microsoft::ImageWatch::WatchedImageFileOp::!WatchedImageFileOp()
{
	delete transformNode_;

	delete irw_;
	delete image_;
}

array<Type^>^ Microsoft::ImageWatch::WatchedImageFileOp::DoGetArgumentTypes() 
{
	auto res = gcnew array<Type^>(1);
	res[0] = String::typeid;
	return res;
}

String^ Microsoft::ImageWatch::WatchedImageFileOp::Filename::get()
{
	if (!CheckArguments())
		return nullptr;

	return dynamic_cast<String^>(Args[0]);
}

Microsoft::ImageWatch::WatchedImageInfo^ 
	Microsoft::ImageWatch::WatchedImageFileOp::MakeInfo()
{	
	if (!CheckArguments())
		return nullptr;
	
	auto nativeInfo = image_->GetImgInfo();
	auto info = gcnew WatchedImageInfo();

	info->IsInitialized = true;

	info->NumBytesPerPixel = 0;
	info->NumStrideBytes = 0;
	info->PixelAddress = 0;
	info->PixelFormat = nullptr;

	info->HasNativePixelValues = true;
	info->HasNativePixelAddress = false;

	info->Width = nativeInfo.width;
	info->Height = nativeInfo.height;
	info->ViewerPixelFormat = nativeInfo.type;

	return info;
}

bool Microsoft::ImageWatch::WatchedImageFileOp::LoadFromFileIfOutOfDate()
{
	if (!File::Exists(Filename))
		return false;

	FileInfo^ fi = gcnew FileInfo(Filename);
	if (fi == nullptr)
		return false;
	
	auto lwt = fi->LastWriteTime;

	if (lwt > lastWriteTime_ || lastFileName_ != Filename)
	{
#ifdef _DEBUG
		System::Diagnostics::Debug::WriteLine("loading @file from disk");
#endif

		// todo: optimize this for vti images? (read directly into cache)
		auto ext = Path::GetExtension(Filename)->ToLower();
		pin_ptr<const wchar_t> wch = PtrToStringChars(Filename);
		
		System::Diagnostics::Debug::Assert(image_ != nullptr);

		if (ext == ".vti" || ext == ".bin")
		{
			if (FAILED((*image_).Load(wch)))
				return false;
		}
		else if (ext == ".jxr")
		{
			if (FAILED(vt::VtLoadHDPhoto(wch, *image_, false)))
				return false;
		}
		else
		{
			if (FAILED(vt::VtLoadImage(wch, *image_, false)))
				return false;
		}

		if (FAILED(image_->Share(*irw_)))
			return false;
	}

	lastFileName_ = Filename;
	lastWriteTime_ = lwt;

	return true;
}

void Microsoft::ImageWatch::WatchedImageFileOp::DoReloadInfo(
	Microsoft::ImageWatch::WatchedImageInfo^% info)
{
	info = nullptr;

	if (!CheckArguments())
		return;

	if (!LoadFromFileIfOutOfDate())
		return;

	info = MakeInfo();
}

void Microsoft::ImageWatch::WatchedImageFileOp::DoReloadPixels(
	bool% hasPixelsLoaded)
{	
	hasPixelsLoaded = false;

	if (!CheckArguments() || !HasValidInfo())
		return;

	auto head = GetTransformGraphHead();
	if (head == nullptr)
		return;

	auto dst = GetReaderWriter();
	if (dst == nullptr)
		return;
		
	head->SetDest(dst);
	
	if (FAILED(WatchedImageHelpers::StartVTTaskManager()))
		return;

	hasPixelsLoaded = SUCCEEDED(vt::PushTransformTaskAndWait(head, 
		(vt::CTaskProgress*)NULL));
}

vt::CTransformGraphNode* Microsoft::ImageWatch::WatchedImageFileOp
	::GetTransformGraphHead()
{
	if (!CheckArguments() || !HasValidInfo())
		return nullptr;

	LoadFromFileIfOutOfDate();
	
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
