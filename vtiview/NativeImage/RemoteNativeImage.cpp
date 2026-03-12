#include "Stdafx.h"

#include "RemoteNativeImage.h"
#include "NativeImageHelpers.h"

BEGIN_NI_NAMESPACE;

RemoteNativeImage::RemoteNativeImage(int processId, UInt64 pixelAddress, 
	int numStrideBytes, int width, int height, int type)
	: processId_(processId), pixelAddress_(pixelAddress), 
	numStrideBytes_(numStrideBytes), width_(width), height_(height), 
	type_(type)
{
	Reload();
}

RemoteNativeImage::~RemoteNativeImage()
{
	this->!RemoteNativeImage();
}

RemoteNativeImage::!RemoteNativeImage()
{
}

bool RemoteNativeImage::CanReload(int processId, UInt64 pixelAddress, 
	int numStrideBytes, int width, int height, int type)
{
	return processId == processId_ && pixelAddress == pixelAddress_ &&
		numStrideBytes == numStrideBytes_ && width == width_ &&
		height == height_ && type == type_;
}

bool RemoteNativeImage::Reload()
{
	ClearMinMaxValue();

	const int numBufBytes = 1 << 24;
	std::vector<vt::Byte> buf;
	try
	{
		buf.resize(numBufBytes);
	}
	catch (const std::bad_alloc&)
	{
		throw gcnew ApplicationException("Not enough memory to "
			"allocate buffer for ReadProcessMemory()");	
	}

	if (numStrideBytes_ < 1)
		throw gcnew ApplicationException("Invalid stride size");
	const int maxRowsPerBuffer = numBufBytes / numStrideBytes_;

	HANDLE process = 
		OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION |
		SYNCHRONIZE, FALSE, processId_);
	if (NULL == process)
		throw gcnew ApplicationException("Error opening process");

	SetImage(new vt::CImgInCache());

	VT_HR_BEGIN();

	VT_HR_EXIT(static_cast<vt::CImgInCache*>(GetImage())
		->Create(vt::CImgInfo(width_, height_, type_)));

	const unsigned char* readPtr = 
		reinterpret_cast<const unsigned char*>(pixelAddress_);
	for (int numRowsLeft = height_; numRowsLeft > 0;)
	{
		const int numRowsToRead = Math::Min(numRowsLeft, maxRowsPerBuffer);
		const int numBytesToRead = numRowsToRead * numStrideBytes_;

		vt::CImg bufImg;
		VT_HR_EXIT(bufImg.Create(&buf[0], width_, numRowsToRead, 
			numStrideBytes_, type_));

		SIZE_T numBytesRead = 0;
		if (!ReadProcessMemory(process, readPtr, bufImg.BytePtr(), 
			numBytesToRead, &numBytesRead) 
			|| (numBytesRead != (SIZE_T)numBytesToRead))
		{
			throw gcnew ApplicationException("Error reading "
				"process memory");
		}

		VT_HR_EXIT(static_cast<vt::CImgInCache*>(GetImage())
			->WriteRegion(vt::CRect(0, height_ - numRowsLeft, width_, 
			height_ - numRowsLeft + numRowsToRead), bufImg));

		readPtr += numBytesToRead;
		numRowsLeft -= numRowsToRead;
	}

	VT_HR_EXIT(BuildMipMap(0));

	VT_HR_EXIT_LABEL();

	CloseHandle(process);

	NativeImageHelpers::ThrowIfHRFailed(hr);

	return true;
}

END_NI_NAMESPACE;