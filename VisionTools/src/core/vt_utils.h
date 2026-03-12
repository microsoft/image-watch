//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Mixed utility functions
//
//  History:
//      2003/11/12-swinder/mattu
//          Created
//
//------------------------------------------------------------------------
#pragma once

#include "vtcommon.h"

namespace vt {

template <class t>
inline t *PointerOffset(t *p, int iByteOffset)
{
    return (t *)((Byte *)p + iByteOffset);
}

template <class t>
inline const t *PointerOffset(const t *p, int iByteOffset)
{
    return (const t *)((const Byte *)p + iByteOffset);
}

const WCHAR * VtGetFileExt(const WCHAR * pchFile);

};
