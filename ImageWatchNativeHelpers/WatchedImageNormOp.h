#pragma once

#include "WatchedImageScaleOpBase.h"

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageNormOpBase abstract 
			: WatchedImageScaleOpBase
		{			
		protected:
			virtual array<Type^>^ GetNonImageArgumentTypes() override
			{
				return nullptr;
			}

			virtual bool AreNonImageArgsValid() override
			{
				return true;
			}
		};

		public ref class WatchedImageNorm8Op : WatchedImageNormOpBase
		{			
		protected:
			virtual property float Scale 
			{ 
				float get() override
				{
					return 1.f / 255;
				}
			}
		};

		public ref class WatchedImageNorm16Op : WatchedImageNormOpBase
		{			
		protected:
			virtual property float Scale 
			{ 
				float get() override
				{
					return 1.f / 65535;
				}
			}
		};
	}
}