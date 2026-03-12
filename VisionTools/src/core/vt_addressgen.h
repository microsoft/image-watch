//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines that generate addresses and bounding polygons for various 
//      image warping operations
//
//------------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_image.h"

namespace vt {

//+-----------------------------------------------------------------------------
//
// implementations of floor()
// 
//------------------------------------------------------------------------------
#if defined(VT_SSE_SUPPORTED)
inline void
floor2SSE(float* pD, const float* pS)
{
	__m128 x0f;
	vget_f32(x0f,0)=pS[0];
    vget_f32(x0f,1)=pS[1];
    vget_f32(x0f,2)=vget_f32(x0f,3)=0;

	__m128i x1i = _mm_cvttps_epi32(x0f);
	__m128  x2f = _mm_cvtepi32_ps(x1i);
    x2f = _mm_cmplt_ps(x0f, x2f);
	x1i = _mm_add_epi32(x1i, _mm_castps_si128(x2f));
	x2f = _mm_cvtepi32_ps(x1i);
	
	pD[0] = vget_f32(x2f,0);
	pD[1] = vget_f32(x2f,1);
}
#endif

inline void
floor2(float* pD, const float* pS)
{
	pD[0] = floorf(pS[0]);
	pD[1] = floorf(pS[1]);
}

#pragma warning(disable:28301) // disable requirement that base and derived class methods all have the same annotations

//+-----------------------------------------------------------------------------
//
// class: IAddressGenerator
//
/// \ingroup geofunc
/// <summary>The base interface for objects that generate source coordinates for 
/// warp operations. Example address generators are:
/// - 3x3 matrix transform of images
/// - lens distortion
/// - panoramic image deformations (cylinder, sphere, etc)
/// </summary>
//------------------------------------------------------------------------------
class IAddressGenerator
{
public:
	/// <summary> Destructor </summary>
	virtual ~IAddressGenerator()
	{ }
	
	/// <summary> Indicates whether the address generator has state and needs
	/// to be cloned if multiple instances are used simulaneously </summary>
	virtual bool RequiresCloneForConcurrency()
	{ return false;	}

	/// <summary> This function is called by the framework when the address
	/// generator needs to be cloned </summary>
	virtual HRESULT Clone(IAddressGenerator** ppClone) = 0;

	/// <summary>
	/// Given a rectangle in the destination image, return the rectangle in the 
	/// source image that maps to this destination region.
	/// </summary>
	/// <param name="rctDst">The rectangle in the destination image</param>
	/// <returns>The rectangle in the source image</returns>	 
	virtual vt::CRect MapDstRectToSrc(IN const CRect& rctDst) = 0;

	/// <summary>
	/// Given a rectangle in the source image, return the rectangle in the 
	/// destination image that maps to this source region.
	/// </summary>
	/// <param name="rctSrc">The rectangle in the source image</param>
	/// <returns>The rectangle in the destination image</returns>	 
	virtual vt::CRect MapSrcRectToDst(IN const CRect& rctSrc) = 0;

	/// <summary>
	/// Map a span of destination coordinates to corresponding source coordinates,
	/// where the destination coordinates are specified by 
	/// (ptDst.x, ptDst.y) to (ptDst.x+iW, ptDst.y)
	/// </summary>
	/// <param name="pSrcCoord">The output source coordinates.  The caller will
	/// provide an array of at least iSpan elements.</param> 
	/// <param name="ptDst">The origin of the span in destination coordinates.
	/// </param> 
	/// <param name="iW">The count of coordinates to generate.</param> 
    /// <returns>
	/// - S_OK on success
	/// - an implementation specific HRESULT on failure
    /// </returns>
	virtual HRESULT MapDstSpanToSrc(_Out_writes_(iW) CVec2f* pSrcCoord, 
									IN const vt::CPoint &ptDst, int iW) = 0;

	/// <summary>
	/// Map a span of destination coordinates to corresponding source coordinates,
	/// where the destination coordinates are specified as an array of coordinates.
	/// </summary>
	/// <param name="pSpan">In/Out parameter.  Is used to pass in the 
	/// destination coordinates, the method will convert the destination coordinates
	/// to source coordinates and return these in the same buffer.</param> 
	/// <param name="iSpan">The count of coordinates to generate.</param> 
    /// <returns>
	/// - S_OK on success
	/// - an implementation specific HRESULT on failure
    /// </returns>
	virtual HRESULT MapDstAddrToSrc(_Inout_updates_(iSpan) CVec2f* pSpan, int iSpan) = 0;

	/// <summary>
	/// Indicates that this address generator can generate values that wrap
	/// around the horizontal bounds of the image (for example a 360 degree
	/// panorama).
	/// </summary>
	/// <returns>
	/// If the returned value is 0 then the coordinates never wrap around the 
	/// horizontal bounds, if non-zero then the value is the maximum horizontal
	/// bound.
	/// </returns>
    virtual int AddressWrapX() { return 0; }

	/// <summary>
	/// Indicates that this address generator can generate values that wrap
	/// around the vertical bounds of the image (for example a 360 degree
	/// panorama).
	/// </summary>
	/// <returns>
	/// If the returned value is 0 then the coordinates never wrap around the 
	/// vertical bounds, if non-zero then the value is the maximum vertical
	/// bound.
	/// </returns>
    virtual int AddressWrapY() { return 0; }
};

// helper methods to implement IAddressGenerator::Clone
template<class T>
HRESULT CloneAddressGenerator(IAddressGenerator **ppClone)
{ 
	HRESULT hr = E_POINTER;
	if( ppClone )
	{
		*ppClone = VT_NOTHROWNEW T();
		hr = *ppClone? S_OK: E_OUTOFMEMORY;
	}
	return hr;
}

//
// utilities that address generator implementations use to flag 
// invalid addresses.  0x7fc00000 is the reserved float value for quiet NAN
// http://en.wikipedia.org/wiki/NaN
//
inline void SetAddressOOB(OUT float& addr)
{ *(int*)&addr = 0x7fc00000; }

inline void SetAddressOOB(OUT CVec2f& addr)
{ SetAddressOOB(addr.x); }

inline bool IsAddressOOB(float addr)
{ return ((*(int*)&addr) & 0x7f800000) == 0x7f800000; }

inline void SetAddressSpanOOB(OUT CVec2f* p, int iSpan)
{
	CVec2f pt;
	SetAddressOOB(pt.x);
	VtFillSpan(p, &pt, sizeof(pt), iSpan);
}

//+-----------------------------------------------------------------------------
//
// Class: CAddressGeneratorChain
//
/// <summary>
/// A simple container class for a chain of address generators
/// </summary>
//
//------------------------------------------------------------------------------
class CAddressGeneratorChain
{
public:
	IAddressGenerator** GetChain()
	{ return (IAddressGenerator**)m_vecAddrGen.begin(); }

	UInt32 GetChainLength() const
	{ return (UInt32)m_vecAddrGen.size(); }

	HRESULT SetChain(IAddressGenerator** ppGen, UInt32 uGenCnt)
	{
		Clear();
		HRESULT hr = m_vecAddrGen.resize(uGenCnt);
		for( UInt32 i = 0; i < uGenCnt && hr == S_OK; i++ )
		{
			hr = ppGen[i]->Clone((IAddressGenerator**)&m_vecAddrGen[i].p);
		}
		return hr;
	}

	HRESULT SetChain(CAddressGeneratorChain& src)
	{ return SetChain(src.GetChain(), src.GetChainLength()); }

	void Clear()
	{ m_vecAddrGen.clear(); }

protected:
	struct AG
	{
		IAddressGenerator* p;
		AG() : p(NULL)
		{}
		~AG() 
		{ delete p; }
	};
	vt::vector<AG> m_vecAddrGen;
};

//+-----------------------------------------------------------------------------
//
// function: TraverseAddressGenChain
//
/// \ingroup geofunc
/// <summary>
/// For a destination scan line specified by (ptDst.x, ptDst.y) to 
/// (ptDst.x+iW, ptDst.y) generate the source coordinates by traversing
/// the given set of address generators. 
/// </summary>
/// <param name="addrbuf">The output set of source coordinates for the destination
/// span.</param>
/// <param name="ptDst">The origin of the destination span in destination 
/// coordinates.</param> 
/// <param name="iW">The count of coordinates to generate.</param>
/// <param name="ppGenChain">An array of \link IAddressGenerator 
/// address generators \endlink to traverse.</param>
/// <param name="uGenChainCnt">The number of address generators in the ppGenChain
/// array.</param>
/// <returns>
/// - S_OK on success
/// - HRESULTs specific to the implementation of the provided ppGenChain.
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
/// The provided address generators will be traversed as follows:
/// \code
/// ppGenChain[0]->MapDstSpanToSrc(addrbuf, ptDst, iW);
/// for( int i = 1; i < uGenChainCnt; i++ )
///     ppGenChain[i]->MapDstAddrToSrc(addrbuf, iW);
/// \endcode
/// Note that this is a simplified code snippet meant to convey the traversal
/// order. The actual implementation is more complex as it checks for spans of 
/// out-of-bounds addresses generated by address generators and does not pass
/// these to subsequent generators.
//
//------------------------------------------------------------------------------

HRESULT
TraverseAddressGenChain(OUT CVec2f* addrbuf, const vt::CPoint& ptDst, int iW, 
						IAddressGenerator** ppGenChain, UInt32 uGenChainCnt);

inline HRESULT
TraverseAddressGenChain(IN OUT CVec2f* addrbuf, const vt::CPoint& ptDst, int iW, 
						CAddressGeneratorChain& chain)
{ 
	return TraverseAddressGenChain(addrbuf, ptDst, iW, 
								   chain.GetChain(), chain.GetChainLength());
}

//+-----------------------------------------------------------------------------
//
// function: WrapAddressesIntoSrcBuf
//
/// <summary>
/// Typically used after calling TraverseAddressGenChain to wrap generated
/// addresses into the coordinate system of a given source buffer.
/// </summary>
//
//------------------------------------------------------------------------------
void 
WrapAddressesIntoSrcBuf(CVec2f* addrbuf, int iWidth, const CRect& rctSrc,
						IAddressGenerator* pAG);

//+-----------------------------------------------------------------------------
//
// function: BoundingRectFromPoints
//
// Synopsis: utility function necessary for many address generators
//  
//------------------------------------------------------------------------------
vt::CRect BoundingRectFromPoints(const CVec2f* pPts, int n);

//+-----------------------------------------------------------------------------
//
// function: ClipPolyToRectHomogeneous
//
// Synopsis: Clips a set of homogeneous points to rctClip.
//           The max possible pDstCnt will be 2 X iSrcCnt.  The caller needs
//           to provide a pDst and pTmp that are at least that size.
//
//------------------------------------------------------------------------------
void ClipPolyToRectHomogeneous(OUT CVec3f* pDst, OUT int* pDstCnt, 
							   OUT CVec3f* pTmp, IN  CVec3f* pSrc, int iSrcCnt,
							   const vt::CRect& rctClip);

//+-----------------------------------------------------------------------------
//
// function: MapDstRectToSrc/MapSrcRectToDst
// 
// Synopsis: a collection of functions that map a rectangle in a destination
//           image to a rect in a source image given an address transformation
// 
//------------------------------------------------------------------------------
inline HRESULT
MapDstRectToSrc(OUT vt::CRect& rctSrc, IN const vt::CRect& rctDst, 
				IAddressGenerator** ppGenChain, UInt32 uGenChainCnt )
{ 
	HRESULT hr = ppGenChain? (uGenChainCnt? S_OK: E_INVALIDARG): E_POINTER; 
	for( UInt32 i = 0; i < uGenChainCnt && hr == S_OK; i++ )
	{
		rctSrc = ppGenChain[i]->MapDstRectToSrc(i==0? rctDst: rctSrc);
	}
	return hr;
}
inline HRESULT
MapDstRectToSrc(OUT vt::CRect& rctSrc, IN const vt::CRect& rctDst,
				CAddressGeneratorChain& chain)
{ 
	return MapDstRectToSrc(rctSrc, rctDst, chain.GetChain(), 
							 chain.GetChainLength());
}

inline HRESULT
MapSrcRectToDst(OUT vt::CRect& rctDst, IN const vt::CRect& rctSrc, 
				IAddressGenerator** ppGenChain, UInt32 uGenChainCnt )
{
	HRESULT hr = ppGenChain? (uGenChainCnt? S_OK: E_INVALIDARG): E_POINTER; 
	for( UInt32 i = 0; i < uGenChainCnt && hr == S_OK; i++ )
	{
		rctDst = ppGenChain[uGenChainCnt-1-i]->MapSrcRectToDst(
			i==0? rctSrc: rctDst);
	}
	return hr;
}
inline HRESULT
MapSrcRectToDst(OUT vt::CRect& rctDst, IN const vt::CRect& rctSrc,
				CAddressGeneratorChain& chain)
{ 
	return MapSrcRectToDst(rctDst, rctSrc, chain.GetChain(), 
						   chain.GetChainLength());
}

//+-----------------------------------------------------------------------------
//
// Class: CFlowFieldAddressGen
// 
/// \ingroup geofunc
/// <summary>
/// An implementation of IAddressGenerator that computes source coordinates by
/// looking them up in a per-pixel destination-to-source coordinate map.
/// </summary>
// 
//------------------------------------------------------------------------------
class CFlowFieldAddressGen: public IAddressGenerator
{
public:
	virtual vt::CRect MapDstRectToSrc(IN const CRect& rctDst);
	virtual vt::CRect MapSrcRectToDst(IN const CRect& rctSrc);
	virtual HRESULT   MapDstSpanToSrc(_Out_writes_(iSpan) CVec2f* pSpan, 
									  IN const vt::CPoint &ptDst, int iSpan);
	virtual HRESULT   MapDstAddrToSrc(_Inout_updates_(iSpan) CVec2f* pSpan, int iSpan);
    virtual HRESULT   Clone(IAddressGenerator **ppClone)
	{
		HRESULT hr = CloneAddressGenerator<CFlowFieldAddressGen>(ppClone);
		if( hr == S_OK )
		{
			hr = ((CFlowFieldAddressGen*)*ppClone)->Initialize(
				m_imgMap, m_bOffsetMode);
			if( hr != S_OK )
			{
				delete *ppClone;
				*ppClone = NULL;
			}
		}
		return hr;
	}

public:

	/// <summary>Initialize the destination-to-source coordinate map.</summary>
	/// <param name="imgMap">A 2 band \ref CVec2fImgAnchor "vec2 image" that 
	/// encodes the destination-to-source coordinate map.</param>
	/// <param name="bOffsetMap">If false then the imgMap image represents an
	/// absolute address into the source image, otherwise it is an offset from 
	/// the current address.</param>
    /// <returns>
	/// - S_OK on success
	/// - E_NOINIT if imgMap is empty
	/// </returns>
    HRESULT Initialize(const CVec2Img& imgMap, bool bOffsetMap=false)
	{
		m_bOffsetMode = bOffsetMap; 
		return imgMap.Share(m_imgMap); 
    }

protected:
	bool     m_bOffsetMode;
    CVec2Img m_imgMap;
};

//+-----------------------------------------------------------------------------
//
// Class: CFlowFieldXYAddressGen
// 
/// \ingroup geofunc
/// <summary>
/// An implementation of IAddressGenerator that computes source coordinates by
/// looking them up in a per-pixel destination to source map.  The flow field
/// in this implementation is represented as separate X map and Y map images.
/// </summary>
// 
//------------------------------------------------------------------------------
class CFlowFieldXYAddressGen: public IAddressGenerator
{
public:
	virtual vt::CRect MapDstRectToSrc(IN const CRect& rctDst);
	virtual vt::CRect MapSrcRectToDst(IN const CRect& rctSrc);
	virtual HRESULT   MapDstSpanToSrc(_Out_writes_(iSpan) CVec2f* pSpan, 
									  IN const vt::CPoint &ptDst, int iSpan);
	virtual HRESULT   MapDstAddrToSrc(_Inout_updates_(iSpan) CVec2f* pSpan, int iSpan);
    virtual HRESULT   Clone(IAddressGenerator **ppClone)
	{
		HRESULT hr = CloneAddressGenerator<CFlowFieldXYAddressGen>(ppClone);
		if( hr == S_OK )
		{
			hr = ((CFlowFieldXYAddressGen*)*ppClone)->Initialize(
				m_imgMapX, m_imgMapY, m_bOffsetMode, m_fieldScale);
			if( hr != S_OK )
			{
				delete *ppClone;
				*ppClone = NULL;
			}
		}
		return hr;
	}

public:

	/// <summary>Initialize the destination-to-source coordinate map.</summary>
	/// <param name="imgMapX">A single band float image that 
	/// encodes the destination-to-source X coordinate.</param> 
	/// <param name="imgMapY">A single band float image that 
	/// encodes the destination-to-source Y coordinate.</param>
	/// <param name="bOffsetMap">If false then the imgMap image represents an
	/// absolute address into the source image, otherwise it is an offset from 
	/// the current address.</param>
	/// <param name="fieldScale">A float value that is used to scale the field
    /// during address generation when bOffsetMap is true.  The valid range is
    /// 0.f to 1.f, and the default is 1.f.</param>
    /// <returns>
	/// - S_OK on success
	/// - E_NOINIT if imgMapX or imgMapY are empty
	/// - E_INVALIDARG if imgMapX or imgMapY have different dimensions or are not 1 band
    /// - E_INVALIDARG is fieldScale is not between 0.f and 1.f
	/// </returns>
    HRESULT Initialize(const CFloatImg& imgMapX, const CFloatImg& imgMapY, 
        bool bOffsetMap=false, float fieldScale=1.f)
	{
		m_bOffsetMode = bOffsetMap; 
        m_fieldScale = fieldScale;

        if( imgMapX.Width()  != imgMapY.Width()  ||
			imgMapX.Height() != imgMapY.Height() ||
			imgMapX.Bands() != 1 || imgMapY.Bands() != 1 ||
            fieldScale < 0.f || fieldScale > 1.f )
		{
			return E_INVALIDARG;
		}

		HRESULT hr = imgMapX.Share(m_imgMapX);
		if( hr == S_OK )
		{
			hr = imgMapY.Share(m_imgMapY);
		}
		return hr;
    }

    HRESULT Update(float fieldScale)
    {
        if( fieldScale < 0.f || fieldScale > 1.f )
		{
			return E_INVALIDARG;
		}
        m_fieldScale = fieldScale;
        return S_OK;
    }

protected:
	bool      m_bOffsetMode;
    float     m_fieldScale;
    CFloatImg m_imgMapX;
    CFloatImg m_imgMapY;
};

//+-----------------------------------------------------------------------------
//
// Class: C3x3TransformAddressGen
// 
// Synopsis: An address generator that creates addresses based on a specified
//           3x3 transform.
// 
//------------------------------------------------------------------------------
template <typename T> bool
IsMatrixAffine(const CMtx3x3<T>& m, const CRect& rctDomain);

template <typename T> bool
IsMatrixAnisoScaleTrans(const CMtx3x3<T>& m, const CRect& rctDomain);

vt::CRect 
MapRegion3x3(IN const CMtx3x3f& mat, const vt::CRect& rctIn, 
			 const vt::CRect* prctOutClip=NULL);

vt::CRect
MapRegion3x3Ex(IN const CMtx3x3f& mat, const vt::CRect& rctIn, int& clipcnt, int& negzcnt,
const vt::CRect* prctOutClip = NULL);

/// \ingroup geofunc
/// <summary>
/// An implementation of IAddressGenerator that computes source coordinates by
/// transforming destination coordinates through a 3x3 matrix.
/// </summary>
class C3x3TransformAddressGen: public IAddressGenerator
{
public:
    virtual vt::CRect MapDstRectToSrc(const CRect &rctDst)
	{ return vt::MapRegion3x3(m_xfrm, rctDst, &m_rctSrcClip); }

    virtual vt::CRect MapSrcRectToDst(const CRect &rctSrc)
	{ return vt::MapRegion3x3(m_invxfrm, rctSrc, &m_rctDstClip); }

	virtual HRESULT   MapDstSpanToSrc(_Out_writes_(iSpan) CVec2f* pSpan, 
									  IN  const CPoint &ptDst, int iSpan);
	virtual HRESULT   MapDstAddrToSrc(_Inout_updates_(iSpan) CVec2f* pSpan, int iSpan);

	virtual HRESULT   Clone(IAddressGenerator **ppClone)
	{ 
		HRESULT hr = CloneAddressGenerator<C3x3TransformAddressGen>(ppClone);
		if( hr == S_OK )
		{
			((C3x3TransformAddressGen*)*ppClone)->Initialize3x3(
				m_xfrm, m_rctSrcClip, m_rctDstClip);
		}
		return hr;
	}

public:
	C3x3TransformAddressGen()
	{}

	/// <summary>Initialze the address generator.</summary>
	/// <param name="xfrm">The 3x3 matrix that maps destination coordinates to
	/// source coordinates.</param>
	/// <param name="rctSrcClip">The clip rectangle on the source. When source
	/// coordinates fall outside these boundaries, the returned coordinates will
	/// be flagged as out-of-bounds.</param>   
	/// <param name="rctDstClip">The clip rectangle on the destination. This is
	/// only used in the MapSrcRectToDst function to clip the returned value.
	/// </param>   
	C3x3TransformAddressGen(const CMtx3x3f& xfrm, const vt::CRect& rctSrcClip,
							const vt::CRect& rctDstClip) 
	{ Initialize3x3(xfrm, rctSrcClip, rctDstClip); }

	/// <summary>Initialze the address generator.</summary>
	/// <param name="xfrm">The 3x3 matrix that maps destination coordinates to
	/// source coordinates.</param>
	/// <param name="rctSrcClip">The clip rectangle on the source. When source
	/// coordinates fall outside these boundaries, the returned coordinates will
	/// be flagged as out-of-bounds.</param>   
	/// <param name="rctDstClip">The clip rectangle on the destination. This is
	/// only used in the MapSrcRectToDst function to clip the returned value.
	/// </param>   
    void Initialize3x3(const CMtx3x3f& xfrm, const vt::CRect& rctSrcClip, 
					   const vt::CRect& rctDstClip)
	{ 
        CMtx3x3f xfrmNorm = (xfrm(2,2)==0)? xfrm: (xfrm/xfrm(2,2));
		m_bIsAffine  = IsMatrixAffine(xfrmNorm, rctDstClip);
		m_xfrm       = m_bIsAffine? xfrmNorm: xfrm;
        // (conditionally) make matrix fully affine so MapDstRectToSrc always matches generated addresses
        if (m_bIsAffine) { m_xfrm[2][0] = m_xfrm[2][1] = 0.f; m_xfrm[2][2] = 1.f; };
		m_invxfrm    = m_xfrm.Inv();
		m_rctSrcClip = rctSrcClip; 
		m_rctDstClip = rctDstClip;
	}

protected:
	bool      m_bIsAffine;
    CMtx3x3f  m_xfrm;
	CMtx3x3f  m_invxfrm;
	vt::CRect m_rctSrcClip;
	vt::CRect m_rctDstClip;
};

};
