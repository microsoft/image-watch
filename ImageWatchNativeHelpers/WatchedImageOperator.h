#pragma once

#include "WatchedImage.h"

namespace vt
{
	class CTransformGraphNode;
}

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageOperator abstract : public WatchedImage
		{
		public:
			~WatchedImageOperator();
			!WatchedImageOperator();

		public:
			void Initialize(array<Object^>^ args, 
				Microsoft::VisualStudio::Debugger::DkmProcess^ process);
			
			array<Type^>^ GetArgumentTypes();

		internal:
			virtual vt::CTransformGraphNode* GetTransformGraphHead() = 0;
						
		protected:
			virtual array<Type^>^ DoGetArgumentTypes() = 0;

			bool CheckArguments();
			property array<Object^>^ Args { array<Object^>^ get(); }			
			property Microsoft::VisualStudio::Debugger::DkmProcess^ Process
			{ Microsoft::VisualStudio::Debugger::DkmProcess^ get(); }

		private:
			array<Object^>^ args_;
			Microsoft::VisualStudio::Debugger::DkmProcess^ process_;
		};
	}
}