//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Mixed utility functions
//
//  History:
//      2003/11/12-swinder/mattu
//			Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_image.h"
#include "vt_convert.h"
#include "vt_stats.h"

#include "vt_function.h"

#include <float.h>

using namespace vt;

//+-----------------------------------------------------------------------------
// 
// Function: VtMinMaxImage
// 
//------------------------------------------------------------------------------
struct MinMaxOpParamType
{
	explicit MinMaxOpParamType(float mn = FLT_MAX, float mx = -FLT_MAX)
		: minVal(mn), maxVal(mx)
	{
	}

	float minVal;
	float maxVal;
};

template<typename SrcElT, bool IgnoreNaN>
class MinMaxOpBase
{
public:
	struct TmpType
	{
		void LoadGeneric(float fMin, float fMax)
		{
			mn = fMin;
			mx = fMax;
		}
#if (defined(_M_IX86)||defined(_M_AMD64))
		void LoadSSE2(float fMin, float fMax)
		{
			xf0 = _mm_set1_ps(fMax);
			xf1 = _mm_set1_ps(fMin);
		}

		__m128 xf0, xf1;		
#endif
		float mn, mx;
	};

public:
	typedef float ImplSrcElType;
	typedef MinMaxOpParamType ParamType;	

	static const int NumSrcBands = 0;
	static const int NumSrcElPerGroup = 1;
	static const int NumGroupsPerOpGeneric = 1;
	static const int NumGroupsPerOpSSE2 = 4;
	
public:
	void InitGeneric(ParamType* param, TmpType& tmp) 
	{
		tmp.LoadGeneric((*param).minVal / float(ElTraits<SrcElT>::MaxVal()), 
			(*param).maxVal / float(ElTraits<SrcElT>::MaxVal()));
	}

	void EvalGeneric(const ImplSrcElType* pSrc, ParamType*, TmpType& tmp) 
	{
		EvalGenericInternal(pSrc, nullptr, tmp, OpHelpers::BoolToType<IgnoreNaN>());
	}

	void EvalGenericInternal(const ImplSrcElType* pSrc, ParamType*, 
		TmpType& tmp, OpHelpers::BoolToType<true>) 
	{
		if (((*(int*)pSrc) & 0x7f800000) == 0x7f800000)
			return;

		EvalGenericInternal(pSrc, nullptr, tmp, OpHelpers::BoolToType<false>());
	}

	void EvalGenericInternal(const ImplSrcElType* pSrc, ParamType*, 
		TmpType& tmp, OpHelpers::BoolToType<false>) 
	{
		tmp.mn = VtMin(*pSrc, tmp.mn);
		tmp.mx = VtMax(*pSrc, tmp.mx);
	}

	void ExitGeneric(ParamType* param, TmpType& tmp) 
	{
		(*param).minVal = tmp.mn * float(ElTraits<SrcElT>::MaxVal());
		(*param).maxVal = tmp.mx * float(ElTraits<SrcElT>::MaxVal());
	}

#if (defined(_M_IX86)||defined(_M_AMD64))
	void InitSSE2(ParamType* param, TmpType& tmp) 
	{
		tmp.LoadSSE2((*param).minVal / float(ElTraits<SrcElT>::MaxVal()), 
			(*param).maxVal / float(ElTraits<SrcElT>::MaxVal()));
	}
	
	void EvalSSE2(const ImplSrcElType* pSrc, ParamType*, TmpType& tmp)
	{
		EvalSSE2Internal(pSrc, nullptr, tmp, OpHelpers::BoolToType<IgnoreNaN>());
	}

	void EvalSSE2Internal(const ImplSrcElType* pSrc, ParamType*, TmpType& tmp,
		OpHelpers::BoolToType<false>)
	{
		__m128 xf2 = _mm_loadu_ps(pSrc);
		tmp.xf0 = _mm_max_ps(tmp.xf0, xf2);
		tmp.xf1 = _mm_min_ps(tmp.xf1, xf2);
	}

	void EvalSSE2Internal(const ImplSrcElType* pSrc, ParamType*, TmpType& tmp,
		OpHelpers::BoolToType<true>)
	{
		__m128 oldxf0 = tmp.xf0;
		__m128 oldxf1 = tmp.xf1;
		
		__m128 xf2 = _mm_loadu_ps(pSrc);
		tmp.xf0 = _mm_max_ps(tmp.xf0, xf2);

		__m128i nan = _mm_set1_epi32(0x7f800000);
		__m128 cmp = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(_mm_and_ps(xf2, _mm_castsi128_ps(nan))), nan));

		tmp.xf0 = _mm_or_ps(_mm_and_ps(cmp, oldxf0), _mm_andnot_ps(cmp, tmp.xf0));
		
		tmp.xf1 = _mm_min_ps(tmp.xf1, xf2);
		tmp.xf1 = _mm_or_ps(_mm_and_ps(cmp, oldxf1), _mm_andnot_ps(cmp, tmp.xf1));
	}

	void ExitSSE2(ParamType* param, TmpType& tmp)
	{
		__m128 xf3 = _mm_set1_ps(float(ElTraits<SrcElT>::MaxVal()));
		__m128 mx = _mm_mul_ps(xf3, tmp.xf0);
		__m128 mn = _mm_mul_ps(xf3, tmp.xf1);

		(*param).minVal = VtMin((*param).minVal, mn.m128_f32[0]);
		(*param).minVal = VtMin((*param).minVal, mn.m128_f32[1]);
		(*param).minVal = VtMin((*param).minVal, mn.m128_f32[2]);
		(*param).minVal = VtMin((*param).minVal, mn.m128_f32[3]);
		(*param).maxVal = VtMax((*param).maxVal, mx.m128_f32[0]);
		(*param).maxVal = VtMax((*param).maxVal, mx.m128_f32[1]);
		(*param).maxVal = VtMax((*param).maxVal, mx.m128_f32[2]);
		(*param).maxVal = VtMax((*param).maxVal, mx.m128_f32[3]);
	}
#endif
};

template<typename SrcElT>
class MinMaxOp : public MinMaxOpBase<SrcElT, false> {};
template <typename SrcElT>
class MinMaxOpIgnoreNaN : public MinMaxOpBase<SrcElT, true> {};

HRESULT vt::VtMinMaxImage(const CImg &imgSrc, float& fMin, float& fMax, 
						  bool ignoreNaN)
{
	VT_HR_BEGIN();

	MinMaxOpParamType param;
	if (ignoreNaN)
    {
		VT_HR_EXIT(StatsImgOpD<MinMaxOpIgnoreNaN>(imgSrc, &param));
    }
	else
    {
		VT_HR_EXIT(StatsImgOpD<MinMaxOp>(imgSrc, &param));
    }
	
	fMin = param.minVal;
	fMax = param.maxVal;
	
	VT_HR_END();
}

template<typename T>
void vt::VtMinMaxSpan(const T* pSpan, int iW, float &fMin, float &fMax, 
					  bool ignoreNaN)
{
	MinMaxOpParamType param(fMin, fMax);		
	if (ignoreNaN)
		StatsSpanOp(pSpan, 1, iW, MinMaxOpIgnoreNaN<T>(), &param);
	else
		StatsSpanOp(pSpan, 1, iW, MinMaxOp<T>(), &param);
	
	fMin = param.minVal;
	fMax = param.maxVal;
}

template
void vt::VtMinMaxSpan(const Byte* pSpan, int iW, float &fMin, float &fMax, bool ignoreNaN);
template
void vt::VtMinMaxSpan(const UInt16* pSpan, int iW, float &fMin, float &fMax, bool ignoreNaN);
template
void vt::VtMinMaxSpan(const float* pSpan, int iW, float &fMin, float &fMax, bool ignoreNaN);


//+-----------------------------------------------------------------------------
// 
// Function: VtMinMaxColorImage
// 
//------------------------------------------------------------------------------
struct MinMaxColorOpParamType
{
	explicit MinMaxColorOpParamType(
		const RGBAFloatPix& mn = RGBAFloatPix(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX),
		const RGBAFloatPix& mx = RGBAFloatPix(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX))
		: minVal(mn), maxVal(mx)		
	{		
	}

	RGBAFloatPix minVal;
	RGBAFloatPix maxVal;	
};

template<typename SrcElT>
class MinMaxColorOp
{
public:
	struct TmpType
	{
		void LoadGeneric(const RGBAFloatPix& fMin, const RGBAFloatPix& fMax,
			float s)
		{
			mn = RGBAFloatPix(fMin.r*s, fMin.g*s, fMin.b*s, fMin.a*s);
			mx = RGBAFloatPix(fMax.r*s, fMax.g*s, fMax.b*s, fMax.a*s);
		}

#if (defined(_M_IX86)||defined(_M_AMD64))
		void LoadSSE1(const RGBAFloatPix& fMin, const RGBAFloatPix& fMax,
			float s)
		{
			mnSSE = _mm_setr_ps(fMin.b*s, fMin.g*s, fMin.r*s, fMin.a*s);
			mxSSE = _mm_setr_ps(fMax.b*s, fMax.g*s, fMax.r*s, fMax.a*s);
		}
		__m128 mnSSE, mxSSE;
#endif
		RGBAFloatPix mn, mx;
	};

public:
	typedef float ImplSrcElType;
	typedef MinMaxColorOpParamType ParamType;	

	static const int NumSrcBands = 4;
	static const int NumSrcElPerGroup = 4;
	static const int NumGroupsPerOpGeneric = 1;	
	static const int NumGroupsPerOpSSE1 = 1;	

public:
	void InitGeneric(ParamType* param, TmpType& tmp) 
	{
		tmp.LoadGeneric((*param).minVal, (*param).maxVal,
			1.f / float(ElTraits<SrcElT>::MaxVal()));
	}

	void EvalGeneric(const ImplSrcElType* pSrc, ParamType*, TmpType& tmp) 
	{
		const RGBAFloatPix& src = reinterpret_cast<const RGBAFloatPix&>(*pSrc);

		tmp.mn.r = VtMin(src.r, tmp.mn.r);
		tmp.mn.g = VtMin(src.g, tmp.mn.g);
		tmp.mn.b = VtMin(src.b, tmp.mn.b);
		tmp.mn.a = VtMin(src.a, tmp.mn.a);
		tmp.mx.r = VtMax(src.r, tmp.mx.r);
		tmp.mx.g = VtMax(src.g, tmp.mx.g);
		tmp.mx.b = VtMax(src.b, tmp.mx.b);
		tmp.mx.a = VtMax(src.a, tmp.mx.a);
	}

	void ExitGeneric(ParamType* param, TmpType& tmp) 
	{
		float s = float(ElTraits<SrcElT>::MaxVal());
		(*param).minVal = RGBAFloatPix(tmp.mn.r*s, tmp.mn.g*s, tmp.mn.b*s, 
			tmp.mn.a*s);
		(*param).maxVal = RGBAFloatPix(tmp.mx.r*s, tmp.mx.g*s, tmp.mx.b*s, 
			tmp.mx.a*s);
	}

#if (defined(_M_IX86)||defined(_M_AMD64))
	void InitSSE1(ParamType* param, TmpType& tmp) 
	{
		tmp.LoadSSE1((*param).minVal, (*param).maxVal, 
			1.f / float(ElTraits<SrcElT>::MaxVal()));
	}

	void EvalSSE1(const ImplSrcElType* pSrc, ParamType*, TmpType& tmp)
	{
		__m128 xf2 = _mm_loadu_ps(pSrc);
		tmp.mxSSE = _mm_max_ps(tmp.mxSSE, xf2);
		tmp.mnSSE = _mm_min_ps(tmp.mnSSE, xf2);
	}

	void ExitSSE1(ParamType* param, TmpType& tmp)
	{
		__m128 xf3 = _mm_set1_ps(float(ElTraits<SrcElT>::MaxVal()));
		__m128 mx = _mm_mul_ps(xf3, tmp.mxSSE);
		__m128 mn = _mm_mul_ps(xf3, tmp.mnSSE);

		(*param).minVal.b = VtMin((*param).minVal.b, mn.m128_f32[0]);
		(*param).minVal.g = VtMin((*param).minVal.g, mn.m128_f32[1]);
		(*param).minVal.r = VtMin((*param).minVal.r, mn.m128_f32[2]);
		(*param).minVal.a = VtMin((*param).minVal.a, mn.m128_f32[3]);
		(*param).maxVal.b = VtMax((*param).maxVal.b, mx.m128_f32[0]);
		(*param).maxVal.g = VtMax((*param).maxVal.g, mx.m128_f32[1]);
		(*param).maxVal.r = VtMax((*param).maxVal.r, mx.m128_f32[2]);
		(*param).maxVal.a = VtMax((*param).maxVal.a, mx.m128_f32[3]);
	}
#endif
};

HRESULT vt::VtMinMaxColorImage(const CImg &imgSrc, RGBAFloatPix &minVal, 
	RGBAFloatPix& maxVal)
{
	VT_HR_BEGIN();

	if (!IsColorImage(imgSrc))
		VT_HR_EXIT(E_INVALIDSRC);

	MinMaxColorOpParamType param;			
	VT_HR_EXIT(StatsImgOpD<MinMaxColorOp>(imgSrc, &param));
	
	minVal = param.minVal;
	maxVal = param.maxVal;
	
	VT_HR_END();
}

template<typename T>
void vt::VtMinMaxColorSpan(const RGBAType<T>* pSpan, int iW, 
	RGBAFloatPix& minVal, RGBAFloatPix& maxVal)
{
	MinMaxColorOpParamType param(minVal, maxVal);			
	StatsSpanOp(reinterpret_cast<const T*>(pSpan), 
		4, iW, MinMaxColorOp<T>(), &param);
	
	minVal = param.minVal;
	maxVal = param.maxVal;
}

template
void vt::VtMinMaxColorSpan(const RGBAType<Byte>* pSpan, int iW, 
	RGBAFloatPix& minVal, RGBAFloatPix& maxVal);
template
void vt::VtMinMaxColorSpan(const RGBAType<UInt16>* pSpan, int iW, 
	RGBAFloatPix& minVal, RGBAFloatPix& maxVal);
template
void vt::VtMinMaxColorSpan(const RGBAType<float>* pSpan, int iW, 
	RGBAFloatPix& minVal, RGBAFloatPix& maxVal);

//+-----------------------------------------------------------------------------
// 
// Function: VtPositiveMinMaxImage
// 
//------------------------------------------------------------------------------
UInt32 vt::VtPositiveMinMaxImage(const CImg& imgSrc, float& fMin, float& fMax)
{
	UInt32 uCount = 0;
	fMax = 1e-38f;
	fMin = 1e38f;

	float tmpbuf[256];
	int  iBands     = imgSrc.Bands();
	UInt32 uBufCount  = sizeof(tmpbuf)/sizeof(tmpbuf[0]);
	UInt32 uByteCount = uBufCount * (UInt32)imgSrc.ElSize(); 
	UInt32 uElWidth   = imgSrc.Width()*iBands;

	for ( int y = 0; y < imgSrc.Height(); y++ )
	{
		const Byte* pS = imgSrc.BytePtr(y);
		for ( UInt32 x = 0; x < uElWidth; x+=uBufCount, pS+=uByteCount )
		{
			UInt32 uElCount = VtMin(uElWidth-x, uBufCount);
			VtConvertSpan(tmpbuf, VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, iBands),
						  pS, imgSrc.GetType(), (int)uElCount);
			for( UInt32 i = 0; i < uElCount; i++ )
			{
				if ( tmpbuf[i] > 0 )
				{
					fMin = VtMin(fMin, tmpbuf[i]);
					fMax = VtMax(fMax, tmpbuf[i]);
					uCount++;
				}
			}
		}
	}
	return uCount;
}

//+-----------------------------------------------------------------------------
// 
// Function: VtAverageRGBA
// 
//------------------------------------------------------------------------------
void
	vt::VtAverageRGBA(const CRGBAImg &img, RGBAFloatPix &averageColor)
{
	int x,y;

#if (defined(_M_IX86)||defined(_M_AMD64))
	if(g_SupportSSE2())
	{
		__m128i mmsum = _mm_setzero_si128(); // four ints
		__m128i mmzero = _mm_setzero_si128();
		for(y=0; y<img.Height(); y++)
		{
			BYTE *prow = (BYTE *)img.Ptr(y);
			for(x=0; x+3<img.Width(); x+=4, prow += 16)
			{
				// sum four pixels at once
				__m128i mm1 = _mm_loadu_si128((__m128i *)prow); // load 16 bytes
				__m128i mm2 = _mm_unpacklo_epi8(mm1, mmzero); // lower 8 bytes -> 16 bit
				__m128i mm3 = _mm_unpackhi_epi8(mm1, mmzero); // upper 8 bytes -> 16 bit
				__m128i mm4 = _mm_add_epi16(mm2, mm3); // add 16 bit
				mm2 = _mm_unpacklo_epi16(mm4, mmzero); // lower 4 shorts -> 32 bit
				mm3 = _mm_unpackhi_epi16(mm4, mmzero); // upper 4 shorts -> 32 bit
				mm4 = _mm_add_epi32(mm2, mm3); // add 32 bit
				mmsum = _mm_add_epi32(mmsum, mm4);
			}
			for( ; x<img.Width(); x++, prow += 4)
			{
				mmsum.m128i_u32[0] += prow[0];
				mmsum.m128i_u32[1] += prow[1];
				mmsum.m128i_u32[2] += prow[2];
				mmsum.m128i_u32[3] += prow[3];
			}
		}
		float normalization = (float)img.Width() * img.Height() * 255;
		averageColor.b = mmsum.m128i_u32[0] / normalization;
		averageColor.g = mmsum.m128i_u32[1] / normalization;
		averageColor.r = mmsum.m128i_u32[2] / normalization;
		averageColor.a = mmsum.m128i_u32[3] / normalization;
	}
	else
#endif
    {
		int rgsum[4];
		rgsum[0] = rgsum[1] = rgsum[2] = rgsum[3] = 0;
		for(y=0; y<img.Height(); y++)
		{
			BYTE *prow = (BYTE *)img.Ptr(y);
			for(x=0 ; x<img.Width(); x++, prow += 4)
			{
				rgsum[0] += prow[0];
				rgsum[1] += prow[1];
				rgsum[2] += prow[2];
				rgsum[3] += prow[3];
			}
		}
		float normalization = (float)img.Width() * img.Height() * 255;
		averageColor.b = rgsum[0] / normalization;
		averageColor.g = rgsum[1] / normalization;
		averageColor.r = rgsum[2] / normalization;
		averageColor.a = rgsum[3] / normalization;
	}
}


HRESULT vt::VtBuildHistogram(const CByteImg &img, int *piHist)
{
    if(piHist==NULL)
        return E_POINTER;

    if(img.Bands()!=1)
        return E_INVALIDARG;

    memset(piHist, 0, 256*sizeof(int));

    int x,y;
    int w = img.Width();
    int h = img.Height();
    for(y=0; y<h; y++)
    {
        const Byte *p = img.Ptr(y);
        for(x=0; x<w; x++)
            piHist[p[x]]++;
    }
    return NOERROR;
}

HRESULT vt::VtBuildHistogram(const CByteImg &img, int *piHistR, int *piHistG, int *piHistB)
{
    if(piHistR==NULL || piHistG==NULL || piHistB==NULL)
        return E_POINTER;

    if(img.Bands()!=3 && img.Bands()!=4)
        return E_INVALIDARG;

    memset(piHistR, 0, 256*sizeof(int));
    memset(piHistG, 0, 256*sizeof(int));
    memset(piHistB, 0, 256*sizeof(int));

    int skip = img.Bands()==3 ? 0 : 1;
    int x,y;
    int w = img.Width();
    int h = img.Height();
    for(y=0; y<h; y++)
    {
        const Byte *p = img.Ptr(y);
        for(x=0; x<w; x++)
        {
            piHistB[*p++]++;
            piHistG[*p++]++;
            piHistR[*p++]++;
            p += skip;
        }
    }
    return NOERROR;
}

//+-----------------------------------------------------------------------------
//
// Class: CTypedLogHistogram
// 
//------------------------------------------------------------------------------
void
CLogHistogram::Clear()
{
	if( m_pHistogram )
	{
#if defined(VT_OSX) || defined(VT_ANDROID)
		memset(m_pHistogram, 0, m_uElementSize*(1<<m_uTotalBits));
#else
		ZeroMemory( m_pHistogram, m_uElementSize*(1<<m_uTotalBits) );
#endif
	}
}

HRESULT
CLogHistogram::Create(UInt16 countbytes, UInt16 valdepth, 
					  short iExpMin, short iExpMax)
{
	VT_HR_BEGIN();

    if (m_uElementSize * (1 << m_uTotalBits) < countbytes * (1 << valdepth))
    {
	    delete[] (BYTE*)m_pHistogram;
	    m_pHistogram = VT_NOTHROWNEW BYTE[countbytes*(1<<valdepth)];
	    VT_PTR_OOM_EXIT( m_pHistogram );
    }

	m_uElementSize = countbytes;
	m_iExponentMin  = iExpMin;
	m_iExponentMax  = iExpMax;
	m_uTotalBits    = valdepth;

	// compute necessary exp bits
	UInt32 uExpRange = iExpMax - iExpMin;
	m_uExpBits = 0;
	while( uExpRange )
	{
		uExpRange >>= 1;
		m_uExpBits++;
	}

    if (m_uMantissaBits < m_uTotalBits - m_uExpBits)
    {
        delete[] m_pMantissaLogs;
        m_pMantissaLogs = VT_NOTHROWNEW UInt32[(size_t)1 << (m_uTotalBits - m_uExpBits)];
	    VT_PTR_OOM_EXIT( m_pMantissaLogs );
    }

    m_uMantissaBits = m_uTotalBits - m_uExpBits;

    for (int i = 0; i < 1 << m_uMantissaBits; i++)
        m_pMantissaLogs[i] = F2I( (float) (1 << m_uMantissaBits) *
		(float) VT_LOG2E * logf(1.f + (float) i / (float) (1 << m_uMantissaBits)) );

	Clear();

    VT_HR_END();
}

//+-----------------------------------------------------------------------------
//
// LogHisto functions
// 
//------------------------------------------------------------------------------
template<> UInt32
vt::ConvertValToLogHistoBin(float v, const CLogHistogram& histo,
                            bool bIncludeFraction)
{
	// for now 0 and negative #s map to min value
	UInt32 tmp = *(UInt32*)(&v);
	if( (tmp & 0x80000000) || (tmp == 0) )
	{
		return 0;
	}

	UInt32 mantbits, expbits, fracbits;

    UInt32 shift = (23 - histo.GetMantissaBitCount());

	int exp = GetExponent(v);
	if( exp < histo.GetExpMin() )
	{
		expbits  = 0;
		mantbits = 0;
        fracbits = 0;
	}
	else if( exp > histo.GetExpMax() )
	{
		expbits  = (histo.GetExpMax() - histo.GetExpMin()) << 
			histo.GetMantissaBitCount();
		mantbits = histo.GetMantissaMask();
        fracbits = (1 << shift) - 1;
	}
	else
	{
		expbits = (exp - histo.GetExpMin()) << histo.GetMantissaBitCount();

		mantbits = GetMantissa(v) >> shift;

        // compute the log mantissa
        mantbits = histo.GetMantissaLog(mantbits);

        fracbits = GetMantissa(v) & ((1 << shift) - 1);
	}

	VT_ASSERT( UInt32(expbits | mantbits) < (UInt32(1) << histo.GetTotalBitCount()) );

	return !bIncludeFraction ? (expbits | mantbits) :
        (((expbits | mantbits) << shift) | fracbits); 
}

template<> UInt32 
vt::ConvertValToLogHistoBin(HALF_FLOAT v, const CLogHistogram& histo,
                            bool bIncludeFraction)
{
	// for now 0 and negative #s map to min value
	UInt16 tmp = *(UInt16*)(&v);
	if( (tmp & 0x8000) || (tmp == 0) )
	{
		return 0;
	}

	UInt32 mantbits, expbits, fracbits;

	// TODO: bug if mantissa of hist is > 10 bits
	UInt32 shift = (10 - histo.GetMantissaBitCount());

	int exp = GetExponent(v);
	if( exp < histo.GetExpMin() )
	{
		expbits  = 0;
		mantbits = 0;
        fracbits = 0;
	}
	else if( exp > histo.GetExpMax() )
	{
		expbits  = (histo.GetExpMax() - histo.GetExpMin()) << 
			histo.GetMantissaBitCount();
		mantbits = histo.GetMantissaMask();
        fracbits = (1 << shift) - 1;
	}
	else
	{
		expbits = (exp - histo.GetExpMin()) << histo.GetMantissaBitCount();   

		mantbits = GetMantissa(v) >> shift;

        // compute the log mantissa
        mantbits = histo.GetMantissaLog(mantbits);

        fracbits = GetMantissa(v) & ((1 << shift) - 1);
	}

	VT_ASSERT( UInt32(expbits | mantbits) < (UInt32(1) << histo.GetTotalBitCount()) );

	return !bIncludeFraction ? (expbits | mantbits) :
        (((expbits | mantbits) << shift) | fracbits); 
}


UInt32
vt::ConvertLogHistoBin(UInt32 index, const CLogHistogram& histoSrc, 
					   const CLogHistogram& histoDst)
{
	UInt32 mantbits, expbits;

	// compute the new exponent
	int exp  = histoSrc.GetExpMin() + (index >> histoSrc.GetMantissaBitCount());
	if( exp < histoDst.GetExpMin() )
	{
		expbits  = 0;
		mantbits = 0;
	}
	else if( exp > histoDst.GetExpMax() )
	{
		expbits  = (histoDst.GetExpMax() - histoDst.GetExpMin()) << 
			histoDst.GetMantissaBitCount();
		mantbits = histoDst.GetMantissaMask();
	}
	else
	{
		expbits = (exp - histoDst.GetExpMin()) << histoDst.GetMantissaBitCount();

		UInt32 bcs = histoSrc.GetMantissaBitCount();
		UInt32 bcd = histoDst.GetMantissaBitCount();
		UInt32 srcmantbits = index & histoSrc.GetMantissaMask(); 
		if( bcd >= bcs )
		{
			mantbits =  srcmantbits << (bcd-bcs);
		}
		else
		{
			UInt32 shift = bcs - bcd;   
			mantbits = srcmantbits >> shift;
		}
	}

	VT_ASSERT( UInt32(expbits | mantbits) < 
			  (UInt32(1) << histoDst.GetTotalBitCount()) );
	return expbits | mantbits; 
}

UInt32
vt::BuildLogHistogram(const CFloatImg& img, CFloatLogHistogram& histo)
{
	int iBands = img.Bands();

	UInt32 uTotCnt = 0;
	if( iBands == 1 )
	{
		for( int y = 0; y < img.Height(); y++ )
		{
			const float* pS = img.Ptr(y);
            int x = 0;
#if (defined(_M_IX86)||defined(_M_AMD64))
            if (g_SupportSSE2())
            {
			    for( ; x < img.Width() - 3; x += 4, pS += 4 )
                {
                    UInt32 packed[4] = { 0 };
                    for (int i = 0; i < 4; i++)
                    {
                        if (pS[i] > 0.f)
                        {
                            packed[i] = ConvertValToLogHistoBin(pS[i], histo, true);
                            uTotCnt++;
                        }
                    }
					histo.AccSSE(packed);
                }
            }
#endif
			for( ; x < img.Width(); x++, pS++ )
			{
				if( *pS > 0 )
				{
					UInt32 packed = ConvertValToLogHistoBin(*pS, histo, true);
					histo.Acc(packed);
					uTotCnt++;
				}
			}
		}
	}
	else
	{
		VT_ASSERT( iBands == 3 || iBands == 4 );
		for( int y = 0; y < img.Height(); y++ )
		{
			const float* pS = img.Ptr(y);
			for( int x = 0; x < img.Width(); x++, pS+=iBands )
			{
				float luma = VtLumaFromRGB_CCIR601YPbPr(pS[2], pS[1], pS[0]);
				if( luma > 0 )
				{
					UInt32 packed = ConvertValToLogHistoBin(luma, histo, true);
					histo.Acc(packed);
					uTotCnt++;
				}
			}
		}
	}
	return uTotCnt;
}

template <class T, class H> UInt32
vt::BuildLogHistogram(const CTypedImg<T>& img, CTypedLogHistogram<H>& histo)
{
	int iBands = img.Bands();

	UInt32 uTotCnt = 0;
	if( iBands == 1 )
	{
		for( int y = 0; y < img.Height(); y++ )
		{
			const T* pS = img.Ptr(y);
			for( int x = 0; x < img.Width(); x++, pS++ )
			{
				if( *pS > 0 )
				{
					UInt32 packed = ConvertValToLogHistoBin(*pS, histo);
					VT_ASSERT( packed < UInt32(1<<histo.GetTotalBitCount()) );
					histo.Acc(packed);
					uTotCnt++;
				}
			}
		}
	}
	else
	{
		VT_ASSERT( iBands == 3 || iBands == 4 );
		for( int y = 0; y < img.Height(); y++ )
		{
			const T* pS = img.Ptr(y);
			for( int x = 0; x < img.Width(); x++, pS+=iBands )
			{
				float luma = VtLumaFromRGB_CCIR601YPbPr(pS[2], pS[1], pS[0]);
				if( luma > 0 )
				{
					UInt32 packed = ConvertValToLogHistoBin(luma, histo);
					VT_ASSERT( packed < UInt32(1<<histo.GetTotalBitCount()) );
					histo.Acc(packed);
					uTotCnt++;
				}
			}
		}
	}
	return uTotCnt;
}

template <class H> UInt32
vt::BuildLogHistogram(const CImg& img, CTypedLogHistogram<H>& histo)
{
	UInt32 uTotCnt = 0;
	switch( EL_FORMAT(img.GetType()) )
	{
	case EL_FORMAT_FLOAT:
		uTotCnt = BuildLogHistogram((CFloatImg&)img, histo);
		break;
	default:
		VT_ASSERT(0);
		break;  
	}
	return uTotCnt;
}

template <class T> void
vt::ComputeExponentHistogram(const CTypedImg<T>& imgSrc, UInt32* pHisto)
{
	// assumes gray scale images
	VT_ASSERT( imgSrc.Bands() == 1 );

#if defined(VT_OSX) || defined(VT_ANDROID)
	memset(pHisto, 0, 256*sizeof(UInt32));
#else
	ZeroMemory( pHisto, 256*sizeof(UInt32) );
#endif

	for( int y = 0; y < imgSrc.Height(); y++ )
	{
		const T* pS = imgSrc.Ptr(y);
		for( int x = 0; x < imgSrc.Width(); x++, pS++ )
		{
			// TODO: handle NAN as well
			if( *(UInt32*)pS != 0 )
			{
				int iExp = GetExponent(*pS);
				VT_ASSERT( iExp != -127 );
				pHisto[BYTE(iExp&0xff)]++;
			}
		}
	}
}

void
vt::ComputeExponentHistogram(const CImg& img, UInt32* pHisto)
{
	switch( EL_FORMAT(img.GetType()) )
	{
	case EL_FORMAT_FLOAT:
		ComputeExponentHistogram((CFloatImg&)img, pHisto);
		break;
	default:
		VT_ASSERT(0);
		break;  
	}
}
//+---------------------------------------------------------------------------
//
// Force Instantiation
//
//----------------------------------------------------------------------------
template UInt32
vt::BuildLogHistogram(const CImg& img, CTypedLogHistogram<UInt16>& histo);
template UInt32
vt::BuildLogHistogram(const CImg& img, CTypedLogHistogram<UInt32>& histo);
template UInt32
vt::BuildLogHistogram(const CImg& img, CTypedLogHistogram<float>& histo);

template void
vt::ComputeExponentHistogram(const CTypedImg<float>& imgSrc, UInt32*);
template void
vt::ComputeExponentHistogram(const CTypedImg<HALF_FLOAT>& imgSrc, UInt32*);

//+-----------------------------------------------------------------------------
//
// Function: SmoothHistogram
// 
// Synopsis: a helper function that smooths a histogram using a 12421 filter
// 
//------------------------------------------------------------------------------
void vt::SmoothHistogram(UInt32 bins, const float* pfSrc, float* pfDst)
{
    float fSrc[3] = { 0 };
    UInt32 imax = VtMin((UInt32)3, bins);
    ANALYZE_ASSUME(imax <= 3);
    for (UInt32 i = 0; i < imax; i++)
    {
        fSrc[i] = pfSrc[i];
        pfDst[i] = 0;
    }

    for (UInt32 i = 0; i < bins; i++)
    {
        float count = fSrc[i % 3];

        pfDst[i] += 0.4f * count;
        if (i > 0)
        {
            pfDst[i - 1] += 0.2f * count;
            if (i > 1)
                pfDst[i - 2] += 0.1f * count;
            else
                pfDst[i - 1] += 0.1f * count;
        }
        else
            pfDst[i] += 0.3f * count;
        if (i < bins - 1)
        {
            pfDst[i + 1] += 0.2f * count;
            if (i < bins - 2)
                pfDst[i + 2] += 0.1f * count;
            else
                pfDst[i + 1] += 0.1f * count;
        }
        else
            pfDst[i] += 0.3f * count;

        if (i < bins - 3)
        {
            fSrc[i % 3] = pfSrc[i + 3];
            pfDst[i + 3] = 0;
        }
    }
}
