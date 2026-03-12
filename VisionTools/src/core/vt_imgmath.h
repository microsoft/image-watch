//+-----------------------------------------------------------------------
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
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_image.h"

#ifndef VT_NO_XFORMS
#include "vt_transform.h"
#endif

namespace vt {

#ifdef MAKE_DOXYGEN_WORK
void foo();
#endif

/// \ingroup arithmetic
/// <summary> Add two images </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc1"> First input image. </param>
/// <param name="imgSrc2"> Second input image. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc1 or imgSrc2 is invalid
///		- E_INVALIDSRC if imgSrc1 and imgSrc2 differ in size, element type, or number of bands
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes imgDst(x,y) = imgSrc1(x,y) + imgSrc2(x,y)
///		- imgDst values are clipped if output format is non-float
HRESULT VtAddImages(CImg& imgDst, const CImg& imgSrc1, const CImg& imgSrc2);

template <class T>
void VtAddSpan(T *pDst, const T *pSrc1, const T *pSrc2, int span);

/// \ingroup arithmetic
/// <summary> Subtract two images </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc1"> First input image. </param>
/// <param name="imgSrc2"> Second input image. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc1 or imgSrc2 is invalid
///		- E_INVALIDSRC if imgSrc1 and imgSrc2 differ in size, element type, or number of bands
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes imgDst(x,y) = imgSrc1(x,y) - imgSrc2(x,y)
///		- imgDst values are clipped if output format is non-float
HRESULT VtSubImages(CImg& imgDst, const CImg& imgSrc1, const CImg& imgSrc2);

template <class T>
void VtSubSpan(T *pDst, const T *pSrc1, const T *pSrc2, int span);

/// \ingroup arithmetic
/// <summary> Multiply two images </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc1"> First input image. </param>
/// <param name="imgSrc2"> Second input image. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc1 or imgSrc2 is invalid
///		- E_INVALIDSRC if imgSrc1 and imgSrc2 differ in size, element type, or number of bands
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes imgDst(x,y) = imgSrc1(x,y) * imgSrc2(x,y)
///		- imgDst values are clipped if output format is non-float
HRESULT VtMulImages(CImg& imgDst, const CImg& imgSrc1, const CImg& imgSrc2);

template <class T>
void VtMulSpan(T *pDst, const T *pSrc1, const T *pSrc2, int span);

/// \ingroup arithmetic
/// <summary> Scale pixel values </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="fScale"> Scale factor. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes imgDst(x,y) = imgSrc(x,y) * fScale
///		- imgDst values are clipped if output format is non-float
HRESULT VtScaleImage(CImg& imgDst, const CImg& imgSrc, float fScale);

template <class TO, class TI> 
void VtScaleSpan(TO* pD, const TI* pS, float v, int iSpan);

/// \ingroup arithmetic
/// <summary> Scale and offset pixel values </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="fScale"> Scale factor. </param>
/// <param name="fOffset"> Offset. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes imgDst(x,y) = imgSrc(x,y) * fScale + fOffset
///		- fOffset is in imgSrc units (e.g. 0...255 for Byte images), regardless of output format
///		- imgDst values are clipped if output format is non-float
HRESULT VtScaleOffsetImage(CImg& imgDst, const CImg& imgSrc, float fScale, 
	float fOffset);

template <class TO, class TI>
void VtScaleOffsetSpan(TO* pD, const TI *pS, float fScale, float fOffset, 
	int iW);

/// \ingroup arithmetic
/// <summary> Scale color pixel values </summary>
/// <param name="imgDst"> Output color image. </param>
/// <param name="imgSrc"> Input color image. </param>
/// <param name="scale"> RGBA scale factors. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_INVALIDSRC if imgSrc is not a color (3 or 4 channel) image
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Scales RGB(A) channels by individual factors 
///		- imgDst values are clipped if output format is non-float
HRESULT VtScaleColorImage(CImg& imgDst, const CImg& imgSrc,
	const RGBAFloatPix& scale);

template <class TO, class TI>
void VtScaleColorSpan(RGBAType<TO>* prgbD, const RGBAType<TI> *prgbS, 
	const RGBAFloatPix& scale, int iW);

/// \ingroup arithmetic
/// <summary> Scale offset color pixel values </summary>
/// <param name="imgDst"> Output color image. </param>
/// <param name="imgSrc"> Input color image. </param>
/// <param name="scale"> RGBA scale factors. </param>
/// <param name="offset"> RGBA offset. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_INVALIDSRC if imgSrc is not a color (3 or 4 channel) image
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Scales and offsets RGB(A) channels individually
///		- Offsets are in imgSrc units (e.g. 0...255 for Byte images), regardless of output format
///		- imgDst values are clipped if output format is non-float
HRESULT VtScaleOffsetColorImage(CImg& imgDst, const CImg& imgSrc, 
	const RGBAFloatPix& scale, const RGBAFloatPix& offset);

template<class TO, class TI>
void VtScaleOffsetColorSpan(RGBAType<TO>* pD, const RGBAType<TI> *pS, 
	const RGBAFloatPix& scale, const RGBAFloatPix& offset, int iW);

/// \ingroup arithmetic
/// <summary> Multiply R,G,B bands by A (alpha) band </summary>
/// <param name="imgDst"> Output color image. </param>
/// <param name="imgSrc"> Input color image. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_INVALIDSRC if imgSrc is not a color (3 or 4 channel) image
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
HRESULT VtMultiplyAlpha(CImg& imgDst, const CImg& imgSrc);

template <class T>
void VtMultiplyAlphaSpan(T* pDst, const T* pSrc, int iCnt);

template <class T>
void VtMultiplyAlphaSpan(RGBAType<T>* pDst, const RGBAType<T>* pSrc, int iCnt)
{ VtMultiplyAlphaSpan((T*)pDst, (const T*)pSrc, iCnt*4); }

/// \ingroup arithmetic
/// <summary> Blends two images </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc1"> First input image. </param>
/// <param name="imgSrc2"> Second input image. </param>
/// <param name="fWeight1"> Weight of first image. </param>
/// <param name="fWeight2"> Weight of second image. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc1 or imgSrc2 is invalid
///		- E_INVALIDSRC if imgSrc1 and imgSrc2 differ in size, element type, or number of bands
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes imgDst(x,y) = fWeight1 * imgSrc1(x,y) + fWeight2 * cSrc2(x,y)
///		- imgDst values are clipped if output format is non-float
HRESULT VtBlendImages(CImg& imgDst, const CImg& imgSrc1, const CImg& imgSrc2, 
	float fWeight1, float fWeight2);

template<typename TI, typename TO>
void VtBlendSpan(TO* pDst, const TI* pSrc1, const TI* pSrc2, float w1, 
	float w2, int numEl);

template<typename TI, typename TO>
void VtBlendSpan(TO* pDst, int numDstBands, const TI* pSrc1, const TI* pSrc2, 
	int numSrcBands, float w1, float w2, int numPix);

#ifndef VT_NO_XFORMS

////////////////////////////////////////////////////////////////////////////////

/// \ingroup pointtransforms
/// <summary> Transform for blending two images</summary>
class CBlendTransform :
    public CImageTransformPoint<CBlendTransform, false>
{
public:
    CBlendTransform(float w1, float w2) : m_w1(w1), m_w2(w2) {};

    virtual HRESULT Transform(OUT CImg* pimgDstRegion, 
                              IN  const CRect& /*rctLayerDst*/,
                              IN  CImg *const *ppimgSrcRegions,
                              IN  UINT /*uSrcCnt*/)
    {
        return VtBlendImages(*pimgDstRegion,
            *ppimgSrcRegions[0], *ppimgSrcRegions[1], m_w1, m_w2);
    }

    HRESULT Clone(ITaskState **ppState)
    {
        return CloneTaskState<CBlendTransform>(ppState,
            VT_NOTHROWNEW CBlendTransform(m_w1, m_w2));
    }

private:
    float m_w1, m_w2;
};
#endif

/// \ingroup arithmetic
/// <summary> Compute natural logarithm of pixel values </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="fMin"> Minimal value. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes imgDst(x,y) = imgSrc(x,y) > 0.0 ? log(imgSrc(x,y)) : fMin
///		- fMin is in imgSrc units (e.g. 0...255 for Byte images), regardless of output format
///		- imgDst values are clipped if output format is non-float
HRESULT VtLogImage(CImg& imgDst, const CImg& imgSrc, float fMin);

/// \ingroup arithmetic
/// <summary> Compute exponential of pixel values </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes imgDst(x,y) = exp(imgSrc(x,y))
///		- imgDst values are clipped if output format is non-float
HRESULT VtExpImage(CImg& imgDst, const CImg& imgSrc);

/// \ingroup arithmetic
/// <summary> Helper for caching function evaluations between calls to VtMap <summary>
struct CACHED_MAP
{
public:
	/// <summary> Constructor </summary>
	CACHED_MAP();

	/// <summary> Initialize cached float->float mapping. </summary>
	/// <param name=srcElFormat"> Source element format </param>
	/// <param name=dstElFormat"> Destination element format </param>
	/// <param name=func"> Map evaluation function (float->float plus a void* parameter). </param>
	/// <param name=funcParams"> Parameters to pass to the evaluation function (second argument). </param>
	/// <returns> S_OK on success, E_OUTOFMEMORY if not enough memory. </returns>
	HRESULT Initialize(int srcElFormat, int dstElFormat, 
		float (*func)(float, void*), void* funcParams = NULL);
	
	/// <summary> Destructor </summary>
	virtual ~CACHED_MAP();

public:
	struct Map;
	Map* m_map;

private:
	CACHED_MAP(const CACHED_MAP&);
	CACHED_MAP& operator=(const CACHED_MAP&);
};

/// \ingroup arithmetic
/// <summary> Derived cache helper for gamma mapping. <summary>
struct CACHED_GAMMA_MAP : public CACHED_MAP
{
	/// <summary> Constructor </summary>
	CACHED_GAMMA_MAP();

	/// <summary> Initialize with map with f(x) = powf(x, gamma). </summary>
	/// <returns> S_OK on success, E_OUTOFMEMORY if not enough memory. </returns>
	HRESULT Initialize(int srcElFormat, int dstElFormat, float gamma);

private:
	float m_gamma;

private:
	static float GammaFunction(float, void*);
};

/// \ingroup arithmetic
/// <summary> Map image elements </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="map"> Map function. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_INVALIDARG if map is not initialized
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes imgDst(x,y) = map(imgSrc(x,y))
///		- Note: this operation is logically carried out in [0..1] float format  
///		- If imgSrc is not float, it is scaled to float using standard conversion before func is applied
///		- imgDst values are clipped if output format is non-float
HRESULT VtMap(CImg& imgDst, const CImg& imgSrc, float (*map)(float, void*));

/// \ingroup arithmetic
/// <summary> Map image elements </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="map"> Map struct, see remarks. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_INVALIDARG if map is not initialized
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes imgDst(x,y) = map(imgSrc(x,y))
///		- Note: this operation is logically carried out in [0..1] float format  
///		- If imgSrc is not float, it is scaled to float using standard conversion before func is applied
///		- imgDst values are clipped if output format is non-float
HRESULT VtMap(CImg& imgDst, const CImg& imgSrc, const CACHED_MAP& map);

/// \ingroup arithmetic
/// <summary> Map image elements (ignoring alpha channel) </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="map"> Map function. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid or not a color image
///		- E_INVALIDARG if map is not initialized
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes imgDst(x,y) = map(imgSrc(x,y))
///		- Note: this operation is logically carried out in [0..1] float format  
///		- If imgSrc is not float, it is scaled to float using standard conversion before func is applied
///		- imgDst values are clipped if output format is non-float
HRESULT VtColorMap(CImg& imgDst, const CImg& imgSrc, float (*map)(float, void*));

/// \ingroup arithmetic
/// <summary> Map image elements (ignoring alpha channel) </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="map"> Map struct, see remarks. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid or not a color image
///		- E_INVALIDARG if map is not initialized
///		- E_NOTIMPL if imgSrc or imgDst format (or the combination) is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes imgDst(x,y) = map(imgSrc(x,y))
///		- Note: this operation is logically carried out in [0..1] float format  
///		- If imgSrc is not float, it is scaled to float using standard conversion before func is applied
///		- imgDst values are clipped if output format is non-float
HRESULT VtColorMap(CImg& imgDst, const CImg& imgSrc, const CACHED_MAP& map);

//+-----------------------------------------------------------------------------
//
// Functions: SRGBLinearToNonLinear and SRGBNonLinearToLinear
//
// Synopsis: apply the tone curve and inverse tone curve described in the SRGB
//           standard.
//
//------------------------------------------------------------------------------
inline float
SRGBLinearToNonLinear(float v, void*)
{
	return ( v <= 0.0031308f )?  12.92f * v: 1.055f * powf(v, 1.f/2.4f) - 0.055f;
}

inline float
SRGBNonLinearToLinear(float v, void*)
{
	return ( v <= 0.04045 )? v / 12.92f: powf((v+0.055f)/1.055f, 2.4f);
}

#ifndef VT_NO_XFORMS

////////////////////////////////////////////////////////////////////////////////

/// \ingroup pointtransforms
/// <summary> Transform for gamma mapping </summary>
class CGammaTransform : 
	public CImageTransformUnaryPoint<CGammaTransform, true>
{
private:
	static float GammaFunction(float v, void* g)
	{
		VT_ASSERT(g != NULL);
		return (g != NULL) ? powf(v, *reinterpret_cast<float*>(g)) : 0.f;
	}

public:
	/// <summary> Constructor. </summary>
	/// <param name="gamma"> Gamma value. </param>
	/// <param name="bColorMap"> If bColorMap is true, the alpha channel will 
	/// not be mapped. Otherwise, all bands are mapped.</param>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes imgDst(x,y) = pow(imgSrc(x,y), gamma)
	CGammaTransform(float gamma, bool bColorMap = false)
		: m_gamma(gamma), m_pMap(NULL), m_bColorMap(bColorMap), 
		  m_srcElT(-1), m_dstElT(-1)
	{
		m_func = GammaFunction;
		m_funcParams = &m_gamma;
	}

	/// <summary> Constructor. </summary>
	/// <param name="func"> Custom gamma map. </param>
	/// <param name="funcParams"> Parameters for custom gamma map. </param>
	/// <param name="bColorMap"> If bColorMap is true, the alpha channel will 
	/// not be mapped. Otherwise, all bands are mapped.</param>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes imgDst(x,y) = pow(imgSrc(x,y), gamma)
	CGammaTransform(float (*func)(float, void*), void* funcParams = NULL, 
					bool bColorMap = false)
		: m_func(func), m_funcParams(funcParams), m_pMap(NULL), 
		  m_bColorMap(bColorMap), m_srcElT(-1), m_dstElT(-1)
	{
	}

	/// <summary> Destructor. </summary>
	~CGammaTransform()
	{
		delete m_pMap;
	}

public:
	HRESULT Transform(CImg* pimgDst, const CRect&, const CImg& src)
	{
		int hr = E_FAIL;

		hr = Initialize(EL_FORMAT(src.GetType()), 
			EL_FORMAT(pimgDst->GetType()));
		if (FAILED(hr))
			return hr;

        if (m_bColorMap)
    		hr = VtColorMap(*pimgDst, src, *m_pMap);
        else
    		hr = VtMap(*pimgDst, src, *m_pMap);
		if (FAILED(hr))
			return hr;

		return S_OK;
	}

	virtual HRESULT GetRequiredSrcRect(OUT TRANSFORM_SOURCE_DESC* pSrcReq, 
		OUT UINT& uSrcReqCount, IN UINT, IN const CRect& rctLayerDst)
	{
		pSrcReq->bCanOverWrite = false;
		pSrcReq->rctSrc = rctLayerDst;
		pSrcReq->uSrcIndex  = 0;
		uSrcReqCount = 1;

		return S_OK;
	}

	virtual HRESULT Clone(ITaskState **ppState)
	{
		return CloneTaskState<CGammaTransform>(ppState, 
			VT_NOTHROWNEW CGammaTransform(*this));
	}

private:
	HRESULT Initialize(int srcElT, int dstElT)
	{
        if (m_srcElT == srcElT && m_dstElT == dstElT)
            return S_OK;

        delete m_pMap;
		m_pMap = NULL;

		m_pMap = VT_NOTHROWNEW CACHED_MAP();

		if (m_pMap == NULL)
			return E_OUTOFMEMORY;

		int hr = E_FAIL;			
		hr = m_pMap->Initialize(srcElT, dstElT, m_func, m_funcParams);

        m_srcElT = srcElT;
        m_dstElT = dstElT;

		return hr;
	}

private:
	CACHED_MAP* m_pMap;
	float (*m_func)(float, void*);
	float m_gamma;
	void* m_funcParams;
    bool m_bColorMap;
    int m_srcElT, m_dstElT;
};

#endif

};
