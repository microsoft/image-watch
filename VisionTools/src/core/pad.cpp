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

#include "stdafx.h"

#include "vt_image.h"
#include "vt_convert.h"
#include "vt_pad.h"
#include "vt_layer.h"

using namespace vt;

//+-----------------------------------------------------------------------------
//
// function: GetNearestInBoundRect
// 
//------------------------------------------------------------------------------
void vt::GetNearestInBoundRect(vt::CRect& rctInB, vt::CRect& rctBuf,
							   const vt::CRect& rctOOB, const vt::CRect& rctSrc)
{
	rctBuf = vt::CRect(0,0,rctOOB.Width(),rctOOB.Height());
	 
	// handle left/right
	if( rctOOB.right <= rctSrc.left )
	{
		rctInB.left  = rctSrc.left;
		rctInB.right = rctInB.left+1;
		rctBuf.left  = rctBuf.right-1; 
	}
	else if( rctOOB.left >= rctSrc.right )
	{
		rctInB.left  = rctSrc.right-1;
		rctInB.right = rctInB.left+1;
		rctBuf.right = 1;
	}
	else
	{
		rctInB.left  = VtMax(LONG(0),rctOOB.left);
		rctInB.right = VtMin(rctOOB.right, rctSrc.right);
		rctBuf.left  = rctInB.left - rctOOB.left;
        rctBuf.right = rctBuf.left+rctInB.Width();
	}

	// handle top/bottom
	if( rctOOB.bottom <= rctSrc.top )
	{
		rctInB.top    = rctSrc.top;
		rctInB.bottom = rctInB.top+1;
		rctBuf.top    = rctBuf.bottom-1; 
	}
	else if( rctOOB.top >= rctSrc.bottom )
	{
		rctInB.top    = rctSrc.bottom-1;
		rctInB.bottom = rctInB.top+1;
		rctBuf.bottom = 1;
	}
	else
	{
		rctInB.top    = VtMax(LONG(0),rctOOB.top);
		rctInB.bottom = VtMin(rctOOB.bottom, rctSrc.bottom);
        rctBuf.top    = rctInB.top - rctOOB.top;
		rctBuf.bottom = rctBuf.top+rctInB.Height();
	}

	VT_ASSERT( rctBuf.Width()!=0 && rctBuf.Height()!=0 &&
			   (rctBuf.Width()==1 || rctBuf.Height()==1) );
			   
	VT_ASSERT( rctSrc.RectInRect(&rctInB) );
}

static void ZeroAlphaChannel(Byte* pDst, size_t elsz, size_t pxsz, int iCount)
{
	// TODO: sse version of this
	for( int i = 0; i < iCount; i++, pDst+=pxsz )
	{
		VtMemset(pDst+3*elsz, 0, elsz);
	}
}

void ZeroAlphaChannel(CImg& img)
{
	size_t pxsz = img.PixSize();
	size_t elsz = img.ElSize();
	for( int y = 0; y < img.Height(); y++ )
	{
		ZeroAlphaChannel( img.BytePtr(y), elsz, pxsz, img.Width() );
	}
}

//+-----------------------------------------------------------------------------
//
// function: AdjustSpanToNearestWrap
// 
// Synopsis: math for the wrap case adjusts a span to be within dim of t0
// 
//------------------------------------------------------------------------------
inline void AdjustSpanToNearestWrap(LONG& s0, LONG& s1, LONG t0, LONG dim)
{
	s1--;
	int m = ((s1 >= t0)? (s1-t0) : (s1-t0-(dim-1)))/dim;
	m *= dim;
	s0 -= m;
	s1  = s1+1-m;
}

//+-----------------------------------------------------------------------------
//
// function: VtGeneratePadOps
// 
//------------------------------------------------------------------------------

void VtGeneratePadOpsIntrnl(OUT PAD_OP* pOps, OUT UInt32& uOpsCnt,
							CRect rctRequestLayer,
							const CLayerImgInfo& infoReader,
							const IMAGE_EXTEND& ex,
							const vt::CPoint* pptDst)
{
	VT_ASSERT(ex.exHoriz == Wrap || ex.exVert == Wrap || ex.IsVerticalSameAsHorizontal());
	
	vt::CPoint ptDst = pptDst? *pptDst: vt::CPoint(0,0);
	vt::CRect rctFullLayer = infoReader.LayerRect();

	// do a quick check if the request is all inbounds then early out
	if( (rctRequestLayer & rctFullLayer) == rctRequestLayer )
	{
		vt::CRect rRead = rctRequestLayer-infoReader.origin;

		pOps->rctDst  = vt::CRect(0,0,rRead.Width(),rRead.Height()) + ptDst;
		pOps->rctRead = rRead;
		pOps->ptWrite = vt::CPoint(0,0);
		pOps->op      = ePadOpCopy;
		uOpsCnt       = 1;
		return;
	}

	uOpsCnt = 0;

	// Handle wrap of layer coordinates, this is a special case where the
	// composite canvas on which the image lives wraps around the horizontal
	// or vertical boundaries. Layers on the canvas might straddle a
	// boundary, so we need to have read operations take this into account. 
	if( infoReader.compositeWrapX )
	{
		int iCompW = infoReader.compositeWidth;
		AdjustSpanToNearestWrap(rctRequestLayer.left, rctRequestLayer.right,
								rctFullLayer.left, iCompW);

		if( CRect().IntersectRect(&rctRequestLayer, &rctFullLayer) )
		{
			// several special cases to handle here all having to do with the
			// request intersecting the source on both left and right side, but 
			// with a gap in the middle - same comment for the wrapY case below
			// 
			// 1st clause) left side of request does not intersect, right side
			//             does, but left side intersects after wrapping
			// 2nd clause) right side of request does not intersect, left side
			//             does, but right side intersects after wrapping
			// NOTE: this works for all padding modes, but the Extend mode
			//       will extend the values from recursed result below.  
			//       A way of dealing better with this may be to split the
			//       rect such that both left and right contribute to extend
			if( (rctRequestLayer.left  <  rctFullLayer.left  &&
				 rctRequestLayer.right <= rctFullLayer.right &&
				 rctRequestLayer.left+iCompW < rctFullLayer.right) )
			{
				// recurse on non-intersecting rect
				vt::CRect rctAdj(rctRequestLayer.left+iCompW,
								 rctRequestLayer.top,
								 rctFullLayer.left+iCompW,
								 rctRequestLayer.bottom);
				UInt32 uSubOpsCnt;
				VtGeneratePadOpsIntrnl(pOps, uSubOpsCnt, rctAdj,
									   infoReader, ex, &ptDst);
				uOpsCnt += uSubOpsCnt;
				pOps    += uSubOpsCnt;

            	VT_ASSERT( uOpsCnt >= 1 && uOpsCnt <= 16  );

				// continue with the intersecting rect
				rctRequestLayer.left = rctFullLayer.left;
				ptDst.x += rctAdj.Width();
			}
			else if( (rctRequestLayer.right >  rctFullLayer.right &&
					  rctRequestLayer.left  >= rctFullLayer.left  &&
					  rctRequestLayer.right-iCompW > rctFullLayer.left) )
			{
				// recurse on non-intersecting rect
				vt::CRect rctAdj(rctFullLayer.right-iCompW,
								 rctRequestLayer.top,
								 rctRequestLayer.right-iCompW,
								 rctRequestLayer.bottom);
				vt::CPoint ptDstAdj(
					ptDst.x+rctFullLayer.right-rctRequestLayer.left, ptDst.y);
				UInt32 uSubOpsCnt;
				VtGeneratePadOpsIntrnl(pOps, uSubOpsCnt, rctAdj,
									   infoReader, ex, &ptDstAdj);
				uOpsCnt += uSubOpsCnt;
				pOps    += uSubOpsCnt;

            	VT_ASSERT( uOpsCnt >= 1 && uOpsCnt <= 16  );

				// continue with the intersecting rect
				rctRequestLayer.right = rctFullLayer.right;
			}
		}
	}
	if( infoReader.compositeWrapY )
	{
		int iCompH = infoReader.compositeHeight;
		AdjustSpanToNearestWrap(rctRequestLayer.top, rctRequestLayer.bottom,
								rctFullLayer.top, iCompH);

		if( CRect().IntersectRect(&rctRequestLayer, &rctFullLayer) )  
		{
			if( (rctRequestLayer.top    <  rctFullLayer.top    &&
				 rctRequestLayer.bottom <= rctFullLayer.bottom &&
				 rctRequestLayer.top+iCompH < rctFullLayer.bottom) )
			{
				// recurse on non-intersecting rect
				vt::CRect rctAdj(rctRequestLayer.left,
								 rctRequestLayer.top+iCompH,
								 rctRequestLayer.right,
								 rctFullLayer.top+iCompH);
				UInt32 uSubOpsCnt;
				VtGeneratePadOpsIntrnl(pOps, uSubOpsCnt, rctAdj,
									   infoReader, ex, &ptDst);
				uOpsCnt += uSubOpsCnt;
				pOps    += uSubOpsCnt;
	
            	VT_ASSERT( uOpsCnt >= 1 && uOpsCnt <= 16  );

				// continue with the intersecting rect
				rctRequestLayer.top = rctFullLayer.top;
				ptDst.y += rctAdj.Height();
			}
			else if( (rctRequestLayer.bottom >  rctFullLayer.bottom &&
					  rctRequestLayer.top    >= rctFullLayer.top    &&
					  rctRequestLayer.bottom-iCompH > rctFullLayer.top) )
			{
				// recurse on non-intersecting rect
				vt::CRect rctAdj(rctRequestLayer.left,
								 rctFullLayer.bottom-iCompH,
								 rctRequestLayer.right,
								 rctRequestLayer.bottom-iCompH);
				vt::CPoint ptDstAdj(
					ptDst.x, ptDst.y+rctFullLayer.bottom-rctRequestLayer.top);
				UInt32 uSubOpsCnt;
				VtGeneratePadOpsIntrnl(pOps, uSubOpsCnt, rctAdj,
									   infoReader, ex, &ptDstAdj);
				uOpsCnt += uSubOpsCnt;
				pOps    += uSubOpsCnt;
	
            	VT_ASSERT( uOpsCnt >= 1 && uOpsCnt <= 16  );

				// continue with the intersecting rect
				rctRequestLayer.bottom = rctFullLayer.bottom;
			}
		}
	}

	// convert from layer coordinates to image coordinates
	vt::CRect rctRequestSrc = rctRequestLayer - rctFullLayer.TopLeft();
	LONG iW = (LONG)infoReader.width;
	LONG iH = (LONG)infoReader.height;

	// test for horizontal wrap
	if( ex.exHoriz == Wrap )
	{
		AdjustSpanToNearestWrap(rctRequestSrc.left, rctRequestSrc.right, 0, iW);

		// if any part of the request is off to the left/right, then split 
		// these off and recurse
		if( rctRequestSrc.right > iW )
		{
			// right hand rect
			vt::CRect rctAdj(iW, rctRequestSrc.top, rctRequestSrc.right, 
							 rctRequestSrc.bottom);
			vt::CPoint ptDstAdj(ptDst.x+iW-rctRequestSrc.left, ptDst.y);
			UInt32 uSubOpsCnt;
		    VtGeneratePadOpsIntrnl(pOps, uSubOpsCnt, rctAdj+rctFullLayer.TopLeft(),
							       infoReader, ex, &ptDstAdj);
			uOpsCnt += uSubOpsCnt;
			pOps    += uSubOpsCnt;

            VT_ASSERT( uOpsCnt >= 1 && uOpsCnt <= 16  );

			// continue with the central rect
			rctRequestSrc.right = iW;
		}
		if( rctRequestSrc.left < 0 )
		{
			// left hand rect
			vt::CRect rctAdj(rctRequestSrc.left, rctRequestSrc.top, 0, 
							 rctRequestSrc.bottom);
			UInt32 uSubOpsCnt;
			VtGeneratePadOpsIntrnl(pOps, uSubOpsCnt, rctAdj+rctFullLayer.TopLeft(),
								   infoReader, ex, &ptDst);
			uOpsCnt += uSubOpsCnt;
			pOps    += uSubOpsCnt;

            VT_ASSERT( uOpsCnt >= 1 && uOpsCnt <= 16  );

			// continue with the central rect
			ptDst.x += rctAdj.Width();
			rctRequestSrc.left = 0;
	    }
	}

	// test for vertical wrap
	if( ex.exVert == Wrap )
	{
		AdjustSpanToNearestWrap(rctRequestSrc.top, rctRequestSrc.bottom, 0, iH);

		// if any part of the request is off to the top/bottom, then split 
		// these off and recurse
		if( rctRequestSrc.bottom > iH )
		{
			// right hand rect
			vt::CRect rctAdj(rctRequestSrc.left, iH, 
							 rctRequestSrc.right, rctRequestSrc.bottom);
			vt::CPoint ptDstAdj(ptDst.x, ptDst.y+iH-rctRequestSrc.top);
			UInt32 uSubOpsCnt;
			VtGeneratePadOpsIntrnl(pOps, uSubOpsCnt, rctAdj+rctFullLayer.TopLeft(),
							       infoReader, ex, &ptDstAdj);
			uOpsCnt += uSubOpsCnt;
			pOps    += uSubOpsCnt;

            VT_ASSERT( uOpsCnt >= 1 && uOpsCnt <= 16  );

			// continue with the central rect
			rctRequestSrc.bottom = iH;
		}
		if( rctRequestSrc.top < 0 )
		{
			// left hand rect
			vt::CRect rctAdj(rctRequestSrc.left, rctRequestSrc.top, 
							 rctRequestSrc.right, 0);
			UInt32 uSubOpsCnt;
			VtGeneratePadOpsIntrnl(pOps, uSubOpsCnt, rctAdj+rctFullLayer.TopLeft(),
								   infoReader, ex, &ptDst);
			uOpsCnt += uSubOpsCnt;
			pOps    += uSubOpsCnt;

            VT_ASSERT( uOpsCnt >= 1 && uOpsCnt <= 16  );

			// continue with the central rect
			ptDst.y += rctAdj.Height();
			rctRequestSrc.top = 0;
		}
	}
 
	// clip the requested rect against the rctFullLayer for that source 
	// and only request what is available
	vt::CRect rctAvailableSrc = rctRequestSrc & infoReader.Rect();
	
	pOps->rctDst = 
		vt::CRect(0,0,rctRequestSrc.Width(),rctRequestSrc.Height()) + ptDst;

	if( rctAvailableSrc == rctRequestSrc )
	{
		// if no padding needs to be done then insert a copy operation
		pOps->rctRead = rctRequestSrc;
		pOps->ptWrite = vt::CPoint(0,0);
		pOps->op      = ePadOpCopy;
	}
	else if( rctAvailableSrc.IsRectEmpty() )
	{
		// if it's empty determine if we are out on the horizontal or vertical direction
		if( (ex.exHoriz != Extend && ex.exHoriz != ExtendZeroAlpha &&
			 (rctRequestSrc.right <= 0 || rctRequestSrc.left >= iW || iW == 0)) )
		{			
			// if zero or constant padding needs to be done then indicate a 
			// pad op that doesn't need to read any data
			pOps->rctRead = vt::CRect(0,0,0,0);
			pOps->ptWrite = vt::CPoint(0,0);
			pOps->op      = ePadOpConstantHoriz;
		}
		else if( (ex.exVert != Extend && ex.exVert != ExtendZeroAlpha &&
				  (rctRequestSrc.bottom <= 0 || rctRequestSrc.top >= iH || iH == 0)) )
		{			
			// if zero or constant padding needs to be done then indicate a 
			// pad op that doesn't need to read any data
			pOps->rctRead = vt::CRect(0,0,0,0);
			pOps->ptWrite = vt::CPoint(0,0);
			pOps->op      = ePadOpConstantVert;
		}
		else
		{
			// find the nearest image edge and use that to pad
			vt::CRect rctInDst;
			GetNearestInBoundRect(rctAvailableSrc, rctInDst,
								  rctRequestSrc,   infoReader.Rect());
			pOps->rctRead = rctAvailableSrc;
			pOps->ptWrite = rctInDst.TopLeft();
			pOps->op      = ePadOpPadNonIntersect;
		}
	}
	else
	{
		pOps->rctRead = rctAvailableSrc;
		pOps->ptWrite = rctAvailableSrc.TopLeft() - rctRequestSrc.TopLeft();
		pOps->op      = ePadOpPad;
	}

	uOpsCnt++;

    VT_ASSERT( uOpsCnt >= 1 && uOpsCnt <= 16  );
}

void vt::VtGeneratePadOps(OUT PAD_OP(& ops)[16], OUT UInt32& uOpsCnt,
						  CRect rctRequestLayer,
						  const CLayerImgInfo& infoReader,
						  const IMAGE_EXTEND& ex,
						  const vt::CPoint* pptDst)
{
	return VtGeneratePadOpsIntrnl(ops, uOpsCnt, rctRequestLayer, infoReader, ex,
							      pptDst);
}


//+-----------------------------------------------------------------------------
//
// function: VtPadImage
// 
//------------------------------------------------------------------------------
HRESULT
CopyPadSrc(OUT CImg& imgDst, const CRect& rSrc, const CImg* pSrc)
{ 
	CImg imgShare;
	pSrc->Share(imgShare, &rSrc);
	return VtConvertImage(imgDst, imgShare);
}

#ifndef VT_NO_XFORMS
HRESULT
CopyPadSrc(OUT CImg& imgDst, const CRect& rctSrc, IImageReader* pSrc)
{ return pSrc->ReadRegion(rctSrc, imgDst); } 
#endif

bool IsValid(const IImageReader*)
{ return true; }
bool IsValid(const CImg* pSrc)
{ return pSrc->IsValid(); }

// Templated functions that return the max/min of specified types
template<typename T>
struct FindTypeMin
{
    T DoOp() const { return ElTraits<T>::TypeMin(); }
};

template<typename T>
struct FindTypeMax
{
    T DoOp() const { return ElTraits<T>::TypeMax(); }
};

// Sets the max/min type value based on the type
template<template<typename> class OpType>
void ExtendTypeValSetter(void *elemPtr, int dstType)
{
    switch( EL_FORMAT(dstType) )
    {
    case EL_FORMAT_BYTE:
        {
            OpType<Byte> fmb;
            *((Byte*) elemPtr) = fmb.DoOp();
            break;
        }
    case EL_FORMAT_SHORT:
        {
            OpType<UInt16> fms;
            *((UInt16*) elemPtr) = fms.DoOp();
            break;
        }
    case EL_FORMAT_INT:
        {
            OpType<int> fmi;
            *((int*)elemPtr) = fmi.DoOp();
            break;}
    case EL_FORMAT_FLOAT:
        {
            OpType<float> fmf;
            *((float*)elemPtr) = fmf.DoOp();
            break;}
    case EL_FORMAT_HALF_FLOAT:
        {
            OpType<HALF_FLOAT> fmhf;
            *((HALF_FLOAT*)elemPtr) = fmhf.DoOp();
            break;
        }
    }

}

HRESULT ConvertConstval(EXTEND_CONSTVAL& excDst, int iDstType,
                        const EXTEND_CONSTVAL& excSrc)
{
    VT_HR_BEGIN()

	VT_HR_EXIT( excDst.Alloc(iDstType) );

    int iDstBands = VT_IMG_BANDS(iDstType);

	if (VT_IMG_BANDS(excSrc.iType) == 1 && iDstBands > 1)
    {
    	EXTEND_CONSTVAL excTmp;
    	VT_HR_EXIT( excTmp.Alloc(VT_IMG_MAKE_TYPE(EL_FORMAT(excSrc.iType),iDstBands)) );
		VtFillSpan(excTmp.Val(), excSrc.Val(), VT_IMG_ELSIZE(excTmp.iType), iDstBands);

        VT_HR_EXIT( VtConvertSpan(excDst.Val(), iDstType,
                                  excTmp.Val(), excTmp.iType,
                                  iDstBands) );
    }
    else
	{
		VT_HR_EXIT( VtConvertSpan(excDst.Val(), excDst.iType,
                                  excSrc.Val(), excSrc.iType,
                                  VT_IMG_BANDS(excSrc.iType)) );
	}

    VT_HR_END()
}

inline HRESULT 
SetInternalExtend(ExtendMode& ex, EXTEND_CONSTVAL& exc, 
                  const ExtendMode& ex_src, const EXTEND_CONSTVAL& exc_src,
                  int iDstType, bool bEmptySrc)
{
    VT_HR_BEGIN()

    if( ex_src==Constant )
	{
        ex = Constant;
        if( VT_IMG_SAME_BE(exc_src.iType, iDstType) )
        {
            VT_HR_EXIT( exc.Initialize(&exc_src) );
        }
        else
        {            
            VT_HR_EXIT( ConvertConstval(exc, iDstType, exc_src) );
        }
    }
    else
    {
        ex = (bEmptySrc && !ExtendModeAllowsEmptySrc(ex_src))? Zero: ex_src;
    }

    VT_HR_END()
}

template <class T>
HRESULT VtCropPadImageInternal(OUT CImg &imgDst, const CRect &rRequest, 
							   const CLayerImgInfo& infoSrc, IN T* pSrc,
							   const IMAGE_EXTEND& ex_src)
{
	// if request is empty, return
	if( rRequest.IsRectEmpty() )
    {
		return S_OK;
    }


	// source needs to be present if source rect is
	bool bEmptySrc = vt::CRect(infoSrc.Rect()).IsRectEmpty();
	if ( !IsValid(pSrc) && !bEmptySrc )
    {
		return E_INVALIDSRC;
    }
	
	VT_HR_BEGIN()

	// create the dest image if it hasn't already been
	VT_HR_EXIT( CreateImageForTransform(
        imgDst, rRequest.Width(), rRequest.Height(), infoSrc.type) );  
  
	// convert extend type to internal copy. This is necessary in two cases
    // 1) extend type Constant and the IMAGE_EXTEND color value is a different
    //    pixel type than the destination image
    // 2) if the source image is empty the convention is to force the extend
    //    mode to Zero if the requested extend mode required us to sample source
    //    pixel values
    IMAGE_EXTEND ex;
    VT_HR_EXIT( SetInternalExtend(ex.exHoriz, ex.excHoriz, 
                                  ex_src.exHoriz, ex_src.excHoriz, 
                                  imgDst.GetType(), bEmptySrc) );
    VT_HR_EXIT( SetInternalExtend(ex.exVert, ex.excVert, 
                                  ex_src.exVert, ex_src.excVert, 
                                  imgDst.GetType(), bEmptySrc) );

    PAD_OP ops[16];
    UInt32 uOpsCnt;
    VtGeneratePadOps(ops, uOpsCnt, vt::CRect(rRequest), infoSrc, ex);

    VT_ASSERT( uOpsCnt >= 1 && uOpsCnt <= 16  );

    for( UInt32 i = 0; i < uOpsCnt; i++ )
    {
        const PAD_OP& o = ops[i];

        VT_ASSERT( (o.rctDst & imgDst.Rect()) == o.rctDst );
        CImg imgDstOp;
        VT_HR_EXIT( imgDst.Share(imgDstOp, &o.rctDst) );

        switch( o.op )
        {
        case ePadOpCopy:
            VT_HR_EXIT( CopyPadSrc(imgDstOp, o.rctRead, pSrc) );
            break;
        case ePadOpConstantHoriz:
            if( ex.exHoriz == Zero || ex.excHoriz.Val()==NULL )
            { 
                VT_HR_EXIT( imgDstOp.Clear() );
            }
            else
            {
                VT_HR_EXIT( imgDstOp.Fill((Byte*)ex.excHoriz.Val()) );
            }
            break;
        case ePadOpConstantVert:
            if( ex.exVert == Zero || ex.excVert.Val()==NULL )
            { 
                VT_HR_EXIT( imgDstOp.Clear() );
            }
            else
            {
                VT_HR_EXIT( imgDstOp.Fill((Byte*)ex.excVert.Val()) );
            }
            break;
        case ePadOpPad:
        case ePadOpPadNonIntersect:
            {
                CImg imgWrite;
                vt::CRect rctWrite = 
                    vt::CRect(0,0,o.rctRead.Width(),o.rctRead.Height())+o.ptWrite;
                VT_HR_EXIT( imgDstOp.Share(imgWrite, &rctWrite) );
                VT_HR_EXIT( CopyPadSrc(imgWrite, o.rctRead, pSrc) );
                if( o.op==ePadOpPadNonIntersect && imgDst.Bands()==4 &&
                    (ex.exHoriz==ExtendZeroAlpha || ex.exVert==ExtendZeroAlpha) )
                {
                    ZeroAlphaChannel(imgWrite);
                }
                VtExtendConstZeroPadImage(
                    imgDstOp, rctWrite, ex.exHoriz, ex.exVert,
                    ex.excHoriz.Val(), ex.excVert.Val());
			}
			break;
		}
	}

	VT_HR_END()
}



HRESULT vt::VtCropPadImage(OUT CImg &imgDst, const CRect &rRequest, 
							  const CImg& imgSrc, const IMAGE_EXTEND& ex)
{
	return VtCropPadImageInternal(imgDst, rRequest, imgSrc.GetImgInfo(), &imgSrc, 
								  ex); 
}

#ifndef VT_NO_XFORMS
HRESULT vt::VtCropPadImage(OUT CImg &imgDst, const CRect &rRequest, 
							  const CLayerImgInfo& infoSrc, 
							  IN IImageReader* pReader, const IMAGE_EXTEND& ex)
{ return VtCropPadImageInternal(imgDst, rRequest, infoSrc, pReader, ex); }

HRESULT vt::VtPrefetchPadded(const CRect &rRequest, const CLayerImgInfo& infoSrc, 
							 IN IImageReader* pReader, const IMAGE_EXTEND& ex)
{
	// if request is empty, return
	if( rRequest.IsRectEmpty() )
	{
		return S_OK;
	}

	// also do a quick check if the request is all inbounds then early out
	if( (rRequest & infoSrc.LayerRect()) == rRequest )
	{
		return pReader->Prefetch(rRequest-infoSrc.origin);
	}

	VT_HR_BEGIN()

	PAD_OP ops[16];
	UInt32 uOpsCnt;
	VtGeneratePadOps(ops, uOpsCnt, rRequest, infoSrc, ex);

	VT_ASSERT( uOpsCnt >= 1 && uOpsCnt <= 16 );

	for( UInt32 j = 0; j < uOpsCnt; j++ )
	{
		const PAD_OP& o = ops[j];
		if( o.op == ePadOpCopy || o.op == ePadOpPad || 
			o.op == ePadOpPadNonIntersect )
		{
			VT_HR_EXIT( pReader->Prefetch(o.rctRead) );
		}
	}

	VT_HR_END()
}
	
#endif

//+-----------------------------------------------------------------------------
//
// function: VtExtendConstZeroPadImage
//
//------------------------------------------------------------------------------
void vt::VtExtendConstZeroPadImage(IN OUT CImg &imgPad, const CRect &rInB, 
								   ExtendMode exHoriz, ExtendMode exVert, 
								   const void *pHorizVal, const void *pVertVal)
{
	int nb      = imgPad.Bands();
	size_t elsz = imgPad.ElSize();
	size_t pxsz = imgPad.PixSize();

	VT_ASSERT( CRect(imgPad.Rect()).RectInRect(rInB) );

    if( exVert == Extend || exVert == ExtendZeroAlpha )
    {
        size_t uiCopyWidth = rInB.Width() * pxsz;

        // extend top
        Byte* pDst = imgPad.BytePtr(rInB.left, 0);
        Byte* pSrc = imgPad.BytePtr(rInB.left, rInB.top);
        for ( int y = 0 ; y < rInB.top; y++, pDst += imgPad.StrideBytes() )
		{
            VtMemcpy(pDst, pSrc, uiCopyWidth);
			if( y==0 && nb==4 && exVert == ExtendZeroAlpha )
			{
				// only zero out first dst then copy results subsequently
				// same comment applies to all calls to ZeroAlphaChannel below
				ZeroAlphaChannel(pDst, elsz, pxsz, rInB.Width());
				pSrc = pDst;
			}
		}

        // extend bottom
        pSrc = imgPad.BytePtr(rInB.left, rInB.bottom-1);
        pDst = pSrc + imgPad.StrideBytes();
        for ( int y = rInB.bottom ; y < imgPad.Height(); 
			  y++, pDst += imgPad.StrideBytes() )
		{
            VtMemcpy(pDst, pSrc, uiCopyWidth);
			if( y==rInB.bottom && nb==4 && exVert == ExtendZeroAlpha )
			{
				ZeroAlphaChannel(pDst, elsz, pxsz, rInB.Width());
				pSrc = pDst;
			}
		}
    }
    else if( exVert == Zero || ((exVert == Constant)  && pVertVal == NULL) )
    {
        // zero top
		const CRect tmpRectTop = CRect(rInB.left, 0, rInB.right, rInB.top);
        imgPad.Clear(&tmpRectTop);

        // zero bottom
		const CRect tmpRectBottom = CRect(rInB.left, rInB.bottom, 
										  rInB.right, imgPad.Height());
        imgPad.Clear(&tmpRectBottom);
    }
    else if( exVert == Constant) 
    {
        // fill top
		const CRect tmpRectTop = CRect(rInB.left, 0, rInB.right, rInB.top);
        imgPad.Fill((Byte*)pVertVal, &tmpRectTop);

        // fill bottom
		const CRect tmpRectBottom = CRect(rInB.left, rInB.bottom, 
                                          rInB.right, imgPad.Height());
        imgPad.Fill((Byte*)pVertVal, &tmpRectBottom);
    }
    else if(exVert == TypeMin || exVert == TypeMax)
    {
        UInt32 val;
        if(exVert == TypeMin)
            ExtendTypeValSetter<FindTypeMin>((void *) &val, imgPad.GetType());
        else
            ExtendTypeValSetter<FindTypeMax>((void *) &val, imgPad.GetType());

        // fill top
        const CRect tmpRectTop = CRect(rInB.left, 0, rInB.right, rInB.top);
        imgPad.Fill((Byte*) &val, &tmpRectTop, -1, true);

        // fill bottom
        const CRect tmpRectBottom = CRect(rInB.left, rInB.bottom, 
                                          rInB.right, imgPad.Height());
        imgPad.Fill((Byte*) &val, &tmpRectBottom, -1, true);
    }
    else
	{
		// shouldn't call this unless vert pad is a nop
		VT_ASSERT(rInB.top==0 && rInB.bottom==imgPad.Height());
	}

    if( exHoriz == Extend || exHoriz == ExtendZeroAlpha )
    {
        // extend left & right
        Byte* pDstL = imgPad.BytePtr(0, 0);
        Byte* pSrcL = pDstL + rInB.left * pxsz;
        int uiSpanCountL = rInB.left;
        Byte* pSrcR = imgPad.BytePtr(rInB.right-1, 0);
        Byte* pDstR = pSrcR + pxsz;
        int uiSpanCountR = imgPad.Width() - rInB.right;
        for( int y = 0 ; y < imgPad.Height(); y++ )
        {
			if( nb==4 && exHoriz == ExtendZeroAlpha && uiSpanCountL )
			{
				VtFillSpan(pDstL, pSrcL, pxsz, 1);
				ZeroAlphaChannel(pDstL, elsz, pxsz, 1);
				VtFillSpan(pDstL+pxsz, pDstL, pxsz, uiSpanCountL-1);
			}
			else
			{
				VtFillSpan(pDstL, pSrcL, pxsz, uiSpanCountL);
			}
            pSrcL += imgPad.StrideBytes();
            pDstL += imgPad.StrideBytes();
			if( nb==4 && exHoriz == ExtendZeroAlpha && uiSpanCountR )
			{
				VtFillSpan(pDstR, pSrcR, pxsz, 1);
				ZeroAlphaChannel(pDstR, elsz, pxsz, 1);
				VtFillSpan(pDstR+pxsz, pDstR, pxsz, uiSpanCountR-1);
			}
			else
			{
				VtFillSpan(pDstR, pSrcR, pxsz, uiSpanCountR);
			}
            pSrcR += imgPad.StrideBytes();
            pDstR += imgPad.StrideBytes();
        }
    }
    else if( exHoriz == Zero || ((exHoriz == Constant) && pHorizVal == NULL) )
    {
        // zero left
		const CRect tmpRectLeft = CRect(0, 0, rInB.left, imgPad.Height()); 
        imgPad.Clear(&tmpRectLeft);

        // zero right
		const CRect tmpRectRight = CRect(rInB.right, 0, imgPad.Width(), 
			imgPad.Height()); 
        imgPad.Clear(&tmpRectRight);
    }
    else if( exHoriz == Constant )
    {
        // fill left
		const CRect tmpRectLeft = CRect(0, 0, rInB.left, imgPad.Height());
        imgPad.Fill((Byte*)pHorizVal, &tmpRectLeft);

        // fill right
		const CRect tmpRectRight = CRect(rInB.right, 0, 
										 imgPad.Width(), imgPad.Height());
        imgPad.Fill((Byte*)pHorizVal, &tmpRectRight);
    }
    else if( exHoriz == TypeMin || exHoriz == TypeMax )
    {
        UInt32 val;
        if(exHoriz == TypeMin)
            ExtendTypeValSetter<FindTypeMin>((void *) &val, imgPad.GetType());
        else 
            ExtendTypeValSetter<FindTypeMax>((void *) &val, imgPad.GetType());

        // fill left
        const CRect tmpRectLeft = CRect(0, 0, rInB.left, imgPad.Height());
        imgPad.Fill((Byte*) &val, &tmpRectLeft, -1, true);

        // fill right
        const CRect tmpRectRight = CRect(rInB.right, 0, 
                                         imgPad.Width(), imgPad.Height());
        imgPad.Fill((Byte*) &val, &tmpRectRight, -1, true);
    }
	else
	{
		// shouldn't call this unless horiz pad is a nop
		VT_ASSERT(rInB.left==0 && rInB.right==imgPad.Width());
	}
}


