//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Separable filter specialization for box kernel
//
//  History:
//      2012/7/5-v-mitoel
//          Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_image.h"
#include "vt_utils.h"
#include "vt_kernel.h"
#include "vt_separablefilter.h"
#include "vt_warp.h"
#include "vt_convert.h"
#include "vt_convert.inl"
#include "vt_pixeliterators.h"
#include "vt_pad.h"

using namespace vt;

//+-----------------------------------------------------------------------------
//
// x86 Processor Type Specialization Functions
//
//------------------------------------------------------------------------------
#if (defined(_M_IX86) || defined(_M_AMD64))

// 2:1
template<__m128 (*pfnLoad)(const float*)>
static int BoxFilter2to1_1BandByte_SSE2( Byte* pDst, int dstw,
                                          const Byte* pSrcT, const Byte* pSrcB )
{
    int i = dstw & ~7;
    // mask to extract every other (even index) pixel using _mm_shuffle_epi8
    __m128i mask = _mm_set_epi8(-1,-1,-1,-1,-1,-1,-1,-1,14,12,10,8,6,4,2,0);
    while (dstw >= 8)
    {
        __m128i x0,x1;
        x0 = _mm_castps_si128(pfnLoad( (const float*)pSrcT )); pSrcT += 16;
        x1 = _mm_castps_si128(pfnLoad( (const float*)pSrcB )); pSrcB += 16;
        x0 = _mm_avg_epu8( x0, x1 ); // T+B
        x1 = _mm_srli_si128( x0, 1); // align to sum odd index values with even
        x0 = _mm_avg_epu8( x0, x1 ); // 
        x1 = _mm_shuffle_epi8( x0, mask );
        _mm_storel_epi64( (__m128i*)pDst, x1 ); pDst += 8;
        dstw -= 8;
    }
    return i;
}

template<__m128 (*pfnLoad)(const float*), void (*pfnStore)(float*, const __m128&)>
static int BoxFilter2to1_1BandFloat_SSE2( float* pDst, int pixcount,
                                          const float* pSrcT, const float* pSrcB )
{
    int i = pixcount & ~3;
    __m128 constScale = _mm_set1_ps(0.25f);
    for( int j = 0; j < i; j+=4, pSrcT+=8, pSrcB+=8, pDst+=4 )
    {
        __m128 x0 = pfnLoad(pSrcT);     // TL0,TR0,TL1,TR1
        __m128 x1 = pfnLoad(pSrcT+4);   // TL2,TR2,TL3,TR3
        __m128 x2 = _mm_shuffle_ps(x0,x1,_MM_SHUFFLE(2,0,2,0)); // TL0,TL1,TL2,TL3
        __m128 x3 = _mm_shuffle_ps(x0,x1,_MM_SHUFFLE(3,1,3,1)); // TR0,TR1,TR2,TR3
        x0 = pfnLoad(pSrcB);     // BL0,BR0,BL1,BR1
        x1 = pfnLoad(pSrcB+4);   // BL2,BR2,BL3,BR3
        __m128 x4 = _mm_shuffle_ps(x0,x1,_MM_SHUFFLE(2,0,2,0)); // BL0,BL1,BL2,BL3
        __m128 x5 = _mm_shuffle_ps(x0,x1,_MM_SHUFFLE(3,1,3,1)); // BR0,BR1,BR2,BR3
        x0 = _mm_add_ps(x2,x3);
        x0 = _mm_add_ps(x0,x4);
        x0 = _mm_add_ps(x0,x5); // sum of TL,TR,BL,BR
        x0 = _mm_mul_ps(x0,constScale); // scale by .25f
        pfnStore(pDst,x0);
    }
    return i;
}

#if defined(_MSC_VER)
template<__m256 (*pfnLoad)(const float*), void (*pfnStore)(float*, const __m256&)>
static int BoxFilter2to1_1BandFloat_AVX( float* pDst, int pixcount,
                                         const float* pSrcT, const float* pSrcB )
{
    int i = pixcount & ~7;
    __m256 constScale = _mm256_set1_ps(0.25f);
    for( int j = 0; j < i; j+=8, pSrcT+=16, pSrcB+=16, pDst+=8 )
    {
        __m256 x0 = pfnLoad(pSrcT);     // TL0,TR0,TL1,TR1,TL2,TR2,TL3,TR3
        __m256 x1 = pfnLoad(pSrcT+8);   // TL4,TR4,TL5,TR5,TL6,TR6,TL7,TR7
        __m256 x2 = _mm256_hadd_ps(x0,x1); // {TL0+TR0,TL1+TR1,TL4+TR4,TL5+TR5,TL2+TR2,TL3+TR3,TL6+TR6,TL7+TR7}
        x0 = pfnLoad(pSrcB);     // BL0,BR0,BL1,BR1,BL2,BR2,BL3,BR3
        x1 = pfnLoad(pSrcB+8);   // BL4,BR4,BL5,BR5,BL6,BR6,BL7,BR7
        __m256 x3 = _mm256_hadd_ps(x0,x1); // {BL0+BR0,BL1+BR1,BL4+BR4,BL5+BR5,BL2+BR2,BL3+BR3,BL6+BR6,BL7+BR7}
        x0 = _mm256_add_ps(x2,x3); // sum of TL,TR,BL,BR in order 0,1,4,5,2,3,6,7
        x0 = _mm256_mul_ps(x0,constScale); // scale by .25f
        x1 = _mm256_permute2f128_ps(x0,x0,1); // R2,R3,R6,R7|R0,R1,R4,R5
        x1 = _mm256_shuffle_ps(x1,x1,_MM_SHUFFLE(1,0,3,2)); // xx,xx,R2,R3|R4,R5,xx,xx
        x0 = _mm256_blend_ps(x0,x1,0x3c); // R0,R1,R2,R3|R4,R5,R6,R7
        pfnStore(pDst,x0);
    }
    return i;
}
#endif

// 4:1
template<__m128 (*pfnLoad)(const float*), void (*pfnStore)(float*, const __m128&)>
static int BoxFilter4to1_1BandByte_SSSE3( Byte* pDst, int pixcount,
                                          const Byte* pSrc0, const Byte* pSrc1,
                                          const Byte* pSrc2, const Byte* pSrc3 )
{
    int i = pixcount & ~7;
    __m128i zero = _mm_setzero_si128();
    for( int j = 0; j < i; j+=8, pSrc0+=32, pSrc1+=32, pSrc2+=32, pSrc3+=32, pDst+=8 )
    {
        __m128i x0,x1,x2,x3,x4,x5,x6;

        x0 = _mm_castps_si128(pfnLoad((const float*)pSrc0));// [s00..15]
        x1 = _mm_castps_si128(pfnLoad((const float*)(pSrc0+16))); // [s16..31]
        x2 = _mm_unpacklo_epi8(x0,zero); // s00..s07
        x3 = _mm_unpackhi_epi8(x0,zero); // s08..s15
        x4 = _mm_unpacklo_epi8(x1,zero); // s16..s23
        x5 = _mm_unpackhi_epi8(x1,zero); // s24..s31
        x2 = _mm_hadd_epi16(x2,x3);     // (s00+s01)..(s14+s15)
        x3 = _mm_hadd_epi16(x4,x5);     // (s16+s17)..(s30+s31)
        x6 = _mm_hadd_epi16(x2,x3);     // (s00+s01+s02+s03)..(s28+s29+s30+s31)

        x0 = _mm_castps_si128(pfnLoad((const float*)pSrc1));// [s00..15]
        x1 = _mm_castps_si128(pfnLoad((const float*)(pSrc1+16))); // [s16..31]
        x2 = _mm_unpacklo_epi8(x0,zero); // s00..s07
        x3 = _mm_unpackhi_epi8(x0,zero); // s08..s15
        x4 = _mm_unpacklo_epi8(x1,zero); // s16..s23
        x5 = _mm_unpackhi_epi8(x1,zero); // s24..s31
        x2 = _mm_hadd_epi16(x2,x3);     // (s00+s01)..(s14+s15)
        x3 = _mm_hadd_epi16(x4,x5);     // (s16+s17)..(s30+s31)
        x5 = _mm_hadd_epi16(x2,x3);     // (s00+s01+s02+s03)..(s28+s29+s30+s31)
        x6 = _mm_add_epi16(x6,x5);      // column sum

        x0 = _mm_castps_si128(pfnLoad((const float*)pSrc2));// [s00..15]
        x1 = _mm_castps_si128(pfnLoad((const float*)(pSrc2+16))); // [s16..31]
        x2 = _mm_unpacklo_epi8(x0,zero); // s00..s07
        x3 = _mm_unpackhi_epi8(x0,zero); // s08..s15
        x4 = _mm_unpacklo_epi8(x1,zero); // s16..s23
        x5 = _mm_unpackhi_epi8(x1,zero); // s24..s31
        x2 = _mm_hadd_epi16(x2,x3);     // (s00+s01)..(s14+s15)
        x3 = _mm_hadd_epi16(x4,x5);     // (s16+s17)..(s30+s31)
        x5 = _mm_hadd_epi16(x2,x3);     // (s00+s01+s02+s03)..(s28+s29+s30+s31)
        x6 = _mm_add_epi16(x6,x5);      // column sum

        x0 = _mm_castps_si128(pfnLoad((const float*)pSrc3));// [s00..15]
        x1 = _mm_castps_si128(pfnLoad((const float*)(pSrc3+16))); // [s16..31]
        x2 = _mm_unpacklo_epi8(x0,zero); // s00..s07
        x3 = _mm_unpackhi_epi8(x0,zero); // s08..s15
        x4 = _mm_unpacklo_epi8(x1,zero); // s16..s23
        x5 = _mm_unpackhi_epi8(x1,zero); // s24..s31
        x2 = _mm_hadd_epi16(x2,x3);     // (s00+s01)..(s14+s15)
        x3 = _mm_hadd_epi16(x4,x5);     // (s16+s17)..(s30+s31)
        x5 = _mm_hadd_epi16(x2,x3);     // (s00+s01+s02+s03)..(s28+s29+s30+s31)
        x6 = _mm_add_epi16(x6,x5);      // column sum

        x1 = _mm_srli_epi16(x6,4);      //
        x0 = _mm_packus_epi16(x1,x1);
        _mm_storel_epi64((__m128i*)pDst,x0);
    }
    return i;
}

template<__m128 (*pfnLoad)(const float*), void (*pfnStore)(float*, const __m128&)>
static int BoxFilter4to1_1BandFloat_SSE2( float* pDst, int pixcount,
                                          const float* pSrc0, const float* pSrc1,
                                          const float* pSrc2, const float* pSrc3 )
{
    int i = pixcount & ~3;
    __m128 constScale = _mm_set1_ps(0.0625f);
    for( int j = 0; j < i; j+=4, pSrc0+=16, pSrc1+=16, pSrc2+=16, pSrc3+=16, pDst+=4 )
    {
        __m128 x0 = pfnLoad(pSrc0+0); 
        __m128 x1 = pfnLoad(pSrc0+4); 
        __m128 x2 = pfnLoad(pSrc0+8); 
        __m128 x3 = pfnLoad(pSrc0+12);
        __m128 x4;
        x4 = pfnLoad(pSrc1+0);  x0 = _mm_add_ps(x0,x4);
        x4 = pfnLoad(pSrc1+4);  x1 = _mm_add_ps(x1,x4);
        x4 = pfnLoad(pSrc1+8);  x2 = _mm_add_ps(x2,x4);
        x4 = pfnLoad(pSrc1+12); x3 = _mm_add_ps(x3,x4);
        x4 = pfnLoad(pSrc2+0);  x0 = _mm_add_ps(x0,x4);
        x4 = pfnLoad(pSrc2+4);  x1 = _mm_add_ps(x1,x4);
        x4 = pfnLoad(pSrc2+8);  x2 = _mm_add_ps(x2,x4);
        x4 = pfnLoad(pSrc2+12); x3 = _mm_add_ps(x3,x4);
        x4 = pfnLoad(pSrc3+0);  x0 = _mm_add_ps(x0,x4);
        x4 = pfnLoad(pSrc3+4);  x1 = _mm_add_ps(x1,x4);
        x4 = pfnLoad(pSrc3+8);  x2 = _mm_add_ps(x2,x4);
        x4 = pfnLoad(pSrc3+12); x3 = _mm_add_ps(x3,x4);
        // x0..x3 hold 16 vertically summed samples A,B,C,D|E,F,G,H|I,J,K,L|M,N,O,P
        _MM_TRANSPOSE4_PS(x0,x1,x2,x3);
        // x0..x3 hold 16 deinterleaved samples A,E,I,M|...

        x0 = _mm_add_ps(x0,x1);
        x0 = _mm_add_ps(x0,x2);
        x0 = _mm_add_ps(x0,x3); // 4x sum of 16 samples
        x0 = _mm_mul_ps(x0,constScale); // scale by .0625f
        pfnStore(pDst,x0);
    }
    return i;
}

#if defined(_MSC_VER)
template<__m256 (*pfnLoad)(const float*), void (*pfnStore)(float*, const __m256&)>
static int BoxFilter4to1_1BandFloat_AVX( float* pDst, int pixcount,
                                         const float* pSrc0, const float* pSrc1,
                                         const float* pSrc2, const float* pSrc3 )
{
    int i = pixcount & ~7;
    __m256 constScale = _mm256_set1_ps(0.0625f);
    for( int j = 0; j < i; j+=8, pSrc0+=32, pSrc1+=32, pSrc2+=32, pSrc3+=32, pDst+=8 )
    {
        __m256 x0 = pfnLoad(pSrc0+0); 
        __m256 x1 = pfnLoad(pSrc0+8); 
        __m256 x2 = pfnLoad(pSrc0+16); 
        __m256 x3 = pfnLoad(pSrc0+24);
        __m256 x4;
        x4 = pfnLoad(pSrc1+0);  x0 = _mm256_add_ps(x0,x4);
        x4 = pfnLoad(pSrc1+8);  x1 = _mm256_add_ps(x1,x4);
        x4 = pfnLoad(pSrc1+16); x2 = _mm256_add_ps(x2,x4);
        x4 = pfnLoad(pSrc1+24); x3 = _mm256_add_ps(x3,x4);
        x4 = pfnLoad(pSrc2+0);  x0 = _mm256_add_ps(x0,x4);
        x4 = pfnLoad(pSrc2+8);  x1 = _mm256_add_ps(x1,x4);
        x4 = pfnLoad(pSrc2+16); x2 = _mm256_add_ps(x2,x4);
        x4 = pfnLoad(pSrc2+24); x3 = _mm256_add_ps(x3,x4);
        x4 = pfnLoad(pSrc3+0);  x0 = _mm256_add_ps(x0,x4);
        x4 = pfnLoad(pSrc3+8);  x1 = _mm256_add_ps(x1,x4);
        x4 = pfnLoad(pSrc3+16); x2 = _mm256_add_ps(x2,x4);
        x4 = pfnLoad(pSrc3+24); x3 = _mm256_add_ps(x3,x4);
        // x0..x3 hold 32 vertically summed samples 
        //  |0,1,2,3,4,5,6,7|8,9,a,b,c,d,e,f|10,11,12,13,14,15,16,17|18,19,1a,1b,1c,1d,1e,1f|

        x4 = _mm256_hadd_ps(x0,x1); // 0+1,2+3,8+9,a+b,4+5,6+7,c+d,e+f
        __m256 x5 = _mm256_hadd_ps(x2,x3); // 10+11,12+13,18+19,1a+1b,14+15,16+17,1c+1d,1e+1f
        __m256 x6 = _mm256_hadd_ps(x4,x5); // |0..,8..,10..,18..,4..,c..,14..,1c..| = |0,2,4,6,1,3,5,7|

        x5 = _mm256_permute2f128_ps(x6,x6,1); // |1,3,5,7, 0,2,4,6|

        // control word 0bxx01xx00 low 4
        //              0b11xx10xx high 4
        //              0b11011000
        x0 = _mm256_permute_ps(x6,0xd8); // 0,x,2,x,x,5,x,7
        // control word 0b01xx00xx low 4
        //              0bxx11xx10 high 4
        //              0b01110010
        x1 = _mm256_permute_ps(x5,0x72); // x,1,x,3,4,x,6,x
        // control word 0b01011010 = 0x5a
        x0 = _mm256_blend_ps(x0,x1,0x5a); // R0,R1,R2,R3|R4,R5,R6,R7

        x0 = _mm256_mul_ps(x0,constScale); // scale by .0625f
        pfnStore(pDst,x0);
    }
    return i;
}
#endif

// this is actually slightly faster than the AVX version...
template<__m128 (*pfnLoad)(const float*), void (*pfnStore)(float*, const __m128&)>
static int BoxFilter4to1_1BandFloat_SSSE3( float* pDst, int pixcount,
                                          const float* pSrc0, const float* pSrc1,
                                          const float* pSrc2, const float* pSrc3 )
{
    int i = pixcount & ~3;
    __m128 constScale = _mm_set1_ps(0.0625f);
    for( int j = 0; j < i; j+=4, pSrc0+=16, pSrc1+=16, pSrc2+=16, pSrc3+=16, pDst+=4 )
    {
        __m128 x0 = pfnLoad(pSrc0+0); 
        __m128 x1 = pfnLoad(pSrc0+4); 
        __m128 x2 = pfnLoad(pSrc0+8); 
        __m128 x3 = pfnLoad(pSrc0+12);
        __m128 x4;
        x4 = pfnLoad(pSrc1+0);  x0 = _mm_add_ps(x0,x4);
        x4 = pfnLoad(pSrc1+4);  x1 = _mm_add_ps(x1,x4);
        x4 = pfnLoad(pSrc1+8);  x2 = _mm_add_ps(x2,x4);
        x4 = pfnLoad(pSrc1+12); x3 = _mm_add_ps(x3,x4);
        x4 = pfnLoad(pSrc2+0);  x0 = _mm_add_ps(x0,x4);
        x4 = pfnLoad(pSrc2+4);  x1 = _mm_add_ps(x1,x4);
        x4 = pfnLoad(pSrc2+8);  x2 = _mm_add_ps(x2,x4);
        x4 = pfnLoad(pSrc2+12); x3 = _mm_add_ps(x3,x4);
        x4 = pfnLoad(pSrc3+0);  x0 = _mm_add_ps(x0,x4);
        x4 = pfnLoad(pSrc3+4);  x1 = _mm_add_ps(x1,x4);
        x4 = pfnLoad(pSrc3+8);  x2 = _mm_add_ps(x2,x4);
        x4 = pfnLoad(pSrc3+12); x3 = _mm_add_ps(x3,x4);
        // x0..x3 hold 16 vertically summed samples A,B,C,D|E,F,G,H|I,J,K,L|M,N,O,P

        x0 = _mm_hadd_ps(x0,x1); // A+B,C+D,E+F,G+H
        x1 = _mm_hadd_ps(x2,x3); // I+J,K+L,M+N,O+P
        x0 = _mm_hadd_ps(x0,x1); // A+B+C+D,E..,I..,M..
        x0 = _mm_mul_ps(x0,constScale); // scale by .0625f
        pfnStore(pDst,x0);
    }
    return i;
}

#endif

//+-----------------------------------------------------------------------------
//
// Arm Processor Type Specialization Functions
//
//------------------------------------------------------------------------------
// TODO

//+-----------------------------------------------------------------------------
//
// External API Functions
//
//------------------------------------------------------------------------------
HRESULT 
vt::VtSeparableFilterBoxDecimate2to1(CImg& imgDst, const CRect& rctDst,
                                     const CImg& imgSrc, CPoint ptSrcOrigin)
{
    VT_HR_BEGIN()

    // position of first non-padding source pixel used for this dest rect
    CPoint npSrcOrigin = CPoint(
        (rctDst.left<<1)-ptSrcOrigin.x, 
        (rctDst.top<<1)-ptSrcOrigin.y);

    // this code requires that source pixels be present for the full dest rect
    VT_HR_EXIT( ((npSrcOrigin.x) < 0) ? E_INVALIDSRC : S_OK );
    VT_HR_EXIT( ((npSrcOrigin.y) < 0) ? E_INVALIDSRC : S_OK );
    VT_HR_EXIT( ((npSrcOrigin.x+(rctDst.Width()<<(1))) > imgSrc.Width()) ? E_INVALIDSRC : S_OK );
    VT_HR_EXIT( ((npSrcOrigin.y+(rctDst.Height()<<(1))) > imgSrc.Height()) ? E_INVALIDSRC : S_OK );

    VT_HR_EXIT( (!imgSrc.IsValid()) ? E_INVALIDSRC : S_OK );
    // create the destination image if it needs to be
    VT_HR_EXIT( CreateImageForTransform(imgDst, rctDst.Width(), rctDst.Height(),
                                        imgSrc.GetType()) );

    // if not supported, call to the fastest generic equivalent (which for
    // this case is Resize)
    int srcT = imgSrc.GetType();
    int dstT = imgDst.GetType();
    if ( !VT_IMG_SAME_BE(srcT, dstT) ||
         (imgSrc.Bands() > 1) ||
         ((EL_FORMAT(srcT) != EL_FORMAT_BYTE) &&
          (EL_FORMAT(srcT) != EL_FORMAT_FLOAT)) )
    {
        CImg imgSrcO;
        CRect rctSrcO = imgSrc.Rect();
        rctSrcO.left += ptSrcOrigin.x;
        rctSrcO.top += ptSrcOrigin.y;
        imgSrc.Share(imgSrcO,rctSrcO);
        // TODO: would be better to call to an internal function to avoid potential
        // infinite loop since VtResizeImage can call here under some conditions
        return VtResizeImage(imgDst, rctDst, imgSrcO,2.f,.5f,2.f,.5f,
            eSamplerKernelBilinear,IMAGE_EXTEND(Extend));
    }

    // loop over the destination blocks
    const UInt32 c_blocksize = 256;

    CImg imgSrcBlk;
    for( CBlockIterator bi(rctDst, c_blocksize); !bi.Done(); bi.Advance() )
    {
        // get blk rct in dst image coords
        vt::CRect rctDstBlk = bi.GetRect();

        CImg imgDstBlk;
        imgDst.Share(imgDstBlk, &rctDstBlk);

        // offset to layer coords
        rctDstBlk += rctDst.TopLeft();

        int iW = imgDstBlk.Width();

        int xs = 2*rctDstBlk.left;
        int ys = 2*rctDstBlk.top;
        bool bIsByte = (EL_FORMAT(srcT) == EL_FORMAT_BYTE);
        for( int y = 0; y < rctDstBlk.Height(); y++, ys+=2 )
        {
            if (bIsByte)
            {
                Byte* pDst = imgDstBlk.BytePtr(0,y);
                const Byte* pSrcT = imgSrc.BytePtr(xs, VtMax((int)imgSrc.Rect().top,VtMin((int)imgSrc.Rect().bottom,ys)));
                const Byte* pSrcB = imgSrc.BytePtr(xs, VtMax((int)imgSrc.Rect().top,VtMin((int)imgSrc.Rect().bottom,ys+1)));
                int i = 0;

#if (defined(_M_IX86) || defined(_M_AMD64))
                if( g_SupportSSE2() && IsAligned4(pDst) && IsAligned4(pSrcT) && IsAligned4(pSrcB))
                {
                    SelectPSTFunc(IsAligned16(pSrcT)&&IsAligned16(pSrcB),
                        i, Load4, BoxFilter2to1_1BandByte_SSE2, pDst, iW, pSrcT, pSrcB);
                }
#endif
                // do vector remainder
                for( ; i<iW; i++ )
                {
                    __int64 offset = static_cast<__int64>(i) << 1;
                    int avgT = ( (int)*(pSrcT+(offset)) + (int)*(pSrcT+(offset)+1) ) >> 1;
                    int avgB = ( (int)*(pSrcB+(offset)) + (int)*(pSrcB+(offset)+1) ) >> 1;
                    *(pDst+i) = (Byte)( (avgT + avgB) >> 1);
                }
            }
            else
            {
                float* pDst = (float*)imgDstBlk.BytePtr(0,y);
                float* pSrcT = (float*)imgSrc.BytePtr(xs, VtMax((int)imgSrc.Rect().top,VtMin((int)imgSrc.Rect().bottom,ys)));
                float* pSrcB = (float*)imgSrc.BytePtr(xs, VtMax((int)imgSrc.Rect().top,VtMin((int)imgSrc.Rect().bottom,ys+1)));
                int i = 0;

#if (defined(_M_IX86) || defined(_M_AMD64))
#if defined(_MSC_VER)
                if( g_SupportAVX() )
                {
                    SelectPSTFunc2(IsAligned32(pDst)&&IsAligned32(pSrcT)&&IsAligned32(pSrcB),
                        i, Load8, Store8, BoxFilter2to1_1BandFloat_AVX, pDst, iW, pSrcT, pSrcB);
                }
                else
#endif
					if( g_SupportSSE2() )
                {
                    SelectPSTFunc2(IsAligned16(pDst)&&IsAligned16(pSrcT)&&IsAligned16(pSrcB),
                        i, Load4, Store4, BoxFilter2to1_1BandFloat_SSE2, pDst, iW, pSrcT, pSrcB);
                }
#endif
                // do vector remainder
                for( ; i<iW; i++ )
                {
                    __int64 offset = static_cast<__int64>(i) << 1;
                    *(pDst+i) = .25f*( *(pSrcT+(offset)) + *(pSrcT+(offset)+1) + 
                                       *(pSrcB+(offset)) + *(pSrcB+(offset)+1) );
                }
            }
        }
    }

    VT_HR_END()
}

HRESULT 
vt::VtSeparableFilterBoxDecimate4to1(CLumaByteImg& imgDst, const CRect& rctDst, 
                                     const CLumaByteImg& imgSrc, CPoint ptSrcOrigin)
{
    VT_HR_BEGIN()

    // position of first non-padding source pixel used for this dest rect
    CPoint npSrcOrigin = CPoint(
        (rctDst.left<<2)-ptSrcOrigin.x, 
        (rctDst.top<<2)-ptSrcOrigin.y);
    // this code requires that source pixels be present for the full dest rect
    VT_HR_EXIT( ((npSrcOrigin.x) < 0) ? E_INVALIDSRC : S_OK );
    VT_HR_EXIT( ((npSrcOrigin.y) < 0) ? E_INVALIDSRC : S_OK );
    VT_HR_EXIT( ((npSrcOrigin.x+(rctDst.Width()<<(2))) > imgSrc.Width()) ? E_INVALIDSRC : S_OK );
    VT_HR_EXIT( ((npSrcOrigin.y+(rctDst.Height()<<(2))) > imgSrc.Height()) ? E_INVALIDSRC : S_OK );

    VT_HR_EXIT( (!imgSrc.IsValid()) ? E_INVALIDSRC : S_OK );
    // create the destination image if it needs to be
    VT_HR_EXIT( CreateImageForTransform(imgDst, rctDst.Width(), rctDst.Height(),
                                        imgSrc.GetType()) );

    // loop over the destination blocks
    const UInt32 c_blocksize = 128;

    CImg imgSrcBlk;
    for( CBlockIterator bi(rctDst, c_blocksize); !bi.Done(); bi.Advance() )
    {
        // get blk rct in dst image coords
        vt::CRect rctDstBlk = bi.GetRect();

        CImg imgDstBlk;
        imgDst.Share(imgDstBlk, &rctDstBlk);

        // offset to layer coords
        rctDstBlk += rctDst.TopLeft();

        int iW = imgDstBlk.Width();

        int xs = rctDstBlk.left<<2;
        int ys = rctDstBlk.top<<2;
        for( int y = 0; y < rctDstBlk.Height(); y++, ys+=4 )
        {
            Byte* pDst = imgDstBlk.BytePtr(0,y);
            const Byte* pSrc0 = imgSrc.BytePtr(xs, VtMax((int)imgSrc.Rect().top,VtMin((int)imgSrc.Rect().bottom,ys+0)));
            const Byte* pSrc1 = imgSrc.BytePtr(xs, VtMax((int)imgSrc.Rect().top,VtMin((int)imgSrc.Rect().bottom,ys+1)));
            const Byte* pSrc2 = imgSrc.BytePtr(xs, VtMax((int)imgSrc.Rect().top,VtMin((int)imgSrc.Rect().bottom,ys+2)));
            const Byte* pSrc3 = imgSrc.BytePtr(xs, VtMax((int)imgSrc.Rect().top,VtMin((int)imgSrc.Rect().bottom,ys+3)));
            int i = 0;

#if (defined(_M_IX86) || defined(_M_AMD64))
            if( g_SupportSSSE3() && IsAligned4(pDst) && IsAligned4(pSrc0) && ((imgSrc.StrideBytes()&3)==0) )
            {
                SelectPSTFunc2(IsAligned16(pSrc0)&&((imgSrc.StrideBytes()&15)==0),
                    i, Load4, Store4, BoxFilter4to1_1BandByte_SSSE3, pDst, iW, pSrc0, pSrc1, pSrc2, pSrc3);
            }
#endif
            // do vector remainder
            for( ; i<iW; i++ )
            {
                __int64 offset = static_cast<__int64>(i) << 2;
                int row0 = (int)*(pSrc0+(offset)+0) +
                           (int)*(pSrc0+(offset)+1) +
                           (int)*(pSrc0+(offset)+2) +
                           (int)*(pSrc0+(offset)+3);
                int row1 = (int)*(pSrc1+(offset)+0) +
                           (int)*(pSrc1+(offset)+1) +
                           (int)*(pSrc1+(offset)+2) +
                           (int)*(pSrc1+(offset)+3);
                int row2 = (int)*(pSrc2+(offset)+0) +
                           (int)*(pSrc2+(offset)+1) +
                           (int)*(pSrc2+(offset)+2) +
                           (int)*(pSrc2+(offset)+3);
                int row3 = (int)*(pSrc3+(offset)+0) +
                           (int)*(pSrc3+(offset)+1) +
                           (int)*(pSrc3+(offset)+2) +
                           (int)*(pSrc3+(offset)+3);
                *(pDst+i) = (Byte)( (row0+row1+row2+row3) >> 4 );
            }
        }
    }

    VT_HR_END()
}

HRESULT 
vt::VtSeparableFilterBoxDecimate4to1(CLumaFloatImg& imgDst, const CRect& rctDst, 
                                     const CLumaFloatImg& imgSrc, CPoint ptSrcOrigin)
{
    VT_HR_BEGIN()

    // position of first non-padding source pixel used for this dest rect
    CPoint npSrcOrigin = CPoint(
        (rctDst.left<<2)-ptSrcOrigin.x, 
        (rctDst.top<<2)-ptSrcOrigin.y);
    // this code requires that source pixels be present for the full dest rect
    VT_HR_EXIT( ((npSrcOrigin.x) < 0) ? E_INVALIDSRC : S_OK );
    VT_HR_EXIT( ((npSrcOrigin.y) < 0) ? E_INVALIDSRC : S_OK );
    VT_HR_EXIT( ((npSrcOrigin.x+(rctDst.Width()<<(2))) > imgSrc.Width()) ? E_INVALIDSRC : S_OK );
    VT_HR_EXIT( ((npSrcOrigin.y+(rctDst.Height()<<(2))) > imgSrc.Height()) ? E_INVALIDSRC : S_OK );

    VT_HR_EXIT( (!imgSrc.IsValid()) ? E_INVALIDSRC : S_OK );
    // create the destination image if it needs to be
    VT_HR_EXIT( CreateImageForTransform(imgDst, rctDst.Width(), rctDst.Height(),
                                        imgSrc.GetType()) );

    // loop over the destination blocks
    const UInt32 c_blocksize = 128;

    CImg imgSrcBlk;
    for( CBlockIterator bi(rctDst, c_blocksize); !bi.Done(); bi.Advance() )
    {
        // get blk rct in dst image coords
        vt::CRect rctDstBlk = bi.GetRect();

        CImg imgDstBlk;
        imgDst.Share(imgDstBlk, &rctDstBlk);

        // offset to layer coords
        rctDstBlk += rctDst.TopLeft();

        int iW = imgDstBlk.Width();

        int xs = rctDstBlk.left<<2;
        int ys = rctDstBlk.top<<2;
        for( int y = 0; y < rctDstBlk.Height(); y++, ys+=4 )
        {
            float* pDst = (float*)imgDstBlk.BytePtr(0,y);
            float* pSrc0 = (float*)imgSrc.BytePtr(xs, VtMax((int)imgSrc.Rect().top,VtMin((int)imgSrc.Rect().bottom,ys+0)));
            float* pSrc1 = (float*)imgSrc.BytePtr(xs, VtMax((int)imgSrc.Rect().top,VtMin((int)imgSrc.Rect().bottom,ys+1)));
            float* pSrc2 = (float*)imgSrc.BytePtr(xs, VtMax((int)imgSrc.Rect().top,VtMin((int)imgSrc.Rect().bottom,ys+2)));
            float* pSrc3 = (float*)imgSrc.BytePtr(xs, VtMax((int)imgSrc.Rect().top,VtMin((int)imgSrc.Rect().bottom,ys+3)));
            int i = 0;

#if (defined(_M_IX86) || defined(_M_AMD64))
            if( g_SupportSSSE3() )
            {
                SelectPSTFunc2(IsAligned16(pDst)&&IsAligned16(pSrc0)&&((imgSrc.StrideBytes()&15)==0),
                    i, Load4, Store4, BoxFilter4to1_1BandFloat_SSSE3, pDst, iW, pSrc0, pSrc1, pSrc2, pSrc3);
            }
            else if( g_SupportSSE2() )
            {
                SelectPSTFunc2(IsAligned16(pDst)&&IsAligned16(pSrc0)&&((imgSrc.StrideBytes()&15)==0),
                    i, Load4, Store4, BoxFilter4to1_1BandFloat_SSE2, pDst, iW, pSrc0, pSrc1, pSrc2, pSrc3);
            }
#endif

            // do vector remainder
            for( ; i<iW; i++ )
            {
                __int64 offset = static_cast<__int64>(i) << 2;
                *(pDst+i) = .0625f*( 
                    *(pSrc0+(offset)+0) + *(pSrc0+(offset)+1) + *(pSrc0+(offset)+2) + *(pSrc0+(offset)+3) +
                    *(pSrc1+(offset)+0) + *(pSrc1+(offset)+1) + *(pSrc1+(offset)+2) + *(pSrc1+(offset)+3) +
                    *(pSrc2+(offset)+0) + *(pSrc2+(offset)+1) + *(pSrc2+(offset)+2) + *(pSrc2+(offset)+3) +
                    *(pSrc3+(offset)+0) + *(pSrc3+(offset)+1) + *(pSrc3+(offset)+2) + *(pSrc3+(offset)+3) );
            }
        }
    }

    VT_HR_END()
}

//+-----------------------------------------------------------------------------
