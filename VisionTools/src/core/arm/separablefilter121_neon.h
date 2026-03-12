//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Separable filter specialization for 121 kernel for Arm/Neon
//------------------------------------------------------------------------
#pragma once

#if defined(VT_NEON_SUPPORTED)

//+----------------------------------------------------------------------------
// 
// Monolithic routines (both vertical and horizontal in one pass)
//
//-----------------------------------------------------------------------------

static inline int Mono_1BandByte16_Neon( Byte* pDst, Byte firstvfp, bool bRExt, int w,
                                        const Byte* pT, const Byte* pC, const Byte* pB )
{
    const Byte* pTLast = pT+(w-1);
    int iret = w&(~15); // return number of destination pixels completed

    uint8x16_t x0 = vmovq_n_u8( firstvfp ); // broadcast first pixel
    uint8x16_t x1,x2,x3,x4;

    // vertically filter first 16 to x1
    {
        // compute next 16 vertically-filtered pixels
        x1 = vld1q_u8(pT); pT += 16;
        x2 = vld1q_u8(pC); pC += 16;
        x3 = vld1q_u8(pB); pB += 16;
        x1 = vrhaddq_u8( x1, x3 ); // T+B
        x1 = vrhaddq_u8( x1, x2 ); // T+C+C+B
    }
    // do 16's excluding last
    while ( w >= 32 )
    {
        // compute next 16 vertically-filtered pixels
        x2 = vld1q_u8(pT); pT += 16;
        x3 = vld1q_u8(pC); pC += 16;
        x4 = vld1q_u8(pB); pB += 16;
        x4 = vrhaddq_u8( x2, x4 ); // T+B
        x2 = vrhaddq_u8( x3, x4 ); // T+C+C+B
        // x0,1,2 = left,center,right vertically filtered

        x3 = vextq_u8( x0, x1, 15 ); // align 'Left' using last pixel of x0
        x4 = vextq_u8( x1, x2, 1 );  // align 'Right' using first pixel of x2
        x3 = vrhaddq_u8( x3, x4 ); // L+R
        x0 = vrhaddq_u8( x3, x1 ); // L+C+C+R
        vst1q_u8(pDst, x0 ); pDst += 16;

        x0 = x1; x1 = x2; // retain previous vertically filtered
        w -= 16;
    }
    // finish last 16
    {
        int offs = (bRExt&&(pT>pTLast))?(-1):(0);
        Byte nextvfp = filter121ByteAvg(*(pT+offs), *(pC+offs), *(pB+offs));
        x2 = vmovq_n_u8(nextvfp);
        x3 = vextq_u8( x0, x1, 15 ); // align 'Left' using last pixel of x0
        x4 = vextq_u8( x1, x2, 1 );  // align 'Right' using first pixel of x2
        x3 = vrhaddq_u8( x3, x4 ); // L+R
        x0 = vrhaddq_u8( x3, x1 ); // L+C+C+R
        vst1q_u8(pDst, x0 ); pDst += 16;
        w -= 16;
    }

    return iret;
}

// monolithic 2:1 decimation for 1 band byte
static inline int Mono_1BandByte8_Deci_Neon( Byte* pDst, Byte firstvfp, int w,
    const Byte* pT, const Byte* pC, const Byte* pB )
{
    uint8x16_t x0 = vmovq_n_u8( firstvfp ); // broadcast first Center pixel to xmm reg
    int iret = w&(~7); // return number of destination pixels completed
    while ( w >= 8 )
    {
        uint8x16_t x1,x2,x3;
        // compute next 16 vertically-filtered pixels
        x1 = vld1q_u8(pT); pT += 16;
        x2 = vld1q_u8(pC); pC += 16;
        x3 = vld1q_u8(pB); pB += 16;
        x1 = vrhaddq_u8( x1, x3 ); // T+B
        x1 = vrhaddq_u8( x1, x2 ); // T+C+C+B

        x2 = vextq_u8( x1, x0, 15 ); // align 'Left' using last pixel of x0
        x3 = vextq_u8( x1, x1, 1); // align 'right' - top value is not used so anything is OK
        x3 = vrhaddq_u8( x2, x3 ); // L+R
        x3 = vrhaddq_u8( x3, x1 ); // L+C+C+R
        uint8x16x2_t x4 = vuzpq_u8( x3, x3 );
        vst1_u8(pDst, vget_low_u8(x4.val[0])); pDst += 8;
        x0 = x1; // retain last filtered Center pixel
        w -= 8;
    }
    return iret;
}
// monolithic 8x4 121 w/ 2:1 decimation for 1 band byte
static inline int Mono_1BandByte8x4_Deci_Neon( 
    Byte* pDst, int dstStride, Bytex4 firstvfp, int w,
    const Byte* pT, const Byte* pC, int srcStride)
{
    const Byte* pCT;
    uint8x16_t L0 = vmovq_n_u8( firstvfp.b0 ); // broadcast first Center pixel to xmm reg
    uint8x16_t L1 = vmovq_n_u8( firstvfp.b1 ); // broadcast first Center pixel to xmm reg
    uint8x16_t L2 = vmovq_n_u8( firstvfp.b2 ); // broadcast first Center pixel to xmm reg
    uint8x16_t L3 = vmovq_n_u8( firstvfp.b3 ); // broadcast first Center pixel to xmm reg
    int iret = w&(~7); // return number of destination pixels completed
    while ( w >= 8 )
    {
        uint8x16_t C0,C1,C2,C3;
        {
            uint8x16_t x0,x1,x2,x3,x4;
            // compute next 16 vertically-filtered pixels
            x0 = vld1q_u8(pT ); pT += 16;
            C0 = vld1q_u8(pC ); pCT = pC+srcStride; pC += 16;
            x1 = vld1q_u8(pCT ); pCT += srcStride;
            C1 = vld1q_u8(pCT ); pCT += srcStride;
            x2 = vld1q_u8(pCT ); pCT += srcStride;
            C2 = vld1q_u8(pCT ); pCT += srcStride;
            x3 = vld1q_u8(pCT ); pCT += srcStride;
            C3 = vld1q_u8(pCT ); pCT += srcStride;
            x4 = vld1q_u8(pCT );
            x0 = vrhaddq_u8( x0, x1 ); // T+B
            C0 = vrhaddq_u8( C0, x0 ); // T+C+C+B
            x0 = vrhaddq_u8( x1, x2 ); // T+B
            C1 = vrhaddq_u8( C1, x0 ); // T+C+C+B
            x0 = vrhaddq_u8( x2, x3 ); // T+B
            C2 = vrhaddq_u8( C2, x0 ); // T+C+C+B
            x0 = vrhaddq_u8( x3, x4 ); // T+B
            C3 = vrhaddq_u8( C3, x0 ); // T+C+C+B
        }

        {
            uint8x16_t x0,x1;
            x0 = vextq_u8( L0, C0, 15 ); // align 'Left' using last pixel of x0
            L0 = C0; // retain last filtered center pixel
            x1 = vextq_u8( C0, C0, 1); // align 'right' - top value is not used so anything is OK
            x0 = vrhaddq_u8( x0, x1 ); // L+R
            x0 = vrhaddq_u8( C0, x0 ); // L+C+C+R
            {
                uint8x16x2_t x3 = vuzpq_u8( x0, x0 );
                vst1_u8(pDst, vget_low_u8(x3.val[0]));
            }
        }
        Byte* pDstT = pDst+dstStride;
        {
            uint8x16_t x0,x1;
            x0 = vextq_u8( L1, C1, 15 ); // align 'Left' using last pixel of x0
            L1 = C1; // retain last filtered center pixel
            x1 = vextq_u8( C1, C1, 1); // align 'right' - top value is not used so anything is OK
            x0 = vrhaddq_u8( x0, x1 ); // L+R
            x0 = vrhaddq_u8( C1, x0 ); // L+C+C+R
            {
                uint8x16x2_t x3 = vuzpq_u8( x0, x0 );
                vst1_u8(pDstT, vget_low_u8(x3.val[0]));
            }
        }
        pDstT += dstStride;
        {
            uint8x16_t x0,x1;
            x0 = vextq_u8( L2, C2, 15 ); // align 'Left' using last pixel of x0
            L2 = C2; // retain last filtered center pixel
            x1 = vextq_u8( C2, C2, 1); // align 'right' - top value is not used so anything is OK
            x0 = vrhaddq_u8( x0, x1 ); // L+R
            x0 = vrhaddq_u8( C2, x0 ); // L+C+C+R
            {
                uint8x16x2_t x3 = vuzpq_u8( x0, x0 );
                vst1_u8(pDstT, vget_low_u8(x3.val[0]));
            }
        }
        pDstT += dstStride;
        {
            uint8x16_t x0,x1;
            x0 = vextq_u8( L3, C3, 15 ); // align 'Left' using last pixel of x0
            L3 = C3; // retain last filtered center pixel
            x1 = vextq_u8( C3, C3, 1); // align 'right' - top value is not used so anything is OK
            x0 = vrhaddq_u8( x0, x1 ); // L+R
            x0 = vrhaddq_u8( C3, x0 ); // L+C+C+R
            {
                uint8x16x2_t x3 = vuzpq_u8( x0, x0 );
                vst1_u8(pDstT, vget_low_u8(x3.val[0]));
            }
        }

        pDst += 8;
        w -= 8;
    }
    return iret;
}


//+----------------------------------------------------------------------------
// 
// Vertical Filtering 
//
//-----------------------------------------------------------------------------

// vertically filter one scanline, band count included in w
static inline void VFilter_Neon(
    const Byte*& pC, const Byte*& pT, const Byte*& pB, UInt16*& pTDst, int& w)
{
    while (w >= 16)
    {
        uint8x8_t pix8Ca = vld1_u8(pC);
        uint8x8_t pix8Ta = vld1_u8(pT);
        uint8x8_t pix8Ba = vld1_u8(pB);
        pC+=8;
        pT+=8;
        pB+=8;
        uint8x8_t pix8Cb = vld1_u8(pC);
        uint8x8_t pix8Tb = vld1_u8(pT);
        uint8x8_t pix8Bb = vld1_u8(pB);
        pC+=8;
        pT+=8;
        pB+=8;
        uint16x8_t acc1 = vaddl_u8(pix8Ca,pix8Ca); // addl does 8->16 convert and add
        uint16x8_t acc2 = vaddl_u8(pix8Ta,pix8Ba);
        acc1 = vaddq_u16(acc1,acc2);
        vst1q_u16(pTDst,acc1);
        pTDst+=8;
        w-=8;
        acc1 = vaddl_u8(pix8Cb,pix8Cb); // addl does 8->16 convert and add
        acc2 = vaddl_u8(pix8Tb,pix8Bb);
        acc1 = vaddq_u16(acc1,acc2);
        vst1q_u16(pTDst,acc1);
        pTDst+=8;
        w-=8;
    }
    while (w >= 8)
    {
        uint8x8_t pix8C = vld1_u8(pC);
        uint8x8_t pix8T = vld1_u8(pT);
        uint8x8_t pix8B = vld1_u8(pB);
        uint16x8_t acc1 = vaddl_u8(pix8C,pix8C); // addl does 8->16 convert and add
        uint16x8_t acc2 = vaddl_u8(pix8T,pix8B);
        acc1 = vaddq_u16(acc1,acc2);
        vst1q_u16(pTDst,acc1);
        pTDst+=8;
        pC+=8;
        pT+=8;
        pB+=8;
        w-=8;
    }
}

static inline void VFilter_Neon(
    const float*& pC, const float*& pT, const float*& pB, float*& pTDst, int& w)
{
    VT_USE(pC); VT_USE(pT); VT_USE(pB); VT_USE(pTDst); VT_USE(w);
}

static inline void HFilter_Neon(Byte*& pDst, const UInt16*& pL, int& w, int nBnds)
{
    if (nBnds == 1)
    {
        pL += 1;
        uint16x8_t curr8P = vld1q_u16(pL-8);
        uint16x8_t next8P = vld1q_u16(pL);
        while (w >= 8)
        {
            pL += 8;
            uint16x8_t prev8P = curr8P;
            curr8P = next8P;
            next8P = vld1q_u16(pL);

            uint16x8_t left8P = vextq_u16(prev8P,curr8P,7);
            uint16x8_t rght8P = vextq_u16(curr8P,next8P,1);

            uint16x8_t res8P = vaddq_u16(curr8P,curr8P);
            res8P = vaddq_u16(res8P,left8P);
            res8P = vaddq_u16(res8P,rght8P);

            uint8x8_t res8 = vqrshrn_n_u16(res8P,4);
            vst1_u8(pDst,res8); pDst+=8;
            w -= 8;
        }
        pL -= 1;
    }
}

static inline void HFilter_Neon(float*& pDst, const float*& pL, int& w, int nBnds)
{
    VT_USE(pDst); VT_USE(pL); VT_USE(w); VT_USE(nBnds);
}

#endif
//+-----------------------------------------------------------------------------
