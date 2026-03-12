//+----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Neon implementations of SSE intrinsics to enable shared source code
//      for SSE and Neon code
//
//-----------------------------------------------------------------------------
#pragma once

#if !defined(_VT_SIMD_INCLUDED)

#if defined(_M_IX86) || defined(_M_AMD64)
#define AI_PLATFORM_HAS_SSE
#elif defined(_M_ARM) || defined(_M_ARM64)
#define AI_PLATFORM_HAS_NEON
#endif

#if defined(_M_IX86) || defined(_M_AMD64) || defined(_M_ARM) || defined(_M_ARM64)
#define _SIMD_SUPPORTED
#endif
#if defined(_M_IX86) || defined(_M_AMD64)
#define _SSE_SUPPORTED
#endif
#if defined(_M_ARM) || defined(_M_ARM64)
#define _NEON_SUPPORTED
#endif

#if !defined(AI_PLATFORM_HAS_SSE)
union __m128i 
{
	uint64_t m128i_u64[2];
	uint32_t m128i_u32[4];
	uint16_t m128i_u16[8];
	uint8_t m128i_u8[16];
	int64_t m128i_i64[2];
	int32_t m128i_i32[4];
	int16_t m128i_i16[8];
	int8_t m128i_i8[16];

#ifdef AI_PLATFORM_HAS_NEON
	__m128i()
	{
	}

	__m128i(__n128 init) 
		: n128(init) 
	{
	}

	operator __n128() const
	{
		return n128;
	}

	__n128 n128;
#endif
};

typedef union __declspec(intrin_type) _CRT_ALIGN(8) __m128 {
     float               m128_f32[4];
     unsigned __int64    m128_u64[2];
     __int8              m128_i8[16];
     __int16             m128_i16[8];
     __int32             m128_i32[4];
     __int64             m128_i64[2];
     unsigned __int8     m128_u8[16];
     unsigned __int16    m128_u16[8];
     unsigned __int32    m128_u32[4];
#ifdef AI_PLATFORM_HAS_NEON
	__m128()
	{
	}

	__m128(__n128 init) 
		: n128(init) 
	{
	}

	operator __n128() const
	{
		return n128;
	}

	__n128 n128;
#endif
 } __m128;

#endif

#if !defined(AI_PLATFORM_HAS_NEON)
union __n64
{
	uint64_t n64_u64[1];
	uint32_t n64_u32[2];
	uint16_t n64_u16[4];
	uint8_t n64_u8[8];
	int64_t n64_i64[1];
	int32_t n64_i32[2];
	int16_t n64_i16[4];
	int8_t n64_i8[8];
	float n64_f32[2];
};

union __n128
{
	uint64_t n128_u64[2];
	uint32_t n128_u32[4];
	uint16_t n128_u16[8];
	uint8_t n128_u8[16];
	int64_t n128_i64[2];
	int32_t n128_i32[4];
	int16_t n128_i16[8];
	int8_t n128_i8[16];
	float n128_f32[4];	

#ifdef AI_PLATFORM_HAS_SSE
	__n128()
	{
	}

	__n128(__m128i init)
		: m128i(init)
	{
	}

	operator __m128i() const
	{
		return m128i;
	}

	__m128i m128i;
#endif
};

typedef float float32_t;
typedef __n64 float32x2_t;
typedef __n128 float32x4_t;

typedef __n64 int8x8_t;
typedef __n64 int16x4_t;
typedef __n64 int32x2_t;
typedef __n64 int64x1_t;
typedef __n64 uint8x8_t;
typedef __n64 uint16x4_t;
typedef __n64 uint32x2_t;
typedef __n64 uint64x1_t;
typedef __n128 int8x16_t;
typedef __n128 int16x8_t;
typedef __n128 int32x4_t;
typedef __n128 int64x2_t;
typedef __n128 uint8x16_t;
typedef __n128 uint16x8_t;
typedef __n128 uint32x4_t;
typedef __n128 uint64x2_t;

#endif

//+----------------------------------------------------------------------------
//
// SSE instruction syntax implemented with Neon intrinsics
//
//-----------------------------------------------------------------------------
#if defined(_M_ARM) || defined(_M_ARM64)

VT_FORCEINLINE __m128i _mm_set1_epi8(char val) { return vdupq_n_s8(val); }
VT_FORCEINLINE __m128i _mm_set1_epi16(short val) { return vdupq_n_s16(val); }
VT_FORCEINLINE __m128i _mm_set1_epi32(int val) { return vdupq_n_s32(val); }
VT_FORCEINLINE __m128i _mm_load_si128(const __m128i* _p) { return vld1q_u8_ex((uint8_t*)_p,128); }
VT_FORCEINLINE __m128i _mm_loadu_si128(const __m128i* _p) { return vld1q_u8((uint8_t*)_p); }
VT_FORCEINLINE __m128 _mm_load_ps(const float* _p) { return vld1q_f32_ex(_p,128); }
VT_FORCEINLINE __m128 _mm_loadu_ps(const float* _p) { return vld1q_f32(_p); }
VT_FORCEINLINE __m128 _mm_load1_ps(const float* _p) { return vld1q_dup_f32(_p); }

VT_FORCEINLINE void _mm_storeu_ps(float* pOut, const __m128& _A) { vst1q_f32(pOut,_A); }
VT_FORCEINLINE void _mm_store_ps(float* pOut, const __m128& _A) { vst1q_f32_ex(pOut,_A,128); }
VT_FORCEINLINE void _mm_store_si128(__m128i* pOut, const __m128i& _A) { vst1q_u8_ex((uint8_t*)pOut,_A,128); }
VT_FORCEINLINE void _mm_storeu_si128(__m128i* pOut, const __m128i& _A) { vst1q_u8((uint8_t*)pOut,_A); }
VT_FORCEINLINE void _mm_storel_epi64(__m128i* pOut, const __m128i& _A) { vst1_u8((uint8_t*)pOut,_A.n128.s.low64); }

VT_FORCEINLINE __m128i _mm_setzero_si128() 
{
    const uint32_t zero = 0x0;
    return vld1q_dup_u32(&zero);
}
VT_FORCEINLINE __m128 _mm_setzero_ps() 
{
    const float zero = 0.f;
    return vld1q_dup_f32(&zero);
}
VT_FORCEINLINE __m128 _mm_set1_ps(float32_t val) { return vdupq_n_f32(val); }
VT_FORCEINLINE __m128 _mm_set_ps1(float32_t val) { return vdupq_n_f32(val); }
VT_FORCEINLINE __m128 _mm_setr_ps(float32_t _A, float32_t _B, float32_t _C, float32_t _D)
{ 
    float32x4_t __tmpreg;
    __tmpreg = vdupq_n_f32(_A);
    __tmpreg = vsetq_lane_f32(_B,__tmpreg,1);
    __tmpreg = vsetq_lane_f32(_C,__tmpreg,2);
    __tmpreg = vsetq_lane_f32(_D,__tmpreg,3);
    return __tmpreg;
}
VT_FORCEINLINE __m128 _mm_set_ps(float32_t _D, float32_t _C, float32_t _B, float32_t _A)
{ 
    float32x4_t __tmpreg;
    __tmpreg = vdupq_n_f32(_A);
    __tmpreg = vsetq_lane_f32(_B,__tmpreg,1);
    __tmpreg = vsetq_lane_f32(_C,__tmpreg,2);
    __tmpreg = vsetq_lane_f32(_D,__tmpreg,3);
    return __tmpreg;
}
VT_FORCEINLINE __m128 _mm_setr_epi32(int32_t _A, int32_t _B, int32_t _C, int32_t _D)
{
    uint32x4_t __tmpreg;
    __tmpreg = vdupq_n_s32(_A);
    __tmpreg = vsetq_lane_s32(_B,__tmpreg,1);
    __tmpreg = vsetq_lane_s32(_C,__tmpreg,2);
    __tmpreg = vsetq_lane_s32(_D,__tmpreg,3);
    return __tmpreg;
}

VT_FORCEINLINE __m128 _mm_mul_ps(const __m128& _A, const __m128& _B) { return vmulq_f32(_A,_B); }
VT_FORCEINLINE __m128 _mm_add_ps(const __m128& _A, const __m128& _B) { return vaddq_f32(_A,_B); }
VT_FORCEINLINE __m128 _mm_sub_ps(const __m128& _A, const __m128& _B) { return vsubq_f32(_A,_B); }
VT_FORCEINLINE __m128 _mm_div_ps(const __m128& _A, const __m128& _B)
{
    // get an initial estimate of 1/_B
    float32x4_t recip = vrecpeq_f32(_B);
    // do two Newton-Raphson steps
    recip = vmulq_f32(vrecpsq_f32(_B, recip), recip);
    recip = vmulq_f32(vrecpsq_f32(_B, recip), recip);
    // apply reciprocal
    return vmulq_f32(_A,recip);
}
VT_FORCEINLINE __m128 _mm_max_ps(const __m128& _A, const __m128& _B) { return vmaxq_f32(_A,_B); }
VT_FORCEINLINE __m128 _mm_min_ps(const __m128& _A, const __m128& _B) { return vminq_f32(_A,_B); }

VT_FORCEINLINE __m128i _mm_packs_epi32(const __m128i& _A, const __m128i& _B) { return vcombine_u16(vqmovn_u32(_A), vqmovn_u32(_B)); }
VT_FORCEINLINE __m128i _mm_packus_epi16(const __m128i& _A, const __m128i& _B) { return vcombine_u8(vqmovun_s16(_A), vqmovun_s16(_B)); }

VT_FORCEINLINE __m128 _mm_andnot_ps(const __m128& _A, const __m128& _B) { return vbicq_u32(_B, _A); }
VT_FORCEINLINE __m128 _mm_and_ps(const __m128& _A, const __m128& _B) { return vandq_u32(_B, _A); }
VT_FORCEINLINE __m128 _mm_or_ps(const __m128& _A, const __m128& _B) { return vorrq_u32(_A, _B); }
VT_FORCEINLINE __m128 _mm_xor_ps(const __m128& _A, const __m128& _B) { return veorq_u32(_A, _B); }

VT_FORCEINLINE __m128i _mm_castps_si128(const __m128& _expr) { return *(__m128i*)(&_expr); }
VT_FORCEINLINE __m128  _mm_castsi128_ps(const __m128i& _expr) { return *(__m128*)(&_expr); }
VT_FORCEINLINE __m128i _mm_cvtps_epi32(const __m128& _A) 
{   
    return vcvtq_s32_f32(vaddq_f32(_A,vdupq_n_f32(0.5f))); 
}
VT_FORCEINLINE __m128 _mm_cvtepi32_ps(const __m128i& _A) { return vcvtq_f32_s32(_A); }
// works only for input values smaller than +/- float(1<<27)
VT_FORCEINLINE __m128i _mm_cvttps_epi32(const __m128& _A)
{
    __m128i ret = vcvtq_n_s32_f32(_A,4);
    ret = vshrq_n_s32(ret,4);
    return ret;
}

// the Neon intrinsics require constant counts
#define _mm_srai_epi32(_A,_Count) vshrq_n_s32(_A,_Count)
#define _mm_srai_epi16(_A,_Count) vshrq_n_s16(_A,_Count)
#define _mm_srli_epi32(_A,_Count) vshrq_n_u32(_A,_Count)
#define _mm_srli_epi16(_A,_Count) vshrq_n_u16(_A,_Count)
#define _mm_slli_epi32(_A,_Count) vshlq_n_u32(_A,_Count)
#define _mm_slli_epi16(_A,_Count) vshlq_n_u16(_A,_Count)

VT_FORCEINLINE __m128i _mm_add_epi8(const __m128i& a, const __m128i& b) { return vaddq_s8(a,b); }
VT_FORCEINLINE __m128i _mm_sub_epi8(const __m128i& a, const __m128i& b) { return vsubq_s8(a,b); }

VT_FORCEINLINE __m128i _mm_adds_epu16(const __m128i& a, const __m128i& b) { return vqaddq_u16(a,b); }
VT_FORCEINLINE __m128i _mm_add_epi16(const __m128i& a, const __m128i& b) { return vaddq_s16(a,b); }
VT_FORCEINLINE __m128i _mm_adds_epi16(const __m128i& a, const __m128i& b) { return vqaddq_s16(a,b); }
VT_FORCEINLINE __m128i _mm_sub_epi16(const __m128i& a, const __m128i& b) { return vsubq_s16(a,b); }
VT_FORCEINLINE __m128i _mm_mullo_epi16(const __m128i& a, const __m128i& b) { return vmulq_u16(a,b); }
VT_FORCEINLINE __m128i _mm_abs_epi16(const __m128i& a) { return vabsq_s16(a); }

VT_FORCEINLINE __m128i _mm_add_epi32(const __m128i& _A, const __m128i& _B) { return vaddq_s32(_A,_B); }
VT_FORCEINLINE __m128i _mm_sub_epi32(const __m128i& a, const __m128i& b) { return vaddq_s32(a,b); }

VT_FORCEINLINE __m128i _mm_and_si128(const __m128i& _A, const __m128i& _B) { return vandq_u8(_A,_B); }
VT_FORCEINLINE __m128i _mm_andnot_si128(const __m128i& _A, const __m128i& _B) { return vbicq_u8(_B,_A); }
VT_FORCEINLINE __m128i _mm_or_si128(const __m128i& _A, const __m128i& _B) { return vorrq_u8(_A,_B); }
VT_FORCEINLINE __m128i _mm_xor_si128(const __m128i& _A, const __m128i& _B) { return veorq_u8(_A,_B); }

VT_FORCEINLINE __m128i _mm_max_epi16(const __m128i& _A, const __m128i& _B) { return vmaxq_s16(_A,_B); }
VT_FORCEINLINE __m128i _mm_min_epu16(const __m128i& _A, const __m128i& _B) { return vminq_u16(_A,_B); }

VT_FORCEINLINE __m128i _mm_cmpeq_epi16(const __m128i& a, const __m128i& b) { return vceqq_s16(a,b); }
VT_FORCEINLINE __m128i _mm_cmpgt_epi16(const __m128i& a, const __m128i& b) { return vcgtq_s16(a,b); }
VT_FORCEINLINE __m128i _mm_cmplt_epi8(const __m128i& a, const __m128i& b) { return vcltq_s8(a,b); }

// this works but compiles (v120) to very inefficient code...
#define _MM_TRANSPOSE4_PS(row0, row1, row2, row3) { \
    __n128x2 __tmp01 = vzipq_f32(row0,row1); \
    __n128x2 __tmp23 = vzipq_f32(row2,row3); \
    __n128 __tmp = __tmp01.val[0]; \
    __tmp01.val[0].s.high64 = __tmp23.val[0].s.low64; \
    __tmp23.val[0].s.low64 = __tmp.s.high64; \
    __tmp = __tmp01.val[1]; \
    __tmp01.val[1].s.high64 = __tmp23.val[1].s.low64; \
    __tmp23.val[1].s.low64 = __tmp.s.high64; \
    row0 = __tmp01.val[0]; \
    row1 = __tmp23.val[0]; \
    row2 = __tmp01.val[1]; \
    row3 = __tmp23.val[1]; }

#define SSE2_mm_extract_epf32(_A, _i) vgetq_lane_f32(_A, _i)
#define _mm_extract_epi16(_A, _Imm) vgetq_lane_u16(_A,_Imm)

#endif

//+----------------------------------------------------------------------------
//
// convenience functions to implement functionality that is efficiently 
// done in both SSE and Neon, but for which the syntax of the SSE is not
// specific to the desired op; these use syntax similar to SSE
//
//-----------------------------------------------------------------------------

#if (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_ARM) || defined(_M_ARM64))
//-----------------------------------------------------------------------------
//
// _simd_srli_epi8 - 8 bit shift for SSE
//
#if (defined(_M_IX86) || defined(_M_AMD64))
inline __m128i _simd_srli_epi8(const __m128i& _A, const int _shiftVal) 
{ 
    // expand to 16bpp
    __m128i x0 = _mm_unpacklo_epi8(_mm_setzero_si128(),_A);
    __m128i x1 = _mm_unpackhi_epi8(_mm_setzero_si128(),_A);
    x0 = _mm_srli_epi16(x0,_shiftVal+8);
    x1 = _mm_srli_epi16(x1,_shiftVal+8);
    return _mm_packus_epi16(x0,x1);
}
#elif (defined(_M_ARM) || defined(_M_ARM64))
#define _simd_srli_epi8(_A,_shiftVal) vshrq_n_u8(_A,_shiftVal)
#endif

//-----------------------------------------------------------------------------
// 
// _simd_abd_epi16 - absolute difference
//
#if (defined(_M_IX86) || defined(_M_AMD64))
inline __m128i _simd_abd_epi16(const __m128i& _A, const __m128i& _B) 
{ 
    return _mm_abs_epi16(_mm_sub_epi16(_A,_B));
}
#elif (defined(_M_ARM) || defined(_M_ARM64))
VT_FORCEINLINE __m128i _simd_abd_epi16(const __m128i& _A, const __m128i& _B) 
{ 
    return vabdq_s16(_A,_B);
}
#endif

//-----------------------------------------------------------------------------
// 
// _simd_interleave_epi16 - interleave two 16bpp registers
//
#if (defined(_M_IX86) || defined(_M_AMD64)) 
#define _simd_interleave_epi16(_reg0, _reg1) { \
    __m128i _tmp = _mm_unpacklo_epi16(_reg0, _reg1); \
    _reg1 = _mm_unpackhi_epi16(_reg0, _reg1); \
    _reg0 = _tmp; }
#elif (defined(_M_ARM) || defined(_M_ARM64))
#define _simd_interleave_epi16(_reg0, _reg1) { \
    __n128x2 _tmp = vzipq_u16(_reg0,_reg1); \
    _reg0 = _tmp.val[0]; \
    _reg1 = _tmp.val[1]; }
#endif

//-----------------------------------------------------------------------------
// 
// _simd_recip_ps - reciprocal instruction
//
#if (defined(_M_IX86) || defined(_M_AMD64))
inline __m128 _simd_recip_ps(const __m128& _A) 
{ 
    __m128 recip;
    recip = _mm_rcp_ps(_A);
	recip = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(2.f), _mm_mul_ps(_A, recip)),recip);
	recip = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(2.f), _mm_mul_ps(_A, recip)),recip);
    return recip;
}
#elif (defined(_M_ARM) || defined(_M_ARM64))
VT_FORCEINLINE __m128 _simd_recip_ps(const __m128& _A)
{
    // get an initial estimate of 1/_A
    float32x4_t recip = vrecpeq_f32(_A);
    // do two Newton-Raphson steps
    recip = vmulq_f32(vrecpsq_f32(_A, recip), recip);
    recip = vmulq_f32(vrecpsq_f32(_A, recip), recip);
    // return reciprocal
    return recip;
}
#endif
//-----------------------------------------------------------------------------
// 
// _simd_recip_lp_ps - low precision reciprocal instruction
//
#if (defined(_M_IX86) || defined(_M_AMD64))
inline __m128 _simd_recip_lp_ps(const __m128& _A) 
{ 
    __m128 recip;
    recip = _mm_rcp_ps(_A);
	recip = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(2.f), _mm_mul_ps(_A, recip)),recip);
    return recip;
}
#elif (defined(_M_ARM) || defined(_M_ARM64))
VT_FORCEINLINE __m128 _simd_recip_lp_ps(const __m128& _A)
{
    // get an initial estimate of 1/_A
    float32x4_t recip = vrecpeq_f32(_A);
    // do one Newton-Raphson steps
    recip = vmulq_f32(vrecpsq_f32(_A, recip), recip);
    // return reciprocal
    return recip;
}
#endif
//-----------------------------------------------------------------------------
// 
// _simd_recip_vlp_ps - very low precision reciprocal instruction (seed only)
//
#if (defined(_M_IX86) || defined(_M_AMD64))
#define _simd_recip_vlp_ps _mm_rcp_ps
#elif (defined(_M_ARM) || defined(_M_ARM64))
#define _simd_recip_vlp_ps vrecpeq_f32
#endif
//-----------------------------------------------------------------------------
//
// _simd_dup##N##_ps(reg) - duplicate reg[N] value to all 4 result elements
//
#if (defined(_M_IX86) || defined(_M_AMD64))
#define _simd_dup0_ps(_A) _mm_shuffle_ps(_A,_A,_MM_SHUFFLE(0,0,0,0))
#elif (defined(_M_ARM) || defined(_M_ARM64))
#define _simd_dup0_ps(_A) vdupq_lane_f32(((__n128)_A).s.low64,0)
#endif
#if (defined(_M_IX86) || defined(_M_AMD64))
#define _simd_dup1_ps(_A) _mm_shuffle_ps(_A,_A,_MM_SHUFFLE(1,1,1,1))
#elif (defined(_M_ARM) || defined(_M_ARM64))
#define _simd_dup1_ps(_A) vdupq_lane_f32(((__n128)_A).s.low64,1)
#endif
#if (defined(_M_IX86) || defined(_M_AMD64))
#define _simd_dup2_ps(_A) _mm_shuffle_ps(_A,_A,_MM_SHUFFLE(2,2,2,2))
#elif (defined(_M_ARM) || defined(_M_ARM64))
#define _simd_dup2_ps(_A) vdupq_lane_f32(((__n128)_A).s.high64,0)
#endif
#if (defined(_M_IX86) || defined(_M_AMD64))
#define _simd_dup3_ps(_A) _mm_shuffle_ps(_A,_A,_MM_SHUFFLE(3,3,3,3))
#elif (defined(_M_ARM) || defined(_M_ARM64))
#define _simd_dup3_ps(_A) vdupq_lane_f32(((__n128)_A).s.high64,1)
#endif
//-----------------------------------------------------------------------------
//
// _simd_merge_012_3_ps(regA,regB) - set result to {regA[0],regA[1],regA[2],regB[3]} 
//
#define _simd_merge_123_3_ps_const() \
    __m128 _simdconst_amask; \
    { __m128i _simdconst_amaski = _mm_setr_epi32(0,0,0,-1); \
    _simdconst_amask = _mm_castsi128_ps(_simdconst_amaski); }
#if (defined(_M_IX86) || defined(_M_AMD64))
#define _simd_merge_012_3_ps(_Ret,_A,_B) \
{ \
    __m128 tmp0,tmp1; \
    tmp0 = _mm_andnot_ps(_simdconst_amask,_A); \
    tmp1 = _mm_and_ps(_simdconst_amask,_B); \
    _Ret = _mm_or_ps(tmp0,tmp1); \
}
#elif (defined(_M_ARM) || defined(_M_ARM64))
#define _simd_merge_012_3_ps(_Ret,_A,_B) \
{ \
    __m128 tmp = _simdconst_amask; \
    tmp = vbslq_u8(tmp, _B,_A); \
    _Ret = tmp; \
}
#endif

//-----------------------------------------------------------------------------
// 
// _simd_mergelo/hi_ps - merge low/high halves of two 128bit registers
//
#if (defined(_M_IX86) || defined(_M_AMD64))
inline __m128i _simd_mergelo_si128(const __m128i& _A, const __m128i& _B) 
{ 
    return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_A),_mm_castsi128_ps(_B),_MM_SHUFFLE(1,0,1,0)));
}
inline __m128i _simd_mergehi_si128(const __m128i& _A, const __m128i& _B) 
{ 
    return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_A),_mm_castsi128_ps(_B),_MM_SHUFFLE(3,2,3,2)));
}
#elif (defined(_M_ARM) || defined(_M_ARM64))
VT_FORCEINLINE __m128i _simd_mergelo_si128(const __m128i& _A, const __m128i& _B) 
{ 
    __m128i x0 = vextq_u64(_B,_A,1); // B1A0
    return vextq_u64(x0,_B,1); // A0B0
}
VT_FORCEINLINE __m128i _simd_mergehi_si128(const __m128i& _A, const __m128i& _B) 
{ 
    __m128i x0 = vextq_u64(_B,_A,1); // B1A0
    return vextq_u64(_A,x0,1); // A1B1
}
#endif

//-----------------------------------------------------------------------------
//
// load 8 bytes and convert to 8 UInt16 values in 128 bit register
//
#define _simd_load_8byte_si128_const()
#if (defined(_M_IX86) || defined(_M_AMD64))
#define _simd_load_8byte_si128(_ret, _ptr) \
    _ret = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i*)(_ptr)),_mm_setzero_si128());
#elif (defined(_M_ARM) || defined(_M_ARM64))
#define _simd_load_8byte_si128(_ret, _ptr) \
    _ret = vmovl_u8(vld1_u8(_ptr));
#endif
//-----------------------------------------------------------------------------
// 
// convert 8 UInt16 values to 8 bit with rounding and store
//
#ifndef _SIMD_STORE_8BYTE_FRAC_BITS 
#define _SIMD_STORE_8BYTE_FRAC_BITS 8
#endif

#if (defined(_M_IX86) || defined(_M_AMD64))
#define _simd_store_si128_8byte_const()
#endif

#if (defined(_M_IX86) || defined(_M_AMD64))
#define _simd_store_si128_8byte(_val, _ptr) \
    _val = _mm_add_epi16(_val,_mm_set1_epi16(1<<(_SIMD_STORE_8BYTE_FRAC_BITS-1))); /* add .5 */ \
    _val = _mm_srli_epi16(_val,_SIMD_STORE_8BYTE_FRAC_BITS); /* shift away fraction */ \
    _val = _mm_packus_epi16(_val,_val); /* narrow */ \
    _mm_storel_epi64((__m128i*)(_ptr),_val);
#elif (defined(_M_ARM) || defined(_M_ARM64))
#define _simd_store_si128_8byte(_val, _ptr) \
{   uint8x8_t _tmp = vqrshrn_n_u16(_val,_SIMD_STORE_8BYTE_FRAC_BITS); /* round, shift, narrow */ \
    vst1_u8((_ptr),_tmp); }
#endif

//-----------------------------------------------------------------------------
// 
// convert 8 Int16 values from fixed point to integer with rounding
//
#if (defined(_M_IX86) || defined(_M_AMD64))
#define _simd_fixed2int_epi16(_val, _bits) \
    _val = _mm_add_epi16(_val,_mm_set1_epi16(1<<((((_bits)>0)?(_bits):1)-1))); \
    _val = _mm_srai_epi16(_val,((_bits)>0)?(_bits):1);
#define _simd_fixed2int_epu16(_val, _bits) \
    _val = _mm_adds_epu16(_val,_mm_set1_epi16(1<<((((_bits)>0)?(_bits):1)-1))); \
    _val = _mm_srli_epi16(_val,((_bits)>0)?(_bits):1);
#elif (defined(_M_ARM) || defined(_M_ARM64))
#define _simd_fixed2int_epi16(_val, _bits) \
    _val = vrshrq_n_s16(_val,((_bits)>0)?(_bits):1);
#define _simd_fixed2int_epu16(_val, _bits) \
    _val = vrshrq_n_u16(_val,((_bits)>0)?(_bits):1);
#endif

#endif

#endif // _VT_SIMD_INCLUDED
//-----------------------------------------------------------------------------
// end