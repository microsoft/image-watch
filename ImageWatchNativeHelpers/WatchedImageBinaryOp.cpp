#include "vtcore.h"
#include "BandSelectTransform.h"

#include "WatchedImageImage.h"
#include "WatchedImageBinaryOp.h"
#include "WatchedImageHelpers.h"

using namespace Microsoft::ImageWatch;

Microsoft::ImageWatch::WatchedImageBinaryOp::WatchedImageBinaryOp()
	: transformNode_(nullptr)
{
	transformNode_ = new vt::CTransformGraphNaryNode();	
	transformNode_->SetSourceCount(2);
}

Microsoft::ImageWatch::WatchedImageBinaryOp::~WatchedImageBinaryOp()
{
	this->!WatchedImageBinaryOp();
}

Microsoft::ImageWatch::WatchedImageBinaryOp::!WatchedImageBinaryOp()
{
	delete transformNode_;
}

array<Type^>^ Microsoft::ImageWatch::WatchedImageBinaryOp
	::DoGetArgumentTypes() 
{
	auto niat = GetNonImageArgumentTypes();
	
	if (niat == nullptr)
	{
		auto res = gcnew array<Type^>(2);
		res[0] = WatchedImage::typeid;
		res[1] = WatchedImage::typeid;
		return res;
	}
	else
	{
		auto res = gcnew array<Type^>(2 + niat->Length);
		res[0] = WatchedImage::typeid;
		res[1] = WatchedImage::typeid;
		for (int i = 0; i < niat->Length; ++i)
			res[2 + i] = niat[i];
		return res;
	}
}

WatchedImage^ Microsoft::ImageWatch::WatchedImageBinaryOp
	::FirstInputImage::get()
{
	if (!CheckArguments())
		return nullptr;

	return dynamic_cast<WatchedImage^>(Args[0]);
}

WatchedImage^ Microsoft::ImageWatch::WatchedImageBinaryOp
	::SecondInputImage::get()
{
	if (!CheckArguments())
		return nullptr;

	return dynamic_cast<WatchedImage^>(Args[1]);
}

void Microsoft::ImageWatch::WatchedImageBinaryOp::SetTransform(
	vt::IImageTransform* t)
{
	if (transformNode_ != nullptr)
		transformNode_->SetTransform(t);
}

Microsoft::ImageWatch::WatchedImageInfo^ 
	Microsoft::ImageWatch::WatchedImageBinaryOp::MakeInfo()
{	
	if (!CheckArguments())
		return nullptr;
	
	auto srcInfo1 = FirstInputImage->GetInfo();
	auto srcInfo2 = SecondInputImage->GetInfo();

	auto info = gcnew WatchedImageInfo();

	info->IsInitialized = srcInfo1->IsInitialized && srcInfo2->IsInitialized;

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

void Microsoft::ImageWatch::WatchedImageBinaryOp::DoReloadInfo(
	Microsoft::ImageWatch::WatchedImageInfo^% info)
{
	info = nullptr;

	if (!CheckArguments())
		return;

	FirstInputImage->ReloadInfo();
	SecondInputImage->ReloadInfo();

	if (!FirstInputImage->HasValidInfo() || 
		!SecondInputImage->HasValidInfo() || 
		!AreImageArgsValid() || !AreNonImageArgsValid())
		return;

	info = MakeInfo();
}

void Microsoft::ImageWatch::WatchedImageBinaryOp::DoReloadPixels(
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

vt::CTransformGraphNode* Microsoft::ImageWatch::WatchedImageBinaryOp
	::GetTransformGraphHead()
{
	if (!CheckArguments() || !FirstInputImage->HasValidInfo() ||
		!SecondInputImage->HasValidInfo())
		return nullptr;

	auto inputs = gcnew array<WatchedImage^>(2);
	inputs[0] = FirstInputImage;
	inputs[1] = SecondInputImage;

	for (int i = 0; i < 2; ++i)
	{
		if (WatchedImageOperator::typeid->IsAssignableFrom(
			inputs[i]->GetType()))
		{
			auto src = dynamic_cast<WatchedImageOperator^>(inputs[i])
				->GetTransformGraphHead();
			if (src == nullptr)
				return nullptr;

			if (FAILED(transformNode_->BindSourceToTransform(i, src)))
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
			inputs[i]->ReloadPixels();

			if (!inputs[i]->HasPixelsLoaded())
				return nullptr;

			auto img = inputs[i]->GetReaderWriter();
			if (img == nullptr)
				return nullptr;

			vt::IMAGE_EXTEND ex(vt::Extend);
			if (FAILED(transformNode_->BindSourceToReader(i, img, &ex)))
				return nullptr;
		}
	}
	
	if (!InitializeTransform())
		return nullptr;

	return transformNode_;
}
