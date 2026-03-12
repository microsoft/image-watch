#pragma once

#include "Namespace.h"
#include "NativeImageBase.h"

using namespace System;

BEGIN_NI_NAMESPACE;

public ref class RemoteNativeImage : public NativeImageBase
{
public:
	RemoteNativeImage(int processId, UInt64 pixelAddress, 
		int numStrideBytes, int width, int height, int type);

	~RemoteNativeImage();
	!RemoteNativeImage();

	bool CanReload(int processId, UInt64 pixelAddress, 
		int numStrideBytes, int width, int height, int type);

	virtual bool Reload() override;

	property int RemoteNumStrideBytes
	{
		int get()
		{
			return numStrideBytes_;
		}
	}

	property UInt64 RemotePixelAddress
	{
		UInt64 get()
		{
			return pixelAddress_;
		}
	}

private:
	int processId_;
	UInt64 pixelAddress_;
	int numStrideBytes_;
	int width_;
	int height_;
	int type_;
};

END_NI_NAMESPACE;