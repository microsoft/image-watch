//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for kernels
//
//  History:
//      2008/2/27-swinder
//          Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_image.h"
#include "vt_utils.h"
#include "vt_kernel.h"
#include "vt_separablefilter.h"
#include "vt_convert.h"
#include "vt_convert.inl"
#include "vt_pixeliterators.h"
#include "vt_pad.h"

using namespace vt;

template <typename T> struct FilterAlignTraits;

//+-----------------------------------------------------------------------------
//
// SSE specializations of Load/Store for Byte images
//
//------------------------------------------------------------------------------

template<> struct FilterAlignTraits<Byte> 
{
    static bool IsLoadAligned4Band(const void* p, int)
    { return ((uintptr_t(p)) & 0) == 0; }
    static bool IsStoreAligned4Band(const void* p, int iStride)
    { return ((uintptr_t(p) | iStride) & 15) == 0; } 
    static bool IsLoadAligned1Band(const void* p, int)
    { return ((uintptr_t(p)) & 0) == 0; }
    static bool IsStoreAligned1Band(const void* p, int)
    { return ((uintptr_t(p)) & 3) == 0; }
};

#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)
inline __m128 LoadFourFloatsAligned(const Byte *pSrc)
{
    __m128i mmi;
    mmi = _mm_set1_epi32(((const int *)pSrc)[0]);
    __m128i mm_zero = _mm_setzero_si128();
    __m128i mmi2 = _mm_unpacklo_epi8(mmi, mm_zero);
    __m128i mmi3 = _mm_unpacklo_epi16(mmi2, mm_zero);
    __m128 mm = _mm_cvtepi32_ps(mmi3);
    return mm;
}

inline __m128 LoadFourFloatsUnaligned(const Byte *pSrc)
{ 
    return LoadFourFloatsAligned(pSrc); 
}

inline void TransposeStoreAlignedFourBands(
    __m128 &mm_acc0, __m128 &mm_acc1, __m128 &mm_acc2, __m128 &mm_acc3, Byte *pDst)
{
    // convert and saturate
    __m128i mmi0 = _mm_cvtps_epi32(mm_acc0);
    __m128i mmi1 = _mm_cvtps_epi32(mm_acc1);
    __m128i mma = _mm_packs_epi32(mmi0, mmi1);

    __m128i mmi2 = _mm_cvtps_epi32(mm_acc2);
    __m128i mmi3 = _mm_cvtps_epi32(mm_acc3);
    __m128i mmb = _mm_packs_epi32(mmi2, mmi3);

    __m128i mmc = _mm_packus_epi16(mma, mmb);

    _mm_store_si128((__m128i *)pDst, mmc);
}

inline void TransposeStoreUnalignedFourBands(
    __m128 &mm_acc0, __m128 &mm_acc1, __m128 &mm_acc2, __m128 &mm_acc3, Byte *pDst)
{
    // convert and saturate
    __m128i mmi0 = _mm_cvtps_epi32(mm_acc0);
    __m128i mmi1 = _mm_cvtps_epi32(mm_acc1);
    __m128i mma = _mm_packs_epi32(mmi0, mmi1);

    __m128i mmi2 = _mm_cvtps_epi32(mm_acc2);
    __m128i mmi3 = _mm_cvtps_epi32(mm_acc3);
    __m128i mmb = _mm_packs_epi32(mmi2, mmi3);

    __m128i mmc = _mm_packus_epi16(mma, mmb);

    _mm_storeu_si128((__m128i *)pDst, mmc);
}

inline void TransposeStoreAlignedOneBand(
    __m128 &mm_acc0, __m128 &mm_acc1, __m128 &mm_acc2, __m128 &mm_acc3, 
    Byte *pDst, int iDstStrideBytes)
{
    _MM_TRANSPOSE4_PS(mm_acc0, mm_acc1, mm_acc2, mm_acc3);

    // convert and saturate
    __m128i mmi0 = _mm_cvtps_epi32(mm_acc0);
    __m128i mmi1 = _mm_cvtps_epi32(mm_acc1);
    __m128i mma = _mm_packs_epi32(mmi0, mmi1);

    __m128i mmi2 = _mm_cvtps_epi32(mm_acc2);
    __m128i mmi3 = _mm_cvtps_epi32(mm_acc3);
    __m128i mmb = _mm_packs_epi32(mmi2, mmi3);

    __m128i mmc = _mm_packus_epi16(mma, mmb);

    ((unsigned __int32 *)pDst)[0] = mmc.m128i_u32[0];
    ((unsigned __int32 *)PointerOffset(pDst, iDstStrideBytes))[0] = mmc.m128i_u32[1];
    ((unsigned __int32 *)PointerOffset(pDst, 2*iDstStrideBytes))[0] = mmc.m128i_u32[2];
    ((unsigned __int32 *)PointerOffset(pDst, 3*iDstStrideBytes))[0] = mmc.m128i_u32[3];
}

inline void TransposeStoreUnalignedOneBand(
    __m128 &mm_acc0, __m128 &mm_acc1, __m128 &mm_acc2, __m128 &mm_acc3, 
    Byte *pDst, int iDstStrideBytes)
{
    _MM_TRANSPOSE4_PS(mm_acc0, mm_acc1, mm_acc2, mm_acc3);

    // convert and saturate
    __m128i mmi0 = _mm_cvtps_epi32(mm_acc0);
    __m128i mmi1 = _mm_cvtps_epi32(mm_acc1);
    __m128i mma = _mm_packs_epi32(mmi0, mmi1);

    __m128i mmi2 = _mm_cvtps_epi32(mm_acc2);
    __m128i mmi3 = _mm_cvtps_epi32(mm_acc3);
    __m128i mmb = _mm_packs_epi32(mmi2, mmi3);

    __m128i mmc = _mm_packus_epi16(mma, mmb);

    // unaligned byte store
    __m128i mmmask = _mm_set_epi32(0,0,0,0xffffffff);
#if 0 // compiler bug workaround - Visual Studio 2001 SP1 stores two of the _mm_maskmoveu_si128 stores to the same address (not applying the stride multiple in the right place)
    _mm_maskmoveu_si128(mmc, mmmask, (char *)pDst);
    mmc = _mm_srli_si128(mmc, 4);
    _mm_maskmoveu_si128(mmc, mmmask, (char *)PointerOffset(pDst, iDstStrideBytes));
    mmc = _mm_srli_si128(mmc, 4);
    _mm_maskmoveu_si128(mmc, mmmask, (char *)PointerOffset(pDst, 2*iDstStrideBytes));
    mmc = _mm_srli_si128(mmc, 4);
    _mm_maskmoveu_si128(mmc, mmmask, (char *)PointerOffset(pDst, 3*iDstStrideBytes));
#else
    _mm_maskmoveu_si128(mmc, mmmask, (char *)pDst); pDst+=iDstStrideBytes;
    mmc = _mm_srli_si128(mmc, 4);
    _mm_maskmoveu_si128(mmc, mmmask, (char *)pDst); pDst+=iDstStrideBytes;
    mmc = _mm_srli_si128(mmc, 4);
    _mm_maskmoveu_si128(mmc, mmmask, (char *)pDst); pDst+=iDstStrideBytes;
    mmc = _mm_srli_si128(mmc, 4);
    _mm_maskmoveu_si128(mmc, mmmask, (char *)pDst);
#endif
}
#endif

//+-----------------------------------------------------------------------------
//
// SSE specializations of Load/Store for UInt16 images
//
//------------------------------------------------------------------------------
template<> struct FilterAlignTraits<UInt16> 
{
    static bool IsLoadAligned4Band(const void* p, int iStride)
    { return ((uintptr_t(p) | iStride) & 7) == 0; }
    static bool IsStoreAligned4Band(const void* p, int iStride)
    { return ((uintptr_t(p) | iStride) & 15) == 0; }
    static bool IsLoadAligned1Band(const void* p, int iStride)
    { return ((uintptr_t(p) | iStride) & 7) == 0; }
    static bool IsStoreAligned1Band(const void* p, int iStride)
    { return ((uintptr_t(p) | iStride) & 7) == 0; }
};

#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)
inline __m128 LoadFourFloatsAligned(const UInt16 *pSrc)
{
    __m128i mmi1 = _mm_loadl_epi64((const __m128i *)pSrc);
    __m128i mm_zero = _mm_setzero_si128();
    __m128i mmi2 = _mm_unpacklo_epi16(mmi1, mm_zero);
    return _mm_cvtepi32_ps(mmi2);
}

inline __m128 LoadFourFloatsUnaligned(const UInt16 *pSrc)
{
    __m128i mmi;
    mmi.m128i_u32[0] = pSrc[0];
    mmi.m128i_u32[1] = pSrc[1];
    mmi.m128i_u32[2] = pSrc[2];
    mmi.m128i_u32[3] = pSrc[3];
    return _mm_cvtepi32_ps(mmi);
}

inline void TransposeStoreAlignedFourBands(
    __m128 &mm_acc0, __m128 &mm_acc1, __m128 &mm_acc2, __m128 &mm_acc3, UInt16 *pDst)
{
    __m128i mm8000_32 = _mm_set1_epi32(0x00008000);
    __m128i mm8000_16 = _mm_set1_epi16(-0x8000);

    __m128i mmi0 = _mm_cvtps_epi32(mm_acc0);
    __m128i mmi1 = _mm_cvtps_epi32(mm_acc1);

    mmi0 = _mm_sub_epi32(mmi0, mm8000_32); // subtract 32768 to make desired range -32768 -> 32767
    mmi1 = _mm_sub_epi32(mmi1, mm8000_32);
    __m128i mmipack = _mm_packs_epi32(mmi0, mmi1); // signed saturate to Int16
    mmipack = _mm_add_epi16(mmipack, mm8000_16); // add 32768 back on to make it UInt16

    _mm_store_si128((__m128i *)pDst, mmipack);

    __m128i mmi2 = _mm_cvtps_epi32(mm_acc2);
    __m128i mmi3 = _mm_cvtps_epi32(mm_acc3);

    mmi2 = _mm_sub_epi32(mmi2, mm8000_32); // subtract 32768 to make desired range -32768 -> 32767
    mmi3 = _mm_sub_epi32(mmi3, mm8000_32);
    mmipack = _mm_packs_epi32(mmi2, mmi3); // signed saturate to Int16
    mmipack = _mm_add_epi16(mmipack, mm8000_16); // add 32768 back on to make it UInt16

    _mm_store_si128((__m128i *)(pDst+8), mmipack);
}

inline void TransposeStoreUnalignedFourBands(
    __m128 &mm_acc0, __m128 &mm_acc1, __m128 &mm_acc2, __m128 &mm_acc3, UInt16 *pDst)
{
    __m128i mm8000_32 = _mm_set1_epi32(0x00008000);
    __m128i mm8000_16 = _mm_set1_epi16(-0x8000);

    __m128i mmi0 = _mm_cvtps_epi32(mm_acc0);
    __m128i mmi1 = _mm_cvtps_epi32(mm_acc1);

    mmi0 = _mm_sub_epi32(mmi0, mm8000_32); // subtract 32768 to make desired range -32768 -> 32767
    mmi1 = _mm_sub_epi32(mmi1, mm8000_32);
    __m128i mmipack = _mm_packs_epi32(mmi0, mmi1); // signed saturate to Int16
    mmipack = _mm_add_epi16(mmipack, mm8000_16); // add 32768 back on to make it UInt16

    _mm_storeu_si128((__m128i *)pDst, mmipack);

    __m128i mmi2 = _mm_cvtps_epi32(mm_acc2);
    __m128i mmi3 = _mm_cvtps_epi32(mm_acc3);

    mmi2 = _mm_sub_epi32(mmi2, mm8000_32); // subtract 32768 to make desired range -32768 -> 32767
    mmi3 = _mm_sub_epi32(mmi3, mm8000_32);
    mmipack = _mm_packs_epi32(mmi2, mmi3); // signed saturate to Int16
    mmipack = _mm_add_epi16(mmipack, mm8000_16); // add 32768 back on to make it UInt16

    _mm_storeu_si128((__m128i *)(pDst+8), mmipack);
}

inline void TransposeStoreAlignedOneBand(
    __m128 &mm_acc0, __m128 &mm_acc1, __m128 &mm_acc2, __m128 &mm_acc3, 
    UInt16 *pDst, int iDstStrideBytes)
{
    _MM_TRANSPOSE4_PS(mm_acc0, mm_acc1, mm_acc2, mm_acc3);

    __m128i mm8000_32 = _mm_set1_epi32(0x00008000);
    __m128i mm8000_16 = _mm_set1_epi16(short(-0x8000));

    __m128i mmi0 = _mm_cvtps_epi32(mm_acc0);
    __m128i mmi1 = _mm_cvtps_epi32(mm_acc1);

    mmi0 = _mm_sub_epi32(mmi0, mm8000_32); // subtract 32768 to make desired range -32768 -> 32767
    mmi1 = _mm_sub_epi32(mmi1, mm8000_32);
    __m128i mmipack = _mm_packs_epi32(mmi0, mmi1); // signed saturate to Int16
    mmipack = _mm_add_epi16(mmipack, mm8000_16); // add 32768 back on to make it UInt16

    _mm_storel_epi64((__m128i *)pDst, mmipack);
    mmipack = _mm_srli_si128(mmipack, 8);
    _mm_storel_epi64((__m128i *)PointerOffset(pDst, iDstStrideBytes), mmipack);

    __m128i mmi2 = _mm_cvtps_epi32(mm_acc2);
    __m128i mmi3 = _mm_cvtps_epi32(mm_acc3);

    mmi2 = _mm_sub_epi32(mmi2, mm8000_32); // subtract 32768 to make desired range -32768 -> 32767
    mmi3 = _mm_sub_epi32(mmi3, mm8000_32);
    mmipack = _mm_packs_epi32(mmi2, mmi3); // signed saturate to Int16
    mmipack = _mm_add_epi16(mmipack, mm8000_16); // add 32768 back on to make it UInt16

    _mm_storel_epi64((__m128i *)PointerOffset(pDst, 2*iDstStrideBytes), mmipack);
    mmipack = _mm_srli_si128(mmipack, 8);
    _mm_storel_epi64((__m128i *)PointerOffset(pDst, 3*iDstStrideBytes), mmipack);
}

inline void TransposeStoreUnalignedOneBand(
    __m128 &mm_acc0, __m128 &mm_acc1, __m128 &mm_acc2, __m128 &mm_acc3, 
    UInt16 *pDst, int iDstStrideBytes)
{
    _MM_TRANSPOSE4_PS(mm_acc0, mm_acc1, mm_acc2, mm_acc3);

    __m128i mm8000_32 = _mm_set1_epi32(0x00008000);
    __m128i mm8000_16 = _mm_set1_epi16(short(-0x8000));
    __m128i mmmask = _mm_set_epi32(0,0,0xffffffff,0xffffffff);

    __m128i mmi0 = _mm_cvtps_epi32(mm_acc0);
    __m128i mmi1 = _mm_cvtps_epi32(mm_acc1);

    mmi0 = _mm_sub_epi32(mmi0, mm8000_32); // subtract 32768 to make desired range -32768 -> 32767
    mmi1 = _mm_sub_epi32(mmi1, mm8000_32);
    __m128i mmipack = _mm_packs_epi32(mmi0, mmi1); // signed saturate to Int16
    mmipack = _mm_add_epi16(mmipack, mm8000_16); // add 32768 back on to make it UInt16

    _mm_maskmoveu_si128(mmipack, mmmask, (char *)pDst);
    mmipack = _mm_srli_si128(mmipack, 8);
    _mm_maskmoveu_si128(mmipack, mmmask, (char *)PointerOffset(pDst, iDstStrideBytes));

    __m128i mmi2 = _mm_cvtps_epi32(mm_acc2);
    __m128i mmi3 = _mm_cvtps_epi32(mm_acc3);

    mmi2 = _mm_sub_epi32(mmi2, mm8000_32); // subtract 32768 to make desired range -32768 -> 32767
    mmi3 = _mm_sub_epi32(mmi3, mm8000_32);
    mmipack = _mm_packs_epi32(mmi2, mmi3); // signed saturate to Int16
    mmipack = _mm_add_epi16(mmipack, mm8000_16); // add 32768 back on to make it UInt16

    // not sure if this is really the best way of storing unaligned UInt16
    _mm_maskmoveu_si128(mmipack, mmmask, (char *)PointerOffset(pDst, 2*iDstStrideBytes));
    mmipack = _mm_srli_si128(mmipack, 8);
    _mm_maskmoveu_si128(mmipack, mmmask, (char *)PointerOffset(pDst, 3*iDstStrideBytes));
}
#endif

//+-----------------------------------------------------------------------------
//
// SSE specializations of Load/Store for Float images
//
//------------------------------------------------------------------------------

template<> struct FilterAlignTraits<float> 
{
    static bool IsLoadAligned4Band(const void* p, int iStride)
    { return ((uintptr_t(p) | iStride) & 15) == 0; }
    static bool IsStoreAligned4Band(const void* p, int iStride)
    { return ((uintptr_t(p) | iStride) & 15) == 0; }
    static bool IsLoadAligned1Band(const void* p, int iStride)
    { return ((uintptr_t(p) | iStride) & 15) == 0; }
    static bool IsStoreAligned1Band(const void* p, int iStride)
    { return ((uintptr_t(p) | iStride) & 15) == 0; }
};

#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)
inline __m128 LoadFourFloatsAligned(const float *pSrc)
{
    return _mm_load_ps(pSrc);
}

inline __m128 LoadFourFloatsUnaligned(const float *pSrc)
{
    return _mm_loadu_ps(pSrc);
}

inline void StoreAlignedFourBands(__m128 &mm_acc0, float *pDst)
{
    _mm_store_ps(pDst, mm_acc0);
}

inline void StoreUnalignedFourBands(__m128 &mm_acc0, float *pDst)
{
    _mm_storeu_ps(pDst, mm_acc0);
}

inline void TransposeStoreAlignedFourBands(
    __m128 &mm_acc0, __m128 &mm_acc1, __m128 &mm_acc2, __m128 &mm_acc3, float *pDst)
{
    _mm_store_ps(pDst, mm_acc0);
    _mm_store_ps(pDst + 4, mm_acc1);
    _mm_store_ps(pDst + 8, mm_acc2);
    _mm_store_ps(pDst + 12, mm_acc3);
}

inline void TransposeStoreUnalignedFourBands(
    __m128 &mm_acc0, __m128 &mm_acc1, __m128 &mm_acc2, __m128 &mm_acc3, float *pDst)
{
    _mm_storeu_ps(pDst, mm_acc0);
    _mm_storeu_ps(pDst + 4, mm_acc1);
    _mm_storeu_ps(pDst + 8, mm_acc2);
    _mm_storeu_ps(pDst + 12, mm_acc3);
}

inline void TransposeStoreAlignedOneBand(
    __m128 &mm_acc0, __m128 &mm_acc1, __m128 &mm_acc2, __m128 &mm_acc3, 
    float *pDst, int iDstStrideBytes)
{
    _MM_TRANSPOSE4_PS(mm_acc0, mm_acc1, mm_acc2, mm_acc3);
    _mm_store_ps(pDst, mm_acc0);
    _mm_store_ps(PointerOffset(pDst, iDstStrideBytes), mm_acc1);
    _mm_store_ps(PointerOffset(pDst, 2*iDstStrideBytes), mm_acc2);
    _mm_store_ps(PointerOffset(pDst, 3*iDstStrideBytes), mm_acc3);
}

inline void TransposeStoreUnalignedOneBand(
    __m128 &mm_acc0, __m128 &mm_acc1, __m128 &mm_acc2, __m128 &mm_acc3, 
    float *pDst, int iDstStrideBytes)
{
    _MM_TRANSPOSE4_PS(mm_acc0, mm_acc1, mm_acc2, mm_acc3);
    _mm_storeu_ps(pDst, mm_acc0);
    _mm_storeu_ps(PointerOffset(pDst, iDstStrideBytes), mm_acc1);
    _mm_storeu_ps(PointerOffset(pDst, 2*iDstStrideBytes), mm_acc2);
    _mm_storeu_ps(PointerOffset(pDst, 3*iDstStrideBytes), mm_acc3);
}

//+-----------------------------------------------------------------------------
//
// SSE implementation of four banded convolution 
//
//------------------------------------------------------------------------------
template <typename TDST, typename TSRC, __m128 (*pfnLoad)(const TSRC*),
          void   (*pfnStore)(__m128 &mm_acc0, __m128 &mm_acc1, 
                             __m128 &mm_acc2, __m128 &mm_acc3, TDST *pDst)>
void ConvolveVerticalSingleKernelFourBandsTranspose_ProcSpecific(
    TDST* pDst0, const TSRC* pSrc0, 
    int iDstStrideBytes, int iSrcStrideBytes,
    int iProcWidth, int iProcHeight, const C1dKernel& k)
{
    // descend a cacheline wide column of the source filtering 4 rows at a time
    for( int y = 0; y < iProcHeight-3;
         y+=4, pDst0+=16, pSrc0=PointerOffset(pSrc0, 4*iSrcStrideBytes) )
    {
        const TSRC* pSrc = pSrc0;
        TDST* pDst       = pDst0;

        // filter individual pixel columns within those 4 rows
        for( int x = 0; x < iProcWidth; 
             x++, pSrc+=4, pDst=PointerOffset(pDst, iDstStrideBytes) )
        {
            const TSRC* pSrcC = pSrc;
            const float* pK   = k.Ptr();

            // load 4 sources and mul k0
            __m128 mm_kval = _mm_load1_ps(pK);
            __m128 mm_0    = pfnLoad(pSrcC);
            __m128 mm_acc0 = _mm_mul_ps(mm_0, mm_kval);
            pSrcC = PointerOffset(pSrcC, iSrcStrideBytes);
            __m128 mm_1    = pfnLoad(pSrcC);
            __m128 mm_acc1 = _mm_mul_ps(mm_1, mm_kval);
            pSrcC = PointerOffset(pSrcC, iSrcStrideBytes);
            __m128 mm_2    = pfnLoad(pSrcC);
            __m128 mm_acc2 = _mm_mul_ps(mm_2, mm_kval);
            pSrcC = PointerOffset(pSrcC, iSrcStrideBytes);
            __m128 mm_3    = pfnLoad(pSrcC);
            __m128 mm_acc3 = _mm_mul_ps(mm_3, mm_kval);

            // run the rest of the filter taps
            ++pK;
            for( int i = 1; i < k.Width(); ++i, ++pK )
            {
                pSrcC = PointerOffset(pSrcC, iSrcStrideBytes);
                mm_kval = _mm_load1_ps(pK);   // load kernel

                mm_0 = mm_1;
                __m128 mm_prod0 = _mm_mul_ps(mm_0, mm_kval);
                mm_acc0 = _mm_add_ps(mm_prod0, mm_acc0);
                mm_1 = mm_2;
                __m128 mm_prod1 = _mm_mul_ps(mm_1, mm_kval);
                mm_acc1 = _mm_add_ps(mm_prod1, mm_acc1);
                mm_2 = mm_3;
                __m128 mm_prod2 = _mm_mul_ps(mm_2, mm_kval);
                mm_acc2 = _mm_add_ps(mm_prod2, mm_acc2);
                mm_3 = pfnLoad(pSrcC);
                __m128 mm_prod3 = _mm_mul_ps(mm_3, mm_kval);
                mm_acc3 = _mm_add_ps(mm_prod3, mm_acc3);
            }

            // store 4 results
            pfnStore(mm_acc0, mm_acc1, mm_acc2, mm_acc3, pDst);
        }
    }
}

template <typename TDST, typename TSRC>
int ConvolveVerticalSingleKernelFourBandsTranspose_ProcSpecific(
    TDST* pDst0, const TSRC* pSrc0, 
    int iDstStrideBytes, int iSrcStrideBytes,
    int iProcWidth, int iProcHeight, const C1dKernel& k)
{

    if( !g_SupportSSE2() )
    {
        return 0;
    }

    int iCompleted = iProcHeight & ~3;
    if( iCompleted )
    {
        // determine what alignment to use
        bool bSSEAlignedLoad  = 
            FilterAlignTraits<TSRC>::IsLoadAligned4Band(pSrc0, iSrcStrideBytes);
        bool bSSEAlignedStore = 
            FilterAlignTraits<TDST>::IsStoreAligned4Band(pDst0, iDstStrideBytes);
        if( bSSEAlignedLoad && bSSEAlignedStore )
        {
            ConvolveVerticalSingleKernelFourBandsTranspose_ProcSpecific<TDST, 
                TSRC, LoadFourFloatsAligned, TransposeStoreAlignedFourBands>(
                    pDst0, pSrc0, iDstStrideBytes, iSrcStrideBytes,
                    iProcWidth, iProcHeight, k);
        }
        else if( bSSEAlignedLoad )
        {
            ConvolveVerticalSingleKernelFourBandsTranspose_ProcSpecific<TDST, 
                TSRC, LoadFourFloatsAligned, TransposeStoreUnalignedFourBands>(
                    pDst0, pSrc0, iDstStrideBytes, iSrcStrideBytes,
                    iProcWidth, iProcHeight, k);
        }
        else
        {
            ConvolveVerticalSingleKernelFourBandsTranspose_ProcSpecific<TDST, 
                TSRC, LoadFourFloatsUnaligned, TransposeStoreUnalignedFourBands>(
                    pDst0, pSrc0, iDstStrideBytes, iSrcStrideBytes,
                    iProcWidth, iProcHeight, k);
        }
    }

    // return number of finished rows
    return iCompleted;
}

//+-----------------------------------------------------------------------------
//
// SSE implementation of one banded convolution 
//
//------------------------------------------------------------------------------
template <typename TDST, typename TSRC, __m128 (*pfnLoad)(const TSRC*),
          void   (*pfnStore)(__m128 &mm_acc0, __m128 &mm_acc1, 
                             __m128 &mm_acc2, __m128 &mm_acc3, TDST*, int)>
void ConvolveVerticalSingleKernelOneBandTranspose_ProcSpecific(
    TDST* pDst0, const TSRC* pSrc0, 
    int iDstStrideBytes, int iSrcStrideBytes,
    int iProcWidth, int iProcHeight, const C1dKernel& k)
{
    // descend a cacheline wide column of the source filtering 4 rows at a time
    for( int y = 0; y < iProcHeight-3;
         y+=4, pDst0+=4, pSrc0=PointerOffset(pSrc0, 4*iSrcStrideBytes) )
    {
        const TSRC* pSrc = pSrc0;
        TDST* pDst       = pDst0;

        // filter individual pixel columns within those 4 rows
        int x = 0;
        for( ; x < iProcWidth-3; 
             x+=4, pSrc+=4, pDst=PointerOffset(pDst, 4*iDstStrideBytes) )
        {
            const TSRC* pSrcC = pSrc;
            const float* pK   = k.Ptr();

            // load 4 sources and mul k0
            __m128 mm_kval = _mm_load1_ps(pK);
            __m128 mm_0    = pfnLoad(pSrcC);
            __m128 mm_acc0 = _mm_mul_ps(mm_0, mm_kval);
            pSrcC = PointerOffset(pSrcC, iSrcStrideBytes);
            __m128 mm_1    = pfnLoad(pSrcC);
            __m128 mm_acc1 = _mm_mul_ps(mm_1, mm_kval);
            pSrcC = PointerOffset(pSrcC, iSrcStrideBytes);
            __m128 mm_2    = pfnLoad(pSrcC);
            __m128 mm_acc2 = _mm_mul_ps(mm_2, mm_kval);
            pSrcC = PointerOffset(pSrcC, iSrcStrideBytes);
            __m128 mm_3    = pfnLoad(pSrcC);
            __m128 mm_acc3 = _mm_mul_ps(mm_3, mm_kval);

            // run the rest of the filter taps
            ++pK;
            for( int i = 1; i < k.Width(); ++i, ++pK )
            {
                pSrcC = PointerOffset(pSrcC, iSrcStrideBytes);
                mm_kval = _mm_load1_ps(pK);   // load kernel

                mm_0 = mm_1;
                __m128 mm_prod0 = _mm_mul_ps(mm_0, mm_kval);
                mm_acc0 = _mm_add_ps(mm_prod0, mm_acc0);
                mm_1 = mm_2;
                __m128 mm_prod1 = _mm_mul_ps(mm_1, mm_kval);
                mm_acc1 = _mm_add_ps(mm_prod1, mm_acc1);
                mm_2 = mm_3;
                __m128 mm_prod2 = _mm_mul_ps(mm_2, mm_kval);
                mm_acc2 = _mm_add_ps(mm_prod2, mm_acc2);
                mm_3 = pfnLoad(pSrcC);
                __m128 mm_prod3 = _mm_mul_ps(mm_3, mm_kval);
                mm_acc3 = _mm_add_ps(mm_prod3, mm_acc3);
            }

            // store 4 results
            pfnStore(mm_acc0, mm_acc1, mm_acc2, mm_acc3, pDst, iDstStrideBytes);
        }

        // finish this row with C code
        for( ; x < iProcWidth; 
             x++, pSrc++, pDst=PointerOffset(pDst, iDstStrideBytes) )
        {
            TDST* pDstC       = pDst;
            const TSRC* pSrcC = pSrc;
            for( int yy = 0; yy < 4; 
                 yy++, pDstC++, pSrcC = PointerOffset(pSrcC, iSrcStrideBytes) )
            {
                const TSRC* pSrcR = pSrcC;
                float v0 = k[0] * float(pSrcR[0]);
                for( int i = 1; i < k.Width(); i++ )
                {
                    pSrcR = PointerOffset(pSrcR, iSrcStrideBytes);
                    v0 += k[i] * float(pSrcR[0]);
                }
                VtClip(pDstC, v0);
            }
        }
    }
}

template <typename TDST, typename TSRC>
int ConvolveVerticalSingleKernelOneBandTranspose_ProcSpecific(
    TDST* pDst0, const TSRC* pSrc0, 
    int iDstStrideBytes, int iSrcStrideBytes,
    int iProcWidth, int iProcHeight, const C1dKernel& k)
{
    if( !g_SupportSSE2() )
    {
        return 0;
    }

    int iCompleted = iProcHeight & ~3;
    if( iCompleted )
    {
        // determine what alignment to use
        bool bSSEAlignedLoad  = 
            FilterAlignTraits<TSRC>::IsLoadAligned1Band(pSrc0, iSrcStrideBytes);
        bool bSSEAlignedStore = 
            FilterAlignTraits<TDST>::IsStoreAligned1Band(pDst0, iDstStrideBytes);
        if( bSSEAlignedLoad && bSSEAlignedStore )
        {
            ConvolveVerticalSingleKernelOneBandTranspose_ProcSpecific<TDST, 
                TSRC, LoadFourFloatsAligned, TransposeStoreAlignedOneBand>(
                    pDst0, pSrc0, iDstStrideBytes, iSrcStrideBytes,
                    iProcWidth, iProcHeight, k);
        }
        else if( bSSEAlignedLoad )
        {
            ConvolveVerticalSingleKernelOneBandTranspose_ProcSpecific<TDST, 
                TSRC, LoadFourFloatsAligned, TransposeStoreUnalignedOneBand>(
                    pDst0, pSrc0, iDstStrideBytes, iSrcStrideBytes,
                    iProcWidth, iProcHeight, k);
        }
        else
        {
            ConvolveVerticalSingleKernelOneBandTranspose_ProcSpecific<TDST, 
                TSRC, LoadFourFloatsUnaligned, TransposeStoreUnalignedOneBand>(
                    pDst0, pSrc0, iDstStrideBytes, iSrcStrideBytes,
                    iProcWidth, iProcHeight, k);
        }
    }

    // return number of finished rows
    return iCompleted;
}
#endif

//+-----------------------------------------------------------------------------
//
// SSE implementation of N banded convolution 
//
//------------------------------------------------------------------------------
template<typename TDST>
TDST* ConvertToDestFormatIfNecessary(float* pSrc, TDST* pConvBuf, int iElCnt)
{
    VtConvertSpan(pConvBuf, pSrc, iElCnt); 
    return pConvBuf;
}
template<>
float* ConvertToDestFormatIfNecessary(float* pSrc, float*, int)
{ 
    return pSrc; 
}

#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)
template <typename TDST, typename TSRC, __m128 (*pfnLoad)(const TSRC*)>
void ConvolveVerticalSingleKernelNBandsTranspose_ProcSpecific(
    TDST* pDst0, const TSRC* pSrc0, int b0, int iSrcB,
    int iDstStrideBytes, int iSrcStrideBytes,
    int iProcWidth, int iProcHeight, const C1dKernel& k)
{
    VT_ASSERT( iProcWidth <= 128 );

    // create temp buffers for temp and dest types
    // four rows at iProcWidth the +64 is to guarantee that the buffer can be
    // 64 byte aligned
    CTypedBuffer1<float, 4*128*4+64> bufTmp(1);
    CTypedBuffer1<TDST,  4*128*4+64> bufDst(1);

    // descend a cacheline wide column of the source filtering 4 rows at a time
    for( int y = 0; y < iProcHeight-3;
         y+=4, pDst0+=4*iSrcB, pSrc0=PointerOffset(pSrc0, 4*iSrcStrideBytes) )
    {
        const TSRC* pSrc = pSrc0;
        float *pTmp = bufTmp.Buffer1();

        // filter 4 horizontal elements at a time 
        int x = 0;
        for( ; x < iProcWidth-3; x+=4, pSrc+=4, pTmp+=4 )
        {
            const TSRC* pSrcC = pSrc;
            const float* pK   = k.Ptr();

            // load 4 sources and mul k0
            __m128 mm_kval = _mm_load1_ps(pK);
            __m128 mm_0    = pfnLoad(pSrcC);
            __m128 mm_acc0 = _mm_mul_ps(mm_0, mm_kval);
            pSrcC = PointerOffset(pSrcC, iSrcStrideBytes);
            __m128 mm_1    = pfnLoad(pSrcC);
            __m128 mm_acc1 = _mm_mul_ps(mm_1, mm_kval);
            pSrcC = PointerOffset(pSrcC, iSrcStrideBytes);
            __m128 mm_2    = pfnLoad(pSrcC);
            __m128 mm_acc2 = _mm_mul_ps(mm_2, mm_kval);
            pSrcC = PointerOffset(pSrcC, iSrcStrideBytes);
            __m128 mm_3    = pfnLoad(pSrcC);
            __m128 mm_acc3 = _mm_mul_ps(mm_3, mm_kval);

            // run the rest of the filter taps
            ++pK;
            for( int i = 1; i < k.Width(); ++i, ++pK )
            {
                pSrcC = PointerOffset(pSrcC, iSrcStrideBytes);
                mm_kval = _mm_load1_ps(pK);   // load kernel

                mm_0 = mm_1;
                __m128 mm_prod0 = _mm_mul_ps(mm_0, mm_kval);
                mm_acc0 = _mm_add_ps(mm_prod0, mm_acc0);
                mm_1 = mm_2;
                __m128 mm_prod1 = _mm_mul_ps(mm_1, mm_kval);
                mm_acc1 = _mm_add_ps(mm_prod1, mm_acc1);
                mm_2 = mm_3;
                __m128 mm_prod2 = _mm_mul_ps(mm_2, mm_kval);
                mm_acc2 = _mm_add_ps(mm_prod2, mm_acc2);
                mm_3 = pfnLoad(pSrcC);
                __m128 mm_prod3 = _mm_mul_ps(mm_3, mm_kval);
                mm_acc3 = _mm_add_ps(mm_prod3, mm_acc3);
            }

            // store to buffer 4 scanlines at a time
            _mm_store_ps(pTmp,               mm_acc0);
            _mm_storeu_ps(pTmp+1*iProcWidth, mm_acc1);
            _mm_storeu_ps(pTmp+2*iProcWidth, mm_acc2);
            _mm_storeu_ps(pTmp+3*iProcWidth, mm_acc3);
        }

        // finish off undone in this iProcWidth
        for( ; x < iProcWidth; x++, pSrc++, pTmp++ )
        {
            float* pTmpC = pTmp;
            const TSRC* pSrcC = pSrc;
            for( int yy = 0; yy < 4; yy++, pTmpC += iProcWidth, 
                 pSrcC = PointerOffset(pSrcC, iSrcStrideBytes) )
            {   
                const TSRC* pSrcR = pSrcC;
                // filter     
                float v0 = k[0] * float(pSrcR[0]);
                for( int i = 1; i < k.Width(); i++ )
                {
                    pSrcR = PointerOffset(pSrcR, iSrcStrideBytes);
                    v0 += k[i] * float(pSrcR[0]);
                }
                *pTmpC = v0;
            }
        }

        // convert to dst element type if necessary
        const TDST* pConv = ConvertToDestFormatIfNecessary(
            bufTmp.Buffer1(), bufDst.Buffer1(), 4*iProcWidth);

        // transpose (note that the bands DONT get transposed)
        TDST* pDst = pDst0;
        for( int yy = 0; yy < 4; yy++, pDst+=iSrcB, pConv+=iProcWidth )
        {
            TDST* pDstR = pDst;
            x = 0;
            int b = b0;
            if( iSrcB == 3 )
            {           
                for( ; b < iSrcB && x < iProcWidth; b++, x++ )
                {
                    pDstR[b] = pConv[x];
                }
                b = 0;
                pDstR = PointerOffset(pDstR, iDstStrideBytes);
                for( ; x < iProcWidth-2; x+=3 )
                {
                    pDstR[0] = pConv[x];
                    pDstR[1] = pConv[x+1];
                    pDstR[2] = pConv[x+2];
                    pDstR = PointerOffset(pDstR, iDstStrideBytes);
                }
            }
            for( ; x < iProcWidth; x++ )
            {
                pDstR[b] = pConv[x];
                if( ++b == iSrcB )
                {
                    b = 0;
                    pDstR = PointerOffset(pDstR, iDstStrideBytes);
                }        
            }
        }
    }
}

template <typename TDST, typename TSRC>
int ConvolveVerticalSingleKernelNBandsTranspose_ProcSpecific(
    TDST* pDst0, const TSRC* pSrc0, int b0, int iSrcB,
    int iDstStrideBytes, int iSrcStrideBytes,
    int iProcWidth, int iProcHeight, const C1dKernel& k)
{

    if( !g_SupportSSE2() )
    {
        return 0;
    }

    int iCompleted = iProcHeight & ~3;
    if( iCompleted )
    {
        // determine what alignment to use
        bool bSSEAlignedLoad  = 
            FilterAlignTraits<TSRC>::IsLoadAligned4Band(pSrc0, iSrcStrideBytes);
        if( bSSEAlignedLoad )
        {
            ConvolveVerticalSingleKernelNBandsTranspose_ProcSpecific<TDST, TSRC, 
                LoadFourFloatsAligned>(pDst0, pSrc0, b0, iSrcB,
                                       iDstStrideBytes, iSrcStrideBytes,
                                       iProcWidth, iProcHeight, k);
        }
        else
        {
            ConvolveVerticalSingleKernelNBandsTranspose_ProcSpecific<TDST, TSRC, 
                LoadFourFloatsUnaligned>(pDst0, pSrc0, b0, iSrcB,
                                         iDstStrideBytes, iSrcStrideBytes,
                                         iProcWidth, iProcHeight, k);
        }
    }

    // return number of finished rows
    return iCompleted;
}
#endif

//+-----------------------------------------------------------------------------
//
// C++ implementation of four banded convolution 
//
//------------------------------------------------------------------------------
void ScaleKernel(C1dKernel& kd, const C1dKernel& ks, float fScale)
{
    for( int i = 0; i < kd.Width(); i++ )
    {
        kd[i] = ks[i]*fScale;
    }
}

template<typename TDST, typename TSRC>
void ScaleKernel(C1dKernel& kd, const C1dKernel& ks)
{
    ScaleKernel(
        kd, ks, (ElTraits<TDST>::MaxVal()) / float(ElTraits<TSRC>::MaxVal()) );
}

void ScaleKernelSet(C1dKernelSet& ksd, const C1dKernelSet& kss, float fScale)
{
    for( UInt32 i = 0; i < ksd.GetCycle(); i++ )
    {
        ScaleKernel(ksd.GetKernel(i), kss.GetKernel(i), fScale);
    }
}

template <typename TDST, typename TSRC>
void ConvolveVerticalSingleKernelFourBandsTranspose(
    CTypedImg<TDST>& imgDst, const CTypedImg<TSRC>& imgSrc, C1dKernel& k,
    int iSrcOffset)
{
    const int c_iSrcB = 4;
    VT_ASSERT( imgSrc.Bands()==c_iSrcB && imgDst.Bands()==c_iSrcB );

    ScaleKernel<TDST, TSRC>(k, k);

    // process columns of one cache line width (64 bytes) at a time
    int iProcWidthMain = 64 / (c_iSrcB*sizeof(TSRC));
    int iProcWidth;
    for( int xx = 0; xx < imgDst.Height(); xx+=iProcWidth)
    {
        TDST* pDst0       = imgDst.Ptr(0, xx);
        const TSRC* pSrc0 = imgSrc.Ptr(xx, iSrcOffset-k.Center());

        // if the source is not aligned to 64byte cache lines process extra
        // in this column such that the next column is aligned
        iProcWidth   = iProcWidthMain;
        int iOffset0 = uintptr_t(pSrc0) & 0x3f;
        if( iOffset0 )
        {
            iProcWidth += (64 - iOffset0) / (c_iSrcB*sizeof(TSRC));
        }
        // process up to the end if last bit isn't a full cache line
        int iToEnd = imgDst.Height()-xx;
        if( iToEnd < iProcWidth + iProcWidthMain )
        {
            iProcWidth = iToEnd;
        }

#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)
        // returns how many rows were processed in this column
        int y = ConvolveVerticalSingleKernelFourBandsTranspose_ProcSpecific(
            pDst0, pSrc0, imgDst.StrideBytes(), imgSrc.StrideBytes(),
            iProcWidth, imgDst.Width(), k);
#else
		int y = 0;
#endif

        // adjust pDst0, pSrc0 for any processing that _ProcSpecific did
        pDst0 += y*4;
        pSrc0 = imgSrc.OffsetRow(pSrc0, y);

        // process any remaining rows using C
        for( ; y < imgDst.Width(); 
             y++, pDst0+=4, pSrc0=imgSrc.NextRow(pSrc0) )
        {        
            const TSRC* pSrc = pSrc0;
            TDST* pDst       = pDst0;

            for( int x = 0; x < iProcWidth; 
                 x++, pSrc+=4, pDst=imgDst.NextRow(pDst) )
            {
                const TSRC* pSrcC = pSrc;
                float fK = k[0];
                float v0 = fK * float(pSrcC[0]);
                float v1 = fK * float(pSrcC[1]);
                float v2 = fK * float(pSrcC[2]);
                float v3 = fK * float(pSrcC[3]);
                for( int i = 1; i < k.Width(); i++ )
                {
                    pSrcC = imgSrc.NextRow(pSrcC);
                    fK = k[i];
                    v0 += fK * float(pSrcC[0]);
                    v1 += fK * float(pSrcC[1]);
                    v2 += fK * float(pSrcC[2]);
                    v3 += fK * float(pSrcC[3]);
                }
                VtClip(pDst+0, v0);
                VtClip(pDst+1, v1);
                VtClip(pDst+2, v2);
                VtClip(pDst+3, v3);
            }
        }
    }
}

//+-----------------------------------------------------------------------------
//
// C++ implementation of one banded convolution 
//
//------------------------------------------------------------------------------
template <typename TDST, typename TSRC>
void ConvolveVerticalSingleKernelOneBandTranspose(
    CTypedImg<TDST>& imgDst, const CTypedImg<TSRC>& imgSrc, C1dKernel& k,
    int iSrcOffset)
{
    const int c_iSrcB = 1;
    VT_ASSERT( imgSrc.Bands()==c_iSrcB && imgDst.Bands()==c_iSrcB );

    ScaleKernel<TDST, TSRC>(k, k);

    // process columns of one cache line width (64 bytes) at a time
    int iProcWidthMain = 64 / (c_iSrcB*sizeof(TSRC));
    int iProcWidth;
    for( int xx = 0; xx < imgDst.Height(); xx+=iProcWidth)
    {
        TDST* pDst0       = imgDst.Ptr(0, xx);
        const TSRC* pSrc0 = imgSrc.Ptr(xx, iSrcOffset-k.Center());

        // if the source is not aligned to 64byte cache lines process extra
        // in this column such that the next column is aligned
        iProcWidth   = iProcWidthMain;
        int iOffset0 = uintptr_t(pSrc0) & 0x3f;
        if( iOffset0 )
        {
            iProcWidth += (64 - iOffset0) / (c_iSrcB*sizeof(TSRC));
        }
        // process up to the end if last bit isn't a full cache line
        int iToEnd = imgDst.Height()-xx;
        if( iToEnd < iProcWidth + iProcWidthMain )
        {
            iProcWidth = iToEnd;
        }

#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)
        // returns how many rows were processed in this column
        int y = ConvolveVerticalSingleKernelOneBandTranspose_ProcSpecific(
            pDst0, pSrc0, imgDst.StrideBytes(), imgSrc.StrideBytes(),
            iProcWidth, imgDst.Width(), k);
#else
		int y = 0;
#endif

        // adjust pDst0, pSrc0 for any processing that _ProcSpecific did
        pDst0 += y;
        pSrc0 = imgSrc.OffsetRow(pSrc0, y);

        // process any remaining rows using C
        for( ; y < imgDst.Width(); y++, pDst0++, pSrc0=imgSrc.NextRow(pSrc0) )
        {        
            const TSRC* pSrc = pSrc0;
            TDST* pDst       = pDst0;

            for( int x = 0; x < iProcWidth; 
                 x++, pSrc++, pDst=imgDst.NextRow(pDst) )
            {
                const TSRC* pSrcC = pSrc;
                float v0 = k[0] * float(pSrcC[0]);
                for( int i = 1; i < k.Width(); i++ )
                {
                    pSrcC = imgSrc.NextRow(pSrcC);
                    v0 += k[i] * float(pSrcC[0]);
                }
                VtClip(pDst, v0);
            }
        }
    }
}

//+-----------------------------------------------------------------------------
//
// C++ implementation of arbitrary banded convolution 
//
//------------------------------------------------------------------------------
template <typename TDST, typename TSRC>
void ConvolveVerticalSingleKernelNBandsTranspose(
    CTypedImg<TDST>& imgDst, const CTypedImg<TSRC>& imgSrc, C1dKernel& k,
    int iSrcOffset)
{
    int iSrcB = imgSrc.Bands();
    VT_ASSERT( imgSrc.Bands()==imgDst.Bands() );

    int dstfrmt = EL_FORMAT(imgDst.GetType());
    if( dstfrmt == EL_FORMAT_FLOAT )
    {
        // only scale the kernel if the dest format float because the
        // ConvertToDestFormatIfNecessary() function is actually doing the
        // conversion in this case
        ScaleKernel<TDST, TSRC>(k, k);
    }

    // create temp buffers for temp and dest types
    CTypedBuffer1<float, 128*4+64> bufTmp(1);
    CTypedBuffer1<TDST,  128*4+64> bufDst(1);

    // process columns of one cache line width (64 bytes) at a time
    int iProcWidthMain = 64 / (sizeof(TSRC));
    int iProcWidth;

    for( int xx = 0; xx < imgDst.Height()*iSrcB; xx+=iProcWidth)
    {
        int yd = xx / iSrcB;
        int b0 = xx - yd*iSrcB;
        TDST* pDst0 = imgDst.Ptr(0, yd);
        const TSRC* pSrc0 = imgSrc.Ptr(iSrcOffset-k.Center()) + xx;

        // if the source is not aligned to 64byte cache lines process extra
        // in this column such that the next column is aligned
        iProcWidth   = iProcWidthMain;
        int iOffset0 = uintptr_t(pSrc0) & 0x3f;
        if( iOffset0 )
        {
            iProcWidth += (64 - iOffset0) / sizeof(TSRC);
        }
        // process up to the end if last bit isn't a full cache line
        int iToEnd = imgDst.Height()*iSrcB-xx;
        if( iToEnd < iProcWidth + iProcWidthMain )
        {
            iProcWidth = iToEnd;
        }

        VT_ASSERT( bufTmp.Capacity() >= iProcWidth );

#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)
        // returns how many rows were processed in this column
        int y = ConvolveVerticalSingleKernelNBandsTranspose_ProcSpecific(
            pDst0, pSrc0, b0, iSrcB, imgDst.StrideBytes(), imgSrc.StrideBytes(),
            iProcWidth, imgDst.Width(), k);
#else
		int y = 0;
#endif

        // adjust pDst0, pSrc0 for any processing that _ProcSpecific did
        pDst0 += y*iSrcB;
        pSrc0 = imgSrc.OffsetRow(pSrc0, y);

        // process any remaining rows using C
        for( ; y < imgDst.Width(); y++, pDst0+=iSrcB, pSrc0=imgSrc.NextRow(pSrc0) )
        {   
            // filter     
            const TSRC* pSrc = pSrc0;
            float* pTmp      = bufTmp.Buffer1();
            for( int x0 = 0; x0 < iProcWidth; x0++, pSrc++, pTmp++ )
            {
                const TSRC* pSrcC = pSrc;
                float v0 = k[0] * float(pSrcC[0]);
                for( int i = 1; i < k.Width(); i++ )
                {
                    pSrcC = imgSrc.NextRow(pSrcC);
                    v0 += k[i] * float(pSrcC[0]);
                }
                *pTmp = v0;
            }

            // convert to dst element type if necessary
            TDST* pConv = ConvertToDestFormatIfNecessary(
                bufTmp.Buffer1(), bufDst.Buffer1(), iProcWidth); 

            // transpose (note that the bands DONT get transposed)
            TDST* pDst = pDst0;

            int x = 0;
            int b = b0;
            if( iSrcB == 3 )
            {           
                for( ; b < iSrcB && x < iProcWidth; b++, x++ )
                {
                    pDst[b] = pConv[x];
                }
                b = 0;
                pDst  = imgDst.NextRow(pDst);
                for( ; x < iProcWidth-2; x+=3 )
                {
                    pDst[0] = pConv[x];
                    pDst[1] = pConv[x+1];
                    pDst[2] = pConv[x+2];
                    pDst  = imgDst.NextRow(pDst);
                }
            }
            for( ; x < iProcWidth; x++ )
            {
                pDst[b] = pConv[x];
                if( ++b == iSrcB )
                {
                    b = 0;
                    pDst = imgDst.NextRow(pDst);
                }        
            }
        }
    }
}

//+-----------------------------------------------------------------------------
//
// top level function to call appropriate code for the provided band-count
//
//------------------------------------------------------------------------------
template<typename TDST, typename TSRC>
void ConvolveVerticalSingleKernelTranspose(
    CTypedImg<TDST>& imgDst, const CTypedImg<TSRC>& imgSrc, C1dKernel& k, 
    int iSrcOffset)
{
    // call band specific code
    if( imgSrc.Bands() == 4 )
    {
        ConvolveVerticalSingleKernelFourBandsTranspose(imgDst, imgSrc, k, 
                                                       iSrcOffset);
    }
    else if( imgSrc.Bands() == 1 )
    {
        ConvolveVerticalSingleKernelOneBandTranspose(imgDst, imgSrc, k, 
                                                     iSrcOffset);
    }
    else
    {
        ConvolveVerticalSingleKernelNBandsTranspose(imgDst, imgSrc, k, 
                                                    iSrcOffset);
    }
}

void
ConvolveVerticalSingleKernelTranspose(
    CImg& imgDst, const CImg& imgSrc, C1dKernel& k, int iSrcOffset,
    int iPass)
{
    // Check that caller set things up correctly.  
    // - intermediate image should always be float format
    // - rows of intermediate should be 64 byte aligned
    // - the band counts should be equal
    // - never deal with half float here
#if !_DEBUG
    UNREFERENCED_PARAMETER(iPass);
#endif
#if _DEBUG
    const CImg* pTst = (iPass == 0)? &imgDst: &imgSrc;
    VT_ASSERT( EL_FORMAT(pTst->GetType()) == EL_FORMAT_FLOAT );
    VT_ASSERT( (uintptr_t(pTst->BytePtr(0))&0x3f) == 0 && 
               (uintptr_t(pTst->BytePtr(1))&0x3f) == 0 );
    VT_ASSERT( imgSrc.Bands()==imgDst.Bands() );
    VT_ASSERT( EL_FORMAT(imgSrc.GetType()) != EL_FORMAT_HALF_FLOAT &&
               EL_FORMAT(imgDst.GetType()) != EL_FORMAT_HALF_FLOAT );
#endif

    // determine which specialization to call
    switch( EL_FORMAT(imgSrc.GetType()) )
    {
    case EL_FORMAT_BYTE:
        ConvolveVerticalSingleKernelTranspose<float, Byte>(
            (CFloatImg&)imgDst, (CByteImg&)imgSrc, k, iSrcOffset);
        break;
    case EL_FORMAT_SHORT:
        ConvolveVerticalSingleKernelTranspose<float, UInt16>(
            (CFloatImg&)imgDst, (CShortImg&)imgSrc, k, iSrcOffset);
        break;
    case EL_FORMAT_FLOAT:
        switch( EL_FORMAT(imgDst.GetType()) )
        {
        case EL_FORMAT_BYTE:
            ConvolveVerticalSingleKernelTranspose(
                (CByteImg&)imgDst, (CFloatImg&)imgSrc, k, iSrcOffset);
            break;
        case EL_FORMAT_SHORT:
            ConvolveVerticalSingleKernelTranspose(
                (CShortImg&)imgDst, (CFloatImg&)imgSrc, k, iSrcOffset);
            break;
        case EL_FORMAT_FLOAT:
            ConvolveVerticalSingleKernelTranspose(
                (CFloatImg&)imgDst, (CFloatImg&)imgSrc, k, iSrcOffset);
            break;
        }
        break;
    }
}

//+---------------------------------------------------------------------------
//
// Function: VtSeparableFilter
// 
//----------------------------------------------------------------------------
HRESULT
vt::VtSeparableFilter(CImg &imgDst, const CRect& rctDst, 
                      const CImg &imgSrc, CPoint ptSrcOrigin,
                      const C1dKernel& kh, const C1dKernel& kv,
                      const IMAGE_EXTEND& ex)
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
    // - always operate in source type except for half-float which should be 
    //   converted to float before passing to filter.
    // - reduce the band-count as early as possible
    int iOpBands = VtMin(imgSrc.Bands(), imgDst.Bands());
    int iOpType   = EL_FORMAT(imgSrc.GetType());
    if( iOpType == EL_FORMAT_HALF_FLOAT )
    {
        iOpType = EL_FORMAT_FLOAT;
    }
    int iConvType = EL_FORMAT(imgDst.GetType());
    if( iConvType == EL_FORMAT_HALF_FLOAT )
    {
        iConvType = EL_FORMAT_FLOAT;
    }

    // determine the rect of the full provided source
    vt::CRect rctSrc = imgSrc.Rect();

    // loop over the destination blocks
    const UInt32 c_blocksize = 128;

    // keep copy of the kernel which is scaled to perform conversions
    C1dKernel ksclh, ksclv;
    VT_HR_EXIT( ksclh.Create(kh) );
    VT_HR_EXIT( ksclv.Create(kv) );

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
        vt::CRect rctSrcBlk = GetRequiredSrcRect(rctDstBlk, kh, kv) - ptSrcOrigin;
        
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

        // create an intermediate float block
        VT_HR_EXIT( imgTmp.Create(imgDstBlk.Height(), imgConv.Width(), iOpBands, 
                                  align64ByteRows) );

        // filter vertically using the vertical kernel and transpose
        memcpy( ksclv.Ptr(), kv.Ptr(), ksclv.Width()*sizeof(float) );
        ConvolveVerticalSingleKernelTranspose(imgTmp, imgConv, ksclv, 
                                              ksclv.Center(), 0);
        
        // filter vertically using the horizontal kernel and transpose
        memcpy( ksclh.Ptr(), kh.Ptr(), ksclh.Width()*sizeof(float) );
        if( iOpBands != imgDst.Bands() || 
            EL_FORMAT(imgDst.GetType())==EL_FORMAT_HALF_FLOAT )
        {
            // sometimes a temp conversion buffer is needed
            VT_HR_EXIT( imgConv.Create(imgDstBlk.Width(), imgDstBlk.Height(),
									   VT_IMG_MAKE_TYPE(iConvType, iOpBands),
                                       align64ByteRows) );
            ConvolveVerticalSingleKernelTranspose(imgConv, imgTmp, ksclh, 
                                                  ksclh.Center(), 1);
            VtConvertImage(imgDstBlk, imgConv);
        }
        else
        {
            ConvolveVerticalSingleKernelTranspose(imgDstBlk, imgTmp, ksclh, 
                                                  ksclh.Center(), 1);
        }
    }

    VT_HR_END()
}

#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)
//+-----------------------------------------------------------------------------
//
// Polyphase support functions ....
// 
//------------------------------------------------------------------------------
template<typename TSRC, __m128 (*pfnLoad)(const TSRC*),
         void   (*pfnStore)(__m128 &mm_acc0, float *pDst)>
void ConvolveVertical_ProcSpecific(
    float* pDst0, const CTypedImg<TSRC>& imgSrc, int iDstStrideBytes, 
    int iLeft, int iProcWidth,int iProcHeight, const C1dKernelSet& ks, 
    UInt32 uKernIndex, int iSrcOffset)
{
    // get kernel info
    UInt32 uCycle      = ks.GetCycle();
    int  iCoordShift = ks.GetCoordShiftPerCycle();

    for( int y=0; y < iProcHeight; y++, pDst0=PointerOffset(pDst0, iDstStrideBytes) )
    {
        // descend the column
        const C1dKernel& k = ks.GetKernel(uKernIndex);
        int iStartY        = iSrcOffset + ks.GetCoord(uKernIndex);
        VT_ASSERT( iStartY >= 0 && iStartY+k.Width() <= imgSrc.Height() );

        // do 16 at once while possible
        const TSRC* pSrc = imgSrc.Ptr(iStartY)+iLeft;
        float* pDst      = pDst0;
        int x = 0;
        for( ; x < iProcWidth-15; x+=16, pSrc+=16, pDst+=16 )
        {
            // run filter on this row acc 16 results
            const TSRC* pSrcR = pSrc;
           
            __m128 mm_kval = _mm_load1_ps(&k[0]);
            __m128 mm_0    = pfnLoad(pSrcR);
            __m128 mm_acc0 = _mm_mul_ps(mm_0, mm_kval);
            __m128 mm_1    = pfnLoad(pSrcR+4);
            __m128 mm_acc1 = _mm_mul_ps(mm_1, mm_kval);
            __m128 mm_2    = pfnLoad(pSrcR+8);
            __m128 mm_acc2 = _mm_mul_ps(mm_2, mm_kval);
            __m128 mm_3    = pfnLoad(pSrcR+12);
            __m128 mm_acc3 = _mm_mul_ps(mm_3, mm_kval);
            for( int i = 1; i < k.Width(); i++ )
            {
                pSrcR=imgSrc.NextRow(pSrcR);

                mm_kval = _mm_load1_ps(&k[i]);
                mm_0    = pfnLoad(pSrcR);
                mm_0    = _mm_mul_ps(mm_0, mm_kval);
                mm_acc0 = _mm_add_ps(mm_acc0, mm_0);
                mm_1    = pfnLoad(pSrcR+4);
                mm_1    = _mm_mul_ps(mm_1, mm_kval);
                mm_acc1 = _mm_add_ps(mm_acc1, mm_1);
                mm_2    = pfnLoad(pSrcR+8);
                mm_2    = _mm_mul_ps(mm_2, mm_kval);
                mm_acc2 = _mm_add_ps(mm_acc2, mm_2);
                mm_3    = pfnLoad(pSrcR+12);
                mm_3    = _mm_mul_ps(mm_3, mm_kval);
                mm_acc3 = _mm_add_ps(mm_acc3, mm_3);
            }
            pfnStore(mm_acc0, pDst);
            pfnStore(mm_acc1, pDst+4);
            pfnStore(mm_acc2, pDst+8);
            pfnStore(mm_acc3, pDst+12);
        }

        // do 4 at a time while possible
        for( ; x < iProcWidth-3; x+=4, pSrc+=4, pDst+=4 )
        {
            // run filter on this row acc 16 results
            const TSRC* pSrcR = pSrc;
           
            __m128 mm_kval = _mm_load1_ps(&k[0]);
            __m128 mm_0    = pfnLoad(pSrcR);
            __m128 mm_acc0 = _mm_mul_ps(mm_0, mm_kval);
            for( int i = 1; i < k.Width(); i++ )
            {
                pSrcR=imgSrc.NextRow(pSrcR);

                mm_kval = _mm_load1_ps(&k[i]);
                mm_0    = pfnLoad(pSrcR);
                mm_0    = _mm_mul_ps(mm_0, mm_kval);
                mm_acc0 = _mm_add_ps(mm_acc0, mm_0);
            }
            pfnStore(mm_acc0, pDst);
        }

        // next kernel
        VT_ASSERT( uKernIndex < uCycle );
        if( ++uKernIndex == uCycle )
        {
            uKernIndex  = 0;
            iSrcOffset += iCoordShift;
        }
    }
}

template<typename TSRC>
int ConvolveVertical_ProcSpecific(
    float* pDst0, const CTypedImg<TSRC>& imgSrc, int iDstStrideBytes, 
    int iLeft, int iProcWidth,int iProcHeight, const C1dKernelSet& ks, 
    UInt32 uKernIndex, int iSrcOffset)
{
    if( !g_SupportSSE2() )
    {
        return 0;
    }

    int iCompleted = iProcWidth & ~3;
    if( iCompleted )
    {
        // determine what alignment to use
        const TSRC* pSrc0 = imgSrc.Ptr()+iLeft;
    
        bool bSSEAlignedLoad  = 
            FilterAlignTraits<TSRC>::IsLoadAligned4Band(pSrc0, imgSrc.StrideBytes());
        bool bSSEAlignedStore = 
            FilterAlignTraits<float>::IsStoreAligned4Band(pDst0, iDstStrideBytes);
        if( bSSEAlignedLoad && bSSEAlignedStore )
        {
            ConvolveVertical_ProcSpecific<TSRC, LoadFourFloatsAligned, StoreAlignedFourBands>(
                pDst0, imgSrc, iDstStrideBytes, iLeft, iProcWidth, iProcHeight, ks,
                uKernIndex, iSrcOffset);
        }
        else if( bSSEAlignedLoad )
        {
            ConvolveVertical_ProcSpecific<TSRC, LoadFourFloatsAligned, StoreUnalignedFourBands>(
                pDst0, imgSrc, iDstStrideBytes, iLeft, iProcWidth, iProcHeight, ks,
                uKernIndex, iSrcOffset);
        }
        else
        {
            ConvolveVertical_ProcSpecific<TSRC, LoadFourFloatsUnaligned, StoreUnalignedFourBands>(
                pDst0, imgSrc, iDstStrideBytes, iLeft, iProcWidth, iProcHeight, ks,
                uKernIndex, iSrcOffset);
        }
    }

    // return any right border that wasn't done
    return iCompleted;
}

template <typename TDST, typename TSRC, __m128 (*pfnLoad)(const TSRC*),
          void   (*pfnStore)(__m128 &mm_acc0, __m128 &mm_acc1, 
                             __m128 &mm_acc2, __m128 &mm_acc3, TDST*, int)>
void ConvolveVerticalOneBandTranspose_ProcSpecific(
    TDST* pDst0, const CTypedImg<TSRC>& imgSrc, int iDstStrideBytes, 
    int iLeft, int iProcWidth,int iProcHeight, const C1dKernelSet& ks, 
    UInt32 uKernIndex, int iSrcOffset)
{
    // get kernel info
    UInt32 uCycle    = ks.GetCycle();
    int  iCoordShift = ks.GetCoordShiftPerCycle();

    VT_ASSERT( uKernIndex < uCycle );

    int y = 0;

    if( uCycle == 1 )
    {
        // special case single kernel
        const C1dKernel& k0 = ks.GetKernel(0);
        const TSRC* pSrcR0  = imgSrc.Ptr(iSrcOffset + ks.GetCoord(0))+iLeft;     

        for( ; y < iProcHeight-3; y+=4, pDst0+=4, 
             pSrcR0=imgSrc.OffsetRow(pSrcR0, 4*iCoordShift) )
        {
            // do 4 wide at a time while possible
            TDST* pDst = pDst0;
            for( int x = 0; x < iProcWidth-3; 
                 x+=4, pDst=PointerOffset(pDst, 4*iDstStrideBytes) )
            {
                // unroll 4 filters
                const TSRC* pSrc1 = pSrcR0 + x;
                const TSRC* pSrc2 = imgSrc.OffsetRow(pSrc1, iCoordShift);
                const TSRC* pSrc3 = imgSrc.OffsetRow(pSrc2, iCoordShift);
                const TSRC* pSrc4 = imgSrc.OffsetRow(pSrc3, iCoordShift);
                __m128 mm_kval = _mm_load1_ps(&k0[0]);
                __m128 mm_acc1 = _mm_mul_ps(pfnLoad(pSrc1), mm_kval);
                __m128 mm_acc2 = _mm_mul_ps(pfnLoad(pSrc2), mm_kval);
                __m128 mm_acc3 = _mm_mul_ps(pfnLoad(pSrc3), mm_kval);
                __m128 mm_acc4 = _mm_mul_ps(pfnLoad(pSrc4), mm_kval);
                for( int i = 1; i < k0.Width(); i++ )
                {
                    pSrc1=imgSrc.NextRow(pSrc1);
                    pSrc2=imgSrc.NextRow(pSrc2);
                    pSrc3=imgSrc.NextRow(pSrc3);
                    pSrc4=imgSrc.NextRow(pSrc4);
    
                    mm_kval = _mm_load1_ps(&k0[i]);
                    __m128 mm_0    = _mm_mul_ps(pfnLoad(pSrc1), mm_kval);
                    mm_acc1 = _mm_add_ps(mm_acc1, mm_0);
                    mm_0    = _mm_mul_ps(pfnLoad(pSrc2), mm_kval);
                    mm_acc2 = _mm_add_ps(mm_acc2, mm_0);
                    mm_0    = _mm_mul_ps(pfnLoad(pSrc3), mm_kval);
                    mm_acc3 = _mm_add_ps(mm_acc3, mm_0);
                    mm_0    = _mm_mul_ps(pfnLoad(pSrc4), mm_kval);
                    mm_acc4 = _mm_add_ps(mm_acc4, mm_0);
                }
    
                pfnStore(mm_acc1, mm_acc2, mm_acc3, mm_acc4, pDst, iDstStrideBytes);
            }
        }

        iSrcOffset += y*iCoordShift;
    }
    else
    {
        for( ; y < iProcHeight-3; y+=4, pDst0+=4 )
        {
            // TODO: make GetKernal and GetCoord inline in vt_kernel.h
            // get four kernels for this row
            const C1dKernel& k0 = ks.GetKernel(uKernIndex);
            int iStartY0        = iSrcOffset + ks.GetCoord(uKernIndex);
            if( ++uKernIndex == uCycle )
            {
                uKernIndex  = 0;
                iSrcOffset += iCoordShift;
            }
            const C1dKernel& k1 = ks.GetKernel(uKernIndex);
            int iStartY1        = iSrcOffset + ks.GetCoord(uKernIndex);
            if( ++uKernIndex == uCycle )
            {
                uKernIndex  = 0;
                iSrcOffset += iCoordShift;
            }
            const C1dKernel& k2 = ks.GetKernel(uKernIndex);
            int iStartY2        = iSrcOffset + ks.GetCoord(uKernIndex);
            if( ++uKernIndex == uCycle )
            {
                uKernIndex  = 0;
                iSrcOffset += iCoordShift;
            }
            const C1dKernel& k3 = ks.GetKernel(uKernIndex);
            int iStartY3        = iSrcOffset + ks.GetCoord(uKernIndex);
            if( ++uKernIndex == uCycle )
            {
                uKernIndex  = 0;
                iSrcOffset += iCoordShift;
            }
    
            VT_ASSERT( iStartY0 >= 0 && iStartY0+k0.Width() <= imgSrc.Height() );
            VT_ASSERT( iStartY1 >= 0 && iStartY1+k1.Width() <= imgSrc.Height() );
            VT_ASSERT( iStartY2 >= 0 && iStartY2+k2.Width() <= imgSrc.Height() );
            VT_ASSERT( iStartY3 >= 0 && iStartY3+k3.Width() <= imgSrc.Height() );
    
            // do 4 wide at a time while possible
            TDST* pDst = pDst0;
    
            for( int x = 0; x < iProcWidth-3; 
                 x+=4, pDst=PointerOffset(pDst, 4*iDstStrideBytes) )
            {
                // unroll 4 filters
                // 0)
                const TSRC* pSrcR = imgSrc.Ptr(iStartY0)+iLeft+x;        
                __m128 mm_kval = _mm_load1_ps(&k0[0]);
                __m128 mm_0    = pfnLoad(pSrcR);
                __m128 mm_acc0 = _mm_mul_ps(mm_0, mm_kval);
                for( int i = 1; i < k0.Width(); i++ )
                {
                    pSrcR=imgSrc.NextRow(pSrcR);
    
                    mm_kval = _mm_load1_ps(&k0[i]);
                    mm_0    = pfnLoad(pSrcR);
                    mm_0    = _mm_mul_ps(mm_0, mm_kval);
                    mm_acc0 = _mm_add_ps(mm_acc0, mm_0);
                }
    
                // 1)
                pSrcR = imgSrc.Ptr(iStartY1)+iLeft+x;        
                mm_kval = _mm_load1_ps(&k1[0]);
                mm_0    = pfnLoad(pSrcR);
                __m128 mm_acc1 = _mm_mul_ps(mm_0, mm_kval);
                for( int i = 1; i < k1.Width(); i++ )
                {
                    pSrcR=imgSrc.NextRow(pSrcR);
    
                    mm_kval = _mm_load1_ps(&k1[i]);
                    mm_0    = pfnLoad(pSrcR);
                    mm_0    = _mm_mul_ps(mm_0, mm_kval);
                    mm_acc1 = _mm_add_ps(mm_acc1, mm_0);
                }
    
                // 2)
                pSrcR = imgSrc.Ptr(iStartY2)+iLeft+x;        
                mm_kval = _mm_load1_ps(&k2[0]);
                mm_0    = pfnLoad(pSrcR);
                __m128 mm_acc2 = _mm_mul_ps(mm_0, mm_kval);
                for( int i = 1; i < k2.Width(); i++ )
                {
                    pSrcR=imgSrc.NextRow(pSrcR);
    
                    mm_kval = _mm_load1_ps(&k2[i]);
                    mm_0    = pfnLoad(pSrcR);
                    mm_0    = _mm_mul_ps(mm_0, mm_kval);
                    mm_acc2 = _mm_add_ps(mm_acc2, mm_0);
                }
    
                // 3)
                pSrcR = imgSrc.Ptr(iStartY3)+iLeft+x;        
                mm_kval = _mm_load1_ps(&k3[0]);
                mm_0    = pfnLoad(pSrcR);
                __m128 mm_acc3 = _mm_mul_ps(mm_0, mm_kval);
                for( int i = 1; i < k3.Width(); i++ )
                {
                    pSrcR=imgSrc.NextRow(pSrcR);
    
                    mm_kval = _mm_load1_ps(&k3[i]);
                    mm_0    = pfnLoad(pSrcR);
                    mm_0    = _mm_mul_ps(mm_0, mm_kval);
                    mm_acc3 = _mm_add_ps(mm_acc3, mm_0);
                }
    
                pfnStore(mm_acc0, mm_acc1, mm_acc2, mm_acc3, pDst, iDstStrideBytes);
            }
        }
    }

    // finish any undone rows
    for( ; y < iProcHeight; y++, pDst0++ )
    {
        const C1dKernel& k0 = ks.GetKernel(uKernIndex);
        int iStartY0        = iSrcOffset + ks.GetCoord(uKernIndex);
        if( ++uKernIndex == uCycle )
        {
            uKernIndex  = 0;
            iSrcOffset += iCoordShift;
        }

        const TSRC* pSrc = imgSrc.Ptr(iStartY0)+iLeft;        
        TDST* pDst       = pDst0;
        for( int x = 0; x < (iProcWidth&~3); x++, pSrc++, pDst=PointerOffset(pDst, iDstStrideBytes) )
        {
            // run filter on this row one element at a time
            const TSRC* pSrcR = pSrc;
            float v  = k0[0]*float(pSrcR[0]);
            for( int i = 1; i < k0.Width(); i++ )
            {
                pSrcR=imgSrc.NextRow(pSrcR);
                v += k0[i]*float(pSrcR[0]);
            }
            VtClip(pDst, v);
        }
    }
}

template<typename TDST, typename TSRC>
int ConvolveVerticalOneBandTranspose_ProcSpecific(
    TDST* pDst0, const CTypedImg<TSRC>& imgSrc, int iDstStrideBytes, 
    int iLeft, int iProcWidth,int iProcHeight, const C1dKernelSet& ks, 
    UInt32 uKernIndex, int iSrcOffset)
{
    if ( !g_SupportSSE2() )
    {
        return 0;
    }

    int iCompleted = iProcWidth & ~3;
    if ( iCompleted )
    {
        // determine what alignment to use
        const TSRC* pSrc0 = imgSrc.Ptr()+iLeft;

        bool bSSEAlignedLoad  = 
        FilterAlignTraits<TSRC>::IsLoadAligned1Band(pSrc0, imgSrc.StrideBytes());
        bool bSSEAlignedStore = 
        FilterAlignTraits<TDST>::IsStoreAligned1Band(pDst0, iDstStrideBytes);
        if ( bSSEAlignedLoad && bSSEAlignedStore )
        {
            ConvolveVerticalOneBandTranspose_ProcSpecific<
                TDST, TSRC, LoadFourFloatsAligned, TransposeStoreAlignedOneBand>(
                    pDst0, imgSrc, iDstStrideBytes, iLeft, iProcWidth, iProcHeight, 
                    ks, uKernIndex, iSrcOffset);
        }
        else if ( bSSEAlignedLoad )
        {
            ConvolveVerticalOneBandTranspose_ProcSpecific<
                TDST, TSRC, LoadFourFloatsAligned, TransposeStoreUnalignedOneBand>(
                    pDst0, imgSrc, iDstStrideBytes, iLeft, iProcWidth, iProcHeight, 
                    ks, uKernIndex, iSrcOffset);
        }
        else
        {
            ConvolveVerticalOneBandTranspose_ProcSpecific<
                TDST, TSRC, LoadFourFloatsUnaligned, TransposeStoreUnalignedOneBand>(
                    pDst0, imgSrc, iDstStrideBytes, iLeft, iProcWidth, iProcHeight, 
                    ks, uKernIndex, iSrcOffset);
        }
    }

    // return any right border that wasn't done
    return iCompleted; 
}

template<typename TDST, void (*pfnStore)(__m128 &mm_acc0, float *pDst)>
void ConvolveHorizontalFourBands_ProcSpecific(
    TDST* pDst, UInt32& uKernIndex, int& iSrcOffset, const float* pSrc0, 
    int iProcWidth, const C1dKernelSet& ks)
{             
    // get kernel info
    UInt32 uCycle      = ks.GetCycle();
    int  iCoordShift = ks.GetCoordShiftPerCycle();

    for( int x = 0; x < iProcWidth; x++, pDst+=4 )
    {
        const C1dKernel& k = ks.GetKernel(uKernIndex);
        int iStartX = iSrcOffset + ks.GetCoord(uKernIndex);

        const float* pSrc = pSrc0 + iStartX*4;

        // run filter on this row one pixel at a time
        __m128 mm_kval = _mm_load1_ps(&k[0]);
        __m128 mm_0    = _mm_load_ps(pSrc);
        __m128 mm_acc0 = _mm_mul_ps(mm_0, mm_kval);

        for( int i = 1; i < k.Width(); i++ )
        {
            pSrc += 4;
            mm_kval = _mm_load1_ps(&k[i]);
            mm_0    = _mm_load_ps(pSrc);
            mm_0    = _mm_mul_ps(mm_0, mm_kval);
            mm_acc0 = _mm_add_ps(mm_acc0, mm_0);
        }

        pfnStore(mm_acc0, pDst);

        // next kernel
        VT_ASSERT( uKernIndex < uCycle );
        if( ++uKernIndex == uCycle )
        {
            uKernIndex  = 0;
            iSrcOffset += iCoordShift;
        }
    }
}

template<typename TDST>
int ConvolveHorizontalFourBands_ProcSpecific(
    TDST* pDst, UInt32& uKernIndex, int& iSrcOffset,
    const float* pSrc0, int iProcWidth, const C1dKernelSet& ks)
{
    if( !g_SupportSSE2() )
    {
        return 0;
    }

    bool bSSEAlignedStore = 
        FilterAlignTraits<float>::IsStoreAligned4Band(pDst, 0);
    if( bSSEAlignedStore )
    {
        ConvolveHorizontalFourBands_ProcSpecific<TDST, StoreAlignedFourBands>(
            pDst, uKernIndex, iSrcOffset, pSrc0, iProcWidth, ks);
    }
    else
    {
        ConvolveHorizontalFourBands_ProcSpecific<TDST, StoreUnalignedFourBands>(
            pDst, uKernIndex, iSrcOffset, pSrc0, iProcWidth, ks);
    }

    // indicate everything done
    return iProcWidth;
}

template<typename TDST>
int ConvolveHorizontalThreeBands_ProcSpecific(
    TDST* pDst, UInt32& uKernIndex, int& iSrcOffset, const float* pSrc0, 
    int iProcWidth, const C1dKernelSet& ks)
{             
    if( !g_SupportSSE2() )
    {
        return 0;
    }

    // get kernel info
    UInt32 uCycle      = ks.GetCycle();
    int  iCoordShift = ks.GetCoordShiftPerCycle();

    for( int x = 0; x < iProcWidth; x++, pDst+=3 )
    {
        const C1dKernel& k = ks.GetKernel(uKernIndex);
        int iStartX = iSrcOffset + ks.GetCoord(uKernIndex);

        const float* pSrc = pSrc0 + iStartX*3;

        // run filter on this row one pixel at a time
        __m128 mm_kval = _mm_load1_ps(&k[0]);
        __m128 mm_0    = _mm_loadu_ps(pSrc);
        __m128 mm_acc0 = _mm_mul_ps(mm_0, mm_kval);

        for( int i = 1; i < k.Width(); i++ )
        {
            pSrc += 3;
            mm_kval = _mm_load1_ps(&k[i]);
            mm_0    = _mm_loadu_ps(pSrc);
            mm_0    = _mm_mul_ps(mm_0, mm_kval);
            mm_acc0 = _mm_add_ps(mm_acc0, mm_0);
        }

        pDst[0] = mm_acc0.m128_f32[0];
        pDst[1] = mm_acc0.m128_f32[1];
        pDst[2] = mm_acc0.m128_f32[2];

        // next kernel
        VT_ASSERT( uKernIndex < uCycle );
        if( ++uKernIndex == uCycle )
        {
            uKernIndex  = 0;
            iSrcOffset += iCoordShift;
        }
    }

    return iProcWidth;
}
#endif

template<typename TSRC>
void ConvolveVertical(CFloatImg& imgDst, const CTypedImg<TSRC>& imgSrc, 
                      const C1dKernelSet& ks, UInt32 uKernIndex0, int iSrcOffset0)
{
    VT_ASSERT( imgDst.Width() == imgSrc.Width() );
    VT_ASSERT( imgDst.Bands() == imgSrc.Bands() );

    // process columns of one cache line width (64 bytes) at a time
    int iProcWidthMain = 64 / sizeof(TSRC);
    int iProcWidth;

    // compute element counts
    int iDstElWidth = imgDst.Width()*imgDst.Bands();

    // get kernel info
    UInt32 uCycle    = ks.GetCycle();
    int  iCoordShift = ks.GetCoordShiftPerCycle();

    // run the filter in cache-line aligned columns
    for( int xx = 0; xx < iDstElWidth; xx+=iProcWidth )
    {
        // if the source is not aligned to 64byte cache lines process extra
        // in this column such that the next column is aligned
        // note: this only helps when the input stride causes the next 
        //       source line to also be cache aligned
        iProcWidth   = iProcWidthMain;
        int iOffset0 = uintptr_t(imgSrc.Ptr()) & 0x3f;
        if( iOffset0 )
        {
            iProcWidth += (64 - iOffset0) / sizeof(TSRC);
        }
        // process up to the end if last bit isn't a full cache line
        int iToEnd = iDstElWidth-xx;
        if( iToEnd < iProcWidth + iProcWidthMain )
        {
            iProcWidth = iToEnd;
        }

        // get the kernel
        int  iSrcOffset = iSrcOffset0;
        UInt32 uKernIndex = uKernIndex0;

        float* pDst0 = imgDst.Ptr() + xx;

#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)
        // try processor specific speed-ups
        int x0 = ConvolveVertical_ProcSpecific(pDst0, imgSrc, imgDst.StrideBytes(),
                                               xx, iProcWidth, imgDst.Height(),
                                               ks, uKernIndex, iSrcOffset);
        if( x0 == iProcWidth )
        {
            continue;
        }

        // adjust for any processing done by _ProcSpecific
        pDst0 += x0;
#else
		int x0 = 0;
#endif
        for( int y=0; y < imgDst.Height(); y++, pDst0=imgDst.NextRow(pDst0) )
        {
            // descend the column
            const C1dKernel& k = ks.GetKernel(uKernIndex);
            int iStartY        = iSrcOffset + ks.GetCoord(uKernIndex);
            VT_ASSERT( iStartY >= 0 && iStartY+k.Width() <= imgSrc.Height() );

            const TSRC* pSrc = imgSrc.Ptr(iStartY) + xx + x0;
            float* pDst      = pDst0;
            for( int x = x0; x < iProcWidth; x++, pSrc++, pDst++ )
            {
                // run filter on this row one element at a time
                const TSRC* pSrcR = pSrc;
                float v  = k[0]*float(pSrcR[0]);
                for( int i = 1; i < k.Width(); i++ )
                {
                    pSrcR=imgSrc.NextRow(pSrcR);
                    v += k[i]*float(pSrcR[0]);
                }
                *pDst = v;
            }

            // next kernel
            VT_ASSERT( uKernIndex < uCycle );
            if( ++uKernIndex == uCycle )
            {
                uKernIndex  = 0;
                iSrcOffset += iCoordShift;
            }
        }
    }
}

void ConvolveVertical(CFloatImg& imgDst, const CImg& imgSrc, const C1dKernelSet& ks, 
                      UInt32 uKernIndex, int iSrcOffset)
{
    // ASSERT that dst/src are setup correctly
    VT_ASSERT( (uintptr_t(imgDst.BytePtr(0))&0x3f) == 0 );
    VT_ASSERT( imgDst.Height() < 2 || (uintptr_t(imgDst.BytePtr(1))&0x3f) == 0 );
    VT_ASSERT( imgSrc.Bands() == imgDst.Bands() );
    VT_ASSERT( EL_FORMAT(imgSrc.GetType()) != EL_FORMAT_HALF_FLOAT );

    // determine which specialization to call
    switch( EL_FORMAT(imgSrc.GetType()) )
    {
    case EL_FORMAT_BYTE:
        ConvolveVertical<Byte>(imgDst, (CByteImg&)imgSrc, ks, uKernIndex, 
                               iSrcOffset);
        break;
    case EL_FORMAT_SHORT:
        ConvolveVertical<UInt16>(imgDst, (CShortImg&)imgSrc, ks, uKernIndex,
                                 iSrcOffset);
        break;
    case EL_FORMAT_FLOAT:
        ConvolveVertical<float>(imgDst, (CFloatImg&)imgSrc, ks, uKernIndex, 
                                iSrcOffset);
        break;
    }
}

template<typename TDST, typename TSRC>
void ConvolveVerticalTransposeOneBand(
    CTypedImg<TDST>& imgDst, const CTypedImg<TSRC>& imgSrc, 
    const C1dKernelSet& ks, UInt32 uKernIndex0, int iSrcOffset0)
{
    VT_ASSERT( imgDst.Height() == imgSrc.Width() );
    VT_ASSERT( imgDst.Bands()  == imgSrc.Bands() );

    // process columns of one cache line width (64 bytes) at a time
    int iProcWidthMain = 64 / sizeof(TSRC);
    int iProcWidth;

    // compute element counts
    int iDstElHeight = imgDst.Height()*imgDst.Bands();

    // get kernel info
    UInt32 uCycle    = ks.GetCycle();
    int  iCoordShift = ks.GetCoordShiftPerCycle();

    // run the filter in cache-line aligned columns
    for( int xx = 0; xx < iDstElHeight; xx+=iProcWidth )
    {
        // if the source is not aligned to 64byte cache lines process extra
        // in this column such that the next column is aligned
        // note: this only helps when the input stride causes the next 
        //       source line to also be cache aligned
        iProcWidth   = iProcWidthMain;
        int iOffset0 = uintptr_t(imgSrc.Ptr()) & 0x3f;
        if( iOffset0 )
        {
            iProcWidth += (64 - iOffset0) / sizeof(TSRC);
        }
        // process up to the end if last bit isn't a full cache line
        int iToEnd = iDstElHeight-xx;
        if( iToEnd < iProcWidth + iProcWidthMain )
        {
            iProcWidth = iToEnd;
        }

        // get the kernel
        int  iSrcOffset   = iSrcOffset0;
        UInt32 uKernIndex = uKernIndex0;

        TDST* pDst0 = imgDst.Ptr(xx);

#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)
        // try processor specific speed-ups
        int x0 = ConvolveVerticalOneBandTranspose_ProcSpecific(
            pDst0, imgSrc, imgDst.StrideBytes(), xx, iProcWidth, imgDst.Width(),
            ks, uKernIndex, iSrcOffset);
        if( x0 == iProcWidth )
        {
            continue;
        }

        // adjust for any processing done by _ProcSpecific
        pDst0 = imgDst.OffsetRow(pDst0, x0);
#else
		int x0 = 0;
#endif
        for( int y=0; y < imgDst.Width(); y++, pDst0++ )
        {
            // descend the column
            const C1dKernel& k = ks.GetKernel(uKernIndex);
            int iStartY        = iSrcOffset + ks.GetCoord(uKernIndex);
            VT_ASSERT( iStartY >= 0 && iStartY+k.Width() <= imgSrc.Height() );

            const TSRC* pSrc = imgSrc.Ptr(iStartY) + xx + x0;
            TDST* pDst       = pDst0;
            for( int x = x0; x < iProcWidth; x++, pSrc++, pDst=imgDst.NextRow(pDst) )
            {
                // run filter on this row one element at a time
                const TSRC* pSrcR = pSrc;
                float v  = k[0]*float(pSrcR[0]);
                for( int i = 1; i < k.Width(); i++ )
                {
                    pSrcR=imgSrc.NextRow(pSrcR);
                    v += k[i]*float(pSrcR[0]);
                }
                VtClip(pDst, v);
            }

            // next kernel
            VT_ASSERT( uKernIndex < uCycle );
            if( ++uKernIndex == uCycle )
            {
                uKernIndex  = 0;
                iSrcOffset += iCoordShift;
            }
        }
    }
}

void ConvolveVerticalTransposeOneBand(CImg& imgDst, const CImg& imgSrc, 
                                      const C1dKernelSet& ks, UInt32 uKernIndex, 
                                      int iSrcOffset, int iPass)
{

    // Check that caller set things up correctly.  
    // - intermediate image should always be float format
    // - rows of intermediate should be 64 byte aligned
    // - the band counts should be equal
    // - never deal with half float here
#if !_DEBUG
    UNREFERENCED_PARAMETER(iPass);
#endif
#if _DEBUG
    const CImg* pTst = (iPass == 0)? &imgDst: &imgSrc;
    VT_ASSERT( EL_FORMAT(pTst->GetType()) == EL_FORMAT_FLOAT );
    VT_ASSERT( imgSrc.Bands()==1 && imgDst.Bands()==1 );
    VT_ASSERT( (uintptr_t(pTst->BytePtr(0))&0x3f) == 0 );
    VT_ASSERT( pTst->Height() < 2 || (uintptr_t(pTst->BytePtr(1))&0x3f) == 0 );
    VT_ASSERT( imgSrc.Bands()==imgDst.Bands() );
    VT_ASSERT( EL_FORMAT(imgSrc.GetType()) != EL_FORMAT_HALF_FLOAT &&
               EL_FORMAT(imgDst.GetType()) != EL_FORMAT_HALF_FLOAT );
#endif

    // determine which specialization to call
    switch( EL_FORMAT(imgSrc.GetType()) )
    {
    case EL_FORMAT_BYTE:
        ConvolveVerticalTransposeOneBand<float, Byte>(
            (CFloatImg&)imgDst, (CByteImg&)imgSrc, ks, uKernIndex, iSrcOffset);
        break;
    case EL_FORMAT_SHORT:
        ConvolveVerticalTransposeOneBand<float, UInt16>(
            (CFloatImg&)imgDst, (CShortImg&)imgSrc, ks, uKernIndex, iSrcOffset);
        break;
    case EL_FORMAT_FLOAT:
        switch( EL_FORMAT(imgDst.GetType()) )
        {
        case EL_FORMAT_BYTE:
            ConvolveVerticalTransposeOneBand(
                (CByteImg&)imgDst, (CFloatImg&)imgSrc, ks, uKernIndex, iSrcOffset);
            break;
        case EL_FORMAT_SHORT:
            ConvolveVerticalTransposeOneBand(
                (CShortImg&)imgDst, (CFloatImg&)imgSrc, ks, uKernIndex, iSrcOffset);
            break;
        case EL_FORMAT_FLOAT:
            ConvolveVerticalTransposeOneBand(
                (CFloatImg&)imgDst, (CFloatImg&)imgSrc, ks, uKernIndex, iSrcOffset);
            break;
        }
        break;
    }
}

template<typename TDST>
void ConvolveHorizontal(CTypedImg<TDST>& imgDst, const CFloatImg& imgSrc,
                        const C1dKernelSet& ks, UInt32 uKernIndex0, int iSrcOffset0)
{
    int iSrcB = imgSrc.Bands();
    int iDstB = imgDst.Bands();
    bool bUseConvBuf = !(EL_FORMAT(imgDst.GetType())==EL_FORMAT_FLOAT && 
                         iSrcB==iDstB);

    // use a temp buffer to convert to dest type
    CTypedBuffer1<float> bufConv(iSrcB);

    // get kernel info
    UInt32 uCycle      = ks.GetCycle();
    int  iCoordShift = ks.GetCoordShiftPerCycle();

    TDST* pDst0 = imgDst.Ptr();
    for( int y=0; y < imgDst.Height(); y++, pDst0=imgDst.NextRow(pDst0) )
    {
        int  iSrcOffset    = iSrcOffset0;
        UInt32 uKernIndex  = uKernIndex0;
        const float* pSrc0 = imgSrc.Ptr(y);
        TDST* pDst         = pDst0;

        // convert into the typed buffer except if dest is already float and
        // band count matches
        for( CSpanIterator sp(imgDst.Width(), bufConv.Capacity()); !sp.Done();
             pDst+=sp.GetCount()*iDstB, sp.Advance() )
        {
            float* pConv = bUseConvBuf? bufConv.Buffer1(): (float*)pDst;
             
            int x = 0;

#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)

            if( iSrcB == 4 )
            {
                x = ConvolveHorizontalFourBands_ProcSpecific(
                    pConv, uKernIndex, iSrcOffset, pSrc0, sp.GetCount(), ks);
            }
            else if( iSrcB == 3 && y != imgDst.Height()-1 )
            {
                // note: a bit of a hack here.  don't run SSE code on last row
                //       of source because the 3 band implementation loads "4"
                //       values at once and the last pixel will index one 
                //       element beyond the end
                x = ConvolveHorizontalThreeBands_ProcSpecific(
                    pConv, uKernIndex, iSrcOffset, pSrc0, sp.GetCount(), ks);
            }
#endif

            for( ; x < sp.GetCount(); x++ )
            {
                const C1dKernel& k = ks.GetKernel(uKernIndex);
                int iStartX = iSrcOffset + ks.GetCoord(uKernIndex);
                VT_ASSERT( iStartX >= 0 && iStartX+k.Width() <= imgSrc.Width() );
                
                const float* pSrc = pSrc0 + iStartX*iSrcB;
                for( int b = 0; b < iSrcB; b++, pConv++ )
                {
                    // run filter on this row one element at a time
                    const float* pSrcC = pSrc+b;
                    float v  = k[0]*pSrcC[0];
                    for( int i = 1; i < k.Width(); i++ )
                    {
                        pSrcC+=iSrcB;
                        v += k[i]*pSrcC[0];
                    }
                    *pConv = v;
                }
                
                // next kernel
                VT_ASSERT( uKernIndex < uCycle );
                if( ++uKernIndex == uCycle )
                {
                    uKernIndex  = 0;
                    iSrcOffset += iCoordShift;
                }
            }

            // convert if necessary
            if( bUseConvBuf )
            {
                VtConvertSpan(pDst, imgDst.GetType(), bufConv.Buffer1(), 
							  VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, iSrcB),
                              sp.GetCount()*iSrcB);
            }
        }
    }
}

void ConvolveHorizontal(CImg& imgDst, const CFloatImg& imgSrc, 
                        const C1dKernelSet& ks, UInt32 uKernIndex, int iSrcOffset)
{
    // ASSERT that dst/src are setup correctly
    VT_ASSERT( (uintptr_t(imgSrc.BytePtr(0))&0x3f) == 0  );
    VT_ASSERT( imgDst.Height() < 2 || (uintptr_t(imgSrc.BytePtr(1))&0x3f) == 0 );

    // determine which specialization to call
    switch( EL_FORMAT(imgDst.GetType()) )
    {
    case EL_FORMAT_BYTE:
        ConvolveHorizontal<Byte>((CByteImg&)imgDst, imgSrc, ks, uKernIndex, 
                                 iSrcOffset);
        break;
    case EL_FORMAT_SHORT:
        ConvolveHorizontal<UInt16>((CShortImg&)imgDst, imgSrc, ks, uKernIndex,
                                   iSrcOffset);
        break;
    case EL_FORMAT_HALF_FLOAT:
        ConvolveHorizontal<HALF_FLOAT>((CHalfFloatImg&)imgDst, imgSrc, ks,
                                       uKernIndex, iSrcOffset);
        break;
    case EL_FORMAT_FLOAT:
        ConvolveHorizontal<float>((CFloatImg&)imgDst, imgSrc, ks, uKernIndex, 
                                  iSrcOffset);
        break;
    }
}

//+-----------------------------------------------------------------------------
//
// Function: VtSeparableFilter  - polyphase version
// 
//------------------------------------------------------------------------------
// TODO: expose this in the kernel API
extern void GetKernelSetPosition(OUT UInt32& uIndexInCycle, OUT int& iSrcStart, 
                                 int iDstStart, const C1dKernelSet& ks);

// return true if kernel sets are equivalent
bool TestEqual(const C1dKernelSet& ks1, const C1dKernelSet& ks2)
{
    if( ks1.GetCoordShiftPerCycle() != ks2.GetCoordShiftPerCycle() ) { return false; }
    if( ks1.GetCycle() != ks2.GetCycle() ) { return false; }
    for( UInt32 i=0; i<ks1.GetCycle(); i++)
    {
        if( ks1.GetKernel(i).Center() != ks2.GetKernel(i).Center() ) { return false; }
        if( ks1.GetKernel(i).Width() != ks2.GetKernel(i).Width() ) { return false; }
        for( int j=0; j<(int)ks1.GetKernel(i).Width(); j++)
        {
            if( ks1.GetKernel(i)[j] != ks2.GetKernel(i)[j] ) { return false; }
            if( ks1.GetCoord(i) != ks2.GetCoord(i) ) { return false; }
        }
    }
    return true;
}

// from separablefilter14641.cpp
extern HRESULT SeparableFilter14641Decimate2to1Internal(CFloatImg& imgDst,const CRect& rctDst,
                    const CFloatImg& imgSrc, CPoint ptSrc);
extern HRESULT SeparableFilter14641Decimate2to1Internal(CByteImg& imgDst,const CRect& rctDst,
                    const CByteImg& imgSrc, CPoint ptSrc);

HRESULT
vt::VtSeparableFilter(CImg &imgDst, const CRect& rctDst, 
                      const CImg &imgSrc, CPoint ptSrcOrigin,
                      const C1dKernelSet& ksh, const C1dKernelSet& ksv,
                      const IMAGE_EXTEND& ex)
{
	if( ksh.GetCycle() == 1  && ksh.GetCoordShiftPerCycle() == 1 &&
		ksh.GetCoord(0) == -ksh.GetKernel(0).Center() &&
		ksv.GetCycle() == 1 && ksv.GetCoordShiftPerCycle() == 1 &&
		ksv.GetCoord(0) == -ksv.GetKernel(0).Center() )		
	{
		return VtSeparableFilter(imgDst, rctDst, imgSrc, ptSrcOrigin, 
								 ksh.GetKernel(0), ksv.GetKernel(0), ex);
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
    // - always operate in source type except for half-float which should be 
    //   converted to float before passing to filter.
    // - reduce the band-count as early as possible
    int iSrcType = EL_FORMAT(imgSrc.GetType());
    int iDstType = EL_FORMAT(imgDst.GetType());
    int iOpBands = VtMin(imgSrc.Bands(), imgDst.Bands());
    int iOpType  = EL_FORMAT(imgSrc.GetType());
    if( iOpType == EL_FORMAT_HALF_FLOAT )
    {
        iOpType = EL_FORMAT_FLOAT;
    }
    int iConvType = iDstType;
    if( iConvType == EL_FORMAT_HALF_FLOAT )
    {
        iConvType = EL_FORMAT_FLOAT;
    }

    // determine the rect of the full provided source
    vt::CRect rctSrc = imgSrc.Rect();

    // determine if diversion to 14641-specific codepath is possible
    bool bUse14641Path = false;
    if( (iOpBands <= 4) &&
        (imgSrc.GetType() == imgDst.GetType()) &&
        ( (iDstType == EL_FORMAT_BYTE) || 
          (iDstType == EL_FORMAT_FLOAT) ) &&
        TestEqual(ksh,ksv) &&
        (ksh.GetCycle() == 1) &&
        (ksh.GetCoordShiftPerCycle() == 2) )
    {
        C14641Kernel k14641;
        if( (ksh.GetKernel(0).Center() == k14641.AsKernel().Center()) &&
            (ksh.GetKernel(0).Width() == k14641.AsKernel().Width()) &&
            (ksh.GetKernel(0)[0] == k14641.AsKernel()[0]) &&
            (ksh.GetKernel(0)[1] == k14641.AsKernel()[1]) &&
            (ksh.GetKernel(0)[2] == k14641.AsKernel()[2]) &&
            (ksh.GetKernel(0)[3] == k14641.AsKernel()[3]) &&
            (ksh.GetKernel(0)[4] == k14641.AsKernel()[4]) )
        {
            bUse14641Path = true;
        }
    }

    // loop over the destination blocks
    const UInt32 c_blocksize = 128;

    // generate a scaled vertical kernel set to perform the type conversion
    // scale inline with the filtering
    C1dKernelSet ksvscl;
    VT_HR_EXIT( ksvscl.Create(ksv) );
    float fMaxSrcVal = 
        (iSrcType==EL_FORMAT_BYTE)?  float(ElTraits<Byte>::MaxVal()): 
        (iSrcType==EL_FORMAT_SHORT)? float(ElTraits<UInt16>::MaxVal()): 
        ElTraits<float>::MaxVal();
    if( iOpBands != 1 || iDstType==EL_FORMAT_HALF_FLOAT )
    {
        ScaleKernelSet(ksvscl, ksv, 1.f/fMaxSrcVal);
    }
    else
    {
        float fMaxDstVal = 
            (iDstType==EL_FORMAT_BYTE)?  float(ElTraits<Byte>::MaxVal()): 
            (iDstType==EL_FORMAT_SHORT)? float(ElTraits<UInt16>::MaxVal()): 
            ElTraits<float>::MaxVal();
        ScaleKernelSet(ksvscl, ksv, fMaxDstVal/fMaxSrcVal);
    }

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
        vt::CRect rctSrcBlk = GetRequiredSrcRect(rctDstBlk, ksh, ksv) - ptSrcOrigin;
        
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

        if( bUse14641Path )
        {
            if( iOpType == EL_FORMAT_FLOAT )
            {
                VT_HR_EXIT( SeparableFilter14641Decimate2to1Internal(*((CFloatImg*)&imgDstBlk),
                    imgDstBlk.Rect(),*((CFloatImg*)&imgConv),CPoint(-2,-2)) );
            }
            else
            {
                VT_HR_EXIT( SeparableFilter14641Decimate2to1Internal(*((CByteImg*)&imgDstBlk),
                    imgDstBlk.Rect(),*((CByteImg*)&imgConv),CPoint(-2,-2)) );
            }
        }
        else if( iOpBands == 1 )
        {
            // create an intermediate float block
            VT_HR_EXIT( imgTmp.Create(imgDstBlk.Height(), imgConv.Width(), 
                                      iOpBands, align64ByteRows) );
    
            // filter vertically
            UInt32 uKernIndex;
            int  iSrcOffset;
            GetKernelSetPosition(uKernIndex, iSrcOffset, rctDstBlk.top, ksv);
            ConvolveVerticalTransposeOneBand(imgTmp, imgConv, ksvscl, uKernIndex, 
                iSrcOffset - (rctSrcBlk.top+ptSrcOrigin.y), 0);

            // filter horizontally
            GetKernelSetPosition(uKernIndex, iSrcOffset, rctDstBlk.left, ksh);
            if( iOpBands != imgDst.Bands() || 
                iDstType==EL_FORMAT_HALF_FLOAT )
            {
                // sometimes a temp conversion buffer is needed
                VT_ASSERT( iConvType==iDstType || (iConvType==EL_FORMAT_FLOAT && iDstType== EL_FORMAT_HALF_FLOAT) );
                VT_HR_EXIT( imgConv.Create(imgDstBlk.Width(), imgDstBlk.Height(),
										   VT_IMG_MAKE_TYPE(iConvType, iOpBands),
                                           align64ByteRows) );
                ConvolveVerticalTransposeOneBand(imgConv, imgTmp, ksh, uKernIndex,
                    iSrcOffset - (rctSrcBlk.left+ptSrcOrigin.x), 1);
                VT_HR_EXIT( VtConvertImage(imgDstBlk, imgConv) );
            }
            else
            {
                ConvolveVerticalTransposeOneBand(imgDstBlk, imgTmp, ksh, uKernIndex,
                    iSrcOffset - (rctSrcBlk.left+ptSrcOrigin.x), 1);
            }

        }
        else
        {
            // create an intermediate float block
            VT_HR_EXIT( imgTmp.Create(imgConv.Width(), imgDstBlk.Height(), iOpBands, 
                                      align64ByteRows) );
    
            // filter vertically
            UInt32 uKernIndex;
            int  iSrcOffset;
            GetKernelSetPosition(uKernIndex, iSrcOffset, rctDstBlk.top, ksv);
            ConvolveVertical(imgTmp, imgConv, ksvscl, uKernIndex, 
                             iSrcOffset - (rctSrcBlk.top+ptSrcOrigin.y));
            
            // filter horizontally
            GetKernelSetPosition(uKernIndex, iSrcOffset, rctDstBlk.left, ksh);
            ConvolveHorizontal(imgDstBlk, imgTmp, ksh, uKernIndex,
                               iSrcOffset - (rctSrcBlk.left+ptSrcOrigin.x));
        }
    }

    VT_HR_END()
}


//+-----------------------------------------------------------------------------
//
// Function: VtFilter1d
// 
// Synopsis: 1 dimension filtering code
// 
//------------------------------------------------------------------------------
const void* TypedPointerOffset(const void* p, int iOff, int iType)
{
    switch(EL_FORMAT(iType))
    {
    case EL_FORMAT_BYTE:
        return (Byte*)p+iOff;
    case EL_FORMAT_SHORT:
        return (UInt16*)p+iOff;
    case EL_FORMAT_HALF_FLOAT:
        return (HALF_FLOAT*)p+iOff;
    case EL_FORMAT_FLOAT:
        return (float*)p+iOff;
    }
    return p;
}

void* TypedPointerOffset(void* p, int iOff, int iType)
{ 
    return const_cast<void*>(TypedPointerOffset(static_cast<const void*>(p), 
                                                iOff, iType));
}

HRESULT
vt::VtFilter1d(void* pDst, int iDstAddr, int iDstWidth, 
			   int iDstType, int iDstBands,
               const void* pSrc, int iSrcAddr, int iSrcWidth, 
			   int iSrcType, int iSrcBands, const C1dKernel& k, ExtendMode ex)
{
    int iOpBands = VtMin(iSrcBands, iDstBands);

	// mask out any extra bits
	iSrcType = EL_FORMAT(iSrcType);
	iDstType = EL_FORMAT(iDstType);
	int iWrkType = VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, iOpBands);

    // verify that src/dst are not overlapping
    if( pDst == pSrc )
    {
        return E_INVALIDDST;
    }

    CTypedBuffer2<float, float, 4096> bufTmp(iOpBands, iDstBands);

	// compute required filter support
	int iFilterL = k.Center();
	int iFilterR = k.Width() - k.Center() - 1;

	if( iFilterL+iFilterR >= bufTmp.Capacity() )
    {
        // filter is too wide - in this case we should allocate memory on 
        // the heap instead, but for now NOTIMPL
        return E_NOTIMPL;
    }
    if( !(ex == Extend || ex == Zero) )
    {
        // need to implement the undone ones
        return E_NOTIMPL;
    }

    int iProcWidth;
    for( int i = 0; i < iDstWidth; i+=iProcWidth )
    {
        // determine required source span
        iProcWidth = bufTmp.Capacity() - (iFilterL+iFilterR);
        iProcWidth = VtMin(iProcWidth, iDstWidth-i);

		int iSrcL = i + iDstAddr - iFilterL;
		int iSrcR = i + iDstAddr + iProcWidth + iFilterR;
	     
        float* pWrkSrc;
        if( iSrcL < iSrcAddr || iSrcR > (iSrcAddr + iSrcWidth) || 
            EL_FORMAT(iSrcType) != EL_FORMAT_FLOAT || iSrcBands != iOpBands )
        {
            // convert source to buffer
            pWrkSrc = bufTmp.Buffer1();

			// compute the start and end source info and clip against available
			// data
            int iSrcStrt = VtMax(VtMin(iSrcL-iSrcAddr, iSrcWidth),0);
            int iSrcEnd  = VtMax(VtMin(iSrcR-iSrcAddr, iSrcWidth),0);
			int iCopySrc = iSrcEnd - iSrcStrt;

			int iExtendL = VtMax(iSrcAddr - iSrcL, 0);
			int iExtendR = VtMax(iSrcR - (iSrcAddr+iSrcWidth), 0);

			VT_ASSERT( (iExtendL + iCopySrc + iExtendR) == 
					   (iFilterL + iProcWidth + iFilterR) );

			if( iCopySrc )
			{
				VtConvertSpan(pWrkSrc+iExtendL*iOpBands, iWrkType,
							  TypedPointerOffset(pSrc, iSrcStrt*iSrcBands, iSrcType),
							  VT_IMG_MAKE_TYPE(iSrcType, iSrcBands),
							  iCopySrc*iOpBands);
			}

            // apply extension if necessary
            if( iExtendL )
            {
                if( ex==Zero )
                {
                    float val = 0;
                    VtFillSpan(pWrkSrc, &val, sizeof(val), iExtendL*iOpBands);
                }
                else
                {
                    VT_ASSERT(ex == Extend);
					VtConvertSpan(pWrkSrc, iWrkType, 
								  pSrc, VT_IMG_MAKE_TYPE(iSrcType, iSrcBands),
								  iSrcBands);
                    VtFillSpan(pWrkSrc+iOpBands, pWrkSrc, sizeof(float)*iOpBands,
							   iExtendL-1);
                }
            }
            if( iExtendR )
            {
                int iBufR = iExtendL + iCopySrc;

                if( ex==Zero )
                {
                    float val = 0;
                    VtFillSpan(pWrkSrc+iBufR*iOpBands, &val, sizeof(val),
							   iExtendR*iOpBands );
                }
                else
                {
					float* pE0 = pWrkSrc+iBufR*iOpBands;
                    VT_ASSERT(ex == Extend);
					VtConvertSpan(pE0, iWrkType,
								  TypedPointerOffset(pSrc, (iSrcWidth-1)*iSrcBands, iSrcType), 
								  VT_IMG_MAKE_TYPE(iSrcType, iSrcBands), 
								  iSrcBands);
                    VtFillSpan(pE0+iOpBands, pE0, sizeof(float)*iOpBands,
							   iExtendR-1);
                }
            }
        }
        else
        {
            pWrkSrc = ((float*)pSrc) + (iSrcL - iSrcAddr)*iSrcBands;
        }

        // determine where to put the dest
        bool bUseTmpDst = 
			EL_FORMAT(iDstType) != EL_FORMAT_FLOAT || iOpBands!=iDstBands;
        float* pWrkDst = bUseTmpDst? bufTmp.Buffer2(): (float*)pDst+i*iDstBands;

        // filter
        for( int j = 0; j < iProcWidth; j++, pWrkSrc+=iOpBands, pWrkDst+=iOpBands )
        {
            for( int b = 0; b < iOpBands; b++ )
            {
                const float* pSrcC = pWrkSrc+b;
                float v = k[0]*pSrcC[0];
                for( int l = 1; l < k.Width(); l++ )
                {
                    pSrcC+=iOpBands;
                    v += k[l]*pSrcC[0];
                }
                pWrkDst[b] = v;
            }
        }

        // convert to dest format if necessary
        if( bUseTmpDst )
        {
            VtConvertSpan(TypedPointerOffset(pDst,i*iDstBands,iDstType), 
						  VT_IMG_MAKE_TYPE(iDstType, iDstBands),
                          bufTmp.Buffer2(), iWrkType,
                          iProcWidth*iOpBands);
        }
    }

    return S_OK;
}

#ifndef VT_NO_XFORMS

//+-----------------------------------------------------------------------------
//
// Class: CSeparableFilterTransform
// 
//------------------------------------------------------------------------------

HRESULT CSeparableFilterTransform::Clone(ITaskState **ppState)
{
	return CloneTaskState<CSeparableFilterTransform>(ppState, 
		[this](CSeparableFilterTransform* pN)
	{ return pN->Initialize( m_dstType, m_kernelsetHoriz, m_kernelsetVert); });
}

void
CSeparableFilterTransform::GetDstPixFormat(OUT int& frmtDst,
                                           IN  const int* pfrmtSrcs, 
                                           IN  UInt32  /*uSrcCnt*/)
{   
	frmtDst = UpdateMutableTypeFields(m_dstType, *pfrmtSrcs);
	if( !IsValidConvertPair(*pfrmtSrcs, frmtDst) )
    {
		frmtDst = OBJ_UNDEFINED;
	}
}

HRESULT
CSeparableFilterTransform::Transform(CImg* pimgDst, IN  const CRect& rctDst,
                                     const CImg& imgSrc, const CPoint& ptSrc)
{
    if( m_kernelsetHoriz.GetCycle() == 1              && 
		m_kernelsetHoriz.GetCoordShiftPerCycle() == 1 &&
		m_kernelsetHoriz.GetCoord(0) == -m_kernelsetHoriz.GetKernel(0).Center() &&
        m_kernelsetVert.GetCycle() == 1 &&
		m_kernelsetVert.GetCoordShiftPerCycle() == 1 &&
		m_kernelsetVert.GetCoord(0) == -m_kernelsetVert.GetKernel(0).Center() )
    {
        return TransformSingleKernel(pimgDst, rctDst, imgSrc, ptSrc);
    }
    else
    {
        return TransformMultiKernel(pimgDst, rctDst, imgSrc, ptSrc);
    }
}

HRESULT
CSeparableFilterTransform::Initialize(int dstType, 
                                      const C1dKernel& krnlHoriz, 
                                      const C1dKernel& krnlVert)
{
    m_dstType  = dstType;

    VT_HR_BEGIN()
    VT_HR_EXIT( m_kernelsetHoriz.Create(krnlHoriz) );
    VT_HR_EXIT( m_kernelsetHorizScl.Create(krnlHoriz) );
    VT_HR_EXIT( m_kernelsetVert.Create(krnlVert) );
    VT_HR_EXIT( m_kernelsetVertScl.Create(krnlVert) );
    VT_HR_END()
}

HRESULT
CSeparableFilterTransform::Initialize(int dstType, 
                                      const C1dKernelSet& krnlsetHoriz,
                                      const C1dKernelSet& krnlsetVert)
{
    m_dstType  = dstType;

    VT_HR_BEGIN()
    VT_HR_EXIT( m_kernelsetHoriz.Create(krnlsetHoriz) );
    VT_HR_EXIT( m_kernelsetHorizScl.Create(krnlsetHoriz) );
    VT_HR_EXIT( m_kernelsetVert.Create(krnlsetVert) );
    VT_HR_EXIT( m_kernelsetVertScl.Create(krnlsetVert) );
    VT_HR_END()
}

HRESULT
CSeparableFilterTransform::InitializeForPyramidGen(int dstType, 
												   const C1dKernel& krnl)
{
    m_dstType  = dstType;

    VT_HR_BEGIN()
    VT_HR_EXIT( m_kernelsetHoriz.Create(1,2) );
    VT_HR_EXIT( m_kernelsetHoriz.Set(0, -krnl.Center(), krnl) );

    VT_HR_EXIT( m_kernelsetHorizScl.Create(m_kernelsetHoriz) );
    VT_HR_EXIT( m_kernelsetVert.Create(m_kernelsetHoriz) );
    VT_HR_EXIT( m_kernelsetVertScl.Create(m_kernelsetHoriz) );
    VT_HR_END()
}

HRESULT
CSeparableFilterTransform::TransformSingleKernel(
    CImg* pimgDst, IN  const CRect& rctDst, const CImg& imgSrc, const CPoint& ptSrc)
{
    // TODO: use the memory pool once it is in place

    VT_ASSERT( !imgSrc.IsSharingMemory(*pimgDst) );

    VT_HR_BEGIN()

    // determine the "operation" type.  
    // - always operate in source type except for half-float which should be 
    //   converted to float before passing to filter.
    // - reduce the band-count as early as possible
    int iOpBands = VtMin(imgSrc.Bands(), pimgDst->Bands());
    int iOpType  = EL_FORMAT(imgSrc.GetType());
    if( iOpType == EL_FORMAT_HALF_FLOAT )
    {
        iOpType = EL_FORMAT_FLOAT;
    }

    // create an intermediate float block
    CFloatImg imgTmp;
    VT_HR_EXIT( imgTmp.Create(pimgDst->Height(), imgSrc.Width(), iOpBands, 
                              align64ByteRows) );

    // determine the required source block
    vt::CRect rctSrc    = vt::CRect(imgSrc.Rect()) + ptSrc;
    vt::CRect rctReqSrc = GetRequiredSrcRect(rctDst);
    VT_ASSERT( rctSrc.RectInRect(&rctReqSrc) );
    int iBeginX = rctDst.left - ptSrc.x;
    int iBeginY = rctDst.top  - ptSrc.y;

    // filter vertically using the vertical kernel and transpose
    const C1dKernel& kernelVert = m_kernelsetVert.GetKernel(0);
    C1dKernel& kernelVertScl    = m_kernelsetVertScl.GetKernel(0);
    memcpy( kernelVertScl.Ptr(), kernelVert.Ptr(), 
            kernelVertScl.Width()*sizeof(float) );
    CImg imgConv;
    if( EL_FORMAT(imgSrc.GetType()) != iOpType || imgSrc.Bands() != iOpBands )
    {
        // convert the source block if necessary
        VT_HR_EXIT( imgConv.Create(imgSrc.Width(), imgSrc.Height(),
								   VT_IMG_MAKE_TYPE(iOpType, iOpBands),
                                   align64ByteRows) );
        VT_HR_EXIT( VtConvertImage(imgConv, imgSrc) );

        ConvolveVerticalSingleKernelTranspose(imgTmp, imgConv, kernelVertScl,
                                              iBeginY, 0);
    }
    else
    {
        ConvolveVerticalSingleKernelTranspose(imgTmp, imgSrc, kernelVertScl, 
                                              iBeginY, 0);
    }        

    // filter vertically using the horizontal kernel and transpose
    const C1dKernel& kernelHoriz = m_kernelsetHoriz.GetKernel(0);
    C1dKernel& kernelHorizScl    = m_kernelsetHorizScl.GetKernel(0);

    memcpy( kernelHorizScl.Ptr(), kernelHoriz.Ptr(), 
            kernelHorizScl.Width()*sizeof(float) );
    if( iOpBands != pimgDst->Bands() || 
        EL_FORMAT(pimgDst->GetType())==EL_FORMAT_HALF_FLOAT )
    {
        // sometimes a temp conversion buffer is needed
        int iConvType = EL_FORMAT(pimgDst->GetType());
        if( iConvType == EL_FORMAT_HALF_FLOAT )
        {
            iConvType = EL_FORMAT_FLOAT;
        }

        VT_HR_EXIT( imgConv.Create(pimgDst->Width(), pimgDst->Height(),
								   VT_IMG_MAKE_TYPE(iConvType, iOpBands),
                                   align64ByteRows) );
        ConvolveVerticalSingleKernelTranspose(imgConv, imgTmp, kernelHorizScl, 
                                              iBeginX, 1);
        VT_HR_EXIT( VtConvertImage(*pimgDst, imgConv) );
    }
    else
    {
        ConvolveVerticalSingleKernelTranspose(*pimgDst, imgTmp, kernelHorizScl, 
                                              iBeginX, 1);
    }

    VT_HR_END()
}

HRESULT
CSeparableFilterTransform::TransformMultiKernel(
    CImg* pimgDst, IN  const CRect& rctDst, const CImg& imgSrc, const CPoint& ptSrc)
{
    // TODO: use the memory pool once it is in place

    VT_ASSERT( !imgSrc.IsSharingMemory(*pimgDst) );

    VT_HR_BEGIN()

    // determine the "operation" type.  
    // - always operate in source type except for half-float which should be 
    //   converted to float before passing to filter.
    // - reduce the band-count as early as possible
    int iSrcType = EL_FORMAT(imgSrc.GetType());
    int iDstType = EL_FORMAT(pimgDst->GetType());
    int iOpBands = VtMin(imgSrc.Bands(), pimgDst->Bands());
    int iOpType  = EL_FORMAT(imgSrc.GetType());
    if( iOpType == EL_FORMAT_HALF_FLOAT )
    {
        iOpType = EL_FORMAT_FLOAT;
    }

    // generate a scaled vertical kernel set to perform the type conversion
    // scale inline with the filtering
    float fMaxSrcVal = 
        (iSrcType==EL_FORMAT_BYTE)?  float(ElTraits<Byte>::MaxVal()): 
        (iSrcType==EL_FORMAT_SHORT)? float(ElTraits<UInt16>::MaxVal()): 
        ElTraits<float>::MaxVal();

    if( iOpBands != 1 || iOpBands != pimgDst->Bands() || 
        EL_FORMAT(pimgDst->GetType())==EL_FORMAT_HALF_FLOAT )
    {
        ScaleKernelSet(m_kernelsetVertScl, m_kernelsetVert, 1.f/fMaxSrcVal);
    }
    else
    {
        float fMaxDstVal = 
            (iDstType==EL_FORMAT_BYTE)?  float(ElTraits<Byte>::MaxVal()): 
            (iDstType==EL_FORMAT_SHORT)? float(ElTraits<UInt16>::MaxVal()): 
            ElTraits<float>::MaxVal();
            ScaleKernelSet(m_kernelsetVertScl, m_kernelsetVert, fMaxDstVal/fMaxSrcVal);
    }

    // ASSERT that enough source data was provided
    vt::CRect rctSrc    = vt::CRect(imgSrc.Rect()) + ptSrc;
    vt::CRect rctReqSrc = GetRequiredSrcRect(rctDst);
    VT_ASSERT( rctSrc.RectInRect(&rctReqSrc) );

    CImg imgConv;
    if( EL_FORMAT(imgSrc.GetType()) == iOpType && imgSrc.Bands() == iOpBands )
    {
        imgSrc.Share(imgConv);
    }
    else
    {
        // if the source is different from the 'optype' then create a second
        // temp buffer to convert the source into.  currently this will only
        // happen when the source is HALF_FLOAT
        VT_HR_EXIT( imgConv.Create(imgSrc.Width(), imgSrc.Height(),
								   VT_IMG_MAKE_TYPE(iOpType, iOpBands),
                                   align64ByteRows) );
        VT_HR_EXIT( VtConvertImage(imgConv, imgSrc) );
    }

    if( iOpBands == 1 )
    {
        // create an intermediate float block
        CFloatImg imgTmp;
        VT_HR_EXIT( imgTmp.Create(pimgDst->Height(), imgSrc.Width(),
                                  iOpBands, align64ByteRows) );

        // filter vertically
        UInt32 uKernIndex;
        int  iSrcOffset;
        GetKernelSetPosition(uKernIndex, iSrcOffset, rctDst.top, m_kernelsetVert);
        ConvolveVerticalTransposeOneBand(imgTmp, imgConv, m_kernelsetVertScl,
                                         uKernIndex, iSrcOffset - ptSrc.y, 0);

        // filter horizontally
        GetKernelSetPosition(uKernIndex, iSrcOffset, rctDst.left, m_kernelsetHoriz);
        if( iOpBands != pimgDst->Bands() || 
            EL_FORMAT(pimgDst->GetType())==EL_FORMAT_HALF_FLOAT )
        {
            // sometimes a temp conversion buffer is needed
            int iConvType = EL_FORMAT(pimgDst->GetType());
            if( iConvType == EL_FORMAT_HALF_FLOAT )
            {
                iConvType = EL_FORMAT_FLOAT;
            }

            VT_HR_EXIT( imgConv.Create(pimgDst->Width(), pimgDst->Height(),
									   VT_IMG_MAKE_TYPE(iConvType, iOpBands),
                                       align64ByteRows) );
            ConvolveVerticalTransposeOneBand(imgConv, imgTmp, m_kernelsetHoriz,
                                             uKernIndex, iSrcOffset - ptSrc.x, 1);
            VT_HR_EXIT( VtConvertImage(*pimgDst, imgConv) );
        }
        else
        {
            ConvolveVerticalTransposeOneBand(*pimgDst, imgTmp, m_kernelsetHoriz,
                                             uKernIndex, iSrcOffset - ptSrc.x, 1);
        }
    }
    else
    {
        // create an intermediate float block
        CFloatImg imgTmp;
        VT_HR_EXIT( imgTmp.Create(imgSrc.Width(), pimgDst->Height(), iOpBands, 
                                  align64ByteRows) );
        // filter vertically
        UInt32 uKernIndex;
        int  iSrcOffset;
        GetKernelSetPosition(uKernIndex, iSrcOffset, rctDst.top, m_kernelsetVert);    
        ConvolveVertical(imgTmp, imgConv, m_kernelsetVertScl, uKernIndex, 
                         iSrcOffset-ptSrc.y);

        // filter horizontally
        GetKernelSetPosition(uKernIndex, iSrcOffset, rctDst.left, m_kernelsetHoriz);
        ConvolveHorizontal(*pimgDst, imgTmp, m_kernelsetHoriz, uKernIndex,
                           iSrcOffset-ptSrc.x);
    }

    VT_HR_END()
}

#endif