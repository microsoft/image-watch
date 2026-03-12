//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      stdafx for file IO project
//
//  History:
//      2004/11/8-swinder
//            Created
//
//------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once

#define _FILEIO_STDAFX_H_

#include "vtfileio.h"

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY!=WINAPI_FAMILY_PHONE_APP)
#if !defined(VT_OSX)
#include <mfidl.h>
#include <mferror.h>
// include WIN8 specific content from mfreadwrite - there is a runtime
// os version check that guards it's use; the WIN8 SDK appears to always
// be installed with VC11, so no need to check the SDK version
#if ( (_MSC_VER >= 1700) && !defined(VT_WINRT) )
#pragma push_macro("WINVER")
#undef WINVER
#define WINVER _WIN32_WINNT_WIN8
#include <mfreadwrite.h>
#pragma pop_macro("WINVER")
#else
#include <mfreadwrite.h>
#endif


#endif
#endif
