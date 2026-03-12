#pragma once

#include "Namespace.h"
#include "AutoPtr.h"
#include "NativeImageBase.h"
#include "LocalNativeImage.h"

using namespace System;

BEGIN_NI_NAMESPACE;

public ref class LocalNativeImageSequence : public LocalNativeMultiFrameImage
{
public:
	LocalNativeImageSequence(String^ path);
	~LocalNativeImageSequence();
	!LocalNativeImageSequence();

	virtual bool Reload() override;
	virtual bool TrySetFrameNumber(int number) override;

	static bool IsMultiFrame(String^ path);

private:
	void ShareCurrentFrame();
	
private:
	String^ path_;
	int width_;
	int height_;	

    AutoPtr<vt::vector<vt::CImgInCache>> images_;
};

END_NI_NAMESPACE