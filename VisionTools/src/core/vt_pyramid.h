//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      General Pyramid class
//
//  History:
//      2004/11/08-mattu
//          Created
//
//------------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#if defined(_MSC_VER)
#include "vt_params.h"
#else
class CParams;
#endif
#include "vt_image.h"
#include "vt_atltypes.h"
#include "vt_separablefilter.h"
#include "vt_warp.h"
#if defined(_MSC_VER)
#include "vt_readerwriter.h"
#include "vt_transform.h"
#endif

namespace vt {

enum ePyramidFilter
{
    ePyramidFilterNone           = 0,

    ePyramidFilter121Primal      = 2,
    // no 121 dual 

    ePyramidFilter14641Primal    = 4,
    // no 14641 dual

    // no bilinear primal
    ePyramidFilterBilinearDual   = 7,

    ePyramidFilterGaussianPrimal = 8,
    ePyramidFilterGaussianDual   = 9,

    ePyramidFilterLanczos3Primal = 10,
    ePyramidFilterLanczos3Dual   = 11
};

struct PYRAMID_PROPERTIES
{
    // control auto generate of pyramids
    ePyramidFilter eAutoFilter;
    bool  bFloatPixelUseSupressRingingFilter;
   
    // control level dimensions 
    bool  bTruncateOddLevels;   // this flag controls the dimensions of the next 
                                // lower level when the current level has odd 
                                // dimensions. For example if level N has a 
                                // width of 13 then level N+1 will have a width
                                // of 6 if this flag is true and 7 if false.
                                // TODO: update this descrip for sub-octaves

    // control sub-octaves
    bool  bSubOctavesAllSameResolution;
    int   iSubOctaveStepSize;
    int   iSubOctavesPerLevel; 
    
    PYRAMID_PROPERTIES() :   
        eAutoFilter(ePyramidFilter14641Primal),

		// TODO: update hdview/hdedit to set this to true
		bFloatPixelUseSupressRingingFilter(false),

        bTruncateOddLevels(true),

        bSubOctavesAllSameResolution(false),
        iSubOctaveStepSize(1),
        iSubOctavesPerLevel(1)
    { }
};

//+-----------------------------------------------------------------------------
//
//  Class:  CPyramid
//
//  Synopsis:  A full-octave pyramid (multi-scale representation of an image
//             containing original resolution, .5x.5, 0.25x0.25, ...)
//
//------------------------------------------------------------------------------
class CPyramid
{
public:
    CPyramid() : m_iLastLevelAutoGen(-1)
    {}

    virtual ~CPyramid()
    {}

private:
    // Copying is verboten
    CPyramid(const CPyramid&);

    CPyramid &operator=(const CPyramid&);

public:
    // various methods for creating a pyramid
    HRESULT Create(int iW, int iH, int iType,
                   const PYRAMID_PROPERTIES* pProps = NULL);

    HRESULT Create(const CImgInfo& info,
                   const PYRAMID_PROPERTIES* pProps = NULL)
    { return Create(info.width, info.height, info.type, pProps); }

    HRESULT Create(Byte *pbBuffer, int iW, int iH, int iStrideBytes,
                   int iType, const PYRAMID_PROPERTIES* pProps = NULL);

    HRESULT Create(const CImg& src, const PYRAMID_PROPERTIES* pProps = NULL);

    int Width()  const
    { return m_base.Width(); }

    int Height()  const
    { return m_base.Height(); }

    int Bands()   const
    { return m_base.Bands(); }

    int GetType() const
    { return m_base.GetType(); }

    bool IsValid() const   
    { return m_base.IsValid(); }

    CImgInfo GetImgInfo(int l = 0, int iSubOctave = 0) const;

    const PYRAMID_PROPERTIES& GetProperties() const
    { return m_props; }

    const CParams* GetMetaData() const 
    {
        return
#if defined(_MSC_VER)
            &m_cMetaData
#else
            NULL
#endif
            ;
    }

    void    Deallocate();

    void    Invalidate()
    { m_iLastLevelAutoGen = -1; }

    HRESULT CopyTo(OUT CPyramid& dst) const;

    CImg&   GetLevel(int iLevel, int iSubOctave = 0);

    int GetNumLevels() const 
    { 
        return IsValid()? 
            1 + (int)(m_vecPyramid.size()) / m_props.iSubOctavesPerLevel: 0;
    }

    int GetSubOctaveStepSize() const 
    { return m_props.iSubOctaveStepSize; }

    int GetSubOctavesPerLevel() const 
    { return m_props.iSubOctavesPerLevel; }

protected:
    HRESULT CreateLevels();
    HRESULT SetProperties(const PYRAMID_PROPERTIES* pProps);

protected:
    CImg               m_base;
    PYRAMID_PROPERTIES m_props;
    int                m_iLastLevelAutoGen; 
    vt::vector<CImg>   m_vecPyramid;
#if defined(_MSC_VER)
    CParams            m_cMetaData;
#endif
};

//+-----------------------------------------------------------------------------
//
//  Class:  CTypedPyramid
//
//  Synopsis:  A pyramid of CTypedImg 
//
//------------------------------------------------------------------------------
template <typename TIMG>
class CTypedPyramid: public CPyramid
{
public:
    typedef typename TIMG::PixelType PixelType;
    
private:
    class CTypedPyramidImg : public TIMG
    {
    public:
        void InitType() { CImg::SetType(TIMG::TypeIs(1)); }
    };

public:
    CTypedPyramid()
    { 
        static_cast<CTypedPyramidImg&>(m_base).InitType();
    }

private:
    // Copying is verboten
    CTypedPyramid(const CTypedPyramid&);

    CTypedPyramid &operator=(const CTypedPyramid&);

public:
    HRESULT Create(int iW, int iH, int iBands = 1, 
                   const PYRAMID_PROPERTIES* pProps = NULL)
    { 
        return CPyramid::Create(iW, iH, TIMG::TypeIs(iBands), pProps); 
    }

    HRESULT Create(PixelType *pbBuffer, int iW, int iH, int iBands, int iStrideBytes,
                   const PYRAMID_PROPERTIES* pProps = NULL) 
    { 
        return CPyramid::Create((Byte*)pbBuffer, iW, iH, iStrideBytes, 
                                TIMG::TypeIs(iBands), pProps); 
    }

    HRESULT Create(const CImg& src, const PYRAMID_PROPERTIES* pProps = NULL)
    { return CPyramid::Create(src, pProps); }

    int GetType() const
    { return TIMG::TypeIs(); }

    TIMG& GetLevel(int iLevel, int iSubOctave = 0)
    { return (TIMG&)CPyramid::GetLevel(iLevel, iSubOctave); }

    TIMG& operator [] (int iLevel)
    { return GetLevel(iLevel, 0); }
};

typedef CTypedPyramid<CByteImg>       CBytePyramid;
typedef CTypedPyramid<CShortImg>      CShortPyramid;
typedef CTypedPyramid<CHalfFloatImg>  CHalfFloatPyramid;
typedef CTypedPyramid<CFloatImg>      CFloatPyramid;

//+-----------------------------------------------------------------------------
//
//  Class:    CCompositePyramid
//
//  Synopsis:  A pyramid of CCompositeImg 
//
//------------------------------------------------------------------------------
template <typename TCIMG>
class CCompositePyramid : public CTypedPyramid<typename TCIMG::BaseImageType>
{
public:
    typedef CTypedPyramid<typename TCIMG::BaseImageType> BasePyramidType;

    typedef typename TCIMG::CompositePixelType CompositePixelType;

private:
    class CCompositePyramidImg : public TCIMG
    {
    public:
        void InitType() { CImg::SetType(TCIMG::TypeIs()); }
    };

public:
    CCompositePyramid() 
    {
        static_cast<CCompositePyramidImg&>(this->m_base).InitType();
    }

private:
    // Copying is verboten
    CCompositePyramid(const CCompositePyramid&);

    CCompositePyramid &operator=(const CCompositePyramid&);

public:
    HRESULT Create(int iW, int iH, const PYRAMID_PROPERTIES* pProps = NULL)
    { 
        return CPyramid::Create(iW, iH, TCIMG::TypeIs(), pProps);
    }

    HRESULT Create(const CImg& src, const PYRAMID_PROPERTIES* pProps = NULL)
    { return CPyramid::Create(src, pProps); }

    int GetType() const
    { return  TCIMG::TypeIs(); }

    TCIMG& GetLevel(int iLevel, int iSubOctave = 0)
    { return (TCIMG&)CPyramid::GetLevel(iLevel, iSubOctave); }

    TCIMG& operator [] (int iLevel)
    { return GetLevel(iLevel, 0); }
};

typedef CCompositePyramid<CLumaImg>      CLumaPyramid;
typedef CCompositePyramid<CLumaShortImg> CLumaShortPyramid;
typedef CCompositePyramid<CLumaFloatImg> CLumaFloatPyramid;
typedef CCompositePyramid<CRGBImg>       CRGBPyramid;
typedef CCompositePyramid<CRGBShortImg>  CRGBShortPyramid;
typedef CCompositePyramid<CRGBFloatImg>  CRGBFloatPyramid;
typedef CCompositePyramid<CRGBAImg>      CRGBAPyramid;
typedef CCompositePyramid<CRGBAShortImg> CRGBAShortPyramid;
typedef CCompositePyramid<CRGBAFloatImg> CRGBAFloatPyramid;

//+-----------------------------------------------------------------------------
//
//  Function: VtFullOctaveConstructPyramid 
//
//------------------------------------------------------------------------------
HRESULT VtConstructFullOctavePyramid(CPyramid& dst, const CImg& imgSrc,
                                     const C1dKernel& k, int iLastLevel = -1,
                                     bool bFloatPixelUseSupressRingingFilter = false);

HRESULT VtConstructFullOctavePyramid(CPyramid& dst, const C1dKernel& k, 
                                     int iLastLevel = -1,
                                     bool bFloatPixelUseSupressRingingFilter = false);

//+-----------------------------------------------------------------------------
//
//  Function: VtConstructPyramid 
//
//------------------------------------------------------------------------------
HRESULT VtConstructPyramid(CPyramid& dst, ePyramidFilter eFilter, 
                           int iLastLevel = -1, int iStartLevel = 0,
                           bool bFloatPixelUseSupressRingingFilter = false);

//+-----------------------------------------------------------------------------
//
//  Function: VtConstruct121Pyramid 
//
//------------------------------------------------------------------------------
HRESULT VtConstruct121Pyramid(CPyramid& dst, int iLastLevel = -1, int iStartLevel = 0);

//+-----------------------------------------------------------------------------
//
//  Function: VtConstruct14641Pyramid 
//
//------------------------------------------------------------------------------
HRESULT VtConstruct14641Pyramid(CPyramid& dst, int iLastLevel = -1, int iStartLevel = 0);

//+-----------------------------------------------------------------------------
//
//  Function: VtConstructDifferenceOfGaussianPyramid 
//
//------------------------------------------------------------------------------
HRESULT VtConstructDifferenceOfGaussianPyramid(
    CPyramid& dst, int iLastLevel = -1, int iStartLevel = 0);

//+-----------------------------------------------------------------------------
//
//  Function: VtConstructGaussianPyramid 
//
//------------------------------------------------------------------------------
HRESULT VtConstructGaussianPyramid(
    CPyramid& dst, int iLastLevel = -1, int iStartLevel = 0,
    eResizeSamplingScheme eSS = eResizeSamplingSchemePrimal);

//+-----------------------------------------------------------------------------
//
//  Function: VtConstructLanczosPyramid
//
//------------------------------------------------------------------------------
HRESULT VtConstructLanczosPyramid(
    CPyramid& dst, int iLastLevel = -1, int iStartLevel=0,
    eResizeSamplingScheme eSS=eResizeSamplingSchemePrimal);

//+-----------------------------------------------------------------------------
//
//  Function:  VtSuppressPyramidRinging
//
//  Synopsis:  Runs a box filter to generate a reduced resolution dst.  If the
//             box filter version is substantially different from the current
//             dst then replace with the box filter version, this tends to 
//             reduce the ringing on very large step edges.
//
//------------------------------------------------------------------------------
void VtSuppressPyramidRinging(CImg& imgDst, const vt::CPoint& ptDst, 
                              const CImg& imgSrc, const vt::CPoint& ptSrc);

//+-----------------------------------------------------------------------------
//
//  Function:  ComputePyramidLevelCount
//
//  Synopsis:  returns the numbers of levels in a pyramid given the resolution
//             of the finest level
//
//------------------------------------------------------------------------------
inline UINT
ComputePyramidLevelCount(int basew, int baseh, int levw, int levh, 
                         bool bTruncate = false)
{
    if ( basew==0 || baseh==0 ) return 0;
    
    levw = VtMax(1, levw);
    levh = VtMax(1, levh);

    // can generate levels down to levw x levh
    UINT uLevelCount = 1;
    int iRnd = bTruncate? 0: 1;
    for( ; basew > levw || baseh > levh
         ; basew = (basew+iRnd) >> 1, baseh = (baseh+iRnd) >> 1 )
    {
        uLevelCount++;
    }
    return uLevelCount;
}

inline UINT
ComputePyramidLevelCount(int basew, int baseh, bool bTruncate = false)
{ return ComputePyramidLevelCount(basew, baseh, 1, 1, bTruncate); }

//+-----------------------------------------------------------------------------
//
//  Function:  MinLevelWithPixCount
//
//  Synopsis:  returns the minimum level that contains <= uTargetCount pixels
//
//------------------------------------------------------------------------------
inline UINT 
MinLevelWithPixCount(UINT uLevel0Count, ULONG uTargetCount)
{
    UINT uL = 0;
    while( (uLevel0Count >> (2*uL)) > uTargetCount  )
    {
        uL++;
    }
    return uL;
}
#ifndef VT_NO_XFORMS
//+-----------------------------------------------------------------------------
//
// function: PushPyramidTransformAndWait
// 
// Synopsis: sets up a pyramid generating transform and runs it via the
//           multi-core transform framework 
// 
//------------------------------------------------------------------------------
inline
HRESULT PushPyramidTransformAndWait(IImageReaderWriter* pPyramid, UINT uDstLevel, 
                                    const C1dKernel& kernel,
                                    CTaskStatus* pStatus = NULL, 
                                    VT_TRANSFORM_TASK_OPTIONS* pOpts = NULL)
{
    HRESULT hr;
    VT_ASSERT(uDstLevel > 0 );

    int iDstType = pPyramid->GetImgInfo().type;

    CSeparableFilterTransform filter;
    if( (hr = filter.InitializeForPyramidGen(iDstType, kernel)) == S_OK )
    {
        CLevelChangeWriter wrlev(pPyramid, uDstLevel);
        CLevelChangeReader rdlev(pPyramid, uDstLevel-1);
        CTransformGraphUnaryNode graph(&filter);
        const IMAGE_EXTEND iex(Extend);
        graph.BindSourceToReader(&rdlev, &iex);
		CRasterBlockMap bm(wrlev.GetImgInfo().LayerRect(), 128, 128);
		graph.SetDest(NODE_DEST_PARAMS(&wrlev, &bm));
        hr = PushTransformTaskAndWait(&graph, pStatus, pOpts);
    }
    return hr;
}

inline
HRESULT GeneratePyramid(IImageReaderWriter* pPyramid, 
                        UINT uFirstLevel, UINT uLastLevel,
                        const C1dKernel& kernel, 
                        CTaskStatus* pStatus = NULL, 
                        VT_TRANSFORM_TASK_OPTIONS* pOpts = NULL)
{
    HRESULT hr = S_OK;

    uFirstLevel = VtMax(UINT(1), uFirstLevel);

    CPhasedTaskStatus report;
    report.SetOuterCallback(pStatus);

    float fSum = 0.f;
    for( UINT l = uFirstLevel; l <= uLastLevel; l++ )
    {
        // every level is 25% of the previous compute
        fSum += powf(0.25f, float(l));
    }
               
    for( UINT l=uFirstLevel; l <= uLastLevel && hr == S_OK; l++)
    {
        report.BeginPhase("GeneratePyramid level",
                          100.f*powf(0.25f, float(l))/fSum);
        hr = PushPyramidTransformAndWait(pPyramid, l, kernel, &report, pOpts);
    }
    return hr;
}
#endif
};
