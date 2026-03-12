//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for image filtering
//
//  History:
//      2004/11/08-swinder
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_image.h"
#include "vt_extend.h"

#ifndef VT_NO_XFORMS
#include "vt_transform.h"
#include "vt_readerwriter.h"
#endif

namespace vt {

#ifdef MAKE_DOXYGEN_WORK
void foo();
#endif

class CLayerImgInfo;

//+-----------------------------------------------------------------------------
//
// function: GetNearestInBoundRect
// 
// synopsis: Given a rctOOB that potentially is outside of an image with
//           position defined by rctSrc, return the nearest rct withing rctSrc
//           that is necessary to accomodate padding operations.  rctBuf is the
//           corresponding rect within a buffer of size rctOOB. 
// 
//------------------------------------------------------------------------------
void GetNearestInBoundRect(vt::CRect& rctInB, vt::CRect& rctBuf,
						   const vt::CRect& rctOOB, const vt::CRect& rctSrc);

//+-----------------------------------------------------------------------------
//
// function: VtGeneratePadOps
// 
// synopsis: Given rctRequestLayer that will be read from a source image that
//           has dimensions defined in infoReader generate a list of operations
//           that fill a destination image.
// 
//------------------------------------------------------------------------------
enum ePadOp
{
	ePadOpCopy,
	ePadOpConstantHoriz,
	ePadOpConstantVert,
	ePadOpPad,
	ePadOpPadNonIntersect
};

struct PAD_OP
{
	vt::CRect  rctDst;
	vt::CRect  rctRead;
	vt::CPoint ptWrite;
	ePadOp     op;
};

void VtGeneratePadOps(OUT PAD_OP(& ops)[16], OUT UInt32& uOpsCnt,
					  CRect rctRequestLayer,
					  const CLayerImgInfo& infoReader,
					  const IMAGE_EXTEND& ex,
					  const vt::CPoint* pptDst = NULL);

/// \ingroup geofunc
/// <summary> Crop or pad image. </summary>
/// <param name="imgSrc"> Input image. </param>
/// <param name="rectSrc"> Region of interest in input image. </param>
/// <param name="imgDst"> Output image. </param>
/// <param name="exInfo"> Extend info. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDDST if imgSrc is invalid
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDARG or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
HRESULT VtCropPadImage(CImg &imgDst, const CRect &rectSrc, const CImg& imgSrc, 
	const IMAGE_EXTEND& exInfo = IMAGE_EXTEND());

#ifndef VT_NO_XFORMS

HRESULT VtCropPadImage(CImg &imgDst, const CRect &rectSrc, 
	const CLayerImgInfo& infoSrc, IImageReader* pReader, 
	const IMAGE_EXTEND& exInfo = IMAGE_EXTEND());

//+-----------------------------------------------------------------------------
//
// function: VtPrefetchPadded
// 
// synopsis: Given a rSrc region call prefetch on the reader taking into account
//           the padding mode
// 
//------------------------------------------------------------------------------
HRESULT
VtPrefetchPadded(const CRect &rSrc, const CLayerImgInfo& infoSrc, 
				 IN IImageReader* pReader, const IMAGE_EXTEND& ex);

#endif

//+-----------------------------------------------------------------------------
//
// function: VtExtendConstZeroPadImage
// 
// synopsis: Given a rect within the provided destination image,
//           pad according to the following:
// 
//           Zero     - Fills everything outside of rDstInBounds with Zero,
//                      in this case rDstInBounds need not intersect imgDst
//           Wrap     - This function doesn't handle Wrap use the above
//                      VtPadImage instead
//           Extend   - Extends the "in bounds" border pixels to the rest 
//                      of imgDst. Note this requires rDstInBounds to 
//                      intersect with imgDst in order to sample the border 
//                      pixels.
//           Constant - Same as Zero except fills with the specified pHorizVal,
//                      pVertVal.  
//           Mirror   - This function doesn't handle Mirror use the above
//                      VtPadImage instead
// 
//------------------------------------------------------------------------------
void 
VtExtendConstZeroPadImage(IN OUT CImg &imgDst, const CRect &rDstInBounds,
						  ExtendMode exHoriz, ExtendMode exVert, 
						  const void *pHorizVal = NULL, 
						  const void *pVertVal  = NULL);

#ifndef VT_NO_XFORMS

//+-----------------------------------------------------------------------------
//
// Class: CPadImageReader
// 
// Synopsis: an image reader that presents the contained image as a larger layer
//           and pads requests outside of the contained source
//
//------------------------------------------------------------------------------
class CPadImageReader: public IImageReader
{
	// IImageReader
public:
	virtual CLayerImgInfo GetImgInfo(UInt32 uLevel=0)
	{
		CLayerImgInfo infoL = m_pSrc->GetImgInfo(uLevel);
		vt::CRect     rctL  = LayerRectAtLevel(m_rctLayerWithPad, uLevel);
		return CLayerImgInfo(CImgInfo(rctL.Width(), rctL.Height(), infoL.type),
			infoL.compositeWidth, infoL.compositeHeight, rctL.TopLeft());
	}
	virtual HRESULT GetMetaData(OUT CParams &params)
	{ return m_pSrc->GetMetaData(params); }

	virtual HRESULT ReadRegion(const CRect &region, OUT CImg &dst, UInt32 uLevel=0)
	{ return DoRegion(region, &dst, uLevel); }

	virtual HRESULT Prefetch(const CRect &region, UInt32 uLevel=0)
	{ return DoRegion(region, NULL, uLevel); }

public:
	CPadImageReader(IImageReader* pSrc, const vt::CRect& rctLayerWithPad,
					ExtendMode exHoriz = Zero, ExtendMode exVert = Zero) :
		m_pSrc(pSrc), m_rctLayerWithPad(rctLayerWithPad), m_ex(exHoriz, exVert)  
	{}

protected:
	HRESULT DoRegion(const CRect &region, CImg* pImg, UInt32 uLevel)
	{
		if( region.IsRectEmpty() )
		{
			return S_OK;
		}

		CLevelChangeReader src(m_pSrc, uLevel);
		CLayerImgInfo infoSrc = src.GetImgInfo();
		vt::CRect layerrect   = 
			region + LayerRectAtLevel(m_rctLayerWithPad, uLevel).TopLeft();
  
		return pImg? VtCropPadImage(*pImg, layerrect, infoSrc, &src, m_ex):
			VtPrefetchPadded(layerrect, infoSrc, &src, m_ex);
	}

protected:
	IImageReader* m_pSrc;
	vt::CRect     m_rctLayerWithPad;
	IMAGE_EXTEND  m_ex;
};

/// \ingroup geotransforms
/// <summary> A transform for cropping and padding 
/// </summary> 
class CCropPadTransform: public IImageTransform
{
public:
	/// <summary> Constructor </summary>
	/// <param name="srcInfo"> Source image info. </param>
	/// <param name="dstRect"> Destination rectangle. </param>
	CCropPadTransform(const CLayerImgInfo& srcInfo, const CRect& dstRect)
		: m_srcInfo(srcInfo), m_dstRect(dstRect)
	{
	}

	/// <summary> Set extend mode </summary>
	/// <param name="ex"> \link vt::IMAGE_EXTEND mode \endlink for padding </param>
	/// <DL><DT> Remarks: </DT></DL>
	/// - Wrap mode is not supported at this point
	HRESULT InitExtendMode(const IMAGE_EXTEND& ex)
	{
		if (ex.exHoriz == Wrap || ex.exVert == Wrap)
			return E_INVALIDARG;

		return m_mode.Initialize(&ex);
	}

public:
	virtual bool RequiresCloneForConcurrency()
	{ return false;	}

	// by default we use the pixel format of the dest
	virtual void    GetSrcPixFormat(IN OUT int* piTypeSrcs, 
									IN UINT  /*uSrcCnt*/,
									IN int iTypeDst)
	{ piTypeSrcs[0] = iTypeDst; }

	virtual void    GetDstPixFormat(OUT int& iTypeDst,
									IN  const int* piTypeSrcs, 
									IN  UINT  /*uSrcCnt*/)
	{ iTypeDst = piTypeSrcs[0]; }

	virtual HRESULT GetRequiredSrcRect(OUT TRANSFORM_SOURCE_DESC* pSrcReq,
									  OUT UINT& uSrcReqCount,
									  IN  UINT,
									  IN  const CRect& rctDst
									  )
	{
		PAD_OP ops[16];
		UInt32 opsCount;
		VtGeneratePadOps(ops, opsCount, rctDst, m_srcInfo, m_mode);
		   
		pSrcReq[0].rctSrc = ops[0].rctRead + CRect(m_srcInfo.LayerRect()).TopLeft();
		pSrcReq[0].uSrcIndex     = 0;
		pSrcReq[0].bCanOverWrite = true;

		uSrcReqCount = (pSrcReq[0].rctSrc.IsRectEmpty())? 0: 1;
		return S_OK;
	}

	virtual HRESULT GetAffectedDstRect(OUT CRect& rctDst,
									  IN  const CRect& /*rctSrc*/,
									  IN  UINT /*uSrcIndex*/,
									  IN  UINT /*uSrcCnt*/)
	{
		rctDst = m_dstRect;
		return S_OK;
	}

	virtual HRESULT GetResultingDstRect(OUT CRect& rctDst,
	                                    IN  const CRect& /*rctSrc*/,
	                                    IN  UINT /*uSrcIndex*/,
										IN  UINT /*uSrcCnt*/)
	{		
		rctDst = m_dstRect;
		return S_OK;
	}

	virtual HRESULT Transform(OUT CImg* pimgDst, 
							  IN  const CRect& rctDst,
							  IN  CImg *const *ppimgSrcRegions,
							  IN  const TRANSFORM_SOURCE_DESC* pSrcDesc,
							  IN  UINT  uSrcCnt
							  )
	{ 
	    VT_ASSERT( m_mode.exHoriz != Wrap && m_mode.exVert != Wrap ); 

		CImg imgEmpty;
		const CImg& src = uSrcCnt? *ppimgSrcRegions[0]: imgEmpty; 		

		CRect dst = rctDst - pSrcDesc[0].rctSrc.TopLeft();

		return VtCropPadImage(*pimgDst, dst, src, m_mode);
	}		

	virtual HRESULT Clone(ITaskState **ppState)
	{
		CCropPadTransform* pC = VT_NOTHROWNEW CCropPadTransform(m_srcInfo, m_dstRect);
		HRESULT hr = CloneTaskState(ppState, pC);
		if( hr == S_OK )
		{
			ANALYZE_ASSUME( pC != NULL );
			if( (hr = pC->InitExtendMode(m_mode)) != S_OK )
			{
				delete pC;
				*ppState = NULL;   
			}
		}
		return hr;
	}

protected:
	CLayerImgInfo m_srcInfo;
	CRect         m_dstRect;
	IMAGE_EXTEND  m_mode;
};

#endif

};
