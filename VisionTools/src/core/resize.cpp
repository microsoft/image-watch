//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for image resizing
//
//  History:
//      2012/05/30
//          Split out from warp.cpp
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_pad.h"
#include "vt_convert.h"
#include "vt_warp.h"
#include "vt_imgmath.h"
#include "vt_imgmath.inl"
#include "vt_pixeliterators.h"
#include "vt_separablefilter.h"

using namespace vt;

// from warp.cpp
extern HRESULT BSplinePostProcess(CImg* pimgDst, 
                                  const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend));

//+----------------------------------------------------------------------------
//
// Byte source resize routines
//
//-----------------------------------------------------------------------------
int 
BilinearResizeHorizontalByte4_ProcSpecific( Byte* pDst, int pixcount, 
                                            const Byte* pSrc, const int* pCoords, 
                                            const UInt16* pCoefs)
{
#if defined(VT_SSE_SUPPORTED)
    if( !g_SupportSSE2() )
    {
        return 0;
    }

    // setup some constants
    __m128i x7 = _mm_setzero_si128 ();   // x7 = 0
    __m128i x6 = _mm_set1_epi16 (0x80);  // 0.5

    // generate 16 outputs per loop
    int x = 0;
    for( ; x < pixcount-3; x+=4, pDst+=16 )
    {
        // result 1
        const Byte* p0 = pSrc + *pCoords;
        __m128i x0 = _mm_loadl_epi64 ((__m128i*)p0);    // load 1st 8 pixels
        x0 = _mm_unpacklo_epi8( x0, x7 );               // conv to 16 bit
        __m128i x5 = _mm_load_si128 ((__m128i*)pCoefs); // load coefs
        x0 = _mm_mullo_epi16( x0, x5 );                 // x0 = pix * coef
        pCoords++; pCoefs+=8;

        // result 2
        p0 = pSrc + *pCoords;
        __m128i x1 = _mm_loadl_epi64 ((__m128i*)p0);    // load 2nd 8 pixels
        x1 = _mm_unpacklo_epi8( x1, x7 );               // conv to 16 bit
        x5 = _mm_load_si128 ((__m128i*)pCoefs);         // load coefs
        x1 = _mm_mullo_epi16( x1, x5 );                 // x1 = pix * coef

        x5 = x0;
        x5 = _mm_unpackhi_epi64( x5, x1 );   // line results up for add 
        x0 = _mm_unpacklo_epi64( x0, x1 );
        x0 = _mm_add_epi16(x0, x5);          // add, round, shift
        x0 = _mm_add_epi16(x0, x6);
        x0 = _mm_srli_epi16( x0, 8 );
        pCoords++; pCoefs+=8;

        // result 3
        p0 = pSrc + *pCoords;
        x1 = _mm_loadl_epi64 ((__m128i*)p0);     // load 3rd 8 pixels
        x1 = _mm_unpacklo_epi8( x1, x7 );        // conv to 16 bit
        x5 = _mm_load_si128 ((__m128i*)pCoefs);   // load coefs
        x1 = _mm_mullo_epi16( x1, x5 );          // x1 = pix * coef
        pCoords++; pCoefs+=8;

        // result 4
        p0 = pSrc + *pCoords;
        __m128i x2 = _mm_loadl_epi64 ((__m128i*)p0);    // load 2nd 8 pixels
        x2 = _mm_unpacklo_epi8( x2, x7 );               // conv to 16 bit
        x5 = _mm_load_si128 ((__m128i*)pCoefs);          // load coefs
        x2 = _mm_mullo_epi16( x2, x5 );                 // x1 = pix * coef

        x5 = x1;
        x5 = _mm_unpackhi_epi64( x5, x2 );   // line results up for add 
        x1 = _mm_unpacklo_epi64( x1, x2 );
        x1 = _mm_add_epi16(x1, x5);          // add, round, shift
        x1 = _mm_add_epi16(x1, x6);
        x1 = _mm_srli_epi16( x1, 8 );
        pCoords++; pCoefs+=8;

        x0 = _mm_packus_epi16(x0, x1);       // pack results

        _mm_store_si128((__m128i*)pDst, x0);
    }

    return x;
#else
	VT_USE(pDst);
	VT_USE(pixcount);
	VT_USE(pSrc);
	VT_USE(pCoords);
	VT_USE(pCoefs);
    return 0;
#endif
}

void BilinearResizeHorizontalByte4( Byte* pDst, int pixcount, const Byte* pSrc, 
                                    const int* pCoords, const UInt16* pCoefs)
{
    int x = BilinearResizeHorizontalByte4_ProcSpecific(pDst, pixcount, pSrc,
                                                       pCoords, pCoefs);
    pDst    += x*4;;
    pCoords += x;
    pCoefs  += x*8;

    // store any remaining pixels
    for( ; x < pixcount; x++, pDst+=4, pCoords++, pCoefs+=8 )
    {
        const Byte* p0 = pSrc + *pCoords;
        pDst[0] = (pCoefs[0] * p0[0] + pCoefs[4] * p0[4] + 0x80) >> 8;
        pDst[1] = (pCoefs[0] * p0[1] + pCoefs[4] * p0[5] + 0x80) >> 8;
        pDst[2] = (pCoefs[0] * p0[2] + pCoefs[4] * p0[6] + 0x80) >> 8;
        pDst[3] = (pCoefs[0] * p0[3] + pCoefs[4] * p0[7] + 0x80) >> 8;
    }
}

inline int32_t fixedNdot16(float val) 
{ 
    const float scale = (float)(1<<16);
    float sfval = val*scale;
    return (int32_t)(sfval+.5f);
}


#include "bilinear.h"
//
// specialization for bilinear resize for Byte source with 1..4 channels only,
// and source band count != 3 (i.e. source expanded to 4 before calling here)
//
HRESULT
BilinearResizeBlock( CImg& imgDstBlk, const vt::CRect rctDstBlk,
                     const CByteImg& imgSrcBlk, const vt::CRect& rctSrcBlk,
                     float sx, float tx, float sy, float ty )
{
    VT_HR_BEGIN()
    VT_ASSERT( (imgSrcBlk.Bands() <= 4) && (imgDstBlk.Bands() <= 4) );
    VT_ASSERT( imgSrcBlk.Bands() != 3 );
    VT_ASSERT( EL_FORMAT(imgSrcBlk.GetType()) == EL_FORMAT_BYTE );

    // the 4 band source code below is faster, particularly when magnifying
    // (since it can reuse scanlines), but it rounds results back to 8bpp
    // between the horizonal and vertical passes so does not match the result
    // of this section (which rounds once); so needs to be commented out until
    // it can be extended to handle all source band counts
    //
    //if ( imgSrcBlk.Bands() != 4)
    {
        int srcBands = imgSrcBlk.Bands();
        int dstBands = imgDstBlk.Bands();
        const Byte* pSrcPix = imgSrcBlk.BytePtr();
        int32_t xsStart = fixedNdot16((sx*float(rctDstBlk.left))+(tx-float(rctSrcBlk.left)));
        int32_t dxs = fixedNdot16(sx);
        int32_t dys = fixedNdot16(sy);
        int dstW = rctDstBlk.Width();
        uint32_t ysStrd = imgSrcBlk.StrideBytes()/srcBands;
        int32_t ys = fixedNdot16((sy*float(rctDstBlk.top))+(ty-float(rctSrcBlk.top)));

        // source rect is expanded on right and bottom, but not top/left, so clamp
        // start values to zero (they can sometimes be very slightly negative)
        if (xsStart < 0) { xsStart = 0x0; }
        if (ys < 0) { ys = 0x0; }

        // generate arrays of fractions and offsets for each source pixel in a
        // scanline; these are the same for every span
        CRGBAByteImg xFracSpan; 
        CRGBAByteImg xOffsSpan;
        VT_HR_EXIT( xFracSpan.Create(dstW,1) );
        VT_HR_EXIT( xOffsSpan.Create(dstW,1) );
        {
            int32_t xs = xsStart;
            uint32_t* pFrac32 = (uint32_t*)xFracSpan.BytePtr();
            uint16_t* pFrac16 = (uint16_t*)xFracSpan.BytePtr();
            uint32_t* pOffs = (uint32_t*)xOffsSpan.BytePtr();
            for (int x=0; x<rctDstBlk.Width(); x++)
            {
                int32_t xsc = (xs < 0)?(0):(xs);
                *pOffs++ = xsc>>16;
                if (srcBands == 1) { *pFrac16++ = ((xsc>>8)&0xff); }
                else { *pFrac32++ = ((xsc>>8)&0xff); }
                xs += dxs;
            }
        }
        // processing routines can directly write result only for matching band
        // counts and byte dest type, so use source-matching scanline buffer as
        // temporary destination otherwise
        bool bUseTempDest = ( (srcBands != dstBands) && !((srcBands==4)&&(dstBands==3)) )
            || (EL_FORMAT(imgDstBlk.GetType()) != EL_FORMAT_BYTE);
        CRGBAByteImg tmpDst;
        if (bUseTempDest)
        {
            // allocate temp scanline buffer for dest that matches source bands
            VT_HR_EXIT( tmpDst.Create(dstW*srcBands,1) );
            // set dest bands to match source bands for writing to tmpDst
            dstBands = srcBands;
        }
        for (int yd = 0; yd < rctDstBlk.Height(); yd++,ys+=dys)
        {
            int32_t xs = xsStart;
            Byte* pDst = (bUseTempDest)?(tmpDst.BytePtr()):(imgDstBlk.BytePtr(yd));
            int w = dstW;
#if defined(VT_SIMD_SUPPORTED)
            if ( (w >= 8) && VTBILINEAR_SIMD_SUPPORTED )
            {
                // reset to start of scanline
                const uint32_t* pFrac = (uint32_t*)xFracSpan.BytePtr();
                const uint32_t* pOffs = (uint32_t*)xOffsSpan.BytePtr();
                if (srcBands == 1)
                {
                    uint16x8_t ysFrac = _mm_set1_epi16(((uint32_t)ys<<16)>>24);
                    while (w >= 8)
                    {
                        uint16x8_t xf0to7 = _mm_load_si128((const uint16x8_t*)pFrac); pFrac+=4;
                        Bilinear1BandBytex8Process(xf0to7,ysFrac,
                            pDst,pOffs,pSrcPix+((ys>>16)*ysStrd),ysStrd);
                        pOffs += 8; pDst += 8; w -= 8;
                    }
                }
                else 
                {
                    uint32x4_t ysFrac = _mm_set1_epi32(((uint32_t)ys<<16)>>24);
                    if (srcBands == 2)
                    {
                        while (w >= 4)
                        {
                            uint32x4_t xf0123 = _mm_load_si128((const uint32x4_t*)pFrac); pFrac+=4;
                            Bilinear2BandBytex4Process(xf0123,ysFrac,pDst,pOffs,
                                (const uint16_t*)pSrcPix+((ys>>16)*ysStrd),ysStrd);
                            pDst+=8; pOffs += 4; w -= 4;
                        }
                    }
                    else if ((srcBands==4)&&(dstBands==3))
                    {
                        while (w >= 4)
                        {
                            uint32x4_t xf0123 = _mm_load_si128((const uint32x4_t*)pFrac); pFrac+=4;
                            Bilinear4to3BandBytex4Process(xf0123,ysFrac,pDst,pOffs,
                                (const uint32_t*)pSrcPix+((ys>>16)*ysStrd),ysStrd);
                            pDst+=12; pOffs += 4; w -= 4;
                        }
                    }
                    else if (srcBands == 4)
                    {
                        while (w >= 4)
                        {
                            uint32x4_t xf0123 = _mm_load_si128((const uint32x4_t*)pFrac); pFrac+=4;
                            Bilinear4BandBytex4Process(xf0123,ysFrac,pDst,pOffs,
                                (const uint32_t*)pSrcPix+((ys>>16)*ysStrd),ysStrd);
                            pDst+=16; pOffs += 4; w -= 4;
                        }
                    }
                }
            }
#endif
            if (w > 0)
            {
                xs += (imgDstBlk.Width()-w)*dxs;
                uint16_t yfrac = (uint16_t)(((uint32_t)ys<<16)>>24);
                while (w > 0)
                {
                    uint16_t xfrac = (uint16_t)(((uint32_t)xs<<16)>>24);
                    BilinearProcessSinglePixel(dstBands, pDst,
                        xs>>16,ys>>16,xfrac,yfrac,ysStrd,pSrcPix);
                    pDst += dstBands;
                    xs += dxs;
                    w--;
                }
            }
            if (bUseTempDest)
            {
                // convert for non-byte dest and/or 1->3, 1->4, or 4->1
                VT_HR_EXIT( VtConvertSpan(
                    (void*)imgDstBlk.BytePtr(yd),imgDstBlk.GetType(),
                    (const void*)tmpDst.BytePtr(),imgSrcBlk.GetType(),
                    srcBands*dstW) );
            }
        }
        return hr;
    }
#if 0
    // here for 4 band byte source specialization for any dest type
    VT_ASSERT( imgSrcBlk.Bands() == 4 );

    int iW       = imgDstBlk.Width();
    int iSB      = imgSrcBlk.Bands();
    int iCoefCnt = iSB*2;

    // 2 temp Byte buffers for the results of horizontal resize
    // 1 temp int src coords of each band in the dest
    // 1 temp float buffer for the bilinear coefficients
    CTypedBuffer4<Byte, Byte, int, UInt16> buf(iSB, iSB, 1, iCoefCnt);

    for( CSpanIterator it(iW, buf.Capacity()); !it.Done(); it.Advance() )
    {
        int x = it.GetPosition();
        int c = it.GetCount();

        // generate bilinear X coefs and convert them to working variables
        int*    pCrdI = buf.Buffer3();
        UInt16* pCoef = buf.Buffer4();
        float dstx = float(rctDstBlk.left+x);
        for( int j = 0; j < c*iSB; dstx++, pCrdI++ )
        {
            float srcx = sx*dstx+tx;
            int   flr  = (int)floor(srcx);
            float c1   = srcx - float(flr);
            float c0   = 1.f-c1;
            UInt16 uC0 = (UInt16)F2I(256.f * c0);
            UInt16 uC1 = (UInt16)F2I(256.f * c1);
            for( int b = 0; b < iSB; b++, j++ )
            {
                pCoef[0] = uC0;
                pCoef[4] = uC1;
                pCoef++;
                if( (j&3)==3 )
                {
                    pCoef += 4;
                }
            }

            *pCrdI  = (flr - rctSrcBlk.left)*iSB;

            // the resize code (unlike the perspective case) expects all source
            // pixels to be provided 
            VT_ASSERT( *pCrdI >= 0 && *pCrdI < (rctSrcBlk.Width()-1)*iSB );
        }

        int line1 = -1, line2 = -1;
        float ysrc = sy*float(rctDstBlk.top)+(ty-rctSrcBlk.top);
        for( int y = 0; y < rctDstBlk.Height(); y++, ysrc+=sy )
        {
            // generate bilinear Y coords
            int y0 = (int)floor(ysrc);

            // test if new lines are required and generate them
            for( int yt = y0; yt < y0+2; yt++ )
            {
                if( yt!=line1 && yt!=line2 )
                {
                    Byte* p0;
                    if( line1 != ((yt==y0)? y0+1: y0) )
                    {
                        p0 = buf.Buffer1();
                        line1 = yt;
                    }
                    else
                    {
                        p0 = buf.Buffer2();
                        line2 = yt;
                    }
                    BilinearResizeHorizontalByte4(
                        p0, c, imgSrcBlk.Ptr(yt), buf.Buffer3(), buf.Buffer4() );
                }
            }

            // run the vertical filter
            Byte *p0  = (line1==y0)? buf.Buffer1(): buf.Buffer2();
            Byte *p1  = (line1==y0)? buf.Buffer2(): buf.Buffer1();
            float fCY1 =  ysrc - float(y0);

#define DO_BLEND(OT) \
    VtBlendSpan((OT*)imgDstBlk.BytePtr(x, y), imgDstBlk.Bands(), p0, p1, iSB, \
    1.f-fCY1, fCY1, c); break;

            switch (EL_FORMAT(imgDstBlk.GetType()))
            {
            case EL_FORMAT_BYTE: DO_BLEND(Byte);
            case EL_FORMAT_SHORT: DO_BLEND(UInt16);
            case EL_FORMAT_HALF_FLOAT: DO_BLEND(HALF_FLOAT);
            case EL_FORMAT_FLOAT: DO_BLEND(float);
            default:
                VT_ASSERT(false);
            }
#undef DO_BLEND
        }
    }
#endif
    VT_HR_END();
}

//+----------------------------------------------------------------------------
//
// fully general resize routines
//
//-----------------------------------------------------------------------------
#if defined(VT_SSE_SUPPORTED)

template<void (*pfnStore)(float*, const __m128&)>
int BilinearResizeHorizontal_SSE( float* pDst, int bands, int pixcount, 
                                           const float* pSrc, const int* pCoords, 
                                           const float* pCoefs)
{
    VT_ASSERT( ((INT_PTR)pCoefs & 0xf) == 0 );

    int i = 0;
    if( bands == 4 )
    {
        for( ; i < pixcount; i++, pCoords+=4, pCoefs+=8, pDst+=4 )
        {
            // generate 4 outputs at a time
            __m128 x0 = _mm_load_ps(pCoefs);
            __m128 x1 = _mm_load_ps(pCoefs+4);
    
            const float* p0 = pSrc + *pCoords;
            __m128 x2 = _mm_loadu_ps(p0); // 4 x row 0
            x2 = _mm_mul_ps(x2, x0);
    
            __m128 x3 = _mm_loadu_ps(p0+4);   // 4 x row 1
            x3 = _mm_mul_ps(x3, x1);
            x2 = _mm_add_ps(x2, x3);
    
            pfnStore(pDst, x2);
        }
    }
    else if( bands == 1 )
    {
        i = pixcount & ~3;
        for( int j = 0; j < i; j+=4, pCoefs+=8, pDst+=4 )
        {
            // load pairs of source float values and unpack/shuffle to group
            // 'Left' and 'Right' samples 
            __m128 x2,x3;
            {
                __m128 s0 = _mm_castsi128_ps(_mm_loadl_epi64(
                    (const __m128i*)(pSrc + *pCoords++))); // L0R0
                __m128 s1 = _mm_castsi128_ps(_mm_loadl_epi64(
                    (const __m128i*)(pSrc + *pCoords++))); // L1R1
                __m128 s2 = _mm_castsi128_ps(_mm_loadl_epi64(
                    (const __m128i*)(pSrc + *pCoords++))); // L2R2
                __m128 s3 = _mm_castsi128_ps(_mm_loadl_epi64(
                    (const __m128i*)(pSrc + *pCoords++))); // L3R3
                __m128 ps0 = _mm_unpacklo_ps(s0,s1); // L0L1R0R1
                __m128 ps1 = _mm_unpacklo_ps(s2,s3); // L2L3R2R3
                x2 = _mm_shuffle_ps(ps0,ps1,_MM_SHUFFLE(1,0,1,0)); // L0L1L2L3
                x3 = _mm_shuffle_ps(ps0,ps1,_MM_SHUFFLE(3,2,3,2)); // R0R1R2R3
            }
       
            // load coefficients for the 4 source pixels
            __m128 x0 = _mm_load_ps(pCoefs);
            __m128 x1 = _mm_load_ps(pCoefs+4);
        
            // apply the coefficients
            x2 = _mm_mul_ps(x2, x0);
            x3 = _mm_mul_ps(x3, x1);
            x2 = _mm_add_ps(x2, x3);
            
            pfnStore(pDst, x2);
        }
    }
    else
    {        
        // run the bilinear filter
        VT_DECLSPEC_ALIGN(16) float packed[8];
        i = pixcount & ~3;
        int lastel = i * bands;
        for( int j = 0; j < lastel; j+=4, pCoefs+=8, pDst+=4 )
        {
            // pack the float values
            const float* p0 = pSrc + *pCoords;
            packed[0] = *(p0);
            packed[4] = *(p0+bands);
            pCoords++;
            
            p0 = pSrc + *pCoords;
            packed[1] = *(p0);
            packed[5] = *(p0+bands);
            pCoords++;
            
            p0 = pSrc + *pCoords;
            packed[2] = *(p0);
            packed[6] = *(p0+bands);
            pCoords++;
            
            p0 = pSrc + *pCoords;
            packed[3] = *(p0);
            packed[7] = *(p0+bands);
            pCoords++;
        
            // generate 4 outputs at a time
            __m128 x0 = _mm_load_ps(pCoefs);
            __m128 x1 = _mm_load_ps(pCoefs+4);
        
            __m128 x2 = _mm_load_ps(packed);   // 4 x first element
            x2 = _mm_mul_ps(x2, x0);
            
            __m128 x3 = _mm_load_ps(packed+4); // 4 x second element
            x3 = _mm_mul_ps(x3, x1);
            x2 = _mm_add_ps(x2, x3);
                
            pfnStore(pDst, x2);
        }
    }

    return i;
}
#endif

void BilinearResizeHorizontal(float* pDst, int bands, int pixcount, 
                              const float* pSrc, const int* pCoords, 
                              const float* pCoefs)
{
    int i = 0;

#if defined(VT_SSE_SUPPORTED)
    if( g_SupportSSE2() )
    {
        SelectPSTFunc(IsAligned16(pDst),i,Store4,
            BilinearResizeHorizontal_SSE, 
            pDst, bands, pixcount, pSrc, pCoords, pCoefs);
    }
#endif

    int j = i*bands;
    pCoords += j;
    pCoefs  += j*2;
    pDst    += j;

    for( ; j < pixcount*bands; j++, pDst++, pCoords++ )
    {
        const float* pS0 = pSrc + *pCoords;
        const float* pS1 = pS0  + bands;
        *pDst = pCoefs[0] * *pS0 + pCoefs[4] * *pS1;
        pCoefs++;
        if( (j&3)==3 )
        {
            pCoefs+=4;
        }
    }
}

HRESULT
BilinearResizeBlock( CImg& imgDstBlk, const vt::CRect rctDstBlk,
                     const CFloatImg& imgSrcBlk, const vt::CRect& rctSrcBlk,
                     float sx, float tx, float sy, float ty )
{
    int iW       = imgDstBlk.Width();
    int iSB      = imgSrcBlk.Bands();

    // check for 2:1 box decimation case where resize is exactly 2:1 and aligned for 'Dual'
    // sampling scheme, in which case each destination pixel is the normalized sum of the four
    // adjacent source pixels; currently supported for 1 band float only
    if( ( iSB == 1 ) && 
        ( imgDstBlk.Bands() == 1 ) && ( EL_FORMAT(imgDstBlk.GetType()) == OBJ_FLOATIMG ) &&
        ( sx == 2.f ) && ( sy == 2.f ) && ( tx == .5f ) && ( ty == .5f ) )
    {
        return VtSeparableFilterBoxDecimate2to1( imgDstBlk, rctDstBlk, imgSrcBlk, CPoint(0,0) );
    }

    int iCoefCnt = iSB*2;

    // 2 temp float buffers for the results of horizontal resize
    // 1 temp int src coords of each band in the dest
    // 1 temp float buffer for the bilinear coefficients
    CTypedBuffer4<float, float, int, float> buf(iSB, iSB, iSB, iCoefCnt);

    // note: because of the way the buffer is used below (expects coefs to
    //       stored in chunks of 8) we should only use the capacity up
    //       to a multiple of 4
    int cap = buf.Capacity() & ~3;
    for( CSpanIterator it(iW, cap); !it.Done(); it.Advance() )
    {
        int x = it.GetPosition();
        int c = it.GetCount();

        // generate bilinear X coefs and convert them to working variables
        int*    pCrdI = buf.Buffer3();
        float*  pCoef = buf.Buffer4();
        float dstx = float(rctDstBlk.left+x);
        for( int j = 0; j < c*iSB; dstx++, pCrdI+=iSB )
        {
            float srcx = sx*dstx+tx;
            int   flr  = (int)floor(srcx);
            float c1   = srcx - float(flr);
            float c0   = 1.f-c1;
            int   cx0  = (flr - rctSrcBlk.left)*iSB;
            for( int b = 0; b < iSB; b++, j++ )
            {
                pCoef[0] = c0;
                pCoef[4] = c1;
                pCrdI[b] = cx0+b;
                // implement Extend extend mode for horizontal axis
                pCrdI[b] = VtMax( pCrdI[b], b );
                pCrdI[b] = VtMin( pCrdI[b], ((imgSrcBlk.Width()-2)*iSB)+b );
                pCoef++;
                if( (j&3)==3 )
                {
                    pCoef += 4;
                }
            }

            // either all source pixels are provided or Extend mode clamp limits pCrdI
            VT_ASSERT( *pCrdI >= 0 && *pCrdI < (rctSrcBlk.Width()-1)*iSB );
        }

        int line1 = -1, line2 = -1;
        float ysrc = sy*float(rctDstBlk.top)+ty-rctSrcBlk.top;
        for( int y = 0; y < rctDstBlk.Height(); y++, ysrc+=sy )
        {
            // generate bilinear Y coords
            int y0 = int(ysrc);

            // test if new lines are required and generate them
            for( int yt = y0; yt < y0+2; yt++ )
            {
                if( yt!=line1 && yt!=line2 )
                {
                    float* p0;
                    if( line1 != ((yt==y0)? y0+1: y0) )
                    {
                        p0 = buf.Buffer1();
                        line1 = yt;
                    }
                    else
                    {
                        p0 = buf.Buffer2();
                        line2 = yt;
                    }
                    // implement Extend extend mode for vertical axis
                    int ytex = yt;
                    ytex = VtMax( ytex, (int)imgSrcBlk.Rect().top );
                    ytex = VtMin( ytex, (int)imgSrcBlk.Rect().bottom-1 );
                    BilinearResizeHorizontal(
                        p0, iSB, c, imgSrcBlk.Ptr(ytex),
                        buf.Buffer3(), buf.Buffer4());
                }
            }

            // run the vertical filter
            float *p0  = (line1==y0)? buf.Buffer1(): buf.Buffer2();
            float *p1  = (line1==y0)? buf.Buffer2(): buf.Buffer1();
            float fCY1 =  ysrc - float(y0);
        
#define DO_BLEND(OT) \
    VtBlendSpan((OT*)imgDstBlk.BytePtr(x, y), imgDstBlk.Bands(), p0, p1, iSB, \
    1.f-fCY1, fCY1, c); break;

            switch (EL_FORMAT(imgDstBlk.GetType()))
            {
            case EL_FORMAT_BYTE: DO_BLEND(Byte);
            case EL_FORMAT_SHORT: DO_BLEND(UInt16);
            case EL_FORMAT_SSHORT: DO_BLEND(Int16);
            case EL_FORMAT_HALF_FLOAT: DO_BLEND(HALF_FLOAT);
            case EL_FORMAT_FLOAT: DO_BLEND(float);
            default:
                VT_ASSERT(false);
            }
#undef DO_BLEND
        }
    }
        return S_OK;
}

//+----------------------------------------------------------------------------
//
// Utility Routines
//
//-----------------------------------------------------------------------------
static HRESULT
CreateKernelSetForResize(C1dKernelSet& k, eSamplerKernel sampler,
                         int iSrcSamples, int iDstSamples, float fPhase)
{
    HRESULT hr = S_OK;

    switch(sampler)
    {
    case eSamplerKernelBilinear:
        hr = Create1dBilinearKernelSet(k, iSrcSamples, iDstSamples, fPhase);
        break;
    case eSamplerKernelBicubic:
        hr = Create1dBicubicKernelSet(k, iSrcSamples, iDstSamples, fPhase);
        break;
    case eSamplerKernelLanczos2:
    case eSamplerKernelLanczos2Precise:
        hr = Create1dLanczosKernelSet(k, iSrcSamples, iDstSamples, 2, fPhase);
        break;
    case eSamplerKernelLanczos3:
    case eSamplerKernelLanczos3Precise:
        hr = Create1dLanczosKernelSet(k, iSrcSamples, iDstSamples, 3, fPhase);
        break;
    case eSamplerKernelLanczos4:
    case eSamplerKernelLanczos4Precise:
        hr = Create1dLanczosKernelSet(k, iSrcSamples, iDstSamples, 4, fPhase);
        break;
    case eSamplerKernelTriggs5:
        hr = Create1dTriggsKernelSet(k, iSrcSamples, iDstSamples, 5, fPhase);
        break;
    case eSamplerKernelBicubicBSpline:
    case eSamplerKernelBicubicBSplineSrcPreprocessed:
        hr = Create1dBicubicBSplineKernelSet(k, iSrcSamples, iDstSamples, fPhase);
        break;
    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}

// applies 'steps' number of 121 filtered 2:1 decimation downsample passes
HRESULT Apply121Downsamples(CImg& dst, const CImg& src, int steps, 
    const IMAGE_EXTEND& ex)
{
    VT_HR_BEGIN();
    VT_ASSERT( steps >= 1 );

    int sW = src.Width();
    int sH = src.Height();
    int s=0; int dsti = 0; int srci = 1;

    // double buffer if CImg's for per-pass dst and src, 
    // indexed by dsti and srci which flip for each pass
    CImg img[2]; 
    VT_HR_EXIT( src.Share(img[srci]) ); 
    for (; s < steps; s++)
    {
        sW >>= 1; sH >>= 1;
        VT_HR_EXIT( img[dsti].Create(sW,sH,src.GetType()) ); 

        // the VtSeparableFilter121 routines only support 'Extend'
        // ExtendMode, so call VtSeparableFilter if extend mode
        // is other then 'Extend' otherwise call '121 routine
        // TODO: just call VtSeparableFilter when it is modified
        // to examine the kernel and call 121 itself
        if (ex.IsVerticalSameAsHorizontal() &&
            (ex.exHoriz == vt::Extend) )
        {
            VT_HR_EXIT( VtSeparableFilter121Decimate2to1(
                img[dsti],img[dsti].Rect(),img[srci],CPoint(0,0)) );
        }
        else
        {
            C121Kernel k121;
   	        C1dKernelSet ks;
	        ks.Create(1,2);
            ks.Set(0, -1, k121.AsKernel());
            VT_HR_EXIT( VtSeparableFilter(img[dsti],
                img[dsti].Rect(),img[srci],CPoint(0,0),ks) );
        }

        if (s < (steps-1)) { srci^=1; dsti^=1; }
    }

    // share results for return
    VT_HR_EXIT( img[dsti].Share(dst) ); 

    VT_HR_END();
}

HRESULT
ResizeImage(CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
            int iSrcSamplesX, int iDstSamplesX, float fPhaseX,
            int iSrcSamplesY, int iDstSamplesY, float fPhaseY,
            eSamplerKernel sampler, const IMAGE_EXTEND& ex,
            eResizeSamplingScheme eSS = eResizeSamplingSchemePrimal)
{
    // checking for valid imgSrc and overlapping imgSrc and imgDst already done

    VT_HR_BEGIN()

    // (potentially decimated) image source for SeparableFilter op
    CImg imgSrcSF;

    if( sampler == eSamplerKernelBicubicBSplineSrcPreprocessed && imgSrc.GetType() != EL_FORMAT_FLOAT)
    {
        // Output of required pre-processing must be float to capture range of possible values.
        // Forget to pre-process? Also, can't convert type in between pre-process and resample.
        VT_ASSERT(false); 
        return E_INVALIDARG;
    }
    else if(
        ( (sampler == eSamplerKernelLanczos2) || // Lanczos* only
          (sampler == eSamplerKernelLanczos3) ||
          (sampler == eSamplerKernelLanczos4) ) )
    {
        // performance optimization for Lanczos filtering - when resize is a
        // large decimation, then use a series of 121 filtered 2:1 decimations
        // to reduce the source dimension, then apply the Lanczos filter

        // create the destination image if needed
        VT_HR_EXIT( CreateImageForTransform(imgDst, rctDst.Width(), rctDst.Height(),
                                            imgSrc.GetType()) );

        // determine minimum source dimension and get same dimension for dest
        bool minSrcDimX = (iSrcSamplesX >= iSrcSamplesY);
        int minSrcDim = minSrcDimX?(iSrcSamplesX):(iSrcSamplesY);
        int minDstDim = minSrcDimX?(iDstSamplesX):(iDstSamplesY);

        // compute the log space of the minimum dimensions - a step of 1.0
        // in *DimLog reflects a 2:1 step in pixel space
        float srcDimLog = logf((float)minSrcDim)/logf(2.f);
        float dstDimLog = logf((float)minDstDim)/logf(2.f);
        float diffDimLog = srcDimLog - dstDimLog;

        // determine the approximate number of 2:1 downsamples between
        // the source and destination - adding .2 and taking the floor 
        // implies that a log2 dimension smaller than .8 is not deemed
        // a full 2:1 step (and anything larger than .8 is)
        int downSampleSteps = int(floorf(diffDimLog+.2f));

        // determine the number of 121 decimation downsample steps leaving
        // sufficient resolution for the final Lanczos step - the image quality
        // is quite good for applying Lanczos for as little as final 2:1
        // downsample (which would be 'downSampleSteps-1' 121 steps), but a
        // close comparison reveals that the results are different enough that
        // leaving 4:1 downsampling for the Lanczos step is warranted, thus
        // subtracing 2
        int downSampleSteps121 = downSampleSteps-2;

        if ( downSampleSteps121 <= 0 )
        {
            // no 121 decimations, so just share out source
            VT_HR_EXIT( imgSrc.Share(imgSrcSF) ); 
        }
        else
        {
            VT_HR_EXIT( Apply121Downsamples(imgSrcSF, imgSrc, 
                downSampleSteps121, ex) );

            // adjust iDstSamples* to reflect the downsamples
            iDstSamplesX <<= downSampleSteps121;
            iDstSamplesY <<= downSampleSteps121;
            // recompute phase values if not primal sampling
            if(eSS != eResizeSamplingSchemePrimal )
            {
                fPhaseX /= (1 << downSampleSteps121);
                fPhaseY /= (1 << downSampleSteps121);
            }

        }
    }
    else
    {
        // no optimization, so just share out the source for separable filter
        VT_HR_EXIT( imgSrc.Share(imgSrcSF) ); 
    }

    // create the filter kernels
    C1dKernelSet kX, kY;
    VT_HR_EXIT( CreateKernelSetForResize(kX, sampler, iSrcSamplesX, iDstSamplesX,
                                         fPhaseX) );
    VT_HR_EXIT( CreateKernelSetForResize(kY, sampler, iSrcSamplesY, iDstSamplesY,
                                         fPhaseY) );

    VT_HR_EXIT( VtSeparableFilter(imgDst, rctDst, imgSrcSF, vt::CPoint(0,0),
                                  kX, kY, ex) );

    /// Check sampler type to see if we need any post-processing.
    if( sampler == eSamplerKernelBicubicBSpline )
    {
        VT_HR_EXIT( BSplinePostProcess(&imgDst, ex) );
    }

    VT_HR_END()
}

//+----------------------------------------------------------------------------
//
// function: VtResizeImg(...)
// 
//-----------------------------------------------------------------------------
HRESULT 
vt::VtResizeImage(CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
                  float sx, float tx, float sy, float ty,
                  eSamplerKernel sampler, const IMAGE_EXTEND& ex)
{
	if( !imgSrc.IsValid() )
    {
        return E_INVALIDSRC;
    }

    if( imgSrc.IsSharingMemory(imgDst) )
    {
        // the operation cannot be done in-place
        return E_INVALIDARG;
    }

    // convert scale parameters to a 3x3 transform
    CMtx3x3f mScale(sx, 0, tx, 0, sy, ty, 0, 0, 1.f);

    // depending on which sampler is selected call the appropriate code. the
    // function below only handles bilinear
    if( sampler != eSamplerKernelBilinear )
    {
        if( sampler==eSamplerKernelBicubic ||
            sampler==eSamplerKernelNearest )
        {
            return VtWarpImage(imgDst, rctDst, imgSrc, mScale, sampler, ex);
        }
        else
        {
            int srcx = F2I(float(rctDst.Width()*sx));
            int srcy = F2I(float(rctDst.Height()*sy));
            return ResizeImage(imgDst, rctDst, imgSrc, srcx, rctDst.Width(), tx,
                               srcy, rctDst.Height(), ty, sampler, ex);
        }
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

    // determine the "operation" type.  reduce band count early if possible.
    // bilinear has a specialization for any 1..4 banded byte source, but in
    // all other cases operate as floats and then convert to the output format
    int iOpBands = VtMin(imgSrc.Bands(), imgDst.Bands());
    if (iOpBands == 3 && EL_FORMAT(imgSrc.GetType())==EL_FORMAT_BYTE)
        iOpBands = 4; // use 4 banded byte specialization for 3 banded byte
    int iOpType = EL_FORMAT_FLOAT;
    if ( ((imgDst.Bands() <= 4) && (imgSrc.Bands() <= 4)) && 
        EL_FORMAT(imgSrc.GetType())==EL_FORMAT_BYTE)
    { iOpType = EL_FORMAT_BYTE; }

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

        // determine the required source block
        vt::CRect rctSrcBlk = MapRegion3x3(mScale, rctDstBlk);
        VT_ASSERT( !rctSrcBlk.IsRectEmpty() );

        VT_ASSERT( sampler==eSamplerKernelBilinear );
        // adjust the rect for the necessary filter support
        // if( sampler==eSamplerKernelBilinear ) // for now only bilinear
        // due to the incremental computation of the coordinates in the
        // following loop vs. the direct computation above, sometimes we
        // see an off by one issue on the right/bottom pixels so increment
        // by 2 here instead of 1
        {
            rctSrcBlk.right+=2;
            rctSrcBlk.bottom+=2;
        }

        // if Extend extend mode can be applied w/o CropPadImage (due to coordinate
        // clamping in BilinearResizeBlock), then modify rctSrcBlk to send the
        // full imgSrc
        if( ( iOpType == EL_FORMAT_FLOAT ) && 
            ( ex.IsVerticalSameAsHorizontal() ) &&
            ( ex.exHoriz == vt::Extend ) )
        {
            rctSrcBlk = CRect(0,0,imgSrc.Width(),imgSrc.Height());
        }

        if( EL_FORMAT(imgSrc.GetType()) == iOpType  &&
            imgSrc.Bands()              == iOpBands && 
            vt::CRect(imgSrc.Rect()).RectInRect(&rctSrcBlk) )
        {
            // share the block out of the source if possible
            imgSrc.Share(imgSrcBlk, &rctSrcBlk);
        }
        else
        {
            VT_HR_EXIT( imgSrcBlk.Create(rctSrcBlk.Width(), rctSrcBlk.Height(),
                                         VT_IMG_MAKE_TYPE(iOpType, iOpBands)) );

            // pad the image when out of bounds pixels are requested
            VT_HR_EXIT( VtCropPadImage(imgSrcBlk, rctSrcBlk, imgSrc, ex) );
        }

        // warp the block
        if( iOpType == EL_FORMAT_BYTE )
        {
            VT_HR_EXIT( BilinearResizeBlock( imgDstBlk, rctDstBlk, 
                (CByteImg&)imgSrcBlk, rctSrcBlk, sx, tx, sy, ty) );
        }
        else
        {
            VT_HR_EXIT( BilinearResizeBlock( imgDstBlk, rctDstBlk, 
                (CFloatImg&)imgSrcBlk, rctSrcBlk, sx, tx, sy, ty) );
        }
    }

    VT_HR_END()
}


HRESULT
vt::VtResizeImage(CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
                  int iSrcSamplesX, int iDstSamplesX, 
                  int iSrcSamplesY, int iDstSamplesY,
                  eSamplerKernel sampler, const IMAGE_EXTEND& ex,
                  eResizeSamplingScheme eSS)
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

    float fPhaseX, fPhaseY;
    if( eSS==eResizeSamplingSchemePrimal )
    {
        fPhaseX = fPhaseY = 0;
    }
    else
    {
        fPhaseX = 0.5f*(float(iSrcSamplesX)/float(iDstSamplesX)-1.f);
        fPhaseY = 0.5f*(float(iSrcSamplesY)/float(iDstSamplesY)-1.f);
    }

    float sx = float(iSrcSamplesX) / float(iDstSamplesX);
    float sy = float(iSrcSamplesY) / float(iDstSamplesY);
    if( sampler==eSamplerKernelBilinear || sampler==eSamplerKernelNearest || 
        (sampler==eSamplerKernelBicubic && sx <= 1.f && sy <= 1.f) )
    {
        return VtResizeImage(imgDst, rctDst, imgSrc, sx, fPhaseX, sy, fPhaseY,
                             sampler, ex);
    }
    else
    {
        return ResizeImage(imgDst, rctDst, imgSrc, 
                           iSrcSamplesX, iDstSamplesX, fPhaseX, 
                           iSrcSamplesY, iDstSamplesY, fPhaseY, sampler, ex, eSS);
    }
}

//+-----------------------------------------------------------------------------
//
// Transform Framework Routines
//
//------------------------------------------------------------------------------
#ifndef VT_NO_XFORMS

void Rationalize(int& num, int& denom, float f)
{
    // compute the continued fraction up to 8 terms, don't
    // allow denom > 1024
    // (http://en.wikipedia.org/wiki/Continued_fraction) 

    const int c_cfterms = 8;
    int cf[c_cfterms];

    // only rationalize the fractional part
    int numbase = (int)f;
    f = f - float(numbase);

    if( f == 0 )
    {
        num = numbase;
        denom = 1;
        return;
    }

    for( int j = c_cfterms; j >= 2; j-- )
    {
        float cur = f;
        int cnt = j;
        for( int i = 0; i < j-1; i++ )
        {
            float flr = floorf(cur);
            cf[i] = F2I(flr);
            cur   = cur-flr;
            if( cur == 0 )
            {
                cnt = i+1;
                break;
            }
            cur = 1.f/cur;
        }
        if( cnt == j )
        {
            cf[cnt-1] = F2I(cur);
        }
    
        // convert the continued fraction terms into rational
        num   = 1;
        denom = cf[cnt-1];
        if( cnt > 1 )
        {
            for( int i = cnt-2; i>=1; i-- )
            {
                if( denom >= 1024 )
                {
                    // avoid integer overflow, early out here 
                    break;
                }
                int tmp = denom;  
                denom = denom * cf[i] + num;
                num   = tmp;
            }
            num = denom * cf[0] + num;
        }
        else if( f > 1 )
        {
            num   = denom;
            denom = 1;
        }

        if( denom < 1024 )
        {
            break;
        }
    }

    if( denom > 1024 && numbase != 0 )
    {
        num = numbase;
        denom = 1;
    }
    else
    {
        num += numbase * denom;
    }
}

HRESULT 
CWarpTransform::InitializeResize(float sx, float tx, float sy, float ty,
                                 int dstType, eSamplerKernel sampler)
{
    if (m_NoRW) { m_NoRW = false; m_src.Deallocate(); m_dst.Deallocate(); }
    VT_HR_BEGIN()

    Clear();
    m_bIsResize = true;
    m_dstType   = (dstType==OBJ_UNDEFINED)? 
		dstType: (VT_IMG_FIXED_PIXFRMT_MASK|VT_IMG_FIXED_ELFRMT_MASK|dstType);
    m_sampler   = sampler;
    if( m_sampler != eSamplerKernelNearest &&
        m_sampler != eSamplerKernelBilinear &&
        !(m_sampler == eSamplerKernelBicubic && sx <= 1.f && sy <= 1.f) )
    {
        // create the filter kernels
        C1dKernelSet kX, kY;

        int iSrcSamplesX, iDstSamplesX;
        Rationalize(iSrcSamplesX, iDstSamplesX, sx);
        VT_HR_EXIT( CreateKernelSetForResize(kX, sampler, iSrcSamplesX, iDstSamplesX,
                                             tx) );

        int iSrcSamplesY, iDstSamplesY;
        Rationalize(iSrcSamplesY, iDstSamplesY, sy);
        VT_HR_EXIT( CreateKernelSetForResize(kY, sampler, iSrcSamplesY, iDstSamplesY,
                                             ty) );

        // initialize the separable filter
        VT_ASSERT( m_pFilterTransform == NULL );

        m_pFilterTransform = VT_NOTHROWNEW CSeparableFilterTransform();
        VT_PTR_OOM_EXIT(m_pFilterTransform);

        VT_HR_EXIT(m_pFilterTransform->Initialize(dstType, kX, kY));

        m_bDownsample = (iSrcSamplesX >= iDstSamplesX) && (iSrcSamplesY >= iDstSamplesY);
    }
    m_mtxResize = CMtx3x3f(sx, 0, tx, 0, sy, ty, 0, 0, 1.f);

    VT_HR_END()
}

#endif
