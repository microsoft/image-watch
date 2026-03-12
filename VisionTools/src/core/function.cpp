//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for image functions
//
//  History:
//      2011/04/26-wkienzle
//          Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_function.h"
#include "vt_image.h"

using namespace vt;

//+-----------------------------------------------------------------------
//
// Initialization and error checking helpers
//
//------------------------------------------------------------------------
HRESULT vt::PrepareUnaryImgOp(const CImg &imgSrc1, CImg &imgDst)
{
	VT_HR_BEGIN();

	if (!imgSrc1.IsValid())
		VT_HR_EXIT(E_INVALIDSRC);

    HRESULT createHR = CreateImageForTransform(imgDst, imgSrc1.Width(), 
		imgSrc1.Height(), imgSrc1.GetType());

	VT_HR_EXIT(createHR == E_INVALIDARG ? E_INVALIDDST : createHR);

	VT_HR_END();
}

HRESULT vt::PrepareBinaryImgOp(const CImg &imgSrc1, const CImg &imgSrc2, 
	CImg &imgDst)
{
	VT_HR_BEGIN();

	if (!imgSrc1.IsValid() || !imgSrc2.IsValid())
		VT_HR_EXIT(E_INVALIDSRC);

	if (EL_FORMAT(imgSrc1.GetType()) != EL_FORMAT(imgSrc2.GetType()) ||
		imgSrc1.Bands() != imgSrc2.Bands() ||
		imgSrc1.Width() != imgSrc2.Width() ||
		imgSrc1.Height() != imgSrc2.Height())
	{
		VT_HR_EXIT(E_INVALIDSRC);
	}

    HRESULT createHR = CreateImageForTransform(imgDst, imgSrc1.Width(), 
		imgSrc1.Height(), imgSrc1.GetType());

	VT_HR_EXIT(createHR == E_INVALIDARG ? E_INVALIDDST : createHR);

	VT_HR_END();
}

////////////////////////////////////////////////////////////////////////////////
/// Helpers

bool vt::IsColorImage(const CImg& img)
{
	return PIX_FORMAT(img.GetType()) == PIX_FORMAT_RGB   ||
		   PIX_FORMAT(img.GetType()) == PIX_FORMAT_RGBA  ||
		   (PIX_FORMAT(img.GetType()) == PIX_FORMAT_NONE && 
			(img.Bands() == 3 || img.Bands() == 4));
}

HRESULT vt::InitDst(CImg& cDst, int w, int h, int type)
{
	HRESULT createHR = CreateImageForTransform(cDst, w, h, type);
	return createHR == E_INVALIDARG ? E_INVALIDDST : createHR;
}

HRESULT vt::InitDst(CImg& cDst, const CImg& cSrc)
{
	return InitDst(cDst, cSrc.Width(), cSrc.Height(), cSrc.GetType());
}

HRESULT vt::InitDstColor(CImg& cDst, const CImg& cSrc)
{
	VT_HR_BEGIN();

	const int defaultDstType = VT_IMG_MAKE_COMP_TYPE(
		(cSrc.Bands() == 3)? PIX_FORMAT_RGB : PIX_FORMAT_RGBA,
		EL_FORMAT(cSrc.GetType()), cSrc.Bands());

	VT_HR_EXIT(InitDst(cDst, cSrc.Width(), cSrc.Height(), defaultDstType));
	
	if (!IsColorImage(cDst))
		VT_HR_EXIT(E_INVALIDDST);

	VT_HR_END();
}