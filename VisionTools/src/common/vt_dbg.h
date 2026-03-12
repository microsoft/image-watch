//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Debugging support.
//
//  History:
//      2004/11/08-mattu
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vt_basetypes.h"

#if !defined(_MSC_VER)
#include <assert.h>
#define _ASSERT assert
#else
#include <crtdbg.h>
#endif

// warning format for Visual Studio
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "(" __STR1__(__LINE__)")"
#define __WARNING__ __LOC__" : warning: "
// #pragma message(__LOC__"Missing implementation for this target platform")

namespace vt {

#ifdef __analysis_assume
#define ANALYZE_ASSUME(expr) __analysis_assume(expr)
#else
#define ANALYZE_ASSUME(expr)
#endif

#ifdef _DEBUG
#define VT_ASSERT(exp) { _ASSERT(exp); ANALYZE_ASSUME(exp); }
#define VT_VERIFY(exp) _ASSERT(exp)
#else
#define VT_ASSERT(exp) ANALYZE_ASSUME(exp)
#define VT_VERIFY(exp) exp
#endif

#if defined(_DEBUG) || defined(VT_BOUNDS_CHECKING)
#define VT_ASSERT_BOUNDS(exp) _ASSERT(exp)
#else
#define VT_ASSERT_BOUNDS(exp) 
#endif


// callback type for logging error reports to external logger
typedef void (*VTLogCallbackType)(const char* msg, ...);
    
#if defined(VT_IOS) || defined(VT_ANDROID)
extern VTLogCallbackType g_VTDebugLog;
extern VTLogCallbackType g_VTConsoleLog;
extern VTLogCallbackType g_VTErrorLog;
#endif
    
//Log to IDE debug window
void VtDebugLog(const char* msg, ...);
void VtDebugLog(const wchar_t* msg, ...);

#if defined(VT_IOS) || defined(VT_ANDROID)
#define VT_DEBUG_LOG_HR(hr) (*vt::g_VTDebugLog)(__LOC__ " : HRESULT = 0x%08x\n", hr)
#define VT_DEBUG_LOG(fmt, ...) (*vt::g_VTDebugLog)(fmt, __VA_ARGS__)
#else
#ifdef _DEBUG
#define VT_DEBUG_LOG_HR(hr) vt::VtDebugLog(__LOC__ " : HRESULT = 0x%08x\n", hr)
#define VT_DEBUG_LOG(fmt, ...) vt::VtDebugLog(fmt, __VA_ARGS__)
#else
#define VT_DEBUG_LOG_HR(hr)
#define VT_DEBUG_LOG(fmt, ...) 
#endif
#endif
    
//Log to stdout
void VtConsoleLog(const char* msg, ...);
void VtConsoleLog(const wchar_t* msg, ...);
    
#if defined(VT_IOS) || defined(VT_ANDROID)
#define VT_CONSOLE_LOG_HR(hr) (*vt::g_VTConsoleLog)(__LOC__ " : HRESULT = 0x%08x\n", hr)
#define VT_CONSOLE_LOG(fmt, ...) (*vt::g_VTConsoleLog)(fmt, __VA_ARGS__)
#else
#define VT_CONSOLE_LOG_HR(hr) vt::VtConsoleLog(__LOC__ " : HRESULT = 0x%08x\n", hr)
#define VT_CONSOLE_LOG(fmt, ...) vt::VtConsoleLog(fmt, __VA_ARGS__)
#endif
    
#if defined(VT_IOS) || defined(VT_ANDROID)
#define VT_DEBUG_CONSOLE_LOG_HR(hr) (*vt::g_VTConsoleLog)(__LOC__ " : HRESULT = 0x%08x\n", hr)
#define VT_DEBUG_CONSOLE_LOG(fmt, ...) (*vt::g_VTConsoleLog)(fmt, __VA_ARGS__)
#else
//Log to stdout - only in debug mode
#ifdef _DEBUG
#define VT_DEBUG_CONSOLE_LOG_HR(hr) vt::VtConsoleLog(__LOC__ " : HRESULT = 0x%08x\n", hr)
#define VT_DEBUG_CONSOLE_LOG(fmt, ...) vt::VtConsoleLog(fmt, __VA_ARGS__)
#else
#define VT_DEBUG_CONSOLE_LOG_HR(hr)
#define VT_DEBUG_CONSOLE_LOG(fmt, ...)
#endif
#endif
    
//Log to stderr
void VtErrorLog(const char* msg, ...);
void VtErrorLog(const wchar_t* msg, ...);
    
#if defined(VT_IOS) || defined(VT_ANDROID)
#define VT_ERROR_LOG_HR(hr) (*vt::g_VTErrorLog)(__LOC__ " : HRESULT = 0x%08x\n", hr)
#define VT_ERROR_LOG(fmt, ...) (*vt::g_VTErrorLog)(fmt, __VA_ARGS__)
#else
#define VT_ERROR_LOG_HR(hr) vt::VtErrorLog(__LOC__ " : HRESULT = 0x%08x\n", hr)
#define VT_ERROR_LOG(fmt, ...) vt::VtErrorLog(fmt, __VA_ARGS__)
#endif

#define VT_USE(x) (void)(x)
};