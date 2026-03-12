#pragma once

#include "Namespace.h"
#include "AutoPtr.h"
#include "NativeImageBase.h"
#include <WinDef.h>

namespace vt
{
	template <class ET>
	class CMtx3x3;	
	class IImageReader;
	class CImg;
}

using namespace System;
using namespace System::Windows;
using namespace System::Windows::Interop;
using namespace System::Windows::Media;

BEGIN_NI_NAMESPACE;

class ColorMap;

public ref class NativeImageViewState
{
public:
	property Point BitmapRef;
	property Point SourceRef;
	property double Scale;
};

public ref class NativeImageView
{
public:
	NativeImageView(NativeImageBase^ source, int width, int height);
	NativeImageView(int width, int height);

	void SetSource(NativeImageBase^ source);
	NativeImageBase^ GetSource()
	{
		return sourceImage_;
	}
	
	void MarkAsDirty()
	{
		isDirty_ = true;
	}

	~NativeImageView();	
	!NativeImageView();

	void Clear();
	void Render();
	property String^ LastError;
		
	property InteropBitmap^ Bitmap 
	{ 
		InteropBitmap^ get()
		{
			return bitmap_;
		}
	}

	property NativeImageViewState^ State
	{
		NativeImageViewState^ get()
		{
			NativeImageViewState^ s = gcnew NativeImageViewState();
			
			s->BitmapRef = BitmapRef;
			s->SourceRef = SourceRef;
			s->Scale = Scale;

			return s;
		}
		
		void set(NativeImageViewState^ value)
		{
			if (value != nullptr)
			{
				BitmapRef = value->BitmapRef;
				SourceRef = value->SourceRef;
				Scale = value->Scale;
			}
		}
	}
	
	property String^ ColorMapName
	{
		String^ get()
		{
			return colorMapName_;
		}
		void set(String^ value)
		{
			if (colorMapName_ == nullptr && value != nullptr)
				MarkAsDirty();

			if (colorMapName_ != nullptr && value != nullptr &&
				!colorMapName_->Equals(value))
			{
				MarkAsDirty();				
			}

			colorMapName_ = value;
		}
	}

	property double ColorMapDomainStart
	{
		double get()
		{
			return colorMapDomainStart_;
		}
		void set(double value)
		{
			if (colorMapDomainStart_ != value)
				MarkAsDirty();

			colorMapDomainStart_ = value;
		}
	}

	property double ColorMapDomainEnd
	{
		double get()
		{
			return colorMapDomainEnd_;
		}
		void set(double value)
		{
			if (colorMapDomainEnd_ != value)
				MarkAsDirty();

			colorMapDomainEnd_ = value;
		}
	}
	
	property int ColorMapSelectedBand
	{
		int get()
		{
			return colorMapSelectedBand_;
		}
		void set(int value)
		{
			if (colorMapSelectedBand_ != value)
				MarkAsDirty();

			colorMapSelectedBand_ = value;
		}
	}

	property Color BackgroundColor
	{
		Color get()
		{
			return backgroundColor_;
		}
		void set(Color value)
		{
			if (!backgroundColor_.Equals(value))
				MarkAsDirty();

			backgroundColor_ = value;
		}
	}

	property Point BitmapRef
	{
		Point get()
		{
			return bitmapRef_;
		}
		void set(Point value)
		{
			if (!bitmapRef_.Equals(value))
				MarkAsDirty();

			bitmapRef_ = value;
		}
	}

	property Point SourceRef
	{
		Point get()
		{
			return sourceRef_;
		}
		void set(Point value)
		{
			if (!sourceRef_.Equals(value))
				MarkAsDirty();

			sourceRef_ = value;
		}
	}

	property double Scale
	{
		double get()
		{
			return scale_;
		}
		void set(double value)
		{
			if (scale_ != value)
				MarkAsDirty();

			scale_ = value;
		}
	}

	property int PixelWidth
	{
		int get()
		{
			return width_;
		}
	}

	property int PixelHeight
	{
		int get()
		{
			return height_;
		}
	}

	property bool GridEnabled
	{
		bool get()
		{
			return gridEnabled_;
		}
		void set(bool value)
		{
			gridEnabled_ = value;
		}
	}

	Point BitmapToSourceXY(Point bitmapXY);
	Point SourceToBitmapXY(Point sourceXY);
	
private:
	int ComputeNearestLowerLevel();
	void ComputeWarpMatrix(vt::CMtx3x3<float>& m);
	void ComputeWarpMatrix(vt::CMtx3x3<float>& m, int dLevel);
		
	void SetDefaultTransformParameters();
	double GetColorMapPreScale(const ColorMap* cmap, int imageType);
	double GetColorMapPreOffset(const ColorMap* cmap, int imageType);

	void AllocateNativeResources(int width, int height);
	void ReleaseNativeResources();	
	
	vt::IImageReader* GetSourceReader();

private:	
	bool isDirty_;
	NativeImageBase^ sourceImage_;	 

	HANDLE fileMapping_;
	void* sharedMemory_;
	AutoPtr<vt::CRGBByteImg> view_;
	
	InteropBitmap^ bitmap_;
	int width_, height_;

	String^ colorMapName_;
	Color backgroundColor_;
	Point bitmapRef_;
	Point sourceRef_;
	double colorMapDomainStart_;
	double colorMapDomainEnd_;
	int colorMapSelectedBand_;
	double scale_;
	bool gridEnabled_;
};

END_NI_NAMESPACE