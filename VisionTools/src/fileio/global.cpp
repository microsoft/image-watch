//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Class for maintinaing global startup/shutdown state for COM or GDIPlus
//
//  History:
//      2004/11/8-swinder
//            Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_global.h"

using namespace vt;

// the global vision tools object
CVtGlobal vt::g_VtGlobal;

////////////////////////////////////////////////////////////////////////
// CVtGlobal
////////////////////////////////////////////////////////////////////////

CVtGlobal::CVtGlobal()
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP)
// windows phone only supports multi thread apartment model
: m_dwCoInit(COINIT_MULTITHREADED)
#else
: m_dwCoInit(COINIT_APARTMENTTHREADED)
#endif
, m_bComStarted(false)
{	
}

CVtGlobal::~CVtGlobal()
{
    ShutdownCOM();
}

void CVtGlobal::SetThreadingModel(DWORD dwCoInit)
{
    if(!m_bComStarted)
        m_dwCoInit = dwCoInit;
}

void CVtGlobal::StartCOM()
{
    if(!m_bComStarted)
    {
        HRESULT hr = CoInitializeEx(NULL, m_dwCoInit);
        if(SUCCEEDED(hr))
            m_bComStarted = true;
    }
}

void CVtGlobal::ShutdownCOM()
{
    if(m_bComStarted)
    {
        CoUninitialize();
        m_bComStarted = false;
    }
}

void vt::VtStartCOM()
{
    g_VtGlobal.StartCOM();
}

void vt::VtShutdownCOM()
{
    g_VtGlobal.ShutdownCOM();
}

