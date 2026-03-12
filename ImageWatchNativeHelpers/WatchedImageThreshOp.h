#pragma once

#include "WatchedImageUnaryOp.h"

class ThreshTransform;

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageThreshOp : WatchedImageUnaryOp
		{			
		public:
			WatchedImageThreshOp();
			~WatchedImageThreshOp();
			!WatchedImageThreshOp();
		
		protected:
			virtual array<Type^>^ GetNonImageArgumentTypes() override;
			virtual bool AreNonImageArgsValid() override;
			virtual bool InitializeTransform() override;
			virtual bool GetOpViewFormat(int% width, int%height, int% vttype) 
				override;

		private:
			property float Threshold { float get(); }

		private:
			ThreshTransform* transform_;
		};
	}
}