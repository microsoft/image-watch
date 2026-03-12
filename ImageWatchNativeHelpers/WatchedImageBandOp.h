#pragma once

#include "WatchedImageUnaryOp.h"

class BandSelectTransform;

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageBandOp : WatchedImageUnaryOp
		{			
		public:
			WatchedImageBandOp();
			~WatchedImageBandOp();
			!WatchedImageBandOp();
		
		protected:
			virtual array<Type^>^ GetNonImageArgumentTypes() override;
			virtual bool AreNonImageArgsValid() override;
			virtual bool InitializeTransform() override;
			virtual bool GetOpViewFormat(int% width, int%height, int% vttype) override;

		private:
			property UInt32 BandIndex { UInt32 get(); }

		private:
			BandSelectTransform* transform_;
		};
	}
}