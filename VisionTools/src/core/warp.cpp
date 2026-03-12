//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for image warping
//
//  History:
//      2010/09/27-mattu
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
#include "vt_bicubicbspline.h"
#include "warp_special.h"
#include "bilinear.h"

using namespace vt;

#define WARP_LOOKUP_COUNT 256

inline float 
BilinearFilterPixel(float i0, float i1, float c0, float c1)
{ return i0*c0 + i1*c1; }

// files containing inline functions for SSE and Neon processing
#if defined(VT_SSE_SUPPORTED)
#include "warp_sse.h"
#endif
#if defined(VT_SIMD_SUPPORTED)
#include "warp_simd.h"
#endif

// from resize.cpp
extern HRESULT
BilinearResizeBlock( CImg& imgDstBlk, const vt::CRect rctDstBlk,
                     const CByteImg& imgSrcBlk, const vt::CRect& rctSrcBlk,
                     float sx, float tx, float sy, float ty );
extern HRESULT
BilinearResizeBlock( CImg& imgDstBlk, const vt::CRect rctDstBlk,
                     const CFloatImg& imgSrcBlk, const vt::CRect& rctSrcBlk,
                     float sx, float tx, float sy, float ty );

// switch to remove usage of parameter-specific specialized codepaths
#if defined(_MSC_VER)
#define USE_WARPSPECIAL_CODEPATHS
#endif

// forward declaration for internal VtWarpImage function
HRESULT
VtWarpImageInternal( CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
                     IAddressGenerator** ppGenChain, UInt32 uGenChainCnt, 
                     eSamplerKernel sampler, const IMAGE_EXTEND& ex,
                     const CMtx3x3f* pMtx = NULL);

//+-----------------------------------------------------------------------------
//
// Routines to calculate the cubic spline coefficients.  Implemented out of: 
//     Parker, et al IEEE Trans. on Medical Imaging VOL. MI-2, NO. 1, March 1983
//
//------------------------------------------------------------------------------
static float interval0(float x, float a)
{
    float x2 = x*x;
    return (a+2.f)*x*x2 - (a+3.f)*x2 + 1.f;
}

static float interval1(float x, float a)
{
    float x2 = x*x;
    return a*(x*x2 - 5.f*x2 + 8.f*x - 4.f);
}

//+-----------------------------------------------------------------------------
//
// Class: CCubicCoefTable
//
// Synopsis: holds a quantized set of cubic filter coefs
//  
//------------------------------------------------------------------------------
class CCubicCoefTable
{
public:
    CCubicCoefTable(float a = -0.5f)
    {
        float d    = 0;
        float dinc = 1.f / float(WARP_LOOKUP_COUNT);
        for ( int i = 0; i < WARP_LOOKUP_COUNT; i++, d+=dinc )
        {
            float* pC = m_table+i*4;
            pC[0] = interval1(1+d, a);
            pC[1] = interval0(d,   a);
            pC[2] = interval0(1-d, a);
            pC[3] = interval1(2-d, a);
        }
        // last set of coefs are needed because sometimes the quantized coef
        // lookup rounds up to next source pixel
        float* pC = m_table+WARP_LOOKUP_COUNT*4;
        pC[0] = 0, pC[1] = 0, pC[2] = 1.f, pC[3] = 0.f;
    }

    float* GetCoef()
    { return m_table; }

protected:
    float m_table[(WARP_LOOKUP_COUNT+1)*4];
};


//+-----------------------------------------------------------------------------
//
// Routines to calculate the cubic b-spline coefficients (eSamplerKernelBicubicBSpline).  
//
//------------------------------------------------------------------------------
static float s_CBS_interval0(float x)
{
    x = 1 - x;
    float x2 = x * x;

    return 1 + 3 * (x + x2 - x * x2);
}

static float s_CBS_interval1(float x)
{
    x = 2 - x;
    return x * x * x;
}

//+-----------------------------------------------------------------------------
//
// Class: CCubicBSplineCoefTable
//
// Synopsis: holds a quantized set of cubic b-spline filter coefs
//  
//------------------------------------------------------------------------------
class CCubicBSplineCoefTable
{
public:
    CCubicBSplineCoefTable()
    {
        const float divSix = 1.0f / 6.0f;
        float d    = 0;
        float dinc = 1.f / float(WARP_LOOKUP_COUNT);
        for ( int i = 0; i < WARP_LOOKUP_COUNT; i++, d+=dinc )
        {
            float* pC = m_table+i*4;
            pC[0] = s_CBS_interval1(1+d) * divSix;
            pC[1] = s_CBS_interval0(  d) * divSix;
            pC[2] = s_CBS_interval0(1-d) * divSix;
            pC[3] = s_CBS_interval1(2-d) * divSix;
        }
        // last set of coefs are needed because sometimes the quantized coef
        // lookup rounds up to next source pixel
        float* pC = m_table+WARP_LOOKUP_COUNT*4;
        pC[0] = 0;
        pC[1] = divSix;
        pC[2] = 4.f * divSix;
        pC[3] = divSix;
    }

    float* GetCoef()
    { return m_table; }

protected:
    float m_table[(WARP_LOOKUP_COUNT+1)*4];
};

static const float* const GetDefaultCoef(eSamplerKernel sampler)
{
    // create a static warp coef table for the default 'a' value
    static CCubicCoefTable g_defaultcubiccoef;

    // and a static table for default cubic bspline coefficients.
    static CCubicBSplineCoefTable g_defaultcubicbsplinecoef;

    switch(sampler)
    {
    case eSamplerKernelBicubic:
        return g_defaultcubiccoef.GetCoef();
    case eSamplerKernelBicubicBSpline:
    case eSamplerKernelBicubicBSplineSrcPreprocessed:
        return g_defaultcubicbsplinecoef.GetCoef();
    default:
        break;
    }
    return NULL;
}

//+-----------------------------------------------------------------------------
//
// function: Warp(...)
// 
//------------------------------------------------------------------------------
void NearestNeighborWarpSpan(float* pDstF, const CFloatImg& imgSrcBlk, 
    const vt::CRect& rctSrcBlk, const CVec2f* pAddr, int iSpan)
{
    int nb = imgSrcBlk.Bands();

    int leftB   = rctSrcBlk.left;
    int rightB  = imgSrcBlk.Width()-2;
    int topB    = rctSrcBlk.top;
    int bottomB = imgSrcBlk.Height()-2;

    int x = 0;

    // convert addresses to pixel locations and bilinear coefficients
    for ( ; x < iSpan; )
    {
        for ( ;x < iSpan; x++ )
        {
            CVec2f aflr, a = pAddr[x];
            floor2(&aflr.x, &a.x);
            int u = F2I(aflr.x)-leftB;
            int v = F2I(aflr.y)-topB;
            int ur = rightB-u;
            int vb = bottomB-v;
            if ( (u|v|ur|vb) & 0x80000000 )
            {
                // if any of the sign bits are not 0 then this output pixel
                // requires source pixels that weren't provided 
                break;
            }

            const float* pSrc = imgSrcBlk.Ptr(u, v);
            for ( int b = 0; b < nb; b++, pDstF++, pSrc++ )
                *pDstF = *pSrc;            
        }

        int iCurSpan = 0;
        for ( ;x < iSpan; x++, iCurSpan++ )
        {
            CVec2f aflr, a = pAddr[x];
            floor2(&aflr.x, &a.x);
            int u = F2I(aflr.x)-leftB;
            int v = F2I(aflr.y)-topB;
            int ur = rightB-u;
            int vb = bottomB-v;
            if ( ((u|v|ur|vb) & 0x80000000) == 0 )
            {
                // opposite termination condition from above while(), 
                // thus keep accumulating OOB addresses
                break;
            }
        }
        if ( iCurSpan )
        {
            // clear the OOB span
            memset(pDstF, 0, iCurSpan*nb*sizeof(float));
            pDstF += iCurSpan*nb;
        }
    }    
}

//-----------------------------------------------------------------------------
// 1..4 band Byte->Byte warp
//-----------------------------------------------------------------------------

inline int32_t AsInt32(float val)       
    { return (*( int32_t*)(&val)); }
inline uint32_t AsUInt32Ndot8(float val)     
    { float vals = val + (float)(1<<15); return (*(uint32_t*)(&vals)); }

void
BilinearWarpSpan(Byte* pDst, int dstBands, const CByteImg& imgSrcBlk, 
    const vt::CRect& rctSrcBlk, const CVec2f* pAddr, int iSpan)
{
    int srcBands = imgSrcBlk.Bands();
    VT_ASSERT( dstBands <= 4 );
    VT_ASSERT( (dstBands == srcBands) || ((dstBands==3)&&(srcBands==4)) );
    float leftB   = (float)(rctSrcBlk.left);
    float rightB  = (float)(imgSrcBlk.Width()-2);
    float topB    = (float)(rctSrcBlk.top);
    float bottomB = (float)(imgSrcBlk.Height()-2);

    int x = 0;
#if defined(VT_SIMD_SUPPORTED)
    if ( VTBILINEAR_SIMD_SUPPORTED )
    {
        if ( (dstBands == 1) && (iSpan >= 8) )
        {
            SelectPSTFunc(IsAligned16(pAddr),x,Load4,
                BilinearWarpSpan1BandByte, pDst, imgSrcBlk, pAddr, 
                leftB,rightB,topB,bottomB,iSpan);
        }
        else if ( (dstBands == 2) && (iSpan >= 4) )
        {
            SelectPSTFunc(IsAligned16(pAddr),x,Load4,
                BilinearWarpSpan2BandByte, pDst, imgSrcBlk, pAddr, 
                leftB,rightB,topB,bottomB,iSpan);
        }
        else if ( (dstBands == 3) && (iSpan >= 4) )
        {
            SelectPSTFunc(IsAligned16(pAddr),x,Load4,
                BilinearWarpSpanS4D3BandByte, pDst, imgSrcBlk, pAddr, 
                leftB,rightB,topB,bottomB,iSpan);
        }
        else if ( (dstBands == 4) && (iSpan >= 4) )
        {
            SelectPSTFunc(IsAligned16(pAddr),x,Load4,
                BilinearWarpSpan4BandByte, pDst, imgSrcBlk, pAddr, 
                leftB,rightB,topB,bottomB,iSpan);
        }
        pDst += dstBands*x; pAddr += x;
    }
#endif
    if (x >= iSpan) { return; }
    int strideScale = imgSrcBlk.StrideBytes(); // srcBands is 1,2, or 4 
    if (srcBands == 2) { strideScale >>= 1; }
    if (srcBands == 4) { strideScale >>= 2; }
    const Byte* pSrc = imgSrcBlk.BytePtr();
    for ( ; x < iSpan; x++, pAddr++)
    {
        // current address in block coordinate space
        float fx = pAddr->x - leftB;
        float fy = pAddr->y - topB;

        // bounds tests - check for any negative and set dest to zero if so
        int32_t oobTest = AsInt32(fx)|AsInt32(fy)|
            AsInt32(rightB - fx)|AsInt32(bottomB - fy);
        if (oobTest & (1<<31))
        {
            for (int j=0; j<dstBands; j++) { *pDst++ = 0x0; }
            continue;
        }

        // convert floating point offsets to 15.8 fixed point
        uint32_t ix = AsUInt32Ndot8(fx);
        uint32_t iy = AsUInt32Ndot8(fy);

        BilinearProcessSinglePixel(dstBands, pDst,
            (ix>>8)&0x7fff, (iy>>8)&0x7fff, // integral bits of source address
            ix & 0xff, iy & 0xff, // fraction bits of source address
            strideScale, pSrc);
        pDst += dstBands;
    }
}


void
BilinearWarpSpan(float* pDstF, const CFloatImg& imgSrcBlk, 
                 const vt::CRect& rctSrcBlk, const CVec2f* pAddr, int iSpan)
{
    int nb = imgSrcBlk.Bands();

    if ( g_SupportSSE2() && nb != 4 )
    {
#if defined(VT_SSE_SUPPORTED)
        BilinearWarpSpanSSE(pDstF, imgSrcBlk, rctSrcBlk, pAddr, iSpan);
#endif
    }
    else
    {
        int leftB   = rctSrcBlk.left;
        int rightB  = imgSrcBlk.Width()-2;
        int topB    = rctSrcBlk.top;
        int bottomB = imgSrcBlk.Height()-2;

        int x = 0;
        if( g_SupportSSE2() && nb == 4 )
        {
#if defined(VT_SSE_SUPPORTED)
            x = BilinearWarpSpan4BandSSE(pDstF, imgSrcBlk, pAddr, iSpan,
                                         leftB, rightB, topB, bottomB);
            pDstF += 4*x;
#endif
        }

        size_t stride = imgSrcBlk.StrideBytes();

        // convert addresses to pixel locations and bilinear coefficients
        for ( ; x < iSpan; )
        {
            for ( ;x < iSpan; x++ )
            {
                CVec2f aflr, a = pAddr[x];
                floor2(&aflr.x, &a.x);
                int u = F2I(aflr.x)-leftB;
                int v = F2I(aflr.y)-topB;
                int ur = rightB-u;
                int vb = bottomB-v;
                if ( (u|v|ur|vb) & 0x80000000 )
                {
                    // if any of the sign bits are not 0 then this output pixel
                    // requires source pixels that weren't provided 
                    break;
                }

                const float* pSrc = imgSrcBlk.Ptr(u, v);
                // compute the bilinear coef
                float xc0 = a.x-aflr.x;
                float xc1 = 1.f - xc0;
                float yc0 = a.y-aflr.y;
                float yc1 = 1.f - yc0;

                for ( int b = 0; b < nb; b++, pDstF++, pSrc++ )
                {
                    const float *pr = pSrc;
                    float y0 = BilinearFilterPixel(pr[0], pr[nb], xc1, xc0);

                    pr = (float*)((Byte*)pr + stride);
                    float y1 = BilinearFilterPixel(pr[0], pr[nb], xc1, xc0);

                    *pDstF = BilinearFilterPixel(y0, y1, yc1, yc0);
                }
            }

            int iCurSpan = 0;
            for ( ;x < iSpan; x++, iCurSpan++ )
            {
                CVec2f aflr, a = pAddr[x];
                floor2(&aflr.x, &a.x);
                int u = F2I(aflr.x)-leftB;
                int v = F2I(aflr.y)-topB;
                int ur = rightB-u;
                int vb = bottomB-v;
                if ( ((u|v|ur|vb) & 0x80000000) == 0 )
                {
                    // opposite termination condition from above while(), 
                    // thus keep accumulating OOB addresses
                    break;
                }
            }
            if ( iCurSpan )
            {
                // clear the OOB span
                memset(pDstF, 0, iCurSpan*nb*sizeof(float));
                pDstF += iCurSpan*nb;
            }
        }
    }
}

inline float 
CubicFilterPixel(float i0, float i1, float i2, float i3, const float* c)
{ return i0*c[0] + i1*c[1] + i2*c[2] + i3*c[3]; }

template <void (*T_FLOOR)(float*, const float*)> void
BicubicWarpSpan(float* pDstF, 
                const CFloatImg& imgSrcBlk, const vt::CRect& rctSrcBlk,
                const CVec2f* pAddr, const float* pFilterCoef, int iSpan)
{
    int nb = imgSrcBlk.Bands();


    const int iFilterSupport = 1;
    const int iFilterWidth   = 4;
    int leftB   = iFilterSupport+rctSrcBlk.left;
    int rightB  = imgSrcBlk.Width()-iFilterWidth;
    int topB    = iFilterSupport+rctSrcBlk.top;
    int bottomB = imgSrcBlk.Height()-iFilterWidth;

    size_t stride = imgSrcBlk.StrideBytes();

    // convert addresses to pixel locations and warp coefficient look-up addresses
    for( int x = 0; x < iSpan; )
    {
        for( ;x < iSpan; x++ )
        {
            CVec2f aflr, a = pAddr[x];
            T_FLOOR(&aflr.x, &a.x);
            int u = F2I(aflr.x)-leftB;
            int v = F2I(aflr.y)-topB;
            int ur = rightB-u;
            int vb = bottomB-v;
            if( (u|v|ur|vb) & 0x80000000 )
            {
                // if any of the sign bits are not 0 then this output pixel
                // requires source pixels that weren't provided 
                break;
            }

            const float* pSrc   = imgSrcBlk.Ptr(u, v);
            // bicubic coefs are in groups of 4, thus uc*4, vc*4
            int uc = F2I((a.x-aflr.x)*float(WARP_LOOKUP_COUNT));
            const float* pxcoef = pFilterCoef+uc*4;
            int vc = F2I((a.y-aflr.y)*float(WARP_LOOKUP_COUNT));
            const float* pycoef = pFilterCoef+vc*4;

            for( int b = 0; b < nb; b++, pDstF++, pSrc++ )
            {
                const float *pr0 = pSrc;
                const float *pr1 = pr0+nb, *pr2 = pr1+nb, *pr3 = pr2+nb; 
                float y0 = CubicFilterPixel(*pr0, *pr1, *pr2, *pr3, pxcoef);

                pr0 = (float*)((Byte*)pr0 + stride);
                pr1 = pr0+nb, pr2 = pr1+nb, pr3 = pr2+nb; 
                float y1 = CubicFilterPixel(*pr0, *pr1, *pr2, *pr3, pxcoef);

                pr0 = (float*)((Byte*)pr0 + stride);
                pr1 = pr0+nb, pr2 = pr1+nb, pr3 = pr2+nb; 
                float y2 = CubicFilterPixel(*pr0, *pr1, *pr2, *pr3, pxcoef);

                pr0 = (float*)((Byte*)pr0 + stride);
                pr1 = pr0+nb, pr2 = pr1+nb, pr3 = pr2+nb;
                float y3 = CubicFilterPixel(*pr0, *pr1, *pr2, *pr3, pxcoef);

                *pDstF = CubicFilterPixel(y0, y1, y2, y3, pycoef);
            }
        }

        int iCurSpan = 0;
        for( ;x < iSpan; x++, iCurSpan++ )
        {
            CVec2f aflr, a = pAddr[x];
            T_FLOOR(&aflr.x, &a.x);
            int u = F2I(aflr.x)-leftB;
            int v = F2I(aflr.y)-topB;
            int ur = rightB-u;
            int vb = bottomB-v;
            if( ((u|v|ur|vb) & 0x80000000) == 0 )
            {
                // opposite termination condition from above while(), 
                // thus keep accumulating OOB addresses
                break;
            }
        }
        if( iCurSpan )
        {
            // clear the OOB span
            memset(pDstF, 0, iCurSpan*nb*sizeof(float));
            pDstF += iCurSpan*nb;
        }
    }
}

inline void
BicubicWarpSpan(float* pDstF, 
                const CFloatImg& imgSrcBlk, const vt::CRect& rctSrcBlk,
                const CVec2f* pAddr, const float* pFilterCoef, int iSpan)
{
    if( g_SupportSSE2() )
    {
#if defined(VT_SSE_SUPPORTED)
        if( imgSrcBlk.Bands()==4  )
        {
            BicubicWarpSpan4SSE(pDstF, imgSrcBlk, rctSrcBlk, pAddr, 
                                pFilterCoef, iSpan);
        }
        else
        {
            BicubicWarpSpan<floor2SSE>(pDstF, imgSrcBlk, rctSrcBlk, pAddr, 
                                       pFilterCoef, iSpan);
        }
#endif
    }
    else
    {
        BicubicWarpSpan<floor2>(pDstF, imgSrcBlk, rctSrcBlk, pAddr, 
                                pFilterCoef, iSpan);
    }
}

//
// WarpBlock -
//
// WarpBlock can only be called with Byte data under the following conditions,
// which must be qualified prior to calling: both srcBlk and dstBlk must be
// byte data; the number of bands must match for 1,2,4 bands; for 3 band dest
// the source must be 4 band; bilinear sampler only;
static HRESULT
WarpBlock(CImg& imgDstBlk, const CPoint& ptDst, 
          const CImg& imgSrcBlk, const CPoint& ptSrc, 
          IAddressGenerator** ppGenChain, UInt32 uGenChainCnt, 
          const float* pFilterCoef, eSamplerKernel sampler)
{
    VT_HR_BEGIN()

    int nSrcB = imgSrcBlk.Bands();
    int nDstB = imgDstBlk.Bands();

    // currently the temp buffer calc assumes less than 4 bands
    VT_ASSERT( nDstB <= 4 );

    // create temp buffers on the stack
    const int c_maxaddrspan = 264;
     // must be even to avoid buffer overruns in the SSE code
    VT_ASSERT( (c_maxaddrspan&1) == 0 ); 

    VT_DECLSPEC_ALIGN(16) float tmpfloatbuf[c_maxaddrspan*4];

    CVec2f addrbuf[c_maxaddrspan];

    VT_ASSERT( uGenChainCnt > 0 );

    vt::CRect rctSrc(ptSrc.x, ptSrc.y,
                     ptSrc.x+imgSrcBlk.Width(), ptSrc.y+imgSrcBlk.Height());

    if( rctSrc.IsRectEmpty() )
    {
        imgDstBlk.Clear();
        return S_OK;
    }

    // for each line
    int iWidth = imgDstBlk.Width();
    bool bAddrCanWrap = ppGenChain[uGenChainCnt-1]->AddressWrapX() != 0 ||
                        ppGenChain[uGenChainCnt-1]->AddressWrapY() != 0;

    for( int y = 0; y < imgDstBlk.Height(); y++)
    {
        for( int k = 0; k < iWidth; k+=c_maxaddrspan )
        {   
            int iCurWidth = VtMin(iWidth-k, c_maxaddrspan);

            VT_HR_EXIT( TraverseAddressGenChain(
                addrbuf, vt::CPoint(ptDst.x+k, ptDst.y+y), iCurWidth,
                ppGenChain, uGenChainCnt) );

            if( bAddrCanWrap )
            {
                WrapAddressesIntoSrcBuf(addrbuf, iCurWidth, rctSrc,
                                        ppGenChain[uGenChainCnt-1]);
            }
                
            // loop over addresses, clear the dst image for spans corresponding
            // to invalid addresses, run warp on spans of valid addresses
            for( int x = 0; x < iCurWidth; )
            {
                int iCurSpan = 0;
                for( ;x < iCurWidth && !IsAddressOOB(addrbuf[x].x); x++ )
                {
                    iCurSpan++;   
                }

                if( iCurSpan )
                {
                    // setup addresses for this span
                    int iX0      = x-iCurSpan;
                    Byte* pDst   = imgDstBlk.BytePtr(k+iX0,y);

                    if( EL_FORMAT(imgSrcBlk.GetType()) == EL_FORMAT_BYTE )
                    {
                        BilinearWarpSpan( pDst, VT_IMG_BANDS(imgDstBlk.GetType()), 
                            static_cast<const CByteImg&>(imgSrcBlk), 
                            rctSrc, addrbuf+iX0, iCurSpan );
                    }

                    else
                    {
                        float* pDstF = (EL_FORMAT(imgDstBlk.GetType())==EL_FORMAT_FLOAT && 
                                        nSrcB==nDstB)? (float*)pDst: tmpfloatbuf;

                        // run the filter
                        if( sampler==eSamplerKernelNearest )
                        {
                            NearestNeighborWarpSpan( pDstF, static_cast<const CFloatImg&>(imgSrcBlk), 
                                                     rctSrc, addrbuf+iX0, iCurSpan );
                        }
                        else if( sampler==eSamplerKernelBilinear )
                        {
                            BilinearWarpSpan( pDstF, static_cast<const CFloatImg&>(imgSrcBlk), rctSrc, addrbuf+iX0, 
                                              iCurSpan );
                        }
                        else
                        {
                            BicubicWarpSpan( pDstF, static_cast<const CFloatImg&>(imgSrcBlk), rctSrc, addrbuf+iX0, 
                                             pFilterCoef, iCurSpan );
                        }

                        // convert to output type if necessary
                        if( (void*)pDstF != (void*)pDst )
                        {
                            VtConvertSpan(pDst,  imgDstBlk.GetType(), pDstF,
                                          VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, nSrcB), 
                                          iCurSpan*nSrcB, true);
                        }
                    }
					iCurSpan = 0;
                }
				    
                for( ;x < iCurWidth && IsAddressOOB(addrbuf[x].x); x++ )
                {
                    iCurSpan++;   
                }
                if( iCurSpan )
                {
                    // zero out invalid spans
                    memset(imgDstBlk.BytePtr(k+x-iCurSpan,y), 0,
                           iCurSpan*imgDstBlk.PixSize());
                }
            }
        }
    }

    VT_HR_END()
}

//+-----------------------------------------------------------------------------
//
// Determine whether the given sampler requires a pre-processing step.
//
//------------------------------------------------------------------------------
static bool NeedsBSplineProcess(const eSamplerKernel sampler)
{
    return sampler == eSamplerKernelBicubicBSpline;
}

//+-----------------------------------------------------------------------------
//
// Pre-process srcOld into srcNew according to sampler type.
//
//------------------------------------------------------------------------------
static HRESULT BSplinePreProcess(CFloatImg& srcNew, 
                                const CImg& srcOld, 
                                const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend))
{
    VT_HR_BEGIN();

    CFloatImg oldFloat;
    const CFloatImg* pFloat = NULL;
    if (EL_FORMAT(srcOld.GetType()) != EL_FORMAT_FLOAT)
    {
        VT_HR_EXIT( oldFloat.Create(srcOld.Width(), srcOld.Height(), srcOld.Bands()) );
        VT_HR_EXIT( VtConvertImage(oldFloat, srcOld) );
        pFloat = &oldFloat;
    }
    else
    {
        pFloat = reinterpret_cast<const CFloatImg*>(&srcOld);
    }
    VT_HR_EXIT( srcNew.Create(
        srcOld.Width(), srcOld.Height(), srcOld.Bands()) );

    VT_HR_EXIT( VtPreprocessBicubicBSpline(srcNew, *pFloat, ex) );

    VT_HR_END();
}

//+-----------------------------------------------------------------------------
//
// Intermediate destination must be floating point. If input pDst is a floating
// point image, do nothing, otherwise replace with properly initialized floatDst.
//
//------------------------------------------------------------------------------
static HRESULT GetBSplinePreProcessDst( CImg*& pDst, CFloatImg& floatDst )
{
    VT_HR_BEGIN();

    VT_PTR_EXIT(pDst);

    if (EL_FORMAT(pDst->GetType()) != EL_FORMAT_FLOAT)
    {
        VT_HR_EXIT( floatDst.Create(pDst->Width(), pDst->Height(), pDst->Bands()) );
        pDst = &floatDst;
    }

    VT_HR_END();
}

//+-----------------------------------------------------------------------------
//
// After processing, might have to convert float image back into the expected
// type.
//------------------------------------------------------------------------------
static HRESULT PutBSplinePreProcessDst( CImg& imgDst, const CImg& floatDst )
{
    VT_HR_BEGIN();

    if( !imgDst.IsSharingMemory(floatDst) )
    {
        VT_ASSERT(EL_FORMAT(floatDst.GetType()) == EL_FORMAT_FLOAT);
        VT_ASSERT(EL_FORMAT(imgDst.GetType()) != EL_FORMAT_FLOAT);

        VT_HR_EXIT( VtConvertImage(imgDst, floatDst) );
    }

    VT_HR_END();
}

//+-----------------------------------------------------------------------------
//
// Post process the image, either converting or copying back into pimgDst.
// Non-static due to usage in resize.cpp.
//
//------------------------------------------------------------------------------
HRESULT BSplinePostProcess(CImg* pimgDst, 
                                const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend))
{
    VT_HR_BEGIN();

    // Note that the PostProcess operation is identical to the pre-processing
    CFloatImg postProcDst;
    VT_HR_EXIT( BSplinePreProcess(postProcDst, *pimgDst, ex) );

    // Need the output of the post processing back in pimgDst. If it is a float
    // image, just copy, otherwise do a convert.
    if (EL_FORMAT(pimgDst->GetType()) == EL_FORMAT_FLOAT)
    {
        VT_HR_EXIT( postProcDst.CopyTo(*pimgDst) );
    }
    else
    {
        VT_HR_EXIT( PutBSplinePreProcessDst(*pimgDst, postProcDst) );
    }

    VT_HR_END();
}

//+-----------------------------------------------------------------------------
//
// Pre-process the image and then perform the requested warp. May have to do some 
// conversions to and from the float representation required by the B-spline
// samplers.
//------------------------------------------------------------------------------
static HRESULT BSplinePreProcessAndWarp( CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
                                 IAddressGenerator** ppGenChain, UInt32 uGenChainCnt, 
                                 const IMAGE_EXTEND& ex )
{
    VT_HR_BEGIN();

    CFloatImg newSrc;
    // Pre-process the source image for bicubic b-spline.
    VT_HR_EXIT( BSplinePreProcess(newSrc, imgSrc, ex) );
        
    CImg* pDst = &imgDst;
    CFloatImg floatDst;
    VT_HR_EXIT( GetBSplinePreProcessDst(pDst, floatDst) );

    // Now call this function again, substituting in the newly created source image, and
    // changing the sampling type to indicate we've already preprocessed.
    VT_HR_EXIT( VtWarpImage(*pDst, rctDst, newSrc, ppGenChain, uGenChainCnt, 
                    eSamplerKernelBicubicBSplineSrcPreprocessed, ex) );

    VT_HR_EXIT( PutBSplinePreProcessDst(imgDst, *pDst) );

    VT_HR_END();
}

HRESULT
vt::VtWarpImage( CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
                 IAddressGenerator** ppGenChain, UInt32 uGenChainCnt, 
                 eSamplerKernel sampler, const IMAGE_EXTEND& ex )
{
    return VtWarpImageInternal(imgDst,rctDst,imgSrc,ppGenChain,uGenChainCnt,sampler,ex);
}

HRESULT
VtWarpImageInternal( CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
                         IAddressGenerator** ppGenChain, UInt32 uGenChainCnt, 
                         eSamplerKernel sampler, const IMAGE_EXTEND& ex,
#if defined(USE_WARPSPECIAL_CODEPATHS)
                         const CMtx3x3f* pMtx)
#else
                         const CMtx3x3f*)
#endif
{
    if( imgSrc.IsSharingMemory(imgDst) )
    {
        // the operation cannot be done in-place
        return E_INVALIDARG;
    }
    if (sampler == eSamplerKernelBicubicBSplineSrcPreprocessed && EL_FORMAT(imgSrc.GetType()) != EL_FORMAT_FLOAT)
    {
        // Output of required pre-processing must be float to capture range of possible values.
        // Forget to preprocess? Also, can't convert type between preprocess and resample.
        VT_ASSERT(false); 
        return E_INVALIDARG;
    }
    
    // mafinch - adding another supported sampler type.
    if( !( sampler==eSamplerKernelNearest
        || sampler==eSamplerKernelBilinear 
        || sampler==eSamplerKernelBicubic 
        || sampler==eSamplerKernelBicubicBSplineSrcPreprocessed
        || sampler==eSamplerKernelBicubicBSpline) )
    {
        return E_NOTIMPL;
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

    if( NeedsBSplineProcess(sampler) )
    {
        // Preprocess the image and then apply the warp, after which we are done.
        return BSplinePreProcessAndWarp(imgDst, rctDst, imgSrc, ppGenChain, uGenChainCnt, ex);
    }

    // If we've gotten this far, we either aren't using the bicubic b-spline, or it
    // has been pre-processed, in which case the sampler should be eSamplerKernelBicubicBSplineSrcPreprocessed.
    VT_ASSERT( sampler != eSamplerKernelBicubicBSpline );

    WarpBlockFunc warpSpecial = NULL;
#if defined(USE_WARPSPECIAL_CODEPATHS)
    SelectWarpSpecial(warpSpecial, imgDst,rctDst,imgSrc,sampler,ex,pMtx);
#endif    

    // warp special routines work directly from source images, for which larger
    // blocks runs faster
    const UInt32 c_blocksize = (warpSpecial)?(768):(128);

    // loop over the destination blocks
    for( CBlockIterator bi(rctDst, c_blocksize); !bi.Done(); bi.Advance() )
    {
        // get blk rct in dst image coords
        vt::CRect rctDstBlk = bi.GetRect();

        CImg imgDstBlk;
        imgDst.Share(imgDstBlk, &rctDstBlk);

        // offset to layer coords
        rctDstBlk += rctDst.TopLeft();

#if defined(USE_WARPSPECIAL_CODEPATHS)
        if (warpSpecial)  
        { 
            VT_HR_EXIT( (*warpSpecial)(imgDstBlk, rctDstBlk.TopLeft(), imgSrc, pMtx, ex) ); 
        }
        else
#endif
        {
            // use the general purpose WarpBlock

            // determine the required source block
            vt::CRect rctSrcBlk;
            VT_HR_EXIT( MapDstRectToSrc(rctSrcBlk, rctDstBlk, ppGenChain, uGenChainCnt) );
            if( rctSrcBlk.IsRectEmpty() )
            {
                imgDstBlk.Clear();
                continue;
            }

            // adjust the rect for the necessary filter support
            // mafinch - same adjustment for cubic B-spine as bicubic
            if( sampler==eSamplerKernelBicubic || sampler==eSamplerKernelBicubicBSplineSrcPreprocessed)
            {
                rctSrcBlk.left-=1; rctSrcBlk.right+=2;
                rctSrcBlk.top-=1; rctSrcBlk.bottom+=2;
            }
            else if( sampler==eSamplerKernelBilinear )
            {
                rctSrcBlk.right+=1;
                rctSrcBlk.bottom+=1;
            }
        
            int iBS = imgSrc.Bands();

            int srcT = imgSrc.GetType();
            int dstT = imgDst.GetType();
            if( VT_IMG_SAME_E(srcT,dstT) &&
                (EL_FORMAT(srcT) == EL_FORMAT_BYTE) &&
                (imgSrc.Bands() <= 4) &&
                ( (imgSrc.Bands() == imgDst.Bands()) ||
                  ((imgSrc.Bands()==4)&&(imgDst.Bands()==3)) ) &&
                (sampler==eSamplerKernelBilinear) )
            {
                CByteImg bimgSrcBlk;
                if( vt::CRect(imgSrc.Rect()).RectInRect(&rctSrcBlk) && 
                    (imgSrc.Bands() != 3) )
                {
                    // share the block out of the source if possible
                    imgSrc.Share(bimgSrcBlk, &rctSrcBlk);
                }
                else
                {
                    // copy/pad the image for out of bounds and/or 3 band source
                    VT_HR_EXIT( bimgSrcBlk.Create(rctSrcBlk.Width(), 
                        rctSrcBlk.Height(), (iBS==3)? 4: iBS) );
                    VT_HR_EXIT( VtCropPadImage(bimgSrcBlk, rctSrcBlk, imgSrc, ex) );
                }

                // warp the block
                VT_HR_EXIT( WarpBlock(imgDstBlk, rctDstBlk.TopLeft(), 
                                      bimgSrcBlk, rctSrcBlk.TopLeft(),
                                      ppGenChain, uGenChainCnt, 
                                      GetDefaultCoef(sampler), sampler) );  
            }
            else
            {
                CFloatImg fimgSrcBlk;

                if( EL_FORMAT(imgSrc.GetType()) == EL_FORMAT_FLOAT  &&
                    vt::CRect(imgSrc.Rect()).RectInRect(&rctSrcBlk) && 
                    iBS!=3 )
                {
                    // share the block out of the source if possible
                    imgSrc.Share(fimgSrcBlk, &rctSrcBlk);
                }
                else
                {
                    // use 4 banded in the 3band case since that code path is faster
                    VT_HR_EXIT( fimgSrcBlk.Create(rctSrcBlk.Width(), rctSrcBlk.Height(), 
                                                 (iBS==3)? 4: iBS) );

                    // pad the image when out of bounds pixels are requested
                    VT_HR_EXIT( VtCropPadImage(fimgSrcBlk, rctSrcBlk, imgSrc, ex) );
                }

                // warp the block
                VT_HR_EXIT( WarpBlock(imgDstBlk, rctDstBlk.TopLeft(), 
                                      fimgSrcBlk, rctSrcBlk.TopLeft(),
                                      ppGenChain, uGenChainCnt, 
                                      GetDefaultCoef(sampler), sampler) );  
            }
        }
    }

    VT_HR_END()
}

HRESULT 
vt::VtWarpImage( CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
                 const CMtx3x3f& xfrm, eSamplerKernel sampler,
                 const IMAGE_EXTEND& ex )
{ 
    if( IsMatrixAnisoScaleTrans(xfrm, rctDst) 
        && sampler!=eSamplerKernelBicubic && sampler!=eSamplerKernelNearest )
    {
        // if the transform is only doing scale+translate then we can call
        // VtResizeImg instead
        // TODO: 
        //       - extend this check for simple matrix to CWarpTransform
        return VtResizeImage(imgDst, rctDst, imgSrc, 
                             xfrm(0,0), xfrm(0,2), xfrm(1,1), xfrm(1,2), sampler, ex);
    }
    else
    {

        // clip to a slightly expanded source image - this gives the warper the
        // necessary pixels of padding 
        vt::CRect rSrcClip(-imgSrc.Width(),-imgSrc.Height(), 
                           2*imgSrc.Width(), 2*imgSrc.Height());
        C3x3TransformAddressGen g(xfrm, rSrcClip, rctDst);
        IAddressGenerator* pg = &g;
        // call internal routine passing transform so specialized routines can potentially be used
        return VtWarpImageInternal(imgDst, rctDst, imgSrc, &pg, 1, sampler, ex, &xfrm);
    }
}


#ifndef VT_NO_XFORMS
//+-----------------------------------------------------------------------------
//
// Pre-process the input source image for bicubic B-spline resampling,
// and then perform the resampling warp operation.
//------------------------------------------------------------------------------
static HRESULT BSplinePreProcessAndTransform(CImg* pimgDst, const vt::CRect& rctDst, 
                          const CImg& src, const vt::CPoint& ptSrc, 
                          CAddressGeneratorChain& genChain)
{
    VT_HR_BEGIN();

    /// Preprocess the image.
    CFloatImg preprocSrc;
            
    VT_HR_EXIT( BSplinePreProcess(preprocSrc, src) );

    /// Call WarpBlock on preprocessed image with revised sampler type.
    const eSamplerKernel preprocSampler = eSamplerKernelBicubicBSplineSrcPreprocessed;
    CFloatImg dstFloat;
    CImg* pDstFloat = pimgDst;
    VT_HR_EXIT( GetBSplinePreProcessDst(pDstFloat, dstFloat) );

    VT_HR_EXIT( WarpBlock(*pDstFloat, rctDst.TopLeft(), preprocSrc, ptSrc, 
                    genChain.GetChain(), genChain.GetChainLength(),
                    GetDefaultCoef(preprocSampler), preprocSampler) );

    VT_HR_EXIT( PutBSplinePreProcessDst(*pimgDst, *pDstFloat) );

    VT_HR_END();
}

//+-----------------------------------------------------------------------------
//
// Pre-process the input source image for bicubic B-spline resampling,
// and then perform the resampling seperable kernel (resize) operation.
// This should be called when upsampling with the Bicubic BSpline.
//------------------------------------------------------------------------------
static HRESULT BSplinePreprocessAndTransform(CImg* pimgDst, const CRect& rctDst,
                            const CImg& src, const CPoint& ptSrc,
                            CSeparableFilterTransform* pFilterTransform)
{
    VT_HR_BEGIN();

    ANALYZE_ASSUME(pimgDst != NULL);
    ANALYZE_ASSUME(pFilterTransform != NULL);
    VT_ASSERT(pimgDst != NULL);
    VT_ASSERT(pFilterTransform != NULL);

    /// Preprocess the image.
    CFloatImg preprocSrc;
            
    VT_HR_EXIT( BSplinePreProcess(preprocSrc, src) );

    CFloatImg dstFloat;
    CImg* pDstFloat = pimgDst;
    VT_HR_EXIT( GetBSplinePreProcessDst(pDstFloat, dstFloat) );

    VT_HR_EXIT( pFilterTransform->Transform(pimgDst, rctDst, src, ptSrc) );

    VT_HR_EXIT( PutBSplinePreProcessDst(*pimgDst, *pDstFloat) );

    VT_HR_END();
}

//+-----------------------------------------------------------------------------
//
// Apply input transformation, and then apply post processing. This
// should be called when downsampling using the BicubicBSpline.
//
//------------------------------------------------------------------------------
static HRESULT BSplineTransformAndPostProcess(CImg* pimgDst, const CRect& rctDst,
                            const CImg& src, const CPoint& ptSrc,
                            CSeparableFilterTransform* pFilterTransform)
{
    VT_HR_BEGIN();

    ANALYZE_ASSUME(pFilterTransform != NULL);
    VT_ASSERT(pimgDst != NULL);
    VT_ASSERT(pFilterTransform != NULL);

    VT_HR_EXIT( pFilterTransform->Transform(pimgDst, rctDst, src, ptSrc) );

    /// Is imgDst now certain to be a float image? I don't think so. I think
    /// it is converted to float strip by strip during processing, 
    /// and immediately converted back to final type. mafinch
    
    VT_HR_EXIT( BSplinePostProcess(pimgDst) );

    VT_HR_END();
}

static HRESULT TransformInternal(CImg* pimgDst, const CRect& rctDst,
                            const CImg& src, const CPoint& ptSrc,
                            const eSamplerKernel sampler, const bool isDownSample,
                            CSeparableFilterTransform* pFilterTransform)
{
    if( NeedsBSplineProcess(sampler) )
    {
        if( isDownSample )
        {
            return BSplineTransformAndPostProcess(pimgDst, rctDst, src, ptSrc, pFilterTransform);
        }
        else
        {
            return BSplinePreprocessAndTransform(pimgDst, rctDst, src, ptSrc, pFilterTransform);
        }
    }

    return pFilterTransform->Transform(pimgDst, rctDst, src, ptSrc);

}

//+-----------------------------------------------------------------------------
//
// Class: CWarpTransform
// 
//------------------------------------------------------------------------------
HRESULT 
CWarpTransform::Clone(ITaskState **ppState)
{
    return CloneTaskState<CWarpTransform>(ppState, [this](CWarpTransform* pN)
    { 
        return m_bIsResize?
            pN->InitializeResize(m_mtxResize(0,0), m_mtxResize(0,2), 
                                    m_mtxResize(1,1), m_mtxResize(1,2), 
                                    m_dstType, m_sampler):
            pN->Initialize( m_genChain.GetChain(), m_genChain.GetChainLength(), 
                            m_dstType, m_sampler ); 
    });
}

void
CWarpTransform::GetSrcPixFormat(IN OUT int* pfrmtSrcs, 
                                IN UInt32  uSrcCnt,
                                IN int frmtDst)
{
    if ( m_pFilterTransform )
    {
        m_pFilterTransform->GetSrcPixFormat(pfrmtSrcs, uSrcCnt, frmtDst);
    }
    else if (m_NoRW)
    {
    }
    else
    {
        // if the dst type has not been fixed then use the dst type from the
        // graph
        // if internal type is undef then use external type
        // else merge the fixed parts
        if( m_dstType==OBJ_UNDEFINED )
        {
             pfrmtSrcs[0] = frmtDst;
        }
        else if( frmtDst == OBJ_UNDEFINED ) 
        {
            pfrmtSrcs[0] = m_dstType;
        }
        else
        {
            pfrmtSrcs[0] = UpdateMutableTypeFields(m_dstType, frmtDst);
        }

        if( pfrmtSrcs[0] & VT_IMG_FIXED_PIXFRMT_MASK )
        {
            // for resize, can use byte specialization for 1..4 bands and
            // byte source
            if( m_bIsResize &&
                (EL_FORMAT(pfrmtSrcs[0])==EL_FORMAT_BYTE) &&
                ((VT_IMG_BANDS(m_dstType) <= 4) && (VT_IMG_BANDS(pfrmtSrcs[0]) <= 4)) )
            { 
                int iOpBands = VT_IMG_BANDS(pfrmtSrcs[0]) == 3 ? 4 :
                    VT_IMG_BANDS(pfrmtSrcs[0]);
                pfrmtSrcs[0]  = VT_IMG_MAKE_COMP_TYPE(
                    PIX_FORMAT(pfrmtSrcs[0]), EL_FORMAT_BYTE, iOpBands);
            }
            // for non-resize, request byte if src/dst are matching 1..4banded byte for bilinear
            else if( !m_bIsResize &&
                (m_dstType != OBJ_UNDEFINED) && 
                (VT_IMG_SAME_PBE(pfrmtSrcs[0],m_dstType)) &&
                (EL_FORMAT(pfrmtSrcs[0]) == EL_FORMAT_BYTE) &&
                (VT_IMG_BANDS(m_dstType) <= 4) &&
                (m_sampler == eSamplerKernelBilinear) )
            {
                // byte code requires 4 band source for 3 band dest
                if (VT_IMG_BANDS(m_dstType) == 3)
                {
                    pfrmtSrcs[0] = VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_RGBA, EL_FORMAT_BYTE, 4);
                }
                else
                {
                    pfrmtSrcs[0]  = VT_IMG_MAKE_COMP_TYPE(
                        PIX_FORMAT(pfrmtSrcs[0]), EL_FORMAT_BYTE, VT_IMG_BANDS(pfrmtSrcs[0]));
                }
            }
            else if (VT_IMG_BANDS(pfrmtSrcs[0])==3)
            {
                // 4banded path is optimized so opt for that in 3banded case
                pfrmtSrcs[0]  = VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_RGBA, EL_FORMAT_FLOAT, 4);
            }
            else
            {
                pfrmtSrcs[0]  = VT_IMG_MAKE_COMP_TYPE(
                    PIX_FORMAT(pfrmtSrcs[0]), EL_FORMAT_FLOAT, VT_IMG_BANDS(pfrmtSrcs[0]));
            }
            pfrmtSrcs[0] |= VT_IMG_FIXED_ELFRMT_MASK|VT_IMG_FIXED_PIXFRMT_MASK;
        }
        else
        {
            pfrmtSrcs[0] = VT_IMG_FIXED_ELFRMT_MASK|EL_FORMAT_FLOAT;
        }
    }
}

void
CWarpTransform::GetDstPixFormat(OUT int& frmtDst,
                                IN  const int* pfrmtSrcs, 
                                IN  UInt32  uSrcCnt)
{ 
    if ( m_pFilterTransform )
    {
        m_pFilterTransform->GetDstPixFormat(frmtDst, pfrmtSrcs, uSrcCnt);
    }
    else if (m_NoRW)
    {
        frmtDst = m_dst.GetFullType();
    }
    else
    {
        frmtDst = UpdateMutableTypeFields(m_dstType, *pfrmtSrcs);
        if( !IsValidConvertPair(*pfrmtSrcs, frmtDst) )
        {
            frmtDst = OBJ_UNDEFINED;
        }
    }
}

vt::CRect
CWarpTransform::GetRequiredSrcRect(const vt::CRect& rctDst)
{
    HRESULT hr = S_OK;
    vt::CRect rctSrc;

    if( m_bIsResize )
    {
        if( m_pFilterTransform )
        {
            return m_pFilterTransform->GetRequiredSrcRect(rctDst);
        }

        rctSrc = MapRegion3x3(m_mtxResize, rctDst);
    }
    else
    {
        VT_ASSERT( m_pFilterTransform == NULL );
        hr = vt::MapDstRectToSrc(rctSrc, rctDst, m_genChain);
    }

    if( hr == S_OK && !rctSrc.IsRectEmpty() )
    {
        // add the necessary pixels of padding
        if( m_sampler==eSamplerKernelBicubic 
            || m_sampler==eSamplerKernelBicubicBSplineSrcPreprocessed)
        {
            rctSrc.left-=1; rctSrc.right+=2;
            rctSrc.top-=1; rctSrc.bottom+=2;  
        }
        else if( m_sampler==eSamplerKernelBicubicBSpline)
        {
            // this sampler will require extra padding for the pre-processing step
            rctSrc.left-=1; rctSrc.right+=2;
            rctSrc.top-=1; rctSrc.bottom+=2;  
            rctSrc.InflateRect(CPreprocessBicubicBSpline::Expansion(), 
                               CPreprocessBicubicBSpline::Expansion());
        }
        // adjust the rect for the necessary filter support
        // if( sampler==eSamplerKernelBilinear ) // for now only bilinear
        // due to the incremental computation of the coordinates in the
        // following loop vs. the direct computation above, sometimes we
        // see an off by one issue on the right/bottom pixels so increment
        // by 2 here instead of 1
        else if( m_sampler==eSamplerKernelBilinear)
        {
            rctSrc.right+=2;
            rctSrc.bottom+=2;
        }
        else
        {
            rctSrc.right+=1;
            rctSrc.bottom+=1;  
        }
    }
    else
    {
        rctSrc = vt::CRect(0,0,0,0);
    }

    return rctSrc;
}

vt::CRect
CWarpTransform::GetAffectedDstRect(const vt::CRect& rctSrc)
{
    HRESULT hr = S_OK;
    vt::CRect rctDst;

    if( m_bIsResize )
    {
        if( m_pFilterTransform )
        {
            return m_pFilterTransform->GetAffectedDstRect(rctSrc);
        }

        rctDst = MapRegion3x3(m_mtxResize.Inv(), rctSrc);
    }
    else
    {
        VT_ASSERT( m_pFilterTransform == NULL );
        hr = vt::MapSrcRectToDst(rctDst, rctSrc, m_genChain);
    }

    if( hr == S_OK && !rctDst.IsRectEmpty() )
    {
        if( m_sampler==eSamplerKernelBicubic 
            || m_sampler==eSamplerKernelBicubicBSplineSrcPreprocessed )
        {
            rctDst.left-=2; rctDst.right+=2;
            rctDst.top-=2; rctDst.bottom+=2;  
        }
        else if( m_sampler==eSamplerKernelBicubicBSpline)
        {
            // this sampler will require extra padding for the pre-processing step
            rctDst.left-=2; rctDst.right+=2;
            rctDst.top-=2; rctDst.bottom+=2;  
            rctDst.InflateRect(CPreprocessBicubicBSpline::Expansion(), CPreprocessBicubicBSpline::Expansion());
        }
        else
        {
            rctDst.left-=1; rctDst.right+=1;
            rctDst.top-=1; rctDst.bottom+=1;  
        }
    }
    else
    {
        rctDst = vt::CRect(0,0,0,0);
    }
    return rctDst;

}

vt::CRect CWarpTransform::GetResultingDstRect(const vt::CRect& rctSrc)
{
    vt::CRect rctDst;
    if( m_bIsResize )
    {
        if( m_pFilterTransform )
        {
            return m_pFilterTransform->GetResultingDstRect(rctSrc);
        }
        rctDst = MapRegion3x3(m_mtxResize.Inv(), rctSrc);
    }
    else
    {
        VT_ASSERT( m_pFilterTransform == NULL );
        vt::MapSrcRectToDst(rctDst, rctSrc, m_genChain);
    }
    return rctDst;
};


HRESULT
CWarpTransform::Transform(CImg* pimgDst, const vt::CRect& rctDst, 
                          const CImg& src, const vt::CPoint& ptSrc)
{
    if (m_NoRW)
    {
        VT_HR_BEGIN()
        if (m_WarpBlockFunc)  
        { 
            CImg imgDstBlk; VT_HR_EXIT( m_dst.Share(imgDstBlk,rctDst) );
            VT_HR_EXIT( (*m_WarpBlockFunc)(imgDstBlk, rctDst.TopLeft(), m_src, 
                                           &m_xfrm, m_extend) ); 
        }
        VT_HR_END()
    }

    if( m_bIsResize )
    {
        if( m_pFilterTransform )
        {
            return TransformInternal(pimgDst, rctDst,
                                            src, ptSrc,
                                            m_sampler, m_bDownsample,
                                            m_pFilterTransform);
        }
        else
        {
            if ( m_sampler == eSamplerKernelNearest )
            {
                C3x3TransformAddressGen g(m_mtxResize, vt::CRect(src.Rect())+ptSrc, rctDst);
                IAddressGenerator* pg = &g;
                return WarpBlock(*pimgDst, rctDst.TopLeft(), src, ptSrc, 
                                 &pg, 1, GetDefaultCoef(m_sampler), m_sampler);
            }
            else if (EL_FORMAT(src.GetType()) == EL_FORMAT_BYTE)
            {
                return BilinearResizeBlock(
                    *pimgDst, rctDst, (CByteImg&)src, vt::CRect(src.Rect())+ptSrc, 
                    m_mtxResize(0,0), m_mtxResize(0,2), m_mtxResize(1,1), m_mtxResize(1,2));
            }
            else
            {
                return BilinearResizeBlock(
                    *pimgDst, rctDst, (CFloatImg&)src, vt::CRect(src.Rect())+ptSrc, 
                    m_mtxResize(0,0), m_mtxResize(0,2), m_mtxResize(1,1), m_mtxResize(1,2));
            }
        }
    }
    else
    {
        VT_ASSERT( m_pFilterTransform == NULL );
        if( NeedsBSplineProcess(m_sampler) )
        {
            return BSplinePreProcessAndTransform(pimgDst, rctDst, src, ptSrc, m_genChain);
        }

        return WarpBlock(*pimgDst, rctDst.TopLeft(), src, ptSrc, 
                         m_genChain.GetChain(), m_genChain.GetChainLength(),
                         GetDefaultCoef(m_sampler), m_sampler);
    }
}
 
HRESULT 
CWarpTransform::Initialize(IAddressGenerator** ppGenChain, UInt32 uGenChainCnt,
                           int dstType, eSamplerKernel sampler)
{
    if (m_NoRW) { m_NoRW = false; m_src.Deallocate(); m_dst.Deallocate(); }

    Clear();
    m_bIsResize = false;
    m_sampler   = sampler;
    m_dstType   = dstType;
    return m_genChain.SetChain(ppGenChain, uGenChainCnt);
}

HRESULT 
CWarpTransform::Initialize(const CMtx3x3f& xfrm, 
                           const vt::CRect& rctSrcClip,
                           const vt::CRect& rctDstClip,
                           int dstType, 
                           eSamplerKernel sampler)
{
    if (m_NoRW) { m_NoRW = false; m_src.Deallocate(); m_dst.Deallocate(); }

    if( IsMatrixAnisoScaleTrans(xfrm, rctDstClip) && 
        sampler!=eSamplerKernelNearest &&
        sampler!=eSamplerKernelBicubic &&
        sampler!=eSamplerKernelBicubicBSplineSrcPreprocessed && 
        sampler!=eSamplerKernelBicubicBSpline )
    {
        return InitializeResize(xfrm(0,0), xfrm(0,2), xfrm(1,1), xfrm(1,2), 
                                dstType, sampler);
    }
    else
    {
        // clip to a slightly expanded source image - this gives the warper the
        // necessary pixels of padding 
        vt::CRect rSrcClipEx(rctSrcClip.left-2, rctSrcClip.top-2, 
                             rctSrcClip.right+2, rctSrcClip.bottom+2);
        C3x3TransformAddressGen agen(xfrm, rSrcClipEx, rctDstClip);
        return Initialize(&agen, dstType, sampler);
    }
}

HRESULT 
CWarpTransform::Initialize(const CVec2Img& imgMap, 
                           int dstType, 
                           eSamplerKernel sampler)
{
    if (m_NoRW) { m_NoRW = false; m_src.Deallocate(); m_dst.Deallocate(); }

    CFlowFieldAddressGen a;
    HRESULT hr = a.Initialize(imgMap);
    if( hr == S_OK )
    {
        hr = Initialize(&a, dstType, sampler);
    }
    return hr;
}

// initialize routine for no-copy transform
HRESULT 
CWarpTransform::Initialize(const CMtx3x3f& xfrm, const vt::CImg& src, vt::CImg& dst,
                           eSamplerKernel sampler, const IMAGE_EXTEND& ex)
{
    m_bIsResize = false; Clear();

    m_NoRW = true;
    m_xfrm = xfrm;
    m_sampler = sampler;
    m_extend.Initialize(&ex);
    src.Share(m_src);
    dst.Share(m_dst);

    SelectWarpSpecial(m_WarpBlockFunc, m_dst,m_dst.Rect(),m_src,m_sampler,m_extend,&xfrm);
    if (NULL == m_WarpBlockFunc) { return E_FAIL; }

    return S_OK;
}

#endif
