//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for filtering images
//
//  History:
//      2008/2/27-swinder
//          Created
//
//------------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_convert.h"
#include "vt_kernel.h"
#include "vt_extend.h"

#ifndef VT_NO_XFORMS
#include "vt_transform.h"
#endif

namespace vt {

#ifdef MAKE_DOXYGEN_WORK
void foo();
#endif

/// \ingroup filter
/// <summary>Applies an image filter kernel to the provided source image.
/// The rctDst and ptSrc agruments enable the filtering operation to be 
/// applied to a sub-region of the source.</summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="rctDst"> Indicates the location and dimensions of the output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="ptSrcOrigin"> The origin of the input image </param>
/// <param name="kh"> The horizontal convolution \link C1dKernel kernel \endlink </param>
/// <param name="kv"> The vertical convolution \link C1dKernel kernel \endlink </param>
/// <param name="ex"> The \link IMAGE_EXTEND extension mode \endlink to apply when the 
/// convolution kernel requires support outside of the source image</param>
/// <returns> 
/// - S_OK on success
/// - E_INVALIDDST if imgDst shares memory with imgSrc
/// - E_INVALIDARG if conversion from imgSrc to imgDst type is not supported
/// - E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///	- Implements all \ref stdconvrules.
/// - The filter can not be applied in place; the provided input and
/// output images must be different.

HRESULT
VtSeparableFilter(CImg &imgDst, const CRect& rctDst, 
				  const CImg &imgSrc, CPoint ptSrcOrigin,
				  const C1dKernel& kh, const C1dKernel& kv,
				  const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend));

/// \ingroup filter
/// <summary>Same function as above except the same kernel is applied in 
/// both horizontal and vertical directions.</summary>
inline HRESULT
VtSeparableFilter(CImg &imgDst, const CRect& rctDst, 
				  const CImg &imgSrc, CPoint ptSrcOrigin,
				  const C1dKernel& k,
				  const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend))
{
	return VtSeparableFilter(imgDst, rctDst, imgSrc, ptSrcOrigin,
							 k, k, ex);
}

/// \ingroup filter
/// <summary>Applies an image filter kernel to the provided source image.</summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="kh"> The horizontal convolution \link C1dKernel kernel \endlink </param>
/// <param name="kv"> The vertical convolution \link C1dKernel kernel \endlink </param>
/// <param name="ex"> The \link IMAGE_EXTEND extension mode \endlink to apply when the 
/// filter kernel requires support outside of the source image</param>
/// <returns> 
/// - S_OK on success
/// - E_INVALIDDST if imgDst shares memory with imgSrc
/// - E_INVALIDARG if conversion from imgSrc to imgDst type is not supported
/// - E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///	- Implements all \ref stdconvrules.
/// - The filter can not be applied in place; the provided input and
/// output images must be different.

inline HRESULT
VtSeparableFilter(CImg &imgDst, const CImg &imgSrc,
				  const C1dKernel& kh, const C1dKernel& kv,
				  const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend))
{
	return VtSeparableFilter(imgDst, imgSrc.Rect(), imgSrc, vt::CPoint(0,0),
							 kh, kv, ex);
}

/// \ingroup filter
/// <summary>Applies an image filter kernel to the provided source image.</summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="k"> The horizontal and vertical convolution \link C1dKernel kernel \endlink </param>
/// <param name="ex"> The \link IMAGE_EXTEND extension mode \endlink to apply when the 
/// filter kernel requires support outside of the source image</param>
/// <returns> 
/// - S_OK on success
/// - E_INVALIDDST if imgDst shares memory with imgSrc
/// - E_INVALIDARG if conversion from imgSrc to imgDst type is not supported
/// - E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///	- Implements all \ref stdconvrules.
/// - The filter can not be applied in place; the provided input and
/// output images must be different.
inline HRESULT
VtSeparableFilter(CImg &imgDst, const CImg &imgSrc, const C1dKernel& k,
				  const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend))
{
	return VtSeparableFilter(imgDst, imgSrc.Rect(), imgSrc, vt::CPoint(0,0),
							 k, k, ex);
}

/// \ingroup filter
/// <summary>Applies a spatially varying filter kernel to the provided 
/// source image.  Spatially varying (also known as polyphase) filters are
/// useful when resampling images.  The rctDst and ptSrc agruments enable 
/// the filtering operation to be applied to a sub-region of the source.</summary>
/// <param name="imgDst"> Output filtered image. </param>
/// <param name="rctDst"> Indicates the location and dimensions of the output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="ptSrcOrigin"> The origin of the input image </param>
/// <param name="ksh"> The horizontal \link C1dKernelSet C1dKernelSet \endlink </param>
/// <param name="ksv"> The vertical \link C1dKernelSet C1dKernelSet \endlink </param>
/// <param name="ex"> The \link IMAGE_EXTEND extension mode \endlink to apply 
/// when the filter kernel requires support outside of the source image.  The
/// default is Extend.</param>
/// <returns> 
/// - S_OK on success
/// - E_INVALIDSRC if imgSrc is not valid
/// - E_INVALIDDST if imgDst shares memory with imgSrc
/// - E_INVALIDARG if conversion from imgSrc to imgDst type is not supported
/// - E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///	- Implements all \ref stdconvrules.
/// - The filter can not be applied in place; the provided input and
/// output images must be different.

HRESULT
VtSeparableFilter(CImg &imgDst, const CRect& rctDst, 
				  const CImg &imgSrc, CPoint ptSrcOrigin,
				  const C1dKernelSet& ksh, const C1dKernelSet& ksv,
				  const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend));

inline HRESULT
VtSeparableFilter(CImg &imgDst, const CRect& rctDst, 
				  const CImg &imgSrc, CPoint ptSrcOrigin,
				  const C1dKernelSet& ks,
				  const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend))
{
	return VtSeparableFilter(imgDst, rctDst, imgSrc, ptSrcOrigin,
							 ks, ks, ex);
}

/// \ingroup filter
/// <summary>Applies a filter kernel to a one-dimensional array.</summary>
/// <param name="pDst"> Pointer to the destination array of values </param>
/// <param name="iDstAddr"> The address of pDst </param>
/// <param name="iDstWidth"> The count of pDst values to generate  </param>
/// <param name="iDstType"> The \link pixformat type \endlink of the values pointed to by pDst </param>
/// <param name="iDstBands"> The number of bands in the pDst array.</param>
/// <param name="pSrc"> Pointer to the source array of values </param>
/// <param name="iSrcAddr"> The address of pSrc (should be in same coordinate system as iDstAddr) </param>
/// <param name="iSrcWidth"> The count of values in the pSrc array  </param>
/// <param name="iSrcType"> The \link pixformat type \endlink of the values pointed to by pSrc </param>
/// <param name="iSrcBands"> The number of bands in the pSrc array.</param>
/// <param name="k"> The filter \link C1dKernel kernel \endlink </param>
/// <param name="ex"> The vt::ExtendMode to apply when the 
/// kernel requires support outside of the source.</param>
/// <returns> 
/// - S_OK on success
/// - E_NOTIMPL if extend mode is other than Extend or Zero or the filter
/// kernel is wider than 2000.
/// - E_INVALIDDST if pDst and pSrc are overlapping - this filter cannot be
/// done in place.
/// </returns>
HRESULT 
VtFilter1d(void* pDst, int iDstAddr, int iDstWidth, 
		   int iDstType, int iDstBands,
		   const void* pSrc, int iSrcAddr, int iSrcWidth, 
		   int iSrcType, int iSrcBands,
		   const C1dKernel& k, ExtendMode ex = Extend);

/// \ingroup filter
/// <summary>Applies a filter kernel to a one-dimensional array.</summary>
/// <param name="pDst"> Pointer to the destination array of values </param>
/// <param name="iDstType"> The \link pixformat type \endlink of the values pointed to by pDst </param>
/// <param name="iDstBands"> The number of bands in the pDst array.</param>
/// <param name="pSrc"> Pointer to the source array of values </param>
/// <param name="iSrcType"> The \link pixformat type \endlink of the values pointed to by pSrc </param>
/// <param name="iSrcBands"> The number of bands in the pSrc array.</param>
/// <param name="iCount"> The count of source and destination values </param>
/// <param name="k"> The filter \link C1dKernel kernel \endlink </param>
/// <param name="ex"> The vt::ExtendMode to apply when the 
/// kernel requires support outside of the source.</param>
/// <returns> 
/// - S_OK on success
/// - E_NOTIMPL if extend mode is other than Extend or Zero or the filter
/// kernel is wider than 2000.
/// - E_INVALIDDST if pDst and pSrc are overlapping - this filter cannot be
/// done in place.
/// </returns>

inline HRESULT
VtFilter1d(void* pDst, int iDstType, int iDstBands,
           const void* pSrc, int iSrcType, int iSrcBands,
		   int iCount, const C1dKernel& k, ExtendMode ex = Extend)
{ 
	return VtFilter1d(pDst, 0, iCount, iDstType, iDstBands,
					  pSrc, 0, iCount, iSrcType, iSrcBands, k, ex);
}

/// \ingroup filter
/// <summary>Applies a filter kernel to a one-dimensional array.</summary>
/// <param name="pDst"> Pointer to the destination array of values </param>
/// <param name="iDstType"> The \link pixformat type \endlink of the values pointed to by pDst </param>
/// <param name="pSrc"> Pointer to the source array of values </param>
/// <param name="iSrcType"> The \link pixformat type \endlink of the values pointed to by pSrc </param>
/// <param name="iCount"> The count of source and destination values </param>
/// <param name="k"> The filter \link C1dKernel kernel \endlink </param>
/// <param name="ex"> The vt::ExtendMode to apply when the 
/// kernel requires support outside of the source.</param>
/// <returns> 
/// - S_OK on success
/// - E_NOTIMPL if extend mode is other than Extend or Zero or the filter 
/// kernel is wider than 2000.
/// - E_INVALIDDST if pDst and pSrc are overlapping - this filter cannot be
/// done in place.
/// </returns>

inline HRESULT
VtFilter1d(void* pDst, int iDstType, const void* pSrc, int iSrcType,
		   int iCount, const C1dKernel& k, ExtendMode ex = Extend)
{ return VtFilter1d(pDst, iDstType, 1, pSrc, iSrcType, 1, iCount, k, ex); }



#ifndef VT_NO_XFORMS

/// \ingroup filtertransforms
/// <summary>Implementation of \link IImageTransform image transform \endlink that applies
/// a filter \link C1dKernel C1dKernel \endlink or \link C1dKernelSet C1dKernelSet \endlink.
/// </summary> 

class CSeparableFilterTransform: 
    public CImageTransformUnaryGeo<CSeparableFilterTransform, true>
{
	// IImageTransform implementation
public:
	void GetDstPixFormat(OUT int& frmtDst,
						 IN  const int* pfrmtSrcs, 
						 IN  UINT  uSrcCnt);

	vt::CRect GetRequiredSrcRect(const vt::CRect& rctDst)
	{ return vt::GetRequiredSrcRect(rctDst, m_kernelsetHoriz, m_kernelsetVert); }

	vt::CRect GetAffectedDstRect(const vt::CRect& rctSrc)
	{ return vt::GetAffectedDstRect(rctSrc, m_kernelsetHoriz, m_kernelsetVert); }

	vt::CRect GetResultingDstRect(const vt::CRect& rctSrc)
	{
		const float factorX = m_kernelsetHoriz.GetCycle() /
			((float)m_kernelsetHoriz.GetCoordShiftPerCycle());  
		const float factorY = m_kernelsetVert.GetCycle() /
			((float)m_kernelsetVert.GetCoordShiftPerCycle());
		return vt::CRect(F2I(rctSrc.left * factorX), F2I(rctSrc.top * factorY), 
			F2I(rctSrc.right * factorX), F2I(rctSrc.bottom * factorY)); 
	}

	HRESULT Transform(CImg* pimgDst, IN  const CRect& rctDst,
					  const CImg& imgSrc, const CPoint& ptSrc);

	virtual HRESULT Clone(ITaskState **ppState);
	
public:
    CSeparableFilterTransform()
	{}

	/// <summary> Initialize transform with a kernel </summary> 
	/// <param name="dstType">The image type that the transform will generate.</param> 
	/// <param name="krnlHoriz">The horizontal filter kernel to apply.</param> 
	/// <param name="krnlVert">The vertical filter kernel to apply.</param> 
	/// <returns> 
	///		- S_OK on success 
	///		- E_OUTOFMEMORY if memory allocation failed 
	/// </returns>
    HRESULT Initialize(int dstType, const C1dKernel& krnlHoriz, 
					   const C1dKernel& krnlVert);

    HRESULT Initialize(int dstType, const C1dKernel& krnl)
    { return Initialize(dstType, krnl, krnl); }

	/// <summary> Initialize transform with a kernel set </summary> 
	/// <param name="dstType">The image type that the transform will generate.</param> 
	/// <param name="krnlsetHoriz">The horizontal filter kernelset to apply.</param> 
	/// <param name="krnlsetVert">The vertical filter kernelset to apply.</param> 
	/// <returns> 
	///		- S_OK on success 
	///		- E_OUTOFMEMORY if memory allocation failed 
	/// </returns>
    HRESULT Initialize(int dstType, 
					   const C1dKernelSet& krnlsetHoriz,
					   const C1dKernelSet& krnlsetVert);

    HRESULT Initialize(int dstType, const C1dKernelSet& krnlset)
	{ return Initialize(dstType, krnlset, krnlset); }

	/// <summary> Initialize transform for special case of 2 to 1 downsampling.</summary> 
	/// <param name="dstType">The image type that the transform will generate.</param> 
	/// <param name="krnl">The filter kernel to apply before downsampling.</param> 
	/// <returns> 
	///		- S_OK on success 
	///		- E_OUTOFMEMORY if memory allocation failed 
	/// </returns>
	HRESULT InitializeForPyramidGen(int dstType, 
									const C1dKernel& krnl);

protected:
	HRESULT TransformSingleKernel(CImg* pimgDst, IN  const CRect& rctDst,
								  const CImg& imgSrc, const CPoint& ptSrc);

	HRESULT TransformMultiKernel(CImg* pimgDst, IN  const CRect& rctDst,
								 const CImg& imgSrc, const CPoint& ptSrc);

protected:
    C1dKernelSet m_kernelsetHoriz;
	C1dKernelSet m_kernelsetHorizScl;
	C1dKernelSet m_kernelsetVert;
	C1dKernelSet m_kernelsetVertScl;
	int          m_dstType;
};

#endif


/// \ingroup filter
/// <summary>Specialization of VtSeparableFilter applying a 121 filter
/// kernel with the following constraints: 'Extend' ExtendMode only, 1
/// through 4 bands only; Byte or Float source and destination only.
/// </summary>
/// <param name="imgDst"> Output filtered image. </param>
/// <param name="rctDst"> Indicates the location and dimensions of the output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="ptSrcOrigin"> The origin of the input image </param>
/// <param name="bLowBytePrecision"> When true, performs the filtering
/// operation for Byte image types with lower precision accumulation and
/// significantly higher performance.  The default is false, which offers very
/// good performance and full precision. The low precision option uses the SSE or
/// Neon 'average' instructions, which are very fast but round and quantize the
/// result to 8 bits several times during the filtering operation.  The lower
/// precision operation results in a positive bias in the destination image;
/// this bias varies with image content but is approximately .5%, and
/// accumulates for each pass when using multipass (so a 9 pass operation will
/// have a positive bias of ~4.5%). </param>
/// <param name="numPasses"> The number of 121 passes to apply. </param>
/// <param name="pimgTmp"> Temporary image of the same size and type as the
/// output for use as a temporary buffer between passes when numPasses > 1; if
/// left NULL when numPasses > 1, routine will allocate a temporary buffer and
/// thus run more slowly. </param>
/// <returns> 
/// - S_OK on success
/// - E_INVALIDSRC if imgSrc is not valid
/// - E_INVALIDDST if imgDst shares memory with imgSrc
/// - E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// - E_OUTOFMEMORY if internal temporary buffer allocation failed 
/// - E_INVALIDARG if pimgTmp is provided and is not the same dimensions and
///   type as imgDst
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///	- Implements all \ref stdconvrules.
/// - numPasses controls the number of times the 121 filter is applied in the
/// generation of the destination image; if set to the default 1, then a single
/// 121 filtering pass is applied; if set to 2, then two passes are applied
/// which has the same effect as applying a 5 wide filter with weights 1-4-6-4-1,
/// and a value of 3 effectively applies a 7 wide 1-6-15-20-15-6-1 filter , etc.
/// (see Pascal's Triangle)
/// - The filter can not be applied in place; the provided input and
/// output images must be different.
/// - The routine functions correctly for arbitrary alignment of source
/// and destination row addresses, but runs significantly faster when
/// source rows starting at (rctDst-ptSrcOrigin) are all at least 8 byte
/// aligned and destination rows at rctDst are all at least 16 byte aligned
/// when called with EL_FORMAT_BYTE images.
/// - The routine will invoke the generic VtSeparableFilter call instead
/// of the specialization code (which may be significantly slower), if:
///     - imgSrc element format is not EL_FORMAT_BYTE or EL_FORMAT_FLOAT
///     - imgDst does not have the same element format as ImgSrc
///     - imgDst does not have the same number of bands as imgSrc
///     - imgSrc has more than 4 bands

HRESULT
VtSeparableFilter121(
    CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, CPoint ptSrcOrigin,
    bool bLowBytePrecision = false, int numPasses = 1, CImg* pimgTmp = NULL);

/// \ingroup filter
/// <summary>Specialization of VtSeparableFilter applying a 121 filter
/// kernel and 2:1 decimation with the following constraints: 'Extend'
/// ExtendMode only, 1 through 4 bands only; Byte and Float source and
/// destination only.</summary>
/// <param name="imgDst"> Output filtered image. </param>
/// <param name="rctDst"> Indicates the location and dimensions of the output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="ptSrcOrigin"> The origin of the input image </param>
/// <param name="bLowBytePrecision"> When true, performs the filtering
/// operation for Byte image types with lower precision accumulation and
/// significantly higher performance.  The default is false, which offers very
/// good performance and full precision. The low precision option uses the SSE or
/// Neon 'average' instructions, which are very fast but round and quantize the
/// result to 8 bits several times during the filtering operation.  The lower
/// precision operation results in a positive bias in the destination image;
/// this bias varies with image content but is approximately .5%, and
/// accumulates for each pass when using multipass (so a 9 pass operation will
/// have a positive bias of ~4.5%). </param>
/// <returns> 
/// - S_OK on success
/// - E_INVALIDSRC if imgSrc is not valid
/// - E_INVALIDDST if imgDst shares memory with imgSrc
/// - E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// - E_OUTOFMEMORY if internal temporary buffer allocation failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///	- Implements all \ref stdconvrules.
/// - The filter can not be applied in place; the provided input and
/// output images must be different.
/// - The routine functions correctly for arbitrary alignment of source
/// and destination row addresses, but runs significantly faster when
/// source rows starting at (rctDst-ptSrcOrigin) are all at least 8 byte
/// aligned and destination rows at rctDst are all at least 16 byte aligned
/// when called with EL_FORMAT_BYTE images.
/// - The routine will invoke the generic VtSeparableFilter call instead
/// of the specialization code (which may be significantly slower), if:
///     - imgSrc element format is not EL_FORMAT_BYTE or EL_FORMAT_FLOAT
///     - imgDst does not have the same element format as ImgSrc
///     - imgDst does not have the same number of bands as imgSrc
///     - imgSrc has more than 4 bands
///     - imgSrc is EL_FORMAT_FLOAT but has more than one band

HRESULT
VtSeparableFilter121Decimate2to1(
    CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, CPoint ptSrcOrigin,
    bool bLowBytePrecision = false);

/// \ingroup geofunc
/// <summary>Specialization of VtSeparableFilter for two-to-one decimation 
/// applying a box filter kernel. Supports 'Extend' ExtendMode only.</summary>
/// <param name="imgDst">Output resized image.</param> 
/// <param name="rctDst">Indicates the location and dimensions of the output image.</param>
/// <param name="imgSrc">Input image.</param>
/// <param name="ptSrcOrigin">The origin of the input image.</param>
/// <dl class="section remarks"><dt>Remarks</dt><dd>
///	Implements all \ref stdconvrules.
/// </dd></dl>
HRESULT 
VtSeparableFilterBoxDecimate2to1(CImg& imgDst, const CRect& rctDst,
                                 const CImg& imgSrc, CPoint ptSrcOrigin);
/// \ingroup geofunc
/// <summary>Specialization of VtSeparableFilter for two-to-one decimation
/// applying a box filter kernel. Supports 'Extend' ExtendMode only, and is
/// currently limited to 1 band float or 1 band byte images.</summary>
/// <param name="imgDst">Output resized image.</param> 
/// <param name="rctDst">Indicates the location and dimensions of the output image. 
/// <param name="imgSrc">Input image.</param>
/// <param name="ptSrcOrigin"> The origin of the input image </param>
/// <dl class="section remarks"><dt>Remarks</dt><dd>
///	Implements all \ref stdconvrules.
/// </dd></dl>
HRESULT 
VtSeparableFilterBoxDecimate4to1(CLumaFloatImg& imgDst, const CRect& rctDst, 
                                 const CLumaFloatImg& imgSrc, CPoint ptSrcOrigin);
/// \ingroup geofunc
/// <summary>Specialization of VtSeparableFilter for four-to-one decimation
/// applying a box filter kernel. Supports 'Extend' ExtendMode only, and is
/// currently limited to 1 band float or 1 band byte images.</summary>
/// <param name="imgDst">Output resized image.</param> 
/// <param name="rctDst">Indicates the location and dimensions of the output image.</param>
/// <param name="imgSrc">Input image.</param>
/// <param name="ptSrcOrigin">The origin of the input image.</param>
/// <dl class="section remarks"><dt>Remarks</dt><dd>
///	Implements all \ref stdconvrules.
/// </dd></dl>
HRESULT 
VtSeparableFilterBoxDecimate4to1(CLumaByteImg& imgDst, const CRect& rctDst, 
                                 const CLumaByteImg& imgSrc, CPoint ptSrcOrigin);

#ifndef VT_NO_XFORMS

//+-----------------------------------------------------------------------------
//
// Class: CSeparableFilter121Transform
// 
// Synopsis: IImageTransform implementation of VtSeparableFilter121
// 
/// \ingroup filtertransforms
/// <summary>Implementation of \link IImageTransform image transform \endlink
/// that applies a 121 filter with or without two-to-one decimation.
/// </summary> 
//------------------------------------------------------------------------------
class CSeparableFilter121Transform: public IImageTransform
{
	// IImageTransform implementation
public:
    virtual bool RequiresCloneForConcurrency()
    { return false; }

    virtual void GetSrcPixFormat(IN OUT int* pfrmtSrcs, 
                                 IN UINT  /*uSrcCnt*/,
                                 IN int frmtDst)
    {
        if (!m_NoRW)
        { pfrmtSrcs[0] = frmtDst; }
    }
    virtual void    GetDstPixFormat(OUT int& frmtDst,
                                    IN  const int* pfrmtSrcs, 
                                    IN  UINT  /*uSrcCnt*/)
    {
        if (!m_NoRW)
        { frmtDst = pfrmtSrcs[0]; }
    }

    virtual HRESULT GetRequiredSrcRect(OUT TRANSFORM_SOURCE_DESC* pSrcReq,
                                      OUT UINT& uSrcReqCount,
                                      IN  UINT  /*uSrcCnt*/,
                                      IN  const CRect& rctLayerDst
                                      )
    { 
        if (!m_NoRW)
        {
            vt::CRect rctRqdSrc = rctLayerDst;
            if (m_bDeci)
            {
                int w = rctRqdSrc.Width();
                int h = rctRqdSrc.Height();
                rctRqdSrc.left = rctRqdSrc.left<<1;
                rctRqdSrc.right = rctRqdSrc.left+(w<<1);
                rctRqdSrc.top = rctRqdSrc.top<<1;
                rctRqdSrc.bottom = rctRqdSrc.top+(h<<1);
            }
            // pad by 16 on left side only to align each row's first non-padded
            // pixel to 16 byte boundary for better SSE performance
            rctRqdSrc.InflateRect(16,1,1,1);

            uSrcReqCount = 1;
    		pSrcReq[0].bCanOverWrite = true;
			pSrcReq[0].rctSrc        = rctRqdSrc;
			pSrcReq[0].uSrcIndex     = 0;
        }
        else
        {
            uSrcReqCount = 0;
        }
        return S_OK;
    }
	virtual HRESULT GetResultingDstRect(OUT CRect& rctDst,
	                                    IN  const CRect& rctSrc,
	                                    IN  UINT /*uSrcIndex*/,
										IN  UINT /*uSrcCnt*/)
	{
        rctDst = rctSrc;
        if (m_bDeci)
        {
            int w = rctSrc.Width();
            int h = rctSrc.Height();
            rctDst.left = rctDst.left>>1;
            rctDst.right = rctDst.left+(w>>1);
            rctDst.top = rctDst.top>>1;
            rctDst.bottom = rctDst.top+(h>>1);
        }
		return S_OK;
	}
		
    virtual HRESULT GetAffectedDstRect(OUT CRect& rctDst,
                                      IN  const CRect& rctSrc,
                                      IN  UINT /*uSrcIndex*/,
                                      IN  UINT /*uSrcCnt*/)
    { 
        GetResultingDstRect(rctDst,rctSrc,0,0);
        return S_OK;
    }

    virtual HRESULT Transform(OUT CImg* pimgDstRegion, 
                              IN  const CRect& rctLayerDst,
                              IN  CImg *const *ppimgSrcRegions,
                              IN  const TRANSFORM_SOURCE_DESC* pSrcDesc,
                              IN  UINT /*uSrcCnt*/ );

	virtual HRESULT Clone(ITaskState ** /*ppState*/ )
	{
		return E_FAIL;
    }

public:
    CSeparableFilter121Transform()
        :m_bDeci(false)
        ,m_NoRW(false)
        ,m_bLowPrecision(false)
    {}

	/// <summary> Initialize transform</summary> 
	/// <returns> 
	///		- always returns S_OK
	/// </returns>
    HRESULT Initialize(bool bLowPrecision = false) { m_NoRW = false; m_bDeci = false; m_bLowPrecision = bLowPrecision; return S_OK; };
	/// <summary> Initialize transform for 2 to 1 decimation </summary> 
	/// <returns> 
	///		- always returns S_OK
	/// </returns>
    HRESULT InitializeDecimate2to1(bool bLowPrecision = false) { m_NoRW = false; m_bDeci = true; m_bLowPrecision = bLowPrecision; return S_OK; };

    // no-copy initializaton routines - use with CTransformGraphNoSrcNode
    HRESULT Initialize(CImg& dst, CImg& src, bool bLowPrecision = false)
    { m_NoRW = true; dst.Share(m_dstImg); src.Share(m_srcImg); m_bDeci = false; m_bLowPrecision = bLowPrecision; return S_OK; }
    HRESULT InitializeDecimate2to1(CImg& dst, CImg& src, bool bLowPrecision = false)
    { m_NoRW = true; dst.Share(m_dstImg); src.Share(m_srcImg); m_bDeci = true; m_bLowPrecision = bLowPrecision; return S_OK; }

protected:
    bool m_bDeci;
    bool m_NoRW; // true if using no-copy
    CImg m_srcImg;
    CImg m_dstImg;
    bool m_bLowPrecision;
    // TODO: create temporary line buffer during Initialize and cache
};
#endif



/// \ingroup filter
/// <summary>Specialization of VtSeparableFilter that applies a 14641 filter
/// kernel to the provided source image while decimating by a factor
/// of two to one (so the output image is half the size of the input).  
/// The rctDst and ptSrc agruments enable the filtering operation to be 
/// applied to a sub-region of the source.</summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="rctDst"> Indicates the location and dimensions of the output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="ptSrcOrigin"> The origin of the input image </param>
/// <param name="ex"> The \link IMAGE_EXTEND extension mode \endlink to apply when the 
/// convolution kernel requires support outside of the source image</param>
/// <returns> 
/// - S_OK on success
/// - E_INVALIDDST if imgDst shares memory with imgSrc
/// - E_INVALIDARG if conversion from imgSrc to imgDst type is not supported
/// - E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///	- Implements all \ref stdconvrules.
/// - The filters can not be applied in place; the provided input and
/// output images must be different.
HRESULT
VtSeparableFilter14641Decimate2to1(CImg &imgDst, const CRect& rctDst, 
             				       const CImg &imgSrc, CPoint ptSrcOrigin,
			            	       const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend));
/// \ingroup filter
/// <summary>Specialization of VtSeparableFilter that applies a 14641 filter
/// kernel to the provided source image while interpolating by a factor of
/// two to one (output is twice the size of the input).  
/// The rctDst and ptSrc agruments enable the filtering operation to be 
/// applied to a sub-region of the source.</summary>
/// <param name="imgDst"> Output image. </param>
/// <param name="rctDst"> Indicates the location and dimensions of the output image. </param>
/// <param name="imgSrc"> Input image. </param>
/// <param name="ptSrcOrigin"> The origin of the input image </param>
/// <param name="ex"> The \link IMAGE_EXTEND extension mode \endlink to apply when the 
/// convolution kernel requires support outside of the source image</param>
/// <returns> 
/// - S_OK on success
/// - E_INVALIDDST if imgDst shares memory with imgSrc
/// - E_INVALIDARG if conversion from imgSrc to imgDst type is not supported
/// - E_OUTOFMEMORY if imgDst allocation was necessary and failed 
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///	- Implements all \ref stdconvrules.
/// - The filters can not be applied in place; the provided input and
/// output images must be different.
HRESULT
VtSeparableFilter14641Interpolate2to1(CImg &imgDst, const CRect& rctDst, 
             				          const CImg &imgSrc, CPoint ptSrcOrigin,
			            	          const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend));

#ifndef VT_NO_XFORMS
//+-----------------------------------------------------------------------------
//
// Class: CSeparableFilter14641Transform
// 
// Synopsis: Specialization of CSeparableFilterTransform for a 14641 filter
// kernel.
// 
/// \ingroup filtertransforms
/// <summary>Implementation of \link IImageTransform image transform \endlink
/// that applies a 14641 filter with two-to-one interpolation or decimation.
/// </summary> 
//------------------------------------------------------------------------------
class CSeparableFilter14641Transform: 
    public CImageTransformUnaryGeo<CSeparableFilter14641Transform, true>
{
	// IImageTransform implementation
public:
	void GetDstPixFormat(OUT int& frmtDst,
						 IN  const int* /*pfrmtSrcs*/, 
						 IN  UINT  /*uSrcCnt*/)
    { frmtDst = m_dstType; }

	vt::CRect GetRequiredSrcRect(const vt::CRect& rctDst)
	{ 
        CRect rctSrc;
        if (m_sampleType == 0)
        {
            // 2:1 upsample
            rctSrc = LayerRectAtLevel(rctDst, 1);
            rctSrc.InflateRect(1,1);
            // don't let source be smaller than 3x3 (happens for 1x1 input)
            if (rctSrc.Width() <= 2) { rctSrc.InflateRect(0,0,1,0); }
            if (rctSrc.Height() <= 2) { rctSrc.InflateRect(0,0,0,1); }
            VT_ASSERT(rctSrc.Width() >= 3);
            VT_ASSERT(rctSrc.Height() >= 3);
        }
        else if (m_sampleType == 1)
        {
            // 2:1 downsample
            rctSrc = CRect((rctDst.left-1)<<1,(rctDst.top-1)<<1,(rctDst.right<<1)+1,(rctDst.bottom<<1)+1);
        }
        else { rctSrc = CRect(0,0,0,0); VT_ASSERT(false); }
        return rctSrc;
    }

	vt::CRect GetAffectedDstRect(const vt::CRect& rctSrc)
	{
        CRect rctDst;
        if (m_sampleType == 0)
        {
            // 2:1 upsample
            rctDst = CRect((rctSrc.left+1)<<1,(rctSrc.top+1)<<1,(rctSrc.right-1)<<1,(rctSrc.bottom-1)<<1);
        }
        else if (m_sampleType == 1)
        {
            // 2:1 downsample
            rctDst = LayerRectAtLevel(rctSrc,1);
            rctDst.InflateRect(-1,-1);
        }
        else { rctDst = CRect(0,0,0,0); VT_ASSERT(false); }
        return rctDst;
    }

	vt::CRect GetResultingDstRect(const vt::CRect& rctSrc)
	{
        CRect rctDst;
        if (m_sampleType == 0)
        {
            // for 2:1 upsample
            rctDst = CRect(rctSrc.left<<1,rctSrc.top<<1,rctSrc.right<<1,rctSrc.bottom<<1);
        }
        else if (m_sampleType == 1)
        {
            // 2:1 downsample
            rctDst = LayerRectAtLevel(rctSrc,1);
        }
        else { rctDst = CRect(0,0,0,0); VT_ASSERT(false); }
        return rctDst;
	}

	HRESULT Transform(CImg* pimgDst, IN  const CRect& rctDst,
					  const CImg& imgSrc, const CPoint& ptSrc);

	virtual HRESULT Clone(ITaskState **ppState);
	
public:
    CSeparableFilter14641Transform()
    {}

	/// <summary> Initialize transform for two-to-one interpolation (upsampling).  
    /// Destination type must be a 4 band Byte or Float image.
    /// </summary> 
	/// <param name="dstType">The image type that the transform will generate.</param> 
	/// <returns> 
	///		- S_OK on success 
    ///     - E_INVALIDARG if dest type is not OBJ_RGBAIMG or OBJ_RGBAFLOATIMG
	///		- E_OUTOFMEMORY if memory allocation failed 
	/// </returns>
    HRESULT InitializeInterpolate2to1(int dstType);

    /// <summary> Initialize transform for two-to-one decimation (downsampling). 
    /// Destination type must be a Byte or Float image from 1 to 4 bands.
    /// </summary> 
	/// <param name="dstType">The image type that the transform will generate.</param> 
	/// <returns> 
	///		- S_OK on success 
    ///     - E_INVALIDARG if dest type is not Byte or Float image with 1 to 4 bands
	///		- E_OUTOFMEMORY if memory allocation failed 
	/// </returns>
    HRESULT InitializeDecimate2to1(int dstType);

protected:
	int     m_dstType;
    int     m_sampleType; // 0 - upsample 2:1; 1 - downsample 2:1
};

#endif

}; // end namespace vt
