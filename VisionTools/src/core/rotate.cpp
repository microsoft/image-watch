//+-----------------------------------------------------------------------------
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
//------------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_pixeliterators.h"
#include "vt_convert.h"
#include "vt_image.h"
#include "vt_rotate.h"
#include "vt_pad.h"

using namespace vt;

//+-----------------------------------------------------------------------------
//
// Helper functions
//
//------------------------------------------------------------------------------
template<typename TS>
void RotateImage(TS* pDst0, int iDstBlkW, int iDstBlkH, const Byte* pSrc0,
    int iStride, int iStepX, int iStepY)
{
    // for each destination row copy the source column
    for (int y = 0; y < iDstBlkH; y++)
    {
        const Byte* pSrc = pSrc0;
        for (int x = 0; x < iDstBlkW; x++, pSrc += iStepX)
            pDst0[x] = *((TS *)pSrc);

        pSrc0 += iStepY;
        pDst0 = (TS *)((Byte *) pDst0 + iStride);
    }
}

template<vt::CPoint  (*pfnStart)(CPoint, CPoint, CPoint, int, int),
	     const Byte* (*pfnStep)(const Byte*, const CImg&, int)>
void RotateImage(CImg& imgDstBlk, const vt::CRect& rctDstBlk, const CImg& imgSrcBlk,
			     vt::CPoint ptSrcBlk, int iSrcW, int iSrcH)
{
	int iSrcPixSize = imgSrcBlk.PixSize();

    CTypedBuffer1<Byte> bufTmp(iSrcPixSize);

	bool bConvertNeeded = !VT_IMG_SAME_BE(imgDstBlk.GetType(),
										  imgSrcBlk.GetType());

    if (!bConvertNeeded)
    {
        // Use faster RotateImage() for most common image types. 
        vt::CPoint pt0 = pfnStart(vt::CPoint(0, 0), rctDstBlk.TopLeft(),
            ptSrcBlk, iSrcW, iSrcH);
        vt::CPoint pt1 = pfnStart(vt::CPoint(0, 1), rctDstBlk.TopLeft(),
            ptSrcBlk, iSrcW, iSrcH);
        const Byte* pSrc0 = imgSrcBlk.BytePtr(pt0);
        int iStepY = (int) (imgSrcBlk.BytePtr(pt1) - pSrc0);
        int iStepX = (int) (pfnStep(pSrc0, imgSrcBlk, 1) - pSrc0);

        Byte* pDst0 = imgDstBlk.BytePtr();
        int iStride = imgDstBlk.StrideBytes();

        int iDstBlkW = rctDstBlk.Width();
        int iDstBlkH = rctDstBlk.Height();

        switch (iSrcPixSize)
        {
        case 1:
            RotateImage((Byte *) pDst0, iDstBlkW, iDstBlkH, pSrc0, iStride, iStepX, iStepY);
            return;
        case 2:
            RotateImage((Int16 *) pDst0, iDstBlkW, iDstBlkH, pSrc0, iStride, iStepX, iStepY);
            return;
        case 3:
            RotateImage((RGBPix *) pDst0, iDstBlkW, iDstBlkH, pSrc0, iStride, iStepX, iStepY);
            return;
        case 4:
            RotateImage((Int32 *) pDst0, iDstBlkW, iDstBlkH, pSrc0, iStride, iStepX, iStepY);
            return;
        case 6:
            RotateImage((RGBShortPix *) pDst0, iDstBlkW, iDstBlkH, pSrc0, iStride, iStepX, iStepY);
            return;
        case 8:
            RotateImage((Int64 *) pDst0, iDstBlkW, iDstBlkH, pSrc0, iStride, iStepX, iStepY);
            return;
        case 12:
            RotateImage((RGBFloatPix *) pDst0, iDstBlkW, iDstBlkH, pSrc0, iStride, iStepX, iStepY);
            return;
        case 16:
            RotateImage((RGBAFloatPix *) pDst0, iDstBlkW, iDstBlkH, pSrc0, iStride, iStepX, iStepY);
            return;
        default:
            break;
        }
    }

	// for each destination row convert and copy the source column
	for( int y = 0; y < rctDstBlk.Height(); y++ )
	{
		vt::CPoint pt0 = pfnStart(vt::CPoint(0,y), rctDstBlk.TopLeft(),
								  ptSrcBlk, iSrcW, iSrcH);
		const Byte* pSrc0 = imgSrcBlk.BytePtr(pt0);
		Byte* pDst0 = imgDstBlk.BytePtr(y);

		for( CSpanIterator sp(rctDstBlk.Width(), bufTmp.Capacity()); !sp.Done();
			 sp.Advance() )
		{
			const Byte* pSrc = pSrc0;
			Byte* pConv = bConvertNeeded? bufTmp.Buffer1(): pDst0;
			for( int x = 0; x < sp.GetCount(); 
				 x++, pConv+=iSrcPixSize, pSrc=pfnStep(pSrc, imgSrcBlk, 1) )
			{
				for (int b=iSrcPixSize-1; b>=0; --b)
					pConv[b]=pSrc[b];				
			}

			// convert if necessary
			if( bConvertNeeded )
			{
				VtConvertSpan(pDst0, imgDstBlk.GetType(),
							  bufTmp.Buffer1(), imgSrcBlk.GetType(),
							  sp.GetCount()*imgSrcBlk.Bands());
			}

			// next buffer
			pSrc0  = pfnStep(pSrc0, imgSrcBlk, sp.GetCount());
			pDst0 += sp.GetCount()*imgDstBlk.PixSize();
		}
	}
}

inline vt::CPoint 
GetSourceCoordinate90(CPoint ptDst, CPoint ptDstBlk, CPoint ptSrcBlk, 
				      int /*iSrcW*/, int iSrcH)
{
	return vt::CPoint(ptDst.y + ptDstBlk.y - ptSrcBlk.x,
					  iSrcH-1 - ptDst.x - ptDstBlk.x - ptSrcBlk.y);
}

inline const Byte*
StepSourcePointer90(const Byte* p, const CImg& imgSrc, int iStep=1)
{ return imgSrc.OffsetRow(p, -1*iStep); }

inline vt::CPoint 
GetSourceCoordinate180(CPoint ptDst, CPoint ptDstBlk, CPoint ptSrcBlk, 
				      int iSrcW, int iSrcH)
{
	return vt::CPoint(iSrcW-1 - ptDst.x - ptDstBlk.x - ptSrcBlk.x,
					  iSrcH-1 - ptDst.y - ptDstBlk.y - ptSrcBlk.y);
}

inline const Byte*
StepSourcePointer180(const Byte* p, const CImg& imgSrc, int iStep=1)
{ return p - iStep*imgSrc.PixSize(); }

inline vt::CPoint 
GetSourceCoordinate270(CPoint ptDst, CPoint ptDstBlk, CPoint ptSrcBlk, 
					   int iSrcW, int /*iSrcH*/)
{
	return vt::CPoint(iSrcW-1 - ptDst.y - ptDstBlk.y - ptSrcBlk.x,
					  ptDst.x + ptDstBlk.x - ptSrcBlk.y);
}

inline const Byte*
StepSourcePointer270(const Byte* p, const CImg& imgSrc, int iStep=1)
{ return imgSrc.OffsetRow(p, iStep); }


inline vt::CPoint 
GetSourceCoordinateTranspose(CPoint ptDst, CPoint ptDstBlk, CPoint ptSrcBlk, 
							 int /*iSrcW*/, int /*iSrcH*/)
{
	return vt::CPoint(ptDst.y + ptDstBlk.y - ptSrcBlk.x, 
					  ptDst.x + ptDstBlk.x - ptSrcBlk.y);
}

inline const Byte*
StepSourcePointerTranspose(const Byte* p, const CImg& imgSrc, int iStep=1)
{ return imgSrc.OffsetRow(p, iStep); }


inline vt::CPoint 
GetSourceCoordinateTransverse(CPoint ptDst, CPoint ptDstBlk, CPoint ptSrcBlk, 
							 int iSrcW, int iSrcH)
{
	return vt::CPoint(iSrcW-1 - ptDst.y - ptDstBlk.y - ptSrcBlk.x, 
					  iSrcH-1 - ptDst.x - ptDstBlk.x - ptSrcBlk.y);
}

inline const Byte*
StepSourcePointerTransverse(const Byte* p, const CImg& imgSrc, int iStep=1)
{ return imgSrc.OffsetRow(p, -iStep); }


inline vt::CPoint 
GetSourceCoordinateHorizontal(CPoint ptDst, CPoint ptDstBlk, CPoint ptSrcBlk, 
							  int iSrcW, int /*iSrcH*/)
{
	return vt::CPoint(iSrcW-1 - ptDst.x - ptDstBlk.x - ptSrcBlk.x,
					  ptDst.y + ptDstBlk.y - ptSrcBlk.y);
}

inline const Byte*
StepSourcePointerHorizontal(const Byte* p, const CImg& imgSrc, int iStep=1)
{ return p - iStep*imgSrc.PixSize(); }

inline vt::CPoint 
GetSourceCoordinateVertical(CPoint ptDst, CPoint ptDstBlk, CPoint ptSrcBlk, 
							int /*iSrcW*/, int iSrcH)
{
	return vt::CPoint(ptDst.x + ptDstBlk.x - ptSrcBlk.x,
					  iSrcH-1 - ptDst.y - ptDstBlk.y - ptSrcBlk.y);
}

inline const Byte*
StepSourcePointerVertical(const Byte* p, const CImg& imgSrc, int iStep=1)
{ return p + iStep*imgSrc.PixSize(); }

vt::CRect
GetRequiredSrcRectFlip(const vt::CRect& rctDstBlk, int iSrcW, int iSrcH,
					   eFlipMode mode)
{
	vt::CRect rctSrc(INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN);
	
	for( int i = 0; i < 4; i++ )
	{
		vt::CPoint ptDst( (i&1)? rctDstBlk.right-1:  rctDstBlk.left,
						  (i&2)? rctDstBlk.bottom-1: rctDstBlk.top );
		vt::CPoint ptSrc;

		switch(mode)
		{
		case eFlipModeNone:
			ptSrc = ptDst;
			break;		    
		case eFlipModeRotate90:
			ptSrc = GetSourceCoordinate90(ptDst, vt::CPoint(0,0), 
										  vt::CPoint(0,0), iSrcW, iSrcH);
			break;
		case eFlipModeRotate180:   
			ptSrc = GetSourceCoordinate180(ptDst, vt::CPoint(0,0), 
										   vt::CPoint(0,0), iSrcW, iSrcH);
			break;	    
		case eFlipModeRotate270:
			ptSrc = GetSourceCoordinate270(ptDst, vt::CPoint(0,0), 
										   vt::CPoint(0,0), iSrcW, iSrcH);
			break;
		case eFlipModeHorizontal:   
			ptSrc = GetSourceCoordinateHorizontal(ptDst, vt::CPoint(0,0), 
												  vt::CPoint(0,0), iSrcW, iSrcH);
			break;	    
		case eFlipModeVertical:
			ptSrc = GetSourceCoordinateVertical(ptDst, vt::CPoint(0,0), 
												vt::CPoint(0,0), iSrcW, iSrcH);
			break;
		case eFlipModeTranspose:
			ptSrc = GetSourceCoordinateTranspose(ptDst, vt::CPoint(0,0), 
												 vt::CPoint(0,0), iSrcW, iSrcH);
			break;
		case eFlipModeTransverse:
			ptSrc = GetSourceCoordinateTransverse(ptDst, vt::CPoint(0,0), 
												 vt::CPoint(0,0), iSrcW, iSrcH);
			break;
		default:
			VT_ASSERT(0);
			break;
		}

		rctSrc.left   = VtMin(rctSrc.left,   ptSrc.x);
		rctSrc.top    = VtMin(rctSrc.top,    ptSrc.y);
		rctSrc.right  = VtMax(rctSrc.right,  ptSrc.x+1);
		rctSrc.bottom = VtMax(rctSrc.bottom, ptSrc.y+1);
	}
    return rctSrc;
}

inline eFlipMode FlipModeFromRotation( int multipleOf90 )
{
	switch( multipleOf90 & 3 )
	{
	case 0:
	    return eFlipModeNone;
		break;
	case 1:
	    return eFlipModeRotate90;
		break;   
	case 2:
	    return eFlipModeRotate180;
		break;
	case 3:
	    return eFlipModeRotate270;
		break;	   	   
	}
    return eFlipModeNone;
}

//+-----------------------------------------------------------------------------
//
// function: VtRotateImage
//
//------------------------------------------------------------------------------
HRESULT 
vt::VtRotateImage(CImg& imgDst, const CImg& imgSrc, int multipleOf90)
{ return VtFlipImage(imgDst, imgSrc, FlipModeFromRotation(multipleOf90) ); }


//+-----------------------------------------------------------------------------
//
// function: VtFlipImage
//
//------------------------------------------------------------------------------
HRESULT
vt::VtFlipImage(CImg& imgDst, const CImg& imgSrc, eFlipMode mode)
{ 
	if ( !imgSrc.IsValid() || 
		 mode < eFlipModeNone || mode > eFlipModeRotate270  )
	{
		return E_INVALIDSRC;
	}

	if( imgDst.IsSharingMemory(imgSrc) )
	{
		return E_INVALIDDST;
	}

	VT_HR_BEGIN()

	int iDstW, iDstH;
	if( mode < eFlipModeTranspose )
	{
		iDstW = imgSrc.Width(), iDstH = imgSrc.Height();
	}
	else
	{
		iDstH = imgSrc.Width(), iDstW = imgSrc.Height();
	}

	VT_HR_EXIT( CreateImageForTransform(imgDst, iDstW, iDstH, imgSrc.GetType()) );

	VT_HR_EXIT( VtIsValidConvertImagePair(imgDst, imgSrc) ? 
		S_OK : E_INVALIDDST );

	vt::CRect rctSrc = imgSrc.Rect();

	CImg imgSrcBlk, imgDstBlk;
	for( CBlockIterator bi(imgDst.Rect(), 128); !bi.Done(); bi.Advance() )
	{
		vt::CRect rctDstBlk = bi.GetCompRect();

		vt::CRect rctSrcBlk = GetRequiredSrcRectFlip(
			rctDstBlk, rctSrc.Width(), rctSrc.Height(), mode);

		if( !rctSrc.RectInRect(&rctSrcBlk) )
		{
			VT_HR_EXIT( VtCropPadImage(imgSrcBlk, rctSrcBlk, imgSrc, 
									   IMAGE_EXTEND(Zero)) );
		}
		else
		{
			imgSrc.Share(imgSrcBlk, &rctSrcBlk);
		}
        imgDst.Share(imgDstBlk, &rctDstBlk);

		switch ( mode )
		{
		case eFlipModeNone:
			VT_HR_EXIT( VtConvertImage(imgDstBlk, imgSrcBlk) );
			break;
		case eFlipModeRotate90:
			RotateImage<GetSourceCoordinate90, StepSourcePointer90>(
				imgDstBlk, rctDstBlk, imgSrcBlk, rctSrcBlk.TopLeft(), 
				rctSrc.Width(), rctSrc.Height());
			break;
		case eFlipModeRotate180:
			RotateImage<GetSourceCoordinate180, StepSourcePointer180>(
				imgDstBlk, rctDstBlk, imgSrcBlk, rctSrcBlk.TopLeft(), 
				rctSrc.Width(), rctSrc.Height());
			break;
		case eFlipModeRotate270:
			RotateImage<GetSourceCoordinate270, StepSourcePointer270>(
				imgDstBlk, rctDstBlk, imgSrcBlk, rctSrcBlk.TopLeft(), 
				rctSrc.Width(), rctSrc.Height());
			break;
		case eFlipModeHorizontal:
			RotateImage<GetSourceCoordinateHorizontal, StepSourcePointerHorizontal>(
				imgDstBlk, rctDstBlk, imgSrcBlk, rctSrcBlk.TopLeft(),
				rctSrc.Width(), rctSrc.Height());
			break;    
		case eFlipModeVertical:
			RotateImage<GetSourceCoordinateVertical, StepSourcePointerVertical>(
				imgDstBlk, rctDstBlk, imgSrcBlk, rctSrcBlk.TopLeft(),
				rctSrc.Width(), rctSrc.Height());
			break; 
		case eFlipModeTranspose:
			RotateImage<GetSourceCoordinateTranspose, StepSourcePointerTranspose>(
				imgDstBlk, rctDstBlk, imgSrcBlk, rctSrcBlk.TopLeft(),
				rctSrc.Width(), rctSrc.Height());
			break;
		case eFlipModeTransverse:
			RotateImage<GetSourceCoordinateTransverse, StepSourcePointerTransverse>(
				imgDstBlk, rctDstBlk, imgSrcBlk, rctSrcBlk.TopLeft(),
				rctSrc.Width(), rctSrc.Height());
			break;
		default:
			VT_ASSERT(0);
			break;
		}
	}

	VT_HR_END()
}

#ifndef VT_NO_XFORMS
//+-----------------------------------------------------------------------------
//
// class: CRotateTransform
//
//------------------------------------------------------------------------------

void
CRotateTransform::InitializeRotate(const RECT& rctSrc, int multipleOf90, 
				                   int dstType)
{ InitializeFlip(rctSrc, FlipModeFromRotation(multipleOf90), dstType); }

vt::CRect 
CRotateTransform::GetRequiredSrcRect(const vt::CRect& rctDst)
{ 
	return GetRequiredSrcRectFlip(rctDst - m_rect.TopLeft(), 
								  m_rect.Width(), m_rect.Height(), 
								  m_eFlipMode) + m_rect.TopLeft();
}
	
vt::CRect
CRotateTransform::GetAffectedDstRect(const vt::CRect& rctSrc)
{ 
	int w, h;
    if( m_eFlipMode < eFlipModeTranspose )
	{
	    w = m_rect.Width(), h = m_rect.Height();
	}
	else
	{
		w = m_rect.Height(), h = m_rect.Width();
	}

	return GetRequiredSrcRectFlip( rctSrc - m_rect.TopLeft(), w, h, 
								   m_eFlipMode ) + m_rect.TopLeft();
}

vt::CRect
CRotateTransform::GetResultingDstRect(const vt::CRect& rctSrc)
{ return GetAffectedDstRect(rctSrc); }

HRESULT
CRotateTransform::Transform(CImg* pimgDst, IN  const CRect& rctDst,
							const CImg& imgSrc, const CPoint& ptSrc)
{
	VT_HR_BEGIN()

	switch( m_eFlipMode )
	{
	case eFlipModeNone:
		VT_HR_EXIT( VtConvertImage(*pimgDst, imgSrc) );
		break;
	case eFlipModeRotate90:
		RotateImage<GetSourceCoordinate90, StepSourcePointer90>(
			*pimgDst, rctDst - m_rect.TopLeft(), imgSrc, 
			ptSrc - m_rect.TopLeft(), m_rect.Width(), m_rect.Height());
		break;
	case eFlipModeRotate180:
		RotateImage<GetSourceCoordinate180, StepSourcePointer180>(
			*pimgDst, rctDst - m_rect.TopLeft(), imgSrc, 
			ptSrc - m_rect.TopLeft(), m_rect.Width(), m_rect.Height());
		break;
	case eFlipModeRotate270:
		RotateImage<GetSourceCoordinate270, StepSourcePointer270>(
			*pimgDst, rctDst - m_rect.TopLeft(), imgSrc, 
			ptSrc - m_rect.TopLeft(), m_rect.Width(), m_rect.Height());
		break;
	case eFlipModeHorizontal:
		RotateImage<GetSourceCoordinateHorizontal, StepSourcePointerHorizontal>(
			*pimgDst, rctDst - m_rect.TopLeft(), imgSrc, 
			ptSrc - m_rect.TopLeft(), m_rect.Width(), m_rect.Height());
		break;    
	case eFlipModeVertical:
		RotateImage<GetSourceCoordinateVertical, StepSourcePointerVertical>(
			*pimgDst, rctDst - m_rect.TopLeft(), imgSrc,
			ptSrc - m_rect.TopLeft(), m_rect.Width(), m_rect.Height());
		break; 
	case eFlipModeTranspose:
		RotateImage<GetSourceCoordinateTranspose, StepSourcePointerTranspose>(
			*pimgDst, rctDst - m_rect.TopLeft(), imgSrc, 
			ptSrc - m_rect.TopLeft(), m_rect.Width(), m_rect.Height());
		break;
	default:
	    VT_ASSERT(0);
		break; 
	}

	VT_HR_END()
}
#endif