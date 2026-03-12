#pragma once

#include <windows.h>
#include "Namespace.h"
#include "AutoPtr.h"

using namespace System;
using namespace System::Windows;

namespace vt
{
	class IImageReader;
	class IImageReaderWriter;
	class CVTResourceManagerInit;
}

BEGIN_NI_NAMESPACE;

			public ref class NativePixelFormat
			{
			public:
				property String^ ElementFormat;
				property int NumBytesPerElement;
				property int NumBands;
				property String^ PixelFormat;
			};

			public ref class NativeImageBase abstract
			{
			public:
				NativeImageBase();
				~NativeImageBase();
				!NativeImageBase();

			public:
				property bool IsDisposed { bool get() { return image_.IsNull(); } }

				property System::Windows::Int32Rect Rectangle { Int32Rect get(); }
				property NativePixelFormat^ PixelFormat { NativePixelFormat^ get(); }

				property bool HasMinMaxElementValue { bool get(); }
				void ComputeMinMaxElementValue(int selectedBand);
				property double MinElementValue { double get(); }
				property double MaxElementValue { double get(); }

				cli::array<Byte>^ ReadPixel(int x, int y);

				property IntPtr IImageReader { IntPtr get(); };

			public:
				// return type is bool--this is not used in old (vtiview) derived impl,
				// which throw exceptions, i.e. it either throws or returns true.
				virtual bool Reload() = 0;

			protected:
				void ClearMinMaxValue();
				void SetImage(vt::IImageReaderWriter* img);
				void SetExternallyManagedImage(vt::IImageReaderWriter* img);
				vt::IImageReaderWriter* GetImage();
				HRESULT BuildMipMap(int baseLevel);

				void SetImageIntPtr(IntPtr img);
				IntPtr GetImageIntPtr(bool throwIfNull);

			private:
				AutoPtr<vt::CVTResourceManagerInit> vtrm_;
				AutoPtr<vt::IImageReaderWriter> image_;
				bool imagePtrManagedExternally_;

				bool minMaxComputed_;
				double minElementValue_;
				double maxElementValue_;
			};

			END_NI_NAMESPACE
