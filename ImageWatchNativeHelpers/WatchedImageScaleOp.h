#pragma once

#include "WatchedImageScaleOpBase.h"

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageScaleOp : WatchedImageScaleOpBase
		{			
		protected:
			virtual array<Type^>^ GetNonImageArgumentTypes() override;
			virtual bool AreNonImageArgsValid() override;
		
		protected:
			virtual property float Scale { float get() override; }
		};
	}
}