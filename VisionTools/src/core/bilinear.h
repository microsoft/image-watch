//+----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      routines for bilinear sampling for warp, resize, etc.
//
//-----------------------------------------------------------------------------
#pragma once

#include "vtcommon.h"

namespace vt 
{

//+----------------------------------------------------------------------------
//
// single pixel processing for vector remainder; does 1,2,4 band bytes with
// matching source and destination band count only, plus 4 band source and
// 3 band destination
//
//-----------------------------------------------------------------------------
inline void
BilinearProcessSinglePixel(int dstBands, Byte* pDst,
    int32_t xaddr, int32_t yaddr, uint16_t ufrac, uint16_t vfrac,
    int srcStridePixels, const Byte* pSrc)    
{
    // compute 8bpp weights for each sample
    uint16_t frac[4];
    {
        frac[3] = ((ufrac*vfrac)+0x80)>>8;   // wd = u*v
        frac[2] = vfrac - frac[3];           // wc = v-(u*v)
        frac[1] = ufrac - frac[3];           // wb = u-(u*v)
        frac[0] = 0x100 - ufrac - (frac[2]); // wa = 1-u-v+(u*v) == 1-u-(v-(u*v)); also wa = 1-(wb+wc+wd)
    }

    // compute source offset
    int offs = (yaddr*srcStridePixels)+xaddr;

    // do band-count specific loading and filtering of source pixels and packing for dest
    if (dstBands == 1)
    {
        // load sources
        uint8_t srcpix[4];
        srcpix[0] = *(pSrc+offs); // sample a
        srcpix[1] = *(pSrc+offs+1); // sample b
        srcpix[2] = *(pSrc+offs+srcStridePixels); // sample c
        srcpix[3] = *(pSrc+offs+srcStridePixels+1); // sample d
        // filter
        uint32_t cacc = (frac[0]*srcpix[0])+
                        (frac[1]*srcpix[1])+
                        (frac[2]*srcpix[2])+
                        (frac[3]*srcpix[3]);
        // round,align and store - note that weights are guaranteed to sum to 0x100, so
        // no need for bounds checking
        cacc = (cacc+0x80)>>8;
        *pDst = (Byte)cacc;
    }
    else if (dstBands == 2)
    {
        // load sources
        uint16_t pix[4];
        pix[0] = *( ((uint16_t*)pSrc)+offs );
        pix[1] = *( ((uint16_t*)pSrc)+offs+1 );
        pix[2] = *( ((uint16_t*)pSrc)+offs+srcStridePixels );
        pix[3] = *( ((uint16_t*)pSrc)+offs+srcStridePixels+1 );
        // unpack channels
        uint32_t pixc[4][2];
        for (int i=0; i<4; i++)
        {
            pixc[i][0] = (pix[i]>>(8*0))&0xff;
            pixc[i][1] = (pix[i]>>(8*1))&0xff;
        }
        // filter, round/align
        uint32_t cacc[2] = { 0, 0 };
        for (int i=0; i<4; i++)
        {
            cacc[0] += pixc[i][0]*frac[i];
            cacc[1] += pixc[i][1]*frac[i];
        }
        cacc[0] = (cacc[0]+0x80)>>8;
        cacc[1] = (cacc[1]+0x80)>>8;
        *(uint16_t*)pDst = (uint16_t)((cacc[1]<<8)|cacc[0]);
    }
    else // srcBands is 4, dstBands is 3 or 4
    {
        // load sources
        uint32_t pixc[4][4];
        {
            uint32_t pix[4];
            pix[0] = *( ((uint32_t*)pSrc)+offs );
            pix[1] = *( ((uint32_t*)pSrc)+offs+1 );
            pix[2] = *( ((uint32_t*)pSrc)+offs+srcStridePixels );
            pix[3] = *( ((uint32_t*)pSrc)+offs+srcStridePixels+1 );
            for (int i=0; i<4; i++)
            {
                pixc[i][0] = (pix[i]>>(8*0))&0xff;
                pixc[i][1] = (pix[i]>>(8*1))&0xff;
                pixc[i][2] = (pix[i]>>(8*2))&0xff;
                pixc[i][3] = (pix[i]>>(8*3))&0xff;
            }
        }
        // filter, round/align
        uint32_t cacc[4] = { 0, 0, 0, 0 };
        for (int i=0; i<4; i++)
        {
            cacc[0] += pixc[i][0]*frac[i];
            cacc[1] += pixc[i][1]*frac[i];
            cacc[2] += pixc[i][2]*frac[i];
            cacc[3] += pixc[i][3]*frac[i];
        }
        cacc[0] = (cacc[0]+0x80)>>8;
        cacc[1] = (cacc[1]+0x80)>>8;
        cacc[2] = (cacc[2]+0x80)>>8;
        cacc[3] = (cacc[3]+0x80)>>8;
        // dest bands is either 3 or 4
        if (dstBands == 3)
        {
            *(pDst+0) = (Byte)cacc[0];
            *(pDst+1) = (Byte)cacc[1];
            *(pDst+2) = (Byte)cacc[2];
        }
        else
        {
            *(uint32_t*)pDst = (cacc[3]<<24)|(cacc[2]<<16)|(cacc[1]<<8)|cacc[0];
        }
    }
}

#if defined(VT_SIMD_SUPPORTED)
//+----------------------------------------------------------------------------
//
// per-architecture specialized support for 1,2,4 bands with matching src,dst
//
//-----------------------------------------------------------------------------

#if defined(VT_SSE_SUPPORTED)
#include "bilinear_sse.h"
#elif defined(VT_NEON_SUPPORTED)
#include "bilinear_neon.h"
#endif

//+----------------------------------------------------------------------------
//
// partially-specialized support for 4 band source and 3 band dest
//
//-----------------------------------------------------------------------------
template <typename T>
VT_FORCEINLINE void 
Bilinear4to3BandBytex4Process(
    const uint32x4_t& xf0123, const uint32x4_t& yf0123,
    _Out_writes_(12) Byte* pDst,
    const T* pOffs, const uint32_t* pSrcPixBase, uint32_t srcPixStride)
{
    // run 4 band dest routine to temp buffer
    VT_DECLSPEC_ALIGN(16) uint8_t tempBuf[4*4];
    Bilinear4BandBytex4Process<T>(xf0123,yf0123,tempBuf,pOffs,pSrcPixBase,srcPixStride);

#if defined(VT_SSE_SUPPORTED)
    __m128i x0 = _mm_load_si128((const __m128i*)tempBuf);
    // pack 3 band data to bottom of register
    __m128i x1 = _mm_setr_epi8(0,1,2,4,5,6,8,9,10,12,13,14,-1,-1,-1,-1);
    x0 = _mm_shuffle_epi8(x0,x1);

    // store (compiler generates 32b stores)
    for (int i=0; i<12; i++)
	{
		*pDst++ =
#if defined(VT_IS_MSVC_COMPILER)
			x0.m128i_u8[i];
#else
    		(unsigned char) SSE2_mm_extract_epi8(x0, i);
#endif
	}
#else
    *pDst++ = tempBuf[ 0];
    *pDst++ = tempBuf[ 1];
    *pDst++ = tempBuf[ 2];
    *pDst++ = tempBuf[ 4];
    *pDst++ = tempBuf[ 5];
    *pDst++ = tempBuf[ 6];
    *pDst++ = tempBuf[ 8];
    *pDst++ = tempBuf[ 9];
    *pDst++ = tempBuf[10];
    *pDst++ = tempBuf[12];
    *pDst++ = tempBuf[13];
    *pDst++ = tempBuf[14];
#endif
}

#endif // VT_SIMD_SUPPORTED
}