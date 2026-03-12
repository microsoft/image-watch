//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      specialized implementation of image warp
//
//      NEON specific code
//
//------------------------------------------------------------------------------
#pragma once

// struct so code only has to pass 4 params to assembler (easier linkage)
typedef struct tagSpanIterInfo
{
    int32_t u,v,du,dv;
    uint32_t srcPixStride;
} SpanIteratorInfo;

//------------------------------------------------------------------------------
// 4 band routine
//------------------------------------------------------------------------------
extern "C" void WarpSpecialSpan_by4_BilinearByte4_Neon_Asm(
    uint32_t* pDstPix, int w4, const uint32_t* pSrcPixBase, SpanIteratorInfo* pSpanIInfo);

#if 0
inline void WarpSpecialSpan_by4_BilinearByte4_Neon(
    uint32_t* pDstPix, int w4, const uint32_t* pSrcPixBase, SpanIteratorInfo* pSpanIInfo)
{
    int32_t u = pSpanIInfo->u;
    int32_t v = pSpanIInfo->v;
    int32_t du = pSpanIInfo->du;
    int32_t dv = pSpanIInfo->dv;
    uint32_t srcPixStride = pSpanIInfo->srcPixStride;
    while (w4 > 0)
    {
        // compute per-sample weights
        uint16x4_t frac; // weights for source pixels abcd
        {
            uint16_t ufrac = (u>>8)&0xff;
            uint16_t vfrac = (v>>8)&0xff;
            frac.n64_u16[3] = ((ufrac*vfrac)+0x80)>>8;           // wd = u*v
            frac.n64_u16[2] = vfrac - frac.n64_u16[3];           // wc = v-(u*v)
            frac.n64_u16[1] = ufrac - frac.n64_u16[3];           // wb = u-(u*v)
            frac.n64_u16[0] = 0x100 - ufrac - (frac.n64_u16[2]); // wa = 1-u-v+(u*v) == 1-u-(v-(u*v)); also wa = 1-(wb+wc+wd)
        }

        /* compute addresses of source pixels */
        uint16_t addru = (uint16_t)(u>>16);
        uint16_t addrv = (uint16_t)(v>>16);
        const uint32_t* pixaddrAB = pSrcPixBase+((uint16_t)srcPixStride*addrv)+addru;
        const uint32_t* pixaddrCD = pixaddrAB + srcPixStride;

        /* load and unpack pixels ab and cd */
        uint16x8_t pixab = vmovl_u8(vld1_u8((const unsigned char*)pixaddrAB));
        uint16x8_t pixcd = vmovl_u8(vld1_u8((const unsigned char*)pixaddrCD));
        /* scale by weights */
        uint16x8_t res;
        res.low64 = vmul_lane_u16(pixab.low64,frac,0);
        res.low64 = vmla_lane_u16(res.low64,pixab.high64,frac,1);
        res.low64 = vmla_lane_u16(res.low64,pixcd.low64,frac,2);
        res.low64 = vmla_lane_u16(res.low64,pixcd.high64,frac,3);
        /* do rounding shift */
        res.low64 = vrshr_n_s16(res.low64,8);
        /* pack to 8bpp and store */
        res.low64 = vmovn_s16(res);
        vst1_lane_u32(pDstPix,res.low64,0);

        u += du;
        v += dv;
        w4--;
        pDstPix++;
    }
}
#endif
//------------------------------------------------------------------------------
// 1 and 2 band routines
//------------------------------------------------------------------------------
extern "C" void WarpSpecialSpan_by4_BilinearByte1_Neon_Asm(
    uint8_t* pDstPix, int w4, const uint8_t* pSrcPixBase, SpanIteratorInfo* pSpanIInfo);
    
extern "C" void WarpSpecialSpan_by4_BilinearByte2_Neon_Asm(
    uint16_t* pDstPix, int w4, const uint16_t* pSrcPixBase, SpanIteratorInfo* pSpanIInfo);

//------------------------------------------------------------------------------
// end
