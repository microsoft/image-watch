#pragma once

#include "WatchedImageUnaryOp.h"

class AbsTransform;

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageAbsOp : WatchedImageUnaryOp
		{			
		public:
			WatchedImageAbsOp();
			~WatchedImageAbsOp();
			!WatchedImageAbsOp();
		
		protected:
			virtual array<Type^>^ GetNonImageArgumentTypes() override;
			virtual bool AreNonImageArgsValid() override;
			virtual bool InitializeTransform() override;
			virtual bool GetOpViewFormat(int% width, int%height, int% vttype) override;

		private:
			AbsTransform* transform_;
		};
	}
}