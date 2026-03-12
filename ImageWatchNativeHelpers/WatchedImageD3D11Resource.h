#pragma once

#include <d3d11.h>
#include "WatchedImageImage.h"

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageD3D11Resource : WatchedImageImage
		{			       
		protected:
			virtual void DoReloadInfo(WatchedImageInfo^% info) override;
			virtual void DoReloadPixels(bool% hasPixelsLoaded) override;
			
		private:
			WatchedImageInfo^ MakeInfo();

			static bool DXToVTFormat(UInt32 dxFormat, int% vtFormat, 
				String^% fmtString, String^% clrMapName);

		private:			
			UInt32 width_;
			UInt32 height_;
			int vtType_;
			String^ pixelFormatString_;
			String^ specialColorMapName_;
			UInt64 pixelAddress_;
			UInt64 deviceAddress_;
			UInt32 numStrideBytes_;
		};
	}
}