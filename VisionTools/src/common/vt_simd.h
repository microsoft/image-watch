//+----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description: Support for single source SIMD code that will compile with
//  both the MS compiler and gnu/clang for both x86/SSE and arm/Neon targets.
//  
//  This SIMD code primarily uses SSE operation syntax combined with the NEON
//  register operand syntax.  SSE operation syntax is selected in part because
//  VT already had a lot of SSE code and it makes the transition of existing 
//  code easier, but also because the SSE operations are generally more restrictive
//  than NEON so using the SSE syntax pushes for a narrower set of operations
//  that will be supported on both architectures.  
//
//  Note that the strongly typed operand/register usage is not required for the
//  MS compiler for NEON, but is for gnu/clang. Using the strongly typed operand
//  syntax forces writing more readable code so is a good thing to do anyway.
//
//  Note that there are differences other than the strong typing between the
//  MS compiler and gnu/clang compiler NEON intrinsics, and this file also
//  provides mappings for those so NEON specific code will compile for both.
//
//-----------------------------------------------------------------------------
#pragma once

#if defined(_VT_SIMD_INCLUDED)

#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386) || defined(__x86_64__)
#define VT_SSE_SUPPORTED
#endif
#if defined(_M_ARM) || defined(__ARM_NEON__) || defined(_M_ARM64) || defined(__arm__) || defined(__aarch64__)
#define VT_NEON_SUPPORTED
#include <arm_neon.h>
#endif

#if (defined(VT_SSE_SUPPORTED) || defined(VT_NEON_SUPPORTED))
#define VT_SIMD_SUPPORTED
#endif

#if defined(VT_SIMD_SUPPORTED)
#if defined(_M_IX86) || defined(_M_AMD64) || defined(_M_ARM) || defined(_M_ARM64)
#define VT_IS_MSVC_COMPILER
#else
#define VT_IS_GNU_COMPILER
#endif
#endif

#if defined(VT_SSE_SUPPORTED) && defined(VT_IS_MSVC_COMPILER)
#define VT_AVX_SUPPORTED
#endif


//+----------------------------------------------------------------------------
//
// NEON intrinsic operand syntax for when compiling for x86/x64; note that use
// is not enforced by x86/x64 compilers (i.e. code could still use __m128) but
// would fail compilation on arm compilers
//
//-----------------------------------------------------------------------------
#if defined(VT_SSE_SUPPORTED)

#define float32_t   float
#define float32x4_t __m128
#define int8x16_t   __m128i
#define int16x8_t   __m128i
#define int32x4_t   __m128i
#define int64x2_t   __m128i
#define uint8x16_t  __m128i
#define uint16x8_t  __m128i
#define uint32x4_t  __m128i
#define uint64x2_t  __m128i

#define vreinterpretq_f32_u8 _mm_castsi128_ps
#define vreinterpretq_f32_s8 _mm_castsi128_ps
#define vreinterpretq_f32_u16 _mm_castsi128_ps
#define vreinterpretq_f32_s16 _mm_castsi128_ps
#define vreinterpretq_f32_u32 _mm_castsi128_ps
#define vreinterpretq_f32_s32 _mm_castsi128_ps

#define vreinterpretq_u16_s16(_val) _val

#endif

//+----------------------------------------------------------------------------
//
// syntax for scalar indexing into registers
//
//-----------------------------------------------------------------------------
#if defined(VT_SSE_SUPPORTED)
#if defined(VT_IS_MSVC_COMPILER)
#define vget_f32(_A,_Idx) _A.m128_f32[_Idx]
#elif defined(VT_IS_GNU_COMPILER)
#define vget_f32(_A,_Idx) _A[_Idx]
#endif
#endif

#if defined(VT_SSE_SUPPORTED)
#if defined(VT_IS_MSVC_COMPILER)
#define vget_low_u64(__reg) __reg.m128i_u64[0]
#define vget_high_u64(__reg) __reg.m128i_u64[1]
#endif
#endif

//+----------------------------------------------------------------------------
// mapping the MSVC compiler Neon aligned load intrinsics to the unaligned
// versions since clang does not directly support them 
//-----------------------------------------------------------------------------
#if !defined(VT_IS_MSVC_COMPILER)

#define vst1q_u8_ex(__ptr,__reg,__align)  vst1q_u8(__ptr,__reg)
#define vst1q_s8_ex(__ptr,__reg,__align)  vst1q_s8(__ptr,__reg)
#define vst1q_u16_ex(__ptr,__reg,__align) vst1q_u16(__ptr,__reg)
#define vst1q_s16_ex(__ptr,__reg,__align) vst1q_s16(__ptr,__reg)
#define vst1q_u32_ex(__ptr,__reg,__align) vst1q_u32(__ptr,__reg)
#define vst1q_s32_ex(__ptr,__reg,__align) vst1q_s32(__ptr,__reg)
#define vst1q_f32_ex(__ptr,__reg,__align) vst1q_f32(__ptr,__reg)

#define vst1_u8_ex(__ptr,__reg,__align)  vst1_u8(__ptr,__reg)
#define vst1_s8_ex(__ptr,__reg,__align)  vst1_s8(__ptr,__reg)
#define vst1_u16_ex(__ptr,__reg,__align) vst1_u16(__ptr,__reg)
#define vst1_s16_ex(__ptr,__reg,__align) vst1_s16(__ptr,__reg)
#define vst1_u32_ex(__ptr,__reg,__align) vst1_u32(__ptr,__reg)
#define vst1_s32_ex(__ptr,__reg,__align) vst1_s32(__ptr,__reg)
#define vst1_f32_ex(__ptr,__reg,__align) vst1_f32(__ptr,__reg)

#define vld1q_u8_ex(__ptr,__align)  vld1q_u8(__ptr)
#define vld1q_s8_ex(__ptr,__align)  vld1q_s8(__ptr)
#define vld1q_u16_ex(__ptr,__align) vld1q_u16(__ptr)
#define vld1q_s16_ex(__ptr,__align) vld1q_s16(__ptr)
#define vld1q_u32_ex(__ptr,__align) vld1q_u32(__ptr)
#define vld1q_s32_ex(__ptr,__align) vld1q_s32(__ptr)
#define vld1q_f32_ex(__ptr,__align) vld1q_f32(__ptr)

#define vld1_u8_ex(__ptr,__align)  vld1_u8(__ptr)
#define vld1_s8_ex(__ptr,__align)  vld1_s8(__ptr)
#define vld1_u16_ex(__ptr,__align) vld1_u16(__ptr)
#define vld1_s16_ex(__ptr,__align) vld1_s16(__ptr)
#define vld1_u32_ex(__ptr,__align) vld1_u32(__ptr)
#define vld1_s32_ex(__ptr,__align) vld1_s32(__ptr)
#define vld1_f32_ex(__ptr,__align) vld1_f32(__ptr)

#endif
//+----------------------------------------------------------------------------
//
// SSE instruction syntax implemented with Neon intrinsics
//
//-----------------------------------------------------------------------------
#if (defined(VT_NEON_SUPPORTED) && !defined(VT_SSE_SUPPORTED))

#define _mm_castps_si128 vreinterpretq_u32_f32
VT_FORCEINLINE float32x4_t  _mm_castsi128_ps(const uint8x16_t& _expr) { return vreinterpretq_f32_u8(_expr); }
#if !defined(VT_IS_MSVC_COMPILER)
VT_FORCEINLINE float32x4_t  _mm_castsi128_ps(const uint32x4_t& _expr) { return vreinterpretq_f32_u32(_expr); }
VT_FORCEINLINE float32x4_t  _mm_castsi128_ps(const int32x4_t& _expr) { return vreinterpretq_f32_u32(_expr); }
#endif

VT_FORCEINLINE int8x16_t _mm_set1_epi8(char val) { return vdupq_n_s8(val); }
VT_FORCEINLINE int16x8_t _mm_set1_epi16(short val) { return vdupq_n_s16(val); }
VT_FORCEINLINE int32x4_t _mm_set1_epi32(int val) { return vdupq_n_s32(val); }

#if defined(VT_IS_MSVC_COMPILER)
#define __SI128_LOAD_OPS__(__type__,__op__,__cast__) \
VT_FORCEINLINE __type__ _mm_load_si128(const __type__* _p) { return vld1q_##__op__##_ex((__cast__*)_p,128); } \
VT_FORCEINLINE __type__ _mm_loadu_si128(const __type__* _p) { return vld1q_##__op__((__cast__*)_p); }
#else
#define __SI128_LOAD_OPS__(__type__,__op__,__cast__) \
VT_FORCEINLINE __type__ _mm_load_si128(const __type__* _p) { return vld1q_##__op__((__cast__*)_p); } \
VT_FORCEINLINE __type__ _mm_loadu_si128(const __type__* _p) { return vld1q_##__op__((__cast__*)_p); }
#endif
__SI128_LOAD_OPS__(uint8x16_t, u8,  uint8_t)
#if !defined(VT_IS_MSVC_COMPILER)
__SI128_LOAD_OPS__(int8x16_t,  s8,  int8_t)
__SI128_LOAD_OPS__(uint16x8_t, u16, uint16_t)
__SI128_LOAD_OPS__(int16x8_t,  s16, int16_t)
__SI128_LOAD_OPS__(uint32x4_t, u32, uint32_t)
__SI128_LOAD_OPS__(int32x4_t,  s32, int32_t)
#endif

VT_FORCEINLINE float32x4_t _mm_load_ps(const float* _p) { return vld1q_f32_ex(_p,128); }
VT_FORCEINLINE float32x4_t _mm_loadu_ps(const float* _p) { return vld1q_f32(_p); }
VT_FORCEINLINE float32x4_t _mm_load1_ps(const float* _p) { return vld1q_dup_f32(_p); }

VT_FORCEINLINE void _mm_storeu_ps(float* pOut, const float32x4_t& _A) { vst1q_f32(pOut,_A); }
VT_FORCEINLINE void _mm_store_ps(float* pOut, const float32x4_t& _A) { vst1q_f32_ex(pOut,_A,128); }

#if defined(VT_IS_MSVC_COMPILER)
#define __SI128_STORE_OPS__(__type__,__op__,__cast__) \
VT_FORCEINLINE void _mm_store_si128(__type__* pOut, const __type__& _A) { vst1q_##__op__##_ex((__cast__*)pOut,_A,128); } \
VT_FORCEINLINE void _mm_storeu_si128(__type__* pOut, const __type__& _A) { vst1q_##__op__((__cast__*)pOut,_A); }
#else
#define __SI128_STORE_OPS__(__type__,__op__,__cast__) \
VT_FORCEINLINE void _mm_store_si128(__type__* pOut, const __type__& _A) { vst1q_##__op__((__cast__*)pOut,_A); } \
VT_FORCEINLINE void _mm_storeu_si128(__type__* pOut, const __type__& _A) { vst1q_##__op__((__cast__*)pOut,_A); }
#endif
__SI128_STORE_OPS__(uint8x16_t, u8,  uint8_t)
#if !defined(VT_IS_MSVC_COMPILER)
__SI128_STORE_OPS__(int8x16_t,  s8,  int8_t)
__SI128_STORE_OPS__(uint16x8_t, u16, uint16_t)
__SI128_STORE_OPS__(int16x8_t,  s16, int16_t)
__SI128_STORE_OPS__(uint32x4_t, u32, uint32_t)
__SI128_STORE_OPS__(int32x4_t,  s32, int32_t)
#endif

VT_FORCEINLINE void _mm_storel_epi64(uint8x16_t* pOut, const uint8x16_t& _A) { vst1_u8((uint8_t*)pOut,vget_low_u8(_A)); }


VT_FORCEINLINE uint32x4_t _mm_setzero_si128() { const uint32_t zero = 0x0; return vld1q_dup_u32(&zero); }

VT_FORCEINLINE float32x4_t _mm_setzero_ps() { const float zero = 0.f; return vld1q_dup_f32(&zero); }
VT_FORCEINLINE float32x4_t _mm_set1_ps(float32_t val) { return vdupq_n_f32(val); }
VT_FORCEINLINE float32x4_t _mm_set_ps1(float32_t val) { return vdupq_n_f32(val); }
VT_FORCEINLINE float32x4_t _mm_setr_ps(float32_t _A, float32_t _B, float32_t _C, float32_t _D)
{ 
    float32x4_t __tmpreg;
    __tmpreg = vdupq_n_f32(_A);
    __tmpreg = vsetq_lane_f32(_B,__tmpreg,1);
    __tmpreg = vsetq_lane_f32(_C,__tmpreg,2);
    __tmpreg = vsetq_lane_f32(_D,__tmpreg,3);
    return __tmpreg;
}
VT_FORCEINLINE float32x4_t _mm_set_ps(float32_t _D, float32_t _C, float32_t _B, float32_t _A)
{ 
    float32x4_t __tmpreg;
    __tmpreg = vdupq_n_f32(_A);
    __tmpreg = vsetq_lane_f32(_B,__tmpreg,1);
    __tmpreg = vsetq_lane_f32(_C,__tmpreg,2);
    __tmpreg = vsetq_lane_f32(_D,__tmpreg,3);
    return __tmpreg;
}
VT_FORCEINLINE int32x4_t  _mm_setr_epi32(int32_t _A, int32_t _B, int32_t _C, int32_t _D)
{
    uint32x4_t __tmpreg;
    __tmpreg = vdupq_n_s32(_A);
    __tmpreg = vsetq_lane_s32(_B,__tmpreg,1);
    __tmpreg = vsetq_lane_s32(_C,__tmpreg,2);
    __tmpreg = vsetq_lane_s32(_D,__tmpreg,3);
    return __tmpreg;
}

VT_FORCEINLINE int32x4_t _mm_cvtps_epi32(const float32x4_t& _A) { return vcvtq_s32_f32(vaddq_f32(_A,vdupq_n_f32(0.5f))); }
VT_FORCEINLINE int32x4_t _mm_cvttps_epi32(const float32x4_t& _A) { return vcvtq_s32_f32(_A); }
VT_FORCEINLINE float32x4_t _mm_cvtepi32_ps(const int32x4_t& _A) { return vcvtq_f32_s32(_A); }

VT_FORCEINLINE float32x4_t _mm_mul_ps(const float32x4_t& _A, const float32x4_t& _B) { return vmulq_f32(_A,_B); }
VT_FORCEINLINE float32x4_t _mm_add_ps(const float32x4_t& _A, const float32x4_t& _B) { return vaddq_f32(_A,_B); }
VT_FORCEINLINE float32x4_t _mm_sub_ps(const float32x4_t& _A, const float32x4_t& _B) { return vsubq_f32(_A,_B); }
VT_FORCEINLINE float32x4_t _mm_div_ps(const float32x4_t& _A, const float32x4_t& _B)
{
    // get an initial estimate of 1/_B
    float32x4_t recip = vrecpeq_f32(_B);
    // do two Newton-Raphson steps
    recip = vmulq_f32(vrecpsq_f32(_B, recip), recip);
    recip = vmulq_f32(vrecpsq_f32(_B, recip), recip);
    // apply reciprocal
    return vmulq_f32(_A,recip);
}
VT_FORCEINLINE float32x4_t _mm_max_ps(const float32x4_t& _A, const float32x4_t& _B) { return vmaxq_f32(_A,_B); }
VT_FORCEINLINE float32x4_t _mm_min_ps(const float32x4_t& _A, const float32x4_t& _B) { return vminq_f32(_A,_B); }
VT_FORCEINLINE float32x4_t _mm_andnot_ps(const float32x4_t& _A, const float32x4_t& _B) { return vbicq_u32(_B, _A); }
VT_FORCEINLINE float32x4_t _mm_and_ps(const float32x4_t& _A, const float32x4_t& _B) { return vandq_u32(_B, _A); }
VT_FORCEINLINE float32x4_t _mm_or_ps(const float32x4_t& _A, const float32x4_t& _B) { return vorrq_u32(_A, _B); }
VT_FORCEINLINE float32x4_t _mm_xor_ps(const float32x4_t& _A, const float32x4_t& _B) { return veorq_u32(_A, _B); }



// the Neon intrinsics require a constant shift count
#define _mm_srai_epi32(_A,_Count) vshrq_n_s32(_A,_Count)
#define _mm_srai_epi16(_A,_Count) vshrq_n_s16(_A,_Count)
#define _mm_srli_epi32(_A,_Count) vshrq_n_u32(_A,_Count)
#define _mm_srli_epi16(_A,_Count) vshrq_n_u16(_A,_Count)
#define _mm_slli_epi32(_A,_Count) vshlq_n_u32(_A,_Count)
#define _mm_slli_epi16(_A,_Count) vshlq_n_u16(_A,_Count)

#define __SI128_LOGIC_OPS__(__type__) \
VT_FORCEINLINE __type__ _mm_and_si128(const __type__& _A, const __type__& _B) { return vandq_u8(_A,_B); } \
VT_FORCEINLINE __type__ _mm_andnot_si128(const __type__& _A, const __type__& _B) { return vbicq_u8(_B,_A); } \
VT_FORCEINLINE __type__ _mm_or_si128(const __type__& _A, const __type__& _B) { return vorrq_u8(_A,_B); } \
VT_FORCEINLINE __type__ _mm_xor_si128(const __type__& _A, const __type__& _B) { return veorq_u8(_A,_B); }
__SI128_LOGIC_OPS__(uint8x16_t)
#if !defined(VT_IS_MSVC_COMPILER)
__SI128_LOGIC_OPS__(int8x16_t)
__SI128_LOGIC_OPS__(uint16x8_t)
__SI128_LOGIC_OPS__(int16x8_t)
__SI128_LOGIC_OPS__(uint32x4_t)
__SI128_LOGIC_OPS__(int32x4_t)
#endif

VT_FORCEINLINE uint16x8_t _mm_packs_epi32(const uint32x4_t& _A, const uint32x4_t& _B) { return vcombine_u16(vqmovn_u32(_A), vqmovn_u32(_B)); }
VT_FORCEINLINE uint8x16_t _mm_packus_epi16(const int16x8_t& _A, const int16x8_t& _B) { return vcombine_u8(vqmovun_s16(_A), vqmovun_s16(_B)); }

VT_FORCEINLINE int8x16_t _mm_add_epi8(const int8x16_t& a, const int8x16_t& b) { return vaddq_s8(a,b); }
VT_FORCEINLINE int8x16_t _mm_sub_epi8(const int8x16_t& a, const int8x16_t& b) { return vsubq_s8(a,b); }
VT_FORCEINLINE uint8x16_t _mm_avg_epu8(const uint8x16_t& a, const uint8x16_t& b) { return vrhaddq_u8(a,b); }

VT_FORCEINLINE uint16x8_t _mm_adds_epu16(const uint16x8_t& a, const uint16x8_t& b) { return vqaddq_u16(a,b); }
VT_FORCEINLINE int16x8_t _mm_add_epi16(const int16x8_t& a, const int16x8_t& b) { return vaddq_s16(a,b); }
VT_FORCEINLINE int16x8_t _mm_adds_epi16(const int16x8_t& a, const int16x8_t& b) { return vqaddq_s16(a,b); }
VT_FORCEINLINE int16x8_t _mm_sub_epi16(const int16x8_t& a, const int16x8_t& b) { return vsubq_s16(a,b); }
VT_FORCEINLINE uint16x8_t _mm_mullo_epi16(const uint16x8_t& a, const uint16x8_t& b) { return vmulq_u16(a,b); }
VT_FORCEINLINE int16x8_t _mm_abs_epi16(const int16x8_t& a) { return vabsq_s16(a); }

VT_FORCEINLINE int32x4_t _mm_sub_epi32(const int32x4_t& a, const int32x4_t& b) { return vsubq_s32(a,b); }
VT_FORCEINLINE int32x4_t _mm_add_epi32(const int32x4_t& a, const int32x4_t& b) { return vaddq_s32(a,b); }

VT_FORCEINLINE int64x2_t _mm_sub_epi64(const int64x2_t& a, const int64x2_t& b) { return vsubq_s64(a,b); }
VT_FORCEINLINE int64x2_t _mm_add_epi64(const int64x2_t& a, const int64x2_t& b) { return vaddq_s64(a,b); }

VT_FORCEINLINE int16x8_t _mm_max_epi16(const int16x8_t& a, const int16x8_t& b) { return vmaxq_s16(a,b); }
VT_FORCEINLINE int16x8_t _mm_min_epi16(const int16x8_t& a, const int16x8_t& b) { return vminq_s16(a,b); }
VT_FORCEINLINE uint16x8_t _mm_max_epu16(const uint16x8_t& a, const uint16x8_t& b) { return vmaxq_u16(a,b); }
VT_FORCEINLINE uint16x8_t _mm_min_epu16(const uint16x8_t& a, const uint16x8_t& b) { return vminq_u16(a,b); }

VT_FORCEINLINE int16x8_t _mm_cmpeq_epi16(const int16x8_t& a, const int16x8_t& b) { return vceqq_s16(a,b); }
VT_FORCEINLINE int16x8_t _mm_cmpgt_epi16(const int16x8_t& a, const int16x8_t& b) { return vcgtq_s16(a,b); }
VT_FORCEINLINE int16x8_t _mm_cmplt_epi16(const int16x8_t& a, const int16x8_t& b) { return vcltq_s16(a,b); }
VT_FORCEINLINE uint8x16_t _mm_cmpeq_epi8(const int8x16_t& a, const int8x16_t& b) { return vceqq_s8(a,b); }
VT_FORCEINLINE uint8x16_t _mm_cmpgt_epi8(const int8x16_t& a, const int8x16_t& b) { return vcgtq_s8(a,b); }
VT_FORCEINLINE uint8x16_t _mm_cmplt_epi8(const int8x16_t& a, const int8x16_t& b) { return vcltq_s8(a,b); }

VT_FORCEINLINE uint64x2_t _mm_sad_epu8(const uint8x16_t& a, const uint8x16_t& b)
{
    uint8x16_t vad16 = vabdq_u8(a,b); // per-channel abs diff
    uint16x8_t vad8 = vpaddlq_u8(vad16); // adjacent pairs added to form 8 16 bit results
    uint32x4_t vad4 = vpaddlq_u16(vad8); // again for 4 32 bit results
    uint64x2_t vad2 = vpaddlq_u32(vad4); // again for result (2x64 results)
    return vad2;
}

VT_FORCEINLINE float32x4_t _mm_unpacklo_ps(const float32x4_t& a, const float32x4_t& b)
{
    float32x2x2_t result = vzip_f32(vget_low_f32(a), vget_low_f32(b));
    return vcombine_f32(result.val[0], result.val[1]);
}

VT_FORCEINLINE float32x4_t _mm_unpackhi_ps(const float32x4_t& a, const float32x4_t& b)
{
    float32x2x2_t result = vzip_f32(vget_high_f32(a), vget_high_f32(b));
    return vcombine_f32(result.val[0], result.val[1]);
}

VT_FORCEINLINE float32x4_t _mm_loadl_pi(float32x4_t& a, const float* b) { return vcombine_f32(vld1_f32(b), vget_high_f32(a)); }

VT_FORCEINLINE float32x4_t _mm_loadh_pi(float32x4_t& a, const float* b) { return vcombine_f32(vget_low_f32(a), vld1_f32(b)); }

VT_FORCEINLINE void _mm_storel_pi(float* a, float32x4_t& b) { vst1_f32(a, vget_low_f32(b)); };

VT_FORCEINLINE void _mm_storeh_pi(float* a, float32x4_t& b) { vst1_f32(a, vget_high_f32(b)); };

#define _mm_extract_epi32(_A, _i) vgetq_lane_u32(_A, _i)

#if 0
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
#endif

#define SSE2_mm_extract_epf32(_A, _i) vgetq_lane_f32(_A, _i)
#define _mm_extract_epi16(_A, _Imm) vgetq_lane_u16(_A,_Imm)

#define _MM_EXTRACT_FLOAT(_floatVal,_vectorVal,_index) _floatVal = vgetq_lane_f32(_vectorVal,_index)

#endif

#if defined(VT_SIMD_SUPPORTED)
//+----------------------------------------------------------------------------
//
// convenience functions to implement functionality that is efficiently 
// done in both SSE and Neon, but for which the syntax of the SSE is not
// specific to the desired op; these use syntax similar to SSE
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// _simd_srli_epi8 - 8 bit shift for SSE
//
#if defined(VT_SSE_SUPPORTED)
inline uint8x16_t _simd_srli_epi8(const uint8x16_t& _A, const int _shiftVal) 
{ 
    // expand to 16bpp
    __m128i x0 = _mm_unpacklo_epi8(_mm_setzero_si128(),_A);
    __m128i x1 = _mm_unpackhi_epi8(_mm_setzero_si128(),_A);
    x0 = _mm_srli_epi16(x0,_shiftVal+8);
    x1 = _mm_srli_epi16(x1,_shiftVal+8);
    return _mm_packus_epi16(x0,x1);
}
#else // NEON
#define _simd_srli_epi8(_A,_shiftVal) vshrq_n_u8(_A,_shiftVal)
#endif

//-----------------------------------------------------------------------------
// 
// _simd_abd_epi16 - absolute difference
//
VT_FORCEINLINE int16x8_t _simd_abd_epi16(const int16x8_t& _A, const int16x8_t& _B) 
{ 
#if defined(VT_SSE_SUPPORTED)
    return _mm_abs_epi16(_mm_sub_epi16(_A,_B));
#else
    return vabdq_s16(_A,_B);
#endif
}

//-----------------------------------------------------------------------------
// 
// _simd_interleave_epi16 - interleave two 16bpp registers
//
#if defined(VT_SSE_SUPPORTED)
#define _simd_interleave_epi16(_reg0, _reg1) { \
    __m128i _tmp = _mm_unpacklo_epi16(_reg0, _reg1); \
    _reg1 = _mm_unpackhi_epi16(_reg0, _reg1); \
    _reg0 = _tmp; }
#elif defined(VT_NEON_SUPPORTED)
#define _simd_interleave_epi16(_reg0, _reg1) { \
    uint16x8x2_t _tmp = vzipq_u16(_reg0,_reg1); \
    _reg0 = _tmp.val[0]; \
    _reg1 = _tmp.val[1]; }
#endif

//-----------------------------------------------------------------------------
// 
// _simd_recip_ps - reciprocal instruction
//
inline float32x4_t _simd_recip_ps(const float32x4_t& _A) 
{ 
#if defined(VT_SSE_SUPPORTED)
    float32x4_t recip = _mm_rcp_ps(_A);
	recip = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(2.f), _mm_mul_ps(_A, recip)),recip);
	recip = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(2.f), _mm_mul_ps(_A, recip)),recip);
#elif defined(VT_NEON_SUPPORTED)
    // get an initial estimate of 1/_A
    float32x4_t recip = vrecpeq_f32(_A);
    // do two Newton-Raphson steps
    recip = vmulq_f32(vrecpsq_f32(_A, recip), recip);
    recip = vmulq_f32(vrecpsq_f32(_A, recip), recip);
#endif
    // return reciprocal
    return recip;
}

//-----------------------------------------------------------------------------
// 
// _simd_recip_lp_ps - low precision reciprocal instruction
//
inline float32x4_t _simd_recip_lp_ps(const float32x4_t& _A) 
{ 
#if defined(VT_SSE_SUPPORTED)
    float32x4_t recip = _mm_rcp_ps(_A);
	recip = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(2.f), _mm_mul_ps(_A, recip)),recip);
#elif defined(VT_NEON_SUPPORTED)
    // get an initial estimate of 1/_A
    float32x4_t recip = vrecpeq_f32(_A);
    // do one Newton-Raphson steps
    recip = vmulq_f32(vrecpsq_f32(_A, recip), recip);
#endif
    return recip;
}

#if 0
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

#endif

//-----------------------------------------------------------------------------
// 
// _simd_mergelo/hi_ps - merge low/high halves of two 128bit registers
//
inline uint8x16_t _simd_mergelo_si128(const uint8x16_t& _A, const uint8x16_t& _B) 
{ 
#if defined(VT_SSE_SUPPORTED)
    return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_A),_mm_castsi128_ps(_B),_MM_SHUFFLE(1,0,1,0)));
#else
    uint8x16_t x0 = vextq_u64(_B,_A,1); // B1A0
    return vextq_u64(x0,_B,1); // A0B0
#endif
}
inline uint8x16_t _simd_mergehi_si128(const uint8x16_t& _A, const uint8x16_t& _B) 
{ 
#if defined(VT_SSE_SUPPORTED)
    return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_A),_mm_castsi128_ps(_B),_MM_SHUFFLE(3,2,3,2)));
#else
    uint8x16_t x0 = vextq_u64(_B,_A,1); // B1A0
    return vextq_u64(_A,x0,1); // A1B1
#endif
}

//-----------------------------------------------------------------------------
//
// load 8 bytes and convert to 8 UInt16 values in 128 bit register
//
#define _simd_load_8byte_si128_const()
#if defined(VT_SSE_SUPPORTED)
#define _simd_load_8byte_si128(_ret, _ptr) \
    _ret = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i*)(_ptr)),_mm_setzero_si128());
#elif defined(VT_NEON_SUPPORTED)
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

#if defined(VT_SSE_SUPPORTED)
#define _simd_store_si128_8byte_const()
#endif

#if defined(VT_SSE_SUPPORTED)
#define _simd_store_si128_8byte(_val, _ptr) \
    _val = _mm_add_epi16(_val,_mm_set1_epi16(1<<(_SIMD_STORE_8BYTE_FRAC_BITS-1))); /* add .5 */ \
    _val = _mm_srli_epi16(_val,_SIMD_STORE_8BYTE_FRAC_BITS); /* shift away fraction */ \
    _val = _mm_packus_epi16(_val,_val); /* narrow */ \
    _mm_storel_epi64((__m128i*)(_ptr),_val);
#elif defined(VT_NEON_SUPPORTED)
#define _simd_store_si128_8byte(_val, _ptr) \
{   uint8x8_t _tmp = vqrshrn_n_u16(_val,_SIMD_STORE_8BYTE_FRAC_BITS); /* round, shift, narrow */ \
    vst1_u8((_ptr),_tmp); }
#endif

//-----------------------------------------------------------------------------
// 
// convert 8 Int16 values from fixed point to integer with rounding
//
#if defined(VT_SSE_SUPPORTED)
#define _simd_fixed2int_epi16(_val, _bits) \
    _val = _mm_add_epi16(_val,_mm_set1_epi16(1<<((((_bits)>0)?(_bits):1)-1))); \
    _val = _mm_srai_epi16(_val,((_bits)>0)?(_bits):1);
#define _simd_fixed2int_epu16(_val, _bits) \
    _val = _mm_adds_epu16(_val,_mm_set1_epi16(1<<((((_bits)>0)?(_bits):1)-1))); \
    _val = _mm_srli_epi16(_val,((_bits)>0)?(_bits):1);
#elif defined(VT_NEON_SUPPORTED)
#define _simd_fixed2int_epi16(_val, _bits) \
    _val = vrshrq_n_s16(_val,((_bits)>0)?(_bits):1);
#define _simd_fixed2int_epu16(_val, _bits) \
    _val = vrshrq_n_u16(_val,((_bits)>0)?(_bits):1);
#endif

#endif

#endif // _VT_SIMD_INCLUDED
//-----------------------------------------------------------------------------
// end