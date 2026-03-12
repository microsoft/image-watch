//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      parameter/type specific routines for image warping
//
//  History:
//      2011/11/07-v-mitoel
//          Created
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
#include "vt_bicubicbspline.h"

using namespace vt;

// from pad.cpp
extern HRESULT ConvertConstval(EXTEND_CONSTVAL& excDst, int iDstType,
                               const EXTEND_CONSTVAL& excSrc);

// files containing inline functions for SSE and Neon processing
#if (defined(_M_IX86) || defined(_M_AMD64))
#include "warp_special_sse.h"
#elif (defined(_M_ARM) || defined(_M_ARM64))
#include "warp_special_neon.h"
#endif

//------------------------------------------------------------------------
// utility functions
//------------------------------------------------------------------------

inline uint32_t AsUInt32(float val)     { return (*( uint32_t*)(&val)); }
inline uint32_t AsUInt32(int32_t val)   { return (*( uint32_t*)(&val)); }
inline int32_t AsInt32(float val)       { return (*( int32_t*)(&val)); }
inline int32_t AsInt32(uint32_t val)    { return (*( int32_t*)(&val)); }
inline int16_t AsInt16(uint16_t val)    { return (*( int16_t*)(&val)); }

//  these snap fractional bits off floating point numbers with unbiased rounding (round to nearest even)
inline void snapNdot0(float& val) { const float snap = (float)((1<<23)+(1<<22)); val += snap; val -= snap; }
inline void snapNdot4(float& val) { const float snap = (float)((1<<19)+(1<<18)); val += snap; val -= snap; }
inline void snapNdot8(float& val) { const float snap = (float)((1<<15)+(1<<14)); val += snap; val -= snap; }
inline void snapNdot16(float& val) { const float snap = (float)((1<<7)+(1<<6)); val += snap; val -= snap; }

inline int32_t fixedNdot8(float val) 
{ 
    const float snap = (float)((1<<15)+(1<<14));
    float sfval = val+snap;
    uint32_t uival = AsUInt32(sfval);
    uival <<= 10;
    int32_t ival = AsInt32(uival);
    return (ival >> 10);
}

inline int32_t fixedNdot16(float val) 
{ 
    const float scale = (float)(1<<16);
    float sfval = val*scale;
    return (int32_t)(sfval+.5f);
}

inline float Ndot16toFloat(int32_t val) 
{ 
    const float scale = 1.f/(float)(1<<16);
    float fval = (float)val;
    return (fval*scale);
}

inline float fracf(float val) 
{
    return val-floorf(val);
}

//------------------------------------------------------------------------
// single pixel bilerp routine which does full address bounds checking;
// used for source boundary pixels and vector remainder pixels
//------------------------------------------------------------------------
// pixel samples are: |a b| top
//                    |c d| bottom
inline void ComputeFrac(uint16_t* frac, int32_t u, int32_t v)
{
    // compute per-sample weights
    {
        uint16_t ufrac = (u>>8)&0xff;
        uint16_t vfrac = (v>>8)&0xff;
        frac[3] = ((ufrac*vfrac)+0x80)>>8;   // wd = u*v
        frac[2] = vfrac - frac[3];           // wc = v-(u*v)
        frac[1] = ufrac - frac[3];           // wb = u-(u*v)
        frac[0] = 0x100 - ufrac - (frac[2]); // wa = 1-u-v+(u*v) == 1-u-(v-(u*v)); also wa = 1-(wb+wc+wd)
    }
}
inline void ComputeOutBooleans(
    bool& leftOut, bool& rghtOut, bool& topOut, bool& botOut, 
    int32_t addru, int32_t addrv, int srcWidth, int srcHeight)
{
    leftOut = ((addru+0)<0)||((addru+0)>=srcWidth);
    rghtOut = ((addru+1)<0)||((addru+1)>=srcWidth);
    topOut =  ((addrv+0)<0)||((addrv+0)>=srcHeight);
    botOut =  ((addrv+1)<0)||((addrv+1)>=srcHeight);
}
inline uint32_t GetExVal4(const IMAGE_EXTEND& ex) { return (ex.exHoriz == Zero)?(0x0):(*((uint32_t*)ex.excHoriz.Val())); }
inline uint16_t GetExVal2(const IMAGE_EXTEND& ex) { return (ex.exHoriz == Zero)?(0x0):(*((uint16_t*)ex.excHoriz.Val())); }
inline uint8_t  GetExVal1(const IMAGE_EXTEND& ex) { return (ex.exHoriz == Zero)?(0x0):(*((uint8_t*)ex.excHoriz.Val())); }

static uint32_t DoPixel4BandByteBilerp(int32_t u, int32_t v, 
    const uint32_t* pSrcPixBase, int srcWidth, int srcHeight, int srcPixStride, const IMAGE_EXTEND& ex)
{
    uint16_t frac[4]; ComputeFrac(frac, u, v);

    // load the 4 contributing source pixels (a,b,c,d)
    uint32_t pix[4];
    {
        int32_t addru = (u>>16);
        int32_t addrv = (v>>16);
        bool leftOut,rghtOut,topOut,botOut;
        ComputeOutBooleans(leftOut,rghtOut,topOut,botOut,addru,addrv,srcWidth,srcHeight);
        const uint32_t* pixaddr = pSrcPixBase+(srcPixStride*addrv)+addru;
        pix[0] = (leftOut||topOut)?(GetExVal4(ex)):(*(pixaddr)); pixaddr += 1;
        pix[1] = (rghtOut||topOut)?(GetExVal4(ex)):(*(pixaddr)); pixaddr += (srcPixStride-1);
        pix[2] = (leftOut||botOut)?(GetExVal4(ex)):(*(pixaddr)); pixaddr += 1;
        pix[3] = (rghtOut||botOut)?(GetExVal4(ex)):(*(pixaddr));
    }

    // do bilinear filter and return result
    uint32_t res;
    {
        uint32_t pixc[4][4];
        for (int i=0; i<4; i++)
        {
            for (int j=0; j<4; j++)
            {
                pixc[i][j] = (pix[i]>>(8*j))&0xff;
            }
        }
        uint32_t cacc[4] = { 0, 0, 0, 0 };
        for (int i=0; i<4; i++)
        {
            for (int j=0; j<4; j++)
            {
                cacc[j] += (pixc[i][j]*frac[i]);
            }
        }
        for (int i=0; i<4; i++) { cacc[i] >>= 8; if (cacc[i] > 0xff) { cacc[i] = 0xff; } }
        res = (cacc[3]<<24)|(cacc[2]<<16)|(cacc[1]<<8)|cacc[0];
    }
    return res;
}
static uint16_t DoPixel2BandByteBilerp(int32_t u, int32_t v, 
    const uint16_t* pSrcPixBase, int srcWidth, int srcHeight, int srcPixStride, const IMAGE_EXTEND& ex)
{
    uint16_t frac[4]; ComputeFrac(frac, u, v);

    // load the 4 contributing source pixels (a,b,c,d)
    uint16_t pix[4];
    {
        int32_t addru = (u>>16);
        int32_t addrv = (v>>16);
        bool leftOut,rghtOut,topOut,botOut;
        ComputeOutBooleans(leftOut,rghtOut,topOut,botOut,addru,addrv,srcWidth,srcHeight);
        const uint16_t* pixaddr = pSrcPixBase+(srcPixStride*addrv)+addru;
        pix[0] = (leftOut||topOut)?(GetExVal2(ex)):(*(pixaddr)); pixaddr += 1;
        pix[1] = (rghtOut||topOut)?(GetExVal2(ex)):(*(pixaddr)); pixaddr += (srcPixStride-1);
        pix[2] = (leftOut||botOut)?(GetExVal2(ex)):(*(pixaddr)); pixaddr += 1;
        pix[3] = (rghtOut||botOut)?(GetExVal2(ex)):(*(pixaddr));
    }

    // do bilinear filter and return result
    uint16_t res;
    {
        uint32_t pixc[4][2];
        for (int i=0; i<4; i++)
        {
            for (int j=0; j<2; j++)
            {
                pixc[i][j] = (pix[i]>>(8*j))&0xff;
            }
        }
        uint32_t cacc[2] = { 0, 0 };
        for (int i=0; i<4; i++)
        {
            for (int j=0; j<2; j++)
            {
                cacc[j] += (pixc[i][j]*frac[i]);
            }
        }
        for (int i=0; i<2; i++) { cacc[i] >>= 8; if (cacc[i] > 0xff) { cacc[i] = 0xff; } }
        res = (uint16_t)((cacc[1]<<8)|cacc[0]);
    }
    return res;
}
static uint8_t DoPixel1BandByteBilerp(int32_t u, int32_t v, 
    const uint8_t* pSrcPixBase, int srcWidth, int srcHeight, int srcPixStride, const IMAGE_EXTEND& ex)
{
    uint16_t frac[4]; ComputeFrac(frac, u, v);

    // load the 4 contributing source pixels (a,b,c,d)
    uint8_t pix[4];
    {
        int32_t addru = (u>>16);
        int32_t addrv = (v>>16);
        bool leftOut,rghtOut,topOut,botOut;
        ComputeOutBooleans(leftOut,rghtOut,topOut,botOut,addru,addrv,srcWidth,srcHeight);
        const uint8_t* pixaddr = pSrcPixBase+(srcPixStride*addrv)+addru;
        pix[0] = (leftOut||topOut)?(GetExVal1(ex)):(*(pixaddr)); pixaddr += 1;
        pix[1] = (rghtOut||topOut)?(GetExVal1(ex)):(*(pixaddr)); pixaddr += (srcPixStride-1);
        pix[2] = (leftOut||botOut)?(GetExVal1(ex)):(*(pixaddr)); pixaddr += 1;
        pix[3] = (rghtOut||botOut)?(GetExVal1(ex)):(*(pixaddr));
    }

    // do bilinear filter and return result
    uint8_t res;
    {
        uint32_t pixc[4];
        for (int i=0; i<4; i++)
        {
            pixc[i] = pix[i];
        }
        uint32_t cacc = 0;
        for (int i=0; i<4; i++)
        {
            cacc += (pixc[i]*frac[i]);
        }
        cacc >>= 8; if (cacc > 0xff) { cacc = 0xff; }
        res = (uint8_t)cacc;
    }
    return res;
}

//------------------------------------------------------------------------
// imgSrc and imgDst 1,2,4 band 
// type: Byte 
// transform: affine warp only
// sampler: bilinear filtering
// imgSrc max dimension: 32768
// extend mode: IMAGE_EXTEND(Zero)
//------------------------------------------------------------------------
HRESULT
WarpBlock_NBand_Byte_Affine_Bilerp_Src32768_ExZorC(
    CImg& imgDstBlk,
    const CPoint& ptDst, 
    const CImg& imgSrc,
    const CMtx3x3f* pMtx,
    const IMAGE_EXTEND& exParam,
    void (VT_STDCALL *setOutOfRangePixel)(uint8_t*& pPix, int offset, bool doPostIncrement, const IMAGE_EXTEND& ex),
    void (VT_STDCALL *doSinglePixel)(uint8_t*& pPix, int offset, bool doPostIncrement, const IMAGE_EXTEND& ex,
        uint32_t u, uint32_t v, const uint8_t* pSrcPixBase, int srcWidth, int srcHeight, int srcStrideBytes),
    void (VT_STDCALL *doInteriorPixelsBy4)(uint8_t*& pPix, int32_t w4, const uint8_t* pSrcPixBase, int32_t srcStrideBytes,
        int32_t u, int32_t v, int32_t deltau, int32_t deltav),
    void (VT_STDCALL *doInteriorPixelsBy8)(uint8_t*& pPix, int32_t w4, const uint8_t* pSrcPixBase, int32_t srcStrideBytes,
        int32_t u, int32_t v, int32_t deltau, int32_t deltav)
    )
{
    VT_HR_BEGIN()

    // copy extend info so constant value can be converted to match source/dest
    // image type and band count
    IMAGE_EXTEND ex; 
    VT_HR_EXIT( ex.Initialize(&exParam) );
    if (ex.exHoriz == Constant)
    {
        VT_HR_EXIT( ConvertConstval(ex.excHoriz, imgDstBlk.GetType(), exParam.excHoriz) );
    }

    const CMtx3x3f& xfrm = *pMtx; // ((*pMtx)(2,2)==0)? (*pMtx): ((*pMtx)/(*pMtx)(2,2));

    int iWidth = imgDstBlk.Width();
    const uint8_t* pSrcPixBase = imgSrc.BytePtr();
    uint32_t srcStrideBytes = imgSrc.StrideBytes(); 
    int srcWidth = imgSrc.Width();
    int srcHeight = imgSrc.Height();

    const int uvMinRange = -65536; // -1.0 in n.16 fixed point
    const int uMaxRange = fixedNdot16((float)srcWidth);
    const int vMaxRange = fixedNdot16((float)srcHeight);
    const int uMaxBoundary = fixedNdot16((float)(srcWidth-1))-1;
    const int vMaxBoundary = fixedNdot16((float)(srcHeight-1))-1;

    for( int y = 0; y < imgDstBlk.Height(); y++)
    {
        // compute source locations for endpoints of current span
        vt::CVec2f leftPos, rghtPos;
        leftPos.x = (xfrm(0,0) * float(ptDst.x       )) + (xfrm(0,1) * float(ptDst.y + y)) + (xfrm(0,2));
        leftPos.y = (xfrm(1,0) * float(ptDst.x       )) + (xfrm(1,1) * float(ptDst.y + y)) + (xfrm(1,2));
        rghtPos.x = (xfrm(0,0) * float(ptDst.x+iWidth)) + (xfrm(0,1) * float(ptDst.y + y)) + (xfrm(0,2));
        rghtPos.y = (xfrm(1,0) * float(ptDst.x+iWidth)) + (xfrm(1,1) * float(ptDst.y + y)) + (xfrm(1,2));

        // compute integral n.16 u,v iterator values
        int32_t leftu = fixedNdot16(leftPos.x);
        int32_t leftv = fixedNdot16(leftPos.y);
        int32_t deltau = (fixedNdot16(rghtPos.x)-leftu)/iWidth;
        int32_t deltav = (fixedNdot16(rghtPos.y)-leftv)/iWidth;
        int32_t rghtu = leftu+((iWidth-1)*deltau);
        int32_t rghtv = leftv+((iWidth-1)*deltav);

        uint8_t* pDstPix = imgDstBlk.BytePtr(y);

        int x = 0; 
        int xe = iWidth-1;
        // narrow span by stepping past area of out-of-range source pixels on right
        while ( ((rghtu<uvMinRange)||(rghtv<uvMinRange)||(rghtu>uMaxRange)||(rghtv>vMaxRange)) && (xe>=x) )
        {
            (*setOutOfRangePixel)(pDstPix,xe,false,ex);
            rghtu -= deltau;
            rghtv -= deltav;
            xe--;
        }
        if (x>xe) { continue; }
        // process pixels on right until within range check
        while (((AsUInt32(rghtu)|AsUInt32(rghtv))>>31)||(rghtu>uMaxBoundary)||(rghtv>vMaxBoundary))
        {
            (*doSinglePixel)(pDstPix,xe,false,ex,rghtu,rghtv,pSrcPixBase,srcWidth,srcHeight,srcStrideBytes);
            rghtu -= deltau;
            rghtv -= deltav;
            xe--;
            if (xe < x) { break; }
        }
        if (x>xe) { continue; }
        // narrow span by stepping past area of out-of-range source pixels on left
        while ( ((leftu<uvMinRange)||(leftv<uvMinRange)||(leftu>uMaxRange)||(leftv>vMaxRange)) && (x<=xe) )
        {
            (*setOutOfRangePixel)(pDstPix,0,true,ex);
            leftu += deltau;
            leftv += deltav;
            x++;
        }
        if (x>xe) { continue; }
        // process pixels on left until within range check
        while (((AsUInt32(leftu)|AsUInt32(leftv))>>31)||(leftu>uMaxBoundary)||(leftv>vMaxBoundary))
        {
            (*doSinglePixel)(pDstPix,0,true,ex,leftu,leftv,pSrcPixBase,srcWidth,srcHeight,srcStrideBytes);
            leftu += deltau;
            leftv += deltav;
            x++;
            if (x > xe) { break; }
        }
        if (x>xe) { continue; }
        int w = xe-x+1;

        int32_t u = leftu;
        int32_t v = leftv;

        // run by8 span iterator
        if ( (doInteriorPixelsBy8 != NULL) && (w >= 8) )
        {
            (*doInteriorPixelsBy8)(pDstPix,w>>3,pSrcPixBase,srcStrideBytes,u,v,deltau,deltav);
            int w8 = (w>>3)<<3; u += (w8*deltau); v += (w8*deltav); w -= w8;
        }
        // run by4 span iterator
        else if ( (doInteriorPixelsBy4 != NULL) && (w >= 4) )
        {
            (*doInteriorPixelsBy4)(pDstPix,w>>2,pSrcPixBase,srcStrideBytes,u,v,deltau,deltav);
            int w4 = (w>>2)<<2; u += (w4*deltau); v += (w4*deltav); w -= w4;
        }
        // process any vector remainder
        while (w>0)
        {
            (*doSinglePixel)(pDstPix,0,true,ex,u,v,pSrcPixBase,srcWidth,srcHeight,srcStrideBytes);
            u += deltau;
            v += deltav;
            w--;
        }
    }    
    VT_HR_END();
}
//------------------------------------------------------------------------
// 4 byte
//------------------------------------------------------------------------
void VT_STDCALL SetOutOfRangePixel_4BandByte(uint8_t*& pPix, int offset, bool doInc, const IMAGE_EXTEND& ex)
{
    *(((uint32_t*)pPix)+offset) = GetExVal4(ex);
    if (doInc) { pPix += 4; }
}
void VT_STDCALL DoSinglePixel_4BandByte(uint8_t*& pPix, int offset, bool doInc, const IMAGE_EXTEND& ex,
    uint32_t u, uint32_t v, const uint8_t* pSrcPixBase, 
    int srcWidth, int srcHeight, int srcStrideBytes)
{
    *(((uint32_t*)pPix)+offset) = DoPixel4BandByteBilerp(u,v,(const uint32_t*)pSrcPixBase,srcWidth,srcHeight,srcStrideBytes>>2,ex);
    if (doInc) { pPix += 4; }
}
void VT_STDCALL DoInteriorPixelsBy4_4BandByte(uint8_t*& pDstPix, int32_t w4, 
    const uint8_t* pSrcPixBase, int32_t srcStrideBytes, 
    int32_t u, int32_t v, int32_t deltau, int32_t deltav)
{
#if (defined(_M_IX86) || defined(_M_AMD64))
    WarpSpecialSpan_by4_BilinearByte4_SSE((uint32_t*)pDstPix,w4,(const uint32_t*)pSrcPixBase,srcStrideBytes>>2,
        u,v,deltau,deltav);
#elif (defined(_M_ARM) || defined(_M_ARM64))
    SpanIteratorInfo iterInfo = { u, v, deltau, deltav, srcStrideBytes>>2 };
    WarpSpecialSpan_by4_BilinearByte4_Neon_Asm((uint32_t*)pDstPix,w4,(uint32_t*)pSrcPixBase,&iterInfo);
#endif
    pDstPix += (w4*4*4);
}
HRESULT VT_STDCALL
WarpBlock_4Band_Byte_Affine_Bilerp_Src32768_ExZorC(
    CImg& imgDstBlk, const CPoint& ptDst, 
    const CImg& imgSrc,
    const CMtx3x3f* pMtx, const IMAGE_EXTEND& ex)
{
    return WarpBlock_NBand_Byte_Affine_Bilerp_Src32768_ExZorC(
        imgDstBlk, ptDst, imgSrc, pMtx, ex,
        &SetOutOfRangePixel_4BandByte,
        &DoSinglePixel_4BandByte,
        &DoInteriorPixelsBy4_4BandByte,
        NULL);
}
//------------------------------------------------------------------------
// 2 byte
//------------------------------------------------------------------------
void VT_STDCALL SetOutOfRangePixel_2BandByte(uint8_t*& pPix, int offset, bool doInc, const IMAGE_EXTEND& ex)
{
    *(((uint16_t*)pPix)+offset) = GetExVal2(ex);
    if (doInc) { pPix += 2; }
}
void VT_STDCALL DoSinglePixel_2BandByte(uint8_t*& pPix, int offset, bool doInc, const IMAGE_EXTEND& ex,
    uint32_t u, uint32_t v, const uint8_t* pSrcPixBase, 
    int srcWidth, int srcHeight, int srcStrideBytes)
{
    *(((uint16_t*)pPix)+offset) = DoPixel2BandByteBilerp(u,v,(const uint16_t*)pSrcPixBase,srcWidth,srcHeight,srcStrideBytes>>1,ex);
    if (doInc) { pPix += 2; }
}
void VT_STDCALL DoInteriorPixelsBy4_2BandByte(uint8_t*& pDstPix, int32_t w4, 
    const uint8_t* pSrcPixBase, int32_t srcStrideBytes, 
    int32_t u, int32_t v, int32_t deltau, int32_t deltav)
{
#if (defined(_M_IX86) || defined(_M_AMD64))
    WarpSpecialSpan_by4_BilinearByte2_SSE((uint16_t*)pDstPix,w4,(const uint16_t*)pSrcPixBase,srcStrideBytes>>1,
        u,v,deltau,deltav);
#elif (defined(_M_ARM) || defined(_M_ARM64))
    SpanIteratorInfo iterInfo = { u, v, deltau, deltav, srcStrideBytes>>1 };
    WarpSpecialSpan_by4_BilinearByte2_Neon_Asm((uint16_t*)pDstPix,w4,(const uint16_t*)pSrcPixBase,&iterInfo);
#endif
    pDstPix += (w4*4*2);
}
HRESULT VT_STDCALL
WarpBlock_2Band_Byte_Affine_Bilerp_Src32768_ExZorC(
    CImg& imgDstBlk, const CPoint& ptDst, 
    const CImg& imgSrc,
    const CMtx3x3f* pMtx, const IMAGE_EXTEND& ex)
{
    return WarpBlock_NBand_Byte_Affine_Bilerp_Src32768_ExZorC(
        imgDstBlk, ptDst, imgSrc, pMtx, ex,
        &SetOutOfRangePixel_2BandByte,
        &DoSinglePixel_2BandByte,
#if (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_ARM) || defined(_M_ARM64))
        &DoInteriorPixelsBy4_2BandByte,
#else
        NULL,
#endif
        NULL);
}
//------------------------------------------------------------------------
// 1 byte
//------------------------------------------------------------------------
void VT_STDCALL SetOutOfRangePixel_1BandByte(uint8_t*& pPix, int offset, bool doInc, const IMAGE_EXTEND& ex)
{
    *(pPix+offset) = GetExVal1(ex);
    if (doInc) { pPix ++; }
}
void VT_STDCALL DoSinglePixel_1BandByte(uint8_t*& pPix, int offset, bool doInc, const IMAGE_EXTEND& ex,
    uint32_t u, uint32_t v, const uint8_t* pSrcPixBase, 
    int srcWidth, int srcHeight, int srcStrideBytes)
{
    *(pPix+offset) = DoPixel1BandByteBilerp(u,v,pSrcPixBase,srcWidth,srcHeight,srcStrideBytes,ex);
    if (doInc) { pPix ++; }
}
#if (defined(_M_ARM) || defined(_M_ARM64))
void VT_STDCALL DoInteriorPixelsBy4_1BandByte(uint8_t*& pDstPix, int32_t w4, 
    const uint8_t* pSrcPixBase, int32_t srcStrideBytes, 
    int32_t u, int32_t v, int32_t deltau, int32_t deltav)
{
    SpanIteratorInfo iterInfo = { u, v, deltau, deltav, srcStrideBytes };
    WarpSpecialSpan_by4_BilinearByte1_Neon_Asm(pDstPix,w4,pSrcPixBase,&iterInfo);
    pDstPix += (w4*4*1);
}
#endif
#if (defined(_M_IX86) || defined(_M_AMD64))
void VT_STDCALL DoInteriorPixelsBy8_1BandByte(uint8_t*& pDstPix, int32_t w8, 
    const uint8_t* pSrcPixBase, int32_t srcStrideBytes, 
    int32_t u, int32_t v, int32_t deltau, int32_t deltav)
{
    WarpSpecialSpan_by8_BilinearByte1_SSE(pDstPix,w8,pSrcPixBase,srcStrideBytes,
        u,v,deltau,deltav);
    pDstPix += (w8*8*1);
}
#endif
HRESULT VT_STDCALL
WarpBlock_1Band_Byte_Affine_Bilerp_Src32768_ExZorC(
    CImg& imgDstBlk, const CPoint& ptDst, 
    const CImg& imgSrc,
    const CMtx3x3f* pMtx, const IMAGE_EXTEND& ex)
{
    return WarpBlock_NBand_Byte_Affine_Bilerp_Src32768_ExZorC(
        imgDstBlk, ptDst, imgSrc, pMtx, ex,
        &SetOutOfRangePixel_1BandByte,
        &DoSinglePixel_1BandByte,
#if (defined(_M_IX86) || defined(_M_AMD64))
        NULL,
        &DoInteriorPixelsBy8_1BandByte
#elif (defined(_M_ARM) || defined(_M_ARM64))
        &DoInteriorPixelsBy4_1BandByte,
        NULL
#else
        NULL,NULL
#endif
        );
}
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// end
