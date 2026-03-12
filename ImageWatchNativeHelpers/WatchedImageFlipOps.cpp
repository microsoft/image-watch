#pragma unmanaged
#include "vtcore.h"
#pragma managed

#include "WatchedImageFlipOps.h"

int Microsoft::ImageWatch::WatchedImageFlipHorizOp::Mode::get()
{
	return (int)vt::eFlipModeHorizontal;
}

int Microsoft::ImageWatch::WatchedImageFlipVertOp::Mode::get()
{
	return (int)vt::eFlipModeVertical;
}

int Microsoft::ImageWatch::WatchedImageTransposeOp::Mode::get()
{
	return (int)vt::eFlipModeTranspose;
}

int Microsoft::ImageWatch::WatchedImageRot90Op::Mode::get()
{
	return (int)vt::eFlipModeRotate90;
}

int Microsoft::ImageWatch::WatchedImageRot180Op::Mode::get()
{
	return (int)vt::eFlipModeRotate180;
}

int Microsoft::ImageWatch::WatchedImageRot270Op::Mode::get()
{
	return (int)vt::eFlipModeRotate270;
}
