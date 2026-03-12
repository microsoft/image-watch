//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      SIMD SSE and Neon (using vt_simd.h mappings) routines for image warping
//
//------------------------------------------------------------------------
#pragma once

//+----------------------------------------------------------------------------
// bilinear warp with byte source and destination images
//-----------------------------------------------------------------------------

// inline routines to apply bilinear sampling to various img types
#include "bilinear.h"

#if defined(VT_SIMD_SUPPORTED)

// processes four AddressGenerator CVec2f addresses and computes
// the integer offsets to the top left and bottom left source pixels
// and extracts the fractional location of the sample position; the
// floating point positions are compared to the rectangle bounds and
// the offset is set to integer -1 if any are out of bounds; the
// floating point addition of the constant value fixedDot8 results
// in the mantissa being a fixed point 15.8 value that has been 
// rounded via the standard IEEE754 post-operation round to nearest even
// (for inputs >= 0.f); y*stride is computed in floating point so is
// limited to 23 bits
#if defined(VT_NEON_SUPPORTED)
#define PROCESS_ADDRESS_4_LOADADDR(_iptr_) \
    { \
        float32x4x2_t addr = vld2q_f32(_iptr_); \
        x0 = addr.val[0]; /* x0..x3 */ \
        x1 = addr.val[1]; /* y0..y3 */ \
    }
#elif defined(VT_SSE_SUPPORTED)
#define PROCESS_ADDRESS_4_LOADADDR(_iptr_) \
    { \
        x2 = pfnLoad((_iptr_)+0);   /* x0,y0,x1,y1 */ \
        x1 = pfnLoad((_iptr_)+4);   /* x2,y2,x3,y3 */ \
        x0 = _mm_shuffle_ps(x2,x1,_MM_SHUFFLE(2,0,2,0)); /* x0,x1,x2,x3 */ \
        x1 = _mm_shuffle_ps(x2,x1,_MM_SHUFFLE(3,1,3,1)); /* y0,y1,y2,y3 */ \
    }
#endif
#define PROCESS_ADDRESS_4(_iptr_,_offsp_,_fracx_,_fracy_) \
{ \
    /* define these here and let the compiler lift the constants out */ \
    float32x4_t lB4 = _mm_set1_ps(lB); \
    float32x4_t rB4 = _mm_set1_ps(rB); \
    float32x4_t tB4 = _mm_set1_ps(tB); \
    float32x4_t bB4 = _mm_set1_ps(bB); \
    float32x4_t fixedDot8 = _mm_set1_ps((float)(1<<15)); \
    float32x4_t stride4f = _mm_set1_ps((float)pixstride); \
    uint32x4_t frac8 = _mm_set1_epi32(0xff); \
    \
    float32x4_t x0,x1,x2,x3; \
    PROCESS_ADDRESS_4_LOADADDR(_iptr_) \
    x0 = _mm_sub_ps(x0,lB4); /* fx0..3 */ \
    x1 = _mm_sub_ps(x1,tB4); /* fy0..3 */ \
    x2 = _mm_sub_ps(rB4,x0); /* right side */ \
    x3 = _mm_sub_ps(bB4,x1); /* bottom side */ \
    x2 = _mm_or_ps(x2,x3); /* OR the 4 subtraction results to determine if any are negative */ \
    x2 = _mm_or_ps(x2,x0); \
    x2 = _mm_or_ps(x2,x1); /* 1 in MSB if oob, else 0 */ \
    x2 = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(x2),31)); /* extend MSB sign bit to all bits */ \
    /* compute and store 4 pixel source offsets */ \
    x0 = _mm_add_ps(x0,fixedDot8); /* low 23 bits is 15.8 fixed point */ \
    x1 = _mm_add_ps(x1,fixedDot8); \
    x3 = _mm_andnot_ps(_mm_castsi128_ps(frac8),x1); /* clear bottom 8 bits of shifted float y value */ \
    x3 = _mm_sub_ps(x3,fixedDot8); /* integral y as float */ \
    x3 = _mm_mul_ps(x3,stride4f); /* integral y*stride as float */ \
    int32x4_t ix0 = _mm_srli_epi32(_mm_slli_epi32(_mm_castps_si128(x0),9),9+8); /* isolate x pixel offset */ \
    int32x4_t ix1 = _mm_cvtps_epi32(x3); /* integer y*stride */ \
    ix1 = _mm_add_epi32(ix1,ix0); /* (maybe) offset to top left source pixel */ \
    ix1 = _mm_castps_si128(_mm_or_ps(_mm_castsi128_ps(ix1),x2)); /* -1 if oob */ \
    _mm_store_si128((int32x4_t*)(_offsp_),ix1); \
    /* isolate fraction bits in low 8 */ \
    _fracx_ = _mm_and_si128(_mm_castps_si128(x0),frac8); \
    _fracy_ = _mm_and_si128(_mm_castps_si128(x1),frac8); \
}

// 1 band warp
template<float32x4_t (*pfnLoad)(const float*)>
int BilinearWarpSpan1BandByte(OUT Byte* pDst,
                                   const IN CByteImg& imgSrcBlk,
                                   const CVec2f* pAddr,
                                   float lB, float rB, float tB, float bB,
                                   int iCnt)
{
    int32_t pixstride = imgSrcBlk.StrideBytes();
    const Byte* pSrcPixBase = imgSrcBlk.BytePtr();

    // offsets to top (a,b)
	VT_DECLSPEC_ALIGN(16) int32_t offs[8];

    int x = 0;
    for ( ;x < iCnt-7; x+=8, pAddr+=8, pDst += 8 )
    {
        // process addresses into source offset and bilinear fractional
        // weights
        uint32x4_t xf0,xf1,xf2,xf3;
        PROCESS_ADDRESS_4(pAddr->Ptr()+0,&offs[0],xf0,xf1);
        PROCESS_ADDRESS_4(pAddr->Ptr()+8,&offs[4],xf2,xf3);
        uint16x8_t xf = _mm_packs_epi32(xf0,xf2); // xfrac 0..7
        uint16x8_t yf = _mm_packs_epi32(xf1,xf3); // yfrac 0..7
        Bilinear1BandBytex8Process(xf,yf,pDst,offs,pSrcPixBase,pixstride);
    }
    return x;
}

// 2 band warp
template<float32x4_t (*pfnLoad)(const float*)>
int BilinearWarpSpan2BandByte(OUT Byte* pDst,
                                   const IN CByteImg& imgSrcBlk,
                                   const CVec2f* pAddr,
                                   float lB, float rB, float tB, float bB,
                                   int iCnt)
{
    int32_t pixstride = imgSrcBlk.StrideBytes()>>1;
    const uint16_t* pSrcPixBase = (uint16_t*)imgSrcBlk.BytePtr();

    // offsets to top (a,b)
	VT_DECLSPEC_ALIGN(16) int32_t offs[4];

    int x = 0;
    for ( ;x < iCnt-3; x+=4, pAddr+=4, pDst += 8 )
    {
        // process addresses into source offset and bilinear fractional
        // weights
        uint32x4_t xf0123,yf0123;
        PROCESS_ADDRESS_4(pAddr->Ptr(),&offs[0],xf0123,yf0123);
        Bilinear2BandBytex4Process(xf0123,yf0123,
            pDst, offs, pSrcPixBase, pixstride);
    }
    return x;
}

// 4 band warp
template<float32x4_t (*pfnLoad)(const float*)>
int BilinearWarpSpan4BandByte(OUT Byte* pDst,
                                   const IN CByteImg& imgSrcBlk,
                                   const CVec2f* pAddr,
                                   float lB, float rB, float tB, float bB,
                                   int iCnt)
{
    int32_t pixstride = imgSrcBlk.StrideBytes()>>2;
    const uint32_t* pSrcPixBase = (uint32_t*)imgSrcBlk.BytePtr();

    // offsets to top (a,b) and bottom (c,d) source pixels
	VT_DECLSPEC_ALIGN(16) int32_t offs[8];

    int x = 0;
    for ( ;x < iCnt-3; x+=4, pAddr+=4, pDst += 16 )
    {
        // process addresses into source offset and bilinear fractional
        // weights
        uint32x4_t xf0123,yf0123;
        PROCESS_ADDRESS_4(pAddr->Ptr(),&offs[0],xf0123,yf0123);
        Bilinear4BandBytex4Process(xf0123,yf0123,pDst,offs,pSrcPixBase,pixstride);
    }
    return x;
}

// 3 band warp - with 4 band source and 3 band dest
template<float32x4_t (*pfnLoad)(const float*)>
int BilinearWarpSpanS4D3BandByte(OUT Byte* pDst,
                                      const IN CByteImg& imgSrcBlk,
                                      const CVec2f* pAddr,
                                      float lB, float rB, float tB, float bB,
                                      int iCnt)
{
    int32_t pixstride = imgSrcBlk.StrideBytes()>>2;
    const uint32_t* pSrcPixBase = (uint32_t*)imgSrcBlk.BytePtr();

    // offsets to top (a,b)
	VT_DECLSPEC_ALIGN(16) int32_t offs[4];

    int x = 0;
    for ( ;x < iCnt-3; x+=4, pAddr+=4,pDst +=12 )
    {
        // process addresses into source offset and bilinear fractional
        // weights
        uint32x4_t xf0123,yf0123;
        PROCESS_ADDRESS_4(pAddr->Ptr(),&offs[0],xf0123,yf0123);
        Bilinear4to3BandBytex4Process(xf0123,yf0123,
            pDst, offs, pSrcPixBase,pixstride);
    }
    return x;
}
#undef PROCESS_ADDRESS_4
#undef PROCESS_ADDRESS_4_LOADADDR

#endif

//-----------------------------------------------------------------------------
// end