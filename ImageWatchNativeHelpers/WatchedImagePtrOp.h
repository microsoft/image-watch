#pragma once

#pragma unmanaged
#include "vtcore.h"
#pragma managed

#include "WatchedImageOperator.h"

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImagePtrOp : WatchedImageOperator
		{			
		public:
			WatchedImagePtrOp();
			~WatchedImagePtrOp();
			!WatchedImagePtrOp();

		internal:
			virtual vt::CTransformGraphNode* GetTransformGraphHead() override;

		protected:
			virtual array<Type^>^ DoGetArgumentTypes() override;

		protected:
			virtual void DoReloadInfo(WatchedImageInfo^% info) override;
			virtual void DoReloadPixels(bool% hasPixelsLoaded) override;

		private:
			UInt64 addr_;
			int vtType_;
			UInt32 width_;
			UInt32 height_;
			UInt32 stride_;

			// not optimal .. currently have 2 copies of the image to fit 
			// debugger's reload info/pixels semantics ..
			vt::CImg* image_;
			vt::CImgReaderWriter<vt::CImg>* irw_;

		private:
			WatchedImageInfo^ MakeInfo();				

		private:
			vt::CTransformGraphUnaryNode* transformNode_;
			vt::CConvertTransform* transform_;
		};
	}
}