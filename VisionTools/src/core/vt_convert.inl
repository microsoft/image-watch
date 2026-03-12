//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//------------------------------------------------------------------------
#pragma once

#include "vt_function.h"
#include "vt_convert.h"

namespace vt {
	//+-----------------------------------------------------------------------------
	//
	// VtConvertImage
	//
	//------------------------------------------------------------------------------

	////////////////////////////////////////////////////////////////////////////////

	template<typename SrcElT, typename DstElT, typename ImplSrcElT = SrcElT,
		typename ImplDstElT = DstElT>
	struct ConvertOpGenericBase
	{
	public:
		typedef ImplSrcElT ImplSrcElType;
		typedef ImplDstElT ImplDstElType;
		typedef int ParamType;
		typedef int TmpType;

		static const int NumSrcBands = 0;
		static const int NumDstBands = 0;
		static const int NumSrcElPerGroup = 1;
		static const int NumDstElPerGroup = 1;
		static const int NumGroupsPerOpGeneric = 1;

	public:
		void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType&)
		{
			VtConv(&pDst[0], pSrc[0]);
		}
	};

	////////////////////////////////////////////////////////////////////////////////

	template<typename SrcElT, typename DstElT, bool BypassCache>
	struct ConvertOpWithCacheBypass;

	template<bool BypassCache>
	struct ConvertOpWithCacheBypass<Byte, float, BypassCache>
	{
	public:
		typedef Byte ImplSrcElType;
		typedef float ImplDstElType;
		typedef int ParamType;
		struct TmpType
		{
#if defined(VT_SSE_SUPPORTED)
			__m128i x7; __m128 xf6;
#if defined(VT_AVX_SUPPORTED)
			__m256 scale;
#endif
#elif defined(VT_NEON_SUPPORTED)
			float32_t scale;
#endif
		};

		static const int NumSrcBands = 0;
		static const int NumDstBands = 0;
		static const int NumSrcElPerGroup = 1;
		static const int NumDstElPerGroup = 1;
		static const int NumGroupsPerOpGeneric = 1;
		static const int NumGroupsPerOpSSE2 = 16;
		static const int NumGroupsPerOpAVX = 8;
		static const int NumGroupsPerOpNEON = 8;

	public:
		void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType&)
		{
			VtConv(&pDst[0], pSrc[0]);
		}

#if defined(VT_SSE_SUPPORTED)
		void InitSSE2(ParamType*, TmpType& tmp)
		{
			tmp.x7 = _mm_setzero_si128();	 // x7 = 0
			tmp.xf6 = _mm_set1_ps(1.f / float(ElTraits<Byte>::MaxVal()));
		}

		void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType& tmp)
		{
			// load
			__m128i x0 = _mm_loadu_si128((__m128i*)pSrc);

			// convert 8bit to 16bit
			__m128i x2 = _mm_unpackhi_epi8(x0, tmp.x7);  // 2 instruc
			x0 = _mm_unpacklo_epi8(x0, tmp.x7);

			// convert 16bit to 32bit
			__m128i x3 = _mm_unpackhi_epi16(x2, tmp.x7); // 2 insturc
			x2 = _mm_unpacklo_epi16(x2, tmp.x7);
			__m128i x1 = _mm_unpackhi_epi16(x0, tmp.x7); // 2 insturc
			x0 = _mm_unpacklo_epi16(x0, tmp.x7);

			// convert 32bit to float
			__m128 x0f = _mm_cvtepi32_ps(x0);
			__m128 x1f = _mm_cvtepi32_ps(x1);
			__m128 x2f = _mm_cvtepi32_ps(x2);
			__m128 x3f = _mm_cvtepi32_ps(x3);

			// mul by 1./255.
			x0f = _mm_mul_ps(x0f, tmp.xf6);
			x1f = _mm_mul_ps(x1f, tmp.xf6);
			x2f = _mm_mul_ps(x2f, tmp.xf6);
			x3f = _mm_mul_ps(x3f, tmp.xf6);

			// store floats
			StoreIntrinsicSSE<BypassCache>::StoreAligned(pDst, x0f);
			StoreIntrinsicSSE<BypassCache>::StoreAligned(pDst + 4, x1f);
			StoreIntrinsicSSE<BypassCache>::StoreAligned(pDst + 8, x2f);
			StoreIntrinsicSSE<BypassCache>::StoreAligned(pDst + 12, x3f);
		}

#if defined(VT_AVX_SUPPORTED)
		void InitAVX(ParamType*, TmpType& tmp)
		{
			tmp.scale = _mm256_set1_ps(1.f / float(ElTraits<Byte>::MaxVal()));
		}

		void EvalAVX(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType& tmp)
		{
			__m128i src128 =
				_mm_cvtepu8_epi32(_mm_castps_si128(_mm_load_ss((float*)pSrc)));
			__m256i src256 = _mm256_castsi128_si256(src128);

			src128 =
				_mm_cvtepu8_epi32(_mm_castps_si128(_mm_load_ss((float*)(pSrc + 4))));
			src256 = _mm256_insertf128_si256(src256, src128, 1);

			StoreIntrinsicAVX<BypassCache>::StoreAligned(pDst,
				_mm256_mul_ps(_mm256_cvtepi32_ps(src256), tmp.scale));
		}
#endif
#elif defined(VT_NEON_SUPPORTED)
		void InitNEON(ParamType*, TmpType& tmp)
		{
			tmp.scale = 1.f / float(ElTraits<Byte>::MaxVal());
		}

		void EvalNEON(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType& tmp)
		{
			uint32x2_t u8 = vld1_u32(reinterpret_cast<const uint32_t*>(pSrc));
			uint16x4_t u16_0 = vget_low_u16(vmovl_u8(vreinterpret_u8_u32(u8)));
			uint16x4_t u16_1 = vget_high_u16(vmovl_u8(vreinterpret_u8_u32(u8)));
			uint32x4_t u32_0 = vmovl_u16(u16_0);
			uint32x4_t u32_1 = vmovl_u16(u16_1);
			float32x4_t f32s_0 = vmulq_n_f32(vcvtq_f32_u32(u32_0), tmp.scale);
			float32x4_t f32s_1 = vmulq_n_f32(vcvtq_f32_u32(u32_1), tmp.scale);
			vst1q_f32_ex(pDst, f32s_0, 128);
			vst1q_f32_ex(&pDst[4], f32s_1, 128);
		}
#endif
	};

	////////////////////////////////////////////////////////////////////////////////

	template<bool BypassCache>
	struct ConvertOpWithCacheBypass<float, Byte, BypassCache>
	{
	public:
		typedef float ImplSrcElType;
		typedef Byte ImplDstElType;
		typedef int ParamType;
		struct TmpType
		{
#if defined(VT_SSE_SUPPORTED)
			__m128 zero;
			__m128 x7;
#if defined(VT_AVX_SUPPORTED)
			__m256 avxMaxVal;
			__m256 avxZero;
#endif
#elif defined(VT_NEON_SUPPORTED)
			float32x4_t scale;
			float32x4_t bias;
#endif
		};

		static const int NumSrcBands = 0;
		static const int NumDstBands = 0;
		static const int NumSrcElPerGroup = 1;
		static const int NumDstElPerGroup = 1;
		static const int NumGroupsPerOpGeneric = 1;
		static const int NumGroupsPerOpSSE2 = 16;
		static const int NumGroupsPerOpAVX = 32;
        static const int NumGroupsPerOpNEON = 8;

	public:
		void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType&)
		{
			VtConv(&pDst[0], pSrc[0]);
		}

#if defined(VT_SSE_SUPPORTED)
		void InitSSE2(ParamType*, TmpType& tmp)
		{
			tmp.x7 = _mm_set1_ps(float(ElTraits<Byte>::MaxVal()));
			tmp.zero = _mm_set1_ps(0.f);
		}

		void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType& tmp)
		{
			// read 1st 4 floats and convert to int32
			__m128  x4 = _mm_loadu_ps(pSrc);
			x4 = _mm_mul_ps(x4, tmp.x7);
			x4 = _mm_max_ps(x4, tmp.zero);
			x4 = _mm_min_ps(x4, tmp.x7);
			__m128i x4i = _mm_cvtps_epi32(x4);

			// read 2nd 4 floats, convert to int32 and pack
			x4 = _mm_loadu_ps(pSrc + 4);
			x4 = _mm_mul_ps(x4, tmp.x7);
			x4 = _mm_max_ps(x4, tmp.zero);
			x4 = _mm_min_ps(x4, tmp.x7);
			__m128i x5i = _mm_cvtps_epi32(x4);
			x4i = _mm_packs_epi32(x4i, x5i);

			// read 3rd 4 floats and convert to int32
			x4 = _mm_loadu_ps(pSrc + 8);
			x4 = _mm_mul_ps(x4, tmp.x7);
			x4 = _mm_max_ps(x4, tmp.zero);
			x4 = _mm_min_ps(x4, tmp.x7);
			x5i = _mm_cvtps_epi32(x4);

			// read 4rd 4 floats, convert to int32 and pack
			x4 = _mm_loadu_ps(pSrc + 12);
			x4 = _mm_mul_ps(x4, tmp.x7);
			x4 = _mm_max_ps(x4, tmp.zero);
			x4 = _mm_min_ps(x4, tmp.x7);
			__m128i x6i = _mm_cvtps_epi32(x4);
			x5i = _mm_packs_epi32(x5i, x6i);
			x4i = _mm_packus_epi16(x4i, x5i);

			// store result
			StoreIntrinsicSSE<BypassCache>::StoreAligned(pDst, x4i);
		}

#if defined(VT_AVX_SUPPORTED)
		void InitAVX(ParamType*, TmpType& tmp)
		{
			tmp.avxMaxVal = _mm256_set1_ps(float(ElTraits<Byte>::MaxVal()));
			tmp.avxZero = _mm256_set1_ps(0.f);
		}

		void EvalAVX(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType& tmp)
		{
			// load the values
			__m256 f0 = _mm256_mul_ps(_mm256_loadu_ps(pSrc), tmp.avxMaxVal);
			f0 = _mm256_max_ps(f0, tmp.avxZero);
			f0 = _mm256_min_ps(f0, tmp.avxMaxVal);
			__m256 f1 = _mm256_mul_ps(_mm256_loadu_ps(pSrc + 8), tmp.avxMaxVal);
			f1 = _mm256_max_ps(f1, tmp.avxZero);
			f1 = _mm256_min_ps(f1, tmp.avxMaxVal);
			__m256 f2 = _mm256_mul_ps(_mm256_loadu_ps(pSrc + 16), tmp.avxMaxVal);
			f2 = _mm256_max_ps(f2, tmp.avxZero);
			f2 = _mm256_min_ps(f2, tmp.avxMaxVal);
			__m256 f3 = _mm256_mul_ps(_mm256_loadu_ps(pSrc + 24), tmp.avxMaxVal);
			f3 = _mm256_max_ps(f3, tmp.avxZero);
			f3 = _mm256_min_ps(f3, tmp.avxMaxVal);

			// convert 32x floats to int32
			__m256i x0 = _mm256_cvtps_epi32(f0);
			__m256i x1 = _mm256_cvtps_epi32(f1);
			__m256i x2 = _mm256_cvtps_epi32(f2);
			__m256i x3 = _mm256_cvtps_epi32(f3);

			// convert 32x int32 to uint16
			__m128i x0_16 = _mm_packus_epi32(_mm256_castsi256_si128(x0),
				_mm256_extractf128_si256(x0, 1));
			__m128i x1_16 = _mm_packus_epi32(_mm256_castsi256_si128(x1),
				_mm256_extractf128_si256(x1, 1));
			__m128i x2_16 = _mm_packus_epi32(_mm256_castsi256_si128(x2),
				_mm256_extractf128_si256(x2, 1));
			__m128i x3_16 = _mm_packus_epi32(_mm256_castsi256_si128(x3),
				_mm256_extractf128_si256(x3, 1));

			// convert 32x uint16 to uint8
			__m128i x0_8 = _mm_packus_epi16(x0_16, x1_16);
			__m128i x1_8 = _mm_packus_epi16(x2_16, x3_16);

			// pack results into an 256 bit register
			__m256 packed = _mm256_castps128_ps256(_mm_castsi128_ps(x0_8));

			StoreIntrinsicAVX<BypassCache>::StoreAligned(pDst,
				_mm256_insertf128_ps(packed, _mm_castsi128_ps(x1_8), 1));
		}
#endif
#elif defined(VT_NEON_SUPPORTED)

		void InitNEON(ParamType*, TmpType& tmp)
		{
			tmp.scale = vdupq_n_f32((float)ElTraits<Byte>::MaxVal());
			tmp.bias = vdupq_n_f32(0.5f);
		}

		void EvalNEON(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType& tmp)
		{
			float32x4_t f32_0 = vld1q_f32(pSrc);
			float32x4_t f32_1 = vld1q_f32(&pSrc[4]);
			float32x4_t f32s_0 = vmulq_f32(f32_0, tmp.scale);
			float32x4_t f32s_1 = vmulq_f32(f32_1, tmp.scale);
			f32s_0 = vaddq_f32(f32s_0, tmp.bias); // bias for 'round to nearest'
			f32s_1 = vaddq_f32(f32s_1, tmp.bias);
			uint32x4_t u32_0 = vcvtq_u32_f32(f32s_0);
			uint32x4_t u32_1 = vcvtq_u32_f32(f32s_1);
			uint16x8_t u16 = vcombine_u16(vqmovn_u32(u32_0), vqmovn_u32(u32_1));
			uint8x8_t u8 = vqmovn_u16(u16);
			vst1_u8_ex(pDst, u8, 64);
		}
#endif
	};

	////////////////////////////////////////////////////////////////////////////////

	template<bool BypassCache>
	struct ConvertOpWithCacheBypass<UInt16, float, BypassCache>
	{
	public:
		typedef UInt16 ImplSrcElType;
		typedef float ImplDstElType;
		typedef int ParamType;
		struct TmpType
		{
#if defined(VT_SSE_SUPPORTED)
			__m128i x7; __m128 x6f;
#if defined(VT_AVX_SUPPORTED)
			__m256 scale;
#endif
#endif
		};

		static const int NumSrcBands = 0;
		static const int NumDstBands = 0;
		static const int NumSrcElPerGroup = 1;
		static const int NumDstElPerGroup = 1;
		static const int NumGroupsPerOpGeneric = 1;
		static const int NumGroupsPerOpSSE2 = 8;
		static const int NumGroupsPerOpAVX = 8;

	public:
		void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType&)
		{
			VtConv(&pDst[0], pSrc[0]);
		}

#if defined(VT_SSE_SUPPORTED)
		void InitSSE2(ParamType*, TmpType& tmp)
		{
			tmp.x7 = _mm_setzero_si128();	 // x7 = 0
			tmp.x6f = _mm_set1_ps(1.f / 65535.f);
		}

		void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType& tmp)
		{
			// load
			__m128i x0 = _mm_loadu_si128((__m128i*)pSrc);

			// convert 16bit to 32bit
			__m128i x1 = _mm_unpackhi_epi16(x0, tmp.x7); // 2 insturc
			x0 = _mm_unpacklo_epi16(x0, tmp.x7);

			// convert 32bit to float
			__m128 x0f = _mm_cvtepi32_ps(x0);
			__m128 x1f = _mm_cvtepi32_ps(x1);

			// mul by 1./65535.
			x0f = _mm_mul_ps(x0f, tmp.x6f);
			x1f = _mm_mul_ps(x1f, tmp.x6f);

			// store floats
			StoreIntrinsicSSE<BypassCache>::StoreAligned(pDst, x0f);
			StoreIntrinsicSSE<BypassCache>::StoreAligned(pDst + 4, x1f);
		}

#if defined(VT_AVX_SUPPORTED)
		void InitAVX(ParamType*, TmpType& tmp)
		{
			tmp.scale = _mm256_set1_ps(1.f / float(ElTraits<UInt16>::MaxVal()));
		}

		void EvalAVX(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType& tmp)
		{
			__m128i src128;

			src128.m128i_u64[0] = *(uint64_t*)pSrc;
			src128 = _mm_cvtepu16_epi32(src128);
			__m256i src256 = _mm256_castsi128_si256(src128);

			src128.m128i_u64[0] = *(uint64_t*)(pSrc + 4);
			src128 = _mm_cvtepu16_epi32(src128);
			src256 = _mm256_insertf128_si256(src256, src128, 1);

			StoreIntrinsicAVX<BypassCache>::StoreAligned(pDst,
				_mm256_mul_ps(_mm256_cvtepi32_ps(src256), tmp.scale));
		}
#endif
#endif
	};

	////////////////////////////////////////////////////////////////////////////////

	template<bool BypassCache>
	struct ConvertOpWithCacheBypass<float, UInt16, BypassCache>
	{
	public:
		typedef float ImplSrcElType;
		typedef UInt16 ImplDstElType;
		typedef int ParamType;
		struct TmpType
		{
#if defined(VT_SSE_SUPPORTED)
			__m128 zero;
			__m128i x0i, x1i; __m128 x7;
#if defined(VT_AVX_SUPPORTED)
			__m256 avxMaxVal;
			__m256 avxZero;
#endif
#endif
		};

		static const int NumSrcBands = 0;
		static const int NumDstBands = 0;
		static const int NumSrcElPerGroup = 1;
		static const int NumDstElPerGroup = 1;
		static const int NumGroupsPerOpGeneric = 1;
		static const int NumGroupsPerOpSSE2 = 8;
		static const int NumGroupsPerOpSSE4_1 = 8;
		static const int NumGroupsPerOpAVX = 16;

	public:
		void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType&)
		{
			VtConv(&pDst[0], pSrc[0]);
		}

#if defined(VT_SSE_SUPPORTED)
		void InitSSE2(ParamType*, TmpType& tmp)
		{
			tmp.x0i = _mm_set1_epi32(0x8000);
			tmp.x1i = _mm_set1_epi16(short(-0x8000));
			tmp.x7 = _mm_set1_ps(65535.f);
			tmp.zero = _mm_set1_ps(0.f);
		}

		void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType& tmp)
		{
			// read 1st 4 float, shift range to signed short, convert to int32
			__m128  x4 = _mm_loadu_ps(pSrc);
			x4 = _mm_mul_ps(x4, tmp.x7);
			x4 = _mm_max_ps(x4, tmp.zero);
			x4 = _mm_min_ps(x4, tmp.x7);
			__m128i x4i = _mm_cvtps_epi32(x4);
			x4i = _mm_sub_epi32(x4i, tmp.x0i);

			// read 2nd 4 floats, shift range to signed short, convert to int32
			x4 = _mm_loadu_ps(pSrc + 4);
			x4 = _mm_mul_ps(x4, tmp.x7);
			x4 = _mm_max_ps(x4, tmp.zero);
			x4 = _mm_min_ps(x4, tmp.x7);
			__m128i x5i = _mm_cvtps_epi32(x4);
			x5i = _mm_sub_epi32(x5i, tmp.x0i);

			// pack the results and shift range pack to unsigned
			x4i = _mm_packs_epi32(x4i, x5i);
			x4i = _mm_add_epi16(x4i, tmp.x1i);

			// store result
			StoreIntrinsicSSE<BypassCache>::StoreAligned(pDst, x4i);
		}

#if !defined(VT_ANDROID) && !defined(VT_OSX) // some sse4 intrinsics are not included with clang
		void InitSSE4_1(ParamType*, TmpType& tmp)
		{
			tmp.x7 = _mm_set1_ps(65535.f);
		}

		void EvalSSE4_1(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType& tmp)
		{
			// read 1st 4 float, shift range to signed short, convert to int32
			__m128  x4 = _mm_loadu_ps(pSrc);
			x4 = _mm_mul_ps(x4, tmp.x7);
			__m128i x4i = _mm_cvtps_epi32(x4);

			// read 2nd 4 floats, shift range to signed short, convert to int32
			x4 = _mm_loadu_ps(pSrc + 4);
			x4 = _mm_mul_ps(x4, tmp.x7);
			__m128i x5i = _mm_cvtps_epi32(x4);

			// pack the results and shift range pack to unsigned
			x4i = _mm_packus_epi32(x4i, x5i);

			// store result
			StoreIntrinsicSSE<BypassCache>::StoreAligned(pDst, x4i);
		}
#endif

#if defined(VT_AVX_SUPPORTED)
		void InitAVX(ParamType*, TmpType& tmp)
		{
			tmp.avxMaxVal = _mm256_set1_ps(float(ElTraits<UInt16>::MaxVal()));
			tmp.avxZero = _mm256_set1_ps(0.f);
		}

		void EvalAVX(const ImplSrcElType* pSrc, ImplDstElType* pDst,
			ParamType*, TmpType& tmp)
		{
			// load the values
			__m256 f0 = _mm256_mul_ps(_mm256_loadu_ps(pSrc), tmp.avxMaxVal);
			f0 = _mm256_max_ps(f0, tmp.avxZero);
			f0 = _mm256_min_ps(f0, tmp.avxMaxVal);
			__m256 f1 = _mm256_mul_ps(_mm256_loadu_ps(pSrc + 8), tmp.avxMaxVal);
			f1 = _mm256_max_ps(f1, tmp.avxZero);
			f1 = _mm256_min_ps(f1, tmp.avxMaxVal);

			// convert 16x floats to int32
			__m256i x0 = _mm256_cvtps_epi32(f0);
			__m256i x1 = _mm256_cvtps_epi32(f1);

			// convert 16x int32 to uint16
			__m128i x0_16 = _mm_packus_epi32(_mm256_castsi256_si128(x0),
				_mm256_extractf128_si256(x0, 1));
			__m128i x1_16 = _mm_packus_epi32(_mm256_castsi256_si128(x1),
				_mm256_extractf128_si256(x1, 1));

			// pack results into an 256 bit register
			__m256 packed = _mm256_castps128_ps256(_mm_castsi128_ps(x0_16));
			StoreIntrinsicAVX<BypassCache>::StoreAligned(pDst,
				_mm256_insertf128_ps(packed, _mm_castsi128_ps(x1_16), 1));
		}
#endif
#endif
	};

	////////////////////////////////////////////////////////////////////////////////

#define HALF_FLOAT_ROUND

	typedef	union
	{
		float	f;
		DWORD	d;
	} UnionFloatDword;

	struct HALF_FLOAT_CONVERT_STATICS
	{
		static const float fminFloatVal;  // min representable half-float
		static const float fmaxFloatVal;   // max representable half-float
		static const float powf2to112;
		static const float oneOverPowf2to112;

#if defined(VT_SSE_SUPPORTED)
		__m128i addVal;
		__m128i hex80000000;
		__m128i hex00007FFF;
		__m128 oneOverPowf2to112Splatted;
		__m128i andVal;
		__m128 multiplyVal;
		__m128i everyOtherValAnd;
#ifdef	HALF_FLOAT_ROUND
		__m128 m128Round;
#else
		__m128i m128Round;
#endif
		__m128i m128LSB;
		__m128 minFloatVal;
		__m128 maxFloatVal;

		HALF_FLOAT_CONVERT_STATICS()
		{
			if (g_SupportSSE2())
			{
				addVal = _mm_set1_epi32(0x70000000);
				hex80000000 = _mm_set1_epi32(0x80000000);
				hex00007FFF = _mm_set1_epi32(0x00007FFF);
				// 2^-112
				oneOverPowf2to112Splatted = _mm_set1_ps(oneOverPowf2to112);
				//since andVal starts with - sign, we have to be careful
				andVal = _mm_set1_epi32(-0x70002000);
				// 2^112
				multiplyVal = _mm_set1_ps(powf2to112);
				//clear the lowest 16 bits? 
				everyOtherValAnd = _mm_set1_epi32(0xFFFF0000);
#ifdef	HALF_FLOAT_ROUND
				m128Round = _mm_set1_ps((float)(1 << 13));
#else	
				m128Round = _mm_set1_epi32((1 << 12) - 1);
#endif	
				m128LSB = _mm_set1_epi32(1);
				minFloatVal = _mm_set1_ps(-131008.f);
				maxFloatVal = _mm_set1_ps(131008.f);
			}
		}
#endif
	};

	extern const HALF_FLOAT_CONVERT_STATICS g_hfcconst;

#if defined(VT_SSE_SUPPORTED)
	inline void EightBytesOfHalfFloatToFloatSSE2(const HALF_FLOAT* pSrc, float* pDst)
	{
		__m128i answer = _mm_loadl_epi64((const __m128i*) pSrc);

		answer = _mm_unpacklo_epi16(answer, answer);
		answer = _mm_and_si128(g_hfcconst.everyOtherValAnd, answer);
		answer = _mm_srli_epi32(answer, 3);

		answer = _mm_add_epi32(answer, g_hfcconst.addVal);
		answer = _mm_and_si128(g_hfcconst.andVal, answer);
		__m128 answerF = _mm_mul_ps(*((__m128*) &answer),
			g_hfcconst.multiplyVal);

		// TODO: add a _mm_stream version of this
		_mm_storeu_ps(pDst, answerF);
	}

#endif

	// Convert float to half float.
	// From "Fast Half Float Conversions" by Jeroen van der Zijp
	// November 2008 (Revised September 2010)
	// ftp://ftp.fox-toolkit.org/pub/fasthalffloatconversion.pdf
	//
	// Tables generated by f16c.cpp
	//
	static unsigned int g_mantissa_table[2048] =
	{
		0x00000000, 0x33800000, 0x34000000, 0x34400000, 0x34800000, 0x34a00000, 0x34c00000, 0x34e00000,
		0x35000000, 0x35100000, 0x35200000, 0x35300000, 0x35400000, 0x35500000, 0x35600000, 0x35700000,
		0x35800000, 0x35880000, 0x35900000, 0x35980000, 0x35a00000, 0x35a80000, 0x35b00000, 0x35b80000,
		0x35c00000, 0x35c80000, 0x35d00000, 0x35d80000, 0x35e00000, 0x35e80000, 0x35f00000, 0x35f80000,
		0x36000000, 0x36040000, 0x36080000, 0x360c0000, 0x36100000, 0x36140000, 0x36180000, 0x361c0000,
		0x36200000, 0x36240000, 0x36280000, 0x362c0000, 0x36300000, 0x36340000, 0x36380000, 0x363c0000,
		0x36400000, 0x36440000, 0x36480000, 0x364c0000, 0x36500000, 0x36540000, 0x36580000, 0x365c0000,
		0x36600000, 0x36640000, 0x36680000, 0x366c0000, 0x36700000, 0x36740000, 0x36780000, 0x367c0000,
		0x36800000, 0x36820000, 0x36840000, 0x36860000, 0x36880000, 0x368a0000, 0x368c0000, 0x368e0000,
		0x36900000, 0x36920000, 0x36940000, 0x36960000, 0x36980000, 0x369a0000, 0x369c0000, 0x369e0000,
		0x36a00000, 0x36a20000, 0x36a40000, 0x36a60000, 0x36a80000, 0x36aa0000, 0x36ac0000, 0x36ae0000,
		0x36b00000, 0x36b20000, 0x36b40000, 0x36b60000, 0x36b80000, 0x36ba0000, 0x36bc0000, 0x36be0000,
		0x36c00000, 0x36c20000, 0x36c40000, 0x36c60000, 0x36c80000, 0x36ca0000, 0x36cc0000, 0x36ce0000,
		0x36d00000, 0x36d20000, 0x36d40000, 0x36d60000, 0x36d80000, 0x36da0000, 0x36dc0000, 0x36de0000,
		0x36e00000, 0x36e20000, 0x36e40000, 0x36e60000, 0x36e80000, 0x36ea0000, 0x36ec0000, 0x36ee0000,
		0x36f00000, 0x36f20000, 0x36f40000, 0x36f60000, 0x36f80000, 0x36fa0000, 0x36fc0000, 0x36fe0000,
		0x37000000, 0x37010000, 0x37020000, 0x37030000, 0x37040000, 0x37050000, 0x37060000, 0x37070000,
		0x37080000, 0x37090000, 0x370a0000, 0x370b0000, 0x370c0000, 0x370d0000, 0x370e0000, 0x370f0000,
		0x37100000, 0x37110000, 0x37120000, 0x37130000, 0x37140000, 0x37150000, 0x37160000, 0x37170000,
		0x37180000, 0x37190000, 0x371a0000, 0x371b0000, 0x371c0000, 0x371d0000, 0x371e0000, 0x371f0000,
		0x37200000, 0x37210000, 0x37220000, 0x37230000, 0x37240000, 0x37250000, 0x37260000, 0x37270000,
		0x37280000, 0x37290000, 0x372a0000, 0x372b0000, 0x372c0000, 0x372d0000, 0x372e0000, 0x372f0000,
		0x37300000, 0x37310000, 0x37320000, 0x37330000, 0x37340000, 0x37350000, 0x37360000, 0x37370000,
		0x37380000, 0x37390000, 0x373a0000, 0x373b0000, 0x373c0000, 0x373d0000, 0x373e0000, 0x373f0000,
		0x37400000, 0x37410000, 0x37420000, 0x37430000, 0x37440000, 0x37450000, 0x37460000, 0x37470000,
		0x37480000, 0x37490000, 0x374a0000, 0x374b0000, 0x374c0000, 0x374d0000, 0x374e0000, 0x374f0000,
		0x37500000, 0x37510000, 0x37520000, 0x37530000, 0x37540000, 0x37550000, 0x37560000, 0x37570000,
		0x37580000, 0x37590000, 0x375a0000, 0x375b0000, 0x375c0000, 0x375d0000, 0x375e0000, 0x375f0000,
		0x37600000, 0x37610000, 0x37620000, 0x37630000, 0x37640000, 0x37650000, 0x37660000, 0x37670000,
		0x37680000, 0x37690000, 0x376a0000, 0x376b0000, 0x376c0000, 0x376d0000, 0x376e0000, 0x376f0000,
		0x37700000, 0x37710000, 0x37720000, 0x37730000, 0x37740000, 0x37750000, 0x37760000, 0x37770000,
		0x37780000, 0x37790000, 0x377a0000, 0x377b0000, 0x377c0000, 0x377d0000, 0x377e0000, 0x377f0000,
		0x37800000, 0x37808000, 0x37810000, 0x37818000, 0x37820000, 0x37828000, 0x37830000, 0x37838000,
		0x37840000, 0x37848000, 0x37850000, 0x37858000, 0x37860000, 0x37868000, 0x37870000, 0x37878000,
		0x37880000, 0x37888000, 0x37890000, 0x37898000, 0x378a0000, 0x378a8000, 0x378b0000, 0x378b8000,
		0x378c0000, 0x378c8000, 0x378d0000, 0x378d8000, 0x378e0000, 0x378e8000, 0x378f0000, 0x378f8000,
		0x37900000, 0x37908000, 0x37910000, 0x37918000, 0x37920000, 0x37928000, 0x37930000, 0x37938000,
		0x37940000, 0x37948000, 0x37950000, 0x37958000, 0x37960000, 0x37968000, 0x37970000, 0x37978000,
		0x37980000, 0x37988000, 0x37990000, 0x37998000, 0x379a0000, 0x379a8000, 0x379b0000, 0x379b8000,
		0x379c0000, 0x379c8000, 0x379d0000, 0x379d8000, 0x379e0000, 0x379e8000, 0x379f0000, 0x379f8000,
		0x37a00000, 0x37a08000, 0x37a10000, 0x37a18000, 0x37a20000, 0x37a28000, 0x37a30000, 0x37a38000,
		0x37a40000, 0x37a48000, 0x37a50000, 0x37a58000, 0x37a60000, 0x37a68000, 0x37a70000, 0x37a78000,
		0x37a80000, 0x37a88000, 0x37a90000, 0x37a98000, 0x37aa0000, 0x37aa8000, 0x37ab0000, 0x37ab8000,
		0x37ac0000, 0x37ac8000, 0x37ad0000, 0x37ad8000, 0x37ae0000, 0x37ae8000, 0x37af0000, 0x37af8000,
		0x37b00000, 0x37b08000, 0x37b10000, 0x37b18000, 0x37b20000, 0x37b28000, 0x37b30000, 0x37b38000,
		0x37b40000, 0x37b48000, 0x37b50000, 0x37b58000, 0x37b60000, 0x37b68000, 0x37b70000, 0x37b78000,
		0x37b80000, 0x37b88000, 0x37b90000, 0x37b98000, 0x37ba0000, 0x37ba8000, 0x37bb0000, 0x37bb8000,
		0x37bc0000, 0x37bc8000, 0x37bd0000, 0x37bd8000, 0x37be0000, 0x37be8000, 0x37bf0000, 0x37bf8000,
		0x37c00000, 0x37c08000, 0x37c10000, 0x37c18000, 0x37c20000, 0x37c28000, 0x37c30000, 0x37c38000,
		0x37c40000, 0x37c48000, 0x37c50000, 0x37c58000, 0x37c60000, 0x37c68000, 0x37c70000, 0x37c78000,
		0x37c80000, 0x37c88000, 0x37c90000, 0x37c98000, 0x37ca0000, 0x37ca8000, 0x37cb0000, 0x37cb8000,
		0x37cc0000, 0x37cc8000, 0x37cd0000, 0x37cd8000, 0x37ce0000, 0x37ce8000, 0x37cf0000, 0x37cf8000,
		0x37d00000, 0x37d08000, 0x37d10000, 0x37d18000, 0x37d20000, 0x37d28000, 0x37d30000, 0x37d38000,
		0x37d40000, 0x37d48000, 0x37d50000, 0x37d58000, 0x37d60000, 0x37d68000, 0x37d70000, 0x37d78000,
		0x37d80000, 0x37d88000, 0x37d90000, 0x37d98000, 0x37da0000, 0x37da8000, 0x37db0000, 0x37db8000,
		0x37dc0000, 0x37dc8000, 0x37dd0000, 0x37dd8000, 0x37de0000, 0x37de8000, 0x37df0000, 0x37df8000,
		0x37e00000, 0x37e08000, 0x37e10000, 0x37e18000, 0x37e20000, 0x37e28000, 0x37e30000, 0x37e38000,
		0x37e40000, 0x37e48000, 0x37e50000, 0x37e58000, 0x37e60000, 0x37e68000, 0x37e70000, 0x37e78000,
		0x37e80000, 0x37e88000, 0x37e90000, 0x37e98000, 0x37ea0000, 0x37ea8000, 0x37eb0000, 0x37eb8000,
		0x37ec0000, 0x37ec8000, 0x37ed0000, 0x37ed8000, 0x37ee0000, 0x37ee8000, 0x37ef0000, 0x37ef8000,
		0x37f00000, 0x37f08000, 0x37f10000, 0x37f18000, 0x37f20000, 0x37f28000, 0x37f30000, 0x37f38000,
		0x37f40000, 0x37f48000, 0x37f50000, 0x37f58000, 0x37f60000, 0x37f68000, 0x37f70000, 0x37f78000,
		0x37f80000, 0x37f88000, 0x37f90000, 0x37f98000, 0x37fa0000, 0x37fa8000, 0x37fb0000, 0x37fb8000,
		0x37fc0000, 0x37fc8000, 0x37fd0000, 0x37fd8000, 0x37fe0000, 0x37fe8000, 0x37ff0000, 0x37ff8000,
		0x38000000, 0x38004000, 0x38008000, 0x3800c000, 0x38010000, 0x38014000, 0x38018000, 0x3801c000,
		0x38020000, 0x38024000, 0x38028000, 0x3802c000, 0x38030000, 0x38034000, 0x38038000, 0x3803c000,
		0x38040000, 0x38044000, 0x38048000, 0x3804c000, 0x38050000, 0x38054000, 0x38058000, 0x3805c000,
		0x38060000, 0x38064000, 0x38068000, 0x3806c000, 0x38070000, 0x38074000, 0x38078000, 0x3807c000,
		0x38080000, 0x38084000, 0x38088000, 0x3808c000, 0x38090000, 0x38094000, 0x38098000, 0x3809c000,
		0x380a0000, 0x380a4000, 0x380a8000, 0x380ac000, 0x380b0000, 0x380b4000, 0x380b8000, 0x380bc000,
		0x380c0000, 0x380c4000, 0x380c8000, 0x380cc000, 0x380d0000, 0x380d4000, 0x380d8000, 0x380dc000,
		0x380e0000, 0x380e4000, 0x380e8000, 0x380ec000, 0x380f0000, 0x380f4000, 0x380f8000, 0x380fc000,
		0x38100000, 0x38104000, 0x38108000, 0x3810c000, 0x38110000, 0x38114000, 0x38118000, 0x3811c000,
		0x38120000, 0x38124000, 0x38128000, 0x3812c000, 0x38130000, 0x38134000, 0x38138000, 0x3813c000,
		0x38140000, 0x38144000, 0x38148000, 0x3814c000, 0x38150000, 0x38154000, 0x38158000, 0x3815c000,
		0x38160000, 0x38164000, 0x38168000, 0x3816c000, 0x38170000, 0x38174000, 0x38178000, 0x3817c000,
		0x38180000, 0x38184000, 0x38188000, 0x3818c000, 0x38190000, 0x38194000, 0x38198000, 0x3819c000,
		0x381a0000, 0x381a4000, 0x381a8000, 0x381ac000, 0x381b0000, 0x381b4000, 0x381b8000, 0x381bc000,
		0x381c0000, 0x381c4000, 0x381c8000, 0x381cc000, 0x381d0000, 0x381d4000, 0x381d8000, 0x381dc000,
		0x381e0000, 0x381e4000, 0x381e8000, 0x381ec000, 0x381f0000, 0x381f4000, 0x381f8000, 0x381fc000,
		0x38200000, 0x38204000, 0x38208000, 0x3820c000, 0x38210000, 0x38214000, 0x38218000, 0x3821c000,
		0x38220000, 0x38224000, 0x38228000, 0x3822c000, 0x38230000, 0x38234000, 0x38238000, 0x3823c000,
		0x38240000, 0x38244000, 0x38248000, 0x3824c000, 0x38250000, 0x38254000, 0x38258000, 0x3825c000,
		0x38260000, 0x38264000, 0x38268000, 0x3826c000, 0x38270000, 0x38274000, 0x38278000, 0x3827c000,
		0x38280000, 0x38284000, 0x38288000, 0x3828c000, 0x38290000, 0x38294000, 0x38298000, 0x3829c000,
		0x382a0000, 0x382a4000, 0x382a8000, 0x382ac000, 0x382b0000, 0x382b4000, 0x382b8000, 0x382bc000,
		0x382c0000, 0x382c4000, 0x382c8000, 0x382cc000, 0x382d0000, 0x382d4000, 0x382d8000, 0x382dc000,
		0x382e0000, 0x382e4000, 0x382e8000, 0x382ec000, 0x382f0000, 0x382f4000, 0x382f8000, 0x382fc000,
		0x38300000, 0x38304000, 0x38308000, 0x3830c000, 0x38310000, 0x38314000, 0x38318000, 0x3831c000,
		0x38320000, 0x38324000, 0x38328000, 0x3832c000, 0x38330000, 0x38334000, 0x38338000, 0x3833c000,
		0x38340000, 0x38344000, 0x38348000, 0x3834c000, 0x38350000, 0x38354000, 0x38358000, 0x3835c000,
		0x38360000, 0x38364000, 0x38368000, 0x3836c000, 0x38370000, 0x38374000, 0x38378000, 0x3837c000,
		0x38380000, 0x38384000, 0x38388000, 0x3838c000, 0x38390000, 0x38394000, 0x38398000, 0x3839c000,
		0x383a0000, 0x383a4000, 0x383a8000, 0x383ac000, 0x383b0000, 0x383b4000, 0x383b8000, 0x383bc000,
		0x383c0000, 0x383c4000, 0x383c8000, 0x383cc000, 0x383d0000, 0x383d4000, 0x383d8000, 0x383dc000,
		0x383e0000, 0x383e4000, 0x383e8000, 0x383ec000, 0x383f0000, 0x383f4000, 0x383f8000, 0x383fc000,
		0x38400000, 0x38404000, 0x38408000, 0x3840c000, 0x38410000, 0x38414000, 0x38418000, 0x3841c000,
		0x38420000, 0x38424000, 0x38428000, 0x3842c000, 0x38430000, 0x38434000, 0x38438000, 0x3843c000,
		0x38440000, 0x38444000, 0x38448000, 0x3844c000, 0x38450000, 0x38454000, 0x38458000, 0x3845c000,
		0x38460000, 0x38464000, 0x38468000, 0x3846c000, 0x38470000, 0x38474000, 0x38478000, 0x3847c000,
		0x38480000, 0x38484000, 0x38488000, 0x3848c000, 0x38490000, 0x38494000, 0x38498000, 0x3849c000,
		0x384a0000, 0x384a4000, 0x384a8000, 0x384ac000, 0x384b0000, 0x384b4000, 0x384b8000, 0x384bc000,
		0x384c0000, 0x384c4000, 0x384c8000, 0x384cc000, 0x384d0000, 0x384d4000, 0x384d8000, 0x384dc000,
		0x384e0000, 0x384e4000, 0x384e8000, 0x384ec000, 0x384f0000, 0x384f4000, 0x384f8000, 0x384fc000,
		0x38500000, 0x38504000, 0x38508000, 0x3850c000, 0x38510000, 0x38514000, 0x38518000, 0x3851c000,
		0x38520000, 0x38524000, 0x38528000, 0x3852c000, 0x38530000, 0x38534000, 0x38538000, 0x3853c000,
		0x38540000, 0x38544000, 0x38548000, 0x3854c000, 0x38550000, 0x38554000, 0x38558000, 0x3855c000,
		0x38560000, 0x38564000, 0x38568000, 0x3856c000, 0x38570000, 0x38574000, 0x38578000, 0x3857c000,
		0x38580000, 0x38584000, 0x38588000, 0x3858c000, 0x38590000, 0x38594000, 0x38598000, 0x3859c000,
		0x385a0000, 0x385a4000, 0x385a8000, 0x385ac000, 0x385b0000, 0x385b4000, 0x385b8000, 0x385bc000,
		0x385c0000, 0x385c4000, 0x385c8000, 0x385cc000, 0x385d0000, 0x385d4000, 0x385d8000, 0x385dc000,
		0x385e0000, 0x385e4000, 0x385e8000, 0x385ec000, 0x385f0000, 0x385f4000, 0x385f8000, 0x385fc000,
		0x38600000, 0x38604000, 0x38608000, 0x3860c000, 0x38610000, 0x38614000, 0x38618000, 0x3861c000,
		0x38620000, 0x38624000, 0x38628000, 0x3862c000, 0x38630000, 0x38634000, 0x38638000, 0x3863c000,
		0x38640000, 0x38644000, 0x38648000, 0x3864c000, 0x38650000, 0x38654000, 0x38658000, 0x3865c000,
		0x38660000, 0x38664000, 0x38668000, 0x3866c000, 0x38670000, 0x38674000, 0x38678000, 0x3867c000,
		0x38680000, 0x38684000, 0x38688000, 0x3868c000, 0x38690000, 0x38694000, 0x38698000, 0x3869c000,
		0x386a0000, 0x386a4000, 0x386a8000, 0x386ac000, 0x386b0000, 0x386b4000, 0x386b8000, 0x386bc000,
		0x386c0000, 0x386c4000, 0x386c8000, 0x386cc000, 0x386d0000, 0x386d4000, 0x386d8000, 0x386dc000,
		0x386e0000, 0x386e4000, 0x386e8000, 0x386ec000, 0x386f0000, 0x386f4000, 0x386f8000, 0x386fc000,
		0x38700000, 0x38704000, 0x38708000, 0x3870c000, 0x38710000, 0x38714000, 0x38718000, 0x3871c000,
		0x38720000, 0x38724000, 0x38728000, 0x3872c000, 0x38730000, 0x38734000, 0x38738000, 0x3873c000,
		0x38740000, 0x38744000, 0x38748000, 0x3874c000, 0x38750000, 0x38754000, 0x38758000, 0x3875c000,
		0x38760000, 0x38764000, 0x38768000, 0x3876c000, 0x38770000, 0x38774000, 0x38778000, 0x3877c000,
		0x38780000, 0x38784000, 0x38788000, 0x3878c000, 0x38790000, 0x38794000, 0x38798000, 0x3879c000,
		0x387a0000, 0x387a4000, 0x387a8000, 0x387ac000, 0x387b0000, 0x387b4000, 0x387b8000, 0x387bc000,
		0x387c0000, 0x387c4000, 0x387c8000, 0x387cc000, 0x387d0000, 0x387d4000, 0x387d8000, 0x387dc000,
		0x387e0000, 0x387e4000, 0x387e8000, 0x387ec000, 0x387f0000, 0x387f4000, 0x387f8000, 0x387fc000,
		0x38000000, 0x38002000, 0x38004000, 0x38006000, 0x38008000, 0x3800a000, 0x3800c000, 0x3800e000,
		0x38010000, 0x38012000, 0x38014000, 0x38016000, 0x38018000, 0x3801a000, 0x3801c000, 0x3801e000,
		0x38020000, 0x38022000, 0x38024000, 0x38026000, 0x38028000, 0x3802a000, 0x3802c000, 0x3802e000,
		0x38030000, 0x38032000, 0x38034000, 0x38036000, 0x38038000, 0x3803a000, 0x3803c000, 0x3803e000,
		0x38040000, 0x38042000, 0x38044000, 0x38046000, 0x38048000, 0x3804a000, 0x3804c000, 0x3804e000,
		0x38050000, 0x38052000, 0x38054000, 0x38056000, 0x38058000, 0x3805a000, 0x3805c000, 0x3805e000,
		0x38060000, 0x38062000, 0x38064000, 0x38066000, 0x38068000, 0x3806a000, 0x3806c000, 0x3806e000,
		0x38070000, 0x38072000, 0x38074000, 0x38076000, 0x38078000, 0x3807a000, 0x3807c000, 0x3807e000,
		0x38080000, 0x38082000, 0x38084000, 0x38086000, 0x38088000, 0x3808a000, 0x3808c000, 0x3808e000,
		0x38090000, 0x38092000, 0x38094000, 0x38096000, 0x38098000, 0x3809a000, 0x3809c000, 0x3809e000,
		0x380a0000, 0x380a2000, 0x380a4000, 0x380a6000, 0x380a8000, 0x380aa000, 0x380ac000, 0x380ae000,
		0x380b0000, 0x380b2000, 0x380b4000, 0x380b6000, 0x380b8000, 0x380ba000, 0x380bc000, 0x380be000,
		0x380c0000, 0x380c2000, 0x380c4000, 0x380c6000, 0x380c8000, 0x380ca000, 0x380cc000, 0x380ce000,
		0x380d0000, 0x380d2000, 0x380d4000, 0x380d6000, 0x380d8000, 0x380da000, 0x380dc000, 0x380de000,
		0x380e0000, 0x380e2000, 0x380e4000, 0x380e6000, 0x380e8000, 0x380ea000, 0x380ec000, 0x380ee000,
		0x380f0000, 0x380f2000, 0x380f4000, 0x380f6000, 0x380f8000, 0x380fa000, 0x380fc000, 0x380fe000,
		0x38100000, 0x38102000, 0x38104000, 0x38106000, 0x38108000, 0x3810a000, 0x3810c000, 0x3810e000,
		0x38110000, 0x38112000, 0x38114000, 0x38116000, 0x38118000, 0x3811a000, 0x3811c000, 0x3811e000,
		0x38120000, 0x38122000, 0x38124000, 0x38126000, 0x38128000, 0x3812a000, 0x3812c000, 0x3812e000,
		0x38130000, 0x38132000, 0x38134000, 0x38136000, 0x38138000, 0x3813a000, 0x3813c000, 0x3813e000,
		0x38140000, 0x38142000, 0x38144000, 0x38146000, 0x38148000, 0x3814a000, 0x3814c000, 0x3814e000,
		0x38150000, 0x38152000, 0x38154000, 0x38156000, 0x38158000, 0x3815a000, 0x3815c000, 0x3815e000,
		0x38160000, 0x38162000, 0x38164000, 0x38166000, 0x38168000, 0x3816a000, 0x3816c000, 0x3816e000,
		0x38170000, 0x38172000, 0x38174000, 0x38176000, 0x38178000, 0x3817a000, 0x3817c000, 0x3817e000,
		0x38180000, 0x38182000, 0x38184000, 0x38186000, 0x38188000, 0x3818a000, 0x3818c000, 0x3818e000,
		0x38190000, 0x38192000, 0x38194000, 0x38196000, 0x38198000, 0x3819a000, 0x3819c000, 0x3819e000,
		0x381a0000, 0x381a2000, 0x381a4000, 0x381a6000, 0x381a8000, 0x381aa000, 0x381ac000, 0x381ae000,
		0x381b0000, 0x381b2000, 0x381b4000, 0x381b6000, 0x381b8000, 0x381ba000, 0x381bc000, 0x381be000,
		0x381c0000, 0x381c2000, 0x381c4000, 0x381c6000, 0x381c8000, 0x381ca000, 0x381cc000, 0x381ce000,
		0x381d0000, 0x381d2000, 0x381d4000, 0x381d6000, 0x381d8000, 0x381da000, 0x381dc000, 0x381de000,
		0x381e0000, 0x381e2000, 0x381e4000, 0x381e6000, 0x381e8000, 0x381ea000, 0x381ec000, 0x381ee000,
		0x381f0000, 0x381f2000, 0x381f4000, 0x381f6000, 0x381f8000, 0x381fa000, 0x381fc000, 0x381fe000,
		0x38200000, 0x38202000, 0x38204000, 0x38206000, 0x38208000, 0x3820a000, 0x3820c000, 0x3820e000,
		0x38210000, 0x38212000, 0x38214000, 0x38216000, 0x38218000, 0x3821a000, 0x3821c000, 0x3821e000,
		0x38220000, 0x38222000, 0x38224000, 0x38226000, 0x38228000, 0x3822a000, 0x3822c000, 0x3822e000,
		0x38230000, 0x38232000, 0x38234000, 0x38236000, 0x38238000, 0x3823a000, 0x3823c000, 0x3823e000,
		0x38240000, 0x38242000, 0x38244000, 0x38246000, 0x38248000, 0x3824a000, 0x3824c000, 0x3824e000,
		0x38250000, 0x38252000, 0x38254000, 0x38256000, 0x38258000, 0x3825a000, 0x3825c000, 0x3825e000,
		0x38260000, 0x38262000, 0x38264000, 0x38266000, 0x38268000, 0x3826a000, 0x3826c000, 0x3826e000,
		0x38270000, 0x38272000, 0x38274000, 0x38276000, 0x38278000, 0x3827a000, 0x3827c000, 0x3827e000,
		0x38280000, 0x38282000, 0x38284000, 0x38286000, 0x38288000, 0x3828a000, 0x3828c000, 0x3828e000,
		0x38290000, 0x38292000, 0x38294000, 0x38296000, 0x38298000, 0x3829a000, 0x3829c000, 0x3829e000,
		0x382a0000, 0x382a2000, 0x382a4000, 0x382a6000, 0x382a8000, 0x382aa000, 0x382ac000, 0x382ae000,
		0x382b0000, 0x382b2000, 0x382b4000, 0x382b6000, 0x382b8000, 0x382ba000, 0x382bc000, 0x382be000,
		0x382c0000, 0x382c2000, 0x382c4000, 0x382c6000, 0x382c8000, 0x382ca000, 0x382cc000, 0x382ce000,
		0x382d0000, 0x382d2000, 0x382d4000, 0x382d6000, 0x382d8000, 0x382da000, 0x382dc000, 0x382de000,
		0x382e0000, 0x382e2000, 0x382e4000, 0x382e6000, 0x382e8000, 0x382ea000, 0x382ec000, 0x382ee000,
		0x382f0000, 0x382f2000, 0x382f4000, 0x382f6000, 0x382f8000, 0x382fa000, 0x382fc000, 0x382fe000,
		0x38300000, 0x38302000, 0x38304000, 0x38306000, 0x38308000, 0x3830a000, 0x3830c000, 0x3830e000,
		0x38310000, 0x38312000, 0x38314000, 0x38316000, 0x38318000, 0x3831a000, 0x3831c000, 0x3831e000,
		0x38320000, 0x38322000, 0x38324000, 0x38326000, 0x38328000, 0x3832a000, 0x3832c000, 0x3832e000,
		0x38330000, 0x38332000, 0x38334000, 0x38336000, 0x38338000, 0x3833a000, 0x3833c000, 0x3833e000,
		0x38340000, 0x38342000, 0x38344000, 0x38346000, 0x38348000, 0x3834a000, 0x3834c000, 0x3834e000,
		0x38350000, 0x38352000, 0x38354000, 0x38356000, 0x38358000, 0x3835a000, 0x3835c000, 0x3835e000,
		0x38360000, 0x38362000, 0x38364000, 0x38366000, 0x38368000, 0x3836a000, 0x3836c000, 0x3836e000,
		0x38370000, 0x38372000, 0x38374000, 0x38376000, 0x38378000, 0x3837a000, 0x3837c000, 0x3837e000,
		0x38380000, 0x38382000, 0x38384000, 0x38386000, 0x38388000, 0x3838a000, 0x3838c000, 0x3838e000,
		0x38390000, 0x38392000, 0x38394000, 0x38396000, 0x38398000, 0x3839a000, 0x3839c000, 0x3839e000,
		0x383a0000, 0x383a2000, 0x383a4000, 0x383a6000, 0x383a8000, 0x383aa000, 0x383ac000, 0x383ae000,
		0x383b0000, 0x383b2000, 0x383b4000, 0x383b6000, 0x383b8000, 0x383ba000, 0x383bc000, 0x383be000,
		0x383c0000, 0x383c2000, 0x383c4000, 0x383c6000, 0x383c8000, 0x383ca000, 0x383cc000, 0x383ce000,
		0x383d0000, 0x383d2000, 0x383d4000, 0x383d6000, 0x383d8000, 0x383da000, 0x383dc000, 0x383de000,
		0x383e0000, 0x383e2000, 0x383e4000, 0x383e6000, 0x383e8000, 0x383ea000, 0x383ec000, 0x383ee000,
		0x383f0000, 0x383f2000, 0x383f4000, 0x383f6000, 0x383f8000, 0x383fa000, 0x383fc000, 0x383fe000,
		0x38400000, 0x38402000, 0x38404000, 0x38406000, 0x38408000, 0x3840a000, 0x3840c000, 0x3840e000,
		0x38410000, 0x38412000, 0x38414000, 0x38416000, 0x38418000, 0x3841a000, 0x3841c000, 0x3841e000,
		0x38420000, 0x38422000, 0x38424000, 0x38426000, 0x38428000, 0x3842a000, 0x3842c000, 0x3842e000,
		0x38430000, 0x38432000, 0x38434000, 0x38436000, 0x38438000, 0x3843a000, 0x3843c000, 0x3843e000,
		0x38440000, 0x38442000, 0x38444000, 0x38446000, 0x38448000, 0x3844a000, 0x3844c000, 0x3844e000,
		0x38450000, 0x38452000, 0x38454000, 0x38456000, 0x38458000, 0x3845a000, 0x3845c000, 0x3845e000,
		0x38460000, 0x38462000, 0x38464000, 0x38466000, 0x38468000, 0x3846a000, 0x3846c000, 0x3846e000,
		0x38470000, 0x38472000, 0x38474000, 0x38476000, 0x38478000, 0x3847a000, 0x3847c000, 0x3847e000,
		0x38480000, 0x38482000, 0x38484000, 0x38486000, 0x38488000, 0x3848a000, 0x3848c000, 0x3848e000,
		0x38490000, 0x38492000, 0x38494000, 0x38496000, 0x38498000, 0x3849a000, 0x3849c000, 0x3849e000,
		0x384a0000, 0x384a2000, 0x384a4000, 0x384a6000, 0x384a8000, 0x384aa000, 0x384ac000, 0x384ae000,
		0x384b0000, 0x384b2000, 0x384b4000, 0x384b6000, 0x384b8000, 0x384ba000, 0x384bc000, 0x384be000,
		0x384c0000, 0x384c2000, 0x384c4000, 0x384c6000, 0x384c8000, 0x384ca000, 0x384cc000, 0x384ce000,
		0x384d0000, 0x384d2000, 0x384d4000, 0x384d6000, 0x384d8000, 0x384da000, 0x384dc000, 0x384de000,
		0x384e0000, 0x384e2000, 0x384e4000, 0x384e6000, 0x384e8000, 0x384ea000, 0x384ec000, 0x384ee000,
		0x384f0000, 0x384f2000, 0x384f4000, 0x384f6000, 0x384f8000, 0x384fa000, 0x384fc000, 0x384fe000,
		0x38500000, 0x38502000, 0x38504000, 0x38506000, 0x38508000, 0x3850a000, 0x3850c000, 0x3850e000,
		0x38510000, 0x38512000, 0x38514000, 0x38516000, 0x38518000, 0x3851a000, 0x3851c000, 0x3851e000,
		0x38520000, 0x38522000, 0x38524000, 0x38526000, 0x38528000, 0x3852a000, 0x3852c000, 0x3852e000,
		0x38530000, 0x38532000, 0x38534000, 0x38536000, 0x38538000, 0x3853a000, 0x3853c000, 0x3853e000,
		0x38540000, 0x38542000, 0x38544000, 0x38546000, 0x38548000, 0x3854a000, 0x3854c000, 0x3854e000,
		0x38550000, 0x38552000, 0x38554000, 0x38556000, 0x38558000, 0x3855a000, 0x3855c000, 0x3855e000,
		0x38560000, 0x38562000, 0x38564000, 0x38566000, 0x38568000, 0x3856a000, 0x3856c000, 0x3856e000,
		0x38570000, 0x38572000, 0x38574000, 0x38576000, 0x38578000, 0x3857a000, 0x3857c000, 0x3857e000,
		0x38580000, 0x38582000, 0x38584000, 0x38586000, 0x38588000, 0x3858a000, 0x3858c000, 0x3858e000,
		0x38590000, 0x38592000, 0x38594000, 0x38596000, 0x38598000, 0x3859a000, 0x3859c000, 0x3859e000,
		0x385a0000, 0x385a2000, 0x385a4000, 0x385a6000, 0x385a8000, 0x385aa000, 0x385ac000, 0x385ae000,
		0x385b0000, 0x385b2000, 0x385b4000, 0x385b6000, 0x385b8000, 0x385ba000, 0x385bc000, 0x385be000,
		0x385c0000, 0x385c2000, 0x385c4000, 0x385c6000, 0x385c8000, 0x385ca000, 0x385cc000, 0x385ce000,
		0x385d0000, 0x385d2000, 0x385d4000, 0x385d6000, 0x385d8000, 0x385da000, 0x385dc000, 0x385de000,
		0x385e0000, 0x385e2000, 0x385e4000, 0x385e6000, 0x385e8000, 0x385ea000, 0x385ec000, 0x385ee000,
		0x385f0000, 0x385f2000, 0x385f4000, 0x385f6000, 0x385f8000, 0x385fa000, 0x385fc000, 0x385fe000,
		0x38600000, 0x38602000, 0x38604000, 0x38606000, 0x38608000, 0x3860a000, 0x3860c000, 0x3860e000,
		0x38610000, 0x38612000, 0x38614000, 0x38616000, 0x38618000, 0x3861a000, 0x3861c000, 0x3861e000,
		0x38620000, 0x38622000, 0x38624000, 0x38626000, 0x38628000, 0x3862a000, 0x3862c000, 0x3862e000,
		0x38630000, 0x38632000, 0x38634000, 0x38636000, 0x38638000, 0x3863a000, 0x3863c000, 0x3863e000,
		0x38640000, 0x38642000, 0x38644000, 0x38646000, 0x38648000, 0x3864a000, 0x3864c000, 0x3864e000,
		0x38650000, 0x38652000, 0x38654000, 0x38656000, 0x38658000, 0x3865a000, 0x3865c000, 0x3865e000,
		0x38660000, 0x38662000, 0x38664000, 0x38666000, 0x38668000, 0x3866a000, 0x3866c000, 0x3866e000,
		0x38670000, 0x38672000, 0x38674000, 0x38676000, 0x38678000, 0x3867a000, 0x3867c000, 0x3867e000,
		0x38680000, 0x38682000, 0x38684000, 0x38686000, 0x38688000, 0x3868a000, 0x3868c000, 0x3868e000,
		0x38690000, 0x38692000, 0x38694000, 0x38696000, 0x38698000, 0x3869a000, 0x3869c000, 0x3869e000,
		0x386a0000, 0x386a2000, 0x386a4000, 0x386a6000, 0x386a8000, 0x386aa000, 0x386ac000, 0x386ae000,
		0x386b0000, 0x386b2000, 0x386b4000, 0x386b6000, 0x386b8000, 0x386ba000, 0x386bc000, 0x386be000,
		0x386c0000, 0x386c2000, 0x386c4000, 0x386c6000, 0x386c8000, 0x386ca000, 0x386cc000, 0x386ce000,
		0x386d0000, 0x386d2000, 0x386d4000, 0x386d6000, 0x386d8000, 0x386da000, 0x386dc000, 0x386de000,
		0x386e0000, 0x386e2000, 0x386e4000, 0x386e6000, 0x386e8000, 0x386ea000, 0x386ec000, 0x386ee000,
		0x386f0000, 0x386f2000, 0x386f4000, 0x386f6000, 0x386f8000, 0x386fa000, 0x386fc000, 0x386fe000,
		0x38700000, 0x38702000, 0x38704000, 0x38706000, 0x38708000, 0x3870a000, 0x3870c000, 0x3870e000,
		0x38710000, 0x38712000, 0x38714000, 0x38716000, 0x38718000, 0x3871a000, 0x3871c000, 0x3871e000,
		0x38720000, 0x38722000, 0x38724000, 0x38726000, 0x38728000, 0x3872a000, 0x3872c000, 0x3872e000,
		0x38730000, 0x38732000, 0x38734000, 0x38736000, 0x38738000, 0x3873a000, 0x3873c000, 0x3873e000,
		0x38740000, 0x38742000, 0x38744000, 0x38746000, 0x38748000, 0x3874a000, 0x3874c000, 0x3874e000,
		0x38750000, 0x38752000, 0x38754000, 0x38756000, 0x38758000, 0x3875a000, 0x3875c000, 0x3875e000,
		0x38760000, 0x38762000, 0x38764000, 0x38766000, 0x38768000, 0x3876a000, 0x3876c000, 0x3876e000,
		0x38770000, 0x38772000, 0x38774000, 0x38776000, 0x38778000, 0x3877a000, 0x3877c000, 0x3877e000,
		0x38780000, 0x38782000, 0x38784000, 0x38786000, 0x38788000, 0x3878a000, 0x3878c000, 0x3878e000,
		0x38790000, 0x38792000, 0x38794000, 0x38796000, 0x38798000, 0x3879a000, 0x3879c000, 0x3879e000,
		0x387a0000, 0x387a2000, 0x387a4000, 0x387a6000, 0x387a8000, 0x387aa000, 0x387ac000, 0x387ae000,
		0x387b0000, 0x387b2000, 0x387b4000, 0x387b6000, 0x387b8000, 0x387ba000, 0x387bc000, 0x387be000,
		0x387c0000, 0x387c2000, 0x387c4000, 0x387c6000, 0x387c8000, 0x387ca000, 0x387cc000, 0x387ce000,
		0x387d0000, 0x387d2000, 0x387d4000, 0x387d6000, 0x387d8000, 0x387da000, 0x387dc000, 0x387de000,
		0x387e0000, 0x387e2000, 0x387e4000, 0x387e6000, 0x387e8000, 0x387ea000, 0x387ec000, 0x387ee000,
		0x387f0000, 0x387f2000, 0x387f4000, 0x387f6000, 0x387f8000, 0x387fa000, 0x387fc000, 0x387fe000,
	};
	static unsigned int g_exponent_table[64] =
	{
		0x00000000, 0x00800000, 0x01000000, 0x01800000, 0x02000000, 0x02800000, 0x03000000, 0x03800000,
		0x04000000, 0x04800000, 0x05000000, 0x05800000, 0x06000000, 0x06800000, 0x07000000, 0x07800000,
		0x08000000, 0x08800000, 0x09000000, 0x09800000, 0x0a000000, 0x0a800000, 0x0b000000, 0x0b800000,
		0x0c000000, 0x0c800000, 0x0d000000, 0x0d800000, 0x0e000000, 0x0e800000, 0x0f000000, 0x47800000,
		0x80000000, 0x80800000, 0x81000000, 0x81800000, 0x82000000, 0x82800000, 0x83000000, 0x83800000,
		0x84000000, 0x84800000, 0x85000000, 0x85800000, 0x86000000, 0x86800000, 0x87000000, 0x87800000,
		0x88000000, 0x88800000, 0x89000000, 0x89800000, 0x8a000000, 0x8a800000, 0x8b000000, 0x8b800000,
		0x8c000000, 0x8c800000, 0x8d000000, 0x8d800000, 0x8e000000, 0x8e800000, 0x8f000000, 0xc7800000,
	};
	static unsigned short g_offset_table[64] =
	{
		0x0000, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400,
		0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400,
		0x0000, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400,
		0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400,
	};

	template<bool BypassCache>
	struct ConvertOpWithCacheBypass<HALF_FLOAT, float, BypassCache>
	{
	public:
		typedef HALF_FLOAT ImplSrcElType;
		typedef float ImplDstElType;
		typedef int ParamType;
		typedef int TmpType;

		static const int NumSrcBands = 0;
		static const int NumDstBands = 0;
		static const int NumSrcElPerGroup = 1;
		static const int NumDstElPerGroup = 1;
		static const int NumGroupsPerOpGeneric = 1;
		// static const int NumGroupsPerOpSSE2 = 8;
	    static const int NumGroupsPerOpAVXwF16C = 8;

public:
	void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
		ParamType*, TmpType&) 
	{
		UnionFloatDword* pOutFDW = (UnionFloatDword*)pDst;
        UInt16 h = *((UInt16 *) pSrc);

        UInt16 mant = h & 0x3ff;
        UInt16 expt = h >> 10;
        pOutFDW->d =
            g_mantissa_table[g_offset_table[expt] + mant] +
            g_exponent_table[expt];
	}

#if 0 //defined(VT_SSE_SUPPORTED)
	void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
		ParamType*, TmpType&) 
	{
		// do full 16 bytes (SSE2 data size) to prevent unnecessary calls to
		// generic impl
		EightBytesOfHalfFloatToFloatSSE2(pSrc, pDst);
		EightBytesOfHalfFloatToFloatSSE2(pSrc + 4, pDst + 4);
	}
#endif

#if defined(VT_AVX_SUPPORTED)
	void EvalAVXwF16C(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
		ParamType*, TmpType&) 
	{
		__m128i src128 = _mm_loadu_si128((const __m128i *) pSrc);

		StoreIntrinsicAVX<BypassCache>::StoreAligned(pDst, 
            _mm256_cvtph_ps(src128));
	}
#endif
};

////////////////////////////////////////////////////////////////////////////////
#if defined(VT_SSE_SUPPORTED)
inline void EightBytesOfFloatToHalfFloatSSE2(const float* pSrc, HALF_FLOAT* pDst)
{
	__m128i answer = _mm_loadu_si128((const __m128i*) pSrc);

	// clamp to valid float16 range
	const __m128 minans = _mm_min_ps(g_hfcconst.maxFloatVal, 
		*(__m128*) &answer);
	answer = *(__m128i*) &minans;
	const __m128 maxans = _mm_max_ps(g_hfcconst.minFloatVal, 
		*(__m128*) &answer);
	answer = *(__m128i*) &maxans;

	// grab the sign bits and pack them
	__m128i signBit = _mm_and_si128(g_hfcconst.hex80000000, answer);
	signBit = _mm_srai_epi32(signBit, 16);
	signBit = _mm_packs_epi32(signBit, signBit);

	// subtract 112 from exponent (changes bias from 127 to 15), 
	// round to nearest even, 
	// and truncate mantissa to 10 bits
#ifdef	HALF_FLOAT_ROUND
	__m128 answerF = *((__m128*) &answer);

	// round to even
	__m128 m128Overflower = _mm_mul_ps(answerF, g_hfcconst.m128Round);
	answerF = _mm_add_ps(answerF, m128Overflower);
	answerF = _mm_sub_ps(answerF, m128Overflower);

	// subtract 112 from exponent
	answerF = _mm_mul_ps(answerF, g_hfcconst.oneOverPowf2to112Splatted);
	answer = *((__m128i*) &answerF);
#else	// !HALF_FLOAT_ROUND
	// subtract 112 from exponent by multiplying by 2^-112
	__m128 answerF = _mm_mul_ps(*((__m128*) &answer), g_hfcconst.oneOverPowf2to112Splatted);
	answer = *((__m128i*) &answerF);

	// round to even (rounding slows code by ~30%)
	answer = _mm_add_epi32(answer, _mm_and_si128(_mm_srli_epi32(answer, 13), 
		g_hfcconst.m128LSB));
	answer = _mm_add_epi32(answer, g_hfcconst.m128Round);
#endif	// HALF_FLOAT_ROUND
	// truncate matissa LSBs
	answer = _mm_srli_epi32(answer, 13);

	// clear upper 16 bits
	answer = _mm_and_si128(g_hfcconst.hex00007FFF, answer);

	// pack the answer
	answer = _mm_packs_epi32(answer,answer);

	// set the sign bit [this is different from what we did in C version]
	answer = _mm_or_si128(answer, signBit);

	// write back lowest 64 bits
	_mm_storel_pd((double*) pDst, *(__m128d*) &answer);
}
#endif

// Convert half float to float.
// From "Fast Half Float Conversions" by Jeroen van der Zijp
// November 2008 (Revised September 2010)
// ftp://ftp.fox-toolkit.org/pub/fasthalffloatconversion.pdf
//
// Tables generated by f16c.cpp
//
static unsigned short g_base_table[512] =
{
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100, 
	0x0200, 0x0400, 0x0800, 0x0c00, 0x1000, 0x1400, 0x1800, 0x1c00, 0x2000, 0x2400, 0x2800, 0x2c00, 0x3000, 0x3400, 0x3800, 0x3c00, 
	0x4000, 0x4400, 0x4800, 0x4c00, 0x5000, 0x5400, 0x5800, 0x5c00, 0x6000, 0x6400, 0x6800, 0x6c00, 0x7000, 0x7400, 0x7800, 0x7c00, 
	0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 
	0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 
	0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 
	0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 
	0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 
	0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 
	0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 0x7c00, 
	0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 
	0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 
	0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 
	0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 
	0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 
	0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 
	0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8001, 0x8002, 0x8004, 0x8008, 0x8010, 0x8020, 0x8040, 0x8080, 0x8100, 
	0x8200, 0x8400, 0x8800, 0x8c00, 0x9000, 0x9400, 0x9800, 0x9c00, 0xa000, 0xa400, 0xa800, 0xac00, 0xb000, 0xb400, 0xb800, 0xbc00, 
	0xc000, 0xc400, 0xc800, 0xcc00, 0xd000, 0xd400, 0xd800, 0xdc00, 0xe000, 0xe400, 0xe800, 0xec00, 0xf000, 0xf400, 0xf800, 0xfc00, 
	0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 
	0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 
	0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 
	0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 
	0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 
	0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 
	0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 0xfc00, 
};
static unsigned char g_shift_table[512] =
{
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 13, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 13, 
};
static unsigned char g_round_table[512] =
{
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
};

template<bool BypassCache>
struct ConvertOpWithCacheBypass<float, HALF_FLOAT, BypassCache>
{
public:
	typedef float ImplSrcElType;
	typedef HALF_FLOAT ImplDstElType;
	typedef int ParamType;
	typedef int TmpType;	

	static const int NumSrcBands = 0;
	static const int NumDstBands = 0;
	static const int NumSrcElPerGroup = 1;
	static const int NumDstElPerGroup = 1;
	static const int NumGroupsPerOpGeneric = 1;	
	// static const int NumGroupsPerOpSSE2 = 8;
	static const int NumGroupsPerOpAVXwF16C = 16;

public:
	void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
		ParamType*, TmpType&) 
	{
		UnionFloatDword tempAnswer;

		tempAnswer.f = *pSrc;
        UInt32 f = tempAnswer.d;

        // truncate
        UInt32 mant = f & 0x007fffff;
        UInt32 expt = f >> 23;
        UInt16 h = g_base_table[expt] + (UInt16) (mant >> g_shift_table[expt]);

#ifdef HALF_FLOAT_ROUND
        mant |= 0x00800000;                 // add explicit lead bit
        UInt32 round = g_round_table[expt]; // 1 less than shift to preserve highest fractional bit
        h +=
            // round to nearest: increment if highest fractional bit set
            (mant >> round) & 1 &
            // round to even: increment if other fractional bits 0 and is odd
		    (((mant & ((1 << round) - 1)) != 0) | h);
#endif	
		//write out the answer
		*pDst = *((HALF_FLOAT *) &h);
	}

#if 0 //defined(VT_SSE_SUPPORTED)
	void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
		ParamType*, TmpType&) 
	{
		// do full 16 bytes (SSE2 data size) to prevent unnecessary calls to
		// generic impl
		EightBytesOfFloatToHalfFloatSSE2(pSrc, pDst);
		EightBytesOfFloatToHalfFloatSSE2(pSrc + 4, pDst + 4);
	}
#endif

#if defined(VT_AVX_SUPPORTED)
	void EvalAVXwF16C(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
		ParamType*, TmpType&) 
	{
		// load the values
		__m256 f0 = _mm256_loadu_ps(pSrc);
		__m256 f1 = _mm256_loadu_ps(pSrc+8);
		
		// convert 16x floats to half
        // 0 == Round to nearest even
        // 1 == Round down
        // 2 == Round up
        // 3 == Truncate
#ifdef HALF_FLOAT_ROUND
		__m128i x0 = _mm256_cvtps_ph(f0, 0);
		__m128i x1 = _mm256_cvtps_ph(f1, 0);
#else
		__m128i x0 = _mm256_cvtps_ph(f0, 3);
		__m128i x1 = _mm256_cvtps_ph(f1, 3);
#endif

		// pack results into an 256 bit register
		__m256 packed = _mm256_castps128_ps256(_mm_castsi128_ps(x0));
		StoreIntrinsicAVX<BypassCache>::StoreAligned(pDst,
			_mm256_insertf128_ps(packed, _mm_castsi128_ps(x1), 1));
	}
#endif
};

////////////////////////////////////////////////////////////////////////////////

// ConvertOp fallback implementation
template<typename SrcElT, typename DstElT>
class ConvertOp : public ConvertOpGenericBase<SrcElT, DstElT> {};
template<typename SrcElT, typename DstElT>
class ConvertOpBypassCache : public ConvertOpGenericBase<SrcElT, DstElT> {};

// Some conversions are faster via intermediate formats ..
#define DECLARE_SPECIALIZED_CONVERT_OP_VIA(TI, TO, TViaI, TViaO) \
	template<> class ConvertOp<TI, TO> : \
	public ConvertOpGenericBase<TI, TO, TViaI, TViaO> {};

DECLARE_SPECIALIZED_CONVERT_OP_VIA(HALF_FLOAT, Byte, float, float);
DECLARE_SPECIALIZED_CONVERT_OP_VIA(HALF_FLOAT, UInt16, float, float);
DECLARE_SPECIALIZED_CONVERT_OP_VIA(Byte, HALF_FLOAT, float, float);
DECLARE_SPECIALIZED_CONVERT_OP_VIA(UInt16, HALF_FLOAT, float, float);

// ConvertOp specialized and with bypass option
#define DECLARE_SPECIALIZED_CONVERT_OP_WITH_BYPASS(TI, TO) \
	template<> class ConvertOp<TI, TO> : \
	public ConvertOpWithCacheBypass<TI, TO, false> {}; \
	template<> class ConvertOpBypassCache<TI, TO> : \
	public ConvertOpWithCacheBypass<TI, TO, true> {}; 

DECLARE_SPECIALIZED_CONVERT_OP_WITH_BYPASS(Byte, float);
DECLARE_SPECIALIZED_CONVERT_OP_WITH_BYPASS(float, Byte);
DECLARE_SPECIALIZED_CONVERT_OP_WITH_BYPASS(UInt16, float);
DECLARE_SPECIALIZED_CONVERT_OP_WITH_BYPASS(float, UInt16);
DECLARE_SPECIALIZED_CONVERT_OP_WITH_BYPASS(HALF_FLOAT, float);
DECLARE_SPECIALIZED_CONVERT_OP_WITH_BYPASS(float, HALF_FLOAT);

////////////////////////////////////////////////////////////////////////////////

template<typename SrcElT, typename DstElT, typename ImplSrcElT = SrcElT, 
	typename ImplDstElT = DstElT, int NSrcBands = 3>
struct RGBToGrayOpGenericBase
{
public:
	typedef ImplSrcElT ImplSrcElType;
	typedef ImplDstElT ImplDstElType;
	typedef int ParamType;
	typedef int TmpType;

	static const int NumSrcBands = NSrcBands;
	static const int NumDstBands = 1;
	static const int NumSrcElPerGroup = NSrcBands;
	static const int NumDstElPerGroup = 1;
	static const int NumGroupsPerOpGeneric = 1;	

public:
	void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
		ParamType*, TmpType&) 
	{
		VtConv(pDst, VtLumaFromRGB_CCIR601YPbPr(
			reinterpret_cast<const RGBType<ImplSrcElType>*>(pSrc)));
	}
};

////////////////////////////////////////////////////////////////////////////////

template<typename SrcElT, typename DstElT>
struct RGBToGrayOp : 
	public RGBToGrayOpGenericBase<SrcElT, DstElT, float, float> {};

template<typename SrcElT, typename DstElT>
struct RGBAToGrayOp : 
	public RGBToGrayOpGenericBase<SrcElT, DstElT, float, float, 4> {};

////////////////////////////////////////////////////////////////////////////////

template<typename SrcElT, typename DstElT, typename ImplSrcElT = SrcElT, 
	typename ImplDstElT = DstElT>
struct GrayToRGBOpBase
{
public:
	typedef ImplSrcElT ImplSrcElType;
	typedef ImplDstElT ImplDstElType;
	typedef int ParamType;
	typedef int TmpType;

	static const int NumSrcBands = 1;
	static const int NumDstBands = 3;
	static const int NumSrcElPerGroup = 1;
	static const int NumDstElPerGroup = 3;
	static const int NumGroupsPerOpGeneric = 1;	

public:
	void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
		ParamType*, TmpType&) 
	{
		ImplDstElType v;
		VtConv(&v, *pSrc);
		
		RGBType<ImplDstElType>& dst = 
			reinterpret_cast<RGBType<ImplDstElType>&>(*pDst);
		dst.r = dst.g = dst.b = v;
	}
};

////////////////////////////////////////////////////////////////////////////////

// Generic impl
template<typename SrcElT, typename DstElT>
struct GrayToRGBOp : public GrayToRGBOpBase<SrcElT, DstElT> {};

// Efficient half float "detours"
template<typename DstElT>
struct GrayToRGBOp<HALF_FLOAT, DstElT> 
	: public GrayToRGBOpBase<HALF_FLOAT, DstElT, float, float> {};
template<typename SrcElT>
struct GrayToRGBOp<SrcElT, HALF_FLOAT> 
	: public GrayToRGBOpBase<SrcElT, HALF_FLOAT, float, float> {};
template<>
struct GrayToRGBOp<HALF_FLOAT, HALF_FLOAT> 
	: public GrayToRGBOpBase<HALF_FLOAT, HALF_FLOAT> {};

////////////////////////////////////////////////////////////////////////////////

template<typename SrcElT, typename DstElT, typename ImplSrcElT = SrcElT, 
	typename ImplDstElT = DstElT>
struct GrayToRGBAOpBase
{
public:
	typedef ImplSrcElT ImplSrcElType;
	typedef ImplDstElT ImplDstElType;
	typedef int ParamType;
	typedef ImplDstElT TmpType;

	static const int NumSrcBands = 1;
	static const int NumDstBands = 4;
	static const int NumSrcElPerGroup = 1;
	static const int NumDstElPerGroup = 4;
	static const int NumGroupsPerOpGeneric = 1;	

public:
	void InitGeneric(ParamType*, TmpType& tmp)
	{
		// use blob initialization because assignment of half_float MaxVal()
		// can cause expensive conversion otherwise
		static_assert(sizeof(TmpType) == sizeof(typename ElTraits<ImplDstElType>::BLOB_T), 
			"Maxval size mismatch");

		typename ElTraits<ImplDstElType>::BLOB_T blob = ElTraits<ImplDstElType>::MaxValBlob();
		tmp = *((TmpType*)&blob);
	}

	void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
		ParamType*, TmpType& tmp) 
	{
		ImplDstElType v;
		VtConv(&v, *pSrc);
		
		RGBAType<ImplDstElType>& dst = 
			reinterpret_cast<RGBAType<ImplDstElType>&>(*pDst);
		dst.r = dst.g = dst.b = v;
		dst.a = tmp;
	}
};

////////////////////////////////////////////////////////////////////////////////

// Generic impl
template<typename SrcElT, typename DstElT>
struct GrayToRGBAOp : public GrayToRGBAOpBase<SrcElT, DstElT> {};

// Efficient half float "detours"
template<typename SrcElT>
struct GrayToRGBAOp<SrcElT, HALF_FLOAT> 
	: public GrayToRGBAOpBase<SrcElT, HALF_FLOAT, SrcElT, float> {};
template<typename DstElT>
struct GrayToRGBAOp<HALF_FLOAT, DstElT> 
	: public GrayToRGBAOpBase<HALF_FLOAT, DstElT, float, DstElT> {};
template<>
struct GrayToRGBAOp<HALF_FLOAT, HALF_FLOAT> 
	: public GrayToRGBAOpBase<HALF_FLOAT, HALF_FLOAT> {};
	
////////////////////////////////////////////////////////////////////////////////

template<typename ImplDstElType>
struct RGBToRGBAOpTmpTypeBase
{
#if defined(VT_SSE_SUPPORTED)
	// prevent compiler alignment warning 
	__m128i _unused; 
#endif

	ImplDstElType maxVal;
};

template<typename SrcElT, typename DstElT, typename ImplSrcElT = SrcElT, 
	typename ImplDstElT = DstElT, 
	typename TmpT = RGBToRGBAOpTmpTypeBase<ImplDstElT> >
struct RGBToRGBAOpBase
{
public:
	typedef ImplSrcElT ImplSrcElType;
	typedef ImplDstElT ImplDstElType;
	typedef int ParamType;
	typedef TmpT TmpType;

	static const int NumSrcBands = 3;
	static const int NumDstBands = 4;
	static const int NumSrcElPerGroup = 3;
	static const int NumDstElPerGroup = 4;
	static const int NumGroupsPerOpGeneric = 1;	

public:
	void InitGeneric(ParamType*, TmpType& tmp)
	{
		// need to do this on the side because ElTraits::maxval is int,
		// and assigment to non-int ImplDstElType results in a conversion 
		// which can be costly, especially for HALF_FLOAT
		typename ElTraits<ImplDstElType>::BLOB_T blob = ElTraits<ImplDstElType>::MaxValBlob();
        memcpy(&tmp.maxVal, &blob, sizeof(tmp.maxVal));
	}

	void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
		ParamType*, TmpType& tmp) 
	{
		const RGBType<ImplSrcElType>& src = 
			reinterpret_cast<const RGBType<ImplSrcElType>&>(*pSrc);
		RGBAType<ImplDstElType>& dst = 
			reinterpret_cast<RGBAType<ImplDstElType>&>(*pDst);
		
		VtConv(&dst.r, src.r);
		VtConv(&dst.g, src.g);
		VtConv(&dst.b, src.b);
		dst.a = tmp.maxVal;
	}
};

////////////////////////////////////////////////////////////////////////////////
struct RGBToRGBAOpTmpTypeByteByte : public RGBToRGBAOpTmpTypeBase<Byte>
{
#if defined(VT_SSE_SUPPORTED)
	__m128i x6, x7;	
#endif
};

template<typename SrcElT, typename DstElT>
struct RGBToRGBAOpByteByte 
	: public RGBToRGBAOpBase<SrcElT, DstElT, Byte, Byte, RGBToRGBAOpTmpTypeByteByte>
{
public:
	typedef RGBToRGBAOpBase<SrcElT, DstElT, Byte, Byte, 
		RGBToRGBAOpTmpTypeByteByte> BT;

public:
	static const int NumGroupsPerOpSSE2 = 4;

public:
#if defined(VT_SSE_SUPPORTED)
	void InitSSE2(typename BT::ParamType*, typename BT::TmpType& tmp) 
	{		
		tmp.x6 = _mm_set1_epi32(0x00ffffff); // x6 = 0x00ffffff x 4
		tmp.x7 = _mm_set1_epi32(0xff000000); // x7 = 0xff000000 x 4
	}

	void EvalSSE2(const typename BT::ImplSrcElType* pSrc, 
		typename BT::ImplDstElType* pDst, 
		typename BT::ParamType*, typename BT::TmpType& tmp) 
	{	
		// load
		__m128i x0 = _mm_loadu_si128 ((__m128i*)pSrc);
		
		// copy, shift 2 upper 3-banded pixels
		__m128i x1 = x0;
		x1 = _mm_srli_si128(x1, 6);
		x0 = _mm_unpacklo_epi64(x0, x1);

		// align 3 banded pixels on 4 banded boundaries
		x1 = x0;
		x1 = _mm_slli_si128(x1, 1);
		x0 = _mm_shuffle_epi32(x0, (0<<0)|(2<<2));
		x1 = _mm_shuffle_epi32(x1, (1<<0)|(3<<2));
		x0 = _mm_unpacklo_epi32(x0, x1);

		// insert alpha
		x0 = _mm_and_si128 (x0, tmp.x6);
		x0 = _mm_or_si128  (x0, tmp.x7);

		// store
		_mm_store_si128((__m128i*)pDst, x0);
	}
#endif
};

////////////////////////////////////////////////////////////////////////////////
struct RGBToRGBAOpTmpTypeShortShort : public RGBToRGBAOpTmpTypeBase<UInt16>
{
#if defined(VT_SSE_SUPPORTED)
	__m128i x6, x7;	
#endif
};

template<typename SrcElT, typename DstElT>
struct RGBToRGBAOpShortShort 
	: public RGBToRGBAOpBase<SrcElT, DstElT, UInt16, UInt16,
	RGBToRGBAOpTmpTypeShortShort>
{
public:
	typedef RGBToRGBAOpBase<SrcElT, DstElT, UInt16, UInt16,
		RGBToRGBAOpTmpTypeShortShort> BT;

public:
	static const int NumGroupsPerOpSSE2 = 2;

public:
#if defined(VT_SSE_SUPPORTED)
	void InitSSE2(typename BT::ParamType*, typename BT::TmpType& tmp) 
	{	
#if defined(VT_AVX_SUPPORTED)
		tmp.x7.m128i_u64[0] = tmp.x7.m128i_u64[1] = UINT64(0xffff000000000000);
		tmp.x6.m128i_u64[0] = tmp.x6.m128i_u64[1] = UINT64(0x0000ffffffffffff);
#else
		tmp.x7 = _mm_setr_epi64(__m64(0xffff000000000000), __m64(0xffff000000000000));
		tmp.x6 = _mm_setr_epi64(__m64(0x0000ffffffffffff), __m64(0x0000ffffffffffff));
#endif
	}

	void EvalSSE2(const typename BT::ImplSrcElType* pSrc, 
		typename BT::ImplDstElType* pDst, typename BT::ParamType*, 
		typename BT::TmpType& tmp) 
	{	
		// load 2 x 3 band 16bit source pixels into lower 96 bits of SSE reg 
		__m128i x0 = _mm_loadu_si128 ((__m128i*)pSrc);

		// shift upper rgb pix into 4 band position
		__m128i x1 = x0;
		x1 = _mm_srli_si128(x1, 6);
		x0 = _mm_unpacklo_epi64(x0, x1);

		// insert alpha
		x0 = _mm_and_si128 (x0, tmp.x6);
		x0 = _mm_or_si128  (x0, tmp.x7);

		// store
		_mm_store_si128((__m128i*)pDst, x0);
	}
#endif
};

////////////////////////////////////////////////////////////////////////////////
struct RGBToRGBAOpTmpTypeHalfFloatHalfFloat : 
	public RGBToRGBAOpTmpTypeBase<HALF_FLOAT>
{
#if defined(VT_SSE_SUPPORTED)
	__m128i x6, x7;	
#endif
};

template<typename SrcElT, typename DstElT>
struct RGBToRGBAOpHalfFloatHalfFloat : 
	public RGBToRGBAOpBase<SrcElT, DstElT, HALF_FLOAT, HALF_FLOAT,
	RGBToRGBAOpTmpTypeHalfFloatHalfFloat>
{
public:
	typedef RGBToRGBAOpBase<SrcElT, DstElT, HALF_FLOAT, HALF_FLOAT,
		RGBToRGBAOpTmpTypeHalfFloatHalfFloat> BT;

public:
	static const int NumGroupsPerOpSSE2 = 2;

public:
#if defined(VT_SSE_SUPPORTED)
	void InitSSE2(typename BT::ParamType*, typename BT::TmpType& tmp) 
	{	
		HALF_FLOAT one;
        one.v = ElTraits<HALF_FLOAT>::MaxValBlob();
#if defined(VT_AVX_SUPPORTED)
		tmp.x7.m128i_u64[0] = tmp.x7.m128i_u64[1] = (UINT64)(*(UINT16*)(&one)) << 48;
		tmp.x6.m128i_u64[0] = tmp.x6.m128i_u64[1] = UINT64(0x0000ffffffffffff);
#else
		const uint64_t ones48 = (*(uint64_t*)(&one)) << 48;
		tmp.x7 = _mm_setr_epi64(__m64(ones48), __m64(ones48));
		tmp.x6 = _mm_setr_epi64(__m64(0x0000ffffffffffff), __m64(0x0000ffffffffffff));
#endif
	}

	void EvalSSE2(const typename BT::ImplSrcElType* pSrc, 
		typename BT::ImplDstElType* pDst, typename BT::ParamType*, 
		typename BT::TmpType& tmp) 
	{	
		// load 2 x 3 band 16bit source pixels into lower 96 bits of SSE reg 
		__m128i x0 = _mm_loadu_si128 ((__m128i*)pSrc);

		// shift upper rgb pix into 4 band position
		__m128i x1 = x0;
		x1 = _mm_srli_si128(x1, 6);
		x0 = _mm_unpacklo_epi64(x0, x1);

		// insert alpha
		x0 = _mm_and_si128 (x0, tmp.x6);
		x0 = _mm_or_si128  (x0, tmp.x7);

		// store
		_mm_store_si128((__m128i*)pDst, x0);
	}
#endif
};

////////////////////////////////////////////////////////////////////////////////
struct RGBToRGBAOpTmpTypeFloatFloat : public RGBToRGBAOpTmpTypeBase<float>
{
#if defined(VT_SSE_SUPPORTED)
	__m128 x7;	
#endif
};

template<typename SrcElT, typename DstElT>
struct RGBToRGBAOpFloatFloat : 
	public RGBToRGBAOpBase<SrcElT, DstElT, float, float,
	RGBToRGBAOpTmpTypeFloatFloat>
{
public:
	typedef RGBToRGBAOpBase<SrcElT, DstElT, float, float,
	RGBToRGBAOpTmpTypeFloatFloat> BT;

public:
	static const int NumGroupsPerOpSSE2 = 1;

public:
#if defined(VT_SSE_SUPPORTED)
	void InitSSE2(typename BT::ParamType*, typename BT::TmpType& tmp) 
	{		
		tmp.x7 = _mm_set1_ps(1.0f);
	}

	void EvalSSE2(const typename BT::ImplSrcElType* pSrc, 
		typename BT::ImplDstElType* pDst, typename BT::ParamType*, 
		typename BT::TmpType& tmp) 
	{	
		// load 1 x 3 band 32bit source pixels into lower 96 bits of SSE reg 
		__m128 x0 = _mm_loadu_ps(pSrc);

		// insert alpha
		__m128 x1 = _mm_unpackhi_ps(x0, tmp.x7);
		x0 = _mm_shuffle_ps(x0, x1, (0<<0)|(1<<2)|(0<<4)|(1<<6));

		// store
		_mm_storeu_ps(pDst, x0);
	}
#endif
};

////////////////////////////////////////////////////////////////////////////////

// Generic impl
template<typename SrcElT, typename DstElT>
struct RGBToRGBAOp : public RGBToRGBAOpBase<SrcElT, DstElT> {};

// Efficient "detours"
// HF output
template<typename SrcElT> 
struct RGBToRGBAOp<SrcElT, HALF_FLOAT> 
	: public RGBToRGBAOpBase<SrcElT, HALF_FLOAT, SrcElT, float> {};
// HF input
template<>
struct RGBToRGBAOp<HALF_FLOAT, Byte> 
	: public RGBToRGBAOpBase<HALF_FLOAT, Byte, float, float> {};
template<>
struct RGBToRGBAOp<HALF_FLOAT, UInt16> 
	: public RGBToRGBAOpBase<HALF_FLOAT, UInt16, float, float> {};
template<>
struct RGBToRGBAOp<HALF_FLOAT, float> 
	: public RGBToRGBAOpBase<HALF_FLOAT, float, float, float> {};
// float input
template<>
struct RGBToRGBAOp<float, Byte> 
	: public RGBToRGBAOpBase<float, Byte, float, float> {};
template<>
struct RGBToRGBAOp<float, UInt16> 
	: public RGBToRGBAOpBase<float, UInt16, float, float> {};

// Optimized implementations
template<>
struct RGBToRGBAOp<Byte, Byte> : public RGBToRGBAOpByteByte<Byte, Byte> {};
template<>
struct RGBToRGBAOp<UInt16, UInt16> 
	: public RGBToRGBAOpShortShort<UInt16, UInt16> {};
template<>
struct RGBToRGBAOp<HALF_FLOAT, HALF_FLOAT> 
	: public RGBToRGBAOpHalfFloatHalfFloat<HALF_FLOAT, HALF_FLOAT> {};
template<>
struct RGBToRGBAOp<float, float> 
	: public RGBToRGBAOpFloatFloat<float, float> {};

////////////////////////////////////////////////////////////////////////////////

template<typename SrcElT, typename DstElT, typename ImplSrcElT = SrcElT, 
	typename ImplDstElT = DstElT>
struct RGBAToRGBOpBase
{
public:
	typedef ImplSrcElT ImplSrcElType;
	typedef ImplDstElT ImplDstElType;
	typedef int ParamType;
	typedef int TmpType;

	static const int NumSrcBands = 4;
	static const int NumDstBands = 3;
	static const int NumSrcElPerGroup = 4;
	static const int NumDstElPerGroup = 3;
	static const int NumGroupsPerOpGeneric = 1;	

public:
	void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
		ParamType*, TmpType&) 
	{
		const RGBAType<ImplSrcElType>& src = 
			reinterpret_cast<const RGBAType<ImplSrcElType>&>(*pSrc);
		RGBType<ImplDstElType>& dst = 
			reinterpret_cast<RGBType<ImplDstElType>&>(*pDst);
		
		VtConv(&dst.r, src.r);
		VtConv(&dst.g, src.g);
		VtConv(&dst.b, src.b);
	}
};

////////////////////////////////////////////////////////////////////////////////

// Generic impl
template<typename SrcElT, typename DstElT>
struct RGBAToRGBOp : public RGBAToRGBOpBase<SrcElT, DstElT> {};

// Efficient "detours"
// HF output
template<typename SrcElT> 
struct RGBAToRGBOp<SrcElT, HALF_FLOAT> 
	: public RGBAToRGBOpBase<SrcElT, HALF_FLOAT, SrcElT, float> {};
// HF input
template<>
struct RGBAToRGBOp<HALF_FLOAT, Byte> 
	: public RGBAToRGBOpBase<HALF_FLOAT, Byte, float, float> {};
template<>
struct RGBAToRGBOp<HALF_FLOAT, UInt16> 
	: public RGBAToRGBOpBase<HALF_FLOAT, UInt16, float, float> {};
template<>
struct RGBAToRGBOp<HALF_FLOAT, float> 
	: public RGBAToRGBOpBase<HALF_FLOAT, float, float, float> {};
// HF input and output
template<>
struct RGBAToRGBOp<HALF_FLOAT, HALF_FLOAT> 
	: public RGBAToRGBOpBase<HALF_FLOAT, HALF_FLOAT> {};
// float input
template<>
struct RGBAToRGBOp<float, Byte> 
	: public RGBAToRGBOpBase<float, Byte, float, float> {};
template<>
struct RGBAToRGBOp<float, UInt16> 
	: public RGBAToRGBOpBase<float, UInt16, float, float> {};


////////////////////////////////////////////////////////////////////////////////

template<typename TO, typename TI>
void VtConvertSpan(TO* pOut, const TI* pIn, int numEl, bool bBypassCache)
{
	if (bBypassCache)
	{
		UnarySpanOp(pIn, 1, pOut, 1, numEl, ConvertOpBypassCache<TI,TO>());
	}
	else
	{
		UnarySpanOp(pIn, 1, pOut, 1, numEl, ConvertOp<TI,TO>());
	}
}

////////////////////////////////////////////////////////////////////////////////

template<typename TIO>
void VtConvertSpan(TIO* pOut, const TIO* pIn, int numEl, bool bBypassCache)
{
	if (bBypassCache)
	{
		VtMemcpy(pOut, pIn, numEl * sizeof(TIO), true);
	}
	else
	{
		memcpy(pOut, pIn, numEl * sizeof(TIO));
	}
}

////////////////////////////////////////////////////////////////////////////////

template <class TO, class TI>
void VtConvertSpanRGBToGray(TO* pOut, const RGBType<TI>* pIn, int pixcnt)
{
	UnarySpanOp(reinterpret_cast<const TI*>(pIn), 3, 
		reinterpret_cast<TO*>(pOut), 1, pixcnt, 
		RGBToGrayOp<TI,TO>());
}

template <class TO, class TI>
void VtConvertSpanRGBAToGray(TO* pOut, const RGBAType<TI>* pIn, int pixcnt)
{
	UnarySpanOp(reinterpret_cast<const TI*>(pIn), 4, 
		reinterpret_cast<TO*>(pOut), 1, pixcnt, 
		RGBAToGrayOp<TI,TO>());
}

template <class TO, class TI>
void VtConvertSpanGrayToRGB(RGBType<TO>* pOut, const TI* pIn, int pixcnt)
{
	UnarySpanOp(reinterpret_cast<const TI*>(pIn), 1, 
		reinterpret_cast<TO*>(pOut), 3, pixcnt, 
		GrayToRGBAOp<TI,TO>());
}

template <class TO, class TI>
void VtConvertSpanGrayToRGBA(RGBAType<TO>* pOut, const TI* pIn, int pixcnt)
{
	UnarySpanOp(reinterpret_cast<const TI*>(pIn), 1, 
		reinterpret_cast<TO*>(pOut), 4, pixcnt, 
		GrayToRGBAOp<TI,TO>());
}

template <class TO, class TI>
void VtConvertSpanRGBToRGBA(RGBAType<TO>* pOut, const RGBType<TI>* pIn, 
	int pixcnt)
{
	UnarySpanOp(reinterpret_cast<const TI*>(pIn), 3, 
		reinterpret_cast<TO*>(pOut), 4, pixcnt, 
		RGBToRGBAOp<TI,TO>());
}

template <class TO, class TI>
void VtConvertSpanRGBAToRGB(RGBType<TO>* pOut, const RGBAType<TI>* pIn, 
	int pixcnt)
{
	UnarySpanOp(reinterpret_cast<const TI*>(pIn), 4, 
		reinterpret_cast<TO*>(pOut), 3, pixcnt, 
		RGBAToRGBOp<TI,TO>());
}

//+-----------------------------------------------------------------------------
//
// VtConvertSpanBands
//
//------------------------------------------------------------------------------
template <class TO, class TI>
HRESULT VtConvertSpanBands(TO* pDst, int iDstBands, const TI* pSrc, 
						   int iSrcBands, int iSrcElCount, bool bBypassCache)
{
    VT_ASSERT((iSrcElCount%iSrcBands) == 0);

	const int numPixels = iSrcElCount/iSrcBands;

#define DO_CONV(op) UnarySpanOp(pSrc, iSrcBands, pDst, iDstBands, \
	numPixels, op<TI, TO>())

	if (iSrcBands == iDstBands)
		VtConvertSpan(pDst, pSrc, numPixels*iSrcBands, bBypassCache);
	else if (iSrcBands == 1 && iDstBands == 3)
		DO_CONV(GrayToRGBOp);
	else if (iSrcBands == 1 && iDstBands == 4)
		DO_CONV(GrayToRGBAOp);
	else if (iSrcBands == 3 && iDstBands == 1)
		DO_CONV(RGBToGrayOp);
	else if (iSrcBands == 3 && iDstBands == 4)
		DO_CONV(RGBToRGBAOp);
	else if (iSrcBands == 4 && iDstBands == 1)
		DO_CONV(RGBAToGrayOp);
	else if (iSrcBands == 4 && iDstBands == 3)
		DO_CONV(RGBAToRGBOp);
	else
		return E_NOTIMPL;
	
#undef DO_CONV

	return S_OK;
}

//+-----------------------------------------------------------------------------
//
// VtConvertImageMBandTo3Band
//
//------------------------------------------------------------------------------
template <class TO, class TI>
HRESULT VtConvertImageMBandTo3Band(const CTypedImg<TI> &cSrc, 
                                     CTypedImg<TO> &cDst)
{
    VT_HR_BEGIN();

	VT_HR_EXIT(cSrc.Bands() >= 3 && cDst.Bands() == 3 ? S_OK : E_INVALIDARG);

	VT_HR_EXIT(cSrc.IsValid() ? S_OK : E_INVALIDARG);

	VT_HR_EXIT(cSrc.IsSharingMemory(cDst) ? E_INVALIDARG : S_OK);		

	VT_HR_EXIT(InitDst(cDst, cSrc.Width(), cSrc.Height(), cSrc.GetType(), 3));

    int iW = cSrc.Width() * cSrc.Bands();
    int iH = cSrc.Height();

    for(int iY=0; iY<iH; iY++)
    {
        VtConvertSpanMBandTo3Band(cDst.Ptr(iY), cSrc.Ptr(iY), iW, 
			cSrc.Bands() - cDst.Bands());
    }

	VT_HR_END();
}

template <class TO, class TI>
TO* VtConvertSpanMBandTo3Band(TO* pOut, const TI* pIn, int inspan, 
	int skippedBands)
{
    // not yet implemented
#if 0
    if( g_SupportSSE2() )
    {
        VtConvertSpanSSEMBandToMBand(pOut, pIn, inspan);
    }
    else
#endif
    {    
        TO *pO = pOut;
        const TI *pEnd = pIn + inspan;
        while (pIn < pEnd)
        {
            VtConv(pO++, *pIn++);
            VtConv(pO++, *pIn++);
            VtConv(pO++, *pIn++);
            pIn += skippedBands;  // drop extra bands
        }
    }
    return pOut;
}

//+-----------------------------------------------------------------------------
//
// Float to RGBE representation
//
//------------------------------------------------------------------------------
template <class TI>
void VtConvertSpanFloatToRGBE(Byte* pOut, TI* pRGB, int pixcnt)
{
	for (int x = 0; x < pixcnt; x++, pOut+=4, pRGB++)
	{
		// We clamp source RGB values at zero (don't allow negative numbers)
		const float fRed   = VtMax(pRGB->r, 0.0f);
		const float fGreen = VtMax(pRGB->g, 0.0f);
		const float fBlue  = VtMax(pRGB->b, 0.0f);

		float fMaxPos = VtMax(VtMax(fGreen, fRed),fBlue);

		if (fMaxPos < 1e-32)
		{
			*(UInt32*)pOut = 0;
		}
		else
		{
			int e;
			const float fScale = (float)frexp(fMaxPos, &e) * 256.f / fMaxPos;

			// rounding SHOULD NOT be added - it has the potential to roll 
			// over to zero (and yes, 256 is the correct multiplier above)
			pOut[0] = (Byte)(fRed   * fScale);	 // R
			pOut[1] = (Byte)(fGreen * fScale);	 // G
			pOut[2] = (Byte)(fBlue  * fScale);	 // B
			pOut[3] = (Byte)(e + 128);			 // E
		}
	}
}

template <class TO, class TI> 
TO * VtConvertSpanARGBTo1BandSSE(TO *pOut, const TI *pIn, int inspan, int band)
{
    TO *pOutTmp = pOut;

	for(int i=0 ; i<inspan; i+=4, pIn+=4, pOut++)
	{
		VtConv(pOut, pIn[band]);
	}

    return pOutTmp;
}

template <>
float* VtConvertSpanARGBTo1BandSSE(float* pOut, const float* pIn, int inspan, int band);

template <>
float* VtConvertSpanARGBTo1BandSSE(float* pOut, const Byte* pIn, 
								   int inspan, int band);
template <>
float* VtConvertSpanARGBTo1BandSSE(float* pOut, const UInt16* pIn,
								   int inspan, int band);

template <class TO, class TI>
TO* VtConvertSpanARGBTo1Band(TO* pOut, const TI* pIn, int inspan, int band)
{
    TO *pOutTmp = pOut;
	if(g_SupportSSE2())
    {
		VtConvertSpanARGBTo1BandSSE(pOut, pIn, inspan, band);
    }
	else
	{
		int i;
		for(i=0; i<inspan-15; i+=16, pIn+=16, pOut+=4)
		{
			VtConv(pOut, pIn[band]);
			VtConv(pOut+1, pIn[band+4]);
			VtConv(pOut+2, pIn[band+8]);
			VtConv(pOut+3, pIn[band+12]);
		}
		for( ; i<inspan; i+=4, pIn+=4, pOut++)
		{
			VtConv(pOut, pIn[band]);
		}
	}

    return pOutTmp;
}

#pragma warning(push)
#pragma warning(disable:26014) // OACR incorrectly reporting problem with writing pD[iB]
template <class TO, class TI>
void VtConvertBandsSpan(_Out_writes_(numPixels*numDstBands) TO* pD, int numDstBands, const TI* pS, 
	int numSrcBands, int numPixels, const BandIndexType* peBandSet, 
	const TO* pFillVals)
{
	if (numSrcBands == 4 && numDstBands == 1 && peBandSet[0] >= BandIndex0)
	{
		VtConvertSpanARGBTo1Band(pD, pS, numPixels*4, peBandSet[0]);
		return;
	}

	for (int iX = 0; iX < numPixels; ++iX, pD += numDstBands, pS += numSrcBands)
	{
		for (int iB = 0; iB < numDstBands; ++iB)
		{
			if (peBandSet[iB] >= BandIndex0)
			{
				VtConv(pD + iB, pS[peBandSet[iB]]);
			}
			else if (peBandSet[iB] == BandIndexFill)
			{
				if (pFillVals != NULL)
					pD[iB] = pFillVals[iB];
				else
					VtMemset(pD+iB, 0, sizeof(*pD), 1);
			}
		}
	}
}
#pragma warning(pop)

// swap RGB to RGB
template <class T>
void VtRGBColorSwapSpan(RGBType<T>* pDst, const RGBType<T>* pSrc, int span)
{
	// TODO: mattu - add an SSE version of color swap
    int i = 0; //VtRGBColorSwapSpanSSE(pDst, pSrc, span);;

    pSrc+=i;
    pDst+=i;
    for(; i<span; i++, pSrc++, pDst++)
    {
        RGBType<T> src = *pSrc;
        pDst->r = src.b;
        pDst->g = src.g;
        pDst->b = src.r;
    }
}

// swap ARGB to ARGB
template <class T>
void VtRGBColorSwapSpan(RGBAType<T>* pDst, const RGBAType<T>* pSrc, int pixcnt)
{
    int i = VtRGBColorSwapSpanSSE(pDst, pSrc, pixcnt);;

    pSrc+=i;
    pDst+=i;
    for(; i<pixcnt; i++, pSrc++, pDst++)
    {
        RGBAType<T> src = *pSrc;
        pDst->r = src.b;
        pDst->g = src.g;
        pDst->b = src.r;
        pDst->a = src.a;
    }
}

//+-----------------------------------------------------------------------------
//
// internal SSE2 routines
//
//------------------------------------------------------------------------------

template <class T>
int VtRGBColorSwapSpanSSE(RGBAType<T>* pDst, const RGBAType<T>* pSrc,
						  int span)
{
    // default indicates that do pixels have been converted
    return 0;
}

template <>
int VtRGBColorSwapSpanSSE(RGBAType<float>* pDst, const RGBAType<float>* pSrc,
						  int pixcnt);

template <>
int VtRGBColorSwapSpanSSE(RGBAType<UInt16>* pDst, const RGBAType<UInt16>* pSrc,
                          int pixcnt);

template <>
inline int VtRGBColorSwapSpanSSE(RGBAType<HALF_FLOAT>* pDst, const RGBAType<HALF_FLOAT>* pSrc,
                                 int pixcnt);

template <>
int VtRGBColorSwapSpanSSE(RGBAType<Byte>* pDst, const RGBAType<Byte>* pSrc,
						  int pixcnt);

template <class TO>
TO* VtConvertSpanHSBToRGB32(TO* pD, const float* pS, int pixcnt)
{
    TO* pDOrig = pD;

    int x;
    for( x = 0; x < pixcnt; x++, pD++, pS+=4 )
    {
        VtConv(&pD->a, pS[3]); // copy alpha

        float fSat = pS[1];
        float fB   = pS[2];
        if( fSat == 0 )
        {
            VtConv(&pD->r, fB);
            pD->g = pD->b = pD->r;
            continue;
        }

        float fHue  = pS[0] / 60.f;
        float fI    = floorf(fHue); 
        float fR    = fHue - fI;

        float fBS = fB*fSat;
        #define CP()  (fB-fBS)
        #define CQ()  (fB-fBS*fR)
        #define CT()  (fB-fBS*(1.f-fR))

        switch( F2I(fI) )
        {
        case 0:
            VtConv(&pD->r, fB);
            VtConv(&pD->g, CT());
            VtConv(&pD->b, CP());
            break;
        case 1:
            VtConv(&pD->r, CQ());
            VtConv(&pD->g, fB);
            VtConv(&pD->b, CP());
            break;
        case 2:
            VtConv(&pD->r, CP());
            VtConv(&pD->g, fB);
            VtConv(&pD->b, CT());
            break;
        case 3:
            VtConv(&pD->r, CP());
            VtConv(&pD->g, CQ());
            VtConv(&pD->b, fB);
            break;
        case 4:
            VtConv(&pD->r, CT());
            VtConv(&pD->g, CP());
            VtConv(&pD->b, fB);
            break;
        default:
            VtConv(&pD->r, fB);
            VtConv(&pD->g, CP());
            VtConv(&pD->b, CQ());
            break;
        }

        #undef CP
        #undef CQ
        #undef CT
    }

    return pDOrig;
}
};