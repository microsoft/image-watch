#pragma once

#include "WatchedImageImage.h"

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageVTCImgInCache : WatchedImageImage
		{			
        public:
            WatchedImageVTCImgInCache() : width_(0), height_(0), type_(OBJ_UNDEFINED),
                blockSizeX_(0), blockSizeY_(0), blocksAddress_(NULL), blocksSize_(0),
                uniqueness_(NULL) {};
            ~WatchedImageVTCImgInCache() { delete[] uniqueness_; };

		protected:
			virtual void DoReloadInfo(WatchedImageInfo^% info) override;
			virtual void DoReloadPixels(bool% hasPixelsLoaded) override;
			
		private:
			WatchedImageInfo^ MakeInfo();

		private:			
			int width_;
			int height_;
			int type_;
			UInt32 blockSizeX_;
			UInt32 blockSizeY_;
            UInt64 blocksAddress_;
            UInt64 blocksSize_;
			UInt32 * uniqueness_;
		};
	}
}