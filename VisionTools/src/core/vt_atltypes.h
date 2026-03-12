//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Classes for point rect and size, copied from MFC
//
//  History:
//      2003/11/12-swinder
//			Created
//
//------------------------------------------------------------------------

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#pragma once

#include "vtcommon.h"

namespace vt {

/////////////////////////////////////////////////////////////////////////////
// Classes declared in this file

class CSize;
class CPoint;
class CRect;

/////////////////////////////////////////////////////////////////////////////
// CSize - An extent, similar to Windows SIZE structure.

class CSize : public tagSIZE
{
public:

// Constructors
	// construct an uninitialized size
	CSize();
	// create from two integers
	CSize(int initCX, int initCY);
	// create from another size
	CSize(SIZE initSize);
	// create from a point
	CSize(POINT initPt);

// Operations
	bool operator==(SIZE size) const ;
	bool operator!=(SIZE size) const ;
	void operator+=(SIZE size);
	void operator-=(SIZE size);
	void SetSize(int CX, int CY);

// Operators returning CSize values
	CSize operator+(SIZE size) const ;
	CSize operator-(SIZE size) const ;
	CSize operator-() const ;

// Operators returning CPoint values
	CPoint operator+(POINT point) const ;
	CPoint operator-(POINT point) const ;

// Operators returning CRect values
	CRect operator+(const RECT* lpRect) const ;
	CRect operator-(const RECT* lpRect) const ;
};

/////////////////////////////////////////////////////////////////////////////
// CPoint - A 2-D point, similar to Windows POINT structure.

class CPoint : public tagPOINT
{
public:
// Constructors

	// create an uninitialized point
	CPoint();
	// create from two integers
	CPoint(int initX, int initY);
	// create from another point
	CPoint(POINT initPt);
	// create from a size
	CPoint(SIZE initSize);

// Operations

// translate the point
	void Offset(int xOffset, int yOffset);
	void Offset(POINT point);
	void Offset(SIZE size);
	void SetPoint(int X, int Y);

	bool operator==(POINT point) const ;
	bool operator!=(POINT point) const ;
	void operator+=(SIZE size);
	void operator-=(SIZE size);
	void operator+=(POINT point);
	void operator-=(POINT point);

// Operators returning CPoint values
	CPoint operator+(SIZE size) const ;
	CPoint operator-(SIZE size) const ;
	CPoint operator-() const ;
	CPoint operator+(POINT point) const ;

// Operators returning CSize values
	CSize operator-(POINT point) const ;

// Operators returning CRect values
	CRect operator+(const RECT* lpRect) const ;
	CRect operator-(const RECT* lpRect) const ;
};

/////////////////////////////////////////////////////////////////////////////
// CRect - A 2-D rectangle, similar to Windows RECT structure.

class CRect : public tagRECT
{
// Constructors
public:
	// uninitialized rectangle
	CRect();
	// from left, top, right, and bottom
	CRect(int l, int t, int r, int b);
	// copy constructor
	CRect(const RECT& srcRect);
	// from a pointer to another rect
	CRect(const RECT* lpSrcRect);
	// from a point and size
	CRect(POINT point, SIZE size);
	// from two points
	CRect(POINT topLeft, POINT bottomRight);

// Attributes (in addition to RECT members)

	// retrieves the width
	int Width() const ;
	// returns the height
	int Height() const ;
	// returns the size
	CSize Size() const ;
	// const reference to the top-left point
	CPoint TopLeft() const ;
	// const reference to the bottom-right point
	CPoint BottomRight() const ;
	CPoint TopRight() const ;
	CPoint BottomLeft() const ;
	// the geometric center point of the rectangle
	CPoint CenterPoint() const ;

	// convert between CRect and LPRECT/const RECT* (no need for &)
	operator const RECT*() const ;

	// returns true if rectangle has no area
	bool IsRectEmpty() const ;
	// returns true if rectangle is at (0,0) and has no area
	bool IsRectNull() const ;
	// returns true if point is within rectangle
	bool PtInRect(POINT point) const ;
	// returns true if rectangle is within rectangle
	bool RectInRect(const RECT* lpRect) const ;

// Operations

	// set rectangle from left, top, right, and bottom
	void SetRect(int x1, int y1, int x2, int y2);
	void SetRect(POINT topLeft, POINT bottomRight);
	// empty the rectangle
	void SetRectEmpty();
	// copy from another rectangle
	void CopyRect(const RECT* lpSrcRect);
	// true if exactly the same as another rectangle
	bool EqualRect(const RECT* lpRect) const ;

	// Inflate rectangle's width and height by
	// x units to the left and right ends of the rectangle
	// and y units to the top and bottom.
	void InflateRect(int x, int y);
	// Inflate rectangle's width and height by
	// size.cx units to the left and right ends of the rectangle
	// and size.cy units to the top and bottom.
	void InflateRect(SIZE size);
	// Inflate rectangle's width and height by moving individual sides.
	// Left side is moved to the left, right side is moved to the right,
	// top is moved up and bottom is moved down.
	void InflateRect(const RECT* lpRect);
	void InflateRect(int l, int t, int r, int b);

	// deflate the rectangle's width and height without
	// moving its top or left
	void DeflateRect(int x, int y);
	void DeflateRect(SIZE size);
	void DeflateRect(const RECT* lpRect);
	void DeflateRect(int l, int t, int r, int b);

	// translate the rectangle by moving its top and left
	void OffsetRect(int x, int y);
	void OffsetRect(SIZE size);
	void OffsetRect(POINT point);
	void NormalizeRect();

	// absolute position of rectangle
	void MoveToY(int y);
	void MoveToX(int x);
	void MoveToXY(int x, int y);
	void MoveToXY(POINT point);

	// set this rectangle to intersection of two others
	bool IntersectRect(const RECT* lpRect1, const RECT* lpRect2);

	// set this rectangle to bounding union of two others
	bool UnionRect(const RECT* lpRect1, const RECT* lpRect2);

	// set this rectangle to minimum of two others
	bool SubtractRect(const RECT* lpRectSrc1, const RECT* lpRectSrc2);

// Additional Operations
	void operator=(const RECT& srcRect);
	bool operator==(const RECT& rect) const ;
	bool operator!=(const RECT& rect) const ;
	void operator+=(POINT point);
	void operator+=(SIZE size);
	void operator+=(const RECT* lpRect);
	void operator-=(POINT point);
	void operator-=(SIZE size);
	void operator-=(const RECT* lpRect);
	void operator&=(const RECT& rect);
	void operator|=(const RECT& rect);

// Operators returning CRect values
	CRect operator+(POINT point) const ;
	CRect operator-(POINT point) const ;
	CRect operator+(const RECT* lpRect) const ;
	CRect operator+(SIZE size) const ;
	CRect operator-(SIZE size) const ;
	CRect operator-(const RECT* lpRect) const ;
	CRect operator&(const RECT& rect2) const ;
	CRect operator|(const RECT& rect2) const ;
	CRect MulDiv(int nMultiplier, int nDivisor) const ;
};

// Implementation

// CSize
inline CSize::CSize()
	{ /* random filled */ }
inline CSize::CSize(int initCX, int initCY)
	{ cx = initCX; cy = initCY; }
inline CSize::CSize(SIZE initSize)
	{ *(SIZE*)this = initSize; }
inline CSize::CSize(POINT initPt)
	{ *(POINT*)this = initPt; }
inline bool CSize::operator==(SIZE size) const
	{ return (cx == size.cx && cy == size.cy); }
inline bool CSize::operator!=(SIZE size) const
	{ return (cx != size.cx || cy != size.cy); }
inline void CSize::operator+=(SIZE size)
	{ cx += size.cx; cy += size.cy; }
inline void CSize::operator-=(SIZE size)
	{ cx -= size.cx; cy -= size.cy; }
inline void CSize::SetSize(int CX, int CY)
	{ cx = CX; cy = CY; }	
inline CSize CSize::operator+(SIZE size) const
	{ return CSize(cx + size.cx, cy + size.cy); }
inline CSize CSize::operator-(SIZE size) const
	{ return CSize(cx - size.cx, cy - size.cy); }
inline CSize CSize::operator-() const
	{ return CSize(-cx, -cy); }
inline CPoint CSize::operator+(POINT point) const
	{ return CPoint(cx + point.x, cy + point.y); }
inline CPoint CSize::operator-(POINT point) const
	{ return CPoint(cx - point.x, cy - point.y); }
inline CRect CSize::operator+(const RECT* lpRect) const
	{ return CRect(lpRect) + *this; }
inline CRect CSize::operator-(const RECT* lpRect) const
	{ return CRect(lpRect) - *this; }

// CPoint
inline CPoint::CPoint()
	{ /* random filled */ }
inline CPoint::CPoint(int initX, int initY)
	{ x = initX; y = initY; }
inline CPoint::CPoint(POINT initPt)
	{ *(POINT*)this = initPt; }
inline CPoint::CPoint(SIZE initSize)
	{ *(SIZE*)this = initSize; }
inline void CPoint::Offset(int xOffset, int yOffset)
	{ x += xOffset; y += yOffset; }
inline void CPoint::Offset(POINT point)
	{ x += point.x; y += point.y; }
inline void CPoint::Offset(SIZE size)
	{ x += size.cx; y += size.cy; }
inline void CPoint::SetPoint(int X, int Y)
	{ x = X; y = Y; }
inline bool CPoint::operator==(POINT point) const
	{ return (x == point.x && y == point.y); }
inline bool CPoint::operator!=(POINT point) const
	{ return (x != point.x || y != point.y); }
inline void CPoint::operator+=(SIZE size)
	{ x += size.cx; y += size.cy; }
inline void CPoint::operator-=(SIZE size)
	{ x -= size.cx; y -= size.cy; }
inline void CPoint::operator+=(POINT point)
	{ x += point.x; y += point.y; }
inline void CPoint::operator-=(POINT point)
	{ x -= point.x; y -= point.y; }
inline CPoint CPoint::operator+(SIZE size) const
	{ return CPoint(x + size.cx, y + size.cy); }
inline CPoint CPoint::operator-(SIZE size) const
	{ return CPoint(x - size.cx, y - size.cy); }
inline CPoint CPoint::operator-() const
	{ return CPoint(-x, -y); }
inline CPoint CPoint::operator+(POINT point) const
	{ return CPoint(x + point.x, y + point.y); }
inline CSize CPoint::operator-(POINT point) const
	{ return CSize(x - point.x, y - point.y); }
inline CRect CPoint::operator+(const RECT* lpRect) const
	{ return CRect(lpRect) + *this; }
inline CRect CPoint::operator-(const RECT* lpRect) const
	{ return CRect(lpRect) - *this; }

// CRect
inline CRect::CRect()
	{ /* random filled */ }
inline CRect::CRect(int l, int t, int r, int b)
	{ left = l; top = t; right = r; bottom = b; }
inline CRect::CRect(const RECT& srcRect)
	{ left = srcRect.left; top = srcRect.top; 
	right = srcRect.right; bottom = srcRect.bottom; }
inline CRect::CRect(const RECT* lpSrcRect)
	{ left = lpSrcRect->left; top = lpSrcRect->top; 
	right = lpSrcRect->right; bottom = lpSrcRect->bottom; }
inline CRect::CRect(POINT point, SIZE size)
	{ right = (left = point.x) + size.cx; 
	bottom = (top = point.y) + size.cy; }
inline CRect::CRect(POINT topLeft, POINT bottomRight)
	{ left = topLeft.x; top = topLeft.y;
	right = bottomRight.x; bottom = bottomRight.y; }
inline int CRect::Width() const
	{ return right - left; }
inline int CRect::Height() const
	{ return bottom - top; }
inline CSize CRect::Size() const
	{ return CSize(right - left, bottom - top); }
inline CPoint CRect::TopLeft() const
	{ return CPoint(left, top); }
inline CPoint CRect::BottomRight() const
	{ return CPoint(right, bottom); }
inline CPoint CRect::TopRight() const
	{ return CPoint(right, top); }
inline CPoint CRect::BottomLeft() const
	{ return CPoint(left, bottom); }
inline CPoint CRect::CenterPoint() const
	{ return CPoint((left+right)/2, (top+bottom)/2); }
inline CRect::operator const RECT*() const
	{ return this; }
inline bool CRect::IsRectEmpty() const
	{ return left >= right || top >= bottom; }
inline bool CRect::IsRectNull() const
	{ return (left == 0 && right == 0 && top == 0 && bottom == 0); }
inline bool CRect::PtInRect(POINT point) const
	{  return point.x >= left && point.x < right && 
		point.y >= top && point.y < bottom; }
inline void CRect::SetRect(int x1, int y1, int x2, int y2)
	{ left = x1; top = y1; right = x2; bottom = y2; }
inline void CRect::SetRect(POINT topLeft, POINT bottomRight)
	{ SetRect(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y); }
inline void CRect::SetRectEmpty()
	{ left = top = right = bottom = 0; }
inline void CRect::CopyRect(const RECT* lpSrcRect)
	{ left = lpSrcRect->left; top = lpSrcRect->top; 
	right = lpSrcRect->right; bottom = lpSrcRect->bottom; }
inline bool CRect::EqualRect(const RECT* lpRect) const
	{ return left == lpRect->left && top == lpRect->top && 
		right == lpRect->right && bottom == lpRect->bottom; }
inline void CRect::InflateRect(int x, int y)
	{ left -= x; right += x; top -= y; bottom += y; }
inline void CRect::InflateRect(SIZE size)
	{ InflateRect(size.cx, size.cy); }
inline void CRect::DeflateRect(int x, int y)
	{ InflateRect(-x, -y); }
inline void CRect::DeflateRect(SIZE size)
	{ InflateRect(-size.cx, -size.cy); }
inline void CRect::OffsetRect(int x, int y)
	{ left += x; right += x; top += y; bottom += y; }
inline void CRect::OffsetRect(POINT point)
	{ OffsetRect(point.x, point.y); }
inline void CRect::OffsetRect(SIZE size)
	{ OffsetRect(size.cx, size.cy); }
inline void CRect::MoveToY(int y)
	{ bottom = Height() + y; top = y; }
inline void CRect::MoveToX(int x)
	{ right = Width() + x; left = x; }
inline void CRect::MoveToXY(int x, int y)
	{ MoveToX(x); MoveToY(y); }
inline void CRect::MoveToXY(POINT pt)
	{ MoveToX(pt.x); MoveToY(pt.y); }
	
inline bool CRect::IntersectRect(const RECT* lpRect1, const RECT* lpRect2)
	{ 
		left = vt::VtMax(lpRect1->left, lpRect2->left);
		top = vt::VtMax(lpRect1->top, lpRect2->top);
		right = vt::VtMin(lpRect1->right, lpRect2->right);
		bottom = vt::VtMin(lpRect1->bottom, lpRect2->bottom);
        bool isEmpty = IsRectEmpty();
        
        if (isEmpty)
        {
            left = right = top = bottom = 0;
        }

		return !isEmpty;
	}	

inline bool CRect::UnionRect(const RECT* lpRect1, const RECT* lpRect2)
	{
		if (!CRect(lpRect2).IsRectEmpty())
		{
			if (CRect(lpRect1).IsRectEmpty())
			{
				*this = *lpRect2;
				return true;
			}
			else
			{
				SetRect(vt::VtMin(lpRect1->left, lpRect2->left),
						vt::VtMin(lpRect1->top, lpRect2->top),
						vt::VtMax(lpRect1->right, lpRect2->right),
						vt::VtMax(lpRect1->bottom, lpRect2->bottom));
				return IsRectEmpty();
			}
		}
		else
		{
			if (CRect(lpRect1).IsRectEmpty())
			{
				SetRectEmpty();
				return true;
			}
			else
			{
				*this = *lpRect1;
				return true;
			}
		}
    }

inline void CRect::operator=(const RECT& srcRect)
	{ CopyRect(&srcRect); }
inline bool CRect::operator==(const RECT& rect) const
	{ return left == rect.left && top == rect.top && 
		right == rect.right && bottom == rect.bottom;  }
inline bool CRect::operator!=(const RECT& rect) const
	{ return !EqualRect(&rect); }
inline void CRect::operator+=(POINT point)
	{ OffsetRect(point.x, point.y); }
inline void CRect::operator+=(SIZE size)
	{ OffsetRect(size.cx, size.cy); }
inline void CRect::operator+=(const RECT* lpRect)
	{ InflateRect(lpRect); }
inline void CRect::operator-=(POINT point)
	{ OffsetRect(-point.x, -point.y); }
inline void CRect::operator-=(SIZE size)
	{ OffsetRect(-size.cx, -size.cy); }
inline void CRect::operator-=(const RECT* lpRect)
	{ DeflateRect(lpRect); }
inline void CRect::operator&=(const RECT& rect)
	{ IntersectRect(this, &rect); }
inline void CRect::operator|=(const RECT& rect)
	{ UnionRect(this, &rect); }
inline CRect CRect::operator+(POINT pt) const
	{ CRect rect(*this); rect.OffsetRect(pt.x, pt.y); return rect; }
inline CRect CRect::operator-(POINT pt) const
	{ CRect rect(*this); rect.OffsetRect(-pt.x, -pt.y); return rect; }
inline CRect CRect::operator+(SIZE size) const
	{ CRect rect(*this); rect.OffsetRect(size.cx, size.cy); return rect; }
inline CRect CRect::operator-(SIZE size) const
	{ CRect rect(*this); rect.OffsetRect(-size.cx, -size.cy); return rect; }
inline CRect CRect::operator+(const RECT* lpRect) const
	{ CRect rect(this); rect.InflateRect(lpRect); return rect; }
inline CRect CRect::operator-(const RECT* lpRect) const
	{ CRect rect(this); rect.DeflateRect(lpRect); return rect; }
inline CRect CRect::operator&(const RECT& rect2) const
	{ CRect rect; rect.IntersectRect(this, &rect2); return rect; }
inline CRect CRect::operator|(const RECT& rect2) const
	{ CRect rect; rect.UnionRect(this, &rect2); return rect; }
inline bool CRect::SubtractRect(const RECT* lpRectSrc1, const RECT* lpRectSrc2)
	{
		CRect first(lpRectSrc1);
		CRect second(lpRectSrc2);
	
		// count corners of first rect contained by second
	
		int nContainedCorners = 0;
    
		if (second.PtInRect(first.TopLeft()))
		{
			nContainedCorners++;
		}
		if (second.PtInRect(first.TopRight()))
		{
			nContainedCorners++;
		}
		if (second.PtInRect(first.BottomLeft()))
		{
			nContainedCorners++;
		}
		if (second.PtInRect(first.BottomRight()))
		{
			nContainedCorners++;
		}
 
		// subtraction result can only be rectangular if the rect 
		// contains exactly 2 of first rect's corners
		if (nContainedCorners != 2)
		{
			return false;
		}
	
		*this = first;
 
		// clip this rect to other rect
		if (left < second.left)
		{
			right = second.left;
		}
		else if (right > second.right)
		{
			left = second.right;
		}
		else if (top < second.top)
		{
			bottom = second.top;
		}
		else
		{
			VT_ASSERT(bottom > second.bottom);
			top = second.bottom;
		}
 
		return true;
	}

inline bool CRect::RectInRect(const RECT* lpRect) const
    { 
        return lpRect->left   >= left && lpRect->left   <= right  &&
               lpRect->right  >= left && lpRect->right  <= right  &&
               lpRect->top    >= top  && lpRect->top    <= bottom &&
               lpRect->bottom >= top  && lpRect->bottom <= bottom;   
    }

inline void CRect::NormalizeRect()
	{
		int nTemp;
		if (left > right)
		{
			nTemp = left;
			left = right;
			right = nTemp;
		}
		if (top > bottom)
		{
			nTemp = top;
			top = bottom;
			bottom = nTemp;
		}
	}

inline void CRect::InflateRect(const RECT* lpRect)
	{
		left -= lpRect->left;		top -= lpRect->top;
		right += lpRect->right;		bottom += lpRect->bottom;
	}

inline void CRect::InflateRect(int l, int t, int r, int b)
	{
		left -= l;			top -= t;
		right += r;			bottom += b;
	}

inline void CRect::DeflateRect(const RECT* lpRect)
	{
		left += lpRect->left;	top += lpRect->top;
		right -= lpRect->right;	bottom -= lpRect->bottom;
	}

inline void CRect::DeflateRect(int l, int t, int r, int b)
	{
		left += l;		top += t;
		right -= r;		bottom -= b;
	}

};

