#include "WatchedImageImage.h"

using namespace Microsoft::ImageWatch;
using namespace Microsoft::Research::NativeImage;

// EnvDTE uses default parameters which are unsupported in C++/CLI. We don't 
// use them, so I guess it's fine to disable the warning.
#pragma warning(disable : 4564)

void Microsoft::ImageWatch::WatchedImageImage::Initialize(
	String^ nativeType, UInt64 objectAddress,
	Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ inspectionContext,
	Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
	Microsoft::VisualStudio::Debugger::DkmProcess^ process)
{
	InvalidateInfo();

	objectPointerType_ = nativeType;
	objectAddress_ = objectAddress;
	
	inspectionContext_ = inspectionContext;
	frame_ = frame;
	process_ = process;
}

String^ Microsoft::ImageWatch::WatchedImageImage::ObjectPointerType::get()
{
	return objectPointerType_;
}

UInt64 Microsoft::ImageWatch::WatchedImageImage::ObjectAddress::get()
{
	return objectAddress_;
}

Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^
	Microsoft::ImageWatch::WatchedImageImage::InspectionContext::get()
{
	return inspectionContext_;
}

Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^
	Microsoft::ImageWatch::WatchedImageImage::Frame::get()
{
	return frame_;
}

Microsoft::VisualStudio::Debugger::DkmProcess^ 
	Microsoft::ImageWatch::WatchedImageImage::Process::get()
{
	return process_;
}
