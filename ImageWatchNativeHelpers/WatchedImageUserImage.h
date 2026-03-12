#pragma once

#include "WatchedImageImage.h"
#include "WatchedImageHelpers.h"

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{	
		///////////////////////////////////////////////////////////////////////

		public enum class UserPixelFormat
		{
			Generic,
			LUMA,
			RG,
			UV,
			BGR,
			BGRA,
			RGB,
			RGBA,
			YUV,
			NV12,
			YV12,
			IYUV,
			YUY2
		};

		ref class UserPixelFormatInfo
		{
		public:
			UserPixelFormatInfo(String^ cmapName, UInt32 numBands)
				: ColorMapName(cmapName), NumBands(numBands)
			{
			}

		public:
			String^ ColorMapName;
			UInt32 NumBands;
		};

		///////////////////////////////////////////////////////////////////////

		public value class PlaneInfo
		{
		public:
			UInt32 Width;
			UInt32 Height;
			UInt32 NumBands;
			UInt64 PixelAddress;
			UInt32 NumStrideBytes;
		};
		
		///////////////////////////////////////////////////////////////////////

		public ref class WatchedImageUserImage : WatchedImageImage
		{	
		public:			
			void SetErrorLogAction(Action<String^>^ errorLogAction)
			{
				errorLogAction_ = errorLogAction;
			}

		protected:
			virtual void DoReloadInfo(WatchedImageInfo^% info) override;
			virtual void DoReloadPixels(bool% hasPixelsLoaded) override;

		private:
			WatchedImageInfo^ MakeInfo();
		
		private:
			Action<String^>^ errorLogAction_;

			void LogError(String^ msg)
			{
				if (errorLogAction_ != nullptr)
					errorLogAction_(msg);
			}

			bool TryParseUInt32Array(String^ str, cli::array<UInt32>^% arr);
			bool TryParseUInt64Array(String^ str, cli::array<UInt64>^% arr);
			bool TryParseDoubleArray(String^ str, cli::array<double>^% arr);

			UInt32 NumPlanes()
			{
				if (planeInfo_ == nullptr)
					return 0;
				
				return planeInfo_->Length;
			}

			UInt32 NumBands()
			{
				UInt32 res = 0;
				for (UInt32 i = 0; i < NumPlanes(); ++i)
					res += planeInfo_[i].NumBands;
				return res;
			}

			UInt32 Width()
			{
				return NumPlanes() > 0 ? planeInfo_[0].Width : 0;
			}

			UInt32 Height()
			{
				return NumPlanes() > 0 ? planeInfo_[0].Height : 0;
			}

			UInt64 PixelAddress()
			{
				return NumPlanes() > 0 ? planeInfo_[0].PixelAddress : 0;
			}

			UInt32 NumStrideBytes()
			{
				return NumPlanes() > 0 ? planeInfo_[0].NumStrideBytes : 0;
			}
			
			static cli::array<PlaneInfo>^ MakePlaneInfos(UInt32 numPlanes, 
				UInt32 numBands, UserPixelFormat pixelFormat, UInt32 width, 
				UInt32 height, 
				cli::array<UInt64>^ addressArr, cli::array<UInt32>^ strideArr);

			static bool ParsePixelFormat(String^ pfstring, 
				UserPixelFormat% format, UInt32% numBands, String^% cmapname);
			
			static String^ GetPixelFormatName(UserPixelFormat fmt)
			{
				return Enum::GetName(UserPixelFormat::typeid, fmt);
			}

		private:
			int elementFormat_;			
			UserPixelFormat pixelFormat_;
			String^ specialColormapName_;

			cli::array<double>^ customRange_;
			cli::array<PlaneInfo>^ planeInfo_;
			
		private:
			static WatchedImageUserImage();
			static System::Collections::Generic::Dictionary<UserPixelFormat, 
				UserPixelFormatInfo^>^ supportedFormats_;			
		};
	}
}