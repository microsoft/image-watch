#pragma once

#include "WatchedImageUnaryOp.h"

//class FlipTransform;
namespace vt
{
	class CRotateTransform;
};

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageFlipOpBase abstract : WatchedImageUnaryOp
		{			
		public:
			WatchedImageFlipOpBase();
			~WatchedImageFlipOpBase();
			!WatchedImageFlipOpBase();

		protected:
			virtual array<Type^>^ GetNonImageArgumentTypes() override;
			virtual bool AreNonImageArgsValid() override;
			virtual bool InitializeTransform() override;
			virtual bool GetOpViewFormat(int% width, int%height, int% vttype) override;

		protected:
			// to be implemented by derived class
			virtual property int Mode { int get() = 0; }

		private:			
			vt::CRotateTransform* transform_;
		};
	}
}