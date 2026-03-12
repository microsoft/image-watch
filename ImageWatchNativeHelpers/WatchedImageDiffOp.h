#pragma once

#include "WatchedImageBinaryOp.h"

class DiffTransform;

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageDiffOp : WatchedImageBinaryOp
		{			
		public:
			WatchedImageDiffOp();
			~WatchedImageDiffOp();
			!WatchedImageDiffOp();
		
		protected:
			virtual array<Type^>^ GetNonImageArgumentTypes() override;
			virtual bool AreNonImageArgsValid() override;
			virtual bool AreImageArgsValid() override;
			virtual bool InitializeTransform() override;
			virtual bool GetOpViewFormat(int% width, int%height, int% vttype) override;

		private:
			DiffTransform* transform_;
		};
	}
}