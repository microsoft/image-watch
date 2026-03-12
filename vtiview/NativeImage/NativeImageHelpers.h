#pragma once

#include <WinDef.h>
#include <string>
#include "msclr\marshal_cppstd.h"
#include "Namespace.h"
#include "NativeImageBase.h"

using namespace System;
using namespace System::Windows;

BEGIN_NI_NAMESPACE;

public ref class NativeImageHelpers abstract sealed
{
public:
	static void StartVTImageDebugger(bool autoAdd);

	static void SetImageCacheMaxMemoryUsage(double fractionOfSystemMemory);
	static bool SetImageCacheMaxMemoryUsageAbsolute(UInt32 numMegaBytes);

	static float HalfFloatRawBytesToFloat(cli::array<Byte>^ rawBytes, 
		int offset);

	static void SafeLockedAction(NativeImageBase^ ni, Action^ actionIfOK,
		Action^ defaultAction);
	static bool SafeLockedAction(NativeImageBase^ ni, Action^ actionIfOK,
		Action^ defaultAction, int timeOutMillis);

	static bool CanConvertTo24BitRGB(NativeImageBase^ ni);	
	static void SaveAs24BitRGB(NativeImageBase^ ni, String^ filename);
	static void SaveAsVTI(NativeImageBase^ ni, String^ filename);
	static bool CanSaveAsJXR(NativeImageBase^ ni);
	static void SaveAsJXR(NativeImageBase^ ni, String^ filename);

	// These functions are for creating image resources inside *this* module
	// (currently NativeImage.dll) when calling from different modules.
	static IntPtr CreateCImgInCache();
	static HRESULT CreateCImgInCacheImage(IntPtr imgInCachePtr, int width, 
		int height, int type);

internal:
	static String^ HRToString(HRESULT hr, bool isIO);
	static void ThrowIfHRFailed(HRESULT hr, bool isIO);
	static void ThrowIfHRFailed(HRESULT hr);
};
	
namespace Details
{
	inline std::wstring ManagedToNativeString(String^ s)
	{
		if (s == nullptr)
			return std::wstring();

		msclr::interop::marshal_context mc;
		return mc.marshal_as<const wchar_t*>(s);
	}
}

END_NI_NAMESPACE
