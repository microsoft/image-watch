//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for detecting the presence of MMX or SSE1/2 capabilities
//
//  History:
//      2004/11/08-swinder
//			Created
//
//------------------------------------------------------------------------

#pragma once

namespace vt {

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(_M_IX86) || defined(_M_AMD64)) || defined(VT_ANDROID)
bool g_SupportMMX (void);
bool g_SupportSSE1 (void);
bool g_SupportSSE2 (void);
bool g_SupportSSE3 (void);
bool g_SupportSSSE3 (void);
bool g_SupportSSE4_1 (void);
bool g_SupportSSE4_2 (void);
bool g_SupportAVX (void);
bool g_SupportAVXwF16C (void);
bool g_SupportPOPCNT (void);
bool g_SupportMVI (void);
bool g_SupportCMOV (void);
bool cpuidIsIntelP5 (void); // To detect Intel P5
#else
#pragma warning( disable : 4127 )
#define g_SupportMMX()    false
#define g_SupportSSE1()   false
#define g_SupportSSE2()   false
#define g_SupportSSE3()   false
#define g_SupportSSSE3()  false
#define g_SupportSSE4_1() false
#define g_SupportSSE4_2() false
#define g_SupportAVX()    false
#define g_SupportAVXwF16C() false
#define g_SupportPOPCNT() false
#define g_SupportMVI()    false
#define g_SupportCMOV()   false
#define cpuidIsIntelP5()  false
#endif

#ifdef __cplusplus
}
#endif

};
