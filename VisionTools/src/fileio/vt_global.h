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

#pragma once

#include "vtcommon.h"

namespace vt {

class CVtGlobal
{
public:
    CVtGlobal();
    ~CVtGlobal();

    void SetThreadingModel(DWORD dwCoInit);

    // call any time you need to use com
    void StartCOM();

    // you dont need to call these
    // call only if you need to control exactly when shutdown occurs
    void ShutdownCOM();

    DWORD m_dwCoInit;
    bool m_bComStarted;
};

extern CVtGlobal g_VtGlobal;

extern void VtStartCOM();
extern void VtShutdownCOM();

};
