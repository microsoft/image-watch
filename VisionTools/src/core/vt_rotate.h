//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for rotating an image
//
//  History:
//      2010/10/06-ericsto
//			Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_convert.h"

#ifndef VT_NO_XFORMS
#include "vt_transform.h"
#endif

namespace vt {

/// \ingroup geofunc
/// <summary> Rotate image clockwise by multiple of 90 degrees </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="multipleOf90"> The input image will be rotated clockwise by 
/// multipleOf90 * 90 degrees.  Negative values are permissible.</param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc invalid
///		- E_INVALIDDST if imgSrc and imgDst share memory, or conversion between
///       the pixel types is not possible.
///		- E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
/// - For arbitrary rotation angles, use \ref VtWarpImage().
///	- Implements all \ref stdconvrules.

HRESULT VtRotateImage(CImg& imgDst, const CImg& imgSrc, int multipleOf90);

/// \ingroup geofunc
/// \anchor eFlipModeAnchor
enum eFlipMode
{
	eFlipModeNone       = 1, ///< No flip
	eFlipModeHorizontal = 2, ///< Flip image horizontally
	eFlipModeRotate180  = 3, ///< Rotate image 180 degrees
	eFlipModeVertical   = 4, ///< Flip image vertically
	eFlipModeTranspose  = 5, ///< Transpose rows and columns of the image
	eFlipModeRotate90   = 6, ///< Rotate image 90 degrees clockwise
	eFlipModeTransverse = 7, ///< Transverse transpose
	eFlipModeRotate270  = 8  ///< Rotate image 270 degrees clockwise
};

/// \ingroup geofunc
/// <summary> Flip image according to the specified \ref eFlipModeAnchor "mode".</summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="mode"> Describes the \ref eFlipModeAnchor "flip mode" to 
/// perform. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc invalid
///		- E_INVALIDDST if imgSrc and imgDst share memory, or conversion between
///       the pixel types is not possible.
///		- E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
/// - For arbitrary rotation angles, use \ref VtWarpImage().
///	- Implements all \ref stdconvrules.

HRESULT VtFlipImage(CImg& imgDst, const CImg& imgSrc, eFlipMode mode);

#ifndef VT_NO_XFORMS
/// \ingroup geotransforms
/// <summary>Implementation of \link IImageTransform image transform \endlink that performs
/// a vt::eFlipMode geometric transformations.
/// </summary> 
class CRotateTransform: public CImageTransformUnaryGeo<CRotateTransform, true>
{
public:
	virtual void GetDstPixFormat(OUT int& frmtDst,
								 IN  const int* pfrmtSrcs, 
								 IN  UINT  /*uSrcCnt*/)
	{
		frmtDst = VT_IMG_ISUNDEF(m_dstType)? *pfrmtSrcs: 
			IsValidConvertPair(*pfrmtSrcs, m_dstType)? m_dstType: OBJ_UNDEFINED;
    }

	virtual vt::CRect GetRequiredSrcRect(const vt::CRect& rctDst);
	
    virtual vt::CRect GetAffectedDstRect(const vt::CRect& rctSrc);

    virtual vt::CRect GetResultingDstRect(const vt::CRect& rctSrc);

    virtual HRESULT Transform(CImg* pimgDst, IN  const CRect& rctDst,
		      const CImg& imgSrc, const CPoint& ptSrc);

    virtual HRESULT Clone(ITaskState **ppState)
    {
		return CloneTaskState<CRotateTransform>(
			ppState, [this](CRotateTransform* pN) -> HRESULT
			{ pN->InitializeFlip(m_rect, m_eFlipMode, m_dstType);
			return S_OK; });
    }

public:
    CRotateTransform()
    {}

    /// <summary> Initialize transform for rotation </summary> 
    /// <param name="iSrcW">The source image width.</param> 
    /// <param name="iSrcH">The source image height.</param> 
    /// <param name="multipleOf90">The image will be rotated clockwise by 
    /// multipleOf90 * 90 degrees</param>
    /// <param name="dstType">The pixel format of the result. If necessary the
    /// transform will convert the supplied source to this format. The default
	/// value indicates that the type will the same as the source image.</param>
  	/// <DL><DT> Remarks: </DT></DL>
	///		- Rotates around (0,0) and translates the upper left corner of the
	///	resultin image back to (0,0)
    void InitializeRotate(int iSrcW, int iSrcH, int multipleOf90, 
						  int dstType=OBJ_UNDEFINED)
	{ InitializeRotate(vt::CRect(0,0,iSrcW,iSrcH), multipleOf90, dstType); }

    /// <summary> Initialize transform for rotation </summary> 
    /// <param name="rctSrc">The source image rectangle.</param> 
    /// <param name="multipleOf90">The input image will be rotated clockwise by 
    /// multipleOf90 * 90 degrees</param>
    /// <param name="dstType">The pixel format of the result. If necessary the
    /// transform will convert the supplied source to this format. The default
	/// value indicates that the type will the same as the source image.</param>
  	/// <DL><DT> Remarks: </DT></DL>
	///		- Rotates around the top left corner of rctSrc and then translates 
	/// the upper left corner of the resulting image back to rctSrc's top left
    void InitializeRotate(const RECT& rctSrc, int multipleOf90, 
						  int dstType=OBJ_UNDEFINED);

	/// <summary> Initialize transform for flip </summary> 
    /// <param name="iSrcW">The source image width.</param> 
    /// <param name="iSrcH">The source image height.</param> 
    /// <param name="mode">Flip mode, see vt::eFlipMode enum</param> 
    /// <param name="dstType">The pixel format of the result. If necessary the
    /// transform will convert the supplied source to this format. The default
	/// value indicates that the type will the same as the source image.</param>
   	/// <DL><DT> Remarks: </DT></DL>
	///		- Flips around (0,0) and translates the upper left corner of the
	///	resulting image back to (0,0)
    void InitializeFlip(int iSrcW, int iSrcH, eFlipMode mode, 
						int dstType=OBJ_UNDEFINED)
    { InitializeFlip(CRect(0, 0, iSrcW, iSrcH), mode, dstType); }

	/// <summary> Initialize transform for flip </summary> 
    /// <param name="rctSrc">The source image rectangle.</param> 
    /// <param name="mode">Flip mode</param>
    /// <param name="dstType">The pixel format of the result. If necessary the
    /// transform will convert the supplied source to this format. The default
	/// value indicates that the type will the same as the source image.</param>
  	/// <DL><DT> Remarks: </DT></DL>
	///		- Flips around the top left corner of rctSrc and then translates 
	/// the upper left corner of the resulting image back to rctSrc's top left
    void InitializeFlip(const RECT& rctSrc, eFlipMode mode, 
						int dstType=OBJ_UNDEFINED)
    {
		m_rect = rctSrc;
		m_eFlipMode = mode;
        m_dstType   = dstType;
    }

protected:
    int m_dstType;
    CRect m_rect;
	eFlipMode m_eFlipMode;
};
#endif

};
