//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for math between images
//
//  History:
//      2004/11/08-swinder
//			Created
//
//------------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_imgmath.h"
#include "vt_imgmath.inl"
#include "vt_convert.h"
#include "vt_convert.inl"

using namespace vt;

HRESULT vt::VtAddImages(CImg& cDst, const CImg& cSrc1, const CImg& cSrc2)
{
	return BinaryImgOpDD<AddOp>(cSrc1, cSrc2, cDst);
}

HRESULT vt::VtSubImages(CImg& cDst, const CImg& cSrc1, const CImg& cSrc2)
{
	return BinaryImgOpDD<SubOp>(cSrc1, cSrc2, cDst);
}

HRESULT vt::VtMulImages(CImg& cDst, const CImg& cSrc1, const CImg& cSrc2)
{
	if (PIX_FORMAT(cSrc1.GetType()) == PIX_FORMAT_COMPLEX ||
		PIX_FORMAT(cSrc2.GetType()) == PIX_FORMAT_COMPLEX)
	{
		if (PIX_FORMAT(cSrc1.GetType()) == PIX_FORMAT(cSrc2.GetType()))
			return BinaryImgOpDD<MulComplexOp>(cSrc1, cSrc2, cDst);
		else
			return E_NOTIMPL;
	}

	return BinaryImgOpDD<MulOp>(cSrc1, cSrc2, cDst);
}

HRESULT vt::VtScaleImage(CImg& imgDst, const CImg& imgSrc, float fScale)
{
	ScaleParams params(fScale);	
	return UnaryImgOpDD<ScaleOp>(imgSrc, imgDst, &params);
}

HRESULT vt::VtScaleOffsetImage(CImg& imgDst, const CImg& imgSrc, float fScale, 
	float fOffset)
{
	ScaleOffsetParams params(fScale, fOffset);
	return UnaryImgOpDD<ScaleOffsetOp>(imgSrc, imgDst, &params);
}

HRESULT vt::VtScaleColorImage(CImg& imgDst, const CImg& imgSrc, 
	const RGBAFloatPix& scale)
{
	VT_HR_BEGIN();

	if (!IsColorImage(imgSrc))
		VT_HR_EXIT(E_INVALIDSRC);

	VT_HR_EXIT(InitDstColor(imgDst, imgSrc));

	ScaleColorParams params(scale);	
	VT_HR_EXIT(UnaryImgOpDD<ScaleColorOp>(imgSrc, imgDst, &params));

	VT_HR_END();
}

HRESULT vt::VtScaleOffsetColorImage(CImg& imgDst, const CImg& imgSrc, 
	const RGBAFloatPix& scale, const RGBAFloatPix& offset)
{
	VT_HR_BEGIN();

	if (!IsColorImage(imgSrc))
		VT_HR_EXIT(E_INVALIDSRC);

	VT_HR_EXIT(InitDstColor(imgDst, imgSrc));

	ScaleOffsetColorParams params(scale, offset);	
	VT_HR_EXIT(UnaryImgOpDD<ScaleOffsetColorOp>(imgSrc, imgDst, &params));

	VT_HR_END();
}

HRESULT vt::VtMultiplyAlpha(CImg& imgDst, const CImg& imgSrc)
{
	VT_HR_BEGIN();

	if (!IsColorImage(imgSrc))
		VT_HR_EXIT(E_INVALIDSRC);

	VT_HR_EXIT(InitDstColor(imgDst, imgSrc));

	VT_HR_EXIT(UnaryImgOpDD<MultiplyAlphaOp>(imgSrc, imgDst));

	VT_HR_END();	
}

HRESULT vt::VtExpImage(CImg& imgDst, const CImg& imgSrc)
{
	return UnaryImgOpDD<ExpOp>(imgSrc, imgDst);
}

HRESULT vt::VtLogImage(CImg& imgDst, const CImg& imgSrc, float fMin)
{
	return UnaryImgOpDD<LogOp>(imgSrc, imgDst, &fMin);
}

HRESULT vt::VtBlendImages(CImg& imgDst, const CImg& imgSrc1, 
	const CImg& imgSrc2, float w1, float w2)
{
	VT_HR_BEGIN();

	BlendOpParams params(w1, w2);

	if (!(0.f <= w1 && w1 <= 1.f) ||
		!(0.f <= w2 && w2 <= 1.f) ||
		!(w1 + w2 <= 1.f + 1e-3))
	{
		VT_HR_EXIT(BinaryImgOpDD<BlendOpBaseFloatFloat>(imgSrc1, imgSrc2, 
			imgDst, &params));
	}
	else if (w1 == 1.f)
	{
		VT_HR_EXIT(VtConvertImage(imgDst, imgSrc1));
	}
	else if (w2 == 1.f)
	{
		VT_HR_EXIT(VtConvertImage(imgDst, imgSrc2));
	}
	else
	{
		VT_HR_EXIT(BinaryImgOpDD<BlendOp>(imgSrc1, imgSrc2, imgDst, &params));
	}

	VT_HR_END();
}

HRESULT vt::VtMap(CImg& imgDst, const CImg& imgSrc, float (*map)(float, void*))
{
	VT_HR_BEGIN();
	
	if (!imgSrc.IsValid())
		VT_HR_EXIT(E_INVALIDSRC);

	VT_HR_EXIT(InitDst(imgDst, imgSrc));

	CACHED_MAP m;
	VT_HR_EXIT(m.Initialize(EL_FORMAT(imgSrc.GetType()), 
		EL_FORMAT(imgDst.GetType()), map));

	VT_HR_EXIT(VtMap(imgDst, imgSrc, m));

	VT_HR_END();
}

HRESULT vt::VtMap(CImg& imgDst, const CImg& imgSrc, const CACHED_MAP& m_map)
{
	VT_HR_BEGIN();
	
	if (!imgSrc.IsValid())
		VT_HR_EXIT(E_INVALIDSRC);

	VT_HR_EXIT(m_map.m_map != nullptr ? S_OK : E_INVALIDARG);

	VT_HR_EXIT(UnaryImgOpDD<MapOp>(imgSrc, imgDst, m_map.m_map));

	VT_HR_END();
}

HRESULT vt::VtColorMap(CImg& imgDst, const CImg& imgSrc, float (*map)(float, void*))
{	
	VT_HR_BEGIN();
	
	if (!imgSrc.IsValid() || !IsColorImage(imgSrc))
		VT_HR_EXIT(E_INVALIDSRC);

	VT_HR_EXIT(InitDstColor(imgDst, imgSrc));
	
	CACHED_MAP m;
	VT_HR_EXIT(m.Initialize(EL_FORMAT(imgSrc.GetType()), 
		EL_FORMAT(imgDst.GetType()), map));

	VT_HR_EXIT(VtColorMap(imgDst, imgSrc, m));

	VT_HR_END();
}

HRESULT vt::VtColorMap(CImg& imgDst, const CImg& imgSrc, const CACHED_MAP& m_map)
{	
	VT_HR_BEGIN();
	
	if (!imgSrc.IsValid() || !IsColorImage(imgSrc))
		VT_HR_EXIT(E_INVALIDSRC);

	VT_HR_EXIT(InitDstColor(imgDst, imgSrc));
	
	VT_HR_EXIT(m_map.m_map != nullptr ? S_OK : E_INVALIDARG);

	VT_HR_EXIT(UnaryImgOpDD<MapColorOp>(imgSrc, imgDst, m_map.m_map));

	VT_HR_END();
}

////////////////////////////////////////////////////////////////////////////////
/// CACHED_MAP impl

vt::CACHED_MAP::CACHED_MAP()
	: m_map(nullptr)
{
}

HRESULT vt::CACHED_MAP::Initialize(int srcElFormat, int dstElFormat, 
	float (*f)(float, void*), void* params)
{	
	delete m_map;
	m_map = nullptr;

	return Map::Create(&m_map, srcElFormat, dstElFormat, f, params);
}

vt::CACHED_MAP::~CACHED_MAP()
{
	delete m_map;
}

////////////////////////////////////////////////////////////////////////////////

vt::CACHED_GAMMA_MAP::CACHED_GAMMA_MAP()
{
}

float vt::CACHED_GAMMA_MAP::GammaFunction(float v, void* g)
{
	VT_ASSERT(g != NULL);
	return (g != NULL) ? powf(v, *reinterpret_cast<float*>(g)) : 0.f;
}

HRESULT vt::CACHED_GAMMA_MAP::Initialize(int srcElType, int dstElType, 
										 float gamma)
{
	delete m_map;
	m_map = nullptr;

	m_gamma = gamma;

	return Map::Create(&m_map, srcElType, dstElType, GammaFunction, 
					   &this->m_gamma);
}	
