//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for image warping
//
//  History:
//      2004/11/08-mattu
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_atltypes.h"
#include "vt_image.h"
#include "vt_addressgen.h"
#include "vt_separablefilter.h"

#ifndef VT_NO_XFORMS
#include "vt_transform.h"
#endif

namespace vt {

/// \ingroup geofunc
/// \anchor eSamplerKernelAnchor
/// <summary>Defines the kernel to use in \link VtResizeImage() resize \endlink
/// and \link VtWarpImage() warp \endlink operations.
/// </summary>

enum eSamplerKernel
{
	eSamplerKernelNearest  = 0, ///< Nearest neighbor sampling
	eSamplerKernelBilinear = 1, ///< Bilinear sampling
	eSamplerKernelBicubic  = 2, ///< Bicubic sampling
	eSamplerKernelLanczos2 = 3, ///< 3 lobed windowed sinc using approximation for resize with large decimation
	eSamplerKernelLanczos3 = 4, ///< 5 lobed windowed sinc using approximation for resize with large decimation
	eSamplerKernelLanczos4 = 5, ///< 7 lobed windowed sinc using approximation for resize with large decimation
	eSamplerKernelTriggs5  = 6,
    eSamplerKernelBicubicBSplineSrcPreprocessed = 7, /// Bspline kernel, image must be preprocessed once, then same cost as bicubic.
    eSamplerKernelBicubicBSpline = 8, /// Bspline kernel, preprocessing done internally/automatically, at moderate speed cost.
	eSamplerKernelLanczos2Precise = 9,  ///< 3 lobed windowed sinc 'precise' (no approximation)
	eSamplerKernelLanczos3Precise = 10, ///< 5 lobed windowed sinc 'precise' (no approximation)
	eSamplerKernelLanczos4Precise = 11, ///< 7 lobed windowed sinc 'precise' (no approximation)
};

/// \ingroup geofunc
/// <summary>Used by \link VtResizeImage() VtResizeImage \endlink.  Describes 
/// the phase of the output pixel grid vs. the input pixel grid.  When the
/// output is half the size of the input - Primal means output pixels align
/// with even coordinate pixels in the input, Dual means output pixels are 
/// generated half-way between even and odd coordinate pixels.
/// </summary>

enum eResizeSamplingScheme
{
	eResizeSamplingSchemePrimal = 0, 
	eResizeSamplingSchemeDual   = 1
};

/// \ingroup geofunc
/// <summary>Performs a geometric warp of an image, where the warp is expressed
/// as a 3x3 perspective matrix.</summary>
/// <param name="imgDst">Output warped image.</param> 
/// <param name="rctDst">Indicates the location and dimensions of the output image. 
/// </param>
/// <param name="imgSrc">Input image.</param> 
/// <param name="xfrm">The 3 x 3 transform that maps ouput image coordinates
/// to input image coordinates.</param>
/// <param name="sampler">Describes which \ref eSamplerKernelAnchor "sampler kernel" 
/// to use, the default is bilinear.</param>
/// <param name="ex"> The \link IMAGE_EXTEND extension mode \endlink to apply 
/// when the sampler requires support outside of the input image.  The
/// default is Zero.</param>
/// <DL><DT> Remarks:</DT></DL>
///	- Implements all \ref stdconvrules.
/// - The warp can not be applied in place; the provided input and
/// output images must be different.
/// - As with all transforms in Vision Tools, VtWarpImage uses column 
/// vectors to represent image coordinates.  In this case, the output image
/// coordinates will be expressed as a 3-element column vector in homogeneous 
/// coordinates, which will be multiplied by the `xfrm` parameter to produce an
/// input image coordinate.
/// - The most general form of "xfrm" will perform a perspective distortion.
/// VtWarpImage will analyze the provided "xfrm" and run optimized code when 
/// a simpler transform is provided.  There are optimized code paths for both
/// affine and scale+translate transform.
/// - The sampler must be either Bilinear or Bicubic.  More sophisticated
/// samplers are not supported for the warp operation.

HRESULT 
VtWarpImage( CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
			 const CMtx3x3f& xfrm, eSamplerKernel sampler = eSamplerKernelBilinear,
			 const IMAGE_EXTEND& ex = IMAGE_EXTEND(Zero) );

/// \ingroup geofunc
/// <summary>Performs a geometric warp of an image, where the warp is expressed
/// as a chain of address generators.</summary>
/// <param name="imgDst">Output warped image.</param> 
/// <param name="rctDst">Indicates the location and dimensions of the output image. 
/// </param>
/// <param name="imgSrc">Input image.</param> 
/// <param name="ppGenChain">Pointer to an array of 
/// \link IAddressGenerator address generators \endlink</param> 
/// <param name="uGenChainCnt">Number of address generators in the ppGenChain
/// array.</param>
/// <param name="sampler">Describes which \ref eSamplerKernelAnchor "sampler kernel" 
/// to use, the default is bilinear.</param>
/// <param name="ex"> The \link IMAGE_EXTEND extension mode \endlink to apply 
/// when the sampler requires support outside of the input image.  The
/// default is Zero.</param>
/// <DL><DT> Remarks:</DT></DL>
///	- Implements all \ref stdconvrules.
/// - The warp can not be applied in place; the provided input and
/// output images must be different.
/// - The sampler must be either Bilinear or Bicubic.  More sophisticated
/// samplers are not supported for the warp operation.

HRESULT
VtWarpImage( CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
			 IAddressGenerator** ppGenChain, UInt32 uGenChainCnt, 
			 eSamplerKernel sampler = eSamplerKernelBilinear,
			 const IMAGE_EXTEND& ex = IMAGE_EXTEND(Zero) );

/// \ingroup geofunc
/// <summary>Performs a geometric warp of an image, where the warp is expressed
/// as a single address generator.</summary>
inline HRESULT 
VtWarpImage( CImg& imgDst, const CRect& rctDst, const CImg& imgSrc,
			 IAddressGenerator* pAddressGen, 
			 eSamplerKernel sampler = eSamplerKernelBilinear, 
			 const IMAGE_EXTEND& ex = IMAGE_EXTEND(Zero)  )
{ return VtWarpImage(imgDst, rctDst, imgSrc, &pAddressGen, 1, sampler, ex); }

/// \ingroup geofunc
/// <summary>Performs a geometric warp of an image, where the warp is expressed
/// as a per-pixel coordinate map.</summary>
/// <param name="imgDst">Output warped image.</param> 
/// <param name="rctDst">Indicates the location and dimensions of the output image. 
/// </param>
/// <param name="imgSrc">Input image.</param> 
/// <param name="imgMap">An 2 element \ref CVec2fImgAnchor "vec2 image" that 
/// encodes the destination-to-source coordinate map.  This image needs to have 
/// the same width and height as rctDst.</param>
/// <param name="sampler">Describes which \ref eSamplerKernelAnchor "sampler kernel" 
/// to use, the default is bilinear.</param>
/// <param name="ex"> The \link IMAGE_EXTEND extension mode \endlink to apply 
/// when the sampler requires support outside of the input image.  The
/// default is Zero.</param>
/// <param name="bOffsetMap">If false then the imgMap image represents an
/// absolute address into the source image, otherwise it is an offset from the
/// current address.</param>
/// <DL><DT> Remarks:</DT></DL>
///	- Implements all \ref stdconvrules.
/// - The warp can not be applied in place; the provided input and
/// output images must be different.
/// - For simple cases, it is more efficient to provide an IAddressGenerator 
/// that procedurally generates the source coordinates.  This imgMap version 
/// is preferable when the coordinate transforms are expensive to compute and
/// will be re-used multiple times.
/// - The sampler must be either Bilinear or Bicubic.  More sophisticated
/// samplers are not supported for the warp operation.

inline HRESULT 
VtWarpImage( CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
			 const CVec2Img& imgMap, 
			 eSamplerKernel sampler = eSamplerKernelBilinear,
			 const IMAGE_EXTEND& ex = IMAGE_EXTEND(Zero),
			 bool bOffsetMap=false )
{ 
	HRESULT hr;
	CFlowFieldAddressGen a;
	if( (hr = a.Initialize(imgMap, bOffsetMap)) == S_OK )
	{
		hr = VtWarpImage(imgDst, rctDst, imgSrc, &a, sampler, ex);
	}
	return hr;
}

/// \ingroup geofunc
/// <summary>Performs an image resize, where the resize is expressed
/// as an independent scale in X and Y and independent translate in X and Y
/// </summary>
/// <param name="imgDst">Output resized image.</param> 
/// <param name="rctDst">Indicates the location and dimensions of the output image. 
/// </param>
/// <param name="imgSrc">Input image.</param>
/// <param name="sx">The scale in x.  The x coordinate of where to sample
/// the source image will be determined by dst_x * sx + tx</param>
/// <param name="tx">The translate in x.  See description of sx.</param>
/// <param name="sy">The scale in y. See description of sx.</param>
/// <param name="ty">The translate in y.  See description of sx.</param> 
/// <param name="sampler">Describes which \ref eSamplerKernelAnchor "sampler kernel" 
/// to use, the default is eSamplerKernelLanczos3.</param>
/// <param name="ex"> The \link IMAGE_EXTEND extension mode \endlink to apply 
/// when the sampler requires support outside of the input image.  The
/// default is Extend.</param>
/// <DL><DT> Remarks:</DT></DL>
///	- Implements all \ref stdconvrules.
/// - The resize can not be applied in place; the provided input and
/// output images must be different.

HRESULT 
VtResizeImage(CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
			  float sx, float tx, float sy, float ty,
			  eSamplerKernel sampler = eSamplerKernelLanczos3,
			  const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend));


/// \ingroup geofunc
/// <summary>Performs an image resize, where the resize is expressed
/// as a pair of rationals - a rational that describes the X resize and a 
/// rational for the Y resize.
/// </summary>
/// <param name="imgDst">Output resized image.</param> 
/// <param name="rctDst">Indicates the location and dimensions of the output image. 
/// </param>
/// <param name="imgSrc">Input image.</param>
/// <param name="iSrcSamplesX">This parameter plus the next "iDstSamplesX 
/// describes the ratio of source width to destination width.</param>
/// <param name="iDstSamplesX">See description of iSrcSamplesX.</param>
/// <param name="iSrcSamplesY">This parameter plus the next "iDstSamplesY 
/// describes the ratio of source height to destination height.</param>
/// <param name="iDstSamplesY">See description of iSrcSamplesY.</param>
/// <param name="sampler">Describes which \ref eSamplerKernelAnchor "sampler kernel" 
/// to use, the default is eSamplerKernelLanczos3.</param>
/// <param name="ex"> The \link IMAGE_EXTEND extension mode \endlink to apply 
/// when the sampler requires support outside of the input image.  The
/// default is Extend.</param>
/// <param name="eSS">The sampling scheme to use.  Default is Primal.</param>
/// <DL><DT> Remarks:</DT></DL>
///	- Implements all \ref stdconvrules.
/// - The resize can not be applied in place; the provided input and
/// output images must be different.

HRESULT
VtResizeImage(CImg& imgDst, const CRect& rctDst, const CImg& imgSrc, 
			  int iSrcSamplesX, int iDstSamplesX, 
			  int iSrcSamplesY, int iDstSamplesY,
			  eSamplerKernel sampler = eSamplerKernelLanczos3,
			  const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend),
			  eResizeSamplingScheme eSS = eResizeSamplingSchemePrimal);

/// \ingroup geofunc
/// <summary>Performs an image resize, where the resize is expressed
/// as a rectangle describing the destination image.</summary>
/// <param name="imgDst">Output resized image.</param> 
/// <param name="rctDst">Indicates the location and dimensions of the output image. 
/// </param>
/// <param name="imgSrc">Input image.</param>
/// <param name="sampler">Describes which \ref eSamplerKernelAnchor "sampler kernel" 
/// to use, the default is eSamplerKernelLanczos3.</param>
/// <param name="ex"> The \link IMAGE_EXTEND extension mode \endlink to apply 
/// when the sampler requires support outside of the input image.  The
/// default is Extend.</param>
/// <param name="eSS">The sampling scheme to use. Default is Primal</param>
/// <DL><DT> Remarks:</DT></DL>
///	- Implements all \ref stdconvrules.
/// - The resize can not be applied in place; the provided input and
/// output images must be different.

inline HRESULT 
VtResizeImage(CImg& imgDst, const CRect& rctDst, const CImg& imgSrc,
			  eSamplerKernel sampler = eSamplerKernelLanczos3,
			  const IMAGE_EXTEND& ex = IMAGE_EXTEND(Extend),
			  eResizeSamplingScheme eSS = eResizeSamplingSchemePrimal)
{
	return VtResizeImage(imgDst, rctDst, imgSrc, imgSrc.Width(), rctDst.Width(),
						 imgSrc.Height(), rctDst.Height(), sampler, ex, eSS);
}

#ifndef VT_NO_XFORMS

/// \ingroup geotransforms
/// <summary>Implementation of \link IImageTransform image transform \endlink that performs
/// a warp or resize geometric transformation.
/// </summary> 

class CWarpTransform: public CImageTransformUnaryGeo<CWarpTransform, true>
{
public:

    virtual bool RequiresCloneForConcurrency()
    {
        return (m_NoRW)?(false):(true);
    }

	virtual void GetSrcPixFormat(IN OUT int* pfrmtSrcs, 
								 IN UInt32  uSrcCnt,
								 IN int frmtDst);

	virtual void GetDstPixFormat(OUT int& frmtDst,
								 IN  const int* pfrmtSrcs, 
								 IN  UInt32  uSrcCnt);

	vt::CRect GetRequiredSrcRect(const vt::CRect& rctDst);

	vt::CRect GetAffectedDstRect(const vt::CRect& rctSrc);

	vt::CRect GetResultingDstRect(const vt::CRect& rctSrc);

	HRESULT Transform(CImg* pimgDst, const vt::CRect& rctDst, 
					  const CImg& src, const vt::CPoint& ptSrc);

	virtual HRESULT Clone(ITaskState **ppState);

public:
	CWarpTransform() : m_pFilterTransform(NULL), m_bIsResize(false), m_NoRW(false)
    {}

	~CWarpTransform()
	{ Clear(); }
 
	/// <summary>Performs a geometric warp transform, where the warp is expressed
	/// as a 3x3 perspective matrix.</summary>
	/// <param name="xfrm">The 3 x 3 transform that maps ouput image coordinates
	/// to input image coordinates.</param>
	/// <param name="rctSrcClip">A clipping rectangle that defines sampling
	/// bounds on the input image.  Note that this can be larger than the
	/// input image if the sampler needs to access extension modes other than
	/// Zero.</param>
	/// <param name="rctDstClip">The dimensions of the output image.</param>
	/// <param name="dstType">The pixel format of the result. If necessary the
    /// transform will convert the supplied source to this format.  The default
	/// value indicates that the type will the same as the source image.</param>
	/// <param name="sampler">Describes which \ref eSamplerKernelAnchor "sampler kernel" 
	/// to use, the default is bilinear.</param>
	/// <DL><DT> Remarks:</DT></DL>
	/// - the sampler must be either Bilinear, Bicubic or one of the Bicubic B-Splines. 
    /// The other more sophisticated samplers are not supported for non-resize transforms.
	HRESULT Initialize(const CMtx3x3f& xfrm, const vt::CRect& rctSrcClip,
		               const vt::CRect& rctDstClip,
					   int dstType = OBJ_UNDEFINED,
					   eSamplerKernel sampler = eSamplerKernelBilinear);

	/// <summary>Performs a geometric warp trasnform, where the warp is expressed
	/// as a chain of address generators.</summary>
	/// <param name="ppGenChain">Pointer to an array of 
	/// \link IAddressGenerator address generators \endlink</param> 
	/// <param name="uGenChainCnt">Number of address generators in the ppGenChain
	/// array.</param>
	/// <param name="dstType">The pixel format of the result. If necessary the
    /// transform will convert the supplied source to this format.  The default
	/// value indicates that the type will the same as the source image.</param>
	/// <param name="sampler">Describes which \ref eSamplerKernelAnchor "sampler kernel" 
	/// to use, the default is bilinear.</param>
	/// <DL><DT> Remarks:</DT></DL>
	/// - the sampler must be either Bilinear, Bicubic or one of the Bicubic B-Splines. 
    /// The other more sophisticated samplers are not supported for non-resize transforms.

	HRESULT Initialize(IAddressGenerator** ppGenChain, UInt32 uGenChainCnt,
					   int dstType = OBJ_UNDEFINED,
					   eSamplerKernel sampler = eSamplerKernelBilinear);

	/// <summary>Performs a geometric warp of an image, where the warp is expressed
	/// as a single address generator.</summary>

	HRESULT Initialize(IAddressGenerator* pAddressGen,
					   int dstType = OBJ_UNDEFINED,
					   eSamplerKernel sampler = eSamplerKernelBilinear)
	{ return Initialize(&pAddressGen, 1, dstType, sampler); }


	/// <summary>Performs a geometric warp of an image, where the warp is expressed
	/// as a per-pixel coordinate map.</summary>
	/// <param name="imgMap">An 2 element \ref CVec2fImgAnchor "vec2 image" that 
	/// encodes the destination-to-source coordinate map.</param>
	/// <param name="dstType">The pixel format of the result. If necessary the
    /// transform will convert the supplied source to this format.  The default
	/// value indicates that the type will the same as the source image.</param>
	/// <param name="sampler">Describes which \ref eSamplerKernelAnchor "sampler kernel" 
	/// to use, the default is bilinear.</param>
	/// <DL><DT> Remarks:</DT></DL>
	/// - the sampler must be either Bilinear, Bicubic or one of the Bicubic B-Splines. 
    /// The other more sophisticated samplers are not supported for non-resize transforms.

	HRESULT Initialize(const CVec2Img& imgMap,
					   int dstType = OBJ_UNDEFINED,
					   eSamplerKernel sampler = eSamplerKernelBilinear);

	/// <summary>Performs an resize transform, where the resize is expressed
	/// as an independent scale in X and Y and independent translate in X and Y
	/// </summary>
	/// <param name="sx">The scale in x.  The x coordinate of where to sample
	/// the source image will be determined by dst_x * sx + tx</param>
	/// <param name="tx">The translate in x.  See description of sx.</param>
	/// <param name="sy">The scale in y. See description of sx.</param>
	/// <param name="ty">The translate in y.  See description of sx.</param> 
	/// <param name="dstType">The pixel format of the result. If necessary the
    /// transform will convert the supplied source to this format.  The default
	/// value indicates that the type will the same as the source image.</param>
	/// <param name="sampler">Describes which \ref eSamplerKernelAnchor "sampler kernel" 
	/// to use, the default is bilinear.</param>

	HRESULT InitializeResize(float sx, float tx, float sy, float ty,
							 int dstType = OBJ_UNDEFINED,
							 eSamplerKernel sampler = eSamplerKernelLanczos3);

    // initialize routine for no-copy transform using warp-special only; returns
    // E_FAIL if the input characteristics are not supported by warp-special
    // (see SelectWarpSpecial in warp_special.h)
	HRESULT Initialize(const CMtx3x3f& xfrm, const vt::CImg& src, vt::CImg& dst,
					   eSamplerKernel sampler = eSamplerKernelBilinear,
                       const IMAGE_EXTEND& ex = IMAGE_EXTEND(Zero));
protected:
	void Clear()
	{ 
		m_genChain.Clear();
		delete m_pFilterTransform;
		m_pFilterTransform = NULL;
    }

protected:
	CAddressGeneratorChain m_genChain;
	int                    m_dstType;
	eSamplerKernel         m_sampler;

    bool     m_bDownsample;
	bool     m_bIsResize;
	CMtx3x3f m_mtxResize;
	CSeparableFilterTransform* m_pFilterTransform;

    // for no-copy transform
    bool         m_NoRW;
    CMtx3x3f     m_xfrm;
    CImg         m_src;
    CImg         m_dst;
    IMAGE_EXTEND m_extend;
    HRESULT (VT_STDCALL *m_WarpBlockFunc)(CImg& imgDstBlk, const CPoint& ptDst, 
        const CImg& imgSrc, const CMtx3x3f* pMtx, const IMAGE_EXTEND& ex);
};

#endif

};