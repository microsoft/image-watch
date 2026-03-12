//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Implementation of the functions defined in vt_layer.cpp
//
//  History:
//      2009/01/10-mattu
//          Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_image.h"
#include "vt_layer.h"
#include "vt_readerwriter.h"
#include "vt_taskmanager.h"
#include "vt_transform.h"
#include "vt_convert.h" 

using namespace vt;

//+-----------------------------------------------------------------------------
//
// function: SplitLayerRect
//
//------------------------------------------------------------------------------
int vt::SplitLayerRect(_Out_writes_to_(4,return) vt::CRect* pR, int iCnt, IN const vt::CRect& rctSrc, 
				       int iWrapW, int iWrapH)
{
	if ( rctSrc.IsRectEmpty() )
	{
		return 0;
	}

	vt::CRect rctMid = rctSrc;
	if( iWrapW )
	{
		if( rctMid.right > iWrapW )
		{
			// wrap the right and recurse
			vt::CRect rctRight(0, rctMid.top, VtMin<int>(iWrapW, rctMid.right-iWrapW),
							   rctMid.bottom);
			iCnt += SplitLayerRect(pR, iCnt, rctRight, iWrapW, iWrapH);
			rctMid.right = iWrapW;
		}
		if( rctMid.left < 0 )
		{
			// wrap the left and recurse
			vt::CRect rctLeft(VtMax<int>(0, iWrapW+rctMid.left), rctMid.top, iWrapW,
							  rctMid.bottom);
			iCnt += SplitLayerRect(pR, iCnt, rctLeft, iWrapW, iWrapH);
			rctMid.left = 0;
		}  
	}
	if( iWrapH )
	{
		if( rctMid.bottom > iWrapH )
		{
			// wrap the bottom and recurse
			vt::CRect rctBottom(rctMid.left, 0, rctMid.right,
								VtMin<int>(iWrapH, rctMid.bottom-iWrapH));
			iCnt += SplitLayerRect(pR, iCnt, rctBottom, iWrapW, iWrapH);
			rctMid.bottom = iWrapH;
		}
		if( rctMid.top < 0 )
		{
			// wrap the top and recurse
			vt::CRect rctTop(rctMid.left, VtMax<int>(0,iWrapH+rctMid.top), 
							 rctMid.right, iWrapH);
			iCnt += SplitLayerRect(pR, iCnt, rctTop, iWrapW, iWrapH);
			rctMid.top = 0;
		}
	}

	int iIsectCnt = 0, iIsectId = 0;
    // max number of rectangles generated can't ever be more than 4 (and is usually 2...)
    ANALYZE_ASSUME(iCnt < 4);
	for( int i = 0; i < iCnt; i++ )
	{
		if( CRect().IntersectRect(pR+i, &rctMid) )
		{
			if( iIsectCnt == 0 )
			{
				iIsectId = i;
				pR[i].UnionRect(pR+i, &rctMid);
			}
			else
			{
				// merge this rect into first isect and shift down rest
				pR[iIsectId].UnionRect(pR+i, pR+iIsectId);
				for( int j = i; j < iCnt-1; j++ )
				{
					pR[j] = pR[j+1];
				}
				i--;
				iCnt--; 
			}
			iIsectCnt++;
		}
	}
	if( iIsectCnt==0 )
	{
        ANALYZE_ASSUME(iCnt <= 3);
		pR[iCnt] = rctMid;
		iCnt++;
	}
	return iCnt;
}

//+-----------------------------------------------------------------------------
//
// function: LayersIntersect
//
//------------------------------------------------------------------------------
bool 
vt::LayersIntersect(const vt::CRect& rct1, const vt::CRect& rct2,
					int iCompW, int iCompH)
{
	vt::CRect rctSplit1[2];
	UINT uSplitCnt1;
	SplitLayerRects(rct1, iCompW, iCompH, uSplitCnt1, rctSplit1);

	vt::CRect rctSplit2[2];
	UINT uSplitCnt2;
	SplitLayerRects(rct2, iCompW, iCompH, uSplitCnt2, rctSplit2);

	// intersect all split rects against each other
	for( UINT j = 0; j < uSplitCnt1; j++ )
	{
		for( UINT k = 0; k < uSplitCnt2; k++ )
		{
			vt::CRect rctIsect;
			if( rctIsect.IntersectRect(&rctSplit1[j], &rctSplit2[k]) )
			{
				return true;
			}
		}
	}
	return false;
}

void
vt::SplitLayerRects(const vt::CLayerImgInfo& linfo,
					OUT UINT& uOutputCount, OUT vt::CRect* prctRegions,
					IN const vt::CRect *prectCrop)
{
	SplitLayerRects(linfo.LayerRect(), 
					linfo.compositeWidth, linfo.compositeHeight,
					uOutputCount, prctRegions, prectCrop);
}

void
vt::SplitLayerRects(const vt::CRect& rct, int iPanoW, int iPanoH,
					OUT UINT& uOutputCount, OUT vt::CRect* prctRegions,
                    IN const vt::CRect *prectCrop)
{
	VT_ASSERT( rct.left >= 0 && rct.top >= 0 );

	if( rct.right > iPanoW ) 
	{
		// panos should only every wrap one way
		VT_ASSERT( rct.bottom <= iPanoH );
		if( rct.left < iPanoW)
		{
			uOutputCount = 2;
			prctRegions[0] = rct;
			prctRegions[0].right = iPanoW;
			prctRegions[1] = rct;
			prctRegions[1].left = 0; 
			prctRegions[1].right = rct.right - iPanoW;
		}
		else
		{
			uOutputCount = 1;
			prctRegions[0] = rct;
			prctRegions[0].left  -= iPanoW;
			prctRegions[0].right -= iPanoW;
		}
	}
	else if( rct.bottom > iPanoH )
	{
		if( rct.top < iPanoH)
		{
			uOutputCount = 2;
			prctRegions[0] = rct;
			prctRegions[0].bottom = iPanoH;
			prctRegions[1] = rct;
			prctRegions[1].top = 0; 
			prctRegions[1].bottom = rct.bottom - iPanoH;
		}
		else
		{
			uOutputCount = 1;
			prctRegions[0] = rct;
			prctRegions[0].top    -= iPanoH;
			prctRegions[0].bottom -= iPanoH;
		}
	}
	else
	{
		uOutputCount = 1;
		prctRegions[0] = rct;
	}

	// finally crop - note that this can change the number of regions as one
	// or more of them may be cropped out
   	vt::CRect rctCrop = prectCrop? *prectCrop: vt::CRect(0, 0, iPanoW, iPanoH);
	UINT uCropCnt = 0;
	for( UINT i = 0; i < uOutputCount; i++ )
	{
		if( prctRegions[uCropCnt].IntersectRect(&prctRegions[i], &rctCrop) )
		{
			uCropCnt++;
		}
	}
	uOutputCount = uCropCnt;
}

vt::CRect 
vt::CompositeRectToImgRect(const vt::CRect& rctComp, 
						   const vt::CLayerImgInfo& linfo)
{
	return CompositeRectToImgRect(rctComp, linfo.LayerRect(),
								  linfo.compositeWidth, linfo.compositeHeight);
}

vt::CRect 
vt::CompositeRectToImgRect(const vt::CRect& rctComp, const vt::CRect& rctLayer,
						   int iCompW, int iCompH)
{
	// this function expects rctComp to be within the composite
	VT_ASSERT( rctComp.right  <= iCompW && rctComp.bottom <= iCompH );

	vt::CRect result = rctComp;

	// special case for layers that wrap a 360 boundary in those cases
	// intersect with both the wrapped and not wrapped cases and pick the
	// one that intersects
	if( rctLayer.right > iCompW )
	{
		// try un-wrapped
		vt::CRect rctTry = result;
		if( !rctTry.IntersectRect(&rctTry, &rctLayer) )
		{
			// if not try to wrap around
			result.left  += iCompW;
			result.right += iCompW;
		}
	}
	else if( rctLayer.bottom > iCompH )
	{
		// try un-wrapped
		vt::CRect rctTry = result;
		if( !rctTry.IntersectRect(&rctTry, &rctLayer) )
		{
			// if not try to wrap around
			result.top    += iCompH;
			result.bottom += iCompH;
		}
	}

	// convert to image coords
	result.OffsetRect(-rctLayer.TopLeft());

	// clip to the image
	vt::CRect rctImg(0, 0, rctLayer.Width(), rctLayer.Height());
	result.IntersectRect(&result, &rctImg);

	return result;
}

//+-----------------------------------------------------------------------------
//
// function: GetContiguousRegion
//
//+-----------------------------------------------------------------------------
HRESULT
vt::GetContiguousRegion(IImageReader& source, 
						const vt::CRect& rctRegion, OUT CImg& imgRegion,
						const Byte* pFill)
{
	HRESULT hr = S_OK;

	CLayerImgInfo infoLayer = source.GetImgInfo();
	vt::CRect rctLayer = infoLayer.LayerRect();
	int iCompW = infoLayer.compositeWidth;
	int iCompH = infoLayer.compositeHeight;

	int iRgnWidth  = rctRegion.Width();
	int iRgnHeight = rctRegion.Height();
	VT_HR_EXIT( CreateImageForTransform(imgRegion, iRgnWidth, iRgnHeight,
									    infoLayer.type) );

	if( pFill )
	{
		imgRegion.Fill(pFill);
	}

	// split the region rect - in case it spans a 360 wrap-around
	vt::CRect rctRgnSplit[2];
	UINT uRgnSplitCnt;
	SplitLayerRects(rctRegion, iCompW, iCompH, uRgnSplitCnt, rctRgnSplit);

	// split the layer rect.  this is necessary because a region can
	// potentially cut a layer in two - e.g. for a horiz. pano, 
	// a right part and a left part that has wrapped around 360 and
	// intersected the region on the left hand side
	vt::CRect rctLayerSplit[2];
	UINT uLayerSplitCnt;
	SplitLayerRects(rctLayer, iCompW, iCompH, uLayerSplitCnt, rctLayerSplit);

	// for each portion of the region copy to the region
	for( UINT j = 0; j < uRgnSplitCnt; j++ )
	{
		for( UINT k = 0; k < uLayerSplitCnt; k++ )
		{
			vt::CRect rctCurSplit;
			if( rctCurSplit.IntersectRect(&rctLayerSplit[k], rctRgnSplit[j]) )
			{
				CImg imgRegionCur;
				vt::CRect rctImgInCache = CompositeRectToImgRect(rctCurSplit, 
												 rctRegion, iCompW, iCompH);
				imgRegion.Share(imgRegionCur, &rctImgInCache);

				vt::CRect rctSrc = CompositeRectToImgRect(rctCurSplit, 
												 rctLayer, iCompW, iCompH);
				VT_HR_EXIT( source.ReadRegion(rctSrc, imgRegionCur) );
			}
		}
	} 

Exit:
	return hr;
}
