//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Conversion between image types
//
//  History:
//      2003/11/12-swinder/mattu
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

/// \ingroup conversion
/// <summary> Convert between pixel formats using \ref stdconvrules. </summary>
/// <param name="imgDst"> Output image of desired output type. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="bBypassCache"> Bypass cache when writing to imgDst. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_INVALIDARG if imgDst shares memory with imgSrc
///		- E_NOTIMPL if conversion from imgSrc to imgDst type is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Implements all \ref stdconvrules.
///		- It is also valid to pass an arbitrary imgSrc and imgDst of identical type. 
///		In that case, VtConvertImage just copies the pixels from src to dst.
///		- bBypassCache is currently only supported for equal number of imgSrc/imgDst bands
HRESULT VtConvertImage(CImg &imgDst, const CImg &imgSrc, bool bBypassCache=false);

#if defined(MAKE_DOXYGEN_WORK) || (defined(VT_WINRT) && defined(_WIN32_WINNT_WINBLUE))
/// \ingroup conversion
/// <summary> Convert a Vision Tools image to a WinRT WriteableBitmap (for
/// consumption in a XAML application). </summary>
/// <param name="writeableBitmap"> Output WriteableBitmap. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="bBypassCache"> Bypass cache when writing to writeableBitmap. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_NOTIMPL if conversion from imgSrc to RGBA is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if allocation was necessary and failed 
/// </returns>
/// <dl class="section remarks"><dt>Remarks</dt><dd>
///		- Available only in WinRT configurations of the core library.
///		- Implements all \ref stdconvrules.
///		- If writeableBitmap is null or the wrong size, a new WriteableBitmap is created.
///		- If writeableBitmap is already the right size, no allocation takes place. 
///		- bBypassCache is currently only supported for equal number of imgSrc/imgDst bands.
/// </dd></dl>
HRESULT VtConvertImageToWriteableBitmap(Windows::UI::Xaml::Media::Imaging::WriteableBitmap^& writeableBitmap, const CImg& imgSrc,
										bool bBypassCache = false);

/// \ingroup conversion
/// <summary> Convert a Vision Tools image to a WinRT byte array (for consumption
/// in an HTML application). </summary>
/// <param name="byteArray"> Output array. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="bBypassCache"> Bypass cache when writing to the array. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_NOTIMPL if conversion from imgSrc to RGBA is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if allocation was necessary and failed 
/// </returns>
/// <dl class="section remarks"><dt>Remarks</dt><dd>
///		- Available only in WinRT configurations of the core library.
///		- Implements all \ref stdconvrules.
///		- If byteArray is null or the wrong size, a new array is created.
///		- If byteArray is already the right size, no allocation takes place. 
///		- bBypassCache is currently only supported for equal number of imgSrc/imgDst bands.
/// </dd></dl>
HRESULT VtConvertImageToByteArray(Platform::Array<byte>^& byteArray, const CImg& imgSrc,
								  bool bBypassCache = false);
#endif

/// \ingroup conversion
/// <summary> Convert to luminance using \ref stdconvrules. </summary>
/// <param name="imgDst"> Output luminance image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="bBypassCache"> Bypass cache when writing to imgDst. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_INVALIDDST if imgDst is valid but not luminance
///		- E_INVALIDARG if imgDst shares memory with imgSrc
///		- E_NOTIMPL if conversion from imgSrc to imgDst type is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
HRESULT VtConvertImageToLuma(CImg &imgDst, const CImg &imgSrc, bool bBypassCache=false);

/// \ingroup conversion
/// <summary> Convert to RGB using \ref stdconvrules. </summary>
/// <param name="imgDst"> Output RGB image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="bBypassCache"> Bypass cache when writing to imgDst. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_INVALIDDST if imgDst is valid but not RGB
///		- E_INVALIDARG if imgDst shares memory with imgSrc
///		- E_NOTIMPL if conversion from imgSrc to imgDst type is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
HRESULT VtConvertImageToRGB(CImg &imgDst, const CImg &imgSrc, bool bBypassCache=false);

/// \ingroup conversion
/// <summary> Convert to RGBA using \ref stdconvrules.</summary>
/// <param name="imgDst"> Output RGBA image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="bBypassCache"> Bypass cache when writing to imgDst. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_INVALIDDST if imgDst is valid but not RGBA
///		- E_INVALIDARG if imgDst shares memory with imgSrc
///		- E_NOTIMPL if conversion from imgSrc to imgDst type is not supported
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
HRESULT VtConvertImageToRGBA(CImg &imgDst, const CImg &imgSrc, bool bBypassCache=false);

/// \ingroup conversion
/// <summary>Determine if the provided image pair have pixel types that are
///          supported by VtConvertImage</summary>
/// <param name="imgDst"> destination image. </param>
/// <param name="imgSrc"> source image </param>
/// <returns> 
///		- true if VtConvertImage can perform the conversion
///		- false otherwise
/// </returns>
bool VtIsValidConvertImagePair(const CImg& imgDst, const CImg& imgSrc);

/// \ingroup conversion
/// <summary> Convert RGB(A) to BGR(A) format </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid or not a color image
///		- E_INVALIDARG if imgDst shares memory with imgSrc
///		- E_NOTIMPL if conversion is not supported for given imgSrc and imgDst type 
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
HRESULT VtRGBColorSwapImage(CImg& imgDst, const CImg& imgSrc);

/// \ingroup conversion
/// <summary> Convert RGB(A) to BGR(A) format (in-place version)</summary>
/// <param name="img"> Input/Output image. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if img is invalid or not a color image
///		- E_NOTIMPL if conversion is not supported for given imgSrc type 
/// </returns>
HRESULT VtRGBColorSwapImage(CImg& img);

/// \ingroup conversion
/// <summary> Convert RGB(A) to HSB(A) format</summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid or not a color image
///		- E_INVALIDARG if imgDst shares memory with imgSrc
///		- E_NOTIMPL if conversion is not supported for given imgSrc type 
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Hue domain is 0.0f to 360.0f, saturation and Brightness are in 0.0f to 1.0f
///		- imgDst's output format is 3 or 4 bands (depending on whether imgSrc has alpha), EL_FORMAT_FLOAT, 
///		and PIX_FORMAT_NONE 
HRESULT VtConvertImageRGBToHSB(CImg &imgDst, const CImg& imgSrc);

/// \ingroup conversion
/// <summary> Convert HSB(A) to RGB(A) format</summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid or not HSB(A) format (see remarks)
///		- E_INVALIDARG if imgDst shares memory with imgSrc
///		- E_NOTIMPL if conversion is not supported for given imgDst type 
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- imgSrc's format must be 3 or 4 bands, EL_FORMAT_FLOAT, PIX_FORMAT_NONE
///		- imgDst's output pixel format is PIX_FORMAT_RGB or PIX_FORMAT_RGBA, depending on whether imgSrc has alpha
HRESULT VtConvertImageHSBToRGB(CImg &imgDst, const CImg &imgSrc);

/// \ingroup conversion
/// <summary> Modify an RGB(A) image via HSB adjustments </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="fHueAdj"> Hue adjustment (additive). </param>
/// <param name="fSatAdj"> Saturation adjustment (multiplicative). </param>
/// <param name="fBrtAdj"> Brightness adjustment (multiplicative). </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid or not a color image
///		- E_NOTIMPL if conversion is not supported for given imgSrc or imgDst type 
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Hue domain is 0.0f to 360.0f, saturation and brightness are in 0.0f to 1.0f
///		- Hue values behave like angles: out-of-range values wrap to 0.0f to 360.0f
HRESULT VtRGBModifyHSB(CImg& imgDst, const CImg& imgSrc, float fHueAdj, 
                       float fSatAdj, float fBrtAdj);

/// \ingroup conversion
/// \enum BandIndexType
/// <summary> Band indices for VtConvertBands() </summary>
enum BandIndexType
{ 
	BandIndexFill = -2,  ///< fill
	BandIndexIgnore, ///< ignore
	BandIndex0, ///< 0
	BandIndex1, ///< 1
	BandIndex2, ///< 2
	BandIndex3, ///< 3
	BandIndex4  ///< 4
};

/// \ingroup conversion
/// <summary> Extract a set of bands </summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="numDstBands"> Number of bands in the destination image. </param>
/// <param name="peBandSet"> Array of numDstBands BandIndexTypes describing imgDst's layout. </param>
/// <param name="pFillVals"> Array of values to use for BandIndexFill bands. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_INVALIDARG if imgDst shares memory with imgSrc
///		- E_INVALIDARG if aBandsSet is NULL or an element in aBandsSet is out of range
///		- E_NOTIMPL if conversion is not supported for given imgSrc or imgDst type 
///		- E_INVALIDDST or E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- If peBandSet[i] is BandIndexType::BandIndexFill, imgDst's i-th band is filled with pFillVals[i]
///		- If peBandSet[i] is BandIndexType::BandIndexIgnore, imgDst's i-th band is left unchanged
HRESULT VtConvertBands(CImg& imgDst, const CImg& imgSrc, 
                       int numDstBands, const BandIndexType* peBandSet, 
                       const void* pFillVals = NULL);

#ifndef VT_NO_XFORMS

/// \ingroup pointtransforms
/// <summary> Transform for extracting a set of bands from one image to another.</summary>
class CConvertBandsTransform :
	public CImageTransformPoint<CConvertBandsTransform, false>
{
public:
	CConvertBandsTransform(int numDstBands, const BandIndexType* peBandSet,
		const void* pFillVals = NULL) : m_numDstBands(numDstBands),
		m_peBandSet(peBandSet), m_pFillVals(pFillVals) {};

	virtual HRESULT Transform(OUT CImg* pimgDstRegion, 
							  IN  const CRect& /*rctLayerDst*/,
							  IN  CImg *const *ppimgSrcRegions,
							  IN  UINT  /*uSrcCnt*/
							  )
	{
        if (!pimgDstRegion->IsSharingMemory(*ppimgSrcRegions[0]))
            return E_INVALIDSRC;
		return VtConvertBands(*pimgDstRegion, *ppimgSrcRegions[1],
			m_numDstBands, m_peBandSet, m_pFillVals);
	}

	HRESULT Clone(ITaskState **ppState)
	{
		return CloneTaskState<CConvertBandsTransform>(ppState,
			VT_NOTHROWNEW CConvertBandsTransform(m_numDstBands,
				m_peBandSet, m_pFillVals));
	}

private:
	int m_numDstBands;
	const BandIndexType* m_peBandSet;
	const void* m_pFillVals;
};

#endif

//+-----------------------------------------------------------------------------
//
// Undocumented legacy functions
// 
//------------------------------------------------------------------------------

// Legacy, very similar to VtConvertBands
template <class TO, class TI>
HRESULT VtConvertImageMBandTo3Band(const CTypedImg<TI> &imgSrc, 
	CTypedImg<TO> &imgDst);

//+-----------------------------------------------------------------------------
//
// Element clipping
// 
//------------------------------------------------------------------------------

// generic clip function
template <class TS, class TD>
inline void VtClip(TD* p, TS v)
{ *p = TD(v<0? 0: (v>ElTraits<TD>::MaxVal())? ElTraits<TD>::MaxVal(): TD(v)); }  

// specialized for float source
template <class TD>
inline void VtClip(TD* p, float v)
{ *p = TD(v<0? 0: (v>ElTraits<TD>::MaxVal())? ElTraits<TD>::MaxVal(): F2I(v)); }

// specialized for sbyte
inline void VtClip(int8_t* p, float v)
{ *p = int8_t(v < ElTraits<int8_t>::TypeMin() ? ElTraits<int8_t>::TypeMin() : 
(v > ElTraits<int8_t>::TypeMax()) ? ElTraits<int8_t>::TypeMax() : F2I(v)); }

// specialized for sshort
inline void VtClip(int16_t* p, float v)
{ *p = int16_t(v < ElTraits<int16_t>::TypeMin() ? ElTraits<int16_t>::TypeMin() : 
(v > ElTraits<int16_t>::TypeMax()) ? ElTraits<int16_t>::TypeMax() : F2I(v)); }

// specialized for copy
inline void VtClip(Byte* p, Byte v) { *p = v; }
inline void VtClip(float* p, Byte f) { *p = f; }
inline void VtClip(float* p, UInt16 v) { *p = v; }
inline void VtClip(float* p, int f) { *p = (float)f; }
inline void VtClip(float* p, float f) { *p = f; }
inline void VtClip(UInt16* p, Byte v) { *p = v; }
inline void VtClip(int*  p,   Byte v) { *p = v; }
inline void VtClip(int*  p,   int v) { *p = v; }

// special case for signed 32-bit int
inline void VtClip(int*  p,  float f) 
//{ *p = f<INT32_MIN ? INT32_MIN : (f>INT32_MAX ? INT32_MAX: F2I(f)); }
{ *p = f<-0x8000000 ? -0x8000000 : (f>0x7fffffff ? 0x7fffffff: F2I(f)); }

// special case for double to float 
inline void VtClip(float* p, double f) 
{ 
	*p = float(f < ElTraits<float>::TypeMin() ? 
		ElTraits<float>::TypeMin() : 
		(f > ElTraits<float>::TypeMax() ? ElTraits<float>::TypeMax() : f)); 
}

inline float VtClip(float f, float fMin, float fMax) 
{ return f<fMin ? fMin : (f>fMax ? fMax : f); }

//+-----------------------------------------------------------------------------
//
// Conversion functions for the following "standard" pixel ranges
// Byte:  black(0) - white(0xff)
// Short: black(0) - white(0xffff)
// Float: black(0) - white(1.0)
// 
//------------------------------------------------------------------------------

inline void VtConv(int8_t* p, int8_t v) {*p = v;}
inline void VtConv(int8_t* p, float v)  {VtClip(p, v*255.f);}
inline void VtConv(Byte* p, Byte v)     {*p = v;}
inline void VtConv(Byte* p, UInt16 v)   {*p = (Byte)((v>=0xfe80)? 0xff: ((v+0x80)>>8));}
inline void VtConv(Byte* p, float v)    {VtClip(p, v*255.f);}
inline void VtConv(int16_t* p, int8_t v) {*p = v;}
inline void VtConv(int16_t* p, int16_t v) {*p = v;}
inline void VtConv(int16_t* p, float v) {VtClip(p, v*float(0xffff));}
inline void VtConv(UInt16* p, Byte v)   {*p = (v<<8)|v;}
inline void VtConv(UInt16* p, UInt16 v) {*p = v;}
inline void VtConv(UInt16* p, float v)  {VtClip(p, v*float(0xffff));}
inline void VtConv(float* p, int8_t v)  {*p = float(v)*(1.f/255.f);}
inline void VtConv(float* p, Byte v)    {*p = float(v)*(1.f/255.f);}
inline void VtConv(float* p, int16_t v) {*p = float(v)*(1.f/float(0xffff));}
inline void VtConv(float* p, UInt16 v)  {*p = float(v)*(1.f/float(0xffff));}
inline void VtConv(float* p, float v)   {*p = v;}
inline void VtConv(float* p, int v)     {*p = float(v);}
inline void VtConv(float* p, double v)  {VtClip(p, v);}
inline void VtConv(int* p, double v)    {*p = F2I(float(v));}
inline void VtConv(int* p, int v)       {*p = v;}
inline void VtConv(double* p, float v)  {*p = v;}
inline void VtConv(double* p, int v)    {*p = v;}
inline void VtConv(double* p, double v) {*p = v;}
inline void VtConv(HALF_FLOAT* p, HALF_FLOAT v) {*p = v;}

// convert to RGB to YPbPr
inline float VtLumaFromRGB_CCIR601YPbPr(Byte r, Byte g, Byte b) 
{ return (0.299f/255.f)*r + (0.587f/255.f)*g + (0.114f/255.f)*b; }

inline float VtLumaFromRGB_CCIR601YPbPr(UInt16 r, UInt16 g, UInt16 b) 
{ return (0.299f/65535.f)*r + (0.587f/65535.f)*g + (0.114f/65535.f)*b; }

inline float VtLumaFromRGB_CCIR601YPbPr(int r, int g, int b) 
{ return 0.299f*float(r) + 0.587f*float(g) + 0.114f*float(b); }

inline float VtLumaFromRGB_CCIR601YPbPr(float r, float g, float b) 
{ return 0.299f*r + 0.587f*g + 0.114f*b; }

template<class T>
inline float VtLumaFromRGB_CCIR601YPbPr(T *prgb)	
{ return VtLumaFromRGB_CCIR601YPbPr(prgb->r, prgb->g, prgb->b); }

// convert to RGB to YCbCr
inline Byte VtLumaFromRGB_CCIR601YCbCr(Byte r, Byte g, Byte b)
{ return (Byte) (((66*int(r) + 129*int(g) + 25*int(b) + 128) >> 8) + 16); }

inline Byte VtCbFromRGB_CCIR601YCbCr(Byte r, Byte g, Byte b)
{ return (Byte) ((-38*int(r) - 74*int(g) + 112*int(b) + 128) >> 8) + 128; }

inline Byte VtCrFromRGB_CCIR601YCbCr(Byte r, Byte g, Byte b)
{ return (Byte) ((112*int(r) - 94*int(g) - 18*int(b) + 128) >> 8) + 128; }

inline UInt16 VtLumaFromRGB_CCIR601YCbCr(UInt16 r, UInt16 g, UInt16 b)
{ return (UInt16)F2I(0.257f*float(r) + 0.504f*float(g) + 0.098f*float(b) + float(16 << 8)); }

inline UInt16 VtCbFromRGB_CCIR601YCbCr(UInt16 r, UInt16 g, UInt16 b)
{ return (UInt16)F2I(-.148f*float(r) - 0.291f*float(g) + .439f*float(b) + float(128 << 8)); }

inline UInt16 VtCrFromRGB_CCIR601YCbCr(UInt16 r, UInt16 g, UInt16 b)
{ return (UInt16)F2I(0.439f*float(r) - 0.368f*float(g) - 0.071f*float(b) + float(128 << 8)); }
//+-----------------------------------------------------------------------------
//
// Span conversion functions 
// 
//------------------------------------------------------------------------------

// generic span conversion
HRESULT VtConvertSpan(void* pDst, int iDstType, const void* pSrc, int iSrcType, 
					  int iSrcElCount, bool bBypassCache = false);

// span conversion with fixed types but variable bands
template <class TO, class TI>
HRESULT VtConvertSpanBands(TO* pDst, int iDstBands, const TI* pSrc, int iSrcBands,
						   int iSrcElCount, bool bBypassCache = false);

// span conversions for fixed types and fixed bands
template <class TO, class TI>
void VtConvertSpan(TO* pOut, const TI* pIn, int span, bool bBypassCache = false);

// span conversions for fixed identical types and bands 
// (= memcopy if not bypasscache == true)
template <class TIO>
void VtConvertSpan(TIO* pOut, const TIO* pIn, int span, bool bBypassCache = false);

template <class TO, class TI>
void VtConvertSpanRGBToGray(TO* pOut, const RGBType<TI>* pIn, int pixcnt);

template <class TO, class TI>
void VtConvertSpanRGBAToGray(TO* pOut, const RGBAType<TI>* pIn, int pixcnt);

template <class TO, class TI>
void VtConvertSpanGrayToRGB(RGBType<TO>* pOut, const TI* pIn, int pixcnt);

template <class TO, class TI>
void VtConvertSpanGrayToRGBA(RGBAType<TO>* pOut, const TI* pIn, int pixcnt);

template <class TO, class TI>
void VtConvertSpanRGBToRGBA(RGBAType<TO>* pOut, const RGBType<TI>* pIn, 
							int pixcnt);

template <class TO, class TI>
void VtConvertSpanRGBAToRGB(RGBType<TO>* pOut, const RGBAType<TI>* pIn, 
							int pixcnt);

////////////////////////////////////////////////////////////////////////////////

template <class TI>
void VtConvertSpanFloatToRGBE(Byte* pOut, TI* pRGB, int pixcnt);

template <class TO, class TI>
TO* VtConvertSpanMBandTo3Band(TO* pOut, const TI* pIn, int pixcnt, 
	int skippedBands);

template <class TO, class TI>
TO* VtConvertSpanARGBTo1Band(TO* pOut, const TI* pIn, int inspan, int band);

HRESULT VtRGBColorSwapSpan(Byte* pDst, int iDstType,
                           const Byte* pSrc, int iSrcType, int span);

template <class T>
void VtRGBColorSwapSpan(RGBType<T>* pDst, const RGBType<T>* pSrc, int span);

template <class T>
void VtRGBColorSwapSpan(RGBAType<T>* pDst, const RGBAType<T>* pSrc, int span);

float* VtConvertSpanRGB32ToHSB(float* pD, const RGBAFloatPix* pS, int pixcnt);

template <class TO>
TO* VtConvertSpanHSBToRGB32(TO* pD, const float* pS, int pixcnt);

float* VtModifySpanHSB(float* pD, const float* pS, float fHueAdj, 
                       float fSatAdj, float fLightAdj, int iSpan);

template <class TO, class TI>
void VtConvertBandsSpan(TO* pD, int numDstBands, const TI* pS, int numSrcBands,
						int numPixels, const BandIndexType* peBandSet, 
						const TO* aFillVals);
#ifndef VT_NO_XFORMS

//+-----------------------------------------------------------------------------
//
// Class: CConvertTransform
// 
// Synopsis: IImageTransform implementation of convert
// 
//------------------------------------------------------------------------------
class CConvertTransform: public CImageTransformUnaryPoint<CConvertTransform, false>
{
public:
    CConvertTransform()
	{}

	explicit CConvertTransform(int compositeType)
		: m_iDestPixFormat(compositeType)
	{ VT_ASSERT(m_iDestPixFormat != OBJ_UNDEFINED); }

public:
	virtual void GetSrcPixFormat(int* pfrmtSrcs, UINT, int)
	{ pfrmtSrcs[0] = OBJ_UNDEFINED; }

	virtual void GetDstPixFormat(OUT int& frmtDst,
								 IN  const int* /*pfrmtSrcs*/, 
								 IN  UINT  /*uSrcCnt*/)
	{ frmtDst = m_iDestPixFormat; }

	virtual HRESULT GetRequiredSrcRect(OUT TRANSFORM_SOURCE_DESC* pSrcReq,
									   OUT UINT& uSrcReqCount,
									   IN  UINT  /*uSrcCnt*/,
									   IN  const CRect& rctLayerDst
									   )
	{
		pSrcReq->bCanOverWrite = false;
		pSrcReq->rctSrc        = rctLayerDst;
		pSrcReq->uSrcIndex     = 0;
		uSrcReqCount           = 1;
		return S_OK;
	}

	HRESULT Transform(CImg* pDst, IN const CRect& /*rctDst*/, const CImg& src)
	{ return VtConvertImage(*pDst, src); }

	virtual HRESULT Clone(ITaskState **ppState)
	{ return CloneTaskState(ppState, VT_NOTHROWNEW CConvertTransform()); }

private:
	int m_iDestPixFormat;
};

#endif

// Internal helpers
bool IsValidConvertPair(int typeA, int typeB);

//+-----------------------------------------------------------------------------
//
// internal HALF_FLOAT specializations of VtConv.  Ideally the functions
// that use this should be restructured to do half-float conversions a span
// at a time instead of 1 element at a time which is very expensive
//
//------------------------------------------------------------------------------
inline float HF2F(HALF_FLOAT hf)
{ 
	float f;
	VtConvertSpan(&f, &hf, 1);
	return f;
}

inline HALF_FLOAT F2HF(float f)
{ 
	HALF_FLOAT hf;
	VtConvertSpan(&hf, &f, 1);
	return hf;
}

inline void VtConv(Byte* p,   HALF_FLOAT v) { VtConv(p, HF2F(v)); }
inline void VtConv(UInt16* p, HALF_FLOAT v) { VtConv(p, HF2F(v)); }
inline void VtConv(float* p,  HALF_FLOAT v) { VtConv(p, HF2F(v)); }
inline void VtConv(HALF_FLOAT* p, Byte v)   {float vf; VtConv(&vf, v); *p = F2HF(vf); }
inline void VtConv(HALF_FLOAT* p, UInt16 v) {float vf; VtConv(&vf, v); *p = F2HF(vf); }
inline void VtConv(HALF_FLOAT* p, float v)  {float vf; VtConv(&vf, v); *p = F2HF(vf); }

};