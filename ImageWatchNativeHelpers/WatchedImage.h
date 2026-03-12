#pragma once

#define MAX_IMAGE_WIDTH (1 << 16)
#define MAX_IMAGE_HEIGHT (1 << 16)

namespace vt
{
	class IImageReaderWriter;
}

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageInfo
		{
		public:
			WatchedImageInfo();

		public:
			bool IsInitialized;
			
			UInt32 Width;
			UInt32 Height;

			property UInt32 NumBands { UInt32 get(); }
			property String^ ElementFormat { String^ get(); }

			String^ PixelFormat;

			UInt32 ViewerPixelFormat;
			
			property bool HasPixels
			{
				bool get()
				{ 
					return IsInitialized && Width * Height * NumBands > 0; 
				}
			}
			
			bool HasNativePixelValues;			
			bool HasNativePixelAddress;	
			
			UInt64 PixelAddress;
			UInt32 NumBytesPerPixel;
			UInt32 NumStrideBytes;

			String^ SpecialColorMapName;			
			
			bool HasCustomValueRange;
			double CustomValueRangeStart;
			double CustomValueRangeEnd;
		};

		public ref class WatchedImage abstract 
			: public Microsoft::Research::NativeImage::NativeImageBase
		{
		public:
			WatchedImage();

			void InvalidateInfo();
			void ReloadInfo();
			bool HasValidInfo();			
			WatchedImageInfo^ GetInfo();
			
			void InvalidatePixels();
			bool HasPixelsLoaded();					
			void ReloadPixels();

		internal:
			// this is both for operator classes and derived classes
			vt::IImageReaderWriter* GetReaderWriter();
			bool GetViewFormat(int% width, int% height, int% vttype);
			
		protected:			
			virtual void DoReloadInfo(WatchedImageInfo^% info) = 0;
			virtual void DoReloadPixels(bool% hasPixelsLoaded) = 0;

		private:
			void ValidateInfo();

			WatchedImageInfo^ info_;
			bool hasPixelsLoaded_;
			
			// legacy API from vtiviewer
		public:
			virtual bool Reload() override;
		};
	}
}