//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Mixed utility functions
//
//  History:
//      2003/11/12-swinder/mattu
//			Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_utils.h"

const WCHAR * vt::VtGetFileExt(const WCHAR * pchFile)
{
    const WCHAR *pch, *pchDot = NULL, *pchSep = NULL;

    for(pch = pchFile; *pch != L'\0'; pch++)
    {
        if(*pch == L'.')
            pchDot = pch;
        else if(*pch == L'\\')
            pchSep = pch;
    }

    if(pchDot==NULL)
        return pch;
    if(pchSep!=NULL && pchSep > pchDot)
        return pch;
    return pchDot;
}
