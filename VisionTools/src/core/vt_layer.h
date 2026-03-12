//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Defintions for layer images.  A layer image is an image that contains
//      a postion on a larger composite surface
//
//  History:
//      2007/11/1-mattu
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_image.h"
#include "vt_atltypes.h"

namespace vt {

class IImageReader;

//+-----------------------------------------------------------------------
//
//  Class: CLayerImg
//
//  Synopsis: A generic image with a position.  The position implies that the 
//            image is a layer on a larger composite.  The image can
//            potentially be split across the composite.  In this case
//            the image width/height will wrap across iCompW/iCompH
//
//------------------------------------------------------------------------
class CLayerImgInfo: public CImgInfo
{
public:
	CLayerImgInfo() : compositeWidth(0), compositeHeight(0), origin(0,0),
		compositeWrapX(false), compositeWrapY(false)
	{}

	CLayerImgInfo(const CImgInfo& inf) :CImgInfo(inf), origin(0,0), 
		compositeWrapX(false), compositeWrapY(false)
	{
		compositeWidth = inf.width;
		compositeHeight = inf.height;
	} 

	CLayerImgInfo(const CImgInfo& inf, int cw, int ch, const CPoint& loc,
		bool bCompWrapX=false, bool bCompWrapY=false) 
		: CImgInfo(inf), compositeWidth(cw), compositeHeight(ch), origin(loc),
		compositeWrapX(bCompWrapX), compositeWrapY(bCompWrapY)
	{} 

   	RECT LayerRect() const { RECT r = { origin.x, origin.y, origin.x + width, 
		origin.y + height }; return r; }

public:
	/// <summary> Composite width </summary>
	int compositeWidth;  
	/// <summary> Composite height </summary>
	int compositeHeight;
	
	/// <summary> Composite origin </summary>
	vt::CPoint origin;   
	
	/// <summary> Horizontal wrap mode </summary>
	bool compositeWrapX; 
	/// <summary> Vertical wrap mode </summary>
	bool compositeWrapY;
};

//+-----------------------------------------------------------------------------
//
// function: BoundAtLevel, LayerRectAtLevel, DimensionsAtLevel
//
// synopsis: this is the standard way of computing the boundary coordinates in
//           a images at a lower level given the coordinate at level 0.
//
//           the BoundAtLevel code computes the following:
//
//           for left, top bounds:     ceil(x / scale)
//           for right, bottom bounds: floor((x-1)/scale)+1
//
//           In words this means don't generate any samples that are spatially
//           to the left/above of the left-most/top-most pixel or to the 
//           right/below of the right-most/bottom-most pixel.  Note that 
//           right/bottom bound is one past the address of the last pixel, 
//           thus the x-1 above.  The two expressions above are equivalent.  
//           The implementation below is for power of 2 scales supplied by 
//           caller as 1<<level - note that BoundAtLevel can be used for 
//           positive or negative numbers.
//          
//------------------------------------------------------------------------------
inline
int BoundAtLevel(int x, UInt32 level)
{
	return ((x - 1) >> level) + 1;
}

inline
vt::CRect LayerRectAtLevel(const vt::CRect& r, UInt32 level)
{
	vt::CRect rctLev = vt::CRect( BoundAtLevel(r.left, level),
								  BoundAtLevel(r.top,  level),
								  BoundAtLevel(r.right,  level),
								  BoundAtLevel(r.bottom, level) );
 
	// in this function and below.  if the resulting size is <2 by <2 then check 
	// if the previous level was also <2 by <2.  if so then this level is 0 by 0
	if( rctLev.Width() <= 1 && rctLev.Height() <= 1 && level != 0 )
	{
		UInt32 l1 = level-1;
		vt::CRect rctLev1 = vt::CRect( BoundAtLevel(r.left, l1),
									   BoundAtLevel(r.top,  l1),
									   BoundAtLevel(r.right,  l1),
									   BoundAtLevel(r.bottom, l1) );
		if( rctLev1.Width() <= 1 && rctLev1.Height() <= 1 )
		{
			rctLev.right  = rctLev.left;
			rctLev.bottom = rctLev.top;
		}
	}
    return rctLev;
}

inline
void DimensionsAtLevel(int& lw, int& lh, int w, int h, UInt32 level)
{
	lw = BoundAtLevel(w, level);
	lh = BoundAtLevel(h, level);
	if( lw <= 1 && lh <= 1 && level != 0 )
	{
		UInt32 l1 = level-1;
		int lw1 = BoundAtLevel(w, l1);
		int lh1 = BoundAtLevel(h, l1);
		if( lw1 <= 1 && lh1 <= 1 )
		{
			lw = lh = 0;
		}
	}
}

inline CLayerImgInfo InfoAtLevel(const CLayerImgInfo& src, UInt32 level)
{
	CRect rct = LayerRectAtLevel(src.LayerRect(), level);
	int   wc, hc;
	DimensionsAtLevel(wc, hc, src.compositeWidth, src.compositeHeight, level);
	return CLayerImgInfo(CImgInfo(rct.Width(), rct.Height(), src.type),
		wc, hc, rct.TopLeft(), src.compositeWrapX, src.compositeWrapY);
}

//+-----------------------------------------------------------------------------
//
//  function: LayerRectExpand
//
// synopsis: This function expands a layer rect by a factor of fPad.  The
//           expansion is done symetrically in all directions
// 
//------------------------------------------------------------------------------
inline
vt::CRect LayerRectExpand(const vt::CRect& r, float fPad)
{
	int iPad = F2I(float(VtMax(r.Width(), r.Height()))*0.5f*fPad);
	return vt::CRect(r.left  - iPad, r.top    - iPad,
	                 r.right + iPad, r.bottom + iPad);
}

//+-----------------------------------------------------------------------------
//
// function: SplitLayerRect
//
// synopsis: For a rect that goes off the edge of a composite this function
//           splits into component rects that are all within the composite
//           bounds.  If the iWrapW, iWrapH parameters are non-zero this
//           indicates a composite surfaces that wraps in that dimension.
//           If these are zero then no split happens.
//
//------------------------------------------------------------------------------
int SplitLayerRect(_Out_writes_to_(4,return) vt::CRect* pR, int iCnt, IN const vt::CRect& rctSrc, 
				   int iWrapW, int iWrapH);

inline int SplitLayerRect(_Out_writes_to_(4,return) vt::CRect* pR, IN const CLayerImgInfo& linfo)
{
	int iWrapW = linfo.compositeWrapX? linfo.compositeWidth:  0;
	int iWrapH = linfo.compositeWrapY? linfo.compositeHeight: 0;
	return SplitLayerRect(pR, 0, linfo.LayerRect(), iWrapW, iWrapH);
}

//+-----------------------------------------------------------------------------
//
// function: LayersIntersect
//
// synopsis: this function determines if a layer intersects with a composite
//           rect.  the special case this is useful for is that sometimes
//           layers wrap a 360 boundary
//
//------------------------------------------------------------------------------
bool
LayersIntersect(const vt::CRect& rctLayer1, const vt::CRect& rctLayer2,
				int iCompW, int iCompH);

//+-----------------------------------------------------------------------------
//
// function: SplitLayerRects
//
// synopsis: this function splits rectangles into 2 rects if the region spans
//           a 360 composite, otherwise it returns a single rect
//
//------------------------------------------------------------------------------
void
SplitLayerRects(const vt::CRect& rct, int iPanoW, int iPanoH,
				OUT UInt32& uOutputCount, OUT vt::CRect* prctRegions,
				IN const vt::CRect *prectCrop = NULL);

void
SplitLayerRects(const vt::CLayerImgInfo& linfo,
				OUT UInt32& uOutputCount, OUT vt::CRect* prctRegions,
				IN const vt::CRect *prectCrop = NULL);


//+-----------------------------------------------------------------------------
//
// function: CompositeRectToImgRect
//
// synopsis: converts the supplied rect in composite coordinates to a rect 
//           in the coordinates of the layer image
//
//------------------------------------------------------------------------------
vt::CRect 
CompositeRectToImgRect(const vt::CRect& rctLayer, const vt::CLayerImgInfo& linfo);

vt::CRect 
CompositeRectToImgRect(const vt::CRect& rctComp, const vt::CRect& rctLayer,
					   int iCompW, int iCompH);


//+-----------------------------------------------------------------------------
//
// function: GetContiguousRegion
//
// synopsis: Returns the specified region within a layer.  This function handles
//           several special cases:
//           1) converts to the output type if it is different from the source
//           2) if the region spans a 360 seam then creates a new image of size
//              region and copies in the appropriate pixels from the layer
//           3) if the region cuts a layer in two (this happens when a layer
//              wraps around 360 and the region contains a portion of the layer
//              on the left and right sides of the region) In this case again
//              a new region image is created and the appropriate pixels are 
//              copied into it.
//
//------------------------------------------------------------------------------
HRESULT
GetContiguousRegion(IImageReader& source, 
       			    const vt::CRect& rctRegion, OUT CImg& imgRegion,
					const Byte* pFill = NULL);

};
