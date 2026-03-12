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

#include "vt_atltypes.h"
#include "vt_addressgen.h"

using namespace vt;

//+-----------------------------------------------------------------------------
//
// function: TraverseAddressGenChain
//
//------------------------------------------------------------------------------
HRESULT
vt::TraverseAddressGenChain(IN OUT CVec2f* addrbuf, const vt::CPoint& ptDst, 
							int iWidth, IAddressGenerator** ppGenChain, 
							UInt32 uGenChainCnt)
{
	VT_HR_BEGIN();

	// ppGenChain list is ordered from dst to src
	VT_HR_EXIT( ppGenChain[0]->MapDstSpanToSrc(addrbuf, ptDst, iWidth) );

	for ( UInt32 i = 1; i < uGenChainCnt; i++ )
	{
		// in the loop below we only send spans of valid addresses to 
		// chained generators
		int iCurValidSpan = 0;
		for ( int x = 0; x < iWidth; x++ )
		{
			if ( !IsAddressOOB(addrbuf[x].x) )
			{
				iCurValidSpan++;
			}
			else
			{
				if ( iCurValidSpan )
				{
					VT_HR_EXIT( ppGenChain[i]->MapDstAddrToSrc(
						addrbuf+x-iCurValidSpan, iCurValidSpan) );
				}
				iCurValidSpan = 0;
			}
		}
		// finish of any remaining 
		if ( iCurValidSpan )
		{
			VT_HR_EXIT( ppGenChain[i]->MapDstAddrToSrc(
				addrbuf+iWidth-iCurValidSpan, iCurValidSpan) );
		}
	}

	VT_HR_END();
}

//+-----------------------------------------------------------------------------
//
// function: WrapAddressesIntoSrcBuf
//
//------------------------------------------------------------------------------
void 
vt::WrapAddressesIntoSrcBuf(CVec2f* addrbuf, int iWidth, const CRect& rctSrc,
							IAddressGenerator* pAG)
{
	int iWrapX = pAG->AddressWrapX();
    if (iWrapX != 0 && (rctSrc.left < 0 || rctSrc.right > iWrapX))
    {
	    for( int x = 0; x < iWidth; x++ )
	    {		
			if( !IsAddressOOB(addrbuf[x].x) )
			{
				if (addrbuf[x].x <  (float) rctSrc.left)
					addrbuf[x].x += (float) iWrapX;
				else if (addrbuf[x].x >= (float) rctSrc.right)
					addrbuf[x].x -= (float) iWrapX;
			}
	    }
    }

	int iWrapY = pAG->AddressWrapY();
    if (iWrapY != 0 && (rctSrc.top < 0 || rctSrc.bottom > iWrapY))
    {
		for( int x = 0; x < iWidth; x++ )
		{
			if( !IsAddressOOB(addrbuf[x].y) )
			{
				if (addrbuf[x].y <  (float) rctSrc.top)
					addrbuf[x].y += (float) iWrapY;
				else if (addrbuf[x].y >= (float) rctSrc.bottom)
					addrbuf[x].y -= (float) iWrapY;
			}
		}
    }
}

//+-----------------------------------------------------------------------------
//
// function: BoundingRectFromPoints
//  
//------------------------------------------------------------------------------
vt::CRect vt::BoundingRectFromPoints(const CVec2f* pPts, int n)
{
	float fMinX = 0.f, fMinY = 0.f, fMaxX = 0.f, fMaxY = 0.f;
	bool  bAllOOB = true;
    for( int i = 0; i < n; i++ )
    {
		if( !IsAddressOOB(pPts[i].x) )
		{
			bAllOOB = false;

			// find the first in-bounds address and init min/max
			fMinX = fMaxX = pPts[i].x; 
			fMinY = fMaxY = pPts[i].y;

			// continue with the remainder of the addresses
			for( ++i; i < n; i++ )
			{
				if( !IsAddressOOB(pPts[i].x) )
				{   
					fMinX = VtMin(fMinX, pPts[i].x);
					fMaxX = VtMax(fMaxX, pPts[i].x);
					fMinY = VtMin(fMinY, pPts[i].y);
					fMaxY = VtMax(fMaxY, pPts[i].y);
				}
			}
		}
	}
	return bAllOOB? vt::CRect(0,0,0,0): 
		            vt::CRect(int(floor(fMinX)),  int(floor(fMinY)), 
							  int(ceil(fMaxX))+1, int(ceil(fMaxY))+1);
}

static CVec3f 
ClipLineIsect(const CVec3f* p0, const CVec3f* p1, const CVec3f& clipeq)
{
    CVec3f dir  = *p1-*p0;
    float alpha = -(*p0*clipeq) / (dir*clipeq); 
    return *p0 + alpha*dir;
}

static void 
ClipPolyToLine(CVec3f* pDst, int* pDstCnt, const CVec3f* pSrc, int srccnt, 
			   const CVec3f& lineeq)
{
    CVec3f* pAdd = pDst;
    const CVec3f* p0 = pSrc+srccnt-1;
    for( int i=0; i<srccnt; i++ )
    {
        const CVec3f* p1 = pSrc+i;
        if( *p1 * lineeq >= 0 )  // convention is dot product positive means inside
        {
            if( *p0 * lineeq < 0 )
            {
                *pAdd++ = ClipLineIsect(p0, p1, lineeq);
            }
            *pAdd++ = *p1;
        }
        else if( *p0 * lineeq >= 0 )
        {
            *pAdd++ = ClipLineIsect(p0, p1, lineeq);
        }
        p0=p1;
    }
    *pDstCnt = int(pAdd-pDst);
}

void 
vt::ClipPolyToRectHomogeneous(OUT CVec3f* pDst, OUT int* pDstCnt, 
							  OUT CVec3f* pTmp, IN  CVec3f* pSrc, int iSrcCnt,
							  const vt::CRect& rctClip)
{
	// clip each corner against provided the bounds via sutherland-hodgman
	// note: the clip poly is a rect, so each source line can only be clipped
	//       by 2 clip poly lines thus iSrcCnt source lines x 2 = max dest lines
	ClipPolyToLine(pTmp, pDstCnt, pSrc, iSrcCnt,
				   CVec3f(1.f,0,-(float)rctClip.left));
	ClipPolyToLine(pDst, pDstCnt, pTmp, *pDstCnt,
				   CVec3f(0,1.f,-(float)rctClip.top));
	ClipPolyToLine(pTmp, pDstCnt, pDst, *pDstCnt,
				   CVec3f(-1.f,0,(float)rctClip.right));
	ClipPolyToLine(pDst, pDstCnt, pTmp, *pDstCnt,
				   CVec3f(0,-1.f,(float)rctClip.bottom));
}

//+-----------------------------------------------------------------------------
//
// Class: CFlowFieldAddressGen
// 
//------------------------------------------------------------------------------

//
// same code as in warp.cpp except that it is specialized for CVec2f and
// calls SetAddressSpanOOB to clear
//

inline CVec2f
BilinearFilterCoord(const CVec2f& i0, const CVec2f& i1, float c0, float c1)
{ return i0*c0 + i1*c1; }

template <void (*T_FLOOR)(float*, const float*)> void
BilinearWarpSpan(_Inout_updates_(iSpan) CVec2f* pAddr, const CVec2Img& imgSrcBlk, 
				 const vt::CRect& rctSrcBlk, int iSpan, bool bOffset)
{
	int   leftB   = rctSrcBlk.left;
	float rightB  = float(imgSrcBlk.Width()-1);
	int   topB    = rctSrcBlk.top;
	float bottomB = float(imgSrcBlk.Height()-1);

	size_t stride = imgSrcBlk.StrideBytes();

	// convert addresses to pixel locations and bilinear coefficients
	for( int x = 0; x < iSpan; )
	{
		for( ;x < iSpan; x++ )
		{
			CVec2f aflr, a = pAddr[x];
			T_FLOOR(&aflr.x, &a.x);
			int u = F2I(aflr.x)-leftB;
			int v = F2I(aflr.y)-topB;
			float urf = rightB-a.x;
			float vbf = bottomB-a.y;
			int ur = *(int*)&urf;
			int vb = *(int*)&vbf;
			if( (u|v|ur|vb) & 0x80000000 )
			{
				// if all of the sign bits are not 0 then this output pixel
				// requires source pixels that weren't provided 
				break;
			}

			// compute the bilinear coef
			float xc0 = a.x-aflr.x;
			float xc1 = 1.f - xc0;
			float yc0 = a.y-aflr.y;
			float yc1 = 1.f - yc0;

			// run the filter
			const CVec2f* pr = imgSrcBlk.Ptr(u, v);
			CVec2f y0 = BilinearFilterCoord(pr[0], pr[1], xc1, xc0);

			pr = (CVec2f*)((Byte*)pr + stride);
			CVec2f y1 = BilinearFilterCoord(pr[0], pr[1], xc1, xc0);

			if( bOffset )
			{
				pAddr[x] += BilinearFilterCoord(y0, y1, yc1, yc0);
			}
			else
			{
				pAddr[x] = BilinearFilterCoord(y0, y1, yc1, yc0);
			}
		}

		int iCurSpan = 0;
		for( ;x < iSpan; x++, iCurSpan++ )
		{
			CVec2f aflr, a = pAddr[x];
			T_FLOOR(&aflr.x, &a.x);
			int u = F2I(aflr.x)-leftB;
			int v = F2I(aflr.y)-topB;
			float urf = rightB-a.x;
			float vbf = bottomB-a.y;
			int ur = *(int*)&urf;
			int vb = *(int*)&vbf;
			if( ((u|v|ur|vb) & 0x80000000) == 0 )
			{
				// opposite termination condition from above while(), 
				// thus keep accumulating OOB addresses
				break;
			}
		}
		if( iCurSpan )
		{
			SetAddressSpanOOB(pAddr+x-iCurSpan, iCurSpan);
		}
	}
}

vt::CRect
CFlowFieldAddressGen::MapDstRectToSrc(const CRect &rctDst)
{
	vt::CRect rctTest = vt::CRect(m_imgMap.Rect()) & rctDst;
	if( rctTest.IsRectEmpty() )
	{
		return vt::CRect(0,0,0,0);
	}

	CVec2f tl = m_imgMap.Pix(rctTest.left, rctTest.top);
	float l = tl.x;
	float t = tl.y;
	float r = tl.x;
	float b = tl.y;
	if( m_bOffsetMode )
	{
		l += rctTest.left;
		t += rctTest.top;
		r += rctTest.left;
		b += rctTest.bottom;
	}
	
	for( int y = rctTest.top; y < rctTest.bottom; y++ )
	{
		const CVec2f* pv = m_imgMap.Ptr(rctTest.left, y);
		if( !m_bOffsetMode )
		{
			for( int x = 0; x < rctTest.Width(); x++, pv++ )
			{   
				if( !IsAddressOOB(pv->x) )
				{
					// TODO: would be nice if VtMinMaxImg returned a banded min/max
					l = VtMin(l, pv->x);
					t = VtMin(t, pv->y);
					r = VtMax(r, pv->x);
					b = VtMax(b, pv->y);
				}
			}
		}
		else
		{
			for( int x = 0; x < rctTest.Width(); x++, pv++ )
			{   
				if( !IsAddressOOB(pv->x) )
				{
					// TODO: this needs special case SSE code
					float xt = pv->x+float(rctTest.left+x);
					float yt = pv->y+float(y);
					l = VtMin(l, xt);
					t = VtMin(t, yt);
					r = VtMax(r, xt);
					b = VtMax(b, yt);
				}
			}
		}
	}

	vt::CRect rctSrc = vt::CRect( F2I(floorf(l)), F2I(floorf(t)), 
								  F2I(ceilf(r))+1,  F2I(ceilf(b))+1 );
	if( rctSrc.IsRectEmpty() )
	{
		return vt::CRect(0,0,0,0);
	}
	return rctSrc;
}

vt::CRect
CFlowFieldAddressGen::MapSrcRectToDst(IN const CRect& /*rctSrc*/)
{
	// For now return the entire dst rectangle.  To compute this more
	// precisely we would need to traverse the entire input flow field and
	// keep track of which dst regions reference the src rect
	return m_imgMap.Rect();
}

HRESULT
CFlowFieldAddressGen::MapDstSpanToSrc(_Out_writes_(iSpan) CVec2f* pSpan, 
									  IN const vt::CPoint &ptDst, int iSpan)
{
	// determine if dst is within the source
	if( ptDst.y < 0 || ptDst.y >= m_imgMap.Height() )
	{
		SetAddressSpanOOB(pSpan, iSpan);
	}
	else
	{
		int iFirst = (ptDst.x < 0)? -ptDst.x: 0;
		if( iFirst )
		{
			SetAddressSpanOOB(pSpan, iFirst);
		}

		int iLast = iSpan;
		if( ptDst.x+iLast > m_imgMap.Width() )
		{
			int c = ptDst.x+iLast-m_imgMap.Width();
			iLast -= c;
			SetAddressSpanOOB(pSpan+iLast, c);
		}
		VtMemcpy(pSpan+iFirst, m_imgMap.Ptr(ptDst.x+iFirst, ptDst.y), 
				 (iLast-iFirst)*sizeof(*pSpan));
		if( m_bOffsetMode )
		{
			for( int i = iFirst; i < iLast; i++ )
			{
				pSpan[i] += CVec2f(float(ptDst.x)+i, float(ptDst.y));
			}
		}
	}

	return S_OK;
}

HRESULT
CFlowFieldAddressGen::MapDstAddrToSrc(_Inout_updates_(iSpan) CVec2f* pSpan, int iSpan)
{
#if (defined(_M_IX86) || defined(_M_AMD64))
	if( g_SupportSSE2() )
	{
		BilinearWarpSpan<floor2SSE>(pSpan, m_imgMap, m_imgMap.Rect(), iSpan, 
			                        m_bOffsetMode);
	}
	else
#endif
	{
		BilinearWarpSpan<floor2>(pSpan, m_imgMap, m_imgMap.Rect(), iSpan,
			                     m_bOffsetMode);
	}
	return S_OK;
}

//+-----------------------------------------------------------------------------
//
// Class: CFlowFieldXYAddressGen
// 
//------------------------------------------------------------------------------
vt::CRect
CFlowFieldXYAddressGen::MapDstRectToSrc(const CRect &rctDst)
{
	vt::CRect rctTest = vt::CRect(m_imgMapX.Rect()) & rctDst;
	if( rctTest.IsRectEmpty() )
	{
		return vt::CRect(0,0,0,0);
	}

	float l = float(m_imgMapX.Width());
	float t = float(m_imgMapX.Height());
	float r = 0;
	float b = 0;
	
	for( int y = rctTest.top; y < rctTest.bottom; y++ )
	{
		const float* pX = m_imgMapX.Ptr(rctTest.left, y);
		const float* pY = m_imgMapY.Ptr(rctTest.left, y);
		for( int x = rctTest.left; x < rctTest.right; x++, pX++, pY++ )
		{   
            float xaddr = *pX; 
            if (m_bOffsetMode) { xaddr *= m_fieldScale; xaddr += (float)x; }
            float yaddr = *pY; 
            if (m_bOffsetMode) { yaddr *= m_fieldScale; yaddr += (float)y; }
			// TODO: would be nice if VtMinMaxImg returned a banded min/max
			if( !IsAddressOOB(xaddr) )
			{
				l = VtMin(l, xaddr);
				t = VtMin(t, yaddr);
				r = VtMax(r, xaddr);
				b = VtMax(b, yaddr);
			}
		}
	}

	vt::CRect rctSrc = vt::CRect( F2I(floorf(l)), F2I(floorf(t)), 
								  F2I(ceilf(r))+1,  F2I(ceilf(b))+1 );
	if( rctSrc.IsRectEmpty() )
	{
		return vt::CRect(0,0,0,0);
	}
	return rctSrc;
}

vt::CRect
CFlowFieldXYAddressGen::MapSrcRectToDst(IN const CRect& /*rctSrc*/)
{
	// For now return the entire dst rectangle.  To compute this more
	// precisely we would need to traverse the entire input flow field and
	// keep track of which dst regions reference the src rect
	return m_imgMapX.Rect();
}

HRESULT
CFlowFieldXYAddressGen::MapDstSpanToSrc(_Out_writes_(iSpan) CVec2f* pSpan, 
										IN const vt::CPoint &ptDst, int iSpan)
{
	// determine if dst is within the source
	if( ptDst.y < 0 || ptDst.y >= m_imgMapX.Height() )
	{
		SetAddressSpanOOB(pSpan, iSpan);
	}
	else
	{
		int iFirst = (ptDst.x < 0)? -ptDst.x: 0;
		if( iFirst )
		{
			SetAddressSpanOOB(pSpan, iFirst);
		}

		int iLast = iSpan;
		if( ptDst.x+iLast > m_imgMapX.Width() )
		{
			int c = ptDst.x+iLast-m_imgMapX.Width();
			iLast -= c;
			SetAddressSpanOOB(pSpan+iLast, c);
		}

		const float* pX = m_imgMapX.Ptr(ptDst.x+iFirst, ptDst.y);
		const float* pY = m_imgMapY.Ptr(ptDst.x+iFirst, ptDst.y);
        ANALYZE_ASSUME(iLast <= iSpan);
		for( int i = iFirst; i < iLast; i++, pX++, pY++ )
		{
			pSpan[i] = CVec2f(*pX, *pY);
    		if( m_bOffsetMode )
	    	{
                pSpan[i] *= m_fieldScale;
				pSpan[i] += CVec2f(float(ptDst.x)+i, float(ptDst.y));
			}
		}
	}

	return S_OK;
}

HRESULT
CFlowFieldXYAddressGen::MapDstAddrToSrc(_Inout_updates_(iSpan) CVec2f*, int iSpan)
{
	VT_USE(iSpan);
	return E_NOTIMPL;
}

//+-----------------------------------------------------------------------------
//
// Class: C3x3TransformAddressGen
// 
//------------------------------------------------------------------------------
template <typename T>
inline bool test_dist(const CVec2<T>& v1, const CVec2<T>& v2)
{ return CVec2<T>(v1-v2).MagnitudeSq() > 0.0001*.0001*v2.MagnitudeSq(); }

template <typename T> bool
vt::IsMatrixAffine(const CMtx3x3<T>& m, const CRect& r)
{
	if( m[2][2]==0 )
	{
		return false;
	}

	// make the last element 1.
	CMtx3x3<T> mA = m / m[2][2]; 

	// test the distance of each point through a perspective transform 
	// relative to the distance through the "affine-ized" transform
	CVec3f a = mA*CVec3f(T(r.left), T(r.top), T(1));
	if( a.z==0 || test_dist(a.Dehom(), CVec2f(a.x,a.y)) ) return false;

	CVec3f b = mA*CVec3f(T(r.right), T(r.top), T(1));
	if( b.z==0 || test_dist(b.Dehom(), CVec2f(b.x,b.y)) ) return false;

	CVec3f c = mA*CVec3f(T(r.left), T(r.bottom), T(1));
	if( c.z==0 || test_dist(c.Dehom(), CVec2f(c.x,c.y)) ) return false;

	CVec3f d = mA*CVec3f(T(r.right), T(r.bottom), T(1));
	if( d.z==0 || test_dist(d.Dehom(), CVec2f(d.x,d.y)) ) return false;

	return true;
}

template <typename T> bool
vt::IsMatrixAnisoScaleTrans(const CMtx3x3<T>& m, const CRect& r)
{
	if( m[2][2]==0 )
	{
		return false;
	}

	// make the last element 1.
	CMtx3x3<T> mA = m / m[2][2]; 

	// test the distance of each point through a perspective transform 
	// relative to the distance through the AnisoScaleTrans-ized transform
	CVec3f a = mA*CVec3f(T(r.left), T(r.top), T(1));
	if( a.z==0 || test_dist(a.Dehom(), CVec2f(mA(0,0)*T(r.left)+mA(0,2), 
											  mA(1,1)*T(r.top)+mA(1,2))) ) 
		return false;

	CVec3f b = mA*CVec3f(T(r.right), T(r.top), T(1));
	if( b.z==0 || test_dist(b.Dehom(), CVec2f(mA(0,0)*T(r.right)+mA(0,2), 
											  mA(1,1)*T(r.top)+mA(1,2))) ) 
		return false;

	CVec3f c = mA*CVec3f(T(r.left), T(r.bottom), T(1));
	if( c.z==0 || test_dist(c.Dehom(), CVec2f(mA(0,0)*T(r.left)+mA(0,2), 
											  mA(1,1)*T(r.bottom)+mA(1,2))) ) 
		return false;

	CVec3f d = mA*CVec3f(T(r.right), T(r.bottom), T(1));
	if( d.z==0 || test_dist(d.Dehom(), CVec2f(mA(0,0)*T(r.right)+mA(0,2), 
											  mA(1,1)*T(r.bottom)+mA(1,2))) ) 
		return false;

	return true;
}

template bool vt::IsMatrixAffine(const CMtx3x3f&, const CRect&);  // force instantiation
template bool vt::IsMatrixAnisoScaleTrans(const CMtx3x3f&, const CRect&);



vt::CRect 
vt::MapRegion3x3(IN const CMtx3x3f& mat, const vt::CRect& rctDst,
const vt::CRect* prctSrc)
{
	int clipcnt, negzcnt;
	return vt::MapRegion3x3Ex(mat, rctDst, clipcnt, negzcnt, prctSrc);
}

vt::CRect
vt::MapRegion3x3Ex(IN const CMtx3x3f& mat, const vt::CRect& rctDst, int& clipcnt, int& negzcnt,
const vt::CRect* prctSrc)
{
    // project
	clipcnt = 4;
	negzcnt = 0;
    CVec3f tmpB[8];
    tmpB[0] = mat*CVec3f((float)rctDst.left,  (float)rctDst.top, 1.f);
    tmpB[1] = mat*CVec3f((float)rctDst.right, (float)rctDst.top, 1.f); 
    tmpB[2] = mat*CVec3f((float)rctDst.right, (float)rctDst.bottom, 1.f);
    tmpB[3] = mat*CVec3f((float)rctDst.left,  (float)rctDst.bottom, 1.f); 

	if( prctSrc )
	{
		CVec3f tmpA[8];
		ClipPolyToRectHomogeneous(tmpB, &clipcnt, tmpA, tmpB, clipcnt, *prctSrc);
		if( clipcnt < 3 )
		{
			return vt::CRect(0,0,0,0);
		}
	}

    // dehom the clipped points
	CVec2f pts[8];
    for( int i = 0; i < clipcnt; i++ )
    {
        ANALYZE_ASSUME(i < 8);
        // if we got here a set of points was succesfully clipped against the
        // provided front facing rect - so z should be positive
		if (tmpB[i].z < 0)
			negzcnt++;
		pts[i] = tmpB[i].Dehom();
    }

#ifdef _MSC_VER
	if (negzcnt > 0)
	{
		VT_CONSOLE_LOG("Front facing rect was found with negative z!\n");
	}
#endif

	// compute the bounding rectangle
	return BoundingRectFromPoints(pts, clipcnt);
}

HRESULT
C3x3TransformAddressGen::MapDstSpanToSrc(_Out_writes_(iSpan) CVec2f* pOut,
										 IN  const CPoint &ptDst, int iSpan)
{
//#define REFERENCE
#ifdef REFERENCE
	// note the incremental calc of addresses can cause some unit
	// tests to mismatch when different block sizes are used because the
	// incremental update of address gives slightly different results than 
	// the full matrix mult. so also implement a non-incremental version
	if( m_bIsAffine )
	{
		for( int x = 0; x < iSpan; x++, pOut++ )
		{
			CVec2f d(float(ptDst.x+x), float(ptDst.y));
			pOut->x = m_xfrm(0,0) * d.x + m_xfrm(0,1) * d.y + m_xfrm(0,2);
			pOut->y = m_xfrm(1,0) * d.x + m_xfrm(1,1) * d.y + m_xfrm(1,2);
		}
	}
	else
	{
        for ( int x = 0; x < iSpan; x++, pOut++ )
        {
            CVec3f v = m_xfrm * CVec3f(float(ptDst.x+x), float(ptDst.y), 1.f);
            if ( v.z <= 0 )
            {
                SetAddressOOB(*pOut);
            }
            else
            {
                pOut->x = v.x/v.z;
                pOut->y = v.y/v.z;
            }
        }
	}

#else

	if( m_bIsAffine )
	{
		// compute X&Y for this row
		float X = float(ptDst.x) * m_xfrm(0,0) + float(ptDst.y) * m_xfrm(0,1) + 
			m_xfrm(0,2);
		float Y = float(ptDst.x) * m_xfrm(1,0) + float(ptDst.y) * m_xfrm(1,1) + 
			m_xfrm(1,2);
	
		int x = 0;

#if defined(VT_SIMD_SUPPORTED)
#if defined(VT_SSE_SUPPORTED)
		if( g_SupportSSE1() )
#endif
		{
			// xmm0 = x0 y0 x1 y1
			float32x4_t xmm0 = _mm_setr_ps(X, Y, X+m_xfrm(0,0), Y+m_xfrm(1,0));
		
			// xmm2 = 2*m00 2*m10 2*m00 2*m10
			float32x4_t xmm2 = _mm_setr_ps(2.f*m_xfrm(0,0), 2.f*m_xfrm(1,0),
									  2.f*m_xfrm(0,0), 2.f*m_xfrm(1,0));
		
			for( ; x < iSpan-1; x+=2, pOut+=2 )
			{
				_mm_storeu_ps((float*)pOut, xmm0);  // store the result				
				xmm0 = _mm_add_ps(xmm0, xmm2);      // xmm0 = xmm0 + xmm2
			}
		
			// need to re-init these for the loop below that finishes
			// off the odd remaining pix
			X = SSE2_mm_extract_epf32(xmm0, 0);
			Y = SSE2_mm_extract_epf32(xmm0, 1);
		}
#endif

		for( ; x < iSpan; x++, pOut++, X+=m_xfrm(0,0), Y+=m_xfrm(1,0) )
		{
			pOut->x = X;
			pOut->y = Y;
		}
	}
	else
	{
        // compute W for this span
        float W = float(ptDst.x) * m_xfrm(2,0) + float(ptDst.y) * m_xfrm(2,1) + 
                  m_xfrm(2,2);

        // if W <= 0 for the entire span then mark it all as invalid
        if ( W <= 0 && (W + m_xfrm(2,0)*float(iSpan-1)) <= 0 )
        {
            SetAddressSpanOOB(pOut, iSpan);
            return S_OK;
        }

        // compute X&Y for this row
        float X = float(ptDst.x) * m_xfrm(0,0) + float(ptDst.y) * m_xfrm(0,1) + 
                  m_xfrm(0,2);
        float Y = float(ptDst.x) * m_xfrm(1,0) + float(ptDst.y) * m_xfrm(1,1) + 
                  m_xfrm(1,2);

        // handle any 0 crossing of W - this allows the loop below to avoid
        // check for divide by 0
        if ( m_xfrm(2,0) != 0 )
        {
            float fX0 = float(-W / m_xfrm(2,0));
            if ( fX0 >= 0 && fX0 <= float(iSpan-1) )
            {
                if ( W < 0)
                {
                    // if we are starting negative then begin section is invalid
                    int beginvalid = F2I(floorf(fX0+1.f));
                    iSpan = iSpan - beginvalid;
                    SetAddressSpanOOB(pOut, beginvalid);
                    pOut += beginvalid;
                    X += float(beginvalid)*m_xfrm(0,0); 
                    Y += float(beginvalid)*m_xfrm(1,0);
                    W += float(beginvalid)*m_xfrm(2,0);
                }
                else
                {
                    // otherwise end section is invalid
                    int beginnotvalid = F2I(ceilf(fX0)); 
                    SetAddressSpanOOB(pOut+beginnotvalid, iSpan-beginnotvalid );
                    iSpan = beginnotvalid;
                }
            }
        }

        int x = 0;

#if defined(VT_SIMD_SUPPORTED)
#if defined(VT_SSE_SUPPORTED)
		if( g_SupportSSE1() )
#endif 
        {
            // xmm0 = x0 y0 x1 y1
            float32x4_t xmm0 = _mm_setr_ps(X, Y, X+m_xfrm(0,0), Y+m_xfrm(1,0));

            // xmm1 = w0 w0 w1 w1
            float32x4_t xmm1 = _mm_setr_ps(W, W, W+m_xfrm(2,0), W+m_xfrm(2,0));     

            // xmm2 = 2*m00 2*m10 2*m00 2*m10
            float32x4_t xmm2 = _mm_setr_ps(2.f*m_xfrm(0,0), 2.f*m_xfrm(1,0),
                                      2.f*m_xfrm(0,0), 2.f*m_xfrm(1,0));

            // xmm3 = 2*m20 2*m20 2*m20 2*m20   
            float32x4_t xmm3 = _mm_set_ps1(2.f*m_xfrm(2,0));

            for ( ; x < iSpan-1; x+=2, pOut+=2 )
            {
                float32x4_t xmm4 = xmm0;                 // copy to another reg
                xmm4 = _mm_div_ps(xmm4, xmm1);      // xmm4 = xmm4 / xmm1
                _mm_storeu_ps((float*)pOut, xmm4);  // store the result

                xmm0 = _mm_add_ps(xmm0, xmm2); // xmm0 = xmm0 + xmm2
                xmm1 = _mm_add_ps(xmm1, xmm3); // xmm1 = xmm1 + xmm3
            }

            // need to re-init these for the loop below that finishes
            // off the odd remaining pix
			X = SSE2_mm_extract_epf32(xmm0, 0);
			Y = SSE2_mm_extract_epf32(xmm0, 1);
			W = SSE2_mm_extract_epf32(xmm1, 0);
        }
#endif

        for ( ; x < iSpan; x++, pOut++, X+=m_xfrm(0,0), Y+=m_xfrm(1,0), W+=m_xfrm(2,0) )
        {
            pOut->x = X/W;
            pOut->y = Y/W;
        }
    }

#endif

	return S_OK;
}

HRESULT
C3x3TransformAddressGen::MapDstAddrToSrc(_Inout_updates_(iSpan) CVec2f* pOut, int iSpan)
{
	if( m_bIsAffine )
	{
		for( int x = 0; x < iSpan; x++, pOut++ )
		{
			CVec2f d = *pOut;
			pOut->x = m_xfrm(0,0) * d.x + m_xfrm(0,1) * d.y + m_xfrm(0,2);
			pOut->y = m_xfrm(1,0) * d.x + m_xfrm(1,1) * d.y + m_xfrm(1,2);
		}
	}
	else
	{
		for( int x = 0; x < iSpan; x++, pOut++ )
		{
			CVec3f r = m_xfrm * pOut->Hom();
			if( r.z > 0 )
			{
				*pOut = r.Dehom();
			}
			else 
			{
				SetAddressOOB(*pOut);
			}
		}
	}
	return S_OK;
}

