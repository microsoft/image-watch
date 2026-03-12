//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//------------------------------------------------------------------------
#pragma once

#define VT_VERSION "1.2"

#ifndef VT_INCLUDED
#define VT_INCLUDED
#endif

// transforms are not implemented in non-Windows builds
#if !defined(_MSC_VER)
#define VT_NO_XFORMS 
#endif

#if defined(__APPLE__)
#define VT_OSX // currently we define VT_OSX for both iOS and Mac
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE == 1
/* iOS on iPhone, iPad, etc. */
#define VT_IOS
#elif TARGET_OS_MAC == 1
/* OSX */
#endif
#endif

#if defined(__ANDROID__)
#define VT_ANDROID
#endif

// BEGIN RAZZLE STL 110 BUG WORKAROUND (build team referred to this bug: http://bugcheck/bugs/Windows8Bugs/38025)
#if defined(_MSC_VER)
#include <crtdefs.h>
#   ifndef __STR2WSTR 
#     define __STR2WSTR(str)    L##str
#   endif
#   ifndef _STR2WSTR
#     define _STR2WSTR(str)     __STR2WSTR(str)
#   endif
#   ifndef __FILEW__
#     define __FILEW__          _STR2WSTR(__FILE__)
#   endif
#   ifndef __FUNCTIONW__
#     define __FUNCTIONW__          _STR2WSTR(__FUNCTION__)
#   endif

_CRTIMP __declspec(noreturn)
void __cdecl _invoke_watson(_In_opt_z_ const wchar_t *, _In_opt_z_ const wchar_t *, _In_opt_z_ const wchar_t *, _In_ unsigned int, _In_ uintptr_t);

#ifdef _DEBUG 
 #ifndef _CRT_SECURE_INVALID_PARAMETER
  #define _CRT_SECURE_INVALID_PARAMETER(expr) ::_invalid_parameter(__STR2WSTR(#expr), __FUNCTIONW__, __FILEW__, __LINE__, 0)
#endif
#else
/* By default, _CRT_SECURE_INVALID_PARAMETER in retail invokes _invalid_parameter_noinfo_noreturn(),
  * which is marked __declspec(noreturn) and does not return control to the application. Even if 
  * _set_invalid_parameter_handler() is used to set a new invalid parameter handler which does return
  * control to the application, _invalid_parameter_noinfo_noreturn() will terminate the application and
  * invoke Watson. You can overwrite the definition of _CRT_SECURE_INVALID_PARAMETER if you need.
  *
  * _CRT_SECURE_INVALID_PARAMETER is used in the Standard C++ Libraries and the SafeInt library.
  */
#ifndef _CRT_SECURE_INVALID_PARAMETER
  #define _CRT_SECURE_INVALID_PARAMETER(expr) ::_invalid_parameter_noinfo_noreturn()
#endif
#endif
#endif
// END RAZZLE WORKAROUND

#include "../src/common/vt_basetypes.h"
#include "../src/common/vt_cpudetect.h"
#include "../src/common/vt_intrinsics.h"
#include "../src/common/vt_dbg.h"
#include "../src/common/vt_error.h"
#include "../src/common/vt_new.h"
#include "../src/common/vt_memory.h"
#include "../src/common/vt_simdreg.h"
#include "../src/common/vt_system.h"
#include "../src/common/vt_com.h"

#ifndef VT_NO_VECTORSTRING
#include "../src/common/vt_vector.h"
#include "../src/common/vt_string.h"
#endif

