//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Interfaces to access portions of an image or image list
//
//  History:
//      2007/06/1-mattu
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vt_atltypes.h"

namespace vt {

//+-----------------------------------------------------------------------------
//
// Struct: BLOCKITER_INIT
// 
// Synopsis: initializer for specifying a region and step size in the
//           CBlockIterator
//
//------------------------------------------------------------------------------
struct BLOCKITER_INIT
{
	vt::CRect region;
    int iBlockWidth;
	int iBlockHeight;

    BLOCKITER_INIT() : region(0,0,0,0)
    {}

	BLOCKITER_INIT(const vt::CRect& regionI, int iBlockWidthI=256, int iBlockHeightI=256) :
	    region(regionI), iBlockWidth(iBlockWidthI), iBlockHeight(iBlockHeightI)  
	{} 
};

//+-----------------------------------------------------------------------------
//
// Class: CBlockIterator
// 
// Synopsis: simple class to step block by block across a specified rectangular
//           region
//
//------------------------------------------------------------------------------
class CBlockIterator
{
public:
    CBlockIterator() : m_bDone(true)
    {}

	CBlockIterator(const BLOCKITER_INIT& init) : m_init(init), m_ptCur(0,0)
	{ m_bDone = m_init.region.IsRectEmpty()? true: false; }

    CBlockIterator(const vt::CRect& rctRegion, UInt32 uBlockSize) :
		m_init(rctRegion, uBlockSize, uBlockSize), m_ptCur(0,0)
	{ m_bDone = m_init.region.IsRectEmpty()? true: false; }

	int BlockSizeX() const
	{ return m_init.iBlockWidth; }

	int BlockSizeY() const
	{ return m_init.iBlockHeight; }

	vt::CRect GetRect() const
	{ 
		return vt::CRect(m_ptCur.x, m_ptCur.y,
						 VtMin(((int)m_ptCur.x + m_init.iBlockWidth), 
							   m_init.region.Width()),
						 VtMin(((int)m_ptCur.y + m_init.iBlockHeight), 
							   m_init.region.Height()));
	}

	vt::CRect GetCompRect() const
    { 
		vt::CRect r = GetRect();
		r.OffsetRect(m_init.region.TopLeft());
		return r;
	}

	bool Advance()
	{
		if( !m_bDone )
		{
			m_ptCur.x += m_init.iBlockWidth;
			if(	m_ptCur.x >= m_init.region.Width() )
			{
				m_ptCur.y += m_init.iBlockHeight;
				if(	m_ptCur.y >= m_init.region.Height() )
				{   
					m_bDone = true; 
				}
				else
				{
					m_ptCur.x = 0;
				}
			}
		}
		return !m_bDone;
	}

	bool Done() const      
	{ return m_bDone; }

protected:
	BLOCKITER_INIT m_init;
	vt::CPoint m_ptCur;
	bool       m_bDone;
};

//+-----------------------------------------------------------------------------
//
// Class: CBlockIndex
// 
// Synopsis: a class to lookup blocks via index
//
//------------------------------------------------------------------------------
class CBlockIndex
{
public:
    CBlockIndex() : m_uBlocksX(0), m_uBlocksY(0), m_uBlockCount(0)
	{} 

	CBlockIndex(const BLOCKITER_INIT& init) : m_init(init)
	{ ComputeBlockCounts(); }

    CBlockIndex(const vt::CRect& rctRegion, UInt32 uBlockSize) :
		m_init(rctRegion, uBlockSize, uBlockSize)
	{ ComputeBlockCounts(); }

	void Initialize(const BLOCKITER_INIT& init)
	{
		m_init = init;
		ComputeBlockCounts();
    }

	UInt32 BlockSizeX() const
	{ return m_init.iBlockWidth; }

	UInt32 BlockSizeY() const
	{ return m_init.iBlockHeight; }

	UInt32 GetBlockCount() const
	{ return m_uBlockCount;	}

	vt::CRect GetRect(UInt32 uBlockIndex) const
	{
		VT_ASSERT( uBlockIndex < m_uBlockCount );
		UInt32 uRow = uBlockIndex / m_uBlocksX;
		UInt32 uCol = uBlockIndex - uRow*m_uBlocksX;
		CRect r;
		r.left = uCol*m_init.iBlockWidth; 
		r.top  = uRow*m_init.iBlockHeight;
		r.right  = VtMin(int(r.left+m_init.iBlockWidth), 
                         m_init.region.Width());
		r.bottom = VtMin(int(r.top+m_init.iBlockHeight),
                         m_init.region.Height());
        return r;
	}

	vt::CRect GetCompRect(const vt::CRect& rct) const
    { 
		vt::CRect r = rct;
		r.OffsetRect(m_init.region.TopLeft());
		return r;
	}

protected:
	void ComputeBlockCounts()
	{
		m_uBlocksX    = (m_init.region.Width()  + 
                         m_init.iBlockWidth-1) / m_init.iBlockWidth;
		m_uBlocksY    = (m_init.region.Height() + 
                         m_init.iBlockHeight-1) / m_init.iBlockHeight;
		m_uBlockCount = m_uBlocksX*m_uBlocksY; 
	}

protected:
	UInt32 m_uBlocksX;
	UInt32 m_uBlocksY;
	UInt32 m_uBlockCount;
	BLOCKITER_INIT m_init;
};

//+-----------------------------------------------------------------------------
//
// Class: CSpanIterator
// 
// Synopsis: simple class to step in size of stepsize across a span with the
//           given width
//
//------------------------------------------------------------------------------
class CSpanIterator
{
public:
    CSpanIterator(int width, int stepsize) : 
        m_iWidth(width), m_iStepSize(stepsize), m_iCurPos(0)
    { m_bDone = m_iWidth==0 || m_iStepSize==0; }

    int GetPosition() const
    { return m_iCurPos; }

    int GetCount() const
    { return VtMin(m_iWidth-m_iCurPos, m_iStepSize); }

    bool Advance()
    {
        if( !m_bDone )
        {
            m_iCurPos += m_iStepSize;
            if( m_iCurPos >= m_iWidth )
            {
                m_bDone   = true;
                m_iCurPos = m_iWidth;
            }
        }
		return !m_bDone;
    }

    bool Done() const
    { return m_bDone; }

protected:
    int      m_iWidth;
    int      m_iStepSize;
    int      m_iCurPos;
    bool     m_bDone;
};

//+-----------------------------------------------------------------------------
//
// class: CTypedBuffer1, CTypedBuffer2, CTypedBuffer3, CTypedBuffer4
// 
// synopsis: a set of helper classes to store spans of pixel data on the stack 
// 
//------------------------------------------------------------------------------
template<typename T1, int bytecount=1024, int align=64>
class CTypedBuffer1
{
public:
    CTypedBuffer1(int bands1)
    { 
        int offsets, sizes;
        sizes = bands1;
        AllocBuf1(&offsets, &sizes, 1);
    }

    int Capacity() const
    { return m_iCapacity; }

    T1* Buffer1()
    { return m_pBuf1; }

    const T1* Buffer1() const
    { return m_pBuf1; }

protected:
	CTypedBuffer1()
	{}

    void AllocBuf1(int* offsets, int* sizes, int bufcount)
    {
        sizes[0] *= sizeof(T1);
        int totsize = 0;
        for( int i = 0; i < bufcount; i++ )
        {
            totsize += sizes[i];
        }

        m_pBuf1 = (T1*)(((INT_PTR(m_buf) + align-1) / align) * align);
        for( m_iCapacity = bytecount / totsize; m_iCapacity > 0; m_iCapacity-- )
        {
            int prevoffset = 0;
            for( int i = 0; i < bufcount; i++ )
            {
                offsets[i] = ((prevoffset + align-1) / align ) * align;
                prevoffset = offsets[i]  + m_iCapacity*sizes[i]; 
            }
            if( ((Byte*)m_pBuf1)+prevoffset <= m_buf+bytecount )
            {
                break;
            }
        }
    }

protected:
    int  m_iCapacity;
    T1*  m_pBuf1;

private:
    Byte m_buf[bytecount];
};

template<typename T1, typename T2, int bytecount=2048, int align=64>
class CTypedBuffer2: public CTypedBuffer1<T1, bytecount, align>
{
public:
    CTypedBuffer2(int bands1, int bands2)
    {
        int offsets[2], sizes[2];
        sizes[0] = bands1, sizes[1] = bands2;
        AllocBuf2(offsets, sizes, 2);
    }

    T2* Buffer2()
    { return m_pBuf2; }

    const T2* Buffer2() const
    { return (T2*)m_pBuf2; }

protected:
	CTypedBuffer2()
	{}

    void AllocBuf2(int* offsets, int* sizes, int bufcount)
    {
        sizes[1] *= sizeof(T2);
		typedef CTypedBuffer1<T1, bytecount, align> BT;
        BT::AllocBuf1(offsets, sizes, bufcount);
        m_pBuf2 = (T2*)(((Byte*)BT::m_pBuf1)+offsets[1]);
    }
protected:
    T2* m_pBuf2;
};

template<typename T1, typename T2, typename T3, int bytecount=3072,  int align=64>
class CTypedBuffer3: public CTypedBuffer2<T1, T2, bytecount, align>
{
public:
    CTypedBuffer3(int bands1, int bands2, int bands3)
    {
        int offsets[3], sizes[3];
        sizes[0] = bands1, sizes[1] = bands2, sizes[2] = bands3;
        AllocBuf3(offsets, sizes, 3);
    }

    T3* Buffer3()
    { return m_pBuf3; }

    const T3* Buffer3() const
    { return (T3*)m_pBuf3; }

protected:
	CTypedBuffer3()
	{}

    void AllocBuf3(int* offsets, int* sizes, int bufcount)
    {
        sizes[2] *= sizeof(T3);
		typedef CTypedBuffer2<T1, T2, bytecount, align> BT;
        BT::AllocBuf2(offsets, sizes, bufcount);
        m_pBuf3 = (T3*)(((Byte*)BT::m_pBuf1)+offsets[2]);
    }
protected:
    T3* m_pBuf3;
};

template<typename T1, typename T2, typename T3, typename T4, int bytecount=4096, int align=64>
class CTypedBuffer4: public CTypedBuffer3<T1, T2, T3, bytecount, align>
{
public:
    CTypedBuffer4(int bands1, int bands2, int bands3, int bands4)
    {
        int offsets[4], sizes[4];
        sizes[0] = bands1, sizes[1] = bands2, sizes[2] = bands3, sizes[3] = bands4;
        AllocBuf4(offsets, sizes, 4);
    }

    T4* Buffer4()
    { return m_pBuf4; }

    const T4* Buffer4() const
    { return (T4*)m_pBuf4; }

protected:
    void AllocBuf4(int* offsets, int* sizes, int bufcount)
    {
        sizes[3] *= sizeof(T4);
        typedef CTypedBuffer3<T1, T2, T3, bytecount, align> BT;
		BT::AllocBuf3(offsets, sizes, bufcount);
        m_pBuf4 = (T4*)(((Byte*)BT::m_pBuf1)+offsets[3]);
    }
protected:
    T4* m_pBuf4;
};

};
