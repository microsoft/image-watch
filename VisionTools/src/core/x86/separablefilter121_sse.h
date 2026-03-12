//+----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Separable filter specialization for 121 kernel - SSE codepaths
//
//-----------------------------------------------------------------------------
#pragma once

#if defined(VT_SSE_SUPPORTED)

inline __m128i LoadSi128AlignedSSE(const __m128i* p)
{ return _mm_load_si128(p); }

inline __m128i LoadSi128UnAlignedSSE(const __m128i* p)
{ return _mm_loadu_si128(p); }

inline void StoreSi128AlignedSSE(__m128i* p, const __m128i& v)
{ _mm_store_si128(p, v); }

inline void StoreSi128UnAlignedSSE(__m128i* p, const __m128i& v)
{ _mm_storeu_si128(p, v); }


// macro to select templated function specializations
#define __SelectSSEFuncLS( __baligned, __ret, __fname, ... ) \
    if (__baligned) { __ret = __fname<LoadSi128AlignedSSE,StoreSi128AlignedSSE>(__VA_ARGS__); } \
 else { __ret = __fname<LoadSi128UnAlignedSSE,StoreSi128UnAlignedSSE>(__VA_ARGS__); }

#define __SelectSSEFuncL( __baligned, __ret, __fname, ... ) \
    if (__baligned) { __ret = __fname<LoadSi128AlignedSSE>(__VA_ARGS__); } \
else { __ret = __fname<LoadSi128UnAlignedSSE>(__VA_ARGS__); }

//+----------------------------------------------------------------------------
// 
// Monolithic routines (both vertical and horizontal in one pass)
//
//-----------------------------------------------------------------------------

// monolithic 16x1 121 for 1 band byte
// .28ms for 1Kx1K
// tried 16x4 and 16x2 versions and they were all slower due to increased
// inside loop instruction count for additional addressing
template<__m128i (*pfnLoad)(const __m128i*), void (*pfnStore)(__m128i*, const __m128i&)>
static inline int Mono_1BandByte16_SSE3( Byte* pDst, Byte firstvfp, bool bRExt, int w,
                                        const Byte* pT, const Byte* pC, const Byte* pB )
{
    const Byte* pTLast = pT+(w-1);
    int iret = w&(~15); // return number of destination pixels completed

    __m128i x0 = _mm_set1_epi8( firstvfp ); // broadcast first pixel to xmm reg
    __m128i x1,x2,x3,x4;

    // vertically filter first 16 to x1
    {
        // compute next 16 vertically-filtered pixels
        x1 = pfnLoad( (__m128i*)pT ); pT += 16;
        x2 = pfnLoad( (__m128i*)pC ); pC += 16;
        x3 = pfnLoad( (__m128i*)pB ); pB += 16;
        x1 = _mm_avg_epu8( x1, x3 ); // T+B
        x1 = _mm_avg_epu8( x1, x2 ); // T+C+C+B
    }
    // do 16's excluding last
    while ( w >= 32 )
    {
        // compute next 16 vertically-filtered pixels
        x2 = pfnLoad( (__m128i*)pT ); pT += 16;
        x3 = pfnLoad( (__m128i*)pC ); pC += 16;
        x4 = pfnLoad( (__m128i*)pB ); pB += 16;
        x4 = _mm_avg_epu8( x2, x4 ); // T+B
        x2 = _mm_avg_epu8( x3, x4 ); // T+C+C+B
        // x0,1,2 = left,center,right vertically filtered

        x3 = _mm_alignr_epi8( x1, x0, 15 ); // align 'Left' using last pixel of x0
        x4 = _mm_alignr_epi8( x2, x1, 1 );  // align 'Right' using first pixel of x2
        x3 = _mm_avg_epu8( x3, x4 ); // L+R
        x0 = _mm_avg_epu8( x3, x1 ); // L+C+C+R
        pfnStore( (__m128i*)pDst, x0 ); pDst += 16;

        x0 = x1; x1 = x2; // retain previous vertically filtered
        w -= 16;
    }
    // finish last 16
    {
        int offs = (bRExt&&(pT>pTLast))?(-1):(0);
        Byte nextvfp = filter121ByteAvg(*(pT+offs), *(pC+offs), *(pB+offs));
        x2 = _mm_set1_epi8(nextvfp);
        x3 = _mm_alignr_epi8( x1, x0, 15 ); // align 'Left' using last pixel of x0
        x4 = _mm_alignr_epi8( x2, x1, 1 );  // align 'Right' using first pixel of x2
        x3 = _mm_avg_epu8( x3, x4 ); // L+R
        x0 = _mm_avg_epu8( x3, x1 ); // L+C+C+R
        pfnStore( (__m128i*)pDst, x0 ); pDst += 16;
        w -= 16;
    }

    return iret;
}

// monolithic 8x1 121 w/ 2:1 decimation for 1 band byte
// .146ms for 1Kx1K
template<__m128i (*pfnLoad)(const __m128i*)>
static inline int Mono_1BandByte8_Deci_SSSE3( Byte* pDst, Byte firstvfp, int w,
                                        const Byte* pT, const Byte* pC, const Byte* pB )
{
    __m128i x0 = _mm_set1_epi8( firstvfp ); // broadcast first Center pixel to xmm reg
    int iret = w&(~7); // return number of destination pixels completed
    // mask to extract every other pixel using _mm_shuffle_epi8
    __m128i mask = _mm_set_epi8(-1,-1,-1,-1,-1,-1,-1,-1,14,12,10,8,6,4,2,0);
    while ( w >= 8 )
    {
        __m128i x1,x2,x3;
        // compute next 16 vertically-filtered pixels
        x1 = pfnLoad( (__m128i*)pT ); pT += 16;
        x2 = pfnLoad( (__m128i*)pC ); pC += 16;
        x3 = pfnLoad( (__m128i*)pB ); pB += 16;
        x1 = _mm_avg_epu8( x1, x3 ); // T+B
        x1 = _mm_avg_epu8( x1, x2 ); // T+C+C+B
        x2 = _mm_alignr_epi8( x1, x0, 15 ); // align 'Left' using last pixel of x0
        x3 = _mm_srli_si128( x1, 1); // align 'right' - top value is not used so zero is OK
        x3 = _mm_avg_epu8( x2, x3 ); // L+R
        x3 = _mm_avg_epu8( x3, x1 ); // L+C+C+R
        x3 = _mm_shuffle_epi8( x3, mask );
        _mm_storel_epi64( (__m128i*)pDst, x3 ); pDst += 8;
        x0 = x1; // retain last filtered Center pixel
        w -= 8;
    }
    return iret;
}

// monolithic 8x4 121 w/ 2:1 decimation for 1 band byte
// .126ms for 1Kx1K
template<__m128i (*pfnLoad)(const __m128i*)>
static inline int Mono_1BandByte8x4_Deci_SSSE3( 
    Byte* pDst, int dstStride, Bytex4 firstvfp, int w,
    const Byte* pT, const Byte* pC, int srcStride)
{
    const Byte* pCT;
    __m128i L0 = _mm_set1_epi8( firstvfp.b0 ); // broadcast first Center pixel to xmm reg
    __m128i L1 = _mm_set1_epi8( firstvfp.b1 ); // broadcast first Center pixel to xmm reg
    __m128i L2 = _mm_set1_epi8( firstvfp.b2 ); // broadcast first Center pixel to xmm reg
    __m128i L3 = _mm_set1_epi8( firstvfp.b3 ); // broadcast first Center pixel to xmm reg
    int iret = w&(~7); // return number of destination pixels completed
    // mask to extract every other pixel using _mm_shuffle_epi8
    __m128i mask = _mm_set_epi8(-1,-1,-1,-1,-1,-1,-1,-1,14,12,10,8,6,4,2,0);
    while ( w >= 8 )
    {
        __m128i C0,C1,C2,C3;
        {
            __m128i x0,x1,x2,x3,x4;
            // compute next 16 vertically-filtered pixels
            x0 = pfnLoad( (__m128i*)pT ); pT += 16;
            C0 = pfnLoad( (__m128i*)pC ); pCT = pC+srcStride; pC += 16;
            x1 = pfnLoad( (__m128i*)pCT ); pCT += srcStride;
            C1 = pfnLoad( (__m128i*)pCT ); pCT += srcStride;
            x2 = pfnLoad( (__m128i*)pCT ); pCT += srcStride;
            C2 = pfnLoad( (__m128i*)pCT ); pCT += srcStride;
            x3 = pfnLoad( (__m128i*)pCT ); pCT += srcStride;
            C3 = pfnLoad( (__m128i*)pCT ); pCT += srcStride;
            x4 = pfnLoad( (__m128i*)pCT );
            x0 = _mm_avg_epu8( x0, x1 ); // T+B
            C0 = _mm_avg_epu8( C0, x0 ); // T+C+C+B
            x0 = _mm_avg_epu8( x1, x2 ); // T+B
            C1 = _mm_avg_epu8( C1, x0 ); // T+C+C+B
            x0 = _mm_avg_epu8( x2, x3 ); // T+B
            C2 = _mm_avg_epu8( C2, x0 ); // T+C+C+B
            x0 = _mm_avg_epu8( x3, x4 ); // T+B
            C3 = _mm_avg_epu8( C3, x0 ); // T+C+C+B
        }

        {
            __m128i x0,x1;
            x0 = _mm_alignr_epi8( C0, L0, 15 ); // align 'Left' using last pixel of x0
            L0 = C0; // retain last filtered center pixel
            x1 = _mm_srli_si128( C0, 1); // align 'right' - top value is not used so zero is OK
            x0 = _mm_avg_epu8( x0, x1 ); // L+R
            x0 = _mm_avg_epu8( C0, x0 ); // L+C+C+R
            x0 = _mm_shuffle_epi8( x0, mask );
            _mm_storel_epi64( (__m128i*)pDst, x0 ); 
        }
        Byte* pDstT = pDst+dstStride;
        {
            __m128i x0,x1;
            x0 = _mm_alignr_epi8( C1, L1, 15 ); // align 'Left' using last pixel of x0
            L1 = C1; // retain last filtered center pixel
            x1 = _mm_srli_si128( C1, 1); // align 'right' - top value is not used so zero is OK
            x0 = _mm_avg_epu8( x0, x1 ); // L+R
            x0 = _mm_avg_epu8( C1, x0 ); // L+C+C+R
            x0 = _mm_shuffle_epi8( x0, mask );
            _mm_storel_epi64( (__m128i*)pDstT, x0 ); 
        }
        pDstT += dstStride;
        {
            __m128i x0,x1;
            x0 = _mm_alignr_epi8( C2, L2, 15 ); // align 'Left' using last pixel of x0
            L2 = C2; // retain last filtered center pixel
            x1 = _mm_srli_si128( C2, 1); // align 'right' - top value is not used so zero is OK
            x0 = _mm_avg_epu8( x0, x1 ); // L+R
            x0 = _mm_avg_epu8( C2, x0 ); // L+C+C+R
            x0 = _mm_shuffle_epi8( x0, mask );
            _mm_storel_epi64( (__m128i*)pDstT, x0 ); 
        }
        pDstT += dstStride;
        {
            __m128i x0,x1;
            x0 = _mm_alignr_epi8( C3, L3, 15 ); // align 'Left' using last pixel of x0
            L3 = C3; // retain last filtered center pixel
            x1 = _mm_srli_si128( C3, 1); // align 'right' - top value is not used so zero is OK
            x0 = _mm_avg_epu8( x0, x1 ); // L+R
            x0 = _mm_avg_epu8( C3, x0 ); // L+C+C+R
            x0 = _mm_shuffle_epi8( x0, mask );
            _mm_storel_epi64( (__m128i*)pDstT, x0 ); 
        }

        pDst += 8;
        w -= 8;
    }
    return iret;
}


//+----------------------------------------------------------------------------
// 
// Higher Precision Monolithics - not currently used
//
//-----------------------------------------------------------------------------
// monolithic 16x1 121 for 1 band byte
// .86ms for 1Kx1K, vs 1.0ms for non-monolithic...
template<__m128i (*pfnLoad)(const __m128i*), void (*pfnStore)(__m128i*, const __m128i&)>
static inline int MonoHP_1BandByte16_SSE3( 
    Byte* pDst, uint16_t firstvfp, bool bRExt, int w,
    const Byte* pT, const Byte* pC, const Byte* pB )
{
    const Byte* pTLast = pT+(w-1);
    int iret = w&(~7); // return number of destination pixels completed

    __m128i x0 = _mm_set1_epi16( firstvfp ); // broadcast first pixel to xmm reg
    __m128i x1,x2,x3,x4;
    __m128i zeroReg = _mm_setzero_si128();
    __m128i halfReg = _mm_set1_epi16(0x0008);

    // vertically filter first 8 to x1
    {
        // load 8 (8 bit) values for center and top/bottom rows
        x1 = _mm_loadl_epi64((__m128i*)pT); pT+=8;
        x2 = _mm_loadl_epi64((__m128i*)pC); pC+=8;
        x3 = _mm_loadl_epi64((__m128i*)pB); pB+=8;
        // unpack 8 pixels to 16bpp in registers
        x1 = _mm_unpacklo_epi8(x1,zeroReg);
        x2 = _mm_unpacklo_epi8(x2,zeroReg);
        x3 = _mm_unpacklo_epi8(x3,zeroReg);
        // do (top + 2*center + bottom)
        x2 = _mm_add_epi16(x2,x2); // C+C
        x1 = _mm_add_epi16(x1,x2); // L+(C+C)
        x1 = _mm_add_epi16(x1,x3); // R+(L+C+C)
    }

    // do 8's excluding last
    while ( w >= 8 )
    {
        // compute next 16 vertically-filtered pixels
        // load 8 (8 bit) values for center and top/bottom rows
        x2 = _mm_loadl_epi64((__m128i*)pT); pT+=8;
        x3 = _mm_loadl_epi64((__m128i*)pC); pC+=8;
        x4 = _mm_loadl_epi64((__m128i*)pB); pB+=8;
        // unpack 8 pixels to 16bpp in registers
        x2 = _mm_unpacklo_epi8(x2,zeroReg);
        x3 = _mm_unpacklo_epi8(x3,zeroReg);
        x4 = _mm_unpacklo_epi8(x4,zeroReg);
        // do (top + 2*center + bottom)
        x3 = _mm_add_epi16(x3,x3); // C+C
        x2 = _mm_add_epi16(x2,x3); // L+(C+C)
        x2 = _mm_add_epi16(x2,x4); // R+(L+C+C)
        // x0,1,2 = left,center,right vertically filtered

        x3 = _mm_alignr_epi8( x1, x0, 14 ); // align 'Left' using last 2xBytes of x0
        x4 = _mm_alignr_epi8( x2, x1, 2 );  // align 'Right' using first 2xBytes of x2
        x0 = _mm_add_epi16(x1,x1); // C+C
        x0 = _mm_add_epi16(x0,x3); // L+(C+C)
        x0 = _mm_add_epi16(x0,x4); // R+(L+C+C)
        x0 = _mm_add_epi16(x0,halfReg); // round and truncate
        x0 = _mm_srli_epi16(x0,4);
        x0 = _mm_packus_epi16(x0, x0); // pack to 8bpp
        _mm_storel_epi64( (__m128i*)pDst, x0 ); pDst += 8;

        x0 = x1; x1 = x2; // retain previous vertically filtered
        w -= 8;
    }
    // finish last 8
    {
        int offs = (bRExt&&(pT>pTLast))?(-1):(0);
        uint16_t nextvfp = filter121A(*(pT+offs), *(pC+offs), *(pB+offs));
        x2 = _mm_set1_epi16(nextvfp);

        x3 = _mm_alignr_epi8( x1, x0, 14 ); // align 'Left' using last 2xBytes of x0
        x4 = _mm_alignr_epi8( x2, x1, 2 );  // align 'Right' using first 2xBytes of x2
        x0 = _mm_add_epi16(x2,x2); // C+C
        x0 = _mm_add_epi16(x0,x3); // L+(C+C)
        x0 = _mm_add_epi16(x0,x4); // R+(L+C+C)
        x0 = _mm_add_epi16(x0,halfReg); // round and truncate
        x0 = _mm_srli_epi16(x0,4);
        x0 = _mm_packus_epi16(x0, x0); // pack to 8bpp
        _mm_storel_epi64( (__m128i*)pDst, x0 );
    }
    return iret;
}

//+----------------------------------------------------------------------------
// 
// Vertical Filtering 
//
//-----------------------------------------------------------------------------

// vertically filter one scanline, band count included in w
static inline void VFilter_SSE3(
    const Byte*& pC, const Byte*& pT, const Byte*& pB, UInt16*& pTDst, int& w)
{
#if 1
    __m128i zeroReg = _mm_setzero_si128();
    while (w >= 8)
    {
        // load 8 (8 bit) values for center and top/bottom rows
        __m128i pix8C = _mm_loadl_epi64((__m128i*)pC); pC+=8;
        __m128i pix8T = _mm_loadl_epi64((__m128i*)pT); pT+=8;
        __m128i pix8B = _mm_loadl_epi64((__m128i*)pB); pB+=8;
        // unpack 8 pixels to 16bpp in registers
        pix8C = _mm_unpacklo_epi8(pix8C,zeroReg);
        pix8T = _mm_unpacklo_epi8(pix8T,zeroReg);
        pix8B = _mm_unpacklo_epi8(pix8B,zeroReg);
        // do (top + 2*center + bottom)
        pix8C = _mm_add_epi16(pix8C,pix8C);
        pix8C = _mm_add_epi16(pix8C,pix8T);
        pix8C = _mm_add_epi16(pix8C,pix8B);
        // store them to 16bpp (16 byte aligned) temp buffer
        _mm_store_si128((__m128i*)pTDst,pix8C); pTDst+=8;
        w-=8;
    }
#else
    // TODO: I expected this to be significantly faster but it is not so
    // since it is also less accurate I'll keep it out for now
    __m128i zeroReg = _mm_setzero_si128();
    while (w >= 16)
    {
        // load 16 (8 bit) values for center and top/bottom rows
        __m128i pixC = _mm_loadu_si128((__m128i*)pC);
        __m128i pixT = _mm_loadu_si128((__m128i*)pT);
        __m128i pixB = _mm_loadu_si128((__m128i*)pB);
        // apply 121 vertically
        __m128i res = _mm_avg_epu8( pixC, pixC ); // C+C - averaging in this order avoids introducing a bias
        __m128i tmp = _mm_avg_epu8( pixT, pixB ); // T+B
        res = _mm_avg_epu8( res, tmp ); // T+C+C+B
        // unpack to 16bpp and scale to 8.2 to store to temp buffer
        tmp = _mm_unpacklo_epi8(res,zeroReg);
        tmp = _mm_slli_epi16(tmp,2);
        res = _mm_unpackhi_epi8(res,zeroReg);
        res = _mm_slli_epi16(res,2);
        // store them to 16bpp (16 byte aligned) temp buffer
        _mm_store_si128((__m128i*)pTDst,tmp); pTDst+=8;
        _mm_store_si128((__m128i*)pTDst,res); pTDst+=8;
        pC+=16;
        pT+=16;
        pB+=16;
        w-=16;
    }
#endif
}

static inline void VFilter_SSE3(
    const float*& pC, const float*& pT, const float*& pB, float*& pTDst, int& w)
{
    __m128 normReg = _mm_set1_ps(.25f);
    while (w >= 4)
    {
        // load 4 values for center and top/bottom rows
        __m128 pix4C = _mm_loadu_ps(pC);
        __m128 pix4T = _mm_loadu_ps(pT);
        __m128 pix4B = _mm_loadu_ps(pB);
        // (T+2*C+B)*.25f
        pix4C = _mm_add_ps(pix4C,pix4C);
        pix4C = _mm_add_ps(pix4C,pix4T);
        pix4C = _mm_add_ps(pix4C,pix4B);
        pix4C = _mm_mul_ps(pix4C,normReg);
        // store
        _mm_store_ps(pTDst, pix4C);
        pTDst += 4;
        pC += 4;
        pT += 4;
        pB += 4;
        w  -= 4;
    }
}

//+----------------------------------------------------------------------------
// 
// Horizontal Filtering 
//
//-----------------------------------------------------------------------------

// no SSE for decimate Byte - too many instructions to do alignment & packing?
static inline void HFilter_Deci_SSE2(Byte*& pDst, const UInt16*& pL, 
    int& w, int nBnds)
{
    VT_USE(pDst); VT_USE(pL); VT_USE(w); VT_USE(nBnds);
}

static inline void HFilter_Deci_SSE2(float*& pDst, const float*& pL, 
    int& w, int nBnds)
{
    if (nBnds != 1) return;
    // with 'previous,current,next' of |xxxA|,|BCDE|,|FGHI|
    // 4 results are {A+2B+C;C+2D+E;E+2F+G;G=2H+I}, so need to form
    // vectors of |ACEG|,|BDFH|,|CEGI| to align with results
    __m128 normReg = _mm_set1_ps(.25f);
    __m128 next4P = _mm_load1_ps(pL); pL++;
    while (w >= 8)
    {
        __m128 prev4P = next4P;          // xxxA
        __m128 curr4P = _mm_load_ps(pL); // BCDE
        next4P = _mm_load_ps(pL+4);      // FGHI
        __m128 t0 = _mm_shuffle_ps(curr4P,next4P,_MM_SHUFFLE(2,0,2,0)); // BDFH
        __m128 t1 = _mm_shuffle_ps(curr4P,next4P,_MM_SHUFFLE(3,1,3,1)); // CEGI
        __m128 t2 = _mm_shuffle_ps(prev4P,curr4P,_MM_SHUFFLE(1,1,3,3)); // AxCx
        t2 = _mm_shuffle_ps(t2,t1,_MM_SHUFFLE(2,1,2,0)); // ACEG
        t0 = _mm_add_ps(t0,t0);
        t0 = _mm_add_ps(t0,t1);
        t0 = _mm_add_ps(t0,t2);
        t0 = _mm_mul_ps(t0,normReg);
        _mm_storeu_ps(pDst,t0); 
        //
        pL += 8;
        pDst += 4;
        w -= 8;
    }
    pL--; // for remainder processing
}

static inline void HFilter_SSE3(Byte*& pDst, const UInt16*& pL, int& w, int nBnds)
{
    // Horizontal filtering holds 24 consecutive channels in three vectors of 8
    // each; the first 8 channels are 'previous', the middle 8 are 'current', and the
    // last 8 are 'next'.  The filtering is applied to the current 8 channels,
    // and the previous and next are used to obtain all of the horizontally
    // neighboring channels for the current 8 channels.
    //
    // Two SSE 128 bit shifts (which zero extend) followed by a logical OR are used 
    // to extract and align the neighboring channels for the left and right filter
    // coefficients.  The shift amount is a function of the number of bands, where
    // for 1 band the shift is 1 or 7, for four bands the shift is 4, etc.
    //
    __m128i biasEPI16 = _mm_set1_epi16(0x8); // 1/2 value (no immediate add...)
    __m128i curr8P = _mm_load_si128((__m128i*)(pL-(8-nBnds))); // preload with previous 8
    __m128i next8P = _mm_load_si128((__m128i*)(pL+(nBnds))); // preload with current 8
    //
    // using macros here because: (1) SSE2 128 bit shift is immediate shift value only,
    // requiring the use of one layer of macro; (2) a single _mm_storeu_ps is quite a
    // bit faster than two _mm_storel_epi64, so the code does two passes of 8 channels
    // per pass and then packs and stores all 16 results at once (using a second macro
    // layer to avoid a loop and conditional copy)
    //
    // (these macros could be templated inline functions, but that would require moving
    // the inside loop code out of the function which would make it harder to read)
    //
#define SSE_FILTER121_B1i(__res8, __nBnds) \
    { \
        __m128i prev8P = curr8P; /* old current is now previous */ \
        curr8P = next8P; /* old next is now current */ \
        next8P = _mm_load_si128((__m128i*)(pL+(8+(__nBnds)))); pL += 8; \
        /* form aligned left and right vectors */ \
        __m128i chnL = _mm_srli_si128(curr8P,2*(__nBnds)); \
        chnL = _mm_or_si128(chnL, _mm_slli_si128(next8P,2*(8-(__nBnds)))); \
        __m128i chnR = _mm_slli_si128(curr8P,2*(__nBnds)); \
        chnR = _mm_or_si128(chnR, _mm_srli_si128(prev8P,2*(8-(__nBnds)))); \
        /* do ((left + 2*center + right + 0x8)>>4) */ \
        __res8 = _mm_add_epi16(curr8P,curr8P); \
        __res8 = _mm_add_epi16(__res8, chnR); \
        __res8 = _mm_add_epi16(__res8, chnL); \
        __res8 = _mm_add_epi16(__res8, biasEPI16); \
        __res8 = _mm_srli_epi16(__res8, 4); \
    }
#define SSE_FILTER121_B(__nBnds) \
    while (w >= 16) \
    { \
        __m128i res8h,res8l; \
        SSE_FILTER121_B1i(res8h,(__nBnds)); \
        SSE_FILTER121_B1i(res8l,(__nBnds)); \
        /* pack to 8bpp and store to dest */ \
        _mm_store_ps((float*)pDst,_mm_castsi128_ps(_mm_packus_epi16(res8h, res8l))); \
        pDst+=16; \
        w-=16; \
    }
#define SSE_FILTER121_B_U(__nBnds) \
    while (w >= 16) \
    { \
        __m128i res8h,res8l; \
        SSE_FILTER121_B1i(res8h,(__nBnds)); \
        SSE_FILTER121_B1i(res8l,(__nBnds)); \
        /* pack to 8bpp and store to dest */ \
        _mm_storeu_ps((float*)pDst,_mm_castsi128_ps(_mm_packus_epi16(res8h, res8l))); \
        pDst+=16; \
        w-=16; \
    }
    // per-band code specialization
    if (IsAligned16(pDst))
    {
        if (nBnds == 1)      { SSE_FILTER121_B(1) }
        else if (nBnds == 2) { SSE_FILTER121_B(2) }
        else if (nBnds == 3) { SSE_FILTER121_B(3) }
        else                 { SSE_FILTER121_B(4) }
    }
    else
    {
        if (nBnds == 1)      { SSE_FILTER121_B_U(1) }
        else if (nBnds == 2) { SSE_FILTER121_B_U(2) }
        else if (nBnds == 3) { SSE_FILTER121_B_U(3) }
        else                 { SSE_FILTER121_B_U(4) }
    }
}

static inline void HFilter_SSE3(float*& pDst, const float*& pL, int& w, int nBnds)
{
    if (nBnds != 1) return;
    // with 'previous,current,next' of |xxxA|,|BCDE|,|FGHI|
    // 4 results are {A+2B+C;B+2C+D;C+2D+E;D+2E+F}, so need to form
    // vectors of |ABCD|,|BCDE|,|CDEF| to align with results
    __m128 normReg = _mm_set1_ps(.25f);
    __m128 curr4P = _mm_load1_ps(pL); pL++; 
    __m128 next4P = _mm_load_ps(pL); pL+=4;
    while (w >= 4)
    {
        __m128 prev4P = curr4P;     // xxxA
        curr4P = next4P;            // BCDE
        next4P = _mm_load_ps(pL);   // FGHI
        __m128i it0 = _mm_srli_si128(_mm_castps_si128(prev4P),12); // A000
        __m128i it1 = _mm_slli_si128(_mm_castps_si128(curr4P),4);  // 0BCD
        __m128 t0 = _mm_castsi128_ps(_mm_or_si128(it0,it1));       // ABCD
        it0 = _mm_srli_si128(_mm_castps_si128(curr4P),4);          // CDE0
        it1 = _mm_slli_si128 (_mm_castps_si128(next4P),12);        // 000F
        __m128 t1 = _mm_castsi128_ps(_mm_or_si128(it0,it1));       // CDEF
        t0 = _mm_add_ps(t0,t1);
        t0 = _mm_add_ps(t0,curr4P);
        t0 = _mm_add_ps(t0,curr4P);
        t0 = _mm_mul_ps(t0,normReg);
        _mm_storeu_ps(pDst,t0); 
        //
        pL += 4;
        pDst += 4;
        w -= 4;
    }
    pL -= 5; // for remainder processing
}

#endif


//+-----------------------------------------------------------------------------
