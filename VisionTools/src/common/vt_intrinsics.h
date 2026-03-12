#pragma once

#include "vt_basetypes.h"

// If you enable this warning, you'll get a "compile with /arch:AVX" warning.
// According to this article 
// http://stackoverflow.com/questions/7839925/using-avx-cpu-instructions
// AVX instructions are always generated where AVX intrinsics are used
// (I've double checked this by looking at the generated code of the 
// ConvertOp class)
// Disabling the warning tells the VC compiler that we know what we're doing 
// and not mix SSE and AVX instructions "improperly", which would result in 
// penalty cycles (that's why the warning).
#pragma warning (disable: 4752)


#if defined(__clang__)
#define VT_INTERLOCKED_INCREMENT(a) (__sync_add_and_fetch(a, 1))
#define VT_INTERLOCKED_DECREMENT(a) (__sync_add_and_fetch(a, -1))
#elif defined(_MSC_VER)
#define VT_INTERLOCKED_INCREMENT(a) InterlockedIncrement(a)
#define VT_INTERLOCKED_DECREMENT(a) InterlockedDecrement(a)
#endif


#if defined(__clang__)
#if (defined(_M_IX86) || defined(_M_AMD64) || defined(__i386) || defined(__x86_64__))
#include <x86intrin.h>
#endif
#else
#if (defined(_M_IX86) || defined(_M_AMD64))
#pragma warning (disable : 6540)
#include <intrin.h>
#pragma warning (default : 6540)
#elif (defined(_M_ARM) || defined(__ARM_NEON__))
#include <arm_neon.h>
#endif
#endif

#if defined(__clang__)
#define VT_DECLSPEC_ALIGN(nbytes) __attribute__((__aligned__(nbytes)))
#else
#define VT_DECLSPEC_ALIGN(nbytes) __declspec(align(nbytes))
#endif	

#if defined(__clang__)
#define nullptr NULL 
#endif

#if defined(__clang__)
#define VT_FORCEINLINE inline
#else
#define VT_FORCEINLINE __forceinline
#endif	


namespace vt {

#include "vt_simd.h"

#if defined(_MSC_VER)
#include "SSEonNeon.h"
#endif

#if (defined(_M_IX86) || defined(_M_AMD64) || defined(__i386) || defined(__x86_64__))

// SSE2 emulation of corresponding intrinsics from SSE4 set
#if defined(_MSC_VER)
#define SSE2_mm_extract_epi8 (x, imm)	((((imm) & 0x1) == 0) ? _mm_extract_epi16((x), (imm) >> 1) & 0xff : _mm_extract_epi16(_mm_srli_epi16((x), 8), (imm) >> 1))
#else
inline int SSE2_mm_extract_epi8(__m128i x, int imm) { return ((imm & 0x1) == 0) ? _mm_extract_epi16(x, imm >> 1) & 0xff : _mm_extract_epi16(_mm_srli_epi16(x, 8), imm >> 1); }
#endif
#define SSE2_mm_extract_epi16(x, imm)	_mm_extract_epi16((x), (imm))
#define SSE2_mm_extract_epi32(x, imm)	_mm_cvtsi128_si32(_mm_srli_si128((x), 4 * (imm)))
#if !defined(_MSC_VER)
#define SSE2_mm_extract_epf32(x, imm)	_mm_cvtss_f32((__m128)_mm_srli_si128((__m128i)(x), 4 * (imm)))
#else
#define SSE2_mm_extract_epf32(x, imm)	_mm_cvtss_f32(_mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(x), 4 * (imm))))
#endif
#ifdef _M_AMD64
#define SSE2_mm_extract_epi64(x, imm)	_mm_cvtsi128_si64(_mm_srli_si128((x), 8 * (imm)))
#else
#define SSE2_mm_extract_epi64(x, imm)	(((__int64) _mm_cvtsi128_si32(_mm_srli_si128((x), 8 * (imm) + 4)) << 32) | \
                                        (_mm_cvtsi128_si32(_mm_srli_si128((x), 8 * (imm))) & 0xffffffff))
#endif

template<bool BypassCache>
struct StoreIntrinsicSSE;

template<>
struct StoreIntrinsicSSE<false>
{
	static void StoreAligned(Byte* dst, const __m128i& A)
	{ _mm_store_si128((__m128i*)dst, A); }
	
	static void StoreAligned(UInt16* dst, const __m128i& A)
	{ _mm_store_si128((__m128i*)dst, A); }

	static void StoreAligned(float* dst, const __m128& A)
	{ _mm_store_ps(dst, A); }
};

template<>
struct StoreIntrinsicSSE<true>
{
	static void StoreAligned(Byte* dst, const __m128i& A)
	{ _mm_stream_si128((__m128i*)dst, A); }
	
	static void StoreAligned(UInt16* dst, const __m128i& A)
	{ _mm_stream_si128((__m128i*)dst, A); }

	static void StoreAligned(float* dst, const __m128& A)
	{ _mm_stream_ps(dst, A); }
};

#if defined(_MSC_VER)
template<bool BypassCache>
struct StoreIntrinsicAVX;

template<>
struct StoreIntrinsicAVX<false>
{
	template<typename T>
	static void StoreAligned(T* dst, const __m256& A)
	{ _mm256_store_ps((float*)dst, A); }	
};

template<>
struct StoreIntrinsicAVX<true>
{
	template<typename T>
	static void StoreAligned(T* dst, const __m256& A)
	{ _mm256_stream_ps((float*)dst, A); }
};
#endif

inline __m128i Load2AlignedSSE(const __m128i* p)
{ return _mm_loadl_epi64(p); }
inline __m128i Load2UnAlignedSSE(const __m128i* p)
{ return _mm_loadl_epi64(p); }

inline __m128 Load4AlignedSSE(const float* p)
{ return _mm_load_ps(p); }
inline __m128i Load4UnAlignedSSE(const __m128i* p)
{ return _mm_loadu_si128(p); }

inline __m128i Load4AlignedSSE(const __m128i* p)
{ return _mm_load_si128(p); }
inline __m128 Load4UnAlignedSSE(const float* p)
{ return _mm_loadu_ps(p); }

inline void Store2AlignedSSE(__m128i* p, const __m128i& v)
{ _mm_storel_epi64(p,v); }
inline void Store2UnAlignedSSE(__m128i* p, const __m128i& v)
{ _mm_storel_epi64(p,v); }

inline void Store4AlignedSSE(float* p, const __m128& v)
{ _mm_store_ps(p, v); }
inline void Store4UnAlignedSSE(float* p, const __m128& v)
{ _mm_storeu_ps(p, v); }

inline void Store4AlignedSSE(__m128i* p, const __m128i& v)
{ _mm_store_si128(p, v); }
inline void Store4UnAlignedSSE(__m128i* p, const __m128i& v)
{ _mm_storeu_si128(p, v); }


#if defined(_MSC_VER)
inline __m256 Load8AlignedSSE(const float* p)
{ return _mm256_load_ps(p); }
inline __m256i Load8AlignedSSE(const __m256i* p)
{ return _mm256_load_si256(p); }
inline __m256 Load8UnAlignedSSE(const float* p)
{ return _mm256_loadu_ps(p); }
inline __m256i Load8UnAlignedSSE(const __m256i* p)
{ return _mm256_loadu_si256(p); }
inline void Store8AlignedSSE(float* p, const __m256& v)
{ _mm256_store_ps(p, v); }
inline void Store8AlignedSSE(__m256i* p, const __m256i& v)
{ _mm256_store_si256(p, v); }
inline void Store8UnAlignedSSE(float* p, const __m256& v)
{ _mm256_storeu_ps(p, v); }
inline void Store8UnAlignedSSE(__m256i* p, const __m256i& v)
{ _mm256_storeu_si256(p, v); }
#endif

// macros to select Processor Specialization functions with templated store func
#define SelectPSTFunc( __cond, __ret, __f1, __fname, ... ) \
    if (__cond) { __ret = __fname<__f1##AlignedSSE>(__VA_ARGS__); } \
    else { __ret = __fname<__f1##UnAlignedSSE>(__VA_ARGS__); }
#define SelectPSTFunc2( __cond, __ret, __f1, __f2, __fname, ... ) \
    if (__cond) { __ret = __fname<__f1##AlignedSSE,__f2##AlignedSSE>(__VA_ARGS__); } \
    else { __ret = __fname<__f1##UnAlignedSSE,__f2##UnAlignedSSE>(__VA_ARGS__); }
#define SelectPSTFunc3( __cond, __ret, __f1, __f2, __f3, __fname, ... ) \
    if (__cond) { __ret = __fname<__f1##AlignedSSE,__f2##AlignedSSE,__f3##AlignedSSE>(__VA_ARGS__); } \
    else { __ret = __fname<__f1##UnAlignedSSE,__f2##UnAlignedSSE,__f3##UnAlignedSSE>(__VA_ARGS__); }
#define SelectPSTFunc2c( __cond1, __cond2, __ret, __f1, __f2, __fname, ... ) \
    if ((__cond1)&&(__cond2)) { __ret = __fname<__f1##AlignedSSE,__f2##AlignedSSE>(__VA_ARGS__); } \
    else if (__cond1) { __ret = __fname<__f1##AlignedSSE,__f2##UnAlignedSSE>(__VA_ARGS__); } \
    else if (__cond2) { __ret = __fname<__f1##UnAlignedSSE,__f2##AlignedSSE>(__VA_ARGS__); } \
    else { __ret = __fname<__f1##UnAlignedSSE,__f2##UnAlignedSSE>(__VA_ARGS__); }

#elif defined(VT_NEON_SUPPORTED)

inline uint8x8_t Load2AlignedNEON(const uint8x8_t* p)
#if defined(VT_IS_MSVC_COMPILER)
{ return vld1_u8_ex((uint8_t*)p,64); }
#else
{ return vld1_u8((uint8_t*)p); }
#endif
inline uint8x8_t Load2UnAlignedNEON(const uint8x8_t* p)
{ return vld1_u8((uint8_t*)p); }

inline float32x4_t Load4AlignedNEON(const float* p)
#if defined(VT_IS_MSVC_COMPILER)
{ return vld1q_f32_ex(p,128); }
#else
{ return vld1q_f32(p); }
#endif
inline float32x4_t Load4UnAlignedNEON(const float* p)
{ return vld1q_f32(p); }

inline uint8x16_t Load4AlignedNEON(const uint8x16_t* p)
#if defined(VT_IS_MSVC_COMPILER)
{ return vld1q_u8_ex((uint8_t*)p,128); }
#else
{ return vld1q_u8((uint8_t*)p); }
#endif
inline uint8x16_t Load4UnAlignedNEON(const uint8x16_t* p)
{ return vld1q_u8((uint8_t*)p); }

inline void Store2AlignedNEON(void* p, const uint8x8_t& v)
#if defined(VT_IS_MSVC_COMPILER)
{ vst1_u8_ex((uint8_t*)p, v, 64); }
#else
{ vst1_u8((uint8_t*)p, v); }
#endif
inline void Store2UnAlignedNEON(void* p, const uint8x8_t& v)
{ vst1_u8((uint8_t*)p, v); }

inline void Store4AlignedNEON(float* p, const float32x4_t& v)
#if defined(VT_IS_MSVC_COMPILER)
{ vst1q_f32_ex(p, v, 128); }
#else
{ vst1q_f32(p, v); }
#endif
inline void Store4UnAlignedNEON(float* p, const float32x4_t& v)
{ vst1q_f32(p, v); }

inline void Store4AlignedNEON(uint8x16_t* p, const uint8x16_t& v)
#if defined(VT_IS_MSVC_COMPILER)
{ vst1q_u8_ex((uint8_t*)p, v, 128); }
#else
{ vst1q_u8((uint8_t*)p, v); }
#endif
inline void Store4UnAlignedNEON(uint8x16_t* p, const uint8x16_t& v)
{ vst1q_u8((uint8_t*)p, v); }

// macros to select Processor Specialization functions with templated store func
#define SelectPSTFunc( __cond, __ret, __f1, __fname, ... ) \
    if (__cond) { __ret = __fname<__f1##AlignedNEON>(__VA_ARGS__); } \
    else { __ret = __fname<__f1##UnAlignedNEON>(__VA_ARGS__); }
#define SelectPSTFunc2( __cond, __ret, __f1, __f2, __fname, ... ) \
    if (__cond) { __ret = __fname<__f1##AlignedNEON,__f2##AlignedNEON>(__VA_ARGS__); } \
    else { __ret = __fname<__f1##UnAlignedNEON,__f2##UnAlignedNEON>(__VA_ARGS__); }
#define SelectPSTFunc3( __cond, __ret, __f1, __f2, __f3, __fname, ... ) \
    if (__cond) { __ret = __fname<__f1##AlignedNEON,__f2##AlignedNEON,__f3##AlignedNEON>(__VA_ARGS__); } \
    else { __ret = __fname<__f1##UnAlignedNEON,__f2##UnAlignedNEON,__f3##UnAlignedNEON>(__VA_ARGS__); }
#define SelectPSTFunc2c( __cond1, __cond2, __ret, __f1, __f2, __fname, ... ) \
    if ((__cond1)&&(__cond2)) { __ret = __fname<__f1##AlignedNEON,__f2##AlignedNEON>(__VA_ARGS__); } \
    else if (__cond1) { __ret = __fname<__f1##AlignedNEON,__f2##UnAlignedNEON>(__VA_ARGS__); } \
    else if (__cond2) { __ret = __fname<__f1##UnAlignedNEON,__f2##AlignedNEON>(__VA_ARGS__); } \
    else { __ret = __fname<__f1##UnAlignedNEON,__f2##UnAlignedNEON>(__VA_ARGS__); }

#elif defined(_M_ARM)
// can remove this when remainder of code is moved from SSEonNeon to vt_simd.h


inline __n64 Load2AlignedNEON(const __n64* p)
{ return vld1_u8_ex((uint8_t*)p,64); }
inline __n64 Load2UnAlignedNEON(const __n64* p)
{ return vld1_u8((uint8_t*)p); }

inline __m128 Load4AlignedNEON(const float* p)
{ return vld1q_f32_ex(p,128); }
inline __m128 Load4UnAlignedNEON(const float* p)
{ return vld1q_f32(p); }

inline __m128 Load4AlignedNEON(const __m128i* p)
{ return vld1q_u8_ex((uint8_t*)p,128); }
inline __m128 Load4UnAlignedNEON(const __m128i* p)
{ return vld1q_u8((uint8_t*)p); }

inline void Store2AlignedNEON(void* p, const __n64& v)
{ vst1_u8_ex((uint8_t*)p, v, 64); }
inline void Store2UnAlignedNEON(void* p, const __n64& v)
{ vst1_u8((uint8_t*)p, v); }

inline void Store4AlignedNEON(float* p, const __m128& v)
{ vst1q_f32_ex(p, v, 128); }
inline void Store4UnAlignedNEON(float* p, const __m128& v)
{ vst1q_f32(p, v); }

inline void Store4AlignedNEON(__m128i* p, const __m128i& v)
{ vst1q_u8_ex((uint8_t*)p, v, 128); }
inline void Store4UnAlignedNEON(__m128i* p, const __m128i& v)
{ vst1q_u8((uint8_t*)p, v); }

// macros to select Processor Specialization functions with templated store func
#define SelectPSTFunc( __cond, __ret, __f1, __fname, ... ) \
    if (__cond) { __ret = __fname<__f1##AlignedNEON>(__VA_ARGS__); } \
    else { __ret = __fname<__f1##UnAlignedNEON>(__VA_ARGS__); }
#define SelectPSTFunc2( __cond, __ret, __f1, __f2, __fname, ... ) \
    if (__cond) { __ret = __fname<__f1##AlignedNEON,__f2##AlignedNEON>(__VA_ARGS__); } \
    else { __ret = __fname<__f1##UnAlignedNEON,__f2##UnAlignedNEON>(__VA_ARGS__); }
#define SelectPSTFunc3( __cond, __ret, __f1, __f2, __f3, __fname, ... ) \
    if (__cond) { __ret = __fname<__f1##AlignedNEON,__f2##AlignedNEON,__f3##AlignedNEON>(__VA_ARGS__); } \
    else { __ret = __fname<__f1##UnAlignedNEON,__f2##UnAlignedNEON,__f3##UnAlignedNEON>(__VA_ARGS__); }
#define SelectPSTFunc2c( __cond1, __cond2, __ret, __f1, __f2, __fname, ... ) \
    if ((__cond1)&&(__cond2)) { __ret = __fname<__f1##AlignedNEON,__f2##AlignedNEON>(__VA_ARGS__); } \
    else if (__cond1) { __ret = __fname<__f1##AlignedNEON,__f2##UnAlignedNEON>(__VA_ARGS__); } \
    else if (__cond2) { __ret = __fname<__f1##UnAlignedNEON,__f2##AlignedNEON>(__VA_ARGS__); } \
    else { __ret = __fname<__f1##UnAlignedNEON,__f2##UnAlignedNEON>(__VA_ARGS__); }

#endif

};