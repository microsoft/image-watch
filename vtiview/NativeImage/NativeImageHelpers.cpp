#include "Stdafx.h"

#include "AutoLock.h"
#include "vtcore.h"
#include "vthdpio.h"
#include "NativeImageHelpers.h"

using namespace System;
using namespace System::Windows;

BEGIN_NI_NAMESPACE;

vt::CVTResourceManagerInit g_resourceManagerInit;

String^ NativeImageHelpers::HRToString(HRESULT hr, bool isIO)
{
	std::vector<wchar_t> buf(256, L'\0');
	
	if (isIO)
		vt::VtIOErrorToString(hr, &buf[0], (int)buf.size());			
	else
		vt::VtErrorToString(hr, &buf[0], (int)buf.size());			

	return gcnew String(&buf[0]);
}

void NativeImageHelpers::ThrowIfHRFailed(HRESULT hr)
{
	ThrowIfHRFailed(hr, false);
}

void NativeImageHelpers::ThrowIfHRFailed(HRESULT hr, bool isIO)
{
	if (FAILED(hr))
		throw gcnew ApplicationException(HRToString(hr, isIO));
}

void NativeImageHelpers::StartVTImageDebugger(bool autoAdd)
{
	vt::VtImageDebuggerStart(autoAdd);
}

void NativeImageHelpers::SetImageCacheMaxMemoryUsage(
	double fractionOfSystemMemory)
{
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms))
        ThrowIfHRFailed(HRESULT_FROM_WIN32(GetLastError()));
    
	// on 64bit use x% of available memory, but don't use less than 256MB
	DWORDLONG memuse = static_cast<DWORDLONG>(ms.ullTotalPhys * 
		fractionOfSystemMemory);
#ifndef  _M_AMD64
    // on 32 bit use min of x% and 512MB, but don't use less than 256MB
	memuse = vt::VtMin(memuse, (DWORDLONG)512*1024*1024);
#endif
	memuse = vt::VtMax(memuse, (DWORDLONG)256*1024*1024);

	if (!SetImageCacheMaxMemoryUsageAbsolute((UInt32)(memuse / (1024*1024))))
		throw gcnew Exception("setting cache memory usage failed");
}

bool NativeImageHelpers::SetImageCacheMaxMemoryUsageAbsolute(
	UInt32 numMegaBytes)
{
    vt::CImageCache* pCache = vt::VtGetImageCacheInstance();
    if (pCache == NULL)
		return false;

	return SUCCEEDED(pCache->SetMaxMemoryUsage((1i64 << 20) * numMegaBytes));
}

float NativeImageHelpers::HalfFloatRawBytesToFloat(cli::array<Byte>^ rawBytes, 
	int offset)
{
	vt::HALF_FLOAT hf;

	IntPtr ptr(&hf.v);

	System::Runtime::InteropServices::Marshal::Copy(rawBytes, offset, 
		ptr, sizeof(hf.v));

	float res;
	vt::VtConv(&res, hf);

	return res;
}

void NativeImageHelpers::SafeLockedAction(NativeImageBase^ ni, 
	Action^ actionIfOK, Action^ defaultAction)
{
	SafeLockedAction(ni, actionIfOK, defaultAction, 
		System::Threading::Timeout::Infinite);
}

bool NativeImageHelpers::SafeLockedAction(NativeImageBase^ ni, 
	Action^ actionIfOK, Action^ defaultAction, int timeOutMillis)
{
	if (ni == nullptr)
	{
#ifdef _DEBUG
		System::Diagnostics::Debug::WriteLine("Warning: trying to use null"
			" image");
#endif

		if (defaultAction != nullptr)
			defaultAction();	

		return false;
	}

	AutoLock lock(ni, timeOutMillis);

	if (!lock.HasLock())
	{
#ifdef _DEBUG
		System::Diagnostics::Debug::WriteLine("Warning: image lock timeout");
#endif

		if (defaultAction != nullptr)
			defaultAction();	

		return false;
	}
		
	if (ni->IsDisposed)
	{
#ifdef _DEBUG
		System::Diagnostics::Debug::WriteLine("Warning: trying to use disposed"
			" image");
#endif

		if (defaultAction != nullptr)
			defaultAction();	
	}
	else
	{
		if (actionIfOK != nullptr)
			actionIfOK();		
	}

	return true;
}

bool NativeImageHelpers::CanConvertTo24BitRGB(NativeImageBase^ ni)
{
	vt::IImageReader* reader = reinterpret_cast<vt::IImageReader*>(
		ni->IImageReader.ToPointer());

	return vt::IsValidConvertPair(reader->GetImgInfo().type, OBJ_RGBIMG);
}

HRESULT CopyReaderToCache(vt::IImageReader* src, vt::CImgInCache& dst)
{
	VT_HR_BEGIN();

	vt::CTransformGraphUnaryNode node;
	
	VT_HR_EXIT(node.BindSourceToReader(src));
	
	vt::CLayerImgInfo dstInfo = dst.GetImgInfo();

	vt::CConvertTransform cvtx(VT_IMG_FIXED(dstInfo.type));
	node.SetTransform(&cvtx);
	
	node.SetDest(&dst);
	
	VT_HR_EXIT(vt::PushTransformTaskAndWait(&node, 
		(vt::CTaskProgress*)NULL));
	
	VT_HR_END(;);
}

void NativeImageHelpers::SaveAs24BitRGB(NativeImageBase^ ni, String^ filename)
{
	VT_HR_BEGIN();

	vt::IImageReader* reader = reinterpret_cast<vt::IImageReader*>(
		ni->IImageReader.ToPointer());
	
	vt::CLayerImgInfo dstInfo = reader->GetImgInfo();
	dstInfo.type = OBJ_RGBIMG;
	
	vt::CImgInCache dst;
	VT_HR_EXIT(dst.Create(dstInfo));

	VT_HR_EXIT(CopyReaderToCache(reader, dst));

	msclr::interop::marshal_context mc;	
	const wchar_t* wcpath = mc.marshal_as<const wchar_t*>(filename);
	VT_HR_EXIT(vt::VtSaveImage(wcpath, &dst));

	VT_HR_EXIT_LABEL();

	ThrowIfHRFailed(hr, true);
}

void NativeImageHelpers::SaveAsVTI(NativeImageBase^ ni, String^ filename)
{
	msclr::interop::marshal_context mc;		
	bool fileCreated = false;
	
	VT_HR_BEGIN();

	vt::IImageReader* reader = reinterpret_cast<vt::IImageReader*>(
		ni->IImageReader.ToPointer());
		
	vt::CImgInCache dst;
	vt::IMG_CACHE_SOURCE_PROPERTIES props;
	props.bAutoLevelGenerate = false;
	props.iMaxLevel = 0;
	props.pBackingBlob = mc.marshal_as<const wchar_t*>(filename);
	vt::CLayerImgInfo dstInfo = reader->GetImgInfo();
	VT_HR_EXIT(dst.Create(dstInfo, &props));
	fileCreated = true;

	VT_HR_EXIT(CopyReaderToCache(reader, dst));

	vt::CImageCache* cache = nullptr;
	VT_PTR_EXIT(cache = vt::VtGetImageCacheInstance());

	VT_HR_EXIT(cache->FlushSource(dst.GetCacheSourceId()));

	VT_HR_EXIT_LABEL();

	if (FAILED(hr))
	{
		if (fileCreated)
			DeleteFile(mc.marshal_as<const wchar_t*>(filename));
	}

	ThrowIfHRFailed(hr, true);
}

bool NativeImageHelpers::CanSaveAsJXR(NativeImageBase^ ni)
{
	vt::IImageReader* reader = reinterpret_cast<vt::IImageReader*>(
		ni->IImageReader.ToPointer());

	auto nb = VT_IMG_BANDS(reader->GetImgInfo().type);
	auto elt = EL_FORMAT(reader->GetImgInfo().type);

	return (nb == 1 || nb == 3 || nb == 4) &&
		(elt == EL_FORMAT_BYTE || elt == EL_FORMAT_SHORT ||
		elt == EL_FORMAT_HALF_FLOAT || elt == EL_FORMAT_FLOAT);
}

void NativeImageHelpers::SaveAsJXR(NativeImageBase^ ni, String^ filename)
{
	VT_HR_BEGIN();

	VT_HR_EXIT(CanSaveAsJXR(ni) ? S_OK : E_FAIL);

	VT_HR_EXIT(
		System::IO::Path::GetExtension(filename)->ToLower() == ".jxr" ?
		S_OK : E_FAIL);

	vt::IImageReader* reader = reinterpret_cast<vt::IImageReader*>(
		ni->IImageReader.ToPointer());
	
	vt::CImg dst;
	VT_HR_EXIT(reader->ReadImg(dst));

	msclr::interop::marshal_context mc;	
	const wchar_t* wcpath = mc.marshal_as<const wchar_t*>(filename);

	vt::CParams p;
    vt::CParamValue v;
	VT_HR_EXIT(v.Set(1.f));
	VT_HR_EXIT(p.SetByName(L"ImageQuality", 0, v));
	
	VT_HR_EXIT(vt::VtSaveHDPhoto(wcpath, dst, true, -1, &p));

	VT_HR_EXIT_LABEL();

	ThrowIfHRFailed(hr, true);
}

IntPtr NativeImageHelpers::CreateCImgInCache()
{
	return IntPtr(new vt::CImgInCache());
}

HRESULT NativeImageHelpers::CreateCImgInCacheImage(IntPtr imgInCachePtr, 
												   int width, int height, 
												   int type)
{
	if (imgInCachePtr.ToPointer() == nullptr)
		return E_FAIL;

	return reinterpret_cast<vt::CImgInCache*>(imgInCachePtr.ToPointer())
		->Create(vt::CImgInfo(width, height, type));
}

END_NI_NAMESPACE