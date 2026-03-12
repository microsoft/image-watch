#pragma once

namespace vt
{
	class CTransformGraphBinaryNode;
	class IImageTransform;
}

#include "WatchedImageOperator.h"

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageBinaryOp abstract : WatchedImageOperator
		{			
		public:
			WatchedImageBinaryOp();
			~WatchedImageBinaryOp();
			!WatchedImageBinaryOp();

		internal:
			virtual vt::CTransformGraphNode* GetTransformGraphHead() override;

		protected:
			virtual array<Type^>^ DoGetArgumentTypes() override;

		protected:
			virtual void DoReloadInfo(WatchedImageInfo^% info) override;
			virtual void DoReloadPixels(bool% hasPixelsLoaded) override;
	
		protected:
			property WatchedImage^ FirstInputImage { WatchedImage^ get(); }
			property WatchedImage^ SecondInputImage { WatchedImage^ get(); }
			void SetTransform(vt::IImageTransform* transform); 
			
			virtual array<Type^>^ GetNonImageArgumentTypes() = 0;
			virtual bool AreNonImageArgsValid() = 0;
			virtual bool AreImageArgsValid() = 0;
			virtual bool GetOpViewFormat(int% width, int%height, int% vttype) = 0;
			virtual bool InitializeTransform() = 0;

		private:
			WatchedImageInfo^ MakeInfo();	

		private:
			vt::CTransformGraphNaryNode* transformNode_;
		};
	}
}