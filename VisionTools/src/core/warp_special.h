//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      specialized implementation of image warp
//
//------------------------------------------------------------------------------
#pragma once

#include "vtcommon.h"

typedef HRESULT (VT_STDCALL *WarpBlockFunc)(vt::CImg& imgDstBlk, const vt::CPoint& ptDst, 
    const vt::CImg& imgSrc, const vt::CMtx3x3f* pMtx, const vt::IMAGE_EXTEND& ex);

// from warp_special.cpp
extern HRESULT VT_STDCALL WarpBlock_4Band_Byte_Affine_Bilerp_Src32768_ExZorC(
    vt::CImg& imgDstBlk, const vt::CPoint& ptDst, 
    const vt::CImg& imgSrc,
    const vt::CMtx3x3f* pMtx, const vt::IMAGE_EXTEND& ex);
extern HRESULT VT_STDCALL WarpBlock_1Band_Byte_Affine_Bilerp_Src32768_ExZorC(
    vt::CImg& imgDstBlk, const vt::CPoint& ptDst, 
    const vt::CImg& imgSrc,
    const vt::CMtx3x3f* pMtx, const vt::IMAGE_EXTEND& ex);
extern HRESULT VT_STDCALL WarpBlock_2Band_Byte_Affine_Bilerp_Src32768_ExZorC(
    vt::CImg& imgDstBlk, const vt::CPoint& ptDst, 
    const vt::CImg& imgSrc,
    const vt::CMtx3x3f* pMtx, const vt::IMAGE_EXTEND& ex);

// check parameters to Warp and see if one of the warp-special routines can
// be used; returned function pointer is NULL if not
inline void SelectWarpSpecial(WarpBlockFunc& wbf,
    vt::CImg& imgDst, const vt::CRect& rctDst, const vt::CImg& imgSrc, 
    vt::eSamplerKernel sampler, const vt::IMAGE_EXTEND& ex, const vt::CMtx3x3f* pMtx)
{
    wbf = NULL;
    if (
#if (defined(_M_IX86) || defined(_M_AMD64))
         vt::g_SupportSSSE3() &&
#endif
         pMtx && IsMatrixAffine(*pMtx,rctDst) &&
        (EL_FORMAT(imgDst.GetType()) == OBJ_BYTEIMG) &&
        (EL_FORMAT(imgSrc.GetType()) == OBJ_BYTEIMG) &&
        (sampler == vt::eSamplerKernelBilinear) &&
        (ex.IsVerticalSameAsHorizontal()) &&
        ( (ex.exHoriz == vt::Zero) || (ex.exHoriz == vt::Constant) )
        )
    {
        if      ((imgDst.Bands() == 4) && 
                 (imgSrc.Bands() == 4) && 
                 ((imgSrc.StrideBytes()>>2) < 0x8000)) 
            { wbf = WarpBlock_4Band_Byte_Affine_Bilerp_Src32768_ExZorC; }
        else if ((imgDst.Bands() == 1) && 
                 (imgSrc.Bands() == 1) && 
                 ((imgSrc.StrideBytes()>>0) < 0x8000)) 
            { wbf = WarpBlock_1Band_Byte_Affine_Bilerp_Src32768_ExZorC; }
        else if ((imgDst.Bands() == 2) && 
                 (imgSrc.Bands() == 2) && 
                 ((imgSrc.StrideBytes()>>1) < 0x8000)) 
            { wbf = WarpBlock_2Band_Byte_Affine_Bilerp_Src32768_ExZorC; }
    }
}


