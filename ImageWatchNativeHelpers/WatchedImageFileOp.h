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
		public ref class WatchedImageFileOp : WatchedImageOperator
		{			
		public:
			WatchedImageFileOp();
			~WatchedImageFileOp();
			!WatchedImageFileOp();

		internal:
			virtual vt::CTransformGraphNode* GetTransformGraphHead() override;

		protected:
			virtual array<Type^>^ DoGetArgumentTypes() override;

		protected:
			virtual void DoReloadInfo(WatchedImageInfo^% info) override;
			virtual void DoReloadPixels(bool% hasPixelsLoaded) override;

		private:
			property String^ Filename { String^ get(); }
			
			bool LoadFromFileIfOutOfDate();
			DateTime lastWriteTime_;
			String^ lastFileName_;

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