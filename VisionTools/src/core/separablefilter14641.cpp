//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Separable filter specialization for 14641 kernel
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
#include "vt_layer.h"

using namespace vt;

//-----------------------------------------------------------------------------
//
// 2:1 Upsampling
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// routines to apply 14641 upsample filter to byte data; uses 16 bit accumulation
// so full precision of intermediate results are retained
//-----------------------------------------------------------------------------

// produce 8.3 16bpp result for vertical (first) pass
inline uint16_t FilterX4X4XP1(uint8_t a, uint8_t b)
{
    uint16_t a16 = (uint16_t)a;
    uint16_t b16 = (uint16_t)b;
    return (a16+b16)<<2;
}
inline uint16_t Filter1X6X1P1(uint8_t l, uint8_t c, uint8_t r)
{
    uint16_t l16 = (uint16_t)l;
    uint16_t c16 = (uint16_t)c;
    uint16_t r16 = (uint16_t)r;
    return ( (c16<<2) + (c16<<1) + l16 + r16 );
}

// produce rounded 8.0 result from 8.3 inputs for horizontal (second) pass
inline uint8_t FilterX4X4XP2(uint16_t a, uint16_t b)
{
    uint16_t acc = a+b;
    return (Byte)((acc+(1<<3))>>4);
}
inline uint8_t Filter1X6X1P2(uint16_t l, uint16_t c, uint16_t r)
{
    uint16_t acc = (c<<2)+(c<<1)+(l)+(r);
    return (Byte)((acc+(1<<5))>>6);
}

//-----------------------------------------------------------------------------
// routines to apply 14641 upsample filter to float data;
//-----------------------------------------------------------------------------

// produce scaled by 8.f results for vertical (first) pass
inline float FilterX4X4XP1(float a, float b)
{
    return (a+b)*4.f;
}
inline float Filter1X6X1P1(float l, float c, float r)
{
    return (l+(6.f*c)+r);
}

// produce result from 8.f scaled inputs for horizontal (second) pass
inline float FilterX4X4XP2(float a, float b)
{
    return (a+b)*.0625f;
}
inline float Filter1X6X1P2(float l, float c, float r)
{
    return (l+(6.f*c)+r)*(.015625f);
}

//-----------------------------------------------------------------------------
// apply 14641 upsample to nb channels
//-----------------------------------------------------------------------------
template <typename Tdst, typename Tsrc>
inline void FilterNBbyX4X4XP1(Tdst* dst, const Tsrc*srca, const Tsrc* srcb, int nb = 4)
{
    for (int i=0; i<nb; i++)
    {
        *(dst+i) = FilterX4X4XP1(*(srca+i), *(srcb+i));
    }
}

template <typename Tdst, typename Tsrc>
inline void FilterNBby1X6X1P1(Tdst* dst,const  Tsrc*srca, const Tsrc* srcb, const Tsrc* srcc, int nb = 4)
{
    for (int i=0; i<nb; i++)
    {
        *(dst+i) = Filter1X6X1P1(*(srca+i), *(srcb+i), *(srcc+i));
    }
}

template <typename Tdst, typename Tsrc>
inline void FilterNBbyX4X4XP2(Tdst* dst, const Tsrc*srca, const Tsrc* srcb, int nb = 4)
{
    for (int i=0; i<nb; i++)
    {
        *(dst+i) = FilterX4X4XP2(*(srca+i), *(srcb+i));
    }
}

template <typename Tdst, typename Tsrc>
inline void FilterNBby1X6X1P2(Tdst* dst,const  Tsrc*srca, const Tsrc* srcb, const Tsrc* srcc, int nb = 4)
{
    for (int i=0; i<nb; i++)
    {
        *(dst+i) = Filter1X6X1P2(*(srca+i), *(srcb+i), *(srcc+i));
    }
}

//-----------------------------------------------------------------------------
// SIMD routines for SSE and Neon
//-----------------------------------------------------------------------------
#if defined(VT_SIMD_SUPPORTED)

// vector routines do multiples of 4 destination RGBA pixels at a time
static const int Upsample2to1by14641SIMDBatch = 4;

#if defined(VT_SSE_SUPPORTED)
#define VT_UPSAMPLE14641_SIMD_SUPPORTED (g_SupportSSSE3())
#else
#define VT_UPSAMPLE14641_SIMD_SUPPORTED true
#endif

// 'between' source pixels vertically; float I/O
template<void (*pfnStore)(uint8x16_t*, const uint8x16_t&)>
int Upsample2to1by14641ARowSIMD(float* pd, 
                               const float* pst, const float* psb,
                               int wd, const float* lvfpix, const float* rvfpix)
{
    int ret = wd&(~(Upsample2to1by14641SIMDBatch-1)); // number of dest pixels completed
    const float* pstlv = pst + (ret/2)*4;

    float32x4_t x0,x1;
    float32x4_t L,CL,CR,R;
    float32x4_t const4 = _mm_set1_ps(4.f);
    float32x4_t const6 = _mm_set1_ps(6.f);
    float32x4_t constNormX4X4X = _mm_set1_ps(.0625f);
    float32x4_t constNorm1X6X1 = _mm_set1_ps(.015625f);

    // TODO: aligned loads on sources

    // load L(eft) pix from top of left pix buffer
    L = _mm_load_ps(lvfpix+4);

    // load and vertically filter one RBBA pixels for C(enter)L(eft)
    x0 = _mm_loadu_ps(pst); pst += 4;
    x1 = _mm_loadu_ps(psb); psb += 4;
    x0 = _mm_add_ps(x0,x1); 
    CL = _mm_mul_ps(x0,const4); // vertically filtered and scaled by 8.f

    while ( wd >= Upsample2to1by14641SIMDBatch )
    {
        // load and vertically filter one RBBA pixels for C(enter)R(ight)
        x0 = _mm_loadu_ps(pst); pst += 4;
        x1 = _mm_loadu_ps(psb); psb += 4;
        x0 = _mm_add_ps(x0,x1); 
        CR = _mm_mul_ps(x0,const4); // vertically filtered and scaled by 8.f

        // load and vertically filter one pixel for R(ight)
        if (pst == pstlv)
        {
            R = _mm_load_ps(rvfpix);
        }
        else
        {
            x0 = _mm_loadu_ps(pst); pst += 4;
            x1 = _mm_loadu_ps(psb); psb += 4;
            x0 = _mm_add_ps(x0,x1);
            R = _mm_mul_ps(x0,const4); // vertically filtered and scaled by 8.f
        }

        // L,CL,CR,R
        // R0 = 1X6X1(L,CL,CR)
        // R1 = X4X4X(CL,CR)
        // R2 = 1X6X1(CL,CR,R)
        // R3 = X4X4(CR,R)

        // first result pixel is 1X6X1 filtered
        x0 = _mm_add_ps(L,CR);
        x1 = _mm_mul_ps(CL,const6);
        x0 = _mm_add_ps(x0,x1); // horizontally filtered by 1x6x1
        x0 = _mm_mul_ps(x0,constNorm1X6X1); // normalized
        pfnStore((uint8x16_t*)pd,_mm_castps_si128(x0)); pd += 4;

        // second result pixel is X4X4X filtered
        x0 = _mm_add_ps(CL,CR);
        x0 = _mm_mul_ps(x0,constNormX4X4X);
        pfnStore((uint8x16_t*)pd,_mm_castps_si128(x0)); pd += 4;

        // third result pixel is 1X6X1 filtered
        x0 = _mm_add_ps(CL,R);
        x1 = _mm_mul_ps(CR,const6);
        x0 = _mm_add_ps(x0,x1); // horizontally filtered by 1x6x1
        x0 = _mm_mul_ps(x0,constNorm1X6X1); // normalized
        pfnStore((uint8x16_t*)pd,_mm_castps_si128(x0)); pd += 4;

        // second result pixel is X4X4X filtered
        x0 = _mm_add_ps(CR,R);
        x0 = _mm_mul_ps(x0,constNormX4X4X);
        pfnStore((uint8x16_t*)pd,_mm_castps_si128(x0)); pd += 4;

        L = CR;
        CL = R;
        wd -= Upsample2to1by14641SIMDBatch;
    }
    return ret;
}

// 'between' source pixels vertically; Byte I/O
template<void (*pfnStore)(uint8x16_t*, const uint8x16_t&)>
int Upsample2to1by14641ARowSIMD(Byte* pd, 
                               const Byte* pst, const Byte* psb,
                               int wd, const uint16_t* lvfpix, const uint16_t* rvfpix)
{
    int ret = wd&(~(Upsample2to1by14641SIMDBatch-1)); // number of dest pixels completed
    const Byte* pstlv = pst + (ret/2)*4;

#if defined(VT_SSE_SUPPORTED)
    __m128i zero = _mm_setzero_si128();
#endif
    uint16x8_t x0,x1,x2,x3;
    uint16x8_t L,C,R;

    // load L(eft) pix from left pix buffer
    L = _mm_load_si128((const uint8x16_t*)lvfpix);

    // load and vertically-filter two RGBA pixels for C(enter)
#if defined(VT_SSE_SUPPORTED)
    x0 = _mm_loadl_epi64((const __m128i*)pst); pst += (4*2);
    x1 = _mm_loadl_epi64((const __m128i*)psb); psb += (4*2);
    x0 = _mm_unpacklo_epi8(x0,zero);
    x1 = _mm_unpacklo_epi8(x1,zero);
    x0 = _mm_adds_epu16(x0,x1);
#elif defined(VT_NEON_SUPPORTED)
    {
        uint8x8_t x0n = vld1_u8(pst); pst += (4*2);
        uint8x8_t x1n = vld1_u8(psb); psb += (4*2);
        x0 = vaddl_u8(x0n,x1n);
    }
#endif
    C = _mm_slli_epi16(x0,2); // 8.3 vertically filtered

    while ( wd >= Upsample2to1by14641SIMDBatch )
    {
        if (pst == pstlv)
        {
            R = _mm_load_si128((const uint8x16_t*)rvfpix);
        }
        else
        {
            // load and vertically-filter two RGBA pixels for R(ight)
#if defined(VT_SSE_SUPPORTED)
            x0 = _mm_loadl_epi64((const __m128i*)pst); pst += (4*2); 
            x1 = _mm_loadl_epi64((const __m128i*)psb); psb += (4*2); 
            x0 = _mm_unpacklo_epi8(x0,zero);
            x1 = _mm_unpacklo_epi8(x1,zero);
            x0 = _mm_adds_epu16(x0,x1);
#elif defined(VT_NEON_SUPPORTED)
            {
                uint8x8_t x0n = vld1_u8(pst); pst += (4*2);
                uint8x8_t x1n = vld1_u8(psb); psb += (4*2);
                x0 = vaddl_u8(x0n,x1n);
            }
#endif
            R = _mm_slli_epi16(x0,2); // 8.3 vertically filtered
        }

        // horizontal filtering
        //
        // for 4 results R0,R1,R2,R3:
        // R0,R2 are 1x6x1 filtered, and R1,R3 are x4x4x filtered
        // L,C,R each have two pixels L0,L1, etc.
        // R0 = 1x6x1(L1,C0,C1)
        // R1 = x4x4x(C0,C1)
        // R2 = 1x6x1(C0,C1,R0)
        // R3 = x4x4x(C1,R0)
        //
        // so can combine computation for R0,R2 and R1,R3:
        //  R0R2 = 1x6x1(L1C0,C0C1,C1R0)
        //  R1R3 = x4x4x(C0C1,C1R0)
        // where:
        //  L1C0 = (L<<64)|(C>>64)
        //  C0C1 = C
        //  C1R0 = (C<<64)|(R>>64)
        // then rejigger R0R2,R1R3 into R0R1R2R3 for store
        //
#if defined(VT_SSE_SUPPORTED)
        x3 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(L),_mm_castsi128_pd(C),1)); // L1C0
        x2 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(C),_mm_castsi128_pd(R),1)); // C1R0
#elif defined(VT_NEON_SUPPORTED)
        x3 = vextq_u16(L,C,4);
        x2 = vextq_u16(C,R,4);
#endif
        x1 = _mm_adds_epu16(x2,x3); // L1C0+C1R0
        x0 = _mm_slli_epi16(C,2);
        x1 = _mm_adds_epu16(x1,x0); // += (C0C1)<<2
        x0 = _mm_slli_epi16(C,1);
        x0 = _mm_adds_epu16(x1,x0); // += (C0C1)<<1 is unnormalized R0R2 in 8.6
        x1 = _mm_adds_epu16(C,x2);  // is unnormalized R1R3 in 8.4
#if defined(VT_SSE_SUPPORTED)
        x0 = _mm_adds_epu16(x0,_mm_set1_epi16(1<<5));
        x0 = _mm_srli_epi16(x0,6); // normalized R0R2
        x1 = _mm_adds_epu16(x1,_mm_set1_epi16(1<<3));
        x1 = _mm_srli_epi16(x1,4); // normalized R1R3
        x0 = _mm_packus_epi16(x0,x1); // Byte R0R2R1R3
        x0 = _mm_shuffle_epi32(x0,_MM_SHUFFLE(3,1,2,0)); // R0R1R2R3
#elif defined(VT_NEON_SUPPORTED)
        {
            uint8x8x2_t x0t0;
            x0t0.val[0] = vrshrn_n_u16(x0,6); // normalized and packed R0R2
            x0t0.val[1] = vrshrn_n_u16(x1,4); // normalized and packed R1R3
            uint32x2x2_t x0t1 = vzip_u32((uint32x2_t)x0t0.val[0],(uint32x2_t)x0t0.val[1]); // R0R1, R2R3
            x0 = vcombine_u64(x0t1.val[0],x0t1.val[1]); // R0R1R2R3
        }
#endif
        pfnStore((uint8x16_t*)pd,x0); pd += (4*4);

        L = C;
        C = R;
        wd -= Upsample2to1by14641SIMDBatch;
    }

    return ret; 
}

// 'on' source pixels vertically; float I/O
template<void (*pfnStore)(uint8x16_t*, const uint8x16_t&)>
int Upsample2to1by14641BRowSIMD(float* pd, 
                               const float* pst, const float* psc, const float* psb,
                               int wd, const float* lvfpix, const float* rvfpix)
{
    int ret = wd&(~(Upsample2to1by14641SIMDBatch-1)); // number of dest pixels completed
    const float* pstlv = pst + (ret/2)*4;

    float32x4_t x0,x1,x2;
    float32x4_t L,CL,CR,R;
    float32x4_t const6 = _mm_set1_ps(6.f);
    float32x4_t constNormX4X4X = _mm_set1_ps(.0625f);
    float32x4_t constNorm1X6X1 = _mm_set1_ps(.015625f);

    // TODO: aligned loads on sources

    // load L(eft) pix from top of left pix buffer
    L = _mm_load_ps(lvfpix+4);

    // load and vertically filter one RBBA pixels for C(enter)L(eft)
    x0 = _mm_loadu_ps(pst); pst += 4;
    x1 = _mm_loadu_ps(psb); psb += 4;
    x2 = _mm_loadu_ps(psc); psc += 4;
    x0 = _mm_add_ps(x0,x1);
    x2 = _mm_mul_ps(x2,const6);
    CL = _mm_add_ps(x0,x2); // horizontally filtered by 1x6x1

    while ( wd >= Upsample2to1by14641SIMDBatch )
    {
        // load and vertically filter one RBBA pixels for C(enter)R(ight)
        x0 = _mm_loadu_ps(pst); pst += 4;
        x1 = _mm_loadu_ps(psb); psb += 4;
        x2 = _mm_loadu_ps(psc); psc += 4;
        x0 = _mm_add_ps(x0,x1);
        x2 = _mm_mul_ps(x2,const6);
        CR = _mm_add_ps(x0,x2); // horizontally filtered by 1x6x1

        // load and vertically filter one pixel for R(ight)
        if (pst == pstlv)
        {
            R = _mm_load_ps(rvfpix);
        }
        else
        {
            x0 = _mm_loadu_ps(pst); pst += 4;
            x1 = _mm_loadu_ps(psb); psb += 4;
            x2 = _mm_loadu_ps(psc); psc += 4;
            x0 = _mm_add_ps(x0,x1);
            x2 = _mm_mul_ps(x2,const6);
            R = _mm_add_ps(x0,x2); // horizontally filtered by 1x6x1
        }

        // L,CL,CR,R
        // R0 = 1X6X1(L,CL,CR)
        // R1 = X4X4X(CL,CR)
        // R2 = 1X6X1(CL,CR,R)
        // R3 = X4X4(CR,R)

        // first result pixel is 1X6X1 filtered
        x0 = _mm_add_ps(L,CR);
        x1 = _mm_mul_ps(CL,const6);
        x0 = _mm_add_ps(x0,x1); // horizontally filtered by 1x6x1
        x0 = _mm_mul_ps(x0,constNorm1X6X1); // normalized
        pfnStore((uint8x16_t*)pd,_mm_castps_si128(x0)); pd += 4;

        // second result pixel is X4X4X filtered
        x0 = _mm_add_ps(CL,CR);
        x0 = _mm_mul_ps(x0,constNormX4X4X);
        pfnStore((uint8x16_t*)pd,_mm_castps_si128(x0)); pd += 4;

        // third result pixel is 1X6X1 filtered
        x0 = _mm_add_ps(CL,R);
        x1 = _mm_mul_ps(CR,const6);
        x0 = _mm_add_ps(x0,x1); // horizontally filtered by 1x6x1
        x0 = _mm_mul_ps(x0,constNorm1X6X1); // normalized
        pfnStore((uint8x16_t*)pd,_mm_castps_si128(x0)); pd += 4;

        // second result pixel is X4X4X filtered
        x0 = _mm_add_ps(CR,R);
        x0 = _mm_mul_ps(x0,constNormX4X4X);
        pfnStore((uint8x16_t*)pd,_mm_castps_si128(x0)); pd += 4;

        L = CR;
        CL = R;
        wd -= Upsample2to1by14641SIMDBatch;
    }
    return ret;
}

// 'on' source pixel vertically; Byte I/O
template<void (*pfnStore)(uint8x16_t*, const uint8x16_t&)>
int Upsample2to1by14641BRowSIMD(Byte* pd, 
                               const Byte* pst, const Byte* psc, const Byte* psb,
                               int wd, const uint16_t* lvfpix, const uint16_t* rvfpix)
{
    int ret = wd&(~(Upsample2to1by14641SIMDBatch-1)); // number of dest pixels completed
    const Byte* pstlv = pst + (ret/2)*4;

#if defined(VT_SSE_SUPPORTED)
    __m128i zero = _mm_setzero_si128();
#endif
    uint16x8_t x0,x1,x2,x3;
    uint16x8_t L,C,R;

    // load L(eft) pix from left pix buffer
    L = _mm_load_si128((const uint8x16_t*)lvfpix);

    // load and vertically-filter two RGBA pixels for C(enter)
#if defined(VT_SSE_SUPPORTED)
    x0 = _mm_loadl_epi64((const __m128i*)pst); pst += (4*2);
    x1 = _mm_loadl_epi64((const __m128i*)psc); psc += (4*2);
    x2 = _mm_loadl_epi64((const __m128i*)psb); psb += (4*2);
    x0 = _mm_unpacklo_epi8(x0,zero);
    x1 = _mm_unpacklo_epi8(x1,zero);
    x2 = _mm_unpacklo_epi8(x2,zero);
    x0 = _mm_adds_epu16(x0,x2); // L+R
    x1 = _mm_slli_epi16(x1,1); 
    x0 = _mm_adds_epu16(x0,x1); // += (C<<1)
    x1 = _mm_slli_epi16(x1,1); 
    C = _mm_adds_epu16(x0,x1); // += (C<<2) is 8.3 vertically filtered
#elif defined(VT_NEON_SUPPORTED)
    {
        uint8x8_t x0n = vld1_u8(pst); pst += (4*2);
        uint8x8_t x1n = vld1_u8(psc); psc += (4*2);
        uint8x8_t x2n = vld1_u8(psb); psb += (4*2);
        x0 = vaddl_u8(x0n,x2n); // L+R
        x1 = vshll_n_u8(x1n,1); // 16bpp C << 1
    }
    x0 = vqaddq_u16(x0,x1);
    x1 = vshlq_n_u16(x1,1); // C << (1+1)
    C = vqaddq_u16(x0,x1);
#endif

    while ( wd >= Upsample2to1by14641SIMDBatch )
    {
        if (pst == pstlv)
        {
            R = _mm_load_si128((const uint8x16_t*)rvfpix);
        }
        else
        {
            // load and vertically-filter two RGBA pixels for R(ight)
#if defined(VT_SSE_SUPPORTED)
            x0 = _mm_loadl_epi64((const __m128i*)pst); pst += (4*2); 
            x1 = _mm_loadl_epi64((const __m128i*)psc); psc += (4*2); 
            x2 = _mm_loadl_epi64((const __m128i*)psb); psb += (4*2); 
            x0 = _mm_unpacklo_epi8(x0,zero);
            x1 = _mm_unpacklo_epi8(x1,zero);
            x2 = _mm_unpacklo_epi8(x2,zero);
            x0 = _mm_adds_epu16(x0,x2); // L+R
            x1 = _mm_slli_epi16(x1,1); 
            x0 = _mm_adds_epu16(x0,x1); // += (C<<1)
            x1 = _mm_slli_epi16(x1,1); 
            R = _mm_adds_epu16(x0,x1); // += (C<<2) is 8.3 vertically filtered
#elif defined(VT_NEON_SUPPORTED)
            {
                uint8x8_t x0n = vld1_u8(pst); pst += (4*2);
                uint8x8_t x1n = vld1_u8(psc); psc += (4*2);
                uint8x8_t x2n = vld1_u8(psb); psb += (4*2);
                x0 = vaddl_u8(x0n,x2n); // L+R
                x1 = vshll_n_u8(x1n,1); // 16bpp C << 1
            }
            x0 = vqaddq_u16(x0,x1);
            x1 = vshlq_n_u16(x1,1); // C << (1+1)
            R = vqaddq_u16(x0,x1);
#endif
        }

        // same horizontal filtering as above
#if defined(VT_SSE_SUPPORTED)
        x3 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(L),_mm_castsi128_pd(C),1)); // L1C0
        x2 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(C),_mm_castsi128_pd(R),1)); // C1R0
#elif defined(VT_NEON_SUPPORTED)
        x3 = vextq_u16(L,C,4);
        x2 = vextq_u16(C,R,4);
#endif
        x1 = _mm_adds_epu16(x2,x3); // L1C0+C1R0
        x0 = _mm_slli_epi16(C,2);
        x1 = _mm_adds_epu16(x1,x0); // += (C0C1)<<2
        x0 = _mm_slli_epi16(C,1);
        x0 = _mm_adds_epu16(x1,x0); // += (C0C1)<<1 is unnormalized R0R2 in 8.6
        x1 = _mm_adds_epu16(C,x2);  // is unnormalized R1R3 in 8.4
#if defined(VT_SSE_SUPPORTED)
        x0 = _mm_adds_epu16(x0,_mm_set1_epi16(1<<5));
        x0 = _mm_srli_epi16(x0,6); // normalized R0R2
        x1 = _mm_adds_epu16(x1,_mm_set1_epi16(1<<3));
        x1 = _mm_srli_epi16(x1,4); // normalized R1R3
        x0 = _mm_packus_epi16(x0,x1); // Byte R0R2R1R3
        x0 = _mm_shuffle_epi32(x0,_MM_SHUFFLE(3,1,2,0)); // R0R1R2R3
#elif defined(VT_NEON_SUPPORTED)
        {
            uint8x8x2_t x0t0;
            x0t0.val[0] = vrshrn_n_u16(x0,6); // normalized and packed R0R2
            x0t0.val[1] = vrshrn_n_u16(x1,4); // normalized and packed R1R3
            uint32x2x2_t x0t1 = vzip_u32((uint8x8_t)x0t0.val[0],(uint8x8_t)x0t0.val[1]); // R0R1, R2R3
            x0 = vcombine_u64(x0t1.val[0],x0t1.val[1]); // R0R1R2R3
        }
#endif
        pfnStore((uint8x16_t*)pd,x0); pd += (4*4);

        L = C;
        C = R;
        wd -= Upsample2to1by14641SIMDBatch;
    }

    return ret; 
}
#endif

//-----------------------------------------------------------------------------
// 2:1 Upsampling
//-----------------------------------------------------------------------------
template <typename Tio, typename Ttmp>
static HRESULT 
SF14641InterpolateInternal(CImg& imgDst, const CRect& rctDst,
                    const CImg& imgSrc, CPoint ptSrc)
{
    VT_HR_BEGIN();

    int nb = imgDst.Bands();
    VT_ASSERT(nb == imgSrc.Bands());
    VT_ASSERT(nb <= 4);

    // determine the required source block
    vt::CRect rctSrc    = vt::CRect(imgSrc.Rect()) + ptSrc;
    CRect rctReqSrc = LayerRectAtLevel(rctDst, 1);
    rctReqSrc.InflateRect(1,1);
    VT_ASSERT( rctSrc.RectInRect(&rctReqSrc) );
    VT_USE(rctSrc);

    // origin in imgSrc
    CPoint srcOrigin = CPoint(
        (rctDst.left - (ptSrc.x<<1))>>1,
        (rctDst.top  - (ptSrc.y<<1))>>1);

    int ys = srcOrigin.y;
    int ydr = rctDst.top;
    for (int yd=0; yd < rctDst.Height(); yd++,ydr++)
    {
        Ttmp pbufa[4], pbufb[4], pbufc[4];
        Tio* pd = (Tio*)imgDst.BytePtr(yd);
        int wd = rctDst.Width();
        int xdr = rctDst.left;
        if (ydr & 0x1)
        {
            // dest is 'between' src pixels vertically, so vfilter with 'x4x4x' (average)

            // ptrs to two scanline sources
            const Tio* pyt = (Tio*)imgSrc.BytePtr(srcOrigin.x,ys);
            const Tio* pyb = (Tio*)imgSrc.BytePtr(srcOrigin.x,ys+1);
            // advance source address when 'between' source only
            ys++;

            // vector implementation
#if defined(VT_SIMD_SUPPORTED)
            if ( (xdr & 0x1) && (wd >= (Upsample2to1by14641SIMDBatch+1)) && (nb == 4) )
            {
                // first pixel is 'between' in x so do scalar since vector code only does first pixel 'on'
                FilterNBbyX4X4XP1(pbufa,pyt,pyb);
                FilterNBbyX4X4XP1(pbufb,pyt+4,pyb+4);
                FilterNBbyX4X4XP2(pd,pbufa,pbufb); pd+=4;
                pyt+=4; pyb+=4;
                wd -= 1; xdr++;
            }
            if ( (VT_UPSAMPLE14641_SIMD_SUPPORTED) && (wd > Upsample2to1by14641SIMDBatch) && (nb == 4) )
            // TODO: figure out what is failing when vector code does only 4 dest 
            {
                // compute left and right vertically filtered pixels for batch
                VT_DECLSPEC_ALIGN(16) Ttmp lpix[8];
                FilterNBbyX4X4XP1(&lpix[4],pyt-4,pyb-4);
                VT_DECLSPEC_ALIGN(16) Ttmp rpix[8];
                int lastROffsV = (wd&(~(Upsample2to1by14641SIMDBatch-1)))*(4/2);
                FilterNBbyX4X4XP1(&rpix[0],pyt+lastROffsV,pyb+lastROffsV);

                int wdv;
                SelectPSTFunc(IsAligned16(pd),wdv,Store4,
                    Upsample2to1by14641ARowSIMD,
                    pd,pyt,pyb,wd,lpix,rpix);

                wd -= wdv;
                pyt += ((wdv/2)*4);
                pyb += ((wdv/2)*4);
                pd += (wdv*4);
                xdr += wdv;
            }
#endif

            // scalar implementation/remainder
            for (int xd = rctDst.Width()-wd; xd < rctDst.Width(); xd++,xdr++)
            {
                if (xdr & 0x1)
                {
                    // 'between' src pixels horizontally and vertically, so
                    // filter two pixels vertically for horizontal inputs

                    // apply filter vertically for left
                    FilterNBbyX4X4XP1(pbufa,pyt,pyb,nb);
                    FilterNBbyX4X4XP1(pbufb,pyt+nb,pyb+nb,nb);

                    // apply horizontal filter and store
                    FilterNBbyX4X4XP2(pd,pbufa,pbufb,nb); pd+=nb;

                    // advance source ptrs for 'between' source only 
                    pyt+=nb; pyb+=nb;
                }
                else
                {
                    // 'on' src pixel horizontally and 'between' vertically,
                    // so filter three pixels vertically for horizontal inputs

                    // apply vertical filter
                    FilterNBbyX4X4XP1(pbufa,pyt-nb,pyb-nb,nb);
                    FilterNBbyX4X4XP1(pbufb,pyt,pyb,nb);
                    FilterNBbyX4X4XP1(pbufc,pyt+nb,pyb+nb,nb);

                    // apply horizontal filter and store
                    FilterNBby1X6X1P2(pd,pbufa,pbufb,pbufc,nb); pd+=nb;
                }
            }
        }
        else
        {
            // dest is 'on' source pixel vertically, so vfilter with '1x6x1'
            const Tio* pyt = (Tio*)imgSrc.BytePtr(srcOrigin.x,ys-1);
            const Tio* pyc = (Tio*)imgSrc.BytePtr(srcOrigin.x,ys);
            const Tio* pyb = (Tio*)imgSrc.BytePtr(srcOrigin.x,ys+1);

            // vector implementation
#if defined(VT_SIMD_SUPPORTED)
            if ( (xdr & 0x1) && (wd >= (Upsample2to1by14641SIMDBatch+1)) && (nb == 4) )
            {
                // first pixel is 'between' in x so do scalar since vector code only does first pixel 'on'
                FilterNBby1X6X1P1(pbufa,pyt,pyc,pyb);
                FilterNBby1X6X1P1(pbufb,pyt+4,pyc+4,pyb+4);
                FilterNBbyX4X4XP2(pd,pbufa,pbufb); pd+=4;
                pyt+=4; pyc+=4; pyb+=4;
                wd -= 1; xdr++;
            }
            if ( (VT_UPSAMPLE14641_SIMD_SUPPORTED) && (wd > Upsample2to1by14641SIMDBatch) && (nb == 4) )
            // TODO: figure out what is failing when vector code does only 4 dest 
            {
                // compute left and right vertically filtered pixels for batch
                VT_DECLSPEC_ALIGN(16) Ttmp lpix[8];
                FilterNBby1X6X1P1(&lpix[4],pyt-4,pyc-4,pyb-4);
                VT_DECLSPEC_ALIGN(16) Ttmp rpix[8];
                int lastROffsV = (wd&(~(Upsample2to1by14641SIMDBatch-1)))*(4/2);
                FilterNBby1X6X1P1(&rpix[0],pyt+lastROffsV,pyc+lastROffsV,pyb+lastROffsV);

                int wdv;
                SelectPSTFunc(IsAligned16(pd),wdv,Store4,
                    Upsample2to1by14641BRowSIMD,
                    pd,pyt,pyc,pyb,wd,lpix,rpix);

                wd -= wdv;
                pyt += ((wdv/2)*4);
                pyc += ((wdv/2)*4);
                pyb += ((wdv/2)*4);
                pd += (wdv*4);
                xdr += wdv;
            }
#endif

            // scalar implementation/remainder
            for (int xd = rctDst.Width()-wd; xd < rctDst.Width(); xd++,xdr++)
            {
                if (xdr & 0x1)
                {
                    // 'between' source pixels horizontally and 'on' vertically,
                    // so filter two pixels vertically for horizontal inputs
                    FilterNBby1X6X1P1(pbufa,pyt,pyc,pyb,nb);
                    FilterNBby1X6X1P1(pbufb,pyt+nb,pyc+nb,pyb+nb,nb);

                    // apply horizontal filter and store
                    FilterNBbyX4X4XP2(pd,pbufa,pbufb,nb); pd+=nb;

                    // advance source ptrs for 'between' source only 
                    pyt+=nb; pyc+=nb; pyb+=nb;
                }
                else
                {
                    // 'on' source pixel horizontally and 'on' vertically,
                    // so filter three pixels vertically for horizontal inputs
                    FilterNBby1X6X1P1(pbufa,pyt-nb,pyc-nb,pyb-nb,nb);
                    FilterNBby1X6X1P1(pbufb,pyt,pyc,pyb,nb);
                    FilterNBby1X6X1P1(pbufc,pyt+nb,pyc+nb,pyb+nb,nb);

                    // apply horizontal filter and store
                    FilterNBby1X6X1P2(pd,pbufa,pbufb,pbufc,nb); pd+=nb;
                }
            }
        }
    }
   VT_HR_END();
}


//-----------------------------------------------------------------------------
//
// 2:1 Downsampling
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// routines to apply 14641 downsample filter; Byte versions use 16 bit accumulation
// so full precision of intermediate results are retained
//-----------------------------------------------------------------------------
inline float x4Scale(float val) { return val*4.f; }
inline float x6Scale(float val) { return val*6.f; }
inline void div256Store(float* p, float val) { *p = val*0.00390625f; }

inline uint16_t x4Scale(uint16_t val) { return val<<2; }
inline uint16_t x6Scale(uint16_t val) { return (val<<2)+(val<<1); }
inline void div256Store(Byte* p, uint16_t val) { *p = (Byte)(val>>8); }

//-----------------------------------------------------------------------------
// SIMD routines for SSE and Neon
//-----------------------------------------------------------------------------
#if defined(VT_SIMD_SUPPORTED)

#if defined(VT_SSE_SUPPORTED)
#define VT_DOWNSAMPLE14641_SIMD_SUPPORTED (g_SupportSSSE3())
#elif defined(VT_NEON_SUPPORTED)
#define VT_DOWNSAMPLE14641_SIMD_SUPPORTED true
#endif

#if defined(VT_NEON_SUPPORTED)
template<uint8x8_t (*pfnLoad2)(const uint8x8_t*), float32x4_t (*pfnLoad4)(const float*), void (*pfnStore)(float*, const float32x4_t&)>
#else
template<__m128i (*pfnLoad2)(const __m128i*), float32x4_t (*pfnLoad4)(const float*), void (*pfnStore)(float*, const float32x4_t&)>
#endif
static int Downsample2to1by14641Vert4BandsSIMD(uint16_t* pd, const Byte* ps0, const Byte* ps1, const Byte* ps2, const Byte* ps3, const Byte* ps4, int count)
{
    int i = 0;
    for ( ; i<(count-7); i+=8)
    {
#if defined(VT_NEON_SUPPORTED)
        // the SSE path will work for Neon, but this is faster, particularly with
        // the 64 bit aligned loads
        uint16x8_t d = vaddl_u8(pfnLoad2((const uint8x8_t*)ps0),pfnLoad2((const uint8x8_t*)ps4)); ps0+=8; ps4+=8;
        d = vaddq_u16(d,vshll_n_u8(pfnLoad2((const uint8x8_t*)ps1),2)); ps1+=8;
        uint8x8_t s2n = pfnLoad2((const uint8x8_t*)ps2); ps2+=8;
        d = vaddq_u16(d,vaddq_u16(vshll_n_u8(s2n,2),vshll_n_u8(s2n,1)));
        d = vaddq_u16(d,vshll_n_u8(pfnLoad2((const uint8x8_t*)ps3),2)); ps3+=8;
#else
        __m128i s0 = _mm_loadl_epi64((const __m128i*)ps0); ps0 += 8;
        __m128i s1 = _mm_loadl_epi64((const __m128i*)ps1); ps1 += 8;
        __m128i s2 = _mm_loadl_epi64((const __m128i*)ps2); ps2 += 8;
        __m128i s3 = _mm_loadl_epi64((const __m128i*)ps3); ps3 += 8;
        __m128i d = _mm_loadl_epi64((const __m128i*)ps4); ps4 += 8;
        s0 = _mm_unpacklo_epi8(s0,_mm_setzero_si128());
        s1 = _mm_unpacklo_epi8(s1,_mm_setzero_si128());
        s2 = _mm_unpacklo_epi8(s2,_mm_setzero_si128());
        s3 = _mm_unpacklo_epi8(s3,_mm_setzero_si128());
        d = _mm_unpacklo_epi8(d,_mm_setzero_si128());
        s1 = _mm_slli_epi16(s1,2);
        __m128i s2t = _mm_slli_epi16(s2,2);
        s2 = _mm_slli_epi16(s2,1);
        s2 = _mm_add_epi16(s2,s2t);
        s3 = _mm_slli_epi16(s3,2);
        d = _mm_add_epi16(d,s0);
        d = _mm_add_epi16(d,s1);
        d = _mm_add_epi16(d,s2);
        d = _mm_add_epi16(d,s3);
#endif
        pfnStore((float*)pd,vreinterpretq_f32_u16(d)); pd += 8;
    }
    return i;
}
#if defined(VT_NEON_SUPPORTED)
template<uint8x8_t (*pfnLoad2)(const uint8x8_t*), float32x4_t (*pfnLoad4)(const float*), void (*pfnStore)(float*, const float32x4_t&)>
#else
template<__m128i (*pfnLoad2)(const __m128i*), float32x4_t (*pfnLoad4)(const float*), void (*pfnStore)(float*, const float32x4_t&)>
#endif
static int Downsample2to1by14641Vert4BandsSIMD(float* pd, const float* ps0, const float* ps1, const float* ps2, const float* ps3, const float* ps4, int count)
{
    int i = 0;
    for ( ; i<(count-3); i+=4)
    {
        // TODO: use Neon multiply-add
        float32x4_t s0 = pfnLoad4(ps0); ps0 += 4;
        float32x4_t s1 = pfnLoad4(ps1); ps1 += 4;
        float32x4_t s2 = pfnLoad4(ps2); ps2 += 4;
        float32x4_t s3 = pfnLoad4(ps3); ps3 += 4;
        float32x4_t d = pfnLoad4(ps4); ps4 += 4;
        s1 = _mm_mul_ps(s1,_mm_set1_ps(4.f));
        s2 = _mm_mul_ps(s2,_mm_set1_ps(6.f));
        s3 = _mm_mul_ps(s3,_mm_set1_ps(4.f));
        d = _mm_add_ps(d,s0);
        d = _mm_add_ps(d,s1);
        d = _mm_add_ps(d,s2);
        d = _mm_add_ps(d,s3);
        pfnStore(pd,d); pd += 4;
    }
    return i;
}

#if defined(VT_NEON_SUPPORTED)
template<void (*pfnStore2)(void*, const uint8x8_t&),void (*pfnStore4)(float*, const float32x4_t&)>
#else
template<void (*pfnStore2)(__m128i*, const __m128i&),void (*pfnStore4)(float*, const float32x4_t&)>
#endif
static int Downsample2to1by14641Horz4BandsSIMD(Byte* pd, const uint16_t* ps, int count)
{
    // operates on 8 source pixels and generates 2 destination pixels per loop;
    // 4 of the source pixels are replaced each iteration
    //
    // src: |A0A1|B0B1|C0C1|D0D1|
    // dst:       R0   R1   R2   R3
    // R0R1 = (A0B0)+(4*A1B1)+(6*B0C0)+(4*B1C1)+(C0D0)
    //
    // Note that D1 in the last loop iteration is beyond the source image bounds,
    // so the line buffer needs to be over allocated by at least 1 pixel
    //
    uint16x8_t A,B,A0B0;
    A = _mm_load_si128((const uint8x16_t*)ps); ps += 8;
    B = _mm_load_si128((const uint8x16_t*)ps); ps += 8;
    A0B0 = _simd_mergelo_si128(A,B);

    int i = 0;
    for ( ; i<(count-1); i+=2)
    {
        uint16x8_t x0,x1;
        x1 = _simd_mergehi_si128(A,B);  // A1B1

        A = _mm_load_si128((const uint8x16_t*)ps); ps += 8;
        x1 = _mm_slli_epi16(x1,2);
        x0 = _mm_adds_epu16(A0B0,x1);     // = 1*s1 + 4*s1

        x1 = _simd_mergehi_si128(B,A);  // B1C1
        x1 = _mm_slli_epi16(x1,2);
        x0 = _mm_adds_epu16(x0,x1);     // += 4*s1

        x1 = _simd_mergelo_si128(B,A);  // B0C0
        x1 = _mm_slli_epi16(x1,1);
        x0 = _mm_adds_epu16(x0,x1);     // += 2*s3
        x1 = _mm_slli_epi16(x1,1);
        x0 = _mm_adds_epu16(x0,x1);     // += 4*s3

        B = _mm_load_si128((const uint8x16_t*)ps); ps += 8;

        A0B0 = _simd_mergelo_si128(A,B);  // C0D0
        x0 = _mm_adds_epu16(x0,A0B0);     // += 1*s4

#if defined(VT_NEON_SUPPORTED)
        uint8x8_t x0n = vshrn_n_u16(x0,8);
        pfnStore2((void*)pd,x0n); pd += 8;
#else
        x0 = _mm_srli_epi16(x0,8); // /= 256
        x0 = _mm_packus_epi16(x0,x0);
        pfnStore2((__m128i*)pd,x0); pd += 8;
#endif
    }
    return i; 
}
#if defined(VT_NEON_SUPPORTED)
template<void (*pfnStore2)(void*, const uint8x8_t&),void (*pfnStore4)(float*, const float32x4_t&)>
#else
template<void (*pfnStore2)(__m128i*, const __m128i&),void (*pfnStore4)(float*, const float32x4_t&)>
#endif
static int Downsample2to1by14641Horz4BandsSIMD(float* pd, const float* ps, int count)
{
    float32x4_t s,d0,d1,d2;

    //     --init--|--loop-->
    // dst0 1 4 6 4 1
    // dst1     1 4 6 4 1
    // dst2         1 4 6 4 1
    // dst3             1 4 6 4 1
    // src  0 1 2 3 4 5 6 7 8 9 ...
    // dst      0   1   2   3 ...

    // initialization of dest accumulator values
    d0 = _mm_load_ps(ps); ps+=4;
    s = _mm_load_ps(ps); ps+=4; d0 = _mm_add_ps(d0,_mm_mul_ps(s,_mm_set1_ps(4.f)));
    d1 = _mm_load_ps(ps); ps+=4; d0 = _mm_add_ps(d0,_mm_mul_ps(d1,_mm_set1_ps(6.f)));
    s = _mm_load_ps(ps); ps+=4; s = _mm_mul_ps(s,_mm_set1_ps(4.f)); d0 = _mm_add_ps(d0,s); d1 = _mm_add_ps(d1,s);
    // loop reads two samples and outputs one result
    for (int i=0; i<count; i++)
    {
        d2 = _mm_load_ps(ps); ps+=4; d0 = _mm_add_ps(d0,d2); d1 = _mm_add_ps(d1,_mm_mul_ps(d2,_mm_set1_ps(6.f)));
        d0 = _mm_mul_ps(d0,_mm_set1_ps(1.f/256.f)); pfnStore4(pd,d0); pd+=4;
        s = _mm_load_ps(ps); ps+=4; s = _mm_mul_ps(s,_mm_set1_ps(4.f)); d1 = _mm_add_ps(d1,s); d2 = _mm_add_ps(d2,s);
        d0 = d1; d1 = d2; // d2 will be replaced
    }
    return count; 
}

#endif

template <typename Tio, typename Ttmp>
HRESULT 
SF14641DecimateInternal(CImg& imgDst, const CRect& rctDst,
                    const CImg& imgSrc, CPoint ptSrc)
{
    VT_HR_BEGIN();

    // determine the required source block
    CRect rctSrc = CRect(imgSrc.Rect()) + ptSrc;
    CRect rctReqSrc = CRect((rctDst.left-1)<<1,(rctDst.top-1)<<1,(rctDst.right<<1)+1,(rctDst.bottom<<1)+1);
    VT_ASSERT( rctSrc.RectInRect(&rctReqSrc) );
    VT_USE(rctReqSrc);

    // origin in imgSrc
    CPoint srcOrigin = CPoint(
        (rctDst.left<<1) - ptSrc.x,
        (rctDst.top<<1)  - ptSrc.y);

    int nb = imgSrc.Bands();
    VT_ASSERT( nb == imgDst.Bands() );

    // allocate line buffer slightly larger due to SIMD loading 1 pixel beyond
    // the end of the buffer
    CByteImg lineBuf; 
    VT_HR_EXIT( lineBuf.Create(sizeof(Ttmp)*(rctSrc.Width()+1),1,nb) );
    Ttmp* plb = (Ttmp*)lineBuf.BytePtr();

    for (int yd = 0; yd < rctDst.Height(); yd++)
    {
        // vertically filter scanline to line buffer
        {
            int sstride = imgSrc.StrideBytes()/sizeof(Tio);
            Tio* ps0 = (Tio*)imgSrc.BytePtr(srcOrigin.x-2,srcOrigin.y-2+(yd<<1));
            Tio* ps1 = ps0+sstride;
            Tio* ps2 = ps1+sstride;
            Tio* ps3 = ps2+sstride;
            Tio* ps4 = ps3+sstride;
            Ttmp* pd = plb;
            int ws = 0;
#if defined(VT_SIMD_SUPPORTED)
            if ( VT_DOWNSAMPLE14641_SIMD_SUPPORTED )
            {
                Tio* vptr = 0;
                SelectPSTFunc3(IsAligned16(pd)&&IsAligned16(ps0)&&IsAligned16(vptr+sstride),
                    ws, Load2, Load4, Store4, Downsample2to1by14641Vert4BandsSIMD,
                    pd,ps0,ps1,ps2,ps3,ps4,nb*rctSrc.Width());
                ps0 += ws; ps1 += ws; ps2 += ws; ps3 += ws; ps4 += ws;
                pd += ws;
            }
#endif
            for ( ; ws < nb*rctSrc.Width(); ws++)
            {
                *pd++ = (*ps0++) + x4Scale((Ttmp)(*ps1++)) + x6Scale((Ttmp)(*ps2++)) + x4Scale((Ttmp)(*ps3++)) + (*ps4++);
            }
        }

        // horizontally filter from line buffer to destination
        //
        {
            Ttmp* ps = plb;
            Tio* pd = (Tio*)imgDst.BytePtr(yd);
            int wd = 0;
#if defined(VT_SIMD_SUPPORTED)
            if ( (nb == 4) && (rctDst.Width() >= 2) && VT_DOWNSAMPLE14641_SIMD_SUPPORTED )
            {
                SelectPSTFunc2(IsAligned16(pd),
                    wd, Store2, Store4, Downsample2to1by14641Horz4BandsSIMD,
                    pd,ps,rctDst.Width());
                ps += nb*(2*(wd-1));
                pd += nb*(wd);
            }
#endif
            if (wd < rctDst.Width())
            {
                Ttmp d0[4], d1[4], d2[4], s;
                //
                // code operates on three destination pixels at a time, accumulating
                // the appropriately scaled contribution of each new source pixel 
                // into the three dest pixels
                //
                //     --init--|--loop-->
                // dst0 1 4 6 4 1
                // dst1     1 4 6 4 1
                // dst2         1 4 6 4 1
                // dst3             1 4 6 4 1
                // src  0 1 2 3 4 5 6 7 8 9 ...
                // dst      0   1   2   3 ...
                //
                // initialization of dest accumulator values
                for (int b=0; b<nb; b++) { s = *ps++; d0[b] = s; }
                for (int b=0; b<nb; b++) { s = *ps++; d0[b] += x4Scale(s); }
                for (int b=0; b<nb; b++) { s = *ps++; d0[b] += x6Scale(s); d1[b] = s; }
                for (int b=0; b<nb; b++) { s = *ps++; s = x4Scale(s); d0[b] += s; d1[b] += s; }
                // loop reads two samples and outputs one result
                for ( ; wd < rctDst.Width(); wd++)
                {
                    for (int b=0; b<nb; b++) { s = *ps++; d0[b] += s; d1[b] += x6Scale(s); d2[b] = s; }
                    for (int b=0; b<nb; b++) { div256Store(pd,d0[b]); pd++; }
                    for (int b=0; b<nb; b++) { s = *ps++; s = x4Scale(s); d1[b] += s; d2[b] += s; }
                    for (int b=0; b<nb; b++) { d0[b] = d1[b]; d1[b] = d2[b]; } // d2[b] will be replaced
                }
            }
        }
    }
    
    VT_HR_END();
}

// non-templated wrappers to call from separablefilter.cpp
HRESULT SeparableFilter14641Decimate2to1Internal(CFloatImg& imgDst,const CRect& rctDst,
                    const CFloatImg& imgSrc, CPoint ptSrc)
{
    return SF14641DecimateInternal<float,float>(imgDst,rctDst,imgSrc,ptSrc);
}
HRESULT SeparableFilter14641Decimate2to1Internal(CByteImg& imgDst,const CRect& rctDst,
                    const CByteImg& imgSrc, CPoint ptSrc)
{
    return SF14641DecimateInternal<Byte,uint16_t>(imgDst,rctDst,imgSrc,ptSrc);
}

//-----------------------------------------------------------------------------
//
// public VtSeparableFilter14641* routines
//
//-----------------------------------------------------------------------------

// sampleType: 0 is 2:1 interpolation, 1 is 2:1 decimation
HRESULT
VtSeparableFilter14641Internal(CImg &imgDst, const CRect& rctDst, 
             				   const CImg &imgSrc, CPoint ptSrcOrigin,
			            	   const IMAGE_EXTEND& ex, int sampleType)
{
    if( !imgSrc.IsValid() )
    {
        return E_INVALIDSRC;
    }

    if( imgSrc.IsSharingMemory(imgDst) )
    {
        // the operation cannot be done in-place
        return E_INVALIDDST;
    }

    VT_HR_BEGIN()

    // create the destination image if it needs to be
    VT_HR_EXIT( CreateImageForTransform(imgDst, rctDst.Width(), rctDst.Height(),
                                        imgSrc.GetType()) );

    // check for valid convert conditions
    if( !VtIsValidConvertImagePair(imgDst, imgSrc) )
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    // determine the "operation" type.  
    int iOpBands = imgDst.Bands();
    int iOpType   = EL_FORMAT(imgDst.GetType());

    // check for valid inputs and outputs
    if( ( (iOpBands > 4) ) ||
        (imgSrc.GetType() != imgDst.GetType()) ||
        ( (iOpType != EL_FORMAT_BYTE) &&
          (iOpType != EL_FORMAT_FLOAT) ) )
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    // determine the rect of the full provided source
    vt::CRect rctSrc = imgSrc.Rect();

    // loop over the destination blocks
    const UInt32 c_blocksize = 128;

    CImg imgConv;
    CFloatImg imgTmp;
    for( CBlockIterator bi(rctDst, c_blocksize); !bi.Done(); bi.Advance() )
    {
        // get blk rct in dst image coords
        vt::CRect rctDstBlk = bi.GetRect();

        CImg imgDstBlk;
        imgDst.Share(imgDstBlk, &rctDstBlk);

        // offset to layer coords
        rctDstBlk += rctDst.TopLeft();

        // determine the required source block
        vt::CRect rctSrcBlk;
        if (sampleType == 0)
        {
            // interpolation
            rctSrcBlk = LayerRectAtLevel(rctDstBlk, 1);
            rctSrcBlk.InflateRect(1,1);
            // don't let source be smaller than 3x3 (happens for 1x1 input)
            if (rctSrcBlk.Width() <= 2) { rctSrcBlk.InflateRect(0,0,1,0); }
            if (rctSrcBlk.Height() <= 2) { rctSrcBlk.InflateRect(0,0,0,1); }
            VT_ASSERT(rctSrcBlk.Width() >= 3);
            VT_ASSERT(rctSrcBlk.Height() >= 3);
        }
        else // (sampleType == 1)
        {
            // decimation
            rctSrcBlk = CRect(
                (rctDstBlk.left-1)<<1,(rctDstBlk.top-1)<<1,
                (rctDstBlk.right<<1)+1,(rctDstBlk.bottom<<1)+1);
        }
        rctSrcBlk = rctSrcBlk - ptSrcOrigin;
        
        VT_ASSERT( !rctSrcBlk.IsRectEmpty() );
		
        if( EL_FORMAT(imgSrc.GetType()) == iOpType && 
            imgSrc.Bands() == iOpBands             && 
            rctSrc.RectInRect(&rctSrcBlk) )
        {
            // share the block out of the source if possible
            imgSrc.Share(imgConv, &rctSrcBlk);
        }
        else
        {
            // otherwise create a temp image block and pad the image when 
            // out of bounds pixels are requested
            VT_HR_EXIT( imgConv.Create(rctSrcBlk.Width(), rctSrcBlk.Height(),
									   VT_IMG_MAKE_TYPE(iOpType, iOpBands),
                                       align64ByteRows) );
            VT_HR_EXIT( VtCropPadImage(imgConv, rctSrcBlk, imgSrc, ex) );
        }

        if (iOpType == EL_FORMAT_BYTE)
        {
            if (sampleType == 0)
            {
                VT_HR_EXIT( (SF14641InterpolateInternal<Byte,uint16_t>(
                    imgDstBlk, imgDstBlk.Rect(), imgConv, CPoint(-1,-1))) );
            }
            else // (sampleType == 1)
            {
                VT_HR_EXIT( (SF14641DecimateInternal<Byte,uint16_t>(
                    imgDstBlk, imgDstBlk.Rect(), imgConv, CPoint(-2,-2))) );
            }
        }
        else // iOpType is EL_FORMAT_FLOAT
        {
            if (sampleType == 0)
            {
                VT_HR_EXIT( (SF14641InterpolateInternal<float,float>(
                    imgDstBlk, imgDstBlk.Rect(), imgConv, CPoint(-1,-1))) );
            }
            else // (sampleType == 1)
            {
                VT_HR_EXIT( (SF14641DecimateInternal<float,float>(
                    imgDstBlk, imgDstBlk.Rect(), imgConv, CPoint(-2,-2))) );
            }
        }
    }

    VT_HR_END()
}

HRESULT
vt::VtSeparableFilter14641Decimate2to1(CImg &imgDst, const CRect& rctDst, 
             				       const CImg &imgSrc, CPoint ptSrcOrigin,
			            	       const IMAGE_EXTEND& ex)
{
    return VtSeparableFilter14641Internal(imgDst,rctDst,imgSrc,ptSrcOrigin,ex,1);
}
HRESULT
vt::VtSeparableFilter14641Interpolate2to1(CImg &imgDst, const CRect& rctDst, 
             				          const CImg &imgSrc, CPoint ptSrcOrigin,
			            	          const IMAGE_EXTEND& ex)
{
    return VtSeparableFilter14641Internal(imgDst,rctDst,imgSrc,ptSrcOrigin,ex,0);
}

//-----------------------------------------------------------------------------
//
// CSeparableFilter14641Transform routines
//
//-----------------------------------------------------------------------------

#ifndef VT_NO_XFORMS

HRESULT CSeparableFilter14641Transform::Clone(ITaskState **ppState)
{
	return CloneTaskState<CSeparableFilter14641Transform>(ppState, 
		[this](CSeparableFilter14641Transform* pN)
	{ 
        return (m_sampleType == 0)?pN->InitializeInterpolate2to1(m_dstType):pN->InitializeDecimate2to1(m_dstType);
    });
}

HRESULT CSeparableFilter14641Transform::Transform(
    CImg* pimgDst, IN  const CRect& rctDst,
	const CImg& imgSrc, const CPoint& ptSrc)

{
    if (EL_FORMAT(pimgDst->GetType()) == EL_FORMAT_BYTE)
    {
        if (m_sampleType == 0)
        {
            if (pimgDst->GetType() == OBJ_RGBAIMG)
            {
                return SF14641InterpolateInternal<Byte,uint16_t>(*pimgDst, rctDst,
                    imgSrc, ptSrc);
            }
        }
        else if (m_sampleType == 1)
        {
            return SF14641DecimateInternal<Byte,uint16_t>(*pimgDst, rctDst,
                imgSrc, ptSrc);
        }
    }
    else if (EL_FORMAT(pimgDst->GetType()) == EL_FORMAT_FLOAT)
    {
        if (m_sampleType == 0)
        {
            if (pimgDst->GetType() == OBJ_RGBAFLOATIMG)
            {
                return SF14641InterpolateInternal<float,float>(*pimgDst, rctDst,
                    imgSrc, ptSrc);
            }
        }
        else if (m_sampleType == 1)
        {
            return SF14641DecimateInternal<float,float>(*pimgDst, rctDst,
                imgSrc, ptSrc);
        }
    }
    return E_INVALIDARG;
}

HRESULT CSeparableFilter14641Transform::InitializeInterpolate2to1(int dstType)
{
    VT_HR_BEGIN();

    if ( (dstType != VT_IMG_FIXED(OBJ_RGBAIMG)) && (dstType != VT_IMG_FIXED(OBJ_RGBAFLOATIMG)) )
    {
        VT_HR_EXIT(E_INVALIDARG);
    }
    m_dstType = dstType;
    m_sampleType = 0;

    VT_HR_END();
}

HRESULT CSeparableFilter14641Transform::InitializeDecimate2to1(int dstType)
{
    VT_HR_BEGIN();

    if ( (dstType != VT_IMG_FIXED(OBJ_RGBAIMG)) && 
         (dstType != VT_IMG_FIXED(OBJ_RGBIMG)) && 
         (dstType != VT_IMG_FIXED(OBJ_UVIMG)) && 
         (dstType != VT_IMG_FIXED(OBJ_LUMAIMG)) && 
         (dstType != VT_IMG_FIXED(OBJ_RGBAFLOATIMG)) &&
         (dstType != VT_IMG_FIXED(OBJ_RGBFLOATIMG)) &&
         (dstType != VT_IMG_FIXED(OBJ_UVFLOATIMG)) &&
         (dstType != VT_IMG_FIXED(OBJ_LUMAFLOATIMG))
         )
    {
        VT_HR_EXIT(E_INVALIDARG);
    }
    m_dstType = dstType;
    m_sampleType = 1;

    VT_HR_END();
}

#endif

//-----------------------------------------------------------------------------
// end