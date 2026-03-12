//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Gaussian Pyramid class
//
//  History:
//      2004/11/08-mattu
//          Created
//
//------------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_pyramid.h"
#include "vt_warp.h"
#include "vt_imgmath.h"

using namespace vt;

HRESULT 
ConstructBilinearPyramid(CPyramid& dst, int iLastLevel = -1, int iStartLevel = 0);

inline UINT
ComputeTotalSubOctaveCount(int basew, int baseh, int sub_octaves = 1,
                           bool bTruncate = false)
{
    // compute the level count without sub octaves.
    int lc = ComputePyramidLevelCount(basew, baseh, 1, 1, bTruncate);

    // factor in the sub-octaves. since the last full octave level is 1x1, it
    // doesn't have sub-octaves so subtract 1 from 'lc' 
    return (lc-1)*sub_octaves + 1;
}

//+-----------------------------------------------------------------------------
//
//  Class:  CPyramid
//
//  Synopsis:  A pyramid of base images
//
//------------------------------------------------------------------------------
HRESULT CPyramid::Create(int iW, int iH, int iType,
                         const PYRAMID_PROPERTIES* pProps)
{
    HRESULT hr;

    Invalidate();
#if defined(_MSC_VER)
    m_cMetaData.DeleteAll();
#endif

    UINT uTotSubOct;

    VT_HR_EXIT( SetProperties(pProps) );

    uTotSubOct = ComputeTotalSubOctaveCount(
        iW, iH, m_props.iSubOctavesPerLevel, m_props.bTruncateOddLevels);
    VT_HR_EXIT( m_vecPyramid.resize(uTotSubOct-1) ); // adjust size for base

    VT_HR_EXIT( m_base.Create(iW, iH, iType) );

    VT_HR_EXIT( CreateLevels() );

Exit:
    return hr;
}

HRESULT CPyramid::Create(Byte *pbBuffer, int iW, int iH, int iStrideBytes,
                         int iType, const PYRAMID_PROPERTIES* pProps)
{
    HRESULT hr;

    Invalidate();
#if defined(_MSC_VER)
    m_cMetaData.DeleteAll();
#endif

    UINT uTotSubOct;

    VT_HR_EXIT( SetProperties(pProps) );

    uTotSubOct = ComputeTotalSubOctaveCount(
        iW, iH, m_props.iSubOctavesPerLevel, m_props.bTruncateOddLevels);
    VT_HR_EXIT( m_vecPyramid.resize(uTotSubOct-1) ); // adjust size for base 

    VT_HR_EXIT( m_base.Create(pbBuffer, iW, iH, iStrideBytes, iType) );

    VT_HR_EXIT( CreateLevels() );

Exit:
    return hr;
}

HRESULT CPyramid::Create(const CImg& src, const PYRAMID_PROPERTIES* pProps)
{
    HRESULT hr;

    Invalidate();
#if defined(_MSC_VER)
    m_cMetaData.DeleteAll();
#endif

    UINT uTotSubOct;

    VT_HR_EXIT( SetProperties(pProps) );

    uTotSubOct = ComputeTotalSubOctaveCount(src.Width(), src.Height(), 
                                                 m_props.iSubOctavesPerLevel,
                                                 m_props.bTruncateOddLevels);
    VT_HR_EXIT( m_vecPyramid.resize(uTotSubOct-1) ); // adjust size for base

    {
        int iType = GetType();
        if( iType == OBJ_UNDEFINED || iType == src.GetType() )
        {
            src.Share(m_base);
        }
        else
        {
            VT_HR_EXIT( VtConvertImage( m_base, src) );
        }
    }

    VT_HR_EXIT( CreateLevels() );

#if defined(_MSC_VER)
    VT_HR_EXIT( m_cMetaData.Merge(src.GetMetaData()) );
#endif

Exit:
    return hr;
}

CImgInfo CPyramid::GetImgInfo(int l, int iSubOctave) const
{
    int iIndex = l*m_props.iSubOctavesPerLevel + iSubOctave;

    if( iIndex == 0 )
    {
        return m_base.GetImgInfo();
    }

    iIndex--; // adjust for base 
    if( iIndex     >= (int)m_vecPyramid.size() || 
        iSubOctave >= m_props.iSubOctavesPerLevel ) 
    {
        return CImgInfo();
    }
    return m_vecPyramid[iIndex].GetImgInfo();
}

void CPyramid::Deallocate()
{
    Invalidate();
    m_base.Deallocate();
    m_vecPyramid.clear();
#if defined(_MSC_VER)
    m_cMetaData.DeleteAll();
#endif
}

CImg& CPyramid::GetLevel(int iLevel, int iSubOctave)
{
    int iIndex = iLevel*m_props.iSubOctavesPerLevel + iSubOctave;

    if( iIndex == 0 )
    {
        return m_base;
    }

    if( iSubOctave >= m_props.iSubOctavesPerLevel ||
        iIndex-1   >= (int)m_vecPyramid.size() )
    {
        VT_ASSERT( 0 );
        return m_base;
    }

    if( m_props.eAutoFilter == ePyramidFilterNone || 
        iLevel <= m_iLastLevelAutoGen )
    {
        return m_vecPyramid[iIndex-1];
    }

    // indicate all levels just generated - this needs to be set before
    // VtConstructPyramid because VtConstructPyramid re-enters GetLevel
    int iPrevLast       = m_iLastLevelAutoGen;
    m_iLastLevelAutoGen = iLevel;

    // ignore HRESULT since GetLevel function doesn't return an error
    // if you need the error code then don't use an autogen pyramid
    VtConstructPyramid(*this, m_props.eAutoFilter, iLevel, iPrevLast+1,
                       m_props.bFloatPixelUseSupressRingingFilter);

    return m_vecPyramid[iIndex-1];
}

HRESULT CPyramid::CopyTo(OUT CPyramid& dst) const
{
    VT_HR_BEGIN()

    dst.m_props = m_props;
    dst.m_iLastLevelAutoGen = m_iLastLevelAutoGen;

    VT_HR_EXIT( m_base.CopyTo(dst.m_base) );

    UINT uTotSubOct = (UINT) m_vecPyramid.size();
    VT_HR_EXIT( dst.m_vecPyramid.resize(uTotSubOct) );

    for( UINT i = 0; i < uTotSubOct; i++ )
    {
        const CImg& src = m_vecPyramid[i];
        VT_HR_EXIT( src.CopyTo(dst.m_vecPyramid[i]) );
    }
#if defined(_MSC_VER)
    dst.m_cMetaData.DeleteAll();
    VT_HR_EXIT( dst.m_cMetaData.Merge(&m_cMetaData) );
#endif

    VT_HR_END()
}

HRESULT 
CPyramid::SetProperties(const PYRAMID_PROPERTIES* pProps)
{
    VT_HR_BEGIN()

    m_props = pProps? *pProps: PYRAMID_PROPERTIES();

    // validate parameters
    if( m_props.iSubOctavesPerLevel <= 0 || 
        m_props.iSubOctaveStepSize  <= 0 )
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    // validate autofilter parameters
    switch( m_props.eAutoFilter )
    {
    case ePyramidFilter121Primal:
    case ePyramidFilter14641Primal:
        if( m_props.iSubOctavesPerLevel != 1 )
        {
            // these kernels don't make sense for sub-octave pyramids
            VT_HR_EXIT(E_INVALIDARG);
        }
        break;
    case ePyramidFilterBilinearDual:
        if ( m_props.iSubOctavesPerLevel != 1 &&
             m_props.bSubOctavesAllSameResolution )
        {
            // the bilinear filter only does down-sizing
            VT_HR_EXIT(E_INVALIDARG);
        }
        break;
    case ePyramidFilterGaussianPrimal:
    case ePyramidFilterGaussianDual:
        break;
    case ePyramidFilterLanczos3Primal:
    case ePyramidFilterLanczos3Dual:
        break;
    case ePyramidFilterNone:
        break;
    default:
        VT_HR_EXIT( E_INVALIDARG );
        break;
    } 
    
    VT_HR_END()
}

HRESULT
CPyramid::CreateLevels()
{
    VT_HR_BEGIN()

    int iType      = GetType();
    int iNumLevels = GetNumLevels();
    int iRnd       = m_props.bTruncateOddLevels? 0: 1;

    for( int iLevel = 0; iLevel < iNumLevels; iLevel++ )
    {
        int w, h, l;
        for( w = Width(), h = Height(), l = iLevel; l > 0; l-- )
        {
            w = (w==1)? 1: ((w+iRnd) >> 1);
            h = (h==1)? 1: ((h+iRnd) >> 1);
        }

        int iIndex = iLevel * m_props.iSubOctavesPerLevel;
        iIndex--; // adjust for base 
        if( iLevel != 0 )
        {
            // except for the base level create the full-octave stepping, the
            // base level has already been created	
            VT_HR_EXIT( m_vecPyramid[iIndex].Create(w, h, iType) );
        }

        if( iLevel != iNumLevels - 1 )
        {
            // for all levels except the last (1x1) level create the sub-octaves
            // that aren't on the full-octave stepping
            for( int j = 1; j < m_props.iSubOctavesPerLevel; j++ )
            {
                iIndex++;
                int wl = w, hl = h;
                if( !m_props.bSubOctavesAllSameResolution )
                {
                    float fDownFactor = 
                        powf(0.5, float(j)/float(m_props.iSubOctaveStepSize));
                    // note: round the nearest here instead of using 
                    //       bTruncateOddLevels which I think only makes sense
                    //       for power 2 pyramids
                    wl = F2I(fDownFactor * float(wl));
                    hl = F2I(fDownFactor * float(hl));

                }
                VT_HR_EXIT( m_vecPyramid[iIndex].Create(wl, hl, iType) );
            }
        }
    }

    VT_HR_END()
}

//+-----------------------------------------------------------------------------
//
//  Function:  VtSuppressPyramidRinging
//
//------------------------------------------------------------------------------
void
vt::VtSuppressPyramidRinging(CImg& imgDst, const vt::CPoint& ptDst, 
                             const CImg& imgSrc, const vt::CPoint& ptSrc)
{
    if( ( EL_FORMAT(imgSrc.GetType()) != EL_FORMAT_FLOAT &&
          EL_FORMAT(imgSrc.GetType()) != EL_FORMAT_HALF_FLOAT ) ||
        ( EL_FORMAT(imgDst.GetType()) != EL_FORMAT_FLOAT &&
          EL_FORMAT(imgDst.GetType()) != EL_FORMAT_HALF_FLOAT ) ||
        ( imgSrc.Bands() != imgDst.Bands() ) )
    {
        VT_ASSERT(0);
        return;
    }
    // 4 elements for filter support is baked into arfSrcBufA so assert for
    // now that no more than 4 bands are supplied
    VT_ASSERT( imgDst.Bands() <= 4 );

    int nb    = imgDst.Bands();
    ANALYZE_ASSUME(nb <= 4);
    int testb = VtMin(nb, 3);
    ANALYZE_ASSUME(testb <= 3);

    // make sure calling code set up coordinates correctly 
    VT_ASSERT( (2*ptDst.x - ptSrc.x) >= 0 );
    VT_ASSERT( (2*ptDst.y - ptSrc.y) >= 0 );

    // temp buffers used here
    float arfDestBuf[128]{};
    float arfBilinBuf[128]{};
    float arfSrcBufA[128 * 2 + 4]{};
    float arfSrcBufB[128 * 2 + 4]{};

    // compute # of pixels to iterate through 
    UINT uBufElementCnt = sizeof(arfDestBuf)/sizeof(arfDestBuf[0]);
    UINT uBufPixelsCnt  = uBufElementCnt / nb;   // make sure an integer # of pixels fit
    UINT uSrcByteCnt = 2 * uBufPixelsCnt * nb * (UINT)imgSrc.ElSize();
    UINT uDstByteCnt = uBufPixelsCnt * nb * (UINT)imgDst.ElSize(); 

    // determine types
    int iSrcType = imgSrc.GetType();
    int iDstType = imgDst.GetType();

    // run the suppression filter
    for( int y = 0; y < imgDst.Height(); y++ )
    {
        Byte* pDB = imgDst.BytePtr(y);
        int srcy  = 2*(ptDst.y+y)-ptSrc.y;
        int srcx  = 2*ptDst.x-ptSrc.x;
        int srcny = (srcy >= imgSrc.Height()-1)? srcy: srcy+1; 
        const Byte* pSB  = imgSrc.BytePtr(srcx, srcy);
        const Byte* pNSB = imgSrc.BytePtr(srcx, srcny);

        for( int x = 0; x < imgDst.Width(); x+=uBufPixelsCnt, 
             pDB+=uDstByteCnt, pSB+=uSrcByteCnt, pNSB+=uSrcByteCnt )
        {
            UINT uCurDstCnt = VtMin(UINT(imgDst.Width()-x), uBufPixelsCnt);
            ANALYZE_ASSUME(uCurDstCnt<=uBufPixelsCnt);
            VtConvertSpan((Byte*)arfDestBuf, VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, nb), 
                          pDB, iDstType, uCurDstCnt*nb);

            // convert source and dest to float
            // note +1 in the srccnt to handle edge cases
            UINT uCurSrcCnt = uCurDstCnt*2+1;
            UINT uPadCnt = 0;
            if( srcx + 2*x + (int)uCurSrcCnt > imgSrc.Width() )
            {
                uPadCnt     = (srcx + 2*x + uCurSrcCnt) - imgSrc.Width();
                uCurSrcCnt -= uPadCnt;
            }
            VtConvertSpan((Byte*)arfSrcBufA, VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, nb), 
                          pSB, iSrcType, uCurSrcCnt*nb);
            VtConvertSpan((Byte*)arfSrcBufB, VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, nb), 
                          pNSB, iSrcType, uCurSrcCnt*nb);

            // extend beyond the end
            for( UINT c = 0; c < uPadCnt; c++ )
            {
                for( int b = 0; b < testb; b++ )
                {
                    float fPadA = arfSrcBufA[(uCurSrcCnt-1)*nb+b];
                    float fPadB = arfSrcBufB[(uCurSrcCnt-1)*nb+b];
                    VT_ASSERT( ((uCurSrcCnt+c)*nb+b) < sizeof(arfSrcBufA)/sizeof(arfSrcBufA[0]) );
                    VT_ASSERT( ((uCurSrcCnt+c)*nb+b) < sizeof(arfSrcBufB)/sizeof(arfSrcBufB[0]) );
                    arfSrcBufA[(uCurSrcCnt+c)*nb+b] = fPadA;   
                    arfSrcBufB[(uCurSrcCnt+c)*nb+b] = fPadB;	
                }
            }

            // generate a bilinear version of the source
            const float *pS  = arfSrcBufA;
            const float *pNS = arfSrcBufB;
            for( UINT c = 0; c < uCurDstCnt*nb; c+=nb )
            {      
                int sc = 2*c;
                for( int b = 0; b < testb; b++, sc++ )
                {
                    VT_ASSERT( (sc+nb) < sizeof(arfSrcBufA)/sizeof(arfSrcBufA[0]) );
                    VT_ASSERT( (pS[sc] >= 0 && pS[sc+nb] >= 0) ||
                        (PIX_FORMAT(imgSrc.GetType()) != PIX_FORMAT_RGB &&
                        (PIX_FORMAT(imgSrc.GetType()) != PIX_FORMAT_RGBA)));
                    VT_ASSERT( (sc+nb) < sizeof(arfSrcBufB)/sizeof(arfSrcBufB[0]) );
                    VT_ASSERT( (pNS[sc] >= 0 && pNS[sc+nb] >= 0) ||
                        (PIX_FORMAT(imgSrc.GetType()) != PIX_FORMAT_RGB &&
                        (PIX_FORMAT(imgSrc.GetType()) != PIX_FORMAT_RGBA)));
                    
                    VT_ASSERT( (c+b) < sizeof(arfBilinBuf)/sizeof(arfBilinBuf[0]) );
                    arfBilinBuf[c+b] = (pS[sc] + pS[sc+nb] + 
                                        pNS[sc] + pNS[sc+nb]) * 0.25f;
                }
            }
   
            // suppress ringing
            const float* pB = arfBilinBuf;
            VT_ASSERT( (uCurDstCnt*nb) <= sizeof(arfBilinBuf)/sizeof(arfBilinBuf[0]) );
            float* pD = arfDestBuf;
            VT_ASSERT( (uCurDstCnt*nb) <= sizeof(arfDestBuf)/sizeof(arfDestBuf[0]) );
            for( UINT c = 0; c < uCurDstCnt; c++, pB+=nb, pD+=nb )
            {
                bool bSuppress = false;
                for( int b = 0; b < testb; b++ )
                {
                    if( pD[b] < 0.667f * pB[b] || pD[b] > 1.5f * pB[b] )
                    {
                        bSuppress = true;
                    }
                }

                if( bSuppress )
                {
                    for( int b = 0; b < testb; b++ )
                    {
                        pD[b] = pB[b];
                    }
                }
            }

            // convert dest back  
            VtConvertSpan(pDB, iDstType, (Byte*)arfDestBuf,
                          VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, nb), uCurDstCnt*nb);
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:  ConstructBilinearPyramid
//
//------------------------------------------------------------------------------
HRESULT 
ConstructBilinearPyramid(CPyramid& dst, int iLastLevel, int iStartLevel)
{
    VT_HR_BEGIN()


    if( iLastLevel < 0 )
    {
        iLastLevel = dst.GetNumLevels() - 1;
    }

    if( iStartLevel > iLastLevel || iLastLevel >= dst.GetNumLevels() ||
        dst.GetProperties().bSubOctavesAllSameResolution )
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    float fScale = powf(2.f, 1.f/float(dst.GetSubOctaveStepSize()));

    int iSubCnt  = dst.GetSubOctavesPerLevel();
    int iSubStep = dst.GetSubOctaveStepSize();

    // the dual filters need to adjust the sampling point
    float fPhase = 1.f;
    for( int i = 1; i < iSubStep; i++ )
    {
        fPhase += powf(fScale, float(i));
    }
    fPhase = 0.5f / fPhase; 

    for( int i = iStartLevel; i <= iLastLevel; i++ )
    {  
        if( i != 0 )
        { 
            CImg& dstLev = dst.GetLevel(i);
            if( iSubCnt <= iSubStep )
            {
                const CImg& srcLev = dst.GetLevel(i-1);
                VT_HR_EXIT( VtResizeImage(dstLev, dstLev.Rect(), srcLev, 
                                          2.f, 0.5, 2.f, 0.5, eSamplerKernelBilinear) );

            }
            else
            {
                // can just copy previous
                const CImg& srcLev = dst.GetLevel(i-1, iSubStep-1);
                srcLev.CopyTo(dstLev);
            }
        }

        if( i != dst.GetNumLevels()-1 )
        {
            for( int j = 1; j < iSubCnt; j++ )
            {   
                CImg& dstLev = dst.GetLevel(i, j);
                const CImg& srcLev = dst.GetLevel(i, j-1);
                
                VT_HR_EXIT( VtResizeImage(dstLev, dstLev.Rect(), srcLev, 
                                          fScale, fPhase, fScale, fPhase, 
                                          eSamplerKernelBilinear) );
            }
        }
    }

    VT_HR_END()
}

//+-----------------------------------------------------------------------------
//
//  Function: VtConstructFullOctavePyramid
//
//------------------------------------------------------------------------------
HRESULT
vt::VtConstructFullOctavePyramid(CPyramid& dst, const C1dKernel& k, 
                                 int iLastLevel, 
                                 bool bFloatPixelUseSupressRingingFilter)
{
    VT_HR_BEGIN()

    if( iLastLevel < 0 )
    {
        iLastLevel = dst.GetNumLevels() - 1;
    }

    if( iLastLevel >= dst.GetNumLevels() || dst.GetSubOctavesPerLevel() != 1 )
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    C1dKernelSet ks;
    VT_HR_EXIT( ks.Create(1, 2) );
    VT_HR_EXIT( ks.Set(0, -k.Center(), k) );

    for( int i = 1; i <= iLastLevel; i++ )
    {
        CImg& levdst = dst.GetLevel(i);
        const CImg& levsrc = dst.GetLevel(i-1);

        VT_HR_EXIT( VtSeparableFilter(levdst, levdst.Rect(), levsrc, 
                                      vt::CPoint(0,0), ks) );

        int iElType = EL_FORMAT(dst.GetType());
        if (bFloatPixelUseSupressRingingFilter &&
            (iElType == EL_FORMAT_HALF_FLOAT || iElType == EL_FORMAT_FLOAT))
        {
            // TODO: for now only supported for full octave pyramids
            VT_ASSERT( dst.GetSubOctavesPerLevel() == 1 );

            // suppress large ringing in float format
            VtSuppressPyramidRinging(levdst, vt::CPoint(0,0), levsrc, 
                                     vt::CPoint(0,0));
        }
    }

    VT_HR_END()
}

HRESULT 
vt::VtConstructFullOctavePyramid(CPyramid& dst, const CImg& imgSrc,
                                 const C1dKernel& k, int iLastLevel,
                                 bool bFloatPixelUseSupressRingingFilter)
{
    VT_HR_BEGIN()

    PYRAMID_PROPERTIES props;
    props.eAutoFilter = ePyramidFilterNone;

    VT_HR_EXIT( dst.Create(imgSrc, &props) );

    VT_HR_EXIT( VtConstructFullOctavePyramid(dst, k, iLastLevel,
                                             bFloatPixelUseSupressRingingFilter) );

    VT_HR_END()
}

//+-----------------------------------------------------------------------------
//
//  Function:  VtConstructPyramid
//
//------------------------------------------------------------------------------
HRESULT
vt::VtConstructPyramid(CPyramid& dst, ePyramidFilter eFilter, 
                       int iLastLevel, int iStartLevel,
                       bool bFloatPixelUseSupressRingingFilter)
{
    VT_HR_BEGIN()

    if( iLastLevel < 0 )
    {
        iLastLevel = dst.GetNumLevels() - 1;
    }

    for( int i = iStartLevel; i <= iLastLevel; i++ )
    {
        switch( eFilter )
        {
        case ePyramidFilter121Primal:
            hr = VtConstruct121Pyramid(dst, i, i);
            break;
    
        case ePyramidFilter14641Primal:
            hr = VtConstruct14641Pyramid(dst, i, i);
            break;
    
        case ePyramidFilterBilinearDual:
            hr = ConstructBilinearPyramid(dst, i, i);
            break;
    
        case ePyramidFilterGaussianPrimal:
            hr = VtConstructGaussianPyramid(dst, i, i, eResizeSamplingSchemePrimal);
            break;
    
        case ePyramidFilterGaussianDual:
            hr = VtConstructGaussianPyramid(dst, i, i, eResizeSamplingSchemeDual);
            break;

        case ePyramidFilterLanczos3Primal:
            hr = VtConstructLanczosPyramid(dst, i, i, eResizeSamplingSchemePrimal);
            break;

        case ePyramidFilterLanczos3Dual:
            hr = VtConstructLanczosPyramid(dst, i, i, eResizeSamplingSchemeDual);
            break;
    
        default:
            VT_ASSERT(0);  // prior validation should avoud this
            break;
        }
        VT_HR_EXIT( hr );
  
        int iElType = EL_FORMAT(dst.GetType());
        if ( i != 0 && bFloatPixelUseSupressRingingFilter && 
            (iElType == EL_FORMAT_HALF_FLOAT || iElType == EL_FORMAT_FLOAT))
        {
            // TODO: for now only supported for full octave pyramids
            VT_ASSERT( dst.GetSubOctavesPerLevel() == 1 );
    
            // suppress large ringing in float format
            VtSuppressPyramidRinging(dst.GetLevel(i),   vt::CPoint(0,0),
                                     dst.GetLevel(i-1), vt::CPoint(0,0));
        }
    }

    VT_HR_END()
}

//+-----------------------------------------------------------------------------
//
//  Function:  VtConstruct121Pyramid
//
//------------------------------------------------------------------------------
HRESULT
vt::VtConstruct121Pyramid(CPyramid& dst, int iLastLevel, int iStartLevel)
{
    VT_HR_BEGIN()

    if( iLastLevel < 0 )
    {
        iLastLevel = dst.GetNumLevels() - 1;
    }

    if( iStartLevel > iLastLevel || iLastLevel >= dst.GetNumLevels() ||
        dst.GetSubOctavesPerLevel() != 1 )
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    for( int i = VtMax(1, iStartLevel); i <= iLastLevel; i++ )
    {
        VT_HR_EXIT( VtSeparableFilter121Decimate2to1(
            dst.GetLevel(i), dst.GetImgInfo(i).Rect(), dst.GetLevel(i-1),
            vt::CPoint(0,0)) );
    }

    VT_HR_END()
}

//+-----------------------------------------------------------------------------
//
//  Function:  VtConstruct14641Pyramid
//
//------------------------------------------------------------------------------
HRESULT
vt::VtConstruct14641Pyramid(CPyramid& dst, int iLastLevel, int iStartLevel)
{
    VT_HR_BEGIN()

    if( iLastLevel < 0 )
    {
        iLastLevel = dst.GetNumLevels() - 1;
    }

    if( iStartLevel > iLastLevel || iLastLevel >= dst.GetNumLevels() ||
        dst.GetSubOctavesPerLevel() != 1 )
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    const C14641Kernel k;
    C1dKernelSet ks;
    VT_HR_EXIT( ks.Create(1, 2) );
    VT_HR_EXIT( ks.Set(0, -k.AsKernel().Center(), k.AsKernel()) );

    for( int i = VtMax(1, iStartLevel); i <= iLastLevel; i++ )
    {
        VT_HR_EXIT( VtSeparableFilter(dst.GetLevel(i), dst.GetImgInfo(i).Rect(), 
                                      dst.GetLevel(i-1), vt::CPoint(0,0), ks) );
    }

    VT_HR_END()
}

//+-----------------------------------------------------------------------------
//
//  Function:  VtConstructGaussianPyramid
//
//------------------------------------------------------------------------------
static void
DownSample2to1(CImg &cDest, const CImg& cSrc)
{
    const Byte *puchSrcRow = cSrc.BytePtr();
    Byte *puchDstRow = cDest.BytePtr();
    int iStepX = 2 * cSrc.PixSize();
    int iStepY = 2 * cSrc.StrideBytes();
    int iDstStride = cDest.StrideBytes();

    int iDstHeight = cDest.Height();
    int iDstWidth  = cDest.Width();

    for(int iY = 0; iY<iDstHeight; iY++, puchSrcRow += iStepY, 
        puchDstRow += iDstStride)
    {
        const Byte *puchSrc = puchSrcRow;
        Byte *puchDst       = puchDstRow;
        if(cSrc.PixSize()==sizeof(Byte))
        {	
            for(int iX = 0; iX<iDstWidth; iX++, puchSrc += iStepX, puchDst++)
                *puchDst = *puchSrc;
        }
        else if(cSrc.PixSize()==sizeof(UInt32))
        {
            for(int iX = 0; iX<iDstWidth; iX++, puchSrc += iStepX, 
                 puchDst+=sizeof(UInt32))
                *(UInt32 *)puchDst = *(UInt32 *)puchSrc;
        }
        else
        {
            for(int iX = 0; iX<iDstWidth; iX++, puchSrc += iStepX, 
                puchDst += cSrc.PixSize())
                memcpy(puchDst, puchSrc, cSrc.PixSize());
        }
    }
}

HRESULT
vt::VtConstructGaussianPyramid(CPyramid& dst, int iLastLevel, int iStartLevel,
                               eResizeSamplingScheme eSS)
{
    VT_HR_BEGIN()

    if( iLastLevel < 0 )
    {
        iLastLevel = dst.GetNumLevels() - 1;
    }

    if( iStartLevel > iLastLevel || iLastLevel >= dst.GetNumLevels() )
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    float fScale     = powf(2.f, 1.f/float(dst.GetSubOctaveStepSize()));
    float fSigmaSub  = sqrtf(fScale*fScale-1);
    float fSigmaFull = sqrtf(2.f*2.f-1);

    int iSubCnt  = dst.GetSubOctavesPerLevel();
    int iSubStep = dst.GetSubOctaveStepSize();
    bool bSubSameRes = dst.GetProperties().bSubOctavesAllSameResolution;

    // the dual filters need to adjust the sampling point
    float fPhase = 0.f;
    if( eSS == eResizeSamplingSchemeDual )
    {
        fPhase = 1.f;
        for( int i = 1; i < iSubStep; i++ )
        {
            fPhase += powf(fScale, float(i));
        }
        fPhase = 0.5f / fPhase;
    }

    for( int i = iStartLevel; i <= iLastLevel; i++ )
    {  
        if( i != 0 )
        { 
            CImg& dstLev = dst.GetLevel(i);
            if( (eSS == eResizeSamplingSchemeDual && bSubSameRes) || 
                iSubCnt <= iSubStep )
            {
                const CImg& srcLev = dst.GetLevel(i-1);

                C1dKernelSet ks;
                VT_HR_EXIT( Create1dGaussianKernelSet(
                    ks, 2, 1, fSigmaFull, 0, 3.f, 
                    (eSS == eResizeSamplingSchemeDual)? 0.5f: 0) );

                VT_HR_EXIT( VtSeparableFilter(dstLev, dstLev.Rect(), srcLev,
                                              vt::CPoint(0,0), ks) );
            }
            else
            {
                // can just downsample/copy previous
                const CImg& srcLev = dst.GetLevel(i-1, iSubStep);
                if( bSubSameRes )
                {
                    DownSample2to1(dstLev, srcLev);
                }
                else
                {
                    VT_ASSERT(srcLev.Width()  == dstLev.Width() &&
                              srcLev.Height() == dstLev.Height() );
                    srcLev.CopyTo(dstLev);
                }
            }
        }

        if( i != dst.GetNumLevels()-1 )
        {
            for( int j = 1; j < iSubCnt; j++ )
            {   
                CImg& dstLev = dst.GetLevel(i, j);
                const CImg& srcLev = dst.GetLevel(i, j-1);
    
                C1dKernelSet ks;
                if( bSubSameRes )
                {
                    // dual only matters when resizing, so 0 the phase
                    VT_HR_EXIT( Create1dGaussianKernelSet(
                        ks, 1, 1, fSigmaSub*powf(fScale, float(j-1)), 0, 3.f, 0) );
                }
                else
                {
                    VT_HR_EXIT( Create1dGaussianKernelSet(
                        ks, srcLev.Width(), dstLev.Width(),
                        fSigmaSub, 0, 3.f, fPhase) );
                }
                
                VT_HR_EXIT( VtSeparableFilter(dstLev, dstLev.Rect(), srcLev,
                                              vt::CPoint(0,0), ks) );
            }
        }
    }

    VT_HR_END()
}

//+-----------------------------------------------------------------------------
//
//  Function:  VtConstructDifferenceOfGaussianPyramid
//
//------------------------------------------------------------------------------
HRESULT
vt::VtConstructDifferenceOfGaussianPyramid(CPyramid& dst, int iLastLevel, int iStartLevel)
{
    VT_HR_BEGIN()

    if( iLastLevel < 0 )
    {
        iLastLevel = dst.GetNumLevels() - 1;
    }

    int dstLevels =  dst.GetNumLevels();
    if( iStartLevel > iLastLevel || iLastLevel >= dstLevels)
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    // Num of suboctaves should be the same as the step size and all sub octaves should be the same
    // resolution for DoG
    if(dst.GetSubOctavesPerLevel() != dst.GetSubOctaveStepSize() || !dst.GetProperties().bSubOctavesAllSameResolution)
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    int iSubCnt = dst.GetSubOctavesPerLevel();
    float fScale     = powf(2.f, 1.f/float(dst.GetSubOctaveStepSize()));
    float fSigmaSub  = sqrtf(fScale*fScale-1);

    VtConstructGaussianPyramid(dst, iLastLevel, iStartLevel, eResizeSamplingSchemePrimal);

    for(int i = iStartLevel; i < iLastLevel; ++i)
    {
        // Do differences until for the last level of DoG pyramid
        for(int j = 0; j < iSubCnt-1; ++j)
        {
            if(i == 0 && j == 0)
            {
                CFloatImg tmp;
                CImg& pyrLev1 = dst.GetLevel(0,0);
                pyrLev1.Share(tmp);
                pyrLev1.Create(tmp.GetImgInfo());
                CImg& pyrLev2 = dst.GetLevel(0,1);
                VtSubImages(pyrLev1, tmp, pyrLev2);
            }
            else
            {
                CImg& pyrLev1 = dst.GetLevel(i, j);
                CImg& pyrLev2 = dst.GetLevel(i, j+1);
                VtSubImages(pyrLev1, pyrLev1, pyrLev2);
            }
        }

        // For the last level we need to create an additional
        // buffer level so that we can take the difference

        // Get the last level
        CImg& pyrLastOctLev = dst.GetLevel(i, iSubCnt-1);
        // buffer image
        CImg buffLevImg;
        buffLevImg.Create(pyrLastOctLev.GetImgInfo());
        // blur
        C1dKernelSet ks;
        VT_HR_EXIT( Create1dGaussianKernelSet(
                        ks, 1, 1, fSigmaSub*powf(fScale, float(iSubCnt-1)), 0, 3.f, 0) );
        VT_HR_EXIT( VtSeparableFilter(buffLevImg, buffLevImg.Rect(), pyrLastOctLev,
                       vt::CPoint(0,0), ks) );
        // DoG
        VtSubImages(pyrLastOctLev, pyrLastOctLev, buffLevImg);
    }
    
    VT_HR_END()
}

//+-----------------------------------------------------------------------------
//
//  Function:  VtConstructLanczosPyramid
//
//  TODO: code above for Gaussian is almost identical - templatize this
//
//------------------------------------------------------------------------------
HRESULT
vt::VtConstructLanczosPyramid(CPyramid& dst, int iLastLevel, int iStartLevel,
                              eResizeSamplingScheme eSS)
{
    VT_HR_BEGIN()

    if( iLastLevel < 0 )
    {
        iLastLevel = dst.GetNumLevels() - 1;
    }

    if( iStartLevel > iLastLevel || iLastLevel >= dst.GetNumLevels() )
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    if( dst.GetProperties().bSubOctavesAllSameResolution )
    {
        VT_HR_EXIT( E_NOTIMPL );
    }

    int iSubCnt  = dst.GetSubOctavesPerLevel();
    int iSubStep = dst.GetSubOctaveStepSize();
    bool bSubSameRes = dst.GetProperties().bSubOctavesAllSameResolution;

    // the dual filters need to adjust the sampling point
    float fPhase = 0.f;
    if( eSS == eResizeSamplingSchemeDual )
    {
        float fScale = powf(2.f, 1.f/float(dst.GetSubOctaveStepSize()));
        fPhase = 1.f;
        for( int i = 1; i < iSubStep; i++ )
        {
            fPhase += powf(fScale, float(i));
        }
        fPhase = 0.5f / fPhase;
    }

    for( int i = iStartLevel; i <= iLastLevel; i++ )
    {  
        if( i != 0 )
        { 
            CImg& dstLev = dst.GetLevel(i);
            if( (eSS == eResizeSamplingSchemeDual && bSubSameRes) || 
                iSubCnt <= iSubStep )
            {
                const CImg& srcLev = dst.GetLevel(i-1);

                C1dKernelSet ks;
                VT_HR_EXIT( Create1dLanczosKernelSet(
                    ks, 2, 1, 3, (eSS == eResizeSamplingSchemeDual)? 0.5f: 0) );

                VT_HR_EXIT( VtSeparableFilter(dstLev, dstLev.Rect(), srcLev,
                                              vt::CPoint(0,0), ks) );
            }
            else
            {
                // can just downsample/copy previous
                const CImg& srcLev = dst.GetLevel(i-1, iSubStep);
                if( bSubSameRes )
                {
                    DownSample2to1(dstLev, srcLev);
                }
                else
                {
                    VT_ASSERT(srcLev.Width()  == dstLev.Width() &&
                              srcLev.Height() == dstLev.Height() );
                    srcLev.CopyTo(dstLev);
                }
            }
        }

        if( i != dst.GetNumLevels()-1 )
        {
            for( int j = 1; j < iSubCnt; j++ )
            {   
                CImg& dstLev = dst.GetLevel(i, j);
                const CImg& srcLev = dst.GetLevel(i, j-1);
    
                C1dKernelSet ks;
                if( bSubSameRes )
                {
                    VT_ASSERT(0);
                    // TODO: need to implement a 1:1 version of lanczos kernel
                }
                else
                {
                    VT_HR_EXIT( Create1dLanczosKernelSet(
                        ks, srcLev.Width(), dstLev.Width(), 3, fPhase) );
                }
                
                VT_HR_EXIT( VtSeparableFilter(dstLev, dstLev.Rect(), srcLev,
                                              vt::CPoint(0,0), ks) );
            }
        }
    }

    VT_HR_END()
}

