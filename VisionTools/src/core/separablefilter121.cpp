//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Separable filter specialization for 121 kernel
//
//  History:
//      2011/8/31-v-mitoel
//          Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "../common/vt_intrinsics.h"
#include "vt_image.h"
#include "vt_utils.h"
#include "vt_kernel.h"
#include "vt_separablefilter.h"
#include "vt_convert.h"
#include "vt_convert.inl"
#include "vt_pixeliterators.h"
#include "vt_pad.h"

using namespace vt;

// apply 121 filter via byte average equivalent to
// SSE _mm_avg_epu8 instruction
inline Byte filter121ByteAvg(Byte l, Byte c, Byte r)
{
    Byte cavg = (c + c + 1) >> 1;
    Byte lravg = (l + r + 1) >> 1;
    return ( (cavg + lravg + 1) >> 1);
}

inline UInt16 filter121A(Byte t, Byte c, Byte b)
{
    // inputs are 8.0; return is 8.2
    UInt16 tmp = (UInt16)t + (((UInt16)c)<<1) + (UInt16)b;
    return tmp;
}
inline Byte filter121B(UInt16 l, UInt16 c, UInt16 r)
{
    // inputs are 8.2; tmp is 8.4
    UInt16 tmp = l + (c<<1) + r + 0x8; // add 0.5 (0x8) to round up
    return (Byte)(tmp>>4); // truncate to return 8.0 value
}
inline float filter121A(float t, float c, float b)
{
    return (t + c + c + b) * .25f;
}
inline float filter121B(float l, float c, float r)
{
    return (l + c + c + r) * .25f;
}

typedef struct { Byte b3,b2,b1,b0; } Bytex4;
#if defined(VT_SSE_SUPPORTED)
#include "x86/separablefilter121_sse.h"
#elif defined(VT_NEON_SUPPORTED)
#include "arm/separablefilter121_neon.h"
#endif

template<typename TD, typename TS>
static void VerticalAxis121Filter(
    _In_range_(>=,0) int nBnds, _In_range_(>=,0) int srcBndsXWidth, int xs, int ys, bool bRExt,
    const CImg& imgSrc, 
    _Out_writes_bytes_((srcBndsXWidth+(2*nBnds))*sizeof(TD)) void* slbufBase
    )
{
    // pC points to first non-padding source pixel for row being filtered
    const TS* pC = (TS*)imgSrc.BytePtr(xs, ys); 
    const TS* pT = (ys>0)?(pC-(imgSrc.StrideBytes()/sizeof(TS))):(pC);
    const TS* pB = (ys<(imgSrc.Height()-1))?(pC+(imgSrc.StrideBytes()/sizeof(TS))):(pC);
    TD* pTDst = (TD*)slbufBase;

    // apply filter to left extend area (does unaligned writes)
    for (int b=0; b<nBnds; b++)
    {
        int offs = (xs>0)?(-nBnds+b):(b);
        *pTDst++ = filter121A(*(pT+offs),*(pC+offs),*(pB+offs));
    }
    // apply filter to dst image bands*width
    int w = srcBndsXWidth;

#if defined(VT_SSE_SUPPORTED) && !defined(VT_NEON_SUPPORTED)
    if ( !g_SupportSSE3() )
    {
        // no SSE or Neon support: unroll to allow compiler to reuse loads
        while (w >= 8)
        {
            pTDst[0] = filter121A(pT[0],pC[0],pB[0]);
            pTDst[1] = filter121A(pT[1],pC[1],pB[1]);
            pTDst[2] = filter121A(pT[2],pC[2],pB[2]);
            pTDst[3] = filter121A(pT[3],pC[3],pB[3]);
            pTDst[4] = filter121A(pT[4],pC[4],pB[4]);
            pTDst[5] = filter121A(pT[5],pC[5],pB[5]);
            pTDst[6] = filter121A(pT[6],pC[6],pB[6]);
            pTDst[7] = filter121A(pT[7],pC[7],pB[7]);
            pTDst+=8; pT+=8; pC+=8;  pB+=8; w-=8;
        }
    }
#endif
#if defined(VT_SSE_SUPPORTED)
    if ( g_SupportSSE3() )
    {
        VFilter_SSE3(pC,pT,pB,pTDst,w);
    }
#elif defined(VT_NEON_SUPPORTED)
    {
        VFilter_Neon(pC,pT,pB,pTDst,w);
    }
#endif
    // scalar processsing for remainder
    while (w > 0)
    {
        *pTDst++ = filter121A(*pT++,*pC++,*pB++);
        w--;
    }
    // apply filter to right extend area
    for (int b=0; b<nBnds; b++)
    {
        int offs = (bRExt)?(-nBnds+b):(b);
        *pTDst++ = filter121A(*(pT+offs),*(pC+offs),*(pB+offs));
    }
}

template<typename TD, typename TS>
static inline void HorizontalAxis121Filter(int nBnds, int srcBndsXWidth,
    bool bDeci, int y, CImg& imgDst, void* slbufBase)
{
    const TS* pL = (TS*)slbufBase; // points to first 'Extend' pixel
    TD* pDst = (TD*)imgDst.BytePtr(0, y);
    int w = srcBndsXWidth; // total number of channels
    if (bDeci)
    {
#if defined(VT_SSE_SUPPORTED)
        if ( g_SupportSSE2() )
        {
            HFilter_Deci_SSE2(pDst, pL, w, nBnds);
        }
#endif
        if (nBnds == 1)
        {
            while (w >= 16)
            {
                pDst[0] = filter121B(pL[ 0],pL[ 0+1],pL[ 0+(2*1)]);
                pDst[1] = filter121B(pL[ 2],pL[ 2+1],pL[ 2+(2*1)]);
                pDst[2] = filter121B(pL[ 4],pL[ 4+1],pL[ 4+(2*1)]);
                pDst[3] = filter121B(pL[ 6],pL[ 6+1],pL[ 6+(2*1)]);
                pDst[4] = filter121B(pL[ 8],pL[ 8+1],pL[ 8+(2*1)]);
                pDst[5] = filter121B(pL[10],pL[10+1],pL[10+(2*1)]);
                pDst[6] = filter121B(pL[12],pL[12+1],pL[12+(2*1)]);
                pDst[7] = filter121B(pL[14],pL[14+1],pL[14+(2*1)]);
                pDst += 8;
                pL += 16;
                w -= 16;
            }
        }
        else if (nBnds == 2)
        {
            while (w >= 16)
            {
                pDst[0] = filter121B(pL[ 0],pL[ 0+2],pL[ 0+(2*2)]);
                pDst[1] = filter121B(pL[ 1],pL[ 1+2],pL[ 1+(2*2)]);
                pDst[2] = filter121B(pL[ 4],pL[ 4+2],pL[ 4+(2*2)]);
                pDst[3] = filter121B(pL[ 5],pL[ 5+2],pL[ 5+(2*2)]);
                pDst[4] = filter121B(pL[ 8],pL[ 8+2],pL[ 8+(2*2)]);
                pDst[5] = filter121B(pL[ 9],pL[ 9+2],pL[ 9+(2*2)]);
                pDst[6] = filter121B(pL[12],pL[12+2],pL[12+(2*2)]);
                pDst[7] = filter121B(pL[13],pL[13+2],pL[13+(2*2)]);
                pDst += 8;
                pL += 16;
                w -= 16;
            }
        }
        else if (nBnds == 3)
        {
            while (w >= 18)
            {
                pDst[0] = filter121B(pL[ 0],pL[ 0+3],pL[ 0+(2*3)]);
                pDst[1] = filter121B(pL[ 1],pL[ 1+3],pL[ 1+(2*3)]);
                pDst[2] = filter121B(pL[ 2],pL[ 2+3],pL[ 2+(2*3)]);
                pDst[3] = filter121B(pL[ 6],pL[ 6+3],pL[ 6+(2*3)]);
                pDst[4] = filter121B(pL[ 7],pL[ 7+3],pL[ 7+(2*3)]);
                pDst[5] = filter121B(pL[ 8],pL[ 8+3],pL[ 8+(2*3)]);
                pDst[6] = filter121B(pL[12],pL[12+3],pL[12+(2*3)]);
                pDst[7] = filter121B(pL[13],pL[13+3],pL[13+(2*3)]);
                pDst[8] = filter121B(pL[14],pL[14+3],pL[14+(2*3)]);
                pDst += 9;
                pL += 18;
                w -= 18;
            }
        }
        else if (nBnds == 4)
        {
            while (w >= 16)
            {
                pDst[0] = filter121B(pL[ 0],pL[ 0+4],pL[ 0+(2*4)]);
                pDst[1] = filter121B(pL[ 1],pL[ 1+4],pL[ 1+(2*4)]);
                pDst[2] = filter121B(pL[ 2],pL[ 2+4],pL[ 2+(2*4)]);
                pDst[3] = filter121B(pL[ 3],pL[ 3+4],pL[ 3+(2*4)]);
                pDst[4] = filter121B(pL[ 8],pL[ 8+4],pL[ 8+(2*4)]);
                pDst[5] = filter121B(pL[ 9],pL[ 9+4],pL[ 9+(2*4)]);
                pDst[6] = filter121B(pL[10],pL[10+4],pL[10+(2*4)]);
                pDst[7] = filter121B(pL[11],pL[11+4],pL[11+(2*4)]);
                pDst += 8;
                pL += 16;
                w -= 16;
            }
        }
    }
    else
    {
        // not decimated
#if defined(VT_SSE_SUPPORTED)
        if ( g_SupportSSE3() ) 
        {
            HFilter_SSE3(pDst, pL, w, nBnds);
        }
#elif defined(VT_NEON_SUPPORTED)
        HFilter_Neon(pDst, pL, w, nBnds);
#endif
        {
            // unrolling to allow compiler to reuse loads
            while (w >= 8)
            {
                pDst[0] = filter121B(pL[0],pL[0+nBnds],pL[0+(2*nBnds)]);
                pDst[1] = filter121B(pL[1],pL[1+nBnds],pL[1+(2*nBnds)]);
                pDst[2] = filter121B(pL[2],pL[2+nBnds],pL[2+(2*nBnds)]);
                pDst[3] = filter121B(pL[3],pL[3+nBnds],pL[3+(2*nBnds)]);
                pDst[4] = filter121B(pL[4],pL[4+nBnds],pL[4+(2*nBnds)]);
                pDst[5] = filter121B(pL[5],pL[5+nBnds],pL[5+(2*nBnds)]);
                pDst[6] = filter121B(pL[6],pL[6+nBnds],pL[6+(2*nBnds)]);
                pDst[7] = filter121B(pL[7],pL[7+nBnds],pL[7+(2*nBnds)]);
                pDst += 8;
                pL += 8;
                w -= 8;
            }
        }
    }

    // scalar processsing for remainder
    if (bDeci)
    {
        while (w > 0)
        {
            *pDst++ = filter121B(pL[0],pL[nBnds],pL[2*nBnds]);
            // skip to next pixel if on pixel boundary or just step 1 channel
            if (nBnds == 1) { pL+= 2; }
            else { pL += (((w>>1) % nBnds)==1) ? (nBnds+1) : 1; }
            w-=2;
        }
    }
    else
    {
        while (w > 0)
        {
            *pDst++ = filter121B(pL[0],pL[nBnds],pL[2*nBnds]);
            pL++;
            w--;
        }
    }
}

//
// This code operates on one row at a time, sourcing that row plus the two
// vertically-adjacent rows to form a single vertically-filtered temporary
// scanline (expanded by one pixel left and right), then (horizontally)
// sourcing the temporary scanline to generate the final result for the
// destination scanline.
//
// bDeci: do 2::1 decimation
HRESULT
VtSeparableFilter121Internal(
    CImg& imgDst, const CRect& rctDst, 
	const CImg& imgSrc, const CPoint ptSrcOrigin,
    bool bDeci, bool bLowBytePrecision)
{
    //printf("(%3d,%3d) (%3d,%3d,%3d,%3d (%3d,%3d)) (%3d,%3d) (%4d,%4d)\n",
    //    imgDst.Width(),imgSrc.Height(), 
    //    rctDst.left,rctDst.right,rctDst.top,rctDst.bottom,rctDst.Width(),rctDst.Height(),
    //    imgSrc.Width(),imgSrc.Height(),
    //    ptSrcOrigin.x,ptSrcOrigin.y);
	VT_HR_BEGIN()

    const int nBnds = imgSrc.Bands();
    int srcBndsXWidth = imgDst.Width()*nBnds;
    if (bDeci) { srcBndsXWidth *= 2; }
    bool bIsByte = (EL_FORMAT(imgSrc.GetType()) == OBJ_BYTEIMG); // else is float

    // position of first non-padding source pixel used for this dest rect
    CPoint npSrcOrigin = CPoint(
        (rctDst.left<<(bDeci?1:0))-ptSrcOrigin.x, 
        (rctDst.top<<(bDeci?1:0))-ptSrcOrigin.y);

    // true if extend mode padding needs to be applied at the right boundary
    bool bRExt = (!bDeci)&&((npSrcOrigin.x+rctDst.Width()) == imgSrc.Width());

    //-------------------------------------------------------------------------
    // special case code for 1 band byte and low precision
    //-------------------------------------------------------------------------
    if ( bLowBytePrecision && bIsByte && (nBnds == 1) )
    {
        int ys = npSrcOrigin.y;
        int yd = 0;
        int srcStride = imgSrc.StrideBytes();
        if (bDeci)
        {
            while ((rctDst.Height()-yd) >= 4 &&
				   (imgSrc.Height()-ys) >= 8)
            {
                const Byte* pC = imgSrc.BytePtr(npSrcOrigin.x, ys);
                const Byte* pT = (ys>0)?(pC-srcStride):(pC);
                const Byte* pB = pC+(7*srcStride);
                Byte* pDst = imgDst.BytePtr(0, yd);
                int w = imgDst.Width();
                int offs = (npSrcOrigin.x == 0)?(0):(-1);

                // compute leftmost vertically-filtered pixels
                Bytex4 Fst;
                {
                    const Byte* pCt = pC+srcStride;
                    Fst.b0 = filter121ByteAvg(*(pT+offs),*(pC+offs),*(pCt+offs));
                    Fst.b1 = filter121ByteAvg(*((pCt)+offs),*((pCt+srcStride)+offs),*((pCt+(2*srcStride))+offs)); pCt += 2*srcStride;
                    Fst.b2 = filter121ByteAvg(*((pCt)+offs),*((pCt+srcStride)+offs),*((pCt+(2*srcStride))+offs)); pCt += 2*srcStride;
                    Fst.b3 = filter121ByteAvg(*((pCt)+offs),*((pCt+srcStride)+offs),*(pB+offs));
                }
                int i = 0;
#if defined(VT_SSE_SUPPORTED)
                if ( g_SupportSSSE3() )
                {
                    if (w >= 8)
                    {
                        __SelectSSEFuncL( 
                            IsAligned16(pT)&&IsAligned16(srcStride), 
                            i, Mono_1BandByte8x4_Deci_SSSE3,
                            pDst, imgDst.StrideBytes(), Fst, w, pT, pC, srcStride );
                    }
                }
#elif defined(VT_NEON_SUPPORTED)
                {
                    if (w >= 8)
                    {
                        i = Mono_1BandByte8x4_Deci_Neon( 
                            pDst, imgDst.StrideBytes(), Fst, w, pT, pC, srcStride);
                    }
                }
#endif
                // do x4 vector remainders
                w -= i;
                if (w > 0)
                {
                    if (i > 0)
                    { 
                        // update src and dst pointers if any vector remainder
                        pDst += i; pT += 2*i; pC += 2*i; pB += 2*i;
                        offs = -1; // not first pixel in row
                        const Byte* pCt = pC+srcStride;
                        Fst.b0 = filter121ByteAvg(*(pT+offs),*(pC+offs),*(pCt+offs));
                        Fst.b1 = filter121ByteAvg(*((pCt)+offs),*((pCt+srcStride)+offs),*((pCt+(2*srcStride))+offs)); pCt += 2*srcStride;
                        Fst.b2 = filter121ByteAvg(*((pCt)+offs),*((pCt+srcStride)+offs),*((pCt+(2*srcStride))+offs)); pCt += 2*srcStride;
                        Fst.b3 = filter121ByteAvg(*((pCt)+offs),*((pCt+srcStride)+offs),*(pB+offs));
                    }
                    Bytex4 L,C,R;
                    R = Fst;
                    while (w > 0)
                    {
                        L = R;
                        {
                            const Byte* pCt = pC+srcStride;
                            C.b0 = filter121ByteAvg(*(pT),*(pC),*(pCt));
                            C.b1 = filter121ByteAvg(*(pCt),*(pCt+srcStride),*(pCt+(2*srcStride))); pCt += 2*srcStride;
                            C.b2 = filter121ByteAvg(*(pCt),*(pCt+srcStride),*(pCt+(2*srcStride))); pCt += 2*srcStride;
                            C.b3 = filter121ByteAvg(*(pCt),*(pCt+srcStride),*(pB));
                            pT++; pC++; pB++;
                        }
                        {
                            const Byte* pCt = pC+srcStride;
                            R.b0 = filter121ByteAvg(*(pT),*(pC),*(pCt));
                            R.b1 = filter121ByteAvg(*(pCt),*(pCt+srcStride),*(pCt+(2*srcStride))); pCt += 2*srcStride;
                            R.b2 = filter121ByteAvg(*(pCt),*(pCt+srcStride),*(pCt+(2*srcStride))); pCt += 2*srcStride;
                            R.b3 = filter121ByteAvg(*(pCt),*(pCt+srcStride),*(pB));
                            pT++; pC++; pB++;
                        }
                        int dstStride = imgDst.StrideBytes();
                        *(pDst+(0*dstStride)) = filter121ByteAvg(L.b0,C.b0,R.b0);
                        *(pDst+(1*dstStride)) = filter121ByteAvg(L.b1,C.b1,R.b1);
                        *(pDst+(2*dstStride)) = filter121ByteAvg(L.b2,C.b2,R.b2);
                        *(pDst+(3*dstStride)) = filter121ByteAvg(L.b3,C.b3,R.b3);
                        pDst++;
                        w--;
                    }
                }
                ys += 8; yd += 4;
            }
        }
        while (yd < rctDst.Height())
        {
            const Byte* pC = imgSrc.BytePtr(npSrcOrigin.x, ys);
            const Byte* pT = (ys>0)?(pC-srcStride):(pC);
            const Byte* pB = (ys<(imgSrc.Height()-1))?(pC+srcStride):(pC);
            Byte* pDst = imgDst.BytePtr(0, yd);
            int w = imgDst.Width();
            int offs = (npSrcOrigin.x == 0)?(0):(-1);
            Byte Fst = filter121ByteAvg(*(pT+offs),*(pC+offs),*(pB+offs));
            int i = 0;
#if defined(VT_SSE_SUPPORTED)
            if ( g_SupportSSSE3() )
            {
                if (bDeci && (w >= 8))
                {
                    __SelectSSEFuncL( 
                        IsAligned16(pT)&&IsAligned16(pC)&&IsAligned16(pB), 
                        i, Mono_1BandByte8_Deci_SSSE3,
                        pDst, Fst, w, pT, pC, pB );
                }
                else if (w >= 16)
                {
                    __SelectSSEFuncLS( 
                        IsAligned16(pT)&&IsAligned16(pC)&&IsAligned16(pB)&&IsAligned16(pDst), 
                        i, Mono_1BandByte16_SSE3,
                        pDst, Fst, bRExt, w, pT, pC, pB );
                }
            }
#elif defined(VT_NEON_SUPPORTED)
            {
                if (bDeci && (w >= 8))
                {
                    i = Mono_1BandByte8_Deci_Neon( pDst, Fst, w, pT, pC, pB );
                }
                else if (w >= 16)
                {
                    i = Mono_1BandByte16_Neon( pDst, Fst, bRExt, w, pT, pC, pB );
                }
            }
#endif
            w -= i;
            if (w > 0)
            {
                if (i > 0)
                {
                    pDst += i; 
                    if (bDeci) { pT += 2*i; pC += 2*i; pB += 2*i; }
                    else       { pT += i; pC += i; pB += i; }
                    Fst = filter121ByteAvg(*(pT-1),*(pC-1),*(pB-1));
                }
                Byte L,C,R;
                if (bDeci) { C = R = Fst; }
                else { C = Fst; R = filter121ByteAvg(*pT++,*pC++,*pB++); }
                while (w > 0)
                {
                    if (bDeci)
                    {
                        L = R;
                        C = filter121ByteAvg(*pT++,*pC++,*pB++);
                        // right always available for decimation
                        R = filter121ByteAvg(*pT++,*pC++,*pB++);
                    }
                    else
                    {
                        L = C;
                        C = R;
                        // right not available when last pixel and bRExt
                        R = (bRExt&&(w==1))?(C):(filter121ByteAvg(*pT++,*pC++,*pB++));
                    }
                    *pDst++ = filter121ByteAvg(L,C,R);
                    w--;
                }
            }
            ys += bDeci?2:1; yd++;
        }

        return S_OK;
    }
    //-------------------------------------------------------------------------

    // allocate one scanline byte buffer that is the destination width plus extra
    // pixels for left/right extend areas and to enable alignment for vector load/store
    CByteImg slbuf; 
    // byte width, with adding 8 temp pixels (UInt16 or Float) on both sides 
    int slbufwidth = (bIsByte)?(2*(srcBndsXWidth+16)):(4*(srcBndsXWidth+16));
    VT_HR_EXIT(slbuf.Create(slbufwidth,1,1)); 
    // pointer for temp scanline, points to first pixel in temp buffer including
    // padding, and is offset such that the second pixel (which is the first non-padding
    // pixel) is 16-byte aligned
    void* slbufBase = slbuf.Ptr(bIsByte?16:32,0) - ((bIsByte?2:4)*nBnds); 

    // loops use source width and dest height (since dest height incorporates decimation)
    for (int y=0; y<rctDst.Height(); y++)
    {
        // apply filter on vertical axis to form vertically filtered scanline in temp buffer
        if (bIsByte)
        {
            VerticalAxis121Filter<UInt16,Byte>(nBnds,srcBndsXWidth,
                npSrcOrigin.x,npSrcOrigin.y+((bDeci)?(y<<1):(y)),bRExt,
                imgSrc,slbufBase);
        }
        else
        {
            VerticalAxis121Filter<float,float>(nBnds,srcBndsXWidth,
                npSrcOrigin.x,npSrcOrigin.y+((bDeci)?(y<<1):(y)),bRExt,
                imgSrc,slbufBase);
        }

        // 
        // apply filter to horizontal axis and place completed pixels in dest buffer
        if (bIsByte)
        {
            HorizontalAxis121Filter<Byte,UInt16>(nBnds,srcBndsXWidth,bDeci,y,
                imgDst,slbufBase);
        }
        else
        {
            HorizontalAxis121Filter<float,float>(nBnds,srcBndsXWidth,bDeci,y,
                imgDst,slbufBase);
        }
    }

    VT_HR_END()
}


//+-----------------------------------------------------------------------------
//
// public (non-transform) function
//
HRESULT
vt::VtSeparableFilter121(
    CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, CPoint ptSrcOrigin,
    bool bLowBytePrecision, int numPasses, CImg* pimgTmp)
{
	VT_HR_BEGIN()

    VT_HR_EXIT( (!imgSrc.IsValid()) ? E_INVALIDSRC : S_OK );
    VT_HR_EXIT( (imgSrc.IsSharingMemory(imgDst)) ? E_INVALIDDST : S_OK );
    // create the destination image if it needs to be
    VT_HR_EXIT( CreateImageForTransform(imgDst, rctDst.Width(), rctDst.Height(),
                                        imgSrc.GetType()) );

    CImg imgTmp;
    if (numPasses > 1)
    {
        if (pimgTmp == NULL)
        {
            imgTmp.Create(rctDst.Width(),rctDst.Height(),imgDst.GetType());
            pimgTmp = &imgTmp;
        }
        else
        {
            VT_HR_EXIT((pimgTmp->Width() != imgDst.Width())?E_INVALIDARG:S_OK);
            VT_HR_EXIT((pimgTmp->Height() != imgDst.Height())?E_INVALIDARG:S_OK);
            VT_HR_EXIT((pimgTmp->GetType() != imgDst.GetType())?E_INVALIDARG:S_OK);
        }
    }

    for (int i = 0; i<numPasses; i++)
    {
        // select source and dest for this pass, using user imgDst as an
        // intermediate temp buffer for numPasses>4 (so for 4 passes, use
        // s->d->t->d, for 5 use s->t->d->t->d, etc.)
        int ic = (numPasses-1)-i;
        const CImg& imgSrcP = (i==0)?(imgSrc):((ic&0x1)?(imgDst):(*pimgTmp));
        CImg& imgDstP = (ic==0)?(imgDst):((ic&0x1)?(*pimgTmp):(imgDst));

        // call to non-specialized (instead of returning failure)
        if ( (imgSrcP.Bands() > 4) ||
             (imgSrcP.Bands() != imgDstP.Bands()) ||
             (EL_FORMAT(imgSrcP.GetType()) != EL_FORMAT(imgDstP.GetType())) ||
             ( (EL_FORMAT(imgSrcP.GetType()) != EL_FORMAT_BYTE) &&
               (EL_FORMAT(imgSrcP.GetType()) != EL_FORMAT_FLOAT) ) )
        {
            C121Kernel k121;
            return VtSeparableFilter(imgDstP, rctDst, imgSrcP, ptSrcOrigin, k121.AsKernel());
        }

        // here when: src and dest bands are <4 and match; src and dest formats
        // match; byte or float only

        // loop over the destination blocks
        const UInt32 c_blocksize = 768/(imgDst.Bands());
	    for( CBlockIterator bi(rctDst, c_blocksize); !bi.Done(); bi.Advance() )
	    {
            // get blk rct in dst image coords
		    vt::CRect rctDstBlk = bi.GetRect();

		    CImg imgDstBlk;
		    imgDstP.Share(imgDstBlk, &rctDstBlk);

            // offset to layer coords
            rctDstBlk += rctDst.TopLeft();
    
            VT_HR_EXIT( VtSeparableFilter121Internal(imgDstBlk, rctDstBlk, 
                imgSrcP, ptSrcOrigin, false, bLowBytePrecision) );
        }
    }

    VT_HR_END()
}

HRESULT
vt::VtSeparableFilter121Decimate2to1(
    CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, CPoint ptSrcOrigin,
    bool bLowBytePrecision)
{
	VT_HR_BEGIN()

    VT_HR_EXIT( (!imgSrc.IsValid()) ? E_INVALIDSRC : S_OK );
    VT_HR_EXIT( (imgSrc.IsSharingMemory(imgDst)) ? E_INVALIDDST : S_OK );
    // create the destination image if it needs to be
    VT_HR_EXIT( CreateImageForTransform(imgDst, rctDst.Width(), rctDst.Height(),
                                        imgSrc.GetType()) );

    // call to non-specialized (instead of returning failure)
    if ( (imgSrc.Bands() > 4) ||
         (imgDst.Bands() > 4) ||
         (imgSrc.Bands() != imgDst.Bands()) ||
         (EL_FORMAT(imgSrc.GetType()) != EL_FORMAT(imgDst.GetType())) ||
         ( (EL_FORMAT(imgSrc.GetType()) != EL_FORMAT_BYTE) &&
           (EL_FORMAT(imgSrc.GetType()) != EL_FORMAT_FLOAT) ) )
    {
        C121Kernel k121;
   	    C1dKernelSet ks;
	    ks.Create(1,2);
        ks.Set(0, -1, k121.AsKernel());

        return VtSeparableFilter(imgDst, rctDst, imgSrc, ptSrcOrigin, ks);
    }

    // here when: src and dest bands are <=4 and match; src and dest formats 
    // match; byte or float only

    // loop over the destination blocks
    int blockWidth = imgDst.Width();
    while (blockWidth > 1024) { blockWidth >>= 1; }
    const UInt32 c_blocksize = blockWidth/imgDst.Bands();
	for( CBlockIterator bi(rctDst, c_blocksize); !bi.Done(); bi.Advance() )
	{
        // get blk rct in dst image coords
		vt::CRect rctDstBlk = bi.GetRect();

		CImg imgDstBlk;
		imgDst.Share(imgDstBlk, &rctDstBlk);

        // offset to layer coords
        rctDstBlk += rctDst.TopLeft();


        VT_HR_EXIT( VtSeparableFilter121Internal(imgDstBlk, rctDstBlk, 
            imgSrc, ptSrcOrigin, true, bLowBytePrecision) );
    }

    VT_HR_END()
}

#ifndef VT_NO_XFORMS

HRESULT CSeparableFilter121Transform::Transform(OUT CImg* pimgDstRegion, 
                              IN  const CRect& rctLayerDst,
                              IN  CImg *const *ppimgSrcRegions,
                              IN  const TRANSFORM_SOURCE_DESC* pSrcDesc,
                              IN  UINT  /*uSrcCnt*/)
{ 
    VT_HR_BEGIN()
    if (!m_NoRW)
    {
        VT_HR_EXIT( VtSeparableFilter121Internal(*pimgDstRegion, rctLayerDst,
            *ppimgSrcRegions[0], pSrcDesc[0].rctSrc.TopLeft(), 
            m_bDeci?true:false, m_bLowPrecision) );
    }
    else
    {
        // transform framework has no source or dest reader/writer in this case,
        // so need to share out the dest rect only from the destination for the
        // processing routine; can use full source image when passing a source 
        // origin of (0,0), which ensures that the extend mode is properly 
        // applied (only) at the source boundaries
        CImg dstImgR; VT_HR_EXIT( m_dstImg.Share(dstImgR, rctLayerDst) );

        VT_HR_EXIT( VtSeparableFilter121Internal(dstImgR, rctLayerDst,
            m_srcImg, CPoint::CPoint(0,0), m_bDeci?true:false, m_bLowPrecision) );
    }
    VT_HR_END()
}

#endif

//+-----------------------------------------------------------------------------
