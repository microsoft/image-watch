#pragma once

#include "WatchedImageUnaryOp.h"

class ClampTransform;

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageClampOp : WatchedImageUnaryOp
		{			
		public:
			WatchedImageClampOp();
			~WatchedImageClampOp();
			!WatchedImageClampOp();
		
		protected:
			virtual array<Type^>^ GetNonImageArgumentTypes() override;
			virtual bool AreNonImageArgsValid() override;
			virtual bool InitializeTransform() override;
			virtual bool GetOpViewFormat(int% width, int%height, int% vttype) override;

		private:
			property float Min { float get(); }
			property float Max { float get(); }

		private:
			ClampTransform* transform_;
		};
	}
}