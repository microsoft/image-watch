//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Image statistics
//
//  History:
//      2008/2/13-swinder
//          Created
//
//------------------------------------------------------------------------
#pragma once

#include "vtcommon.h"

namespace vt {

#ifdef MAKE_DOXYGEN_WORK
void foo();
#endif

//// \ingroup imgstats
/// <summary> Compute minimum and maximum element value. </summary>
/// <param name="imgSrc"> Input image. </param>
/// <param name="minVal"> Minimum element value. </param>
/// <param name="maxVal"> Maximum element value. </param>
/// <param name="ignoreNaN"> Ignore NaNs. If false (default), NaN pixels result in undefined behavior. If true, NaNs are ignored (at the cost of additional CPU cycles).</param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid
///		- E_NOTIMPL if imgSrc is not supported
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes minimum and maximum value of all elements (not pixels) of imgSrc
HRESULT VtMinMaxImage(const CImg &imgSrc, float& minVal, float& maxVal, 
					  bool ignoreNaN = false);

template<typename T>
void VtMinMaxSpan(const T* pSpan, int iW, float &fMin, float &fMax, 
				  bool ignoreNaN = false);

//// \ingroup imgstats
/// <summary> Compute minimum and maximum color value. </summary>
/// <param name="imgSrc"> Input image. </param>
/// <param name="minVal"> Minimum color value. </param>
/// <param name="maxVal"> Maximum color value. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDSRC if imgSrc is invalid or not a color image
///		- E_NOTIMPL if imgSrc is not supported
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///		- Computes minimum and maximum values of the R, G, B, and A component of imgSrc
HRESULT VtMinMaxColorImage(const CImg &imgSrc, RGBAFloatPix &minVal, 
	RGBAFloatPix& maxVal);

template<typename T>
void VtMinMaxColorSpan(const RGBAType<T>* pSpan, int iW, 
	RGBAFloatPix& minVal, RGBAFloatPix& maxVal);

// legacy
UInt32 VtPositiveMinMaxImage(const CImg& imgSrc, float& fMin, float& fMax);

//+-----------------------------------------------------------------------------
//
// function: VtAverageRGBA
//
// Synopsis: Return the average rgba value of an image.
// 
//           Note: this algorithm is limited to images of around 4096 x 4096 
//                 otherwise overflow will result.  to run this on larger 
//                 images you would have to tile the image and use the 
//                 CImg.Share() feature to wrap tiles.
// 
//------------------------------------------------------------------------------

// not documented
void
VtAverageRGBA(const CRGBAImg &img, RGBAFloatPix &averageColor);



// build histogram of 1 band byte image
HRESULT VtBuildHistogram(const CByteImg &img, int *piHist);

// build histogram of 3 or 4 band byte image (RGB space)
HRESULT VtBuildHistogram(const CByteImg &img, 
						 int *piHistR, int *piHistG, int *piHistB);

//+-----------------------------------------------------------------------------
//
// Class: CLogHistogram
// 
// Synopsis: a class to bin a range of values in a log space
// 
//------------------------------------------------------------------------------
class CLogHistogram
{
public:
	CLogHistogram() : m_pHistogram(NULL), m_pMantissaLogs(NULL),
        m_uElementSize(0), m_uTotalBits(0), m_uMantissaBits(0), m_uExpBits(0)
	{}

	~CLogHistogram() 
	{ delete (BYTE*)m_pHistogram; delete m_pMantissaLogs; }

	void Clear();

	HRESULT Create(UInt16 countbytes, UInt16 depth, short expmin, short expmax);

	bool IsValid() const { return m_pHistogram != NULL; }

	//void  Acc(UInt32 value, UInt32 count = 1);
	//UInt32  GetCount(UInt32 index);
	float GetVal(UInt32 index) const { return EvalPacked(index); }
	float GetMinVal() const { return EvalUnpacked(0, 0); }
	float GetMaxVal() const { return EvalUnpacked(GetMantissaMask(), 
												  m_iExponentMax-m_iExponentMin); }

    int   GetExpMin()      const { return m_iExponentMin;  } 
    int   GetExpMax()      const { return m_iExponentMax;  } 
	UInt32  GetExpBitCount() const { return m_uExpBits;      }
	UInt32  GetExpMask()     const { return ((1<<m_uExpBits)-1) << m_uMantissaBits; }
	UInt32  GetMantissaBitCount() const { return m_uMantissaBits; }
	UInt32  GetMantissaMask()     const { return ((1<<m_uMantissaBits)-1); }
	UInt32  GetTotalBitCount()    const { return m_uTotalBits;    }
	UInt32  GetBinCount()         const 
		{ return (m_iExponentMax-m_iExponentMin+1) * GetBinsPerOctave(); }
	UInt32  GetBinsPerOctave()    const { return (1<<m_uMantissaBits); }

    UInt32  GetMantissaLog(UInt32 uMantissa) const
        { return m_pMantissaLogs[uMantissa]; }

protected:
	float EvalPacked(UInt32 packed) const
	{ 
		return EvalUnpacked( packed & GetMantissaMask(), 
							 packed >> m_uMantissaBits );
    }

	float EvalUnpacked(UInt32 uMant, UInt32 uExp) const
	{ 
	    return ldexpf(float((1<<m_uMantissaBits) + uMant), 
					 m_iExponentMin + (int)uExp - m_uMantissaBits);
    }

protected:
	short   m_iExponentMin;
	short   m_iExponentMax;
	UInt16  m_uExpBits;
	UInt16  m_uMantissaBits;
	UInt16  m_uTotalBits;
	UInt16  m_uElementSize;
	void*   m_pHistogram;
    UInt32*   m_pMantissaLogs;
};

//+-----------------------------------------------------------------------------
//
// Class: CTypedLogHistogram
// Lu
// Synopsis: a wrapper class that sepecifies the width of the counts
// 
//------------------------------------------------------------------------------
template <class T>
class CTypedLogHistogram: public CLogHistogram
{
public:
	HRESULT Create(UInt16 depth, short expmin, short expmax)
    { return CLogHistogram::Create(sizeof(T), depth, expmin, expmax); }

	void Acc(UInt32 value, T count = T(1))
		{ ((T*)m_pHistogram)[value] += count; }

	T GetCount(UInt32 index) const
	    { return ((T*)m_pHistogram)[index]; }

	T* GetBuffer()
		{ return (T*)m_pHistogram; }

	const T* GetBuffer() const
	    { return (T*)m_pHistogram; }
};

//+-----------------------------------------------------------------------------
//
// Class: CFloatLogHistogram
// 
// Synopsis: a wrapper class that allows interpolated counts
// 
//------------------------------------------------------------------------------
class CFloatLogHistogram: public CTypedLogHistogram<float>
{
public:
	void Acc(UInt32 packed, float count = 1.f)
	{
        UInt32 shift = 23 - GetMantissaBitCount();
        UInt32 mask = (1 << shift) - 1;
        float inv_mask = 1.f / (float) (mask + 1);

        // find index
        UInt32 index = packed >> shift;

        // find fraction * count
        float fract_1 = (float) (packed & mask) * inv_mask;
        fract_1 *= count;
        float fract0 = count - fract_1;

        float* pHist = &((float*)m_pHistogram)[index];
        *pHist += fract0;
        if (index < GetBinCount() - 1)
            *(pHist + 1) += fract_1;
    }

#if (defined(_M_IX86)||defined(_M_AMD64)) && defined(_MSC_VER)
    void AccSSE(const UInt32 packed[4], float count = 1.f)
	{
        UInt32 maxbin = GetBinCount() - 1;
        UInt32 shift = 23 - GetMantissaBitCount();
        UInt32 mask = (1 << shift) - 1;
        float inv_mask = 1.f / (float) (mask + 1);

        // find index
        __m128i xp = _mm_loadu_si128((const __m128i *) packed);
        __m128i xi = _mm_srli_epi32(xp, shift);

        // find fraction * count
        __m128 xfc = _mm_set1_ps(count);
        __m128 xfm = _mm_set1_ps(inv_mask);
        __m128i xm = _mm_set1_epi32(mask);
        xp = _mm_and_si128(xp, xm);
        __m128 xf1 = _mm_cvtepi32_ps(xp);
        xf1 = _mm_mul_ps(xf1, xfm);
        xf1 = _mm_mul_ps(xf1, xfc);
        __m128 xf0 = _mm_sub_ps(xfc, xf1);

        for (int i = 0; i < 4; i++)
        {
            if (packed[i] == 0)
                continue;
            float* pHist = &((float*)m_pHistogram)[xi.m128i_u32[i]];
            *pHist += xf0.m128_f32[i];
            if (xi.m128i_u32[i] < maxbin)
                *(pHist + 1) += xf1.m128_f32[i];
        }
    }
#endif
};

//+-----------------------------------------------------------------------------
//
// Function: GetMantissa, GetExponent
// 
// Synopsis: inline functions that return mantissa and exponent for various types
// 
//------------------------------------------------------------------------------
template <class T> UInt32 GetMantissa(T v);
template <class T> int   GetExponent(T v);

template <> inline
UInt32 GetMantissa(float v)
{
	UInt32 tmp = *(UInt32*)(&v);
	return tmp & 0x007fffff;
}
template <> inline
int GetExponent(float v)
{
	UInt32 tmp = *(UInt32*)(&v);
	return int((tmp >> 23) & 0xff) - 127;
}

template <> inline
UInt32 GetMantissa(HALF_FLOAT v)
{
	UInt16 tmp = *(UInt16*)(&v);
	return tmp & 0x3ff;
}
template <> inline
int GetExponent(HALF_FLOAT v)
{
    // Bug: 0xff is not correct here, I think that it should be 0x1f
	UInt16 tmp = *(UInt16*)(&v);
	return int((tmp >> 10) & 0xff) - 15;
}

//+-----------------------------------------------------------------------------
//
// Function: ConvertValToLogHistoBin
// 
// Synopsis: a helper that packs a value into the log index representation of
//           CLogHistogram
// 
//------------------------------------------------------------------------------
template <class T>
UInt32 ConvertValToLogHistoBin(T v, const CLogHistogram& histo,
                              bool bIncludeFraction = false);

//+-----------------------------------------------------------------------------
//
// Function: ConvertLogHistoBin
// 
// Synopsis: a helper function that converts an bin from one histogram 
//           into another
// 
//------------------------------------------------------------------------------
UInt32
ConvertLogHistoBin(UInt32 bin, const CLogHistogram& histoSrc, 
				   const CLogHistogram& histoDst);

//+-----------------------------------------------------------------------------
//
// Function: 
// 
// Synopsis: functions to compute histograms
// 
//------------------------------------------------------------------------------
template <class T>
void ComputeExponentHistogram(const CTypedImg<T>& imgSrc, UInt32* pHisto);

void ComputeExponentHistogram(const CImg& imgSrc, UInt32* pHisto);

template <class H> UInt32
BuildLogHistogram(const CImg& img, CTypedLogHistogram<H>& histo);

template <class T, class H>
UInt32 BuildLogHistogram(const CTypedImg<T>& img, CTypedLogHistogram<H>& histo);

UInt32 BuildLogHistogram(const CFloatImg& img, CFloatLogHistogram& histo);

//+-----------------------------------------------------------------------------
//
// Function: SmoothHistogram
// 
// Synopsis: a helper function that smooths a histogram using a 12421 filter
// 
//------------------------------------------------------------------------------
void SmoothHistogram(UInt32 maxbin, const float* pfSrc, float* pfDst);

};
