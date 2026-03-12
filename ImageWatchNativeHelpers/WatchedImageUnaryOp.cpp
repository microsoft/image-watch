#include "vtcore.h"
#include "BandSelectTransform.h"

#include "WatchedImageImage.h"
#include "WatchedImageUnaryOp.h"
#include "WatchedImageHelpers.h"

using namespace Microsoft::ImageWatch;

Microsoft::ImageWatch::WatchedImageUnaryOp::WatchedImageUnaryOp()
	: transformNode_(nullptr)
{
	transformNode_ = new vt::CTransformGraphUnaryNode();	
}

Microsoft::ImageWatch::WatchedImageUnaryOp::~WatchedImageUnaryOp()
{
	this->!WatchedImageUnaryOp();
}

Microsoft::ImageWatch::WatchedImageUnaryOp::!WatchedImageUnaryOp()
{
	delete transformNode_;
}

array<Type^>^ Microsoft::ImageWatch::WatchedImageUnaryOp::DoGetArgumentTypes() 
{
	auto niat = GetNonImageArgumentTypes();
	
	if (niat == nullptr)
	{
		auto res = gcnew array<Type^>(1);
		res[0] = WatchedImage::typeid;
		return res;
	}
	else
	{
		auto res = gcnew array<Type^>(1 + niat->Length);
		res[0] = WatchedImage::typeid;
		for (int i = 0; i < niat->Length; ++i)
			res[1 + i] = niat[i];
		return res;
	}
}

WatchedImage^ Microsoft::ImageWatch::WatchedImageUnaryOp::InputImage::get()
{
	if (!CheckArguments())
		return nullptr;

	return dynamic_cast<WatchedImage^>(Args[0]);
}

void Microsoft::ImageWatch::WatchedImageUnaryOp::SetTransform(
	vt::IImageTransform* t)
{
	if (transformNode_ != nullptr)
		transformNode_->SetTransform(t);
}

Microsoft::ImageWatch::WatchedImageInfo^ 
	Microsoft::ImageWatch::WatchedImageUnaryOp::MakeInfo()
{	
	if (!CheckArguments())
		return nullptr;
	
	auto srcInfo = InputImage->GetInfo();

	auto info = gcnew WatchedImageInfo();

	info->IsInitialized = srcInfo->IsInitialized;

	info->NumBytesPerPixel = 0;
	info->NumStrideBytes = 0;
	info->PixelAddress = 0;
	info->PixelFormat = nullptr;

	info->HasNativePixelValues = true;
	info->HasNativePixelAddress = false;

	int w = 0;
	int h = 0;
	int vt = 0;
	if (!GetOpViewFormat(w, h, vt))
		return nullptr;
	
	info->Width = w;
	info->Height = h;	
	info->ViewerPixelFormat = vt;

	return info;
}

void Microsoft::ImageWatch::WatchedImageUnaryOp::DoReloadInfo(
	Microsoft::ImageWatch::WatchedImageInfo^% info)
{
	info = nullptr;

	if (!CheckArguments())
		return;

	InputImage->ReloadInfo();

	if (!InputImage->HasValidInfo() || !AreNonImageArgsValid())
		return;

	info = MakeInfo();
}

void Microsoft::ImageWatch::WatchedImageUnaryOp::DoReloadPixels(
	bool% hasPixelsLoaded)
{	
	hasPixelsLoaded = false;

	if (!HasValidInfo())
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

vt::CTransformGraphNode* Microsoft::ImageWatch::WatchedImageUnaryOp
	::GetTransformGraphHead()
{
	if (!CheckArguments() || !InputImage->HasValidInfo())
		return nullptr;

	if (WatchedImageOperator::typeid->IsAssignableFrom(InputImage->GetType()))
	{
		auto src = dynamic_cast<WatchedImageOperator^>(InputImage)
			->GetTransformGraphHead();
		if (src == nullptr)
			return nullptr;

		if (FAILED(transformNode_->BindSourceToTransform(src)))
			return nullptr;
	}
	else
	{
#ifdef _DEBUG
		System::Diagnostics::Debug::WriteLine(
			"operator with direct image input: loading input pixels");
#endif
		// NOTE: must reload manually, because we cannot read input pixels 
		// from the debuggee's memory in parallel using the transform 
		// framework (reading is not thread safe)
		InputImage->ReloadPixels();

		if (!InputImage->HasPixelsLoaded())
			return nullptr;

		auto img = InputImage->GetReaderWriter();
		if (img == nullptr)
			return nullptr;

		vt::IMAGE_EXTEND ex(vt::Extend);
		if (FAILED(transformNode_->BindSourceToReader(img, &ex)))
			return nullptr;
	}
	
	if (!InitializeTransform())
		return nullptr;

	return transformNode_;
}
