//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for image 
//
//  History:
//      2011/04/26-wkienzle
//			Created
//
//------------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_image.h"
#include "vt_convert.inl"

namespace vt 
{

HRESULT PrepareUnaryImgOp(const CImg &imgSrc1, CImg &imgDst);

HRESULT PrepareBinaryImgOp(const CImg &imgSrc1, const CImg &imgSrc2, 
	CImg &imgDst);

namespace OpHelpers 
{
	// 
	// Definitions for generic span operations
	// 

	const size_t ConvertBufSize = 4096;

	template<typename T> struct HasEvalGeneric;

	template<typename OpType>
	inline void StaticAssertSingle(OpType)
	{
		static_assert(OpType::NumSrcBands >= 0,
			"Operation member has invalid value");
		static_assert(OpType::NumSrcElPerGroup >= 0, 
			"Operation member has invalid value");
		
		static_assert(HasEvalGeneric<OpType>::value == true,
			"Operation does not implement EvalGeneric()");

		static_assert(OpType::NumGroupsPerOpGeneric == 1,
			"Operation must not process more than one group in generic mode");
	}

	template<typename OpType>
	inline void StaticAssert(OpType op)
	{
		StaticAssertSingle(op);

		static_assert(OpType::NumDstBands >= 0,
			"Operation member has invalid value");
		static_assert(OpType::NumDstElPerGroup >= 0, 
			"Operation member has invalid value");

		static_assert((OpType::NumSrcBands == 0 && OpType::NumDstBands == 0) ||
			(OpType::NumSrcBands != 0 && OpType::NumDstBands != 0), 
			"Operation has inconsistent src/dst band combination");
	}

	template<bool B>
	struct BoolToType
	{
		typedef BoolToType<B> type;
		static const int value = B;	
	};

	template<typename T1, typename T2>
	struct IsSameType
	{
		static const bool value = false;
	};

	template<typename T>
	struct IsSameType<T, T> 
	{
		static const bool value = true;
	};

	// determine number of src/dst bands for a given src/dst/op triple
	// if op specifies band count (not 0), return that band count
	template<int NumOpSrcBands, int NumOpDstBands>
	inline int NumWorkingSrcBandsInt(int, int)
	{
		return NumOpSrcBands;
	}
	template<int NumOpSrcBands, int NumOpDstBands>
	inline int NumWorkingDstBandsInt(int, int)
	{
		return NumOpDstBands;
	}

	// if op doesnt care about band count, work with source band count
	template<>
	inline int NumWorkingSrcBandsInt<0,0>(int numSrcBands, int)
	{
		return numSrcBands;
	}	
	template<>
	inline int NumWorkingDstBandsInt<0,0>(int numSrcBands, int)
	{
		return numSrcBands;
	}
	
	// actual "public" function
	template<typename OpType>
	inline int NumWorkingSrcBands(int numSrcBands, int numDstBands)
	{
		return NumWorkingSrcBandsInt<OpType::NumSrcBands, OpType::NumDstBands>
			(numSrcBands, numDstBands);
	}

	template<typename OpType>
	inline int NumWorkingDstBands(int numSrcBands, int numDstBands)
	{
		return NumWorkingDstBandsInt<OpType::NumSrcBands, OpType::NumDstBands>
			(numSrcBands, numDstBands);
	}

	// does constellation of src/dst/op require conversions?
	template<typename OpType, typename SrcElType>
	inline bool NeedsSrcConversion(int numSrcBands, int numDstBands)
	{
		// prevent warning in release build
		UNREFERENCED_PARAMETER(numSrcBands);
		UNREFERENCED_PARAMETER(numDstBands);

		return !IsSameType<SrcElType, typename OpType::ImplSrcElType>::value || 
			NumWorkingSrcBands<OpType>(numSrcBands, numDstBands) != numSrcBands;
	}

	template<typename OpType, typename DstElType>
	inline bool NeedsDstConversion(int numSrcBands, int numDstBands)
	{
		// prevent warning in release build
		UNREFERENCED_PARAMETER(numSrcBands);
		UNREFERENCED_PARAMETER(numDstBands);

		return !IsSameType<DstElType, typename OpType::ImplDstElType>::value || 
			NumWorkingDstBands<OpType>(numSrcBands, numDstBands) != numDstBands;
	}

    template<int NumOpSrcBands>
    inline int NumWorkingSrcBandsSingleInt(int)
    {
        return NumOpSrcBands;
    }

    template<>
    inline int NumWorkingSrcBandsSingleInt<0>(int numSrcBands)
    {
        return numSrcBands;
    }

	// band count logic for single image functions
	template<typename OpType>
	inline int NumWorkingSrcBandsSingle(int numSrcBands)
	{
		return NumWorkingSrcBandsSingleInt<OpType::NumSrcBands>(numSrcBands);
	}

	template<typename OpType, typename SrcElType>
	inline bool NeedsSrcConversionSingle(int numSrcBands)
	{
		UNREFERENCED_PARAMETER(numSrcBands);

		return !IsSameType<SrcElType, typename OpType::ImplSrcElType>::value || 
			NumWorkingSrcBandsSingle<OpType>(numSrcBands) != numSrcBands;
	}

	template <typename TO, typename TI>
	struct BothAreStandardTypes
	{
		static const bool value = IsStandardElementType<TO>::value 
			&& IsStandardElementType<TO>::value;
		typedef typename std::conditional<value, std::true_type, std::false_type>::type
			type;
	};

	template <typename TO, typename TI>
	HRESULT ConvertSpanProxy(TO* pDst, int iDstBands, const TI* pSrc, 
		int iSrcBands, int iSrcElCount, bool bBypassCache, std::true_type)
	{
		return VtConvertSpanBands(pDst, iDstBands, pSrc, iSrcBands, 
			iSrcElCount, bBypassCache);
	}

	template <typename TO, typename TI>
	HRESULT ConvertSpanProxy(TO* pDst, int iDstBands, const TI* pSrc, 
		int iSrcBands, int iSrcElCount, bool bBypassCache, std::false_type)
	{
		VT_ASSERT(iSrcBands == iDstBands);
		UNREFERENCED_PARAMETER(iSrcBands);
		UNREFERENCED_PARAMETER(iDstBands);

		VtConvertSpan(pDst, pSrc, iSrcElCount, bBypassCache);

		return S_OK;
	}	

	///////////////////////////

	enum ArchEnum
	{
		Generic,
		SSE1,
		SSE2,
		SSE4_1,
		AVX,
        AVXwF16C,
		NEON
	};

	template<ArchEnum arch>
	struct ArchType
	{
		typedef ArchType<arch> type;
		static const ArchEnum value = arch;
	};

	// Has#### determines at compile time (via SFINAE) whether a type 
	// has a member named ####. 
	// Note: in order to work with member functions declared in base
	// classes (which have the base class in their signature, not the
	// derived class), this uses a member name ambiguity in Derived,
	// which derives from both Ambiguous and T. This means that this
	// test only checks if a member exists, not what type it is. 
#define VTOP_DECLARE_MEMBER_TEST(name) \
	template<typename T> struct Has##name { \
	struct Ambiguous { int name; }; \
		struct Derived : T, Ambiguous { }; \
		template<typename U, U> struct Helper; \
		template<typename U> static char \
			(&f(Helper<int Ambiguous::*, &U::name>*))[1]; \
		template<typename U> static char (&f(...))[2]; \
		static bool const value = sizeof(f<Derived>(0)) == 2; \
	}; 

	VTOP_DECLARE_MEMBER_TEST(InitGeneric);
	VTOP_DECLARE_MEMBER_TEST(EvalGeneric);
	VTOP_DECLARE_MEMBER_TEST(ExitGeneric);

	VTOP_DECLARE_MEMBER_TEST(InitSSE1);
	VTOP_DECLARE_MEMBER_TEST(EvalSSE1);
	VTOP_DECLARE_MEMBER_TEST(ExitSSE1);

	VTOP_DECLARE_MEMBER_TEST(InitSSE2);
	VTOP_DECLARE_MEMBER_TEST(EvalSSE2);
	VTOP_DECLARE_MEMBER_TEST(ExitSSE2);

	VTOP_DECLARE_MEMBER_TEST(InitSSE4_1);
	VTOP_DECLARE_MEMBER_TEST(EvalSSE4_1);
	VTOP_DECLARE_MEMBER_TEST(ExitSSE4_1);

	VTOP_DECLARE_MEMBER_TEST(InitAVX);
	VTOP_DECLARE_MEMBER_TEST(EvalAVX);
	VTOP_DECLARE_MEMBER_TEST(ExitAVX);

	VTOP_DECLARE_MEMBER_TEST(InitAVXwF16C);
	VTOP_DECLARE_MEMBER_TEST(EvalAVXwF16C);
	VTOP_DECLARE_MEMBER_TEST(ExitAVXwF16C);

	VTOP_DECLARE_MEMBER_TEST(InitNEON);
	VTOP_DECLARE_MEMBER_TEST(EvalNEON);
	VTOP_DECLARE_MEMBER_TEST(ExitNEON);
	
	// BestImpl's member indicates the best available implementation 
	// for a given operation and system architecture 
	template<typename OpType, ArchEnum Arch>
	struct BestImpl
	{
		typedef ArchType<Generic> type;
	};

#define VTOP_DECLARE_BEST_IMPL(arch, fallback) \
	template<typename OpType> \
	struct BestImpl<OpType, arch> \
	{ \
	typedef typename BestImpl<OpType, fallback>::type BIT; \
	typedef ArchType<HasEval##arch<OpType>::value ? arch : \
	BIT::value> type; \
	}; 
	
	// BestImpl traits can be recursively defined
	VTOP_DECLARE_BEST_IMPL(SSE1, Generic);
	VTOP_DECLARE_BEST_IMPL(SSE2, SSE1);
	VTOP_DECLARE_BEST_IMPL(SSE4_1, SSE2);
	VTOP_DECLARE_BEST_IMPL(AVX, SSE4_1);
	VTOP_DECLARE_BEST_IMPL(AVXwF16C, AVX);
	VTOP_DECLARE_BEST_IMPL(NEON, Generic);

	// Stepsize helpers
	template<typename OpType, ArchEnum arch>
	struct SrcStepSize;
	template<typename OpType, ArchEnum arch>
	struct DstStepSize;

#define VTOP_DECLARE_STEP_SIZE(arch) \
	template<typename OpType> \
	struct SrcStepSize<OpType, arch> \
	{ \
	static const int value = OpType::NumSrcElPerGroup * \
	OpType::NumGroupsPerOp##arch; \
	}; \
	template<typename OpType> \
	struct DstStepSize<OpType, arch> \
	{ \
	static const int value = OpType::NumDstElPerGroup * \
	OpType::NumGroupsPerOp##arch; \
	};

	VTOP_DECLARE_STEP_SIZE(Generic);
	VTOP_DECLARE_STEP_SIZE(SSE1);
	VTOP_DECLARE_STEP_SIZE(SSE2);
	VTOP_DECLARE_STEP_SIZE(SSE4_1);
	VTOP_DECLARE_STEP_SIZE(AVX);
	VTOP_DECLARE_STEP_SIZE(AVXwF16C);
	VTOP_DECLARE_STEP_SIZE(NEON);

	template<typename OpType, ArchEnum arch>
	struct SrcStepBytes
	{ 
		static const size_t value = 
			(SrcStepSize<OpType, arch>::value *
			sizeof(typename OpType::ImplSrcElType));
	};

	template<typename OpType, ArchEnum arch>
	struct DstStepBytes
	{ 
		static const size_t value = 
			(DstStepSize<OpType, arch>::value *
			sizeof(typename OpType::ImplDstElType));
	};

	// Helpers for computing the end of a span for safe access using
	// operation-specific step size and architecture-specific load/store size
	// Rule: the operation is safe to access up to the smallest integer multiple 
	// of LoadStoreGranularity that is larger than the stepsize (in bytes)
	template<ArchEnum arch>
	struct LoadStoreGranularity;

#define VTOP_DECLARE_LOAD_STORE_GRANULARITY(arch, bytes) \
	template<> struct LoadStoreGranularity<arch> \
	{ static const size_t value = bytes; };
	
	VTOP_DECLARE_LOAD_STORE_GRANULARITY(Generic, 1);
	VTOP_DECLARE_LOAD_STORE_GRANULARITY(SSE1, 16);
	VTOP_DECLARE_LOAD_STORE_GRANULARITY(SSE2, 16);
	VTOP_DECLARE_LOAD_STORE_GRANULARITY(SSE4_1, 16);
	VTOP_DECLARE_LOAD_STORE_GRANULARITY(AVX, 32);
	VTOP_DECLARE_LOAD_STORE_GRANULARITY(AVXwF16C, 32);
	VTOP_DECLARE_LOAD_STORE_GRANULARITY(NEON, 16);

	template<ArchEnum arch>
	bool IsArchAligned(const void* p)
	{
		return (reinterpret_cast<intptr_t>(p) & 
			(LoadStoreGranularity<arch>::value-1)) == 0;
	};

	template<size_t v, size_t base>
	struct NextMultipleOf
	{
		static const size_t value = (v % base > 0) ? (1 + v/base) * base : v;
	};

	template<typename OpType, ArchEnum arch>
	struct SrcEndSpanMargin
	{
		static const size_t bytes = 
			NextMultipleOf<SrcStepBytes<OpType, arch>::value, 
			LoadStoreGranularity<arch>::value>::value;
		static const size_t steps = 
			NextMultipleOf<bytes, SrcStepBytes<OpType, arch>::value>::value
			/ SrcStepBytes<OpType, arch>::value;
		static const size_t elements = steps * SrcStepSize<OpType, arch>::value;
	};

	template<typename OpType, ArchEnum arch>
	struct DstEndSpanMargin
	{
		static const size_t bytes = 
			NextMultipleOf<DstStepBytes<OpType, arch>::value, 
			LoadStoreGranularity<arch>::value>::value;
		static const size_t steps = 
			NextMultipleOf<bytes, DstStepBytes<OpType, arch>::value>::value			
			/ DstStepBytes<OpType, arch>::value;
		static const size_t elements = steps * DstStepSize<OpType, arch>::value;
	};

	// Init helpers
	// Init() calls the architecture specific init routine if it exists
#define VTOP_DECLARE_INIT(arch) \
	template<typename OpType> \
	void Init(OpType op, typename OpType::ParamType* params, \
	typename OpType::TmpType& tmp, ArchType<arch> a) \
	{ \
	Init(op, params, tmp, a, BoolToType<HasInit##arch<OpType>::value>()); \
	} \
	template<typename OpType> \
	void Init(OpType op, typename OpType::ParamType* params, \
	typename OpType::TmpType& tmp, ArchType<arch>, BoolToType<true>) \
	{ \
	op.Init##arch(params, tmp); \
	} \
	template<typename OpType> \
	void Init(OpType, typename OpType::ParamType*, \
	typename OpType::TmpType&, ArchType<arch>, BoolToType<false>) \
	{ \
	} 

	VTOP_DECLARE_INIT(Generic);
	VTOP_DECLARE_INIT(SSE1);
	VTOP_DECLARE_INIT(SSE2);
	VTOP_DECLARE_INIT(SSE4_1);
	VTOP_DECLARE_INIT(AVX);
	VTOP_DECLARE_INIT(AVXwF16C);
	VTOP_DECLARE_INIT(NEON);

	// Exit helpers
	// Exit() calls the architecture specific init routine if it exists
#define VTOP_DECLARE_EXIT(arch) \
	template<typename OpType> \
	void Exit(OpType op, typename OpType::ParamType* params, \
	typename OpType::TmpType& tmp, ArchType<arch> a) \
	{ \
	Exit(op, params, tmp, a, BoolToType<HasExit##arch<OpType>::value>()); \
	} \
	template<typename OpType> \
	void Exit(OpType op, typename OpType::ParamType* params, \
	typename OpType::TmpType& tmp, ArchType<arch>, BoolToType<true>) \
	{ \
	op.Exit##arch(params, tmp); \
	} \
	template<typename OpType> \
	void Exit(OpType, typename OpType::ParamType*, \
	typename OpType::TmpType&, ArchType<arch>, BoolToType<false>) \
	{ \
	} 

	VTOP_DECLARE_EXIT(Generic);
	VTOP_DECLARE_EXIT(SSE1);
	VTOP_DECLARE_EXIT(SSE2);
	VTOP_DECLARE_EXIT(SSE4_1);
	VTOP_DECLARE_EXIT(AVX);
	VTOP_DECLARE_EXIT(AVXwF16C);
	VTOP_DECLARE_EXIT(NEON);

	// Eval helpers
#define VTOP_DECLARE_EVAL_STATS(arch) \
	template<typename StatsOpType> \
	void EvalStats(StatsOpType statsOp, \
	const typename StatsOpType::ImplSrcElType* srcPtr, \
	typename StatsOpType::ParamType* params, \
	typename StatsOpType::TmpType& tmp, ArchType<arch>) \
	{ \
	statsOp.Eval##arch(srcPtr, params, tmp); \
	}

	VTOP_DECLARE_EVAL_STATS(Generic);
	VTOP_DECLARE_EVAL_STATS(SSE1);
	VTOP_DECLARE_EVAL_STATS(SSE2);
	VTOP_DECLARE_EVAL_STATS(SSE4_1);
	VTOP_DECLARE_EVAL_STATS(AVX);
	VTOP_DECLARE_EVAL_STATS(AVXwF16C);
	VTOP_DECLARE_EVAL_STATS(NEON);

#define VTOP_DECLARE_EVAL_UNARY(arch) \
	template<typename UnaryOpType> \
	void EvalUnary(UnaryOpType unaryOp, \
	const typename UnaryOpType::ImplSrcElType* srcPtr, \
	typename UnaryOpType::ImplDstElType* dstPtr, \
	typename UnaryOpType::ParamType* params, \
	typename UnaryOpType::TmpType& tmp, ArchType<arch>) \
	{ \
	unaryOp.Eval##arch(srcPtr, dstPtr, params, tmp); \
	}

	VTOP_DECLARE_EVAL_UNARY(Generic);
	VTOP_DECLARE_EVAL_UNARY(SSE1);
	VTOP_DECLARE_EVAL_UNARY(SSE2);
	VTOP_DECLARE_EVAL_UNARY(SSE4_1);
	VTOP_DECLARE_EVAL_UNARY(AVX);
	VTOP_DECLARE_EVAL_UNARY(AVXwF16C);
	VTOP_DECLARE_EVAL_UNARY(NEON);

#define VTOP_DECLARE_EVAL_BINARY(arch) \
	template<typename BinaryOpType> \
	void EvalBinary(BinaryOpType binaryOp, \
	const typename BinaryOpType::ImplSrcElType* srcPtr1, \
	const typename BinaryOpType::ImplSrcElType* srcPtr2, \
	typename BinaryOpType::ImplDstElType* dstPtr, \
	typename BinaryOpType::ParamType* params, \
	typename BinaryOpType::TmpType& tmp, ArchType<arch>) \
	{ \
	binaryOp.Eval##arch(srcPtr1, srcPtr2, dstPtr, params, tmp); \
	}

	VTOP_DECLARE_EVAL_BINARY(Generic);
	VTOP_DECLARE_EVAL_BINARY(SSE1);
	VTOP_DECLARE_EVAL_BINARY(SSE2);
	VTOP_DECLARE_EVAL_BINARY(SSE4_1);
	VTOP_DECLARE_EVAL_BINARY(AVX);
	VTOP_DECLARE_EVAL_BINARY(AVXwF16C);
	VTOP_DECLARE_EVAL_BINARY(NEON);
};

////////////////////////////////////////////////////////////////////////////////
//
//				STATS IMAGE OPERATION
//
////////////////////////////////////////////////////////////////////////////////

template<OpHelpers::ArchEnum ImplAT, typename StatsOpType>
void StatsSpanOpInternal(
	const typename StatsOpType::ImplSrcElType* srcPtr, 
	const typename StatsOpType::ImplSrcElType* srcEnd, 
	StatsOpType statsOp, 
	typename StatsOpType::ParamType* params)
{
	typename StatsOpType::TmpType tmp;
	
	// 1) process until dst is aligned
	const size_t srcStepGeneric = 
		OpHelpers::SrcStepSize<StatsOpType, OpHelpers::Generic>::value;

	OpHelpers::Init(statsOp, params, tmp, 
		OpHelpers::ArchType<OpHelpers::Generic>());
	
	for (; srcPtr < srcEnd && !OpHelpers::IsArchAligned<ImplAT>(srcPtr); 
		srcPtr += srcStepGeneric)
	{
		statsOp.EvalGeneric(srcPtr, params, tmp);
	}

	OpHelpers::Exit(statsOp, params, tmp, 
		OpHelpers::ArchType<OpHelpers::Generic>());

	// 2) process with ImplAT	
	VT_ASSERT(OpHelpers::IsArchAligned<ImplAT>(srcPtr) || srcEnd == srcPtr);

	const size_t srcStepImplAT = 
		OpHelpers::SrcStepSize<StatsOpType, ImplAT>::value;
	
	OpHelpers::Init(statsOp, params, tmp, 
		OpHelpers::ArchType<ImplAT>());
	
	const typename StatsOpType::ImplSrcElType* srcEndSafe = srcEnd - 
		(OpHelpers::SrcEndSpanMargin<StatsOpType, ImplAT>::steps
		* srcStepImplAT - 1);

	for (; srcPtr < srcEndSafe; srcPtr += srcStepImplAT)
	{	
		OpHelpers::EvalStats(statsOp, srcPtr, params, tmp,
			OpHelpers::ArchType<ImplAT>());
	}

	OpHelpers::Exit(statsOp, params, tmp, 
		OpHelpers::ArchType<ImplAT>());

	// 3) process remaining pixels
	OpHelpers::Init(statsOp, params, tmp, 
		OpHelpers::ArchType<OpHelpers::Generic>());

	for (; srcPtr < srcEnd; srcPtr += srcStepGeneric)
	{
		statsOp.EvalGeneric(srcPtr, params, tmp);
	}

	OpHelpers::Exit(statsOp, params, tmp, 
		OpHelpers::ArchType<OpHelpers::Generic>());

	VT_ASSERT(srcPtr == srcEnd);
}

#if defined(VT_NEON_SUPPORTED)
#define VTOP_ARCH_SWITCH_STATS(opType, srcPtr, srcEnd, op, params) \
	StatsSpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::NEON>::type::value>(srcPtr, \
	srcEnd, op, params); 
#else
#define VTOP_ARCH_SWITCH_STATS(opType, srcPtr, srcEnd, op, params) \
	if (g_SupportAVXwF16C()) \
	StatsSpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::AVXwF16C>::type::value>(srcPtr, \
	srcEnd, op, params); \
	else if (g_SupportAVX()) \
	StatsSpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::AVX>::type::value>(srcPtr, \
	srcEnd, op, params); \
	else if (g_SupportSSE4_1()) \
	StatsSpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::SSE4_1>::type::value>(srcPtr, \
	srcEnd, op, params); \
	else if (g_SupportSSE2()) \
	StatsSpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::SSE2>::type::value>(srcPtr, \
	srcEnd, op, params); \
	else if (g_SupportSSE1()) \
	StatsSpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::SSE1>::type::value>(srcPtr, \
	srcEnd, op, params); \
	else \
	StatsSpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::Generic>::type::value>(srcPtr, \
	srcEnd, op, params);
#endif

template<typename SrcElType, typename StatsOpType>
HRESULT StatsSpanOp(const SrcElType* pSrc, int numSrcBands, 
	int numSpanPixels, StatsOpType statsOp, 
	typename StatsOpType::ParamType* params)
{
	typedef typename StatsOpType::ImplSrcElType IST;
	
	OpHelpers::StaticAssertSingle(statsOp);

	VT_HR_BEGIN();	

	// find effective number of bands for this operation
	const int numWorkingSrcBands = 
		OpHelpers::NumWorkingSrcBandsSingle<StatsOpType>(numSrcBands);

	// compute number of pixels per block
	const int numPixelsPerBlock = OpHelpers::ConvertBufSize / 
		(numWorkingSrcBands * sizeof(IST));
	
	VT_ASSERT(numPixelsPerBlock > 0);

	VT_DECLSPEC_ALIGN(32) Byte srcBuf[OpHelpers::ConvertBufSize];

	// do blocks
	int numPixelsDone = 0;
	while (numPixelsDone < numSpanPixels)
	{
		const int numPixelsToDo = VtMin(numPixelsPerBlock, numSpanPixels - 
			numPixelsDone);

		const IST* srcPtr = NULL;
		// convert source if necessary
		if (OpHelpers::NeedsSrcConversionSingle<StatsOpType, SrcElType>(
			numSrcBands))
		{
			VT_HR_EXIT(OpHelpers::ConvertSpanProxy(
				reinterpret_cast<IST*>(srcBuf),
				numWorkingSrcBands, pSrc + numPixelsDone * numSrcBands, 
				numSrcBands, numPixelsToDo * numSrcBands, false, 
				typename OpHelpers::BothAreStandardTypes<SrcElType, IST>::type()));

			srcPtr = reinterpret_cast<const IST*>(srcBuf);
		}
		else
		{
			srcPtr = reinterpret_cast<const IST*>(pSrc + 
				numPixelsDone * numSrcBands);
		}

		VT_ASSERT(srcPtr != NULL);		
		
		VTOP_ARCH_SWITCH_STATS(StatsOpType, srcPtr, 
			reinterpret_cast<const IST*>(srcPtr) + numPixelsToDo * 
			numWorkingSrcBands, statsOp, params);

		numPixelsDone += numPixelsToDo;
	}

	VT_HR_END();
}

#undef VTOP_ARCH_SWITCH_STATS

template<template<typename> class StatsOpType, 
	typename SrcType, typename ParamType>
	HRESULT StatsImgOpS(const CImg& src, ParamType* params)
{
	typedef StatsOpType<SrcType> OT;
	
	static_assert(OpHelpers::IsSameType<ParamType, 
		typename OT::ParamType>::value, "ParamType mismatch");

	VT_HR_BEGIN();

	for (int y = 0; y < src.Height(); ++y)
	{ 
		VT_HR_EXIT(StatsSpanOp(
			reinterpret_cast<const SrcType*>(src.BytePtr(y)),
			src.Bands(), src.Width(), OT(), params));
	}

	VT_HR_END();
}

template<template<typename> class StatsOpType, 
	typename ParamType>
	HRESULT StatsImgOpD(const CImg& src, ParamType* p)
{
	switch (EL_FORMAT(src.GetType()))
	{
	case EL_FORMAT_BYTE:
		return StatsImgOpS<StatsOpType, Byte>(src, p);
	case EL_FORMAT_SHORT:
		return StatsImgOpS<StatsOpType, UInt16>(src, p);
	case EL_FORMAT_FLOAT:
		return StatsImgOpS<StatsOpType, float>(src, p);
	case EL_FORMAT_HALF_FLOAT:
		return StatsImgOpS<StatsOpType, HALF_FLOAT>(src, p);
	default: 
		return E_NOTIMPL;
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//				UNARY IMAGE OPERATION
//
////////////////////////////////////////////////////////////////////////////////

template<OpHelpers::ArchEnum ImplAT, typename UnaryOpType>
void UnarySpanOpInternal(
	const typename UnaryOpType::ImplSrcElType* srcPtr, 
	typename UnaryOpType::ImplDstElType* dstPtr, 
	const typename UnaryOpType::ImplDstElType* dstEnd, 
	UnaryOpType unaryOp, 
	typename UnaryOpType::ParamType* params)
{
	VT_ASSERT((dstEnd - dstPtr) % UnaryOpType::NumDstElPerGroup == 0);

	typename UnaryOpType::TmpType tmp;
	
	// 1) process until dst is aligned
	const size_t srcStepGeneric = 
		OpHelpers::SrcStepSize<UnaryOpType, OpHelpers::Generic>::value;
	const size_t dstStepGeneric = 
		OpHelpers::DstStepSize<UnaryOpType, OpHelpers::Generic>::value;

	OpHelpers::Init(unaryOp, params, tmp, 
		OpHelpers::ArchType<OpHelpers::Generic>());
	
	for (; dstPtr < dstEnd && !OpHelpers::IsArchAligned<ImplAT>(dstPtr); 
		srcPtr += srcStepGeneric, 
		dstPtr += dstStepGeneric)
	{
		unaryOp.EvalGeneric(srcPtr, dstPtr, params, tmp);
	}

	OpHelpers::Exit(unaryOp, params, tmp, 
		OpHelpers::ArchType<OpHelpers::Generic>());

	VT_ASSERT(OpHelpers::IsArchAligned<ImplAT>(dstPtr) || dstEnd == dstPtr);

	// 2) process with ImplAT
	const size_t srcStepImplAT = 
		OpHelpers::SrcStepSize<UnaryOpType, ImplAT>::value;
	const size_t dstStepImplAT = 
		OpHelpers::DstStepSize<UnaryOpType, ImplAT>::value;
	
	OpHelpers::Init(unaryOp, params, tmp, 
		OpHelpers::ArchType<ImplAT>());

	const typename UnaryOpType::ImplDstElType* dstEndSafe = dstEnd - 
		(VtMax(OpHelpers::SrcEndSpanMargin<UnaryOpType, ImplAT>::steps,
		OpHelpers::DstEndSpanMargin<UnaryOpType, ImplAT>::steps) 
		* dstStepImplAT - 1);

	for (; dstPtr < dstEndSafe; srcPtr += srcStepImplAT, 
		dstPtr += dstStepImplAT)
	{	
		OpHelpers::EvalUnary(unaryOp, srcPtr, dstPtr, params, tmp,
			OpHelpers::ArchType<ImplAT>());
	}

	OpHelpers::Exit(unaryOp, params, tmp, 
		OpHelpers::ArchType<ImplAT>());

	// 3) process remaining pixels
	OpHelpers::Init(unaryOp, params, tmp, 
		OpHelpers::ArchType<OpHelpers::Generic>());

	for (; dstPtr < dstEnd; srcPtr += srcStepGeneric, 
		dstPtr += dstStepGeneric)
	{
		unaryOp.EvalGeneric(srcPtr, dstPtr, params, tmp);
	}

	OpHelpers::Exit(unaryOp, params, tmp, 
		OpHelpers::ArchType<OpHelpers::Generic>());

	VT_ASSERT(dstPtr == dstEnd);
}

#if defined(VT_NEON_SUPPORTED)
#define VTOP_ARCH_SWITCH_UNARY(opType, srcPtr, dstPtr, dstEnd, op, params) \
	UnarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::NEON>::type::value>(srcPtr, \
	dstPtr, dstEnd, op, params); 
#else
#define VTOP_ARCH_SWITCH_UNARY(opType, srcPtr, dstPtr, dstEnd, op, params) \
	if (g_SupportAVXwF16C()) \
	UnarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::AVXwF16C>::type::value>(srcPtr, \
	dstPtr, dstEnd, op, params); \
	else if (g_SupportAVX()) \
	UnarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::AVX>::type::value>(srcPtr, \
	dstPtr, dstEnd, op, params); \
	else if (g_SupportSSE4_1()) \
	UnarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::SSE4_1>::type::value>(srcPtr, \
	dstPtr, dstEnd, op, params); \
	else if (g_SupportSSE2()) \
	UnarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::SSE2>::type::value>(srcPtr, \
	dstPtr, dstEnd, op, params); \
	else if (g_SupportSSE1()) \
	UnarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::SSE1>::type::value>(srcPtr, \
	dstPtr, dstEnd, op, params); \
	else \
	UnarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::Generic>::type::value>(srcPtr, \
	dstPtr, dstEnd, op, params);
#endif

template<typename SrcElType, typename DstElType, typename UnaryOpType>
HRESULT UnarySpanOp(const SrcElType* pSrc, int numSrcBands, DstElType* pDst, 
	int numDstBands, int numSpanPixels, UnaryOpType unaryOp, 
	typename UnaryOpType::ParamType* params = nullptr)
{
	typedef typename UnaryOpType::ImplSrcElType IST;
	typedef typename UnaryOpType::ImplDstElType IDT;

	OpHelpers::StaticAssert(unaryOp);

	VT_HR_BEGIN();	

	// find effective number of bands for this operation
	const int numWorkingSrcBands = 
		OpHelpers::NumWorkingSrcBands<UnaryOpType>(numSrcBands, numDstBands);
	const int numWorkingDstBands = 
		OpHelpers::NumWorkingDstBands<UnaryOpType>(numSrcBands, numDstBands);

	// compute number of pixels per block
	const int numPixelsPerBlock = 
		static_cast<int>(VtMin(OpHelpers::ConvertBufSize / 
		(numWorkingSrcBands * sizeof(IST)), OpHelpers::ConvertBufSize / 
		(numWorkingDstBands * sizeof(IDT))));

	VT_ASSERT(numPixelsPerBlock > 0);

	VT_DECLSPEC_ALIGN(32) Byte srcBuf[OpHelpers::ConvertBufSize];
	VT_DECLSPEC_ALIGN(32) Byte dstBuf[OpHelpers::ConvertBufSize];

	// do blocks
	int numPixelsDone = 0;
	while (numPixelsDone < numSpanPixels)
	{
		const int numPixelsToDo = VtMin(numPixelsPerBlock, numSpanPixels - 
			numPixelsDone);

		const IST* srcPtr = NULL;
		// convert source if necessary (overwrites srcPtr/2)
		if (OpHelpers::NeedsSrcConversion<UnaryOpType, SrcElType>(numSrcBands, 
			numDstBands))
		{
			VT_HR_EXIT(OpHelpers::ConvertSpanProxy(
				reinterpret_cast<IST*>(srcBuf),
				numWorkingSrcBands, pSrc + numPixelsDone * numSrcBands,
				numSrcBands, numPixelsToDo * numSrcBands, false,
				typename OpHelpers::BothAreStandardTypes<SrcElType, IST>::type()));

			srcPtr = reinterpret_cast<const IST*>(srcBuf);
		}
		else
		{
			srcPtr = reinterpret_cast<const IST*>(pSrc + 
				numPixelsDone * numSrcBands);
		}

		VT_ASSERT(srcPtr != NULL);		
		
		// apply operation and convert destination if necessary 
		if (OpHelpers::NeedsDstConversion<UnaryOpType, DstElType>(numSrcBands, 
			numDstBands))
		{
			VTOP_ARCH_SWITCH_UNARY(UnaryOpType, srcPtr, 
				reinterpret_cast<IDT*>(dstBuf), 
				reinterpret_cast<IDT*>(dstBuf) + numPixelsToDo * 
				numWorkingDstBands, unaryOp, params);

			VT_HR_EXIT(OpHelpers::ConvertSpanProxy(pDst + numPixelsDone * numDstBands,
				numDstBands, reinterpret_cast<IDT*>(dstBuf), 
				numWorkingDstBands, numPixelsToDo * numWorkingDstBands, false,
				typename OpHelpers::BothAreStandardTypes<IDT, DstElType>::type()));
		}
		else
		{
			VTOP_ARCH_SWITCH_UNARY(UnaryOpType, srcPtr, 
				reinterpret_cast<IDT*>(pDst + numPixelsDone * 
				numDstBands), reinterpret_cast<IDT*>(pDst + 
				(numPixelsDone + numPixelsToDo) * numDstBands), unaryOp, 
				params);
		}

		numPixelsDone += numPixelsToDo;
	}

	VT_HR_END();
}

#undef VTOP_ARCH_SWITCH_UNARY

template<template<typename, typename> class UnaryOpType, 
	typename SrcType, typename DstType, typename ParamType>
	HRESULT UnaryImgOpSS(const CImg& src, CImg& dst, 
	ParamType* params = nullptr)
{
	typedef UnaryOpType<SrcType, DstType> OT;
		
	static_assert(OpHelpers::IsSameType<ParamType, 
		typename OT::ParamType>::value, "ParamType mismatch");

	VT_HR_BEGIN();

	for (int y = 0; y < dst.Height(); ++y)
	{ 
		VT_HR_EXIT(UnarySpanOp(
			reinterpret_cast<const SrcType*>(src.BytePtr(y)),
			src.Bands(), reinterpret_cast<DstType*>(dst.BytePtr(y)),
			dst.Bands(), dst.Width(), OT(), params));
	}

	VT_HR_END();
}

template<template<typename, typename> class UnaryOpType, 
	typename SrcType, typename ParamType>
	HRESULT UnaryImgOpSD(const CImg& src, CImg& dst, ParamType* p = nullptr)
{
	switch (EL_FORMAT(dst.GetType()))
	{
	case EL_FORMAT_BYTE: 
		return UnaryImgOpSS<UnaryOpType, SrcType, Byte>(src, dst, p);
	case EL_FORMAT_SHORT:
		return UnaryImgOpSS<UnaryOpType, SrcType, UInt16>(src, dst, p);
	case EL_FORMAT_FLOAT:
		return UnaryImgOpSS<UnaryOpType, SrcType, float>(src, dst, p);
	case EL_FORMAT_HALF_FLOAT:
		return UnaryImgOpSS<UnaryOpType, SrcType, HALF_FLOAT>(src, dst, p);
	default:
		return E_NOTIMPL;
	}
}

template<template<typename, typename> class UnaryOpType, 
	typename ParamType>
	HRESULT UnaryImgOpDD(const CImg& src, CImg& dst, ParamType* p = nullptr)
{
	VT_HR_BEGIN();

	VT_HR_EXIT(PrepareUnaryImgOp(src, dst));

	switch (EL_FORMAT(src.GetType()))
	{
	case EL_FORMAT_BYTE:
		return UnaryImgOpSD<UnaryOpType, Byte>(src, dst, p);
	case EL_FORMAT_SHORT:
		return UnaryImgOpSD<UnaryOpType, UInt16>(src, dst, p);
	case EL_FORMAT_FLOAT:
		return UnaryImgOpSD<UnaryOpType, float>(src, dst, p);
	case EL_FORMAT_HALF_FLOAT:
		return UnaryImgOpSD<UnaryOpType, HALF_FLOAT>(src, dst, p);
	default: 
		return E_NOTIMPL;
	}

	VT_HR_END();
}

template<template<typename, typename> class UnaryOpType>
HRESULT UnaryImgOpDD(const CImg& src, CImg& dst)
{
	return UnaryImgOpDD<UnaryOpType>(src, dst, static_cast<int*>(nullptr));
}

////////////////////////////////////////////////////////////////////////////////
//
//				BINARY IMAGE OPERATION
//
////////////////////////////////////////////////////////////////////////////////

template<OpHelpers::ArchEnum ImplAT, typename BinaryOpType>
void BinarySpanOpInternal(
	const typename BinaryOpType::ImplSrcElType* srcPtr1, 
	const typename BinaryOpType::ImplSrcElType* srcPtr2, 
	typename BinaryOpType::ImplDstElType* dstPtr, 
	const typename BinaryOpType::ImplDstElType* dstEnd, 
	BinaryOpType binaryOp, 
	typename BinaryOpType::ParamType* params)
{
	typename BinaryOpType::TmpType tmp;	
	
	// 1) process until dst is aligned
	const size_t srcStepGeneric = 
		OpHelpers::SrcStepSize<BinaryOpType, OpHelpers::Generic>::value;
	const size_t dstStepGeneric = 
		OpHelpers::DstStepSize<BinaryOpType, OpHelpers::Generic>::value;

	OpHelpers::Init(binaryOp, params, tmp, 
		OpHelpers::ArchType<OpHelpers::Generic>());
	
	for (; dstPtr < dstEnd && !OpHelpers::IsArchAligned<ImplAT>(dstPtr); 
		srcPtr1 += srcStepGeneric, srcPtr2 += srcStepGeneric, 
		dstPtr += dstStepGeneric)
	{
		binaryOp.EvalGeneric(srcPtr1, srcPtr2, dstPtr, params, tmp);
	}

	OpHelpers::Exit(binaryOp, params, tmp, 
		OpHelpers::ArchType<OpHelpers::Generic>());

	VT_ASSERT(OpHelpers::IsArchAligned<ImplAT>(dstPtr) || dstEnd == dstPtr);

	// 2) process with ImplAT
	const size_t srcStepImplAT = 
		OpHelpers::SrcStepSize<BinaryOpType, ImplAT>::value;
	const size_t dstStepImplAT = 
		OpHelpers::DstStepSize<BinaryOpType, ImplAT>::value;

	OpHelpers::Init(binaryOp, params, tmp, 
		OpHelpers::ArchType<ImplAT>());
	
	const typename BinaryOpType::ImplDstElType* dstEndSafe = dstEnd - 
		(VtMax(OpHelpers::SrcEndSpanMargin<BinaryOpType, ImplAT>::steps,
		OpHelpers::DstEndSpanMargin<BinaryOpType, ImplAT>::steps) 
		* dstStepImplAT - 1);

	for (; dstPtr < dstEndSafe; srcPtr1 += srcStepImplAT, 
		srcPtr2 += srcStepImplAT, dstPtr += dstStepImplAT)
	{
		OpHelpers::EvalBinary(binaryOp, srcPtr1, srcPtr2, dstPtr, params, tmp,
			OpHelpers::ArchType<ImplAT>());
	}

	OpHelpers::Exit(binaryOp, params, tmp, 
		OpHelpers::ArchType<ImplAT>());

	// 3) process remaining pixels
	OpHelpers::Init(binaryOp, params, tmp, 
		OpHelpers::ArchType<OpHelpers::Generic>());

	for (; dstPtr < dstEnd; 
		srcPtr1 += srcStepGeneric, srcPtr2 += srcStepGeneric, 
		dstPtr += dstStepGeneric)
	{
		binaryOp.EvalGeneric(srcPtr1, srcPtr2, dstPtr, params, tmp);
	}

	OpHelpers::Exit(binaryOp, params, tmp, 
		OpHelpers::ArchType<OpHelpers::Generic>());

	VT_ASSERT(dstPtr == dstEnd);
}


#if defined(VT_NEON_SUPPORTED)
#define VTOP_ARCH_SWITCH_BINARY(opType, s1, s2, dstPtr, dstEnd, op, params) \
	BinarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::NEON>::type::value>(s1, s2, \
	dstPtr, dstEnd, op, params);
#else
#define VTOP_ARCH_SWITCH_BINARY(opType, s1, s2, dstPtr, dstEnd, op, params) \
	if (g_SupportAVXwF16C()) \
	BinarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::AVXwF16C>::type::value>(s1, s2, \
	dstPtr, dstEnd, op, params); \
	else if (g_SupportAVX()) \
	BinarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::AVX>::type::value>(s1, s2, \
	dstPtr, dstEnd, op, params); \
	else if (g_SupportSSE4_1()) \
	BinarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::SSE4_1>::type::value>(s1, s2, \
	dstPtr, dstEnd, op, params); \
	else if (g_SupportSSE2()) \
	BinarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::SSE2>::type::value>(s1, s2, \
	dstPtr, dstEnd, op, params); \
	else if (g_SupportSSE1()) \
	BinarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::SSE1>::type::value>(s1, s2, \
	dstPtr, dstEnd, op, params); \
	else \
	BinarySpanOpInternal< \
	OpHelpers::BestImpl<opType, OpHelpers::Generic>::type::value>(s1, s2, \
	dstPtr, dstEnd, op, params);
#endif

template<typename SrcElType, typename DstElType, typename BinaryOpType>
HRESULT BinarySpanOp(const SrcElType* pSrc1, const SrcElType* pSrc2, 
	int numSrcBands, DstElType* pDst, int numDstBands, 
	int numSpanPixels, BinaryOpType binaryOp, 
	typename BinaryOpType::ParamType* params = nullptr)
{
	typedef typename BinaryOpType::ImplSrcElType IST;
	typedef typename BinaryOpType::ImplDstElType IDT;

	OpHelpers::StaticAssert(binaryOp);

	VT_HR_BEGIN();	

	// find effective number of bands for this operation
	const int numWorkingSrcBands = 
		OpHelpers::NumWorkingSrcBands<BinaryOpType>(numSrcBands, numDstBands);
	const int numWorkingDstBands = 
		OpHelpers::NumWorkingDstBands<BinaryOpType>(numSrcBands, numDstBands);

	// compute number of pixels per block
	const int numPixelsPerBlock = 
		static_cast<int>(VtMin(OpHelpers::ConvertBufSize / 
		(numWorkingSrcBands * sizeof(IST)),
		OpHelpers::ConvertBufSize / 
		(numWorkingDstBands * sizeof(IDT))));

	VT_ASSERT(numPixelsPerBlock > 0);

	VT_DECLSPEC_ALIGN(32) Byte srcBuf1[OpHelpers::ConvertBufSize];
	VT_DECLSPEC_ALIGN(32) Byte srcBuf2[OpHelpers::ConvertBufSize];
	VT_DECLSPEC_ALIGN(32) Byte dstBuf[OpHelpers::ConvertBufSize];

	// do blocks
	int numPixelsDone = 0;
	while (numPixelsDone < numSpanPixels)
	{
		const int numPixelsToDo = VtMin(numPixelsPerBlock, numSpanPixels - 
			numPixelsDone);

		const IST* srcPtr1 = reinterpret_cast<const IST*>(pSrc1 + 
			numPixelsDone * numSrcBands);

		const IST* srcPtr2 = reinterpret_cast<const IST*>(pSrc2 + 
			numPixelsDone * numSrcBands);

		// convert source if necessary (overwrites srcPtr1/2)
		if (OpHelpers::NeedsSrcConversion<BinaryOpType, SrcElType>(numSrcBands,
			numDstBands))
		{
			VT_HR_EXIT(OpHelpers::ConvertSpanProxy(reinterpret_cast<IST*>(srcBuf1), 
				numWorkingSrcBands, pSrc1 + numPixelsDone * numSrcBands,
				numSrcBands, numPixelsToDo * numSrcBands, false, 
				typename OpHelpers::BothAreStandardTypes<SrcElType, IST>::type()));

			srcPtr1 = reinterpret_cast<const IST*>(srcBuf1);

			VT_HR_EXIT(OpHelpers::ConvertSpanProxy(reinterpret_cast<IST*>(srcBuf2), 
				numWorkingSrcBands, pSrc2 + numPixelsDone * numSrcBands,
				numSrcBands, numPixelsToDo * numSrcBands, false,
				typename OpHelpers::BothAreStandardTypes<SrcElType, IST>::type()));

			srcPtr2 = reinterpret_cast<const IST*>(srcBuf2);
		}
		else
		{
			srcPtr1 = reinterpret_cast<const IST*>(pSrc1 + 
				numPixelsDone * numSrcBands);

			srcPtr2 = reinterpret_cast<const IST*>(pSrc2 + 
				numPixelsDone * numSrcBands);
		}

		VT_ASSERT(srcPtr1 != NULL);		
		VT_ASSERT(srcPtr2 != NULL);		

		// apply operation and convert destination if necessary 
		if (OpHelpers::NeedsDstConversion<BinaryOpType, DstElType>(numSrcBands,
			numDstBands))
		{
			VTOP_ARCH_SWITCH_BINARY(BinaryOpType, srcPtr1, srcPtr2, 
				reinterpret_cast<IDT*>(dstBuf),
				reinterpret_cast<IDT*>(dstBuf) + numPixelsToDo * 
				numWorkingDstBands, binaryOp, params);

			VT_HR_EXIT(OpHelpers::ConvertSpanProxy(pDst + numPixelsDone * numDstBands,
				numDstBands, reinterpret_cast<IDT*>(dstBuf), 
				numWorkingDstBands, numPixelsToDo * numWorkingDstBands, false,
				typename OpHelpers::BothAreStandardTypes<IDT, DstElType>::type()));
		}
		else
		{
			VTOP_ARCH_SWITCH_BINARY(BinaryOpType, srcPtr1, srcPtr2, 
				reinterpret_cast<IDT*>(pDst + numPixelsDone * numDstBands), 
				reinterpret_cast<IDT*>(pDst + (numPixelsDone + numPixelsToDo) 
				* numDstBands), binaryOp, params);
		}

		numPixelsDone += numPixelsToDo;
	}

	VT_HR_END();
}

#undef VTOP_ARCH_SWITCH_BINARY

template<template<typename, typename> class BinaryOpType, 
	typename SrcType, typename DstType, typename ParamType>
	HRESULT BinaryImgOpSS(const CImg& src1, const CImg& src2, CImg& dst, 
	ParamType* params = nullptr)
{
	typedef BinaryOpType<SrcType, DstType> OT;

	static_assert(OpHelpers::IsSameType<ParamType, 
		typename OT::ParamType>::value, "ParamType mismatch");

	VT_HR_BEGIN();

	for (int y = 0; y < dst.Height(); ++y)
	{ 
		VT_HR_EXIT(BinarySpanOp(
			reinterpret_cast<const SrcType*>(src1.BytePtr(y)),
			reinterpret_cast<const SrcType*>(src2.BytePtr(y)),
			src1.Bands(), reinterpret_cast<DstType*>(dst.BytePtr(y)),
			dst.Bands(), dst.Width(), OT(), params));
	}
	
	VT_HR_END();
}

template<template<typename, typename> class BinaryOpType, 
	typename SrcType, typename ParamType>
	HRESULT BinaryImgOpSD(const CImg& src1, const CImg& src2, CImg& dst, 
	ParamType* p = nullptr)
{
	switch (EL_FORMAT(dst.GetType()))
	{
	case EL_FORMAT_BYTE: 
		return BinaryImgOpSS<BinaryOpType, SrcType, Byte>(src1, src2, dst, p);
	case EL_FORMAT_SHORT:
		return BinaryImgOpSS<BinaryOpType, SrcType, UInt16>(src1, src2, dst, p);
	case EL_FORMAT_FLOAT:
		return BinaryImgOpSS<BinaryOpType, SrcType, float>(src1, src2, dst, p);
	case EL_FORMAT_HALF_FLOAT:
		return BinaryImgOpSS<BinaryOpType, SrcType, HALF_FLOAT>(src1, src2, dst, 
			p);
	default:
		return E_NOTIMPL;
	}
}

template<template<typename, typename> class BinaryOpType, 
	typename ParamType>
	HRESULT BinaryImgOpDD(const CImg& src1, const CImg& src2, CImg& dst, 
	ParamType* p = nullptr)
{
	VT_HR_BEGIN();

	VT_HR_EXIT(PrepareBinaryImgOp(src1, src2, dst));

	switch (EL_FORMAT(src1.GetType()))
	{
	case EL_FORMAT_BYTE:
		return BinaryImgOpSD<BinaryOpType, Byte>(src1, src2, dst, p);
	case EL_FORMAT_SHORT:
		return BinaryImgOpSD<BinaryOpType, UInt16>(src1, src2, dst, p);
	case EL_FORMAT_FLOAT:
		return BinaryImgOpSD<BinaryOpType, float>(src1, src2, dst, p);
	case EL_FORMAT_HALF_FLOAT:
		return BinaryImgOpSD<BinaryOpType, HALF_FLOAT>(src1, src2, dst, p);
	default: 
		return E_NOTIMPL;
	}

	VT_HR_END();
}

template<template<typename, typename> class BinaryOpType>
HRESULT BinaryImgOpDD(const CImg& src1, const CImg& src2, CImg& dst)
{
	return BinaryImgOpDD<BinaryOpType>(src1, src2, dst, 
		static_cast<int*>(nullptr));
}

// Internal helpers
bool IsColorImage(const CImg& img);
HRESULT InitDst(CImg& cDst, int w, int h, int type);
HRESULT InitDst(CImg& cDst, const CImg& cSrc);
HRESULT InitDstColor(CImg& cDst, const CImg& cSrc);

};
