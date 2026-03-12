//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Main include file for the file IO library
//
//  History:
//      2004/11/8-swinder
//            Created
//
//------------------------------------------------------------------------
#pragma once

#if !defined(VT_WINRT)
#pragma comment(lib, "vfw32.lib")
#pragma comment(lib, "mscms.lib")
#endif
#pragma comment(lib, "windowscodecs.lib")

#if !defined(VT_OSX) && !defined(VT_ANDROID)
#include <mfapi.h>
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY!=WINAPI_FAMILY_PHONE_APP)
#pragma comment(lib, "mfuuid.lib")
#endif

#if defined(VT_WINRT)
#undef WINVER
#define WINVER _WIN32_WINNT_WINBLUE
#endif

#include "vtcore.h"

#if !defined(VT_WINRT)
#include <vfw.h>
#include <icm.h>
#pragma warning( push )
#pragma warning( disable : 6387 )
#include <thumbcache.h>
#pragma warning( pop )
#endif

#pragma warning( push )
#pragma warning( disable : 4005 )
#include <wincodec.h>
#include <wincodecsdk.h>
#pragma warning( pop )

#ifndef _FILEIO_STDAFX_H_

#include "..\src\fileio\vt_global.h"
#include "..\src\fileio\vt_iointerfaces.h"
#include "..\src\fileio\vt_io.h"
#include "..\src\fileio\wicio.h"

#endif
#endif

#if defined(VT_OSX)
#include "../src/fileio/vt_io.h"
#include "../VisionToolsMac/VisionToolsMac/AVVideoSrc.h"
#include "../VisionToolsMac/VisionToolsMac/AVVideoDst.h"
#endif