#include "WatchedImageScaleOp.h"

///////////////////////////////////////////////////////////////////////////////

using namespace Microsoft::ImageWatch;

float Microsoft::ImageWatch::WatchedImageScaleOp::Scale::get()
{
	if (!CheckArguments())
		return 0.f;

	return (float)Args[1];
}

array<Type^>^ Microsoft::ImageWatch::WatchedImageScaleOp::
	GetNonImageArgumentTypes()
{
	auto res = gcnew array<Type^>(1);
	res[0] = float::typeid;
	return res;
}

bool Microsoft::ImageWatch::WatchedImageScaleOp::AreNonImageArgsValid()
{	
	return true;
}

