
//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for image resampling
//
//  History:
//      2010/10/05-mafinch
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_image.h"
#include "vt_extend.h"
#ifndef VT_NO_XFORMS
#include "vt_transform.h"
#endif // VT_NO_XFORMS

namespace vt {

/// \ingroup geotransforms
/// <summary> Preprocess image for Bicubic BSpline interpolation </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="ex">Image extend mode</param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
/// - Ideal use is to preprocess an image to be resampled once, either via this function
///   or the CPreprocessBicubicBSpline object below, then resample it multiple times using
///   warp or resize using the sampler type eSamplerFilterBicubicBSplineSrcPreprocessed.
///   This gives higher fidelity resampling at the same cost as Bicubic.
///   Alternately, at a tradeoff of performance for convenience, the eSamplerFilterBicubicBSpline
///   sampler will perform the preprocessing automatically.
///   - NOTE: Manually preprocessing using this class AND using eSamplerFilterBicubicBSpline
///   will produce incorrect results, as the image will be redundantly preprocessed.
HRESULT VtPreprocessBicubicBSpline(CFloatImg& imgDst, const CFloatImg& imgSrc, 
								   const IMAGE_EXTEND& ex = IMAGE_EXTEND(Wrap));

#ifndef VT_NO_XFORMS

/// \ingroup geotransforms
/// <summary>Implementation of \link IImageTransform image transform \endlink that preprocesses
/// an image to be resampled using the Bicubic B-Spline filter.
/// </summary> 
/// <DL><DT> Remarks: </DT></DL>
/// - Ideal use is to preprocess an image to be resampled once, either via this function
///   or the VtPreprocessBicubicBSpline function above, then resample it multiple times using
///   warp or resize using the sampler type eSamplerFilterBicubicBSplineSrcPreprocessed.
///   This gives higher fidelity resampling at the same cost as Bicubic.
///   Alternately, at a tradeoff of performance for convenience, the eSamplerFilterBicubicBSpline
///   sampler will perform the preprocessing automatically.
///   - NOTE: Manually preprocessing using this class AND using eSamplerFilterBicubicBSpline
///   will produce incorrect results, as the image will be redundantly preprocessed.

class CPreprocessBicubicBSpline : public CImageTransformUnaryGeo<CPreprocessBicubicBSpline, false>
{
public:
    CPreprocessBicubicBSpline()
    {
    };

    virtual ~CPreprocessBicubicBSpline()
    {
    };

public:
	/// <summary>Establish destination format for source format.</summary>
	/// <param name="frmtDst">The computed destination format.</param>
    /// <param name="pfrmtSrcs>Array of source formats. See remarks.</param>
	/// <DL><DT> Remarks:</DT></DL>
	/// - This is a unary transform, the length of pfrmtSrcs must be one.
    /// - Output is always floating point.
	virtual void    GetDstPixFormat(OUT int& frmtDst,
									IN  const int* pfrmtSrcs, 
									IN  UINT  /*uSrcCnt*/);

    /// <summary>Get (expanded) source rect necessary to compute dest rect.</summary>
    /// <param name="rctDst">Rect in destination image to be processed.</param>
    /// <DL><DT> Remarks:</DT></DL>
    /// - Will return the destination rect expanded out by a constant number of pixels in all directions.
	CRect GetRequiredSrcRect(IN  const CRect& rctDst);

    /// <summary>Get rect in destination image affected by pixels within rect in source.</summary>
    /// <param name="rctSrc">Rect within source image.</param>
    /// <DL><DT> Remarks:</DT></DL>
    /// - Affected rect will be source rect expanded out by a constant number of pixels in all directions.
	CRect GetAffectedDstRect(IN  const CRect& rctSrc);

    /// <summary>Dermine the size of a destination rect given a source rect.</summary>
    /// <param name="rctSrc">Rect within source image.</param>
    /// <DL><DT> Remarks:</DT></DL>
	CRect GetResultingDstRect(const vt::CRect& rctSrc)
	{
		return rctSrc;
	}

    /// <summary>Get (expanded) source rect necessary to compute dest rect.</summary>
    /// <param name="rctDst">Rect in destination image to be processed.</param>
    /// <DL><DT> Remarks:</DT></DL>
    /// - Will return the destination rect expanded out by a constant number of pixels in all directions.
    /// - This transform is unary, uSrcReqCount will always be one.
	virtual HRESULT GetRequiredSrcRect(OUT TRANSFORM_SOURCE_DESC* pSrcReq,
									  OUT UINT& uSrcReqCount,
									  IN  UINT  /*uSrcCnt*/,
									  IN  const CRect& rctDst
									  )
	{
		pSrcReq->bCanOverWrite = false;
		pSrcReq->rctSrc = GetRequiredSrcRect(rctDst);
		pSrcReq->uSrcIndex     = 0;
		uSrcReqCount           = 1;
		return S_OK;
	}


    /// <summary>Transform signature called by base class.</summary>
    /// <param name="pDstImg">Destination image.</param>
    /// <param name="srcRegion">Region from source image to be processed into destination image.</param>
    /// <param name="topLeft">Position of srcRegion UL-corner relative to rctDst.</param>
    /// <DL><DT> Remarks:</DT></DL>
    /// - pDstImg and srcRegions assumed to be copies of blocks from
    ///   within original source and destination images. Mappings relative
    ///   to each other and original images from rctDst and topLeft.
    HRESULT Transform(OUT CImg* pDstImg, 
							  IN  const CRect& rctDst,
                              IN const CImg& srcRegion,
                              IN const CPoint& topLeft);

    /// <summary>Create clone of self.</summary>
    /// <param name="ppClone">Pointer to be filled in with pointer to clone</param>
    /// <DL><DT> Remarks:</DT></DL>
    /// - This class is stateless, clone of self returns a new object.
    HRESULT Clone(ITaskState** ppClone)
    {
        *ppClone = VT_NOTHROWNEW CPreprocessBicubicBSpline();
        return (*ppClone)? S_OK: E_OUTOFMEMORY;
    }

    static int Expansion() { return CUBIC_BSPLINE_PROLOGUE; }

private:

    static const int CUBIC_BSPLINE_PROLOGUE = 11;

};

#endif // VT_NO_XFORMS


};