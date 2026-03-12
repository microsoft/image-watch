//+----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      SSE routines for bilinear sampling for warp, resize, etc.
//
//-----------------------------------------------------------------------------
#pragma once

#if defined(VT_NEON_SUPPORTED)

// Neon support required for ARM
#define VTBILINEAR_SIMD_SUPPORTED true

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
    const uint16x8_t& xf0to7, const uint16x8_t& yf0to7,
    Byte* pDst,
    const T* pOffs, const Byte* pSrcPixBase, uint32_t srcPixStride)
{
    static const uint8_t zeroMem[2] = { 0, 0 };

    uint16x8_t wa,wb,wc,wd;     // weights for source pixels abcd
    {
        wa = vdupq_n_u16(0x100);    // 1
        wd = vmulq_u16(xf0to7,yf0to7);
        wd = vrshrq_n_u16(wd,8);    // normalized x*y
        wc = vsubq_u16(yf0to7,wd);  // y - (x*y)
        wa = vsubq_u16(wa,xf0to7);  // 1 - x
        wb = vsubq_u16(xf0to7,wd);  // x - (x*y)
        wa = vsubq_u16(wa,wc);      // 1-x-(y-(x*y))
    }

    // load samples and filter
    {
        // the Arm compiler insists on using a single set of registers for all
        // uint8x8x2_t variables and doing lots of copying back and forth, so
        // need to have just one and do the offset conditional twice as many
        // times as would otherwise be necessary
#define LOAD_SRCPIX(_n_,_reg,_ptr) \
        { T offs = pOffs[_n_]; \
            if (offs >= 0) { \
                _reg = vld2_lane_u8(_ptr+offs,_reg,_n_); \
            } else { \
                _reg = vld2_lane_u8(zeroMem,_reg,_n_); \
            } \
        }

        uint16x8_t res;
        uint8x8x2_t src;
        // use dup for first to avoid extra instructions to initialize src
        { T offs = pOffs[0];
            if (offs >= 0) {
                src = vld2_dup_u8(pSrcPixBase+offs);
            } else {
                src = vld2_dup_u8(zeroMem);
            } 
        }
        LOAD_SRCPIX(1,src,pSrcPixBase);
        LOAD_SRCPIX(2,src,pSrcPixBase);
        LOAD_SRCPIX(3,src,pSrcPixBase);
        LOAD_SRCPIX(4,src,pSrcPixBase);
        LOAD_SRCPIX(5,src,pSrcPixBase);
        LOAD_SRCPIX(6,src,pSrcPixBase);
        LOAD_SRCPIX(7,src,pSrcPixBase);
        res = vmulq_u16(vmovl_u8(src.val[0]),wa);
        res = vmlaq_u16(res,vmovl_u8(src.val[1]),wb);

        LOAD_SRCPIX(0,src,pSrcPixBase+srcPixStride);
        LOAD_SRCPIX(1,src,pSrcPixBase+srcPixStride);
        LOAD_SRCPIX(2,src,pSrcPixBase+srcPixStride);
        LOAD_SRCPIX(3,src,pSrcPixBase+srcPixStride);
        LOAD_SRCPIX(4,src,pSrcPixBase+srcPixStride);
        LOAD_SRCPIX(5,src,pSrcPixBase+srcPixStride);
        LOAD_SRCPIX(6,src,pSrcPixBase+srcPixStride);
        LOAD_SRCPIX(7,src,pSrcPixBase+srcPixStride);
        res = vmlaq_u16(res,vmovl_u8(src.val[0]),wc);
        res = vmlaq_u16(res,vmovl_u8(src.val[1]),wd);

        // round, normalize, and narrow for result
#if defined(VT_IS_MSVC_COMPILER)
        vst1_u8_ex(pDst,vqrshrn_n_u16(res,8),8);
#else
        vst1_u8(pDst,vqrshrn_n_u16(res,8));
#endif
    }
#undef LOAD_SRCPIX
}

template <typename T>
VT_FORCEINLINE void 
Bilinear2BandBytex4Process(
    const uint32x4_t& xf0123, const uint32x4_t& yf0123,
    Byte* pDst,
    const T* pOffs, const uint16_t* pSrcPixBase, uint32_t srcPixStride)
{
    static const uint16_t zeroMem[2] = { 0, 0 };

    uint16x8_t wa,wb,wc,wd;     // weights for source pixels abcd
    {
        // weights are duplicated so wa holds [wa0,wa0,wa1,wa1,wa2,wa2,wa3,wa3]
        wb = vreinterpretq_u16_u32(vshlq_n_u32(xf0123,16));
        wb = vorrq_u16(wb,vreinterpretq_u16_u32(xf0123));
        wc = vreinterpretq_u16_u32(vshlq_n_u32(yf0123,16));
        wc = vorrq_u16(wc,vreinterpretq_u16_u32(yf0123));
        wa = vdupq_n_u16(0x100);// 1
        wd = vmulq_u16(wb,wc);  //
        wd = vrshrq_n_u16(wd,8);// normalized x*y
        wc = vsubq_u16(wc,wd);  // y - (x*y)
        wa = vsubq_u16(wa,wb);  // 1 - x
        wb = vsubq_u16(wb,wd);  // x - (x*y)
        wa = vsubq_u16(wa,wc);  // 1-x-(y-(x*y))
    }

    // load samples and filter
    {
        // the Arm compiler insists on using a single set of registers for all
        // uint8x8x2_t variables and doing lots of copying back and forth, so
        // need to have just one and do the offset conditional twice as many
        // times as would otherwise be necessary
#define LOAD_SRCPIX(_n_,_reg,_ptr) \
        { T offs = pOffs[_n_]; \
            if (offs >= 0) { \
                _reg = vld2_lane_u16(_ptr+offs,_reg,_n_); \
            } else { \
                _reg = vld2_lane_u16(zeroMem,_reg,_n_); \
            } \
        }

        uint16x8_t res;
        uint16x4x2_t src;
        // use dup for first to avoid extra instructions to initialize src
        { T offs = pOffs[0];
            if (offs >= 0) {
                src = vld2_dup_u16(pSrcPixBase+offs);
            } else {
                src = vld2_dup_u16(zeroMem);
            } 
        }
        LOAD_SRCPIX(1,src,pSrcPixBase);
        LOAD_SRCPIX(2,src,pSrcPixBase);
        LOAD_SRCPIX(3,src,pSrcPixBase);
        res = vmulq_u16(vmovl_u8(vreinterpret_u8_u16(src.val[0])),wa);
        res = vmlaq_u16(res,vmovl_u8(vreinterpret_u8_u16(src.val[1])),wb);

        LOAD_SRCPIX(0,src,pSrcPixBase+srcPixStride);
        LOAD_SRCPIX(1,src,pSrcPixBase+srcPixStride);
        LOAD_SRCPIX(2,src,pSrcPixBase+srcPixStride);
        LOAD_SRCPIX(3,src,pSrcPixBase+srcPixStride);
        res = vmlaq_u16(res,vmovl_u8(vreinterpret_u8_u16(src.val[0])),wc);
        res = vmlaq_u16(res,vmovl_u8(vreinterpret_u8_u16(src.val[1])),wd);

        // round, normalize, and narrow for result
#if defined(VT_IS_MSVC_COMPILER)
        vst1_u8_ex(pDst,vqrshrn_n_u16(res,8),8);
#else
        vst1_u8(pDst,vqrshrn_n_u16(res,8));
#endif
    }
#undef LOAD_SRCPIX
}

template<typename T>
VT_FORCEINLINE void 
Bilinear4BandBytex4Process(
    const uint32x4_t& xf0123, const uint32x4_t& yf0123,
    Byte* pDst,
    const T* pOffs, const uint32_t* pSrcPixBase, uint32_t srcPixStride)
{
    uint16x4_t wa,wb,wc,wd; // weights for source pixels abcd
    {
        wa = vdup_n_u16(0x100); // 1
        wb = vmovn_u16(xf0123); // pack to 16b
        wc = vmovn_u16(yf0123);
        wd = vmul_u16(wb,wc);
        wd = vrshr_n_u16(wd,8); // normalized x*y
        wc = vsub_u16(wc,wd);   // y - (x*y)
        wa = vsub_u16(wa,wb);   // 1 - x
        wb = vsub_u16(wb,wd);   // x - (x*y)
        wa = vsub_u16(wa,wc);   // 1-x-(y-(x*y))
    }

    // load abcd samples for pixels 0 and 1
    uint8x8_t pab0,pcd0,pab1,pcd1;
    {
        T offs = pOffs[0];
        if (offs >= 0)
        {
            pab0 = vld1_u8((const unsigned char*)(pSrcPixBase+offs));
            pcd0 = vld1_u8((const unsigned char*)(pSrcPixBase+offs+srcPixStride));
        } else { pab0 = vdup_n_u8(0); pcd0 = vdup_n_u8(0); }
        offs = pOffs[1];
        if (offs >= 0)
        {
            pab1 = vld1_u8((const unsigned char*)(pSrcPixBase+offs));
            pcd1 = vld1_u8((const unsigned char*)(pSrcPixBase+offs+srcPixStride));
        } else { pab1 = vdup_n_u8(0); pcd1 = vdup_n_u8(0); }
    }

    // scale by weights and generate half of 4x result
    {
        // need to do one 16x4 at a time using scalar weights
        uint16x4_t res0,res1;
        uint16x8_t x0;
        x0 = vmovl_u8(pab0);
        res0 = vmul_lane_u16(vget_low_u16(x0),wa,0);
        res0 = vmla_lane_u16(res0,vget_high_u16(x0),wb,0);
        x0 = vmovl_u8(pcd0);
        res0 = vmla_lane_u16(res0,vget_low_u16(x0),wc,0);
        res0 = vmla_lane_u16(res0,vget_high_u16(x0),wd,0);
        x0 = vmovl_u8(pab1);
        res1 = vmul_lane_u16(vget_low_u16(x0),wa,1);
        res1 = vmla_lane_u16(res1,vget_high_u16(x0),wb,1);
        x0 = vmovl_u8(pcd1);
        res1 = vmla_lane_u16(res1,vget_low_u16(x0),wc,1);
        res1 = vmla_lane_u16(res1,vget_high_u16(x0),wd,1);
        // round, normalize, and narrow for result
#if defined(VT_IS_MSVC_COMPILER)
        vst1_u8_ex(pDst,vqrshrn_n_u16(vcombine_u16(res0,res1),8),8);
#else
        vst1_u8(pDst,vqrshrn_n_u16(vcombine_u16(res0,res1),8));
#endif
    }

    // load abcd samples for pixels 0 and 1
    {
        T offs = pOffs[2];
        if (offs >= 0)
        {
            pab0 = vld1_u8((const unsigned char*)(pSrcPixBase+offs));
            pcd0 = vld1_u8((const unsigned char*)(pSrcPixBase+offs+srcPixStride));
        } else { pab0 = vdup_n_u8(0); pcd0 = vdup_n_u8(0); }
        offs = pOffs[3];
        if (offs >= 0)
        {
            pab1 = vld1_u8((const unsigned char*)(pSrcPixBase+offs));
            pcd1 = vld1_u8((const unsigned char*)(pSrcPixBase+offs+srcPixStride));
        } else { pab1 = vdup_n_u8(0); pcd1 = vdup_n_u8(0); }
    }

    // scale by weights and generate half of 4x result
    {
        // need to do one 16x4 at a time using scalar weights
        uint16x4_t res0,res1;
        uint16x8_t x0;
        x0 = vmovl_u8(pab0);
        res0 = vmul_lane_u16(vget_low_u16(x0),wa,2);
        res0 = vmla_lane_u16(res0,vget_high_u16(x0),wb,2);
        x0 = vmovl_u8(pcd0);
        res0 = vmla_lane_u16(res0,vget_low_u16(x0),wc,2);
        res0 = vmla_lane_u16(res0,vget_high_u16(x0),wd,2);
        x0 = vmovl_u8(pab1);
        res1 = vmul_lane_u16(vget_low_u16(x0),wa,3);
        res1 = vmla_lane_u16(res1,vget_high_u16(x0),wb,3);
        x0 = vmovl_u8(pcd1);
        res1 = vmla_lane_u16(res1,vget_low_u16(x0),wc,3);
        res1 = vmla_lane_u16(res1,vget_high_u16(x0),wd,3);
        // round, normalize, and narrow for result
#if defined(VT_IS_MSVC_COMPILER)
        vst1_u8_ex(pDst+8,vqrshrn_n_u16(vcombine_u16(res0,res1),8),8);
#else
        vst1_u8(pDst+8,vqrshrn_n_u16(vcombine_u16(res0,res1),8));
#endif
    }
}

#endif
