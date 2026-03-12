//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for filter kernels
//
//  History:
//      2008/2/27-swinder
//          Created
//
//------------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_atltypes.h"

namespace vt {

/// \ingroup filter
/// <summary>Class that contains a one dimensionsal filter kernel.</summary> 
class C1dKernel
{
public:
    C1dKernel() : m_iTaps(0), m_iCenter(0)
	{}

	/// <summary>Allocate a new filter kernel and optionally initialize it.</summary> 
	/// <param name="iTaps">The number of kernel taps.</param> 
	/// <param name="iCenter">The center tap of the kernel (TODO: see diagram).</param> 
	/// <param name="pfK">Optional parameter.  If non-null this pointer to source
	/// data that will be used to initialize the iTaps of the kernel.</param> 
	/// <returns> 
	///		- S_OK on success 
	///		- E_OUTOFMEMORY if memory allocation failed 
	/// </returns>
    HRESULT Create(int iTaps, int iCenter, const float *pfK=NULL);

	/// <summary>Create a copy of an existing kernel.</summary> 
	/// <param name="k">The kernel to copy into this kernel.</param> 
	/// <returns> 
	///		- S_OK on success 
	///		- E_OUTOFMEMORY if memory allocation failed 
	/// </returns>
    HRESULT Create(const C1dKernel &k)
	{ return (&k == this)? S_OK: Create(k.Width(), k.Center(), k.Ptr()); }

	/// <summary>Set the kernel values.</summary> 
	/// <param name="pfK">Pointer to source data that will be used to initialize 
	/// the kernel.  pfK should point to at least kernel width number of taps.
	/// </param> 
	void Set(const float *pfK) 
	{ memcpy(m_cMem.Ptr(), pfK, Width() * sizeof(float)); }

	/// <summary>Return a pointer to the kernel taps.</summary> 
	float *Ptr()                           
	{ return (float*)m_cMem.Ptr(); }

	/// <summary>Return a pointer to the kernel taps.</summary> 
	const float *Ptr() const
	{ return (float*)m_cMem.Ptr(); }

	/// <summary>Kernel value access, returns the i-th kernel tap.</summary> 
	float &operator[] (int i)             
	{ return Ptr()[i]; }

	/// <summary>Kernel value access, returns the i-th kernel tap.</summary> 
	const float &operator[] (int i) const
	{ return Ptr()[i]; }

	/// <summary>Return the number of kernel taps.</summary> 
    int Width()  const 
	{ return m_iTaps; }

	/// <summary>Return the kernel center tap.</summary> 
	int Center() const
	{ return m_iCenter; }

private: 
	// Copying is verboten	
	C1dKernel(const C1dKernel&);
    C1dKernel &operator=(const C1dKernel &k);  

protected:
    CMemShare m_cMem;
    int m_iTaps, m_iCenter;
};

/// \ingroup filter
/// <summary>A fixed \link C1dKernel C1dKernel \endlink that implements a 
/// 3 tap kernel with values 0.25, 0.5, 0.25.</summary> 
class C121Kernel : protected C1dKernel
{
public:
    C121Kernel()
	{
        static const float fdf121[] = {0.25f, 0.5f, 0.25f};  
		m_cMem.Use((Byte*)fdf121);
        m_iTaps   = 3;
        m_iCenter = 1;
	}

	/// <summary>Returns the C1dKernel to be used in functions like 
	/// VtSeparableFilter() that require
	/// a kernel.</summary> 
	const C1dKernel& AsKernel() const
	{ return *this; }
};


/// \ingroup filter
/// <summary>A fixed \link C1dKernel C1dKernel \endlink that implements a 
/// 5 tap kernel with values 0.0625, 0.25, 0.375, 0.25, 0.0625.</summary> 
class C14641Kernel : protected C1dKernel
{
public:
    C14641Kernel()
	{
		static const float fdf14641[] = {0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f };  
		m_cMem.Use((Byte*)fdf14641);
        m_iTaps   = 5;
        m_iCenter = 2;
	}

	/// <summary>Returns the C1dKernel to be used in functions like 
	/// VtSeparableFilter() that require
	/// a kernel.</summary> 
	const C1dKernel& AsKernel() const
	{ return *this; }
};

/// \ingroup filter
/// <summary>A fixed \link C1dKernel C1dKernel \endlink that implements a
/// a Lanczos windowed sinc.  This is a high quality low pass filter, which
/// is useful when pre-filtering to generate an image pyramid.</summary> 
class C1d8TapMipMapKernel : protected C1dKernel
{
public:
    C1d8TapMipMapKernel()
	{
		static const float fdf8pt[] = 
			{-0.04456f, -0.05877f, 0.15592f, 0.447409f, 0.447409f, 0.15592f,
             -0.05877f, -0.04456f};
		m_cMem.Use((Byte*)fdf8pt);
        m_iTaps   = 8;
        m_iCenter = 3;
	}

	/// <summary>Returns the C1dKernel to be used in functions like 
	/// VtSeparableFilter() that require
	/// a kernel.</summary> 
	const C1dKernel& AsKernel() const
	{ return *this; }
};


/// \ingroup filter
/// <summary>Create a Gaussian \ref C1dKernel.
/// </summary>
/// <param name="dst">The resulting kernel</param>
/// <param name="fSigma">Controls the amount of smoothing. Large sigma yields more
/// smoothing, but a more expensive to apply filter kernel.</param>
/// <param name="iDeriv">When set to a non-zero number returns the iDeriv derivative
/// of the Gaussian function</param>
/// <param name="fKW">Controls the kernel width. The default value of 3 does a
/// reasonable job truncating the kernel to significant values.  Lower this 
/// number to get a narrower (less precise but faster to apply kernel), or 
/// raise the value to get a wider (more precise and expensive kernel).
/// </param> 
/// <returns> 
/// - S_OK on success 
/// - E_INVALIDARG if any of the parameters are out of range
/// - E_OUTOFMEMORY if memory allocation failed 
/// </returns>
HRESULT 
Create1dGaussKernel(C1dKernel& dst, float fSigma, int iDeriv = 0, 
					float fKW = 3.0);

// 'correct' discrete scalespace Gaussian
HRESULT 
Create1dGaussBesselKernel(C1dKernel& dst, float fSigma, float fKW = 3.0);

/// \ingroup filter
/// <summary>Class that contains a bank of \link C1dKernel C1dKernels \endlink. 
/// This allows for a spatially varying filter (also known as polyphase), 
/// which is useful for resampling operations among other things.
/// </summary>
/// 
/// <DL><DT> Remarks: </DT></DL>
/// This class defines a set of kernels to be applied over a cycle to a 1d set 
/// of data. The cycle count indicates how many filters are in the polyphase 
/// filter bank and therefore sets the number of output pixels that are generated 
/// as the convolution code applies the polyphase filter kernels one by one in
/// turn. The first tap of each kernel is located at some offset relative to the 
/// origin of the cycle. This offset is called the coordinate of the kernel. 
/// The coordinate shift per cycle is used to advance the kernel origin from one 
/// cycle to the next.

class C1dKernelSet
{
public:
    C1dKernelSet() : m_iCoordShiftPerCycle(1) {}

	/// <summary>Create a polyphase kernel bank.</summary> 
	/// <param name="iCycle">The number of phases in the bank.</param>
	/// <param name="iCoordShiftPerCycle">The amount to shift in the
	/// source span from the start of one cycle to the next.</param>
    HRESULT          Create(int iCycle, int iCoordShiftPerCycle);

	/// <summary>Create a polyphase kernel bank with one phase.</summary> 
	/// <param name="k">The kernel associated with the single phase.</param>
	/// <DL><DT> Remarks: </DT></DL>
	///     The resulting kernel set will have a "Cycle" of 1 and a 
	///     "CoordShiftPerCycle" of 1.
    HRESULT          Create(const C1dKernel& k);

	/// <summary>Create a polyphase kernel bank by copying another kernel
	/// bank.</summary>
	/// <param name="ks">The source kernel set to copy.</param>
    HRESULT          Create(const C1dKernelSet& ks);

	/// <summary>Set the kernel at the uIndex phase of the cycle.</summary>
	/// <param name="uIndex">The phase of the cycle, must be less than the total
	/// phase count returned by GetCycle.</param>
	/// <param name="iCoord">The offset relative to the first source pixel in
	/// the cycle at which to apply this kernel.</param>
	/// <param name="k">The filter kernel.</param>
    HRESULT          Set(UInt32 uIndex, int iCoord, const C1dKernel& k);

	HRESULT          Set(UInt32 uIndex, int iCoord, int iTaps, const float *pfK);
    C1dKernel&       GetKernel(UInt32 uIndex);
    const C1dKernel& GetKernel(UInt32 uIndex) const;
    int              GetCoord(UInt32 uIndex)  const;

	UInt32             GetCycle() const 
	{ return (UInt32)m_vecKernelSet.size(); }

	int              GetCoordShiftPerCycle() const 
    { return m_iCoordShiftPerCycle; }

	// calculates the source origin and span in a larger source image given 
	// information about the output image during polyphase filtering. The source 
	// region is not clipped against the source image.
	void GetSourceRegionNoClipping(int dx, int iDstTileWidth, int &sx, 
								   int &iSrcTileWidth) const;

	// calculates the source origin and span in a larger source image given 
	// information about the output image during polyphase filtering.
	void GetSourceRegion(int iTotalSrcWidth, int dx, int iDstTileWidth, 
						 int &sx, int &iSrcTileWidth) const;
	
	// caculates the destination pixel span that would be changed if the source 
	// span pixels were updated.
	void GetDestinationRegion(int sx, int iSrcTileWidth, int &dx, 
							  int &iDstTileWidth) const;

private:
    vt::vector<C1dKernel> m_vecKernelSet;
    int                   m_iCoordShiftPerCycle;
    vt::vector<int>       m_vecCoordSet;
};

// kernel for Gaussian blur with arbitrary re-sample (up or down) set 
// inputsamples = width of input image, outputsamples = width of output image
// sigma is defined in the space of the input image
// you need to work out what blur is appropriate to resample the image without 
// aliasing
HRESULT 
Create1dGaussianKernelSet(C1dKernelSet& dst, int iInputSamples, int iOutputSamples, 
						  float fSigma, int iDeriv = 0, float fKW = 3.0,
						  float fPhase = 0.f);

// 'a' controls the length of the kernel and how close to a sinc we get.
// typically a is 2 or 3.
HRESULT
Create1dLanczosKernelSet(C1dKernelSet& dst, int iInputSamples, int iOutputSamples, 
						 int a, float fPhase = 0.f);

HRESULT
Create1dBilinearKernelSet(C1dKernelSet& dst, int iInputSamples, int iOutputSamples, 
						  float fPhase = 0.f);

HRESULT
Create1dBicubicKernelSet(C1dKernelSet& dst, int iInputSamples, int iOutputSamples, 
						 float fPhase = 0.f);

// cycles can be 4, 5, or 6 with larger numbers giving longer filters
HRESULT 
Create1dTriggsKernelSet(C1dKernelSet& dst, int iInputSamples, int iOutputSamples,
						int iCycles, float fPhase = 0.f);

// Note this requires a preprocessing via VtPreprocessBicubicBSpline() or CPreprocessBicubicBSpline
HRESULT
Create1dBicubicBSplineKernelSet(C1dKernelSet& dst, int iInputSamples, int iOutputSamples, 
						 float fPhase = 0.f);

//+-----------------------------------------------------------------------------
//
//  function: GetRequiredSrcRect
//
//  Synopsis: Calculate size of source region needed to generate rctDst via the 
//            provided kernel. The first function specifies the vertical and 
//            horizontal kernels separately. The second uses the same kernel 
//            horizontally and vertically.
// 
//------------------------------------------------------------------------------
inline vt::CRect 
GetRequiredSrcRect(const vt::CRect& rctDst, const C1dKernel& krnlH,
				   const C1dKernel& krnlV )
{
	int iPadL = krnlH.Center();
	int iPadR = krnlH.Width() - krnlH.Center() - 1;
	int iPadT = krnlV.Center();
	int iPadB = krnlV.Width() - krnlV.Center() - 1;
	return vt::CRect(rctDst.left-iPadL,  rctDst.top-iPadT,
					 rctDst.right+iPadR, rctDst.bottom+iPadB);
}

inline vt::CRect 
GetRequiredSrcRect(const vt::CRect& rctDst, const C1dKernel& krnl )
{
	int iPadL = krnl.Center();
	int iPadR = krnl.Width() - krnl.Center() - 1;
	return vt::CRect(rctDst.left-iPadL,  rctDst.top-iPadL,
					 rctDst.right+iPadR, rctDst.bottom+iPadR);
}

//+-----------------------------------------------------------------------------
//
//  function: GetAffectedDstRect
//
//  Synopsis: Calculate size of destination region that is affected by 
//            changes in the given source region.
// 
//------------------------------------------------------------------------------
inline vt::CRect 
GetAffectedDstRect(const vt::CRect& rctSrc, const C1dKernel& krnl )
{
	int iPadL = krnl.Center();
	int iPadR = krnl.Width() - krnl.Center() - 1;
	return vt::CRect(rctSrc.left-iPadR,  rctSrc.top-iPadR,
					 rctSrc.right+iPadL, rctSrc.bottom+iPadL);
}

//+-----------------------------------------------------------------------------
//
//  function: GetRequiredSrcRect
//
//  Synopsis: Calculate size of source region needed to generate 
//            rctDst via the provided kernel set.  The first function specifies 
//            the vertical and horizontal kernel sets separately. The second
//            uses the same kernel set horizontally and vertically. 
// 
//------------------------------------------------------------------------------
inline vt::CRect 
GetRequiredSrcRect(const vt::CRect& rctDst, const C1dKernelSet& ksh, 
				   const C1dKernelSet& ksv)
{
	// Calculate size of source region needed.
	int iX, iWidth, iY, iHeight;
	ksh.GetSourceRegionNoClipping(rctDst.left, rctDst.Width(),  iX, iWidth);
	ksv.GetSourceRegionNoClipping(rctDst.top,  rctDst.Height(), iY, iHeight);
	return CRect(iX, iY, iX + iWidth, iY + iHeight);
}

inline vt::CRect 
GetRequiredSrcRect(const vt::CRect& rctDst, const C1dKernelSet& ks)
{ return GetRequiredSrcRect( rctDst, ks, ks); }

//+-----------------------------------------------------------------------------
//
//  function: GetAffectedDstRect
//
//  Synopsis: Calculate size of destination region that is affected by 
//            changes in the given source region.
// 
//------------------------------------------------------------------------------
inline vt::CRect 
GetAffectedDstRect(const vt::CRect& rctSrc, const C1dKernelSet& ksh, 
				   const C1dKernelSet& ksv)
{
	// Calculate size of source region needed.
	// TODO: the GetDestinationRegion function does not work for negative
	//       coordinates.  It is also extremely inefficient as it searches the
	//       entire destination image space from coordiante 0 to result
	int iX, iWidth, iY, iHeight;
	ksh.GetDestinationRegion(rctSrc.left, rctSrc.Width(),  iX, iWidth);
	ksv.GetDestinationRegion(rctSrc.top,  rctSrc.Height(), iY, iHeight);
	return CRect(iX, iY, iX + iWidth, iY + iHeight);
}

inline vt::CRect 
GetAffectedDstRect(const vt::CRect& rctSrc, const C1dKernelSet& ks)
{ return GetAffectedDstRect(rctSrc, ks, ks); }

};
