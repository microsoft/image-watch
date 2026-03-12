//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      specialized implementation of image warp
//
//      SSE specific code
//
//------------------------------------------------------------------------------
#pragma once

// utility routines for applying bilinear filtering
#include "bilinear.h"

//------------------------------------------------------------------------------
inline void WarpSpecialSpan_by4_BilinearByte4_SSE(
    uint32_t* pDstPix, int w4, const uint32_t* pSrcPixBase, uint32_t srcPixStride,
    int32_t u, int32_t v, int32_t du, int32_t dv)
{
    // initialize 4 vector u,v interpolator
    __m128i u4,v4;
	u4 = _mm_setr_epi32(u, u+du, u+2*du, u+3*du);
	v4 = _mm_setr_epi32(v, v+dv, v+2*dv, v+3*dv);
    int32_t duStep4 = du<<2;
    int32_t dvStep4 = dv<<2;

    // offsets to top (a,b)
    VT_DECLSPEC_ALIGN(16) uint32_t offs[4];

    __m128i du4 = _mm_set1_epi32(duStep4);
    __m128i dv4 = _mm_set1_epi32(dvStep4);
    __m128i srcStride32 = _mm_set1_epi32(srcPixStride);

    while (w4)
    {
        // compute pixel offsets for top and bottom adjacent pixels
        {
            __m128i tmp0,tmp1;
            tmp0 = _mm_srli_epi32(v4,16); // isolate address part of v in low 16
            tmp0 = _mm_madd_epi16(tmp0,srcStride32);
            tmp1 = _mm_srli_epi32(u4,16); // isolate address part of u in low 16
            tmp0 = _mm_add_epi32(tmp0,tmp1); // pixel offset to top adjacent pixels
            _mm_store_si128((__m128i*)(&offs[0]),tmp0);
        }

        __m128i xf0123 = _mm_srli_epi32(_mm_slli_epi32(u4,16),24); // u fraction
        __m128i yf0123 = _mm_srli_epi32(_mm_slli_epi32(v4,16),24); // v fraction

        Bilinear4BandBytex4Process(xf0123,yf0123,(Byte*)pDstPix,
            offs,pSrcPixBase,srcPixStride);
        pDstPix += 4;
        u4 = _mm_add_epi32(u4,du4);
        v4 = _mm_add_epi32(v4,dv4);
        w4--;
    }
}

//------------------------------------------------------------------------------
inline void WarpSpecialSpan_by8_BilinearByte1_SSE(
    uint8_t* pDstPix, int w8, const uint8_t* pSrcPixBase, uint32_t srcPixStride,
    int32_t u, int32_t v, int32_t du, int32_t dv)
{
    // initialize 8 (4x2) vector u,v interpolators
    __m128i u8l,u8h,v8l,v8h;
	int32_t uh = u+(du<<2);
	int32_t vh = v+(dv<<2);
	u8l = _mm_setr_epi32(u, u+du, u+2*du, u+3*du); u8h = _mm_setr_epi32(uh, uh+du, uh+2*du, uh+3*du);
	v8l = _mm_setr_epi32(v, v+dv, v+2*dv, v+3*dv); v8h = _mm_setr_epi32(vh, vh+dv, vh+2*dv, vh+3*dv);
    int32_t duStep8 = du<<3;
    int32_t dvStep8 = dv<<3;

    __m128i du8 = _mm_set1_epi32(duStep8);
    __m128i dv8 = _mm_set1_epi32(dvStep8);
    __m128i srcPixStride32 = _mm_set1_epi32(srcPixStride);

    // offsets to top (a,b)
    VT_DECLSPEC_ALIGN(16) uint32_t offs[8];

    while (w8)
    {
        // compute pixel offsets for top and bottom adjacent pixels
        {
            __m128i tmp0,tmp1;
            tmp0 = _mm_srli_epi32(v8l,16); // isolate address part of v in low 16
            tmp0 = _mm_madd_epi16(tmp0,srcPixStride32);
            tmp1 = _mm_srli_epi32(u8l,16); // isolate address part of u in low 16
            tmp0 = _mm_add_epi32(tmp0,tmp1); // pixel offset to top adjacent pixels
            _mm_store_si128((__m128i*)(&offs[0]),tmp0);
        }
        {
            __m128i tmp0,tmp1;
            tmp0 = _mm_srli_epi32(v8h,16); // isolate address part of v in low 16
            tmp0 = _mm_madd_epi16(tmp0,srcPixStride32);
            tmp1 = _mm_srli_epi32(u8h,16); // isolate address part of u in low 16
            tmp0 = _mm_add_epi32(tmp0,tmp1); // pixel offset to top adjacent pixels
            _mm_store_si128((__m128i*)(&offs[4]),tmp0);
        }

        __m128i x0,x1,x2;
        x0 = _mm_srli_epi32(_mm_slli_epi32(u8l,16),24);
        x1 = _mm_srli_epi32(_mm_slli_epi32(u8h,16),24);
        x0 = _mm_packs_epi32(x0,x1); // xfrac 0..7
        x1 = _mm_srli_epi32(_mm_slli_epi32(v8l,16),24);
        x2 = _mm_srli_epi32(_mm_slli_epi32(v8h,16),24);
        x1 = _mm_packs_epi32(x1,x2); // yfrac 0..7
        Bilinear1BandBytex8Process(x0,x1,pDstPix,offs,pSrcPixBase,srcPixStride);
        pDstPix+=8;

        u8l = _mm_add_epi32(u8l,du8); u8h = _mm_add_epi32(u8h,du8);
        v8l = _mm_add_epi32(v8l,dv8); v8h = _mm_add_epi32(v8h,dv8);
        w8--;
    }
}

//------------------------------------------------------------------------------
inline void WarpSpecialSpan_by4_BilinearByte2_SSE(
    uint16_t* pDstPix, int w4, const uint16_t* pSrcPixBase, uint32_t srcPixStride,
    int32_t u, int32_t v, int32_t du, int32_t dv)
{
    // initialize 4 vector u,v interpolator
    __m128i u4,v4;
	u4 = _mm_setr_epi32(u, u+du, u+2*du, u+3*du);
	v4 = _mm_setr_epi32(v, v+dv, v+2*dv, v+3*dv);
    int32_t duStep4 = du<<2;
    int32_t dvStep4 = dv<<2;

    __m128i du4 = _mm_set1_epi32(duStep4);
    __m128i dv4 = _mm_set1_epi32(dvStep4);
    __m128i srcStride32 = _mm_set1_epi32(srcPixStride);

    // offsets to top (a,b)
    VT_DECLSPEC_ALIGN(16) uint32_t offs[4];

    while (w4)
    {
        // compute pixel offsets for top and bottom adjacent pixels
        {
            __m128i tmp0,tmp1;
            tmp0 = _mm_srli_epi32(v4,16); // isolate address part of v in low 16
            tmp0 = _mm_madd_epi16(tmp0,srcStride32);
            tmp1 = _mm_srli_epi32(u4,16); // isolate address part of u in low 16
            tmp0 = _mm_add_epi32(tmp0,tmp1); // pixel offset to top adjacent pixels
            _mm_store_si128((__m128i*)(&offs[0]),tmp0);
        }

        __m128i xf0123 = _mm_srli_epi32(_mm_slli_epi32(u4,16),24); // u fraction
        __m128i yf0123 = _mm_srli_epi32(_mm_slli_epi32(v4,16),24); // v fraction
        Bilinear2BandBytex4Process(xf0123,yf0123,(Byte*)pDstPix,offs,pSrcPixBase,srcPixStride);

        pDstPix+=4;
        u4 = _mm_add_epi32(u4,du4);
        v4 = _mm_add_epi32(v4,dv4);
        w4--;
    }
}

//------------------------------------------------------------------------------
// end
