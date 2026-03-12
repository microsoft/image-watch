//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      VisionTools new operator
//
//  History:
//      2004/11/08-mattu
//			Created
//
//------------------------------------------------------------------------

#pragma once

#include <new>

#define VT_PLACENEW(P) new((void*)P)

#if defined(_MSC_VER)
namespace vt 
{
    static const std::nothrow_t nothrow; // local copy to avoid warning LNK4049 in unoptimized (ENABLE_OPTIMIZER=0) builds
}
#define VT_NOTHROWNEW  new(vt::nothrow)
#else
#define VT_NOTHROWNEW  new(std::nothrow)
#endif