#pragma once

#include "WatchedImage.h"

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageImage abstract : public WatchedImage
		{
		public:
			void Initialize(String^ objectPointerType, UInt64 objectAddress,
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ inspectionContext,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				Microsoft::VisualStudio::Debugger::DkmProcess^ process);
			
		protected:
			property String^ ObjectPointerType { String^ get(); }
			property UInt64 ObjectAddress { UInt64 get(); }
			property Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ InspectionContext
			{
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ get();
			}
			property Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ Frame
			{
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ get();
			}
			property Microsoft::VisualStudio::Debugger::DkmProcess^ Process 
			{ Microsoft::VisualStudio::Debugger::DkmProcess^ get(); }

		private:
			String^ objectPointerType_;
			UInt64 objectAddress_;
			Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ inspectionContext_;
			Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame_;
			Microsoft::VisualStudio::Debugger::DkmProcess^ process_;
		};
	}
}