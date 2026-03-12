//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Image classes
//
//  History:
//      2004/11/08-swinder
//          Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_image.h"
#include "vt_atltypes.h"

#ifdef _MSC_VER
#include "vt_imgdbg.h"
#include "vt_params.h"
#include "vtiio.h"
#endif

using namespace vt;

#define IMGVALID_CHK_EXIT() if(!IsValid()) { hr = E_NOINIT; goto Exit; } else;
#define IMGVALID_CHK_RET()  if(!IsValid()) { return E_NOINIT; } else;

CImgDebugHook g_CImgConstructorDebugHook = NULL;
CImgDebugHook g_CImgDestructorDebugHook = NULL;

//+-----------------------------------------------------------------------
//
//  Member:    CImg::CImg
//
//------------------------------------------------------------------------

CImg::CImg(): m_pbPtr(NULL), m_iStrideBytes(0), m_pMem(NULL), m_pMetaData(NULL)
{
	CheckInvariant(m_info.type);

	if (g_CImgConstructorDebugHook != NULL)
		g_CImgConstructorDebugHook(*this);
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::~CImg
//
//------------------------------------------------------------------------

CImg::~CImg()
{
	if (g_CImgDestructorDebugHook != NULL)
		g_CImgDestructorDebugHook(*this);

	if (m_pMem)
		m_pMem->Release();

	m_pMem = NULL;

#ifdef _MSC_VER
	delete m_pMetaData;
#endif

	CheckInvariant(m_info.type);
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::CheckInvariant
//
//  Synopsis:  Asserts that image is in consistent state
//
//------------------------------------------------------------------------
void CImg::CheckInvariant(int iType) const
{
#ifdef _DEBUG
	VT_ASSERT(m_info.width   >= 0);
	VT_ASSERT(m_info.height  >= 0);
	VT_ASSERT(m_info.Bands() >= 0);

	VT_ASSERT(m_iStrideBytes >= m_info.PixSize() * m_info.width);

	if (m_info.width * m_info.height * m_info.Bands() > 0)
		VT_ASSERT(m_pbPtr != NULL);
	if (m_pMem != NULL)
		VT_ASSERT(m_pbPtr != NULL);

	if( iType & VT_IMG_FIXED_ELFRMT_MASK )
		VT_ASSERT( (m_info.type & VT_IMG_FIXED_ELFRMT_MASK) != 0 &&
		           EL_FORMAT(iType) == EL_FORMAT(m_info.type) );

	if( iType & VT_IMG_FIXED_PIXFRMT_MASK )
		VT_ASSERT( (m_info.type & VT_IMG_FIXED_PIXFRMT_MASK) != 0 &&
		           PIX_FORMAT(iType)   == PIX_FORMAT(m_info.type) &&
				   VT_IMG_BANDS(iType) == VT_IMG_BANDS(m_info.type) );

#else
	UNREFERENCED_PARAMETER(iType);
#endif
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::IsCreatableAs
//
//------------------------------------------------------------------------
bool CImg::IsCreatableAs(int iType) const
{
	if ( (m_info.type & VT_IMG_FIXED_PIXFRMT_MASK) &&
		 (Bands() != VT_IMG_BANDS(iType) || PIX_FORMAT(m_info.type) != PIX_FORMAT(iType)) )
	{
		return false;
	}

	if ( (m_info.type & VT_IMG_FIXED_ELFRMT_MASK) && 
		 (EL_FORMAT(m_info.type) != EL_FORMAT(iType)) )
	{
		return false;
	}

	return true;
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::CreateInternal
//
//------------------------------------------------------------------------
HRESULT CImg::CreateInternal(int iW, int iH, int iType, AlignMode eAlign, 
							 bool keepExistingMemoryIfImgSizeMatches)
{
	VT_HR_BEGIN(); 

	if(iW<0 || iH<0)
		VT_HR_EXIT(E_INVALIDARG);

	// if the incoming pixformat is not set and this is a fixed pixfrmt image
	// then tweak the type to match pixfrmt fields
	if( (m_info.type & VT_IMG_FIXED_PIXFRMT_MASK) &&
		PIX_FORMAT( iType ) == PIX_FORMAT_NONE )
	{
		iType &= ~VT_IMG_PIXFRMT_MASK;
		iType |= PIX_FORMAT(m_info.type);
	}

	VT_HR_EXIT(IsCreatableAs(iType) ? S_OK : E_INVALIDARG);

	int iPixSize = VT_IMG_ELSIZE(iType) * VT_IMG_BANDS(iType);

	if (!(IsValid() && keepExistingMemoryIfImgSizeMatches && 
		  iW == Width() && iH == Height() && iPixSize == PixSize()) )
	{
		int iAlignMask = 0x0; // default row alignment is without gaps
		if(eAlign==align16ByteRows)
			iAlignMask = 0xf;
		else if(eAlign==align64ByteRows)
			iAlignMask = 0x3f;

		UInt64 llRowBytes = ((UInt64)iW * (UInt64)iPixSize + iAlignMask) & 
			(~(UInt64)iAlignMask);

		UInt64 llMemSize = llRowBytes * iH;
		if( llMemSize > 0xffffffff )
		{
			VT_HR_EXIT(E_OUTOFMEMORY);
		}
		size_t uiMemSize = (size_t)llMemSize;

		// if shared memory, remove reference to it
		if(m_pMem && m_pMem->RefCount() > 1)
		{
			m_pMem->Release();
			m_pMem = NULL;
		}

		// create new memory if needed
		if(m_pMem==NULL)
		{
			m_pMem = VT_NOTHROWNEW CMemShare();
			VT_PTR_OOM_EXIT(m_pMem);
		}

		// allocate the memory
		Byte *puchData = m_pMem->Alloc(uiMemSize, eAlign);
		if(puchData==NULL)
		{
			delete m_pMem;
			m_pMem = NULL;
			VT_HR_EXIT(E_OUTOFMEMORY);
		}

		m_pbPtr = puchData;
		m_iStrideBytes = (int)llRowBytes;
	}

	m_info.width  = iW;
	m_info.height = iH;

	// keep the fixed flags from the original type (set by SetType)
	m_info.type   =
		(iType & ~(VT_IMG_FIXED_ELFRMT_MASK|VT_IMG_FIXED_PIXFRMT_MASK)) |
		(m_info.type & (VT_IMG_FIXED_ELFRMT_MASK|VT_IMG_FIXED_PIXFRMT_MASK));

	VT_HR_EXIT_LABEL();

	CheckInvariant(m_info.type);

	return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::Create (wrap version)
//
//------------------------------------------------------------------------
HRESULT CImg::Create(Byte *pbBuffer, int iW, int iH, int iStrideBytes, int iType) 
{
	VT_HR_BEGIN();

	// if the incoming pixformat is not set and this is a fixed pixfrmt image
	// then tweak the type to match pixfrmt fields
	if( (m_info.type & VT_IMG_FIXED_PIXFRMT_MASK) &&
		PIX_FORMAT( iType ) == PIX_FORMAT_NONE )
	{
		iType &= ~VT_IMG_PIXFRMT_MASK;
		iType |= PIX_FORMAT(m_info.type);
	}

	VT_HR_EXIT(IsCreatableAs(iType) ? S_OK : E_INVALIDARG);

	int iBands = VT_IMG_BANDS(iType);

	if (NULL == pbBuffer && iW * iH * iBands != 0)
		return E_INVALIDARG;

	// if shared memory, remove reference to it
	if(m_pMem)
	{
		m_pMem->Release();
		m_pMem = NULL;
	}

	m_pbPtr = pbBuffer;

	m_info.width   = iW;
	m_info.height  = iH; 
	m_iStrideBytes = iStrideBytes;

	// keep the fixed flags from the original type (set by SetType)
	m_info.type    = 
		(iType & ~(VT_IMG_FIXED_ELFRMT_MASK|VT_IMG_FIXED_PIXFRMT_MASK)) |
		(m_info.type & (VT_IMG_FIXED_ELFRMT_MASK|VT_IMG_FIXED_PIXFRMT_MASK));

	VT_HR_EXIT_LABEL();

	CheckInvariant(m_info.type);

	return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::Deallocate
//
//------------------------------------------------------------------------

void CImg::Deallocate()
{
	if(m_pMem)
	{
		m_pMem->Release();
		m_pMem = NULL;
	}

	m_pbPtr = NULL;

	m_info.width = 0;
	m_info.height = 0;
	m_iStrideBytes = 0;

	CheckInvariant(m_info.type);
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::SetMetaData
//
//------------------------------------------------------------------------

HRESULT CImg::SetMetaData(const CParams *pParams)
{
#ifdef _MSC_VER
	VT_HR_BEGIN();

	if (m_pMetaData == NULL)
		VT_HR_EXIT((m_pMetaData = VT_NOTHROWNEW CParams()) != NULL ? S_OK : E_OUTOFMEMORY);

	if (pParams == NULL)
		m_pMetaData->DeleteAll();		
	else
		m_pMetaData->Merge(pParams);

	VT_HR_EXIT_LABEL();

	CheckInvariant(m_info.type);

	return hr;
#else
	return E_NOTIMPL;
#endif
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::ClipRect
//
//------------------------------------------------------------------------

RECT CImg::ClipRect(const RECT *prct) const
{
	if(prct==NULL)
		return Rect();

	CRect rct(prct);
	rct.NormalizeRect();
	CRect rctThis = Rect();
	rctThis.IntersectRect(rctThis, rct);

	return rctThis;
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::Share
//
//------------------------------------------------------------------------
HRESULT CImg::Share(CImg &cDest, const RECT *prctSrc, bool bShareMetaData) const
{
	VT_HR_BEGIN();

	IMGVALID_CHK_EXIT();

	VT_HR_EXIT(cDest.IsCreatableAs(GetType()) ? S_OK : E_INVALIDARG);

	CRect rct;
	rct = ClipRect(prctSrc);
	if(&cDest==this || rct.IsRectEmpty())
		VT_HR_EXIT(E_INVALIDARG);

	if (cDest.m_pMem)
		cDest.m_pMem->Release();
	cDest.m_pMem = m_pMem;
	if(m_pMem)
		m_pMem->AddRef();

	cDest.m_info.width   = rct.Width();
	cDest.m_info.height  = rct.Height();
	cDest.m_info.type    = 
		(m_info.type & ~(VT_IMG_FIXED_ELFRMT_MASK|VT_IMG_FIXED_PIXFRMT_MASK)) |
		(cDest.m_info.type & (VT_IMG_FIXED_ELFRMT_MASK|VT_IMG_FIXED_PIXFRMT_MASK));
	cDest.m_iStrideBytes = StrideBytes();

	// see note at declaration of Share()
	cDest.m_pbPtr = const_cast<Byte*>(BytePtr(rct.left, rct.top));

	if (bShareMetaData)
		cDest.SetMetaData(m_pMetaData);

	VT_HR_EXIT_LABEL();

	CheckInvariant(m_info.type);
	cDest.CheckInvariant(cDest.m_info.type);

	return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::IsSharingMemory
//
//------------------------------------------------------------------------
bool CImg::IsSharingMemory(const CImg& test) const
{
	if (!IsValid() || !test.IsValid() || 
		Width()*Height() == 0 || test.Width()*test.Height() == 0)
		return false;

	const Byte* pThisStart = BytePtr(0,0);
	const Byte* pThisEnd   = BytePtr(Width()-1,Height()-1)+PixSize();
	const Byte* pTestStart = test.BytePtr(0,0);
	const Byte* pTestEnd   = 
		test.BytePtr(test.Width()-1,test.Height()-1)+test.PixSize();

	return (pThisStart<pTestStart)? pThisEnd>pTestStart: pThisStart<pTestEnd;
	// TODO: doesn't handle case where images are sharing memory but their
	//       strides are setup in such a way that their use of the memory is
	//       interleaved and non-overlapping
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::Fill
//
//------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable:22105) // no way to annotate *pbValue buffer size since it depends on a runtime variable
HRESULT CImg::Fill(const Byte *pbValue, const RECT *prct, int iBand, bool bReplicateBands)
{
	IMGVALID_CHK_RET();

	CRect rct = ClipRect(prct);

	if(rct.IsRectEmpty())
		return NOERROR;

	int i = rct.Height();
	if(i<=0)
		return NOERROR;

	if(iBand<0 || Bands()==1)
	{
		Byte *puchDst = BytePtr(rct.left, rct.top);

		// fill all bands by copying the whole band set from bpValue
		// pbValue points to array of size PixSize()
		int elsize     = bReplicateBands? ElSize(): PixSize();
		int    iFillCount = bReplicateBands? rct.Width()*Bands(): rct.Width();

		for( int y = 0; y < rct.Height(); y++, puchDst += StrideBytes() )
		{
			VtFillSpan(puchDst, pbValue, elsize, iFillCount, true);
		}
	}
	else
	{
		// just fill the one band specified with the first element in pbValue
		int iX;
		Byte *puchDst = BytePtr(rct.left, rct.top, iBand);
		Byte *puchDstRow;
		int iW = rct.Width();
		for(; i; i--, puchDst += StrideBytes())
			for(iX = 0, puchDstRow = puchDst; iX<iW; iX++, puchDstRow += PixSize())
				memcpy(puchDstRow, pbValue, ElSize());
	}

	return NOERROR;
}
#pragma warning(pop)

//+-----------------------------------------------------------------------
//
//  Member:    CImg::Clear
//
//------------------------------------------------------------------------

HRESULT CImg::Clear(const RECT *prct)
{
	IMGVALID_CHK_RET()

		CRect rct = ClipRect(prct);

	if(rct.IsRectEmpty())
		return NOERROR;

	int i = rct.Height();
	if(i<=0)
		return NOERROR;

	Byte *puchDst = BytePtr(rct.left, rct.top);
	int iCopyWidth = rct.Width() * PixSize();
	for(; i; i--, puchDst += StrideBytes())
		VtMemset(puchDst, 0, iCopyWidth, true);

	return NOERROR;
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::Paste
//
//------------------------------------------------------------------------

HRESULT CImg::Paste(int iDstX, int iDstY, const CImg &cSrc, const RECT *prct)
{
	VT_HR_BEGIN();

	IMGVALID_CHK_EXIT();

	CRect rctA, rctB, rctC, rctD, rctE;

	if(&cSrc==this || !cSrc.IsValid())
		VT_HR_EXIT(E_INVALIDARG);

	rctA = cSrc.Rect();
	rctB = cSrc.Rect();
	if(prct)
	{
		rctA = *prct;
		rctA.NormalizeRect();
		const RECT srcRect = cSrc.Rect();
		if(!rctB.IntersectRect(rctA, &srcRect))
			// provided rect contains no source pixels so clear equivalent area in destination
			return Clear(CRect(iDstX, iDstY, iDstX + rctA.Width(), iDstY + rctA.Height()));
	}
	rctC.SetRect(iDstX, iDstY, iDstX + rctA.Width(), iDstY + rctA.Height());
	const RECT thisRect = Rect();
	rctC.IntersectRect(rctC, &thisRect);

	int iTransferL = iDstX - rctA.left;
	int iTransferT = iDstY - rctA.top;

	rctD.SetRect(rctB.left + iTransferL, rctB.top + iTransferT, 
				 rctB.right + iTransferL, rctB.bottom + iTransferT); 
	if(!rctE.IntersectRect(rctD, &thisRect))
		// no actual pixels to copy so clear appropriate area in destination
		return Clear(rctC);

	int iBorderL = rctE.left - rctC.left;
	int iBorderT = rctE.top - rctC.top;
	int iBorderR = rctC.right - rctE.right;
	int iBorderB = rctC.bottom - rctE.bottom;

	int iClearWidth = rctC.Width() * PixSize();
	int iCopyWidth  = rctE.Width() * PixSize();
	int iOffsetR    = iBorderL + iCopyWidth;
	int iSrcStride  = cSrc.StrideBytes();

	const Byte *puchSrc = cSrc.BytePtr(rctE.left - iTransferL, rctE.top - iTransferT);
	Byte *puchDst = BytePtr(rctC.left, rctC.top);

	int iY;
	for(iY=0; iY<iBorderT; iY++, puchDst += StrideBytes())
		VtMemset(puchDst, 0, iClearWidth);

	for(iY = rctE.Height(); iY; iY--, puchDst += StrideBytes(), puchSrc += iSrcStride)
	{
		if(iBorderL>0)
			memset(puchDst, 0, iBorderL * PixSize());

		VtMemcpy(puchDst + iBorderL * PixSize(), puchSrc, iCopyWidth);

		if(iBorderR>0)
			memset(puchDst + iOffsetR, 0, iBorderR * PixSize());
	}

	for(iY = 0; iY<iBorderB; iY++, puchDst += StrideBytes())
		VtMemset(puchDst, 0, iClearWidth);

	VT_HR_END();
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::CopyTo
//
//------------------------------------------------------------------------

HRESULT CImg::CopyTo(CImg &cDest, const RECT *prctSrc) const
{
	VT_HR_BEGIN();

	IMGVALID_CHK_EXIT();

	CRect rctImg = Rect();
	CRect rctSrc = prctSrc? (CRect(*prctSrc) & rctImg) : rctImg;
	if(&cDest==this)
    {
        // if cDest and this are the same image then copy is only valid if
        // copying the entire image over itself, and in that case there is
        // nothing to do.
        if( rctSrc==rctImg)
        {
            return S_OK;
        }
        else
        {
            VT_HR_EXIT( E_INVALIDARG );
        }
    }

    // this is another case with no work to do - if src and dest point to the
    // same memory and all of the image dimensions match.
    if( cDest.BytePtr() == BytePtr() && 
        cDest.Width()   == Width()   &&
        cDest.Height()  == Height()  &&
        cDest.Bands()   == Bands()   &&
        EL_FORMAT(cDest.GetType()) == EL_FORMAT(GetType()) &&
        rctSrc==rctImg )
    {
        return S_OK;
    }

	VT_HR_EXIT(cDest.CreateInternal(rctSrc.Width(), rctSrc.Height(), 
		                            m_info.type, DEFAULT_ALIGN, true));

	const Byte *puchSrc = BytePtr(rctSrc.left, rctSrc.top);
	Byte *puchDst = cDest.BytePtr();
	int iCopyWidth  = VtMin(rctSrc.Width(), cDest.Width()) * PixSize();
	int iCopyHeight = VtMin(rctSrc.Height(), cDest.Height());
	int iDstStride = cDest.StrideBytes();

	int iY;
	for(iY = 0; iY<iCopyHeight; iY++, puchDst += iDstStride, puchSrc += StrideBytes())
		VtMemcpy(puchDst, puchSrc, iCopyWidth, false);

	VT_HR_END();
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::Load
//
//------------------------------------------------------------------------

HRESULT CImg::Load(const WCHAR * pszFilex)
{
#if defined(_MSC_VER)
	VT_HR_BEGIN();

    CVtiReaderWriter r;

	VT_HR_EXIT( r.OpenBlob(pszFilex, 0, true) );

    VT_HR_EXIT( r.ReadImg(0, *this) );

    VT_HR_EXIT( r.CloseBlob() );

	VT_HR_EXIT_LABEL();

	CheckInvariant(m_info.type);

	return hr;
#else
	return E_NOTIMPL;
#endif
}

//+-----------------------------------------------------------------------
//
//  Member:    CImg::Save
//
//------------------------------------------------------------------------

HRESULT CImg::Save(const WCHAR * pszFilex) const
{
#if defined(_MSC_VER)
	VT_HR_BEGIN();

    CVtiReaderWriter w;

    CLayerImgInfo info(m_info);
    vector<CLayerImgInfo> vecInfos;
    VT_HR_EXIT( vecInfos.push_back(info) );
	VT_HR_EXIT( w.CreateBlob(vecInfos, 0, pszFilex) );

    VT_HR_EXIT( w.WriteImg(0, *this) );

    VT_HR_EXIT( w.CloseBlob() );

	VT_HR_EXIT_LABEL();

	return hr;
#else
	return E_NOTIMPL;
#endif
}

//+-----------------------------------------------------------------------
//
//  Function:  VtElFormatStringFromType
//
//------------------------------------------------------------------------
const wchar_t* vt::VtElFormatStringFromType(int iType)
{
	if (iType == VT_IMG_PIXFRMT_MASK)
		return L"NOTINITIALIZED";

	switch (EL_FORMAT(iType))
	{
	case EL_FORMAT_BYTE:
		return L"BYTE";
	case EL_FORMAT_SBYTE:
		return L"SBYTE";
	case EL_FORMAT_SHORT:
		return L"SHORT";
	case EL_FORMAT_SSHORT:
		return L"SSHORT";
	case EL_FORMAT_INT:
		return L"INT";
	case EL_FORMAT_HALF_FLOAT:
		return L"HALF_FLOAT";
	case EL_FORMAT_FLOAT:
		return L"FLOAT";
	case EL_FORMAT_DOUBLE:
		return L"DOUBLE";
	default:
		return L"UNKNOWN";
	}
}

const wchar_t* vt::VtPixFormatStringFromType(int iType, bool bTrueOrder)
{
	if (iType == VT_IMG_PIXFRMT_MASK)
		return L"NOTINITIALIZED";

	switch( PIX_FORMAT(iType) )
	{
	case PIX_FORMAT_COMPLEX:
		return L"COMPLEX";
	case PIX_FORMAT_FLOW:
		return L"FLOW";
    case PIX_FORMAT_UV:
		return L"UV";
	case PIX_FORMAT_RGB:
		return bTrueOrder ? L"BGR" : L"RGB";
	case PIX_FORMAT_YUV:
		return L"YUV";
	case PIX_FORMAT_RGBA:
		return bTrueOrder ? L"BGRA" : L"RGBA";
	case PIX_FORMAT_LUMA:
		return L"LUMA";
	case PIX_FORMAT_NONE:
		return L"NONE";
	default:
		return L"UNKNOWN";
	}
}

//+-----------------------------------------------------------------------------
//
// Function: UpdateMutableTypeFields
// 
//------------------------------------------------------------------------------
int
vt::UpdateMutableTypeFields(int iType, int iUpdateType)
{
	int fix = ((iType >> (VT_IMG_FIXED_PIXFRMT_SHIFT-1)) & 2) |
		      ((iType >> VT_IMG_FIXED_ELFRMT_SHIFT)      & 1);

	switch(fix)
	{
	case 0:  // nothing is fixed
		iType = iUpdateType;
		break;
	case 1:  // only element type is fixed (pix format and bands count mutable)
		iType = VT_IMG_FIXED_ELFRMT_MASK | VT_IMG_MAKE_COMP_TYPE(
			PIX_FORMAT(iUpdateType), EL_FORMAT(iType), VT_IMG_BANDS(iUpdateType));
		break;
	case 2: // only pixel format is fixed (element format is mutable)
		iType = VT_IMG_FIXED_PIXFRMT_MASK |
			VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT(iType), EL_FORMAT(iUpdateType),
								  VT_IMG_BANDS(iType));
		break;
	case 3: // everything is fixed so leave type unchanged
		break;
	}

	return iType | (iUpdateType & (VT_IMG_FIXED_PIXFRMT_MASK|VT_IMG_FIXED_ELFRMT_MASK));
}

//+-----------------------------------------------------------------------------
//
// Function: CreateImageForTransform
// 
//------------------------------------------------------------------------------
HRESULT
vt::CreateImageForTransform(CImg& dst, int iWidth, int iHeight, int iDefType)
{
	HRESULT hr = S_OK;

	// check if the size is correct - if it is then don't call create
	// assumption here is that the caller pre-created the image with the
	// desired type
	int iType = dst.GetFullType();
	if( dst.Width() != iWidth || dst.Height() != iHeight || 
		iType == OBJ_UNDEFINED  )
	{
		hr = dst.Create(iWidth, iHeight,
						UpdateMutableTypeFields(iType, iDefType)); 
	}

	return hr;
}

CImgDebugHook vt::InstallDebugHookCImgConstructor(CImgDebugHook newHook)
{
	CImgDebugHook oldHook = g_CImgConstructorDebugHook;
	
	g_CImgConstructorDebugHook = newHook;
	
	return oldHook;
}

CImgDebugHook vt::InstallDebugHookCImgDestructor(CImgDebugHook newHook)
{
	CImgDebugHook oldHook = g_CImgDestructorDebugHook;
	
	g_CImgDestructorDebugHook = newHook;
	
	return oldHook;
}