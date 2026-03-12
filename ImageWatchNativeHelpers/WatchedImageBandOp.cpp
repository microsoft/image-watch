#include "vtcore.h"
#include "BandSelectTransform.h"

#include "WatchedImageBandOp.h"

using namespace Microsoft::ImageWatch;

Microsoft::ImageWatch::WatchedImageBandOp::WatchedImageBandOp()
	: transform_(nullptr)
{
	transform_ = new BandSelectTransform();	

	SetTransform(transform_);
}

Microsoft::ImageWatch::WatchedImageBandOp::~WatchedImageBandOp()
{
	this->!WatchedImageBandOp();
}

Microsoft::ImageWatch::WatchedImageBandOp::!WatchedImageBandOp()
{
	delete transform_;
}

UInt32 Microsoft::ImageWatch::WatchedImageBandOp::BandIndex::get()
{
	if (!CheckArguments())
		return 0;

	return (UInt32)Args[1];
}

array<Type^>^ Microsoft::ImageWatch::WatchedImageBandOp::
	GetNonImageArgumentTypes()
{
	auto res = gcnew array<Type^>(1);
	res[0] = UInt32::typeid;
	return res;
}

bool Microsoft::ImageWatch::WatchedImageBandOp::AreNonImageArgsValid()
{
	if (InputImage == nullptr)
		return false;

	auto src = InputImage->GetReaderWriter();
	if (src == nullptr)
		return false;
	
	return BandIndex < (UInt32)VT_IMG_BANDS(src->GetImgInfo().type);
}

bool Microsoft::ImageWatch::WatchedImageBandOp::InitializeTransform()
{	
	transform_->Initialize(BandIndex, false);

	return true;
}

bool Microsoft::ImageWatch::WatchedImageBandOp::GetOpViewFormat(
	int% width, int%height, int% vttype)
{
	if (InputImage == nullptr || !InputImage->HasValidInfo())
		return false;

	int inputWidth = 0;
	int inputHeight = 0;
	int inputType = 0;
	if (!InputImage->GetViewFormat(inputWidth, inputHeight, inputType))
		return false;

	width = inputWidth;
	height = inputHeight;
	vttype = VT_IMG_MAKE_TYPE(EL_FORMAT(inputType), 1);

	return true;
}
