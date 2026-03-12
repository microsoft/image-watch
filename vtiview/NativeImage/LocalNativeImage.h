#pragma once

#include "Namespace.h"
#include "AutoPtr.h"
#include "NativeImageBase.h"

using namespace System;

BEGIN_NI_NAMESPACE;

public ref class LocalNativeImage : public NativeImageBase
{
public:
	LocalNativeImage(String^ imagePath);
	~LocalNativeImage();
	!LocalNativeImage();

	virtual bool Reload() override;

private:
	HRESULT CreateCacheFromImageFile(const wchar_t* imagePath);
	HRESULT CreateCacheFromBlob(const wchar_t* blobPath);

private:			
	String^ imagePath_;

	AutoPtr<vt::CImgInCache> mipMapCache_;	
	AutoPtr<vt::CImgInCache> vtiBaseCache_;	
};

public ref class LocalNativeMultiFrameImage abstract: public NativeImageBase
{
public:
	property int NumFrames { int get(); }
	virtual property int FrameNumber { int get(); }
	virtual bool TrySetFrameNumber(int number) = 0;
protected:
	int currentFrame_;
	int numFrames_;
};


END_NI_NAMESPACE