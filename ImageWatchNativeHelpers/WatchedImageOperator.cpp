#include "WatchedImageOperator.h"

using namespace Microsoft::ImageWatch;
using namespace Microsoft::Research::NativeImage;

Microsoft::ImageWatch::WatchedImageOperator::~WatchedImageOperator()
{
	this->!WatchedImageOperator();
}

Microsoft::ImageWatch::WatchedImageOperator::!WatchedImageOperator()
{
	if (args_ == nullptr)
		return;

	// this class owns all its child images. dispose them here so that
	// derived classes dont have to think about it
	for (int i = 0; i < args_->Length; ++i)
	{
		auto wi = dynamic_cast<WatchedImage^>(args_[i]);
		if (wi != nullptr && !wi->IsDisposed)
			wi->~WatchedImage();		
	}
}

void Microsoft::ImageWatch::WatchedImageOperator::
	Initialize(array<Object^>^ args,
	Microsoft::VisualStudio::Debugger::DkmProcess^ process)
{
	InvalidateInfo();

	args_ = args;
	process_ = process;
}		

bool Microsoft::ImageWatch::WatchedImageOperator::CheckArguments()
{
	auto at = DoGetArgumentTypes();
	
	if (at == nullptr && Args == nullptr)
		return true;

	if (at == nullptr || Args == nullptr)
		return false;

	if (at->Length != Args->Length)
		return false;

	for (int i=0; i<at->Length; ++i)
	{
		if (at[i] == nullptr || Args[i] == nullptr)
			return false;

		if (at[i]->GetType()->IsAssignableFrom(Args[i]->GetType()))
			return false;
	}

	return true;
}

array<Object^>^ Microsoft::ImageWatch::WatchedImageOperator::Args::get()
{
	return args_;
}

array<Type^>^ Microsoft::ImageWatch::WatchedImageOperator::GetArgumentTypes()
{
	return DoGetArgumentTypes();
}

Microsoft::VisualStudio::Debugger::DkmProcess^ 
	Microsoft::ImageWatch::WatchedImageOperator::Process::get()
{
	return process_;
}


