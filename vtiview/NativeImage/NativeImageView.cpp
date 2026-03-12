#include "Stdafx.h"

#include "NativeImageView.h"
#include "NativeImageHelpers.h"
#include "DrawPixelGridTransform.h"
#include "ColorMap.h"
#include "AutoLock.h"

#include <Windows.h>

using namespace System;
using namespace System::Windows;

BEGIN_NI_NAMESPACE;

#ifdef _DEBUG
#define COUNT_NIV_OBJECTS
#ifdef COUNT_NIV_OBJECTS
int g_niv_objects = 0;
#endif
#endif

NativeImageView::NativeImageView(NativeImageBase^ source, int width, int height)
	: isDirty_(true), fileMapping_(INVALID_HANDLE_VALUE), 
	sharedMemory_(nullptr), bitmap_(nullptr), gridEnabled_(true)
{
	if (width * height <= 0)
		throw gcnew ArgumentOutOfRangeException();

	AllocateNativeResources(width, height);

	SetSource(source);

#ifdef COUNT_NIV_OBJECTS
	System::Diagnostics::Debug::WriteLine("NIV: {0}", ++g_niv_objects);	
#endif
}

NativeImageView::NativeImageView(int width, int height)
	: isDirty_(true), fileMapping_(INVALID_HANDLE_VALUE), 
	sharedMemory_(nullptr), bitmap_(nullptr), gridEnabled_(true)
{
	if (width * height <= 0)
		throw gcnew ArgumentOutOfRangeException();

	AllocateNativeResources(width, height);

#ifdef COUNT_NIV_OBJECTS
	System::Diagnostics::Debug::WriteLine("NIV: {0}", ++g_niv_objects);	
#endif
}

NativeImageView::~NativeImageView()
{
	this->!NativeImageView();
}

NativeImageView::!NativeImageView()
{
	ReleaseNativeResources();

#ifdef COUNT_NIV_OBJECTS
	System::Diagnostics::Debug::WriteLine("NIV: {0}", --g_niv_objects);	
#endif
}

void NativeImageView::SetSource(NativeImageBase^ source)
{
	/*if (source == nullptr)
		throw gcnew ArgumentNullException();
		*/

	if (source != sourceImage_)
		MarkAsDirty();

	sourceImage_ = source;

	SetDefaultTransformParameters();
}

vt::IImageReader* NativeImageView::GetSourceReader()
{
	if (sourceImage_ == nullptr)
		return nullptr;
	
	AutoLock lock(sourceImage_);
	
	if (sourceImage_->IsDisposed)
		return nullptr;

	return reinterpret_cast<vt::IImageReader*>(sourceImage_->IImageReader
		.ToPointer());
}

void NativeImageView::SetDefaultTransformParameters()
{
	BitmapRef = Point(0.5 * view_->Width(), 0.5 * view_->Height());

	auto src = GetSourceReader();

	if (src != nullptr)
	{
		const vt::CRect sourceRect = 
			GetSourceReader()->GetImgInfo().LayerRect();

		SourceRef = Point(0.5 * (sourceRect.left + sourceRect.right),
			0.5 * (sourceRect.top + sourceRect.bottom));
	}
	else
	{
		SourceRef = Point(0, 0);
	}

	Scale = 1.0;
}

int NativeImageView::ComputeNearestLowerLevel()
{
	int maxLevel = vt::ComputePyramidLevelCount(
		GetSourceReader()->GetImgInfo().width,
		GetSourceReader()->GetImgInfo().height) - 1;

	if (maxLevel < 0)
		return 0;

	return Math::Min(maxLevel, Math::Max(0, 
		(int)Math::Floor(Math::Log(1.0/Scale)/Math::Log(2.0))));
}

void NativeImageView::ComputeWarpMatrix(vt::CMtx3x3<float>& m)
{
	ComputeWarpMatrix(m, 0);
}

void NativeImageView::ComputeWarpMatrix(vt::CMtx3x3<float>& m, int dLevel)
{
	System::Diagnostics::Debug::Assert(Scale > 0.0);
	System::Diagnostics::Debug::Assert(dLevel >= 0);
	
	vt::CMtx3x3f t0(1.0f, 0.0, (float)-BitmapRef.X,
		0.0, 1.0f, (float)-BitmapRef.Y,
		0.0, 0.0, 1.0f);
	
	double f = 1.0/(1 << dLevel);

	vt::CMtx3x3f st1((float)(f/Scale), 0.0, (float)(f*SourceRef.X),
		0.0, (float)(f/Scale), (float)(f*SourceRef.Y),
		0.0, 0.0, 1.0f);

	m = st1 * t0;
}

Point NativeImageView::BitmapToSourceXY(Point bitmapXY)
{
	vt::CVec3f p((float)bitmapXY.X, (float)bitmapXY.Y, 1.0f);

	vt::CMtx3x3f warp;
	ComputeWarpMatrix(warp);

	vt::CVec3f q = warp * p;

	return Point(q.x, q.y);
}

Point NativeImageView::SourceToBitmapXY(Point sourceXY)
{
	vt::CVec3f p((float)sourceXY.X, (float)sourceXY.Y, 1.0f);

	vt::CMtx3x3f warp;
	ComputeWarpMatrix(warp);

	vt::CVec3f q = warp.Inv() * p;

	return Point(q.x, q.y);
}

double NativeImageView::GetColorMapPreScale(const ColorMap* cmap, 
	int imageType)
{
	double reqRange = ColorMapDomainEnd - ColorMapDomainStart;
	double defRange = cmap->GetDomainEnd(imageType) - 
		cmap->GetDomainStart(imageType);
	
	if (reqRange <= 0)
		return 0;

	return defRange / reqRange; 
}

double NativeImageView::GetColorMapPreOffset(const ColorMap* cmap,
	int imageType)
{
	// all values the same? -> make "mid" tone
	if (ColorMapDomainEnd == ColorMapDomainStart)
	{
		// get mid tone for this image type
		return cmap->GetDomainStart(imageType) + 
			0.5*(cmap->GetDomainEnd(imageType) - 
			cmap->GetDomainStart(imageType));
	}
	else
	{
		return -ColorMapDomainStart * GetColorMapPreScale(cmap, imageType) + 
			cmap->GetDomainStart(imageType);
	}
}

void NativeImageView::Clear()
{	
	vt::RGBPix c(BackgroundColor.R, BackgroundColor.G, BackgroundColor.B);	
	view_->Fill(c);
}

void NativeImageView::Render()
{
	if (!isDirty_)
		return;
	
#ifdef _DEBUG
    System::Diagnostics::Debug::WriteLine("  *** rendering ... ");
#endif

	if (sourceImage_ == nullptr)
	{
		LastError = "Error: no source";
		return;
	}
	
	AutoLock lock(sourceImage_);
	
	if (sourceImage_->IsDisposed)
	{
#ifdef _DEBUG
		System::Diagnostics::Debug::WriteLine("Warning: image disposed");
#endif
		LastError = "Error: image already disposed";
		return;
	}

	if (GetSourceReader()->GetImgInfo().PixSize() > vt::OpHelpers::ConvertBufSize)
	{
		LastError = "Error: pixel size too large";
		return;
	}

	VT_HR_BEGIN();

	LastError = nullptr;

	vt::CImgReaderWriter<vt::CImg> dst;
	VT_HR_EXIT(view_->Share(dst));

	int level = ComputeNearestLowerLevel();

	// workaround for pyramid generation not working with nonstandard types
	int selt = EL_FORMAT(GetSourceReader()->GetImgInfo().type);
	if (selt != EL_FORMAT_BYTE && selt != EL_FORMAT_SHORT && 
		selt != EL_FORMAT_HALF_FLOAT && selt != EL_FORMAT_FLOAT)
	{
		if (level > 0)
		{
#ifdef _DEBUG
			System::Diagnostics::Debug::WriteLine(
				"Warning: downsampling non-standard type without pyramid");
#endif
			level = 0;
		}
	}

	vt::CLevelChangeReader reader(GetSourceReader(), level);

	vt::CMtx3x3f warp;
	ComputeWarpMatrix(warp, level);

	if (colorMapName_ == nullptr)
	{
		LastError = String::Format("Error: colormap not set");
		return;
	}

	vt::IMAGE_EXTEND ex(vt::Extend);

	// step 0 (optional): select bands
	vt::CTransformGraphUnaryNode bandSelectNode;
	BandSelectTransform bsXform;	
	//vt::CConvertTransform bsXform(EL_FORMAT_FLOAT, 1);
	bool selectSingleBand = ColorMapSelectedBand >= 0;
	if (selectSingleBand)
	{
		bsXform.Initialize(ColorMapSelectedBand, true);
		bandSelectNode.SetTransform(&bsXform);
		VT_HR_EXIT(bandSelectNode.BindSourceToReader(&reader, &ex));
	}

	vt::CTransformGraphUnaryNode graph[7];
	
	const ColorMap* cmap = g_colorMapStore.GetMapByName(
		Details::ManagedToNativeString(colorMapName_));
	
	if (cmap == nullptr)
	{
		LastError = "Error: colormap not found";
		return;
	}

	// step 1: prescale/preoffset
	ScaleOffsetTransform scaleOffsetXform(
		(float)GetColorMapPreScale(cmap, reader.GetImgInfo().type),
		(float)GetColorMapPreOffset(cmap, reader.GetImgInfo().type),
		reader.GetImgInfo().type, 
		selectSingleBand ? 1 : reader.GetImgInfo().Bands());
	graph[0].SetTransform(&scaleOffsetXform);

	if (selectSingleBand)
		VT_HR_EXIT(graph[0].BindSourceToTransform(&bandSelectNode));
	else
		VT_HR_EXIT(graph[0].BindSourceToReader(&reader, &ex));

	auto cminputinfo = reader.GetImgInfo();
	if (selectSingleBand)
	{
		cminputinfo.type = VT_IMG_MAKE_TYPE(EL_FORMAT(cminputinfo.type), 1);
	}

	if (!cmap->IsCompatibleWith(cminputinfo))
	{
		LastError = String::Format("Error: cannot render this pixel format "
			"with colormap '{0}'", gcnew String(cmap->GetName().c_str()));				
		return;
	}
	
	// step 2: map colors to RGBA byte
	ColorMapTransform colorMapXform(cmap, 
		selectSingleBand ? 1 : reader.GetImgInfo().Bands());
	VT_HR_EXIT(graph[1].BindSourceToTransform(&graph[0]));
	graph[1].SetTransform(&colorMapXform);

	// step 3: convert to rgba float
	// this is a workaround because warp requires floats, but colormap should
	// produce rgba bytes (if colormap produces floats, ringing may occur 
	// during the resize-warp
	vt::CConvertTransform floatXform(VT_IMG_FIXED(OBJ_RGBAFLOATIMG));
	VT_HR_EXIT(graph[2].BindSourceToTransform(&graph[1]));
	graph[2].SetTransform(&floatXform);

	// step 4: warp
	vt::CWarpTransform warpXform;
	const float minDisplayPixelSizeForNN = 2.0;
	vt::CRect warpSrcRect = vt::MapRegion3x3(warp, dst.Rect());		
		
	if (warp(0,0) > 1.0 / minDisplayPixelSizeForNN)
	{
		VT_HR_EXIT(warpXform.InitializeResize(warp(0,0), warp(0,2),
			warp(1,1), warp(1,2), VT_IMG_FIXED(OBJ_RGBAFLOATIMG),
			vt::eSamplerKernelLanczos3));
	}
	else
	{
		VT_HR_EXIT(warpXform.Initialize(warp, warpSrcRect,
			dst.Rect(), VT_IMG_FIXED(OBJ_RGBAFLOATIMG), 
			vt::eSamplerKernelNearest));
	}

	VT_HR_EXIT(graph[3].BindSourceToTransform(&graph[2]));
	graph[3].SetTransform(&warpXform);

	// step 5: draw pixel grid if high magnification
	vt::RGBPix bgColor;
	bgColor.b = BackgroundColor.B;
	bgColor.g = BackgroundColor.G;
	bgColor.r = BackgroundColor.R;
	DrawPixelGridTransform dpgXform(warp, bgColor, 80, 
		gridEnabled_ ? 16.f : 1e10f);
	VT_HR_EXIT(graph[4].BindSourceToTransform(&graph[3]));
	graph[4].SetTransform(&dpgXform);

	// step 6: crop = fill invalid areas with clearvalue
	vt::CRect warpDstRect = vt::MapRegion3x3(warp.Inv(), 
		reader.GetImgInfo().LayerRect());
	vt::CLayerImgInfo cropSrcImgInfo(vt::CImgInfo(warpDstRect.Width(), 
		warpDstRect.Height(), OBJ_RGBAFLOATIMG), 
		warpDstRect.Width(), warpDstRect.Height(), 
		warpDstRect.TopLeft());
	
	vt::IMAGE_EXTEND extend;
	VT_HR_EXIT(extend.Initialize(vt::Constant, vt::Constant, bgColor, bgColor));
	
	vt::CCropPadTransform cropXform(cropSrcImgInfo, dst.Rect());
	VT_HR_EXIT(cropXform.InitExtendMode(extend));
	VT_HR_EXIT(graph[5].BindSourceToTransform(&graph[4]));
	graph[5].SetTransform(&cropXform);	

	vt::CTransformGraphUnaryNode* graphHead = nullptr;
	
	// step 7 (optional): alpha 	
	VT_HR_EXIT(graph[6].BindSourceToTransform(&graph[5]));
	AlphaBlendTransform abXform;
	graph[6].SetTransform(&abXform);
	if (cmap->UsesAlpha(reader.GetImgInfo()))
		graphHead = &graph[6];
	else
		graphHead = &graph[5];
	
	(*graphHead).SetDest(&dst);

	vt::VT_TRANSFORM_TASK_OPTIONS opt;	
	//opt.maxthreads = 1;
	VT_HR_EXIT(vt::PushTransformTaskAndWait(graphHead, 
		(vt::CTaskProgress*)NULL, &opt));

	VT_HR_EXIT_LABEL();

	VT_ASSERT(hr == S_OK);
	
	if (FAILED(hr))
	{
		LastError = String::Format("Error: rendering failed: {0}",
			NativeImageHelpers::HRToString(hr, false));
	}

	isDirty_ = FAILED(hr);
}

void NativeImageView::AllocateNativeResources(int width, int height)
{	
	ReleaseNativeResources();

	// create rgb byte image buffer
	int numStrideBytes = 3 * width;
	int numBytes = numStrideBytes * height;

	Guid guid = Guid::NewGuid();

	fileMapping_ = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, 
		PAGE_READWRITE, 0, numBytes, 
		Details::ManagedToNativeString(guid.ToString()).c_str());

	if (GetLastError() == ERROR_ALREADY_EXISTS || fileMapping_ == nullptr)
		throw gcnew ApplicationException("Error in CreateFileMapping()");

	sharedMemory_ = MapViewOfFile(fileMapping_, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (sharedMemory_ == nullptr)
		throw gcnew ApplicationException("Error in MapViewOfFile()");	

	try
	{
		view_.Reset(new vt::CRGBByteImg());		
	}
	catch (const std::bad_alloc&)
	{
		throw gcnew OutOfMemoryException();
	}

	// create rgb byte image 
	NativeImageHelpers::ThrowIfHRFailed(
		view_->Create(reinterpret_cast<vt::Byte*>(sharedMemory_),
		width, height, numStrideBytes));	
		
	bitmap_ = safe_cast<InteropBitmap^>(
		Interop::Imaging::CreateBitmapSourceFromMemorySection(
		IntPtr(fileMapping_), width, height, PixelFormats::Bgr24, 
		numStrideBytes, 0));

	width_ = width;
	height_ = height;
}

void NativeImageView::ReleaseNativeResources()
{
	delete bitmap_;
	bitmap_ = nullptr;

	view_.Release();

	if (sharedMemory_ != nullptr)
	{
		UnmapViewOfFile(sharedMemory_);
		sharedMemory_ = nullptr;
	}

	if (fileMapping_ != INVALID_HANDLE_VALUE)
	{	
		CloseHandle(fileMapping_);
		fileMapping_ = INVALID_HANDLE_VALUE;
	}
}

END_NI_NAMESPACE