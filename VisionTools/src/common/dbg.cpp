//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Debugging support
//
//  History:
//      2004/11/08-mattu
//          Created
//		2011/06/09-wkienzle
//			Simplified
//
//------------------------------------------------------------------------

#include "vt_dbg.h"
#include "vt_string.h"

#if defined(VT_ANDROID)
#include <android/log.h>
#include <cstdlib>
#endif

using namespace vt;

#if defined(VT_IOS) || defined(VT_ANDROID)
// external log callbacks
VTLogCallbackType vt::g_VTDebugLog = vt::VtDebugLog;
VTLogCallbackType vt::g_VTConsoleLog = vt::VtConsoleLog;
VTLogCallbackType vt::g_VTErrorLog = vt::VtErrorLog;
#endif

void vt::VtDebugLog(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
    vt::string buf;
    buf.format_with_resize(msg, args);
#ifdef _MSC_VER
	OutputDebugStringA(buf.get_constbuffer());	
#elif defined(VT_ANDROID)
    __android_log_write(ANDROID_LOG_VERBOSE, "VisionTools", buf.get_constbuffer());
#elif defined(VT_IOS)
    fprintf(stdout, "%s", buf.get_constbuffer()); fflush(stdout);
#else
#warning DebugLog not implemented yet
#endif
    va_end(args);
}

void vt::VtDebugLog(const wchar_t* msg, ...)
{
	va_list args;
	va_start(args, msg);
    vt::wstring buf;
    buf.format_with_resize(msg, args);
#ifdef _MSC_VER
	OutputDebugStringW(buf.get_constbuffer());
#elif defined(VT_ANDROID)
    char cstr[4096];
    wcstombs(cstr,buf.get_constbuffer(),sizeof(cstr)*sizeof(char));
    __android_log_write(ANDROID_LOG_VERBOSE, "VisionTools", cstr);
#elif defined(VT_IOS)
    fprintf(stdout, "%S", buf.get_constbuffer()); fflush(stdout);
#else
#warning DebugLog not implemented yet
#endif
    va_end(args);
}


void vt::VtConsoleLog(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	vprintf(msg, args);
	va_end(args);
}

void vt::VtConsoleLog(const wchar_t* msg, ...)
{
	va_list args;
	va_start(args, msg);
	vwprintf(msg, args);
	va_end(args);
}


void vt::VtErrorLog(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
}

void vt::VtErrorLog(const wchar_t* msg, ...)
{
	va_list args;
	va_start(args, msg);
	vfwprintf(stderr, msg, args);
	va_end(args);
}
