//+----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      SSE routines for bilinear sampling for warp, resize, etc.
//
//-----------------------------------------------------------------------------
#pragma once

#if defined(VT_SSE_SUPPORTED)

// uses up to SSSE3 instructions
#define VTBILINEAR_SIMD_SUPPORTED (g_SupportSSSE3())

//+----------------------------------------------------------------------------
//
// Byte image type; 1-4 bands; same band count for source and dest for 1,2,4
// bands, 3 band dest requires 4 band source
//
// templating is to selectively compile the conditional when loading source
// pixels - use T of int32_t and negative pOffs values to indicate out-of-bounds
// address (to use Zero for source samples); use T of uint32_t when OOB
// processing is not needed and the conditionals on pOffs values are removed by
// the compiler
//
//-----------------------------------------------------------------------------
template <typename T> 
VT_FORCEINLINE void 
Bilinear1BandBytex8Process(
    const __m128i& xf0to7, const __m128i& yf0to7,
    Byte* pDst,
    const T* pOffs, const Byte* pSrcPixBase, uint32_t srcPixStride)
{
    __m128i halfReg16 = _mm_set1_epi16(0x0080);
    __m128i unitReg16 = _mm_set1_epi16(0x0100);

    // compute sample weights
    // pixel samples are: |a b| top
    //                    |c d| bottom
    __m128i wa8,wb8,wc8,wd8; // weights for pixels a,b,c,d
    {
        wd8 = _mm_mullo_epi16(xf0to7,yf0to7);   // ufrac*vfrac
        wd8 = _mm_add_epi16(wd8,halfReg16);     // rounding bias
        wd8 = _mm_srli_epi16(wd8,8);            // wd
        wc8 = _mm_sub_epi16(yf0to7,wd8);        // wc = vfrac - wd;
        wb8 = _mm_sub_epi16(xf0to7,wd8);        // wb = ufrac - wd;
        wa8 = _mm_add_epi16(xf0to7,wc8);        // ufrac+wc
        wa8 = _mm_sub_epi16(unitReg16,wa8);     // wa = 1 - (ufrac + wc)
    }

    // load samples and filter
    {
        // initialize samples to zero for oob handling; load if valid offset
        __m128i a8 = _mm_setzero_si128();
        __m128i b8 = _mm_setzero_si128();
        __m128i c8 = _mm_setzero_si128();
        __m128i d8 = _mm_setzero_si128();
#define LOAD_SRCPIX(_n_) \
        { \
            T offs = pOffs[_n_]; \
            if (offs >= 0) \
            { \
                a8 = _mm_insert_epi16(a8,(uint16_t)(*(pSrcPixBase+offs+0)),_n_); \
                b8 = _mm_insert_epi16(b8,(uint16_t)(*(pSrcPixBase+offs+1)),_n_); \
                c8 = _mm_insert_epi16(c8,(uint16_t)(*(pSrcPixBase+offs+0+srcPixStride)),_n_); \
                d8 = _mm_insert_epi16(d8,(uint16_t)(*(pSrcPixBase+offs+1+srcPixStride)),_n_); \
            } \
        }
        LOAD_SRCPIX(0) LOAD_SRCPIX(1) LOAD_SRCPIX(2) LOAD_SRCPIX(3)
        LOAD_SRCPIX(4) LOAD_SRCPIX(5) LOAD_SRCPIX(6) LOAD_SRCPIX(7)
#undef LOAD_SRCPIX
        __m128i x0,x1;
        // multiply-accumulate samples*weights
        x0 = _mm_mullo_epi16(a8, wa8);
        x1 = _mm_mullo_epi16(b8, wb8);
        x0 = _mm_add_epi16(x0,x1);
        x1 = _mm_mullo_epi16(c8, wc8);
        x0 = _mm_add_epi16(x0,x1);
        x1 = _mm_mullo_epi16(d8, wd8);
        x0 = _mm_add_epi16(x0,x1);
        // add half and shift out fractional bits
        x0 = _mm_adds_epu16(x0,halfReg16);
        x0 = _mm_srli_epi16(x0,8);
        // pack to 8bpp and store
        x0 = _mm_packus_epi16(x0,x0);
        _mm_storel_epi64((__m128i*)(pDst),x0);
    }
}

template <typename T>
VT_FORCEINLINE void 
Bilinear2BandBytex4Process(
    const __m128i& xf0123, const __m128i& yf0123,
    Byte* pDst,
    const T* pOffs, const uint16_t* pSrcPixBase, uint32_t srcPixStride)
{
    __m128i halfReg16 = _mm_set1_epi16(0x0080);
    __m128i unitReg16 = _mm_set1_epi16(0x0100);

    // compute sample weights
    // w#4 = |w#0 w#0 w#1 w#1 w#2 w#2 w#3 w#3| suitable for 2 band weighting
    __m128i wa4,wb4,wc4,wd4;
    {
        __m128i x0,x1;

        x1 = _mm_or_si128(xf0123,_mm_slli_si128(xf0123,2)); // |uf0 uf0 uf1 uf1 ...|
        wc4 = _mm_or_si128(yf0123,_mm_slli_si128(yf0123,2));// |vf0 vf0 vf1 vf1 ...|
        wd4 = _mm_mullo_epi16(x1,wc4);          // ufrac*vfrac
        wd4 = _mm_add_epi16(wd4,halfReg16);     // rounding bias
        wd4 = _mm_srli_epi16(wd4,8);            // wd
        wb4 = _mm_sub_epi16(x1,wd4);            // |wb0 wb0 wb1 wb1 ...|
        wc4 = _mm_sub_epi16(wc4,wd4);           // |wc0 wc0 wc1 wc1 ...|
        x0 = _mm_add_epi16(x1,wc4);             // ufrac + wc
        wa4 = _mm_sub_epi16(unitReg16,x0);      // wa = 1 - (ufrac + wc)
    }

    // load samples and filter
    {
        __m128i zeroReg = _mm_setzero_si128();
        // initialize samples to zero for oob handling; load if valid offset
        __m128i a4 = zeroReg;
        __m128i b4 = zeroReg;
        __m128i c4 = zeroReg;
        __m128i d4 = zeroReg;
#define LOAD_SRCPIX(_n_) \
        { \
            T offs = pOffs[_n_]; \
            if (offs >= 0) \
            { \
                a4 = _mm_insert_epi16(a4,*(pSrcPixBase+offs+0),_n_); \
                b4 = _mm_insert_epi16(b4,*(pSrcPixBase+offs+1),_n_); \
                c4 = _mm_insert_epi16(c4,*(pSrcPixBase+offs+0+srcPixStride),_n_); \
                d4 = _mm_insert_epi16(d4,*(pSrcPixBase+offs+1+srcPixStride),_n_); \
            } \
        }
        LOAD_SRCPIX(0) LOAD_SRCPIX(1) LOAD_SRCPIX(2) LOAD_SRCPIX(3)
#undef LOAD_SRCPIX
        // two channel data packed at bottom of a4..d4
        a4 = _mm_unpacklo_epi8(a4,zeroReg);
        b4 = _mm_unpacklo_epi8(b4,zeroReg);
        c4 = _mm_unpacklo_epi8(c4,zeroReg);
        d4 = _mm_unpacklo_epi8(d4,zeroReg);

        __m128i x0,x1;
        // multiply-accumulate samples*weights
        x0 = _mm_mullo_epi16(a4, wa4);
        x1 = _mm_mullo_epi16(b4, wb4);
        x0 = _mm_add_epi16(x0,x1);
        x1 = _mm_mullo_epi16(c4, wc4);
        x0 = _mm_add_epi16(x0,x1);
        x1 = _mm_mullo_epi16(d4, wd4);
        x0 = _mm_add_epi16(x0,x1);
        // add half and shift out fractional bits
        x0 = _mm_adds_epu16(x0,halfReg16);
        x0 = _mm_srli_epi16(x0,8);
        // pack to 8bpp and store
        x0 = _mm_packus_epi16(x0,x0);
        _mm_storel_epi64((__m128i*)(pDst),x0);
    }
}

template<typename T>
VT_FORCEINLINE void 
Bilinear4BandBytex4Process(
    const __m128i& xf0123, const __m128i& yf0123,
    Byte* pDst,
    const T* pOffs, const uint32_t* pSrcPixBase, uint32_t srcPixStride)
{
    __m128i halfReg16 = _mm_set1_epi16(0x0080);
    __m128i unitReg16 = _mm_set1_epi16(0x0100);

    // compute sample weights
    // wa = |wa0 xxx wa2 xxx wa1 xxx wa3 xxx|
    // same for b,c,d
    //
    __m128i wa,wb,wc,wd;
    {
        wd = _mm_mullo_epi16(xf0123,yf0123); 
        wd = _mm_add_epi16(wd,halfReg16);               
        wd = _mm_srli_epi16(wd,8);              // |d0 xx d1 xx d2 xx d3 xx|
        wc = _mm_sub_epi16(yf0123,wd);          // |c0 xx c1 xx c2 xx c3 xx|
        wb = _mm_sub_epi16(xf0123,wd);          // |b0 xx b1 xx b2 xx b3 xx|
        wa = _mm_sub_epi16(unitReg16,xf0123);   
        wa = _mm_sub_epi16(wa,wc);              // |a0 xx a1 xx a2 xx a3 xx|
    }

    // load samples and filter
    {
        __m128i zeroReg = _mm_setzero_si128();
        __m128i x0,x1,x2,x3,x4,x5,x6,x7;

        T offs = pOffs[0];
        if (offs >= 0)
        {
            x0 = _mm_loadl_epi64((const __m128i*)(pSrcPixBase+offs));
            x1 = _mm_loadl_epi64((const __m128i*)(pSrcPixBase+offs+srcPixStride));
        } else { x0 = x1 = zeroReg; }
        offs = pOffs[1];
        if (offs >= 0)
        {
            x2 = _mm_loadl_epi64((const __m128i*)(pSrcPixBase+offs));
            x3 = _mm_loadl_epi64((const __m128i*)(pSrcPixBase+offs+srcPixStride));
        } else { x2 = x3 = zeroReg; }

        x0 = _mm_unpacklo_epi8(x0,zeroReg); // a0,b0
        x1 = _mm_unpacklo_epi8(x1,zeroReg); // c0,d0
        x2 = _mm_unpacklo_epi8(x2,zeroReg); // a1,b1
        x3 = _mm_unpacklo_epi8(x3,zeroReg); // c1,d1
        x4 = _mm_unpacklo_epi64(x0,x2);     // a0,a1
        x5 = _mm_unpackhi_epi64(x0,x2);     // b0,b1
        x6 = _mm_unpacklo_epi64(x1,x3);     // c0,c1
        x7 = _mm_unpackhi_epi64(x1,x3);     // d0,d1

        {
            __m128i xt = _mm_setr_epi8(0,1,0,1,0,1,0,1,4,5,4,5,4,5,4,5);
            x0 = _mm_shuffle_epi8(wa,xt); // wa0,wa1
            x1 = _mm_shuffle_epi8(wb,xt); // wb0,wb1
            x2 = _mm_shuffle_epi8(wc,xt); // wc0,wc1
            x3 = _mm_shuffle_epi8(wd,xt); // wd0,wd1
        }

        // multiply-accumulate samples*weights
        x0 = _mm_mullo_epi16(x0, x4);
        x1 = _mm_mullo_epi16(x1, x5);
        x0 = _mm_add_epi16(x0,x1);
        x1 = _mm_mullo_epi16(x2, x6);
        x0 = _mm_add_epi16(x0,x1);
        x1 = _mm_mullo_epi16(x3, x7);
        x0 = _mm_add_epi16(x0,x1);
        // add half and shift out fractional bits
        x0 = _mm_adds_epu16(x0,halfReg16);
        x0 = _mm_srli_epi16(x0,8);
        // pack to 8bpp and store
        x0 = _mm_packus_epi16(x0,x0);
        _mm_storel_epi64((__m128i*)(pDst+0),x0);

        offs = pOffs[2];
        if (offs >= 0)
        {
            x0 = _mm_loadl_epi64((const __m128i*)(pSrcPixBase+offs));
            x1 = _mm_loadl_epi64((const __m128i*)(pSrcPixBase+offs+srcPixStride));
        } else { x0 = x1 = zeroReg; }
        offs = pOffs[3];
        if (offs >= 0)
        {
            x2 = _mm_loadl_epi64((const __m128i*)(pSrcPixBase+offs));
            x3 = _mm_loadl_epi64((const __m128i*)(pSrcPixBase+offs+srcPixStride));
        } else { x2 = x3 = zeroReg; }

        x0 = _mm_unpacklo_epi8(x0,zeroReg); // a2,b2
        x1 = _mm_unpacklo_epi8(x1,zeroReg); // c2,d2
        x2 = _mm_unpacklo_epi8(x2,zeroReg); // a3,b3
        x3 = _mm_unpacklo_epi8(x3,zeroReg); // c3,d3
        x4 = _mm_unpacklo_epi64(x0,x2);     // a2,a3
        x5 = _mm_unpackhi_epi64(x0,x2);     // b2,b3
        x6 = _mm_unpacklo_epi64(x1,x3);     // c2,c3
        x7 = _mm_unpackhi_epi64(x1,x3);     // d2,d3

        {
            __m128i xt = _mm_setr_epi8(8,9,8,9,8,9,8,9,12,13,12,13,12,13,12,13);
            x0 = _mm_shuffle_epi8(wa,xt); // wa2,wa3
            x1 = _mm_shuffle_epi8(wb,xt); // wb2,wb3
            x2 = _mm_shuffle_epi8(wc,xt); // wc2,wc3
            x3 = _mm_shuffle_epi8(wd,xt); // wd2,wd3
        }

        // multiply-accumulate samples*weights
        x0 = _mm_mullo_epi16(x0, x4);
        x1 = _mm_mullo_epi16(x1, x5);
        x0 = _mm_add_epi16(x0,x1);
        x1 = _mm_mullo_epi16(x2, x6);
        x0 = _mm_add_epi16(x0,x1);
        x1 = _mm_mullo_epi16(x3, x7);
        x0 = _mm_add_epi16(x0,x1);
        // add half and shift out fractional bits
        x0 = _mm_adds_epu16(x0,halfReg16);
        x0 = _mm_srli_epi16(x0,8);
        // pack to 8bpp and store
        x0 = _mm_packus_epi16(x0,x0);
        _mm_storel_epi64((__m128i*)(pDst+8),x0);
    }
}

#endif
