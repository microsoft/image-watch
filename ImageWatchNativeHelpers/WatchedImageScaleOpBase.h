#pragma once

#include "WatchedImageUnaryOp.h"

class ScaleTransform;

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageScaleOpBase abstract : WatchedImageUnaryOp
		{			
		public:
			WatchedImageScaleOpBase();
			~WatchedImageScaleOpBase();
			!WatchedImageScaleOpBase();
		
		protected:
			virtual bool InitializeTransform() override;
			virtual bool GetOpViewFormat(int% width, int%height, int% vttype) 
				override;

		protected:
			virtual property float Scale { float get() = 0; }

		private:
			ScaleTransform* transform_;
		};
	}
}