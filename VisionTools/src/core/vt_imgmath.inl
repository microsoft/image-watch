#pragma once

#include "vt_function.h"
#include "vt_convert.h"

namespace vt {

//+-----------------------------------------------------------------------------
// 
// Function: VtAddImages
// 
//------------------------------------------------------------------------------
template<typename SrcElT, typename DstElT>
struct AddOp
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef int ParamType;
    typedef int TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE1 = 4;

public:
    void EvalGeneric(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType&) 
    {
        pDst[0] = pSrc1[0] + pSrc2[0];
    }

#if defined(VT_SSE_SUPPORTED)
    void EvalSSE1(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType&) 
    {
        __m128 x0 = _mm_loadu_ps(pSrc1);
        __m128 x1 = _mm_loadu_ps(pSrc2);
        __m128 x2 = _mm_add_ps(x0, x1);
        _mm_storeu_ps(pDst, x2);
    }
#endif
};

/*struct AddOp<Byte, Byte>
{
public:
    typedef Byte ImplSrcElType;
    typedef Byte ImplDstElType;
    typedef int ParamType;
    typedef int TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 16;

public:
    void EvalGeneric(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType&)
    {
        VtClip(&pDst[0], pSrc1[0] + pSrc2[0]);
    }

    void EvalSSE2(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType&)
    {
        __m128i x0 = _mm_loadu_si128((__m128i*)pSrc1);
        __m128i x1 = _mm_loadu_si128((__m128i*)pSrc2);
        __m128i x2 = _mm_adds_epu8(x0, x1);
        _mm_storeu_si128((__m128i*)pDst, x2);        
    }
};

struct AddOp<UInt16, UInt16>
{
public:
    typedef UInt16 ImplSrcElType;
    typedef UInt16 ImplDstElType;
    typedef int ParamType;
    typedef int TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 8;

public:
    void EvalGeneric(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType&)
    {
        VtClip(&pDst[0], pSrc1[0] + pSrc2[0]);
    }

    void EvalSSE2(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType&)
    {
        __m128i x0 = _mm_loadu_si128((__m128i*)pSrc1);
        __m128i x1 = _mm_loadu_si128((__m128i*)pSrc2);
        __m128i x2 = _mm_adds_epu16(x0, x1);
        _mm_storeu_si128((__m128i*)pDst, x2);        
    }
};*/

template<class T>
void VtAddSpan(T *pDst, const T *pSrc1, const T *pSrc2, int span)
{
    BinarySpanOp(pSrc1, pSrc2, 1, pDst, 1, span, AddOp<T,T>());
}

//+-----------------------------------------------------------------------------
// 
// Function: VtSubImages
// 
//------------------------------------------------------------------------------
template<typename SrcElT, typename DstElT>
struct SubOp
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef int ParamType;
    typedef int TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE1 = 4;

public:
    void EvalGeneric(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType&)
    {
        pDst[0] = pSrc1[0] - pSrc2[0];
    }

#if defined(VT_SSE_SUPPORTED)
    void EvalSSE1(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType&)
    {
        __m128 x0 = _mm_loadu_ps(pSrc1);
        __m128 x1 = _mm_loadu_ps(pSrc2);
        __m128 x2 = _mm_sub_ps(x0, x1);
        _mm_storeu_ps(pDst, x2);
    }
#endif
};

template<class T>
void VtSubSpan(T *pDst, const T *pSrc1, const T *pSrc2, int span)
{
    BinarySpanOp(pSrc1, pSrc2, 1, pDst, 1, span, SubOp<T,T>());
}


//+-----------------------------------------------------------------------------
// 
// Function: VtMulImages
// 
//------------------------------------------------------------------------------
template<typename SrcElT, typename DstElT>
struct MulOp
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef int ParamType;
    typedef int TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE1 = 4;

public:
    void EvalGeneric(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType&) 
    {
        pDst[0] = pSrc1[0] * pSrc2[0];
    }

#if defined(VT_SSE_SUPPORTED)
    void EvalSSE1(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType&) 
    {
        __m128 x0 = _mm_loadu_ps(pSrc1);
        __m128 x1 = _mm_loadu_ps(pSrc2);
        __m128 x2 = _mm_mul_ps(x0, x1);
        _mm_storeu_ps(pDst, x2);
    }
#endif
};

template<typename SrcElT, typename DstElT>
struct MulComplexOp
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef int ParamType;
    typedef int TmpType;

    static const int NumSrcBands = 2;
    static const int NumDstBands = 2;
    static const int NumSrcElPerGroup = 2;
    static const int NumDstElPerGroup = 2;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE1 = 2;

public:
    void EvalGeneric(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType&)
    {
        pDst[0] = pSrc1[0] * pSrc2[0] - pSrc1[1] * pSrc2[1];
        pDst[1] = pSrc1[0] * pSrc2[1] + pSrc1[1] * pSrc2[0];
    }

#if defined(VT_SSE_SUPPORTED)
    void EvalSSE1(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType&)
    {
        __m128 x0 = _mm_loadu_ps((float*)pSrc1); // im1 | re1 | im0 | re0
        __m128 x1 = _mm_loadu_ps((float*)pSrc2); // im3 | re3 | im2 | re2
        // re1 | im1 | re0 | im0
        __m128 x2 = _mm_shuffle_ps(x0, x0, _MM_SHUFFLE(2, 3, 0, 1)); 
        // re3 | re3 | re2 | re2
        __m128 x3 = _mm_shuffle_ps(x1, x1, _MM_SHUFFLE(2, 2, 0, 0)); 
        // im3 | im3 | im2 | im2
        __m128 x4 = _mm_shuffle_ps(x1, x1, _MM_SHUFFLE(3, 3, 1, 1)); 
        __m128 x5 = _mm_mul_ps(x0, x3); // im1*re3 | re1*re3 | im0*re2 | 
        // re0*re2
        __m128 x6 = _mm_mul_ps(x2, x4); // re1*im3 | im1*im3 | re0*im2 | 
        // im0*im2
        // X | re1*re3-im1*im3 | X | re0*re2-im0*im2
        __m128 x7 = _mm_sub_ps(x5, x6); 
        // im1*re3+re1*im3 | X | im0*re2+re0*im2 | X
        __m128 x8 = _mm_add_ps(x5, x6); 
        // im1*re3+re1*im3 | X | im0*re2+re0*im2 | re0*re2-im0*im2
        __m128 x9 = _mm_move_ss(x8, x7); 
        // im0*re2+re0*im2 | re0*re2-im0*im2 | im1*re3+re1*im3 | X
        __m128 x10 = _mm_shuffle_ps(x9, x9, _MM_SHUFFLE(1, 0, 3, 2)); 
        // X | X | X | re1*re3-im1*im3
        x7 = _mm_shuffle_ps(x7, x7, _MM_SHUFFLE(2, 2, 2, 2)); 
        // im0*re2+re0*im2 | re0*re2-im0*im2 | im1*re3+re1*im3 | 
        // re1*re3-im1*im3
        __m128 x11 = _mm_move_ss(x10, x7); 
        // im1*re3+re1*im3 | re1*re3-im1*im3 | im0*re2+re0*im2 | 
        // re0*re2-im0*im2
        x11 = _mm_shuffle_ps(x11, x11, _MM_SHUFFLE(1, 0, 3, 2)); 
        _mm_storeu_ps((float *)pDst, x11);
    }
#endif
};

template<class T>
void VtMulSpan(T *pDst, const T *pSrc1, const T *pSrc2, int span)
{
    BinarySpanOp(pSrc1, pSrc2, 1, pDst, 1, span, MulOp<T,T>());
}


//+-----------------------------------------------------------------------------
// 
// Function: VtScaleImage
// 
//------------------------------------------------------------------------------

#if defined(VT_SSE_SUPPORTED)
template<typename TmpType>
void ScaleSSE1(const float* pSrc, float* pDst, TmpType& tmp)
{
    __m128 x6 = _mm_loadu_ps(pSrc);
    x6 = _mm_mul_ps(x6, tmp.scaleSSE);
    _mm_storeu_ps(pDst, x6);
}

template<typename TmpType>
void ScaleSSE2(const float* pSrc, const Byte* pDst, TmpType& tmp)
{
    // 1st four results
    __m128 x4 = _mm_loadu_ps(pSrc);      // 4 x input
    x4 = _mm_mul_ps(x4, tmp.scaleSSE);
    pSrc += 4;

    // 2nd four results
    __m128 x5 = _mm_loadu_ps(pSrc);      // 4 x input
    x5 = _mm_mul_ps(x5, tmp.scaleSSE);
    pSrc += 4;

    // pack results 1,2 into 8 16 bit results
    __m128i x4i = _mm_cvtps_epi32(x4);          // 4x float to i32
    __m128i x5i = _mm_cvtps_epi32(x5);          // 4x float to i32
    x4i = _mm_packs_epi32(x4i, x5i);    // i32 to i16

    // 3rd four results
    x5 = _mm_loadu_ps(pSrc);      // 4 x input
    x5 = _mm_mul_ps(x5, tmp.scaleSSE);
    pSrc += 4;

    // 4th four results
    __m128 x6 = _mm_loadu_ps(pSrc);      // 4 x input
    x6 = _mm_mul_ps(x6, tmp.scaleSSE);
    pSrc += 4;

    // pack results 3,4 into 8 16 bit results
    x5i = _mm_cvtps_epi32(x5);          // 4x float to i32
    __m128i x6i = _mm_cvtps_epi32(x6);          // 4x float to i32
    x5i = _mm_packs_epi32(x5i, x6i);    // i32 to i16

    // pack results 1,2,3,4 into 16 8 bit results
    x4i = _mm_packus_epi16(x4i, x5i);   // i16 to i8

    _mm_storeu_si128((__m128i*)pDst, x4i);
}

template<typename TmpType>
void ScaleSSE2(const float* pSrc, const UInt16* pDst, TmpType& tmp)
{
    __m128 x2 = _mm_set1_ps(0x8000);
    __m128i x3 = _mm_set1_epi16(short(-0x8000));

    // 1st four results
    __m128 x4 = _mm_loadu_ps(pSrc);      // 4 x input
    x4 = _mm_mul_ps(x4, tmp.scaleSSE);
    pSrc += 4;

    // 2nd four results
    __m128 x5 = _mm_loadu_ps(pSrc);      // 4 x input
    x5 = _mm_mul_ps(x5, tmp.scaleSSE);
    pSrc += 4;

    x4 = _mm_sub_ps(x4, x2);                   // shift to signed
    __m128i x4i = _mm_cvtps_epi32(x4);          // 4x float to i32

    x5 = _mm_sub_ps(x5, x2);                   // shift to signed
    __m128i x5i = _mm_cvtps_epi32(x5);          // 4x float to i32

    x4i = _mm_packs_epi32(x4i, x5i);            // i32 to i16
    x4i = _mm_add_epi16(x4i, x3);               // shift back to unsign

    _mm_storeu_si128((__m128i*)pDst, x4i);
}
#endif

struct ScaleParams
{
    ScaleParams(float s) : scale(s) { }
    float scale;
};

struct ScaleTmp
{
#if defined(VT_SSE_SUPPORTED)
    __m128 scaleSSE;
#elif defined(VT_NEON_SUPPORTED)
    float32_t scale;
#endif
};

template<typename SrcElT, typename DstElT>
struct ScaleOp
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef ScaleParams ParamType;
    typedef ScaleTmp TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE1 = 4;
    static const int NumGroupsPerOpNEON = 4;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        pDst[0] = pSrc[0] * (*param).scale;
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE1(ParamType* param, TmpType& tmp)
    {
        tmp.scaleSSE = _mm_set1_ps((*param).scale);
    }

    void EvalSSE1(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleSSE1(pSrc, pDst, tmp);
    }
#elif defined(VT_NEON_SUPPORTED)
    void InitNEON(ParamType* param, TmpType& tmp)
    {
        tmp.scale = (*param).scale;
    }

    void EvalNEON(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        auto in = vld1q_f32(pSrc);
        auto res = vmulq_n_f32(in, tmp.scale);
        vst1q_f32(pDst, res);
    }
#endif
};

template<>
struct ScaleOp<float, Byte>
{
public:
    typedef float ImplSrcElType;
    typedef Byte ImplDstElType;
    typedef ScaleParams ParamType;
    typedef ScaleTmp TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 16;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        VtConv(&pDst[0], pSrc[0] * (*param).scale);
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
         tmp.scaleSSE = _mm_set1_ps((*param).scale * float(0xff));
    }

    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleSSE2(pSrc, pDst, tmp);
    }
#endif
};

template<>
struct ScaleOp<float, UInt16>
{
public:
    typedef float ImplSrcElType;
    typedef UInt16 ImplDstElType;
    typedef ScaleParams ParamType;
    typedef ScaleTmp TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 8;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        VtConv(&pDst[0], pSrc[0] * (*param).scale);
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
         tmp.scaleSSE = _mm_set1_ps((*param).scale * float(0xffff));
    }
    
    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleSSE2(pSrc, pDst, tmp);
    }
#endif
};

template<class TO, class TI>
void VtScaleSpan(TO* pD, const TI* pS, float v, int iSpan)
{
    ScaleParams params(v);
    UnarySpanOp(pS, 1, pD, 1, iSpan, ScaleOp<TI,TO>(), &params);
}

//+-----------------------------------------------------------------------------
// 
// Function: VtScaleOffsetImage
// 
//------------------------------------------------------------------------------

#if defined(VT_SSE_SUPPORTED)
template<typename TmpType>
void ScaleOffsetSSE1(const float* pSrc, float* pDst, TmpType& tmp)
{
    __m128 xf2 = _mm_loadu_ps(pSrc);
    xf2 = _mm_mul_ps(xf2, tmp.scaleSSE);
    xf2 = _mm_add_ps(xf2, tmp.offsetSSE);
    _mm_storeu_ps(pDst, xf2);
}

template<typename TmpType>
void ScaleOffsetSSE2(const float* pSrc, Byte* pDst, TmpType& tmp)
{
    __m128 xf2 = _mm_loadu_ps(pSrc);
    xf2 = _mm_mul_ps(xf2, tmp.scaleSSE);
    xf2 = _mm_add_ps(xf2, tmp.offsetSSE);
    __m128i xi0 = _mm_cvtps_epi32(xf2);

    xf2 = _mm_loadu_ps(pSrc + 4);
    xf2 = _mm_mul_ps(xf2, tmp.scaleSSE);
    xf2 = _mm_add_ps(xf2, tmp.offsetSSE);
    __m128i xi1 = _mm_cvtps_epi32(xf2);

    xf2 = _mm_loadu_ps(pSrc + 8);
    xf2 = _mm_mul_ps(xf2, tmp.scaleSSE);
    xf2 = _mm_add_ps(xf2, tmp.offsetSSE);
    __m128i xi2 = _mm_cvtps_epi32(xf2);

    xf2 = _mm_loadu_ps(pSrc + 12);
    xf2 = _mm_mul_ps(xf2, tmp.scaleSSE);
    xf2 = _mm_add_ps(xf2, tmp.offsetSSE);
    __m128i xi3 = _mm_cvtps_epi32(xf2);

    xi0 = _mm_packs_epi32(xi0, xi1);
    xi2 = _mm_packs_epi32(xi2, xi3);
    xi0 = _mm_packus_epi16(xi0, xi2);

    _mm_storeu_si128((__m128i*)pDst, xi0);    
}

template<typename TmpType>
void ScaleOffsetSSE2(const Byte* pSrc, float* pDst, TmpType& tmp)
{
    __m128i xi7 = _mm_setzero_si128 ();  // x7 = 0

    // load 16 bytes
    __m128i xi2 = _mm_loadu_si128 ((__m128i*)pSrc);

    // convert 8bit to 16bit
    __m128i xi4 = _mm_unpackhi_epi8 (xi2, xi7);  // 2 instruc
    xi2 = _mm_unpacklo_epi8 (xi2, xi7);    

    // convert 16bit to 32bit
    __m128i xi5 = _mm_unpackhi_epi16 (xi4, xi7); // 2 insturc
    xi4 = _mm_unpacklo_epi16 (xi4, xi7);
    __m128i xi3 = _mm_unpackhi_epi16 (xi2, xi7); // 2 insturc
    xi2 = _mm_unpacklo_epi16 (xi2, xi7);

    // convert 32bit to float
    __m128 xf2 = _mm_cvtepi32_ps(xi2);
    __m128 xf3 = _mm_cvtepi32_ps(xi3);
    __m128 xf4 = _mm_cvtepi32_ps(xi4);
    __m128 xf5 = _mm_cvtepi32_ps(xi5);

    // mul and add
    xf2 = _mm_mul_ps(xf2, tmp.scaleSSE);
    xf2 = _mm_add_ps(xf2, tmp.offsetSSE);
    xf3 = _mm_mul_ps(xf3, tmp.scaleSSE);
    xf3 = _mm_add_ps(xf3, tmp.offsetSSE);
    xf4 = _mm_mul_ps(xf4, tmp.scaleSSE);
    xf4 = _mm_add_ps(xf4, tmp.offsetSSE);
    xf5 = _mm_mul_ps(xf5, tmp.scaleSSE);
    xf5 = _mm_add_ps(xf5, tmp.offsetSSE);

    // store floats
    _mm_storeu_ps(pDst, xf2);
    _mm_storeu_ps((pDst + 4), xf3);
    _mm_storeu_ps((pDst + 8), xf4);
    _mm_storeu_ps((pDst + 12), xf5);
}

template<typename TmpType>
void ScaleOffsetSSE2(const float* pSrc, UInt16* pDst, TmpType& tmp)
{
    __m128i x0i = _mm_set1_epi32(0x8000);
    __m128i x1i = _mm_set1_epi16(short(-0x8000));

    // read 1st 4 float, shift range to signed short, convert to int32
    __m128  x4 = _mm_loadu_ps(pSrc);
    x4 = _mm_mul_ps(x4, tmp.scaleSSE);
    x4 = _mm_add_ps(x4, tmp.offsetSSE);
    __m128i x4i = _mm_cvtps_epi32(x4);
    x4i = _mm_sub_epi32(x4i, x0i);

    // read 2nd 4 floats, shift range to signed short, convert to int32
    x4 = _mm_loadu_ps(pSrc+4);
    x4 = _mm_mul_ps(x4, tmp.scaleSSE);
    x4 = _mm_add_ps(x4, tmp.offsetSSE);
    __m128i x5i = _mm_cvtps_epi32(x4);
    x5i = _mm_sub_epi32(x5i, x0i);

    // pack the results and shift range pack to unsigned
    x4i = _mm_packs_epi32(x4i, x5i);
    x4i = _mm_add_epi16(x4i, x1i);

    // store result - not aligned
    _mm_storeu_si128((__m128i*)pDst, x4i);    
}

template<typename TmpType>
void ScaleOffsetSSE2(const UInt16* pSrc, float* pDst, TmpType& tmp)
{
    __m128i x7 = _mm_setzero_si128 ();     // x7 = 0

    // load
    __m128i x0 = _mm_loadu_si128 ((__m128i*)pSrc);

    // convert 16bit to 32bit
    __m128i x1 = _mm_unpackhi_epi16 (x0, x7); // 2 insturc
    x0 = _mm_unpacklo_epi16 (x0, x7);

    // convert 32bit to float
    __m128 x0f = _mm_cvtepi32_ps(x0);
    __m128 x1f = _mm_cvtepi32_ps(x1);

    // scale and offset
    x0f = _mm_mul_ps(x0f, tmp.scaleSSE);
    x0f = _mm_add_ps(x0f, tmp.offsetSSE);
    x1f = _mm_mul_ps(x1f, tmp.scaleSSE);
    x1f = _mm_add_ps(x1f, tmp.offsetSSE);

    // store floats
    _mm_storeu_ps(pDst, x0f);
    _mm_storeu_ps((pDst + 4),  x1f);
}
#endif

struct ScaleOffsetParams
{
    ScaleOffsetParams(float s, float o) : scale(s), offset(o) { }
    float scale;
    float offset;
};

struct ScaleOffsetTmp
{
#if defined(VT_SSE_SUPPORTED)
    __m128 scaleSSE;
    __m128 offsetSSE;
#endif
    float scaleGeneric;
    float offsetGeneric;
};

template<typename SrcElT, typename DstElT>
struct ScaleOffsetOp
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef ScaleOffsetParams ParamType;
    typedef ScaleOffsetTmp TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE1 = 4;

public:
    void InitGeneric(ParamType* param, TmpType& tmp)
    {
        tmp.scaleGeneric = (*param).scale;
        tmp.offsetGeneric = (*param).offset *
            (1.0f / float(ElTraits<SrcElT>::MaxVal()));
    }
    
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        pDst[0] = pSrc[0] * tmp.scaleGeneric + tmp.offsetGeneric;
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE1(ParamType* param, TmpType& tmp)
    {
        tmp.scaleSSE = _mm_set1_ps((*param).scale);
        tmp.offsetSSE = _mm_set1_ps((*param).offset * 
            (1.0f / float(ElTraits<SrcElT>::MaxVal())));
    }

    void EvalSSE1(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleOffsetSSE1(pSrc, pDst, tmp);
    }
#endif
};

template<>
struct ScaleOffsetOp<float, Byte>
{
public:
    typedef float ImplSrcElType;
    typedef Byte ImplDstElType;
    typedef ScaleOffsetParams ParamType;
    typedef ScaleOffsetTmp TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 16;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        VtConv(&pDst[0], pSrc[0] * (*param).scale + (*param).offset);
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.scaleSSE = _mm_set1_ps((*param).scale * float(0xff));
        tmp.offsetSSE = _mm_set1_ps((*param).offset * float(0xff));
    }

    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleOffsetSSE2(pSrc, pDst, tmp);
    }
#endif
};

template<>
struct ScaleOffsetOp<Byte, float>
{
public:
    typedef Byte ImplSrcElType;
    typedef float ImplDstElType;
    typedef ScaleOffsetParams ParamType;
    typedef ScaleOffsetTmp TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 16;

public:
    void InitGeneric(ParamType* param, TmpType& tmp)
    {
        tmp.scaleGeneric = (*param).scale / ElTraits<Byte>::MaxVal();
        tmp.offsetGeneric = (*param).offset / ElTraits<Byte>::MaxVal();
    }

    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        pDst[0] = float(pSrc[0]) * tmp.scaleGeneric + tmp.offsetGeneric;
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.scaleSSE = _mm_set1_ps((*param).scale * (1.0f / float(0xff)));
        tmp.offsetSSE = _mm_set1_ps((*param).offset * (1.0f / float(0xff)));
    }

    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleOffsetSSE2(pSrc, pDst, tmp);
    }
#endif
};

template<>
struct ScaleOffsetOp<float, UInt16>
{
public:
    typedef float ImplSrcElType;
    typedef UInt16 ImplDstElType;
    typedef ScaleOffsetParams ParamType;
    typedef ScaleOffsetTmp TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 8;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        VtConv(&pDst[0], float(pSrc[0]) * (*param).scale + (*param).offset);
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.scaleSSE = _mm_set1_ps((*param).scale * float(0xffff));
        tmp.offsetSSE = _mm_set1_ps((*param).offset * float(0xffff));
    }

    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleOffsetSSE2(pSrc, pDst, tmp);        
    }
#endif
};

template<>
struct ScaleOffsetOp<UInt16, float>
{
public:
    typedef UInt16 ImplSrcElType;
    typedef float ImplDstElType;
    typedef ScaleOffsetParams ParamType;
    typedef ScaleOffsetTmp TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 8;

public:
    void InitGeneric(ParamType* param, TmpType& tmp)
    {
        tmp.scaleGeneric = (*param).scale / ElTraits<UInt16>::MaxVal();
        tmp.offsetGeneric = (*param).offset / ElTraits<UInt16>::MaxVal();
    }

    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        pDst[0] = float(pSrc[0]) * tmp.scaleGeneric + tmp.offsetGeneric;
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.scaleSSE = _mm_set1_ps((*param).scale * (1.0f / float(0xffff)));
        tmp.offsetSSE = _mm_set1_ps((*param).offset * (1.0f / float(0xffff)));
    }

    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleOffsetSSE2(pSrc, pDst, tmp);
    }
#endif
};

template<class TO, class TI>
void VtScaleOffsetSpan(TO* pD, const TI* pS, float fScale, float fOffset, 
    int iSpan)
{
    ScaleOffsetParams params(fScale, fOffset);
    UnarySpanOp(pS, 1, pD, 1, iSpan, 
        ScaleOffsetOp<TI,TO>(), &params);
}


//+-----------------------------------------------------------------------------
// 
// Function: VtScaleColorImage
// 
//------------------------------------------------------------------------------
struct ScaleColorParams
{
    ScaleColorParams(const RGBAFloatPix& s) : scale(s) { }
    RGBAFloatPix scale;
};

struct ScaleColorTmp
{
#if defined(VT_SSE_SUPPORTED)
    void LoadSSE(const RGBAFloatPix& scale, float factor = 1.0f)
    {
        scaleSSE = _mm_set_ps(scale.a, scale.r, scale.g, scale.b);
        __m128 sf = _mm_set1_ps(factor);
        scaleSSE = _mm_mul_ps(scaleSSE, sf);
    }

    __m128 scaleSSE;
#endif
};

template<typename SrcElT, typename DstElT>
struct ScaleColorOp
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef ScaleColorParams ParamType;
    typedef ScaleColorTmp TmpType;

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE1 = 1;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        const RGBAFloatPix& src = *reinterpret_cast<const RGBAFloatPix*>(pSrc);
        RGBAFloatPix& dst = *reinterpret_cast<RGBAFloatPix*>(pDst);

        dst.a = src.a * (*param).scale.a;
        dst.r = src.r * (*param).scale.r;
        dst.g = src.g * (*param).scale.g;
        dst.b = src.b * (*param).scale.b;
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE1(ParamType* param, TmpType& tmp)
    {
        tmp.LoadSSE((*param).scale);
    }

    void EvalSSE1(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleSSE1(pSrc, pDst, tmp);
    }
#endif
};

template<>
struct ScaleColorOp<float, Byte>
{
public:
    typedef float ImplSrcElType;
    typedef Byte ImplDstElType;
    typedef ScaleColorParams ParamType;
    typedef ScaleColorTmp TmpType;

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 4;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        const RGBAFloatPix& src = *reinterpret_cast<const RGBAFloatPix*>(pSrc);
        RGBAPix& dst = *reinterpret_cast<RGBAPix*>(pDst);

        VtConv(&dst.a, src.a * (*param).scale.a);
        VtConv(&dst.r, src.r * (*param).scale.r);
        VtConv(&dst.g, src.g * (*param).scale.g);
        VtConv(&dst.b, src.b * (*param).scale.b);
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.LoadSSE((*param).scale, float(0xff));
    }

    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleSSE2(pSrc, pDst, tmp);
    }
#endif
};

template<>
struct ScaleColorOp<float, UInt16>
{
public:
    typedef float ImplSrcElType;
    typedef UInt16 ImplDstElType;
    typedef ScaleColorParams ParamType;
    typedef ScaleColorTmp TmpType;

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 2;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        const RGBAFloatPix& src = *reinterpret_cast<const RGBAFloatPix*>(pSrc);
        RGBAShortPix& dst = *reinterpret_cast<RGBAShortPix*>(pDst);

        VtConv(&dst.a, src.a * (*param).scale.a);
        VtConv(&dst.r, src.r * (*param).scale.r);
        VtConv(&dst.g, src.g * (*param).scale.g);
        VtConv(&dst.b, src.b * (*param).scale.b);
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.LoadSSE((*param).scale, float(0xffff));
    }

    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleSSE2(pSrc, pDst, tmp);
    }
#endif
};

template<class TO, class TI>
void VtScaleColorSpan(RGBAType<TO>* pD, const RGBAType<TI> *pS, 
    const RGBAFloatPix& scale, int iW)
{
    ScaleColorParams params(scale);
    UnarySpanOp(reinterpret_cast<const TI*>(pS), 4, 
        reinterpret_cast<TO*>(pD), 4, iW, 
        ScaleColorOp<TI,TO>(), &params);
}


//+-----------------------------------------------------------------------------
// 
// Function: VtScaleOffsetColorImage
// 
//------------------------------------------------------------------------------
struct ScaleOffsetColorParams
{
    ScaleOffsetColorParams(const RGBAFloatPix& s, const RGBAFloatPix& o) 
        : scale(s), offset(o) { }
    RGBAFloatPix scale, offset;
};

struct ScaleOffsetColorTmp
{
    void LoadGeneric(const RGBAFloatPix& scale, const RGBAFloatPix& offset,
        float scaleFactor, float offsetFactor)
    {
        scaleGeneric.r = scale.r * scaleFactor;
        scaleGeneric.g = scale.g * scaleFactor;
        scaleGeneric.b = scale.b * scaleFactor;
        scaleGeneric.a = scale.a * scaleFactor;

        offsetGeneric.r = offset.r * offsetFactor;
        offsetGeneric.g = offset.g * offsetFactor;
        offsetGeneric.b = offset.b * offsetFactor;
        offsetGeneric.a = offset.a * offsetFactor;
    }

#if defined(VT_SSE_SUPPORTED)
    void LoadSSE(const RGBAFloatPix& scale, float scaleFactor, 
        const RGBAFloatPix& offset, float offsetFactor)
    {
        scaleSSE = _mm_set_ps(scale.a, scale.r, scale.g, scale.b);
        __m128 sf = _mm_set1_ps(scaleFactor);
        scaleSSE = _mm_mul_ps(scaleSSE, sf);
        offsetSSE = _mm_set_ps(offset.a, offset.r, offset.g, offset.b);
        __m128 of = _mm_set1_ps(offsetFactor);
        offsetSSE = _mm_mul_ps(offsetSSE, of);
    }
    
    __m128 scaleSSE, offsetSSE;
#endif    

    RGBAFloatPix scaleGeneric, offsetGeneric;
};

template<typename SrcElT, typename DstElT>
struct ScaleOffsetColorOp
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef ScaleOffsetColorParams ParamType;
    typedef ScaleOffsetColorTmp TmpType;

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE1 = 1;

public:
    void InitGeneric(ParamType* param, TmpType& tmp)
    {
        tmp.LoadGeneric((*param).scale, (*param).offset, 
            1.0f, 1.0f / float(ElTraits<SrcElT>::MaxVal()));
    }

    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        const RGBAFloatPix& src = *reinterpret_cast<const RGBAFloatPix*>(pSrc);
        RGBAFloatPix& dst = *reinterpret_cast<RGBAFloatPix*>(pDst);

        dst.a = src.a * tmp.scaleGeneric.a + tmp.offsetGeneric.a;
        dst.r = src.r * tmp.scaleGeneric.r + tmp.offsetGeneric.r;
        dst.g = src.g * tmp.scaleGeneric.g + tmp.offsetGeneric.g;
        dst.b = src.b * tmp.scaleGeneric.b + tmp.offsetGeneric.b;
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE1(ParamType* param, TmpType& tmp)
    {
        tmp.LoadSSE((*param).scale, 1.0f, (*param).offset,  
            1.0f / float(ElTraits<SrcElT>::MaxVal()));
    }

    void EvalSSE1(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleOffsetSSE1(pSrc, pDst, tmp);
    }
#endif
};

template<>
struct ScaleOffsetColorOp<float, Byte>
{
public:
    typedef float ImplSrcElType;
    typedef Byte ImplDstElType;
    typedef ScaleOffsetColorParams ParamType;
    typedef ScaleOffsetColorTmp TmpType;

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 4;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        const RGBAFloatPix& src = *reinterpret_cast<const RGBAFloatPix*>(pSrc);
        RGBAPix& dst = *reinterpret_cast<RGBAPix*>(pDst);

        VtConv(&dst.a, src.a * (*param).scale.a + (*param).offset.a);
        VtConv(&dst.r, src.r * (*param).scale.r + (*param).offset.r);
        VtConv(&dst.g, src.g * (*param).scale.g + (*param).offset.g);
        VtConv(&dst.b, src.b * (*param).scale.b + (*param).offset.b);
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.LoadSSE((*param).scale, float(0xff), (*param).offset, float(0xff));
    }

    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleOffsetSSE2(pSrc, pDst, tmp);
    }
#endif
};

template<>
struct ScaleOffsetColorOp<Byte, float>
{
public:
    typedef Byte ImplSrcElType;
    typedef float ImplDstElType;
    typedef ScaleOffsetColorParams ParamType;
    typedef ScaleOffsetColorTmp TmpType;

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 4;

public:
    void InitGeneric(ParamType* param, TmpType& tmp)
    {
        tmp.LoadGeneric((*param).scale, (*param).offset, 
            1.0f / float(ElTraits<Byte>::MaxVal()),
            1.0f / float(ElTraits<Byte>::MaxVal()));
    }

    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        const RGBAPix& src = *reinterpret_cast<const RGBAPix*>(pSrc);
        RGBAFloatPix& dst = *reinterpret_cast<RGBAFloatPix*>(pDst);

        dst.a = src.a * tmp.scaleGeneric.a + tmp.offsetGeneric.a;
        dst.r = src.r * tmp.scaleGeneric.r + tmp.offsetGeneric.r;
        dst.g = src.g * tmp.scaleGeneric.g + tmp.offsetGeneric.g;
        dst.b = src.b * tmp.scaleGeneric.b + tmp.offsetGeneric.b;
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.LoadSSE((*param).scale, 1.0f / float(0xff), (*param).offset, 
            1.0f / float(0xff));
    }

    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleOffsetSSE2(pSrc, pDst, tmp);
    }
#endif
};

template<>
struct ScaleOffsetColorOp<float, UInt16>
{
public:
    typedef float ImplSrcElType;
    typedef UInt16 ImplDstElType;
    typedef ScaleOffsetColorParams ParamType;
    typedef ScaleOffsetColorTmp TmpType;

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 2;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        const RGBAFloatPix& src = *reinterpret_cast<const RGBAFloatPix*>(pSrc);
        RGBAShortPix& dst = *reinterpret_cast<RGBAShortPix*>(pDst);

        VtConv(&dst.a, src.a * (*param).scale.a + (*param).offset.a);
        VtConv(&dst.r, src.r * (*param).scale.r + (*param).offset.r);
        VtConv(&dst.g, src.g * (*param).scale.g + (*param).offset.g);
        VtConv(&dst.b, src.b * (*param).scale.b + (*param).offset.b);
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.LoadSSE((*param).scale, float(0xffff), (*param).offset, 
            float(0xffff));
    }

    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleOffsetSSE2(pSrc, pDst, tmp);
    }
#endif
};

template<>
struct ScaleOffsetColorOp<UInt16, float>
{
public:
    typedef UInt16 ImplSrcElType;
    typedef float ImplDstElType;
    typedef ScaleOffsetColorParams ParamType;
    typedef ScaleOffsetColorTmp TmpType;

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 2;

public:
    void InitGeneric(ParamType* param, TmpType& tmp)
    {
        tmp.LoadGeneric((*param).scale, (*param).offset, 
            1.0f / float(ElTraits<UInt16>::MaxVal()),
            1.0f / float(ElTraits<UInt16>::MaxVal()));
    }

    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        const RGBAShortPix& src = *reinterpret_cast<const RGBAShortPix*>(pSrc);
        RGBAFloatPix& dst = *reinterpret_cast<RGBAFloatPix*>(pDst);

        dst.a = src.a * tmp.scaleGeneric.a + tmp.offsetGeneric.a;
        dst.r = src.r * tmp.scaleGeneric.r + tmp.offsetGeneric.r;
        dst.g = src.g * tmp.scaleGeneric.g + tmp.offsetGeneric.g;
        dst.b = src.b * tmp.scaleGeneric.b + tmp.offsetGeneric.b;
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.LoadSSE((*param).scale, 1.0f / float(0xffff), (*param).offset, 
            1.0f / float(0xffff));
    }

    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        ScaleOffsetSSE2(pSrc, pDst, tmp);
    }
#endif
};

template<class TO, class TI>
void VtScaleOffsetColorSpan(RGBAType<TO>* pD, const RGBAType<TI> *pS, 
    const RGBAFloatPix& scale, const RGBAFloatPix& offset, int iW)
{
    ScaleOffsetColorParams params(scale, offset);
    UnarySpanOp(reinterpret_cast<const TI*>(pS), 4, 
        reinterpret_cast<TO*>(pD), 4, iW, 
        ScaleOffsetColorOp<TI,TO>(), &params);
}

//+-----------------------------------------------------------------------------
// 
// Function: VtMultiplyAlpha
// 
//------------------------------------------------------------------------------
template<typename SrcElT, typename DstElT>
struct MultiplyAlphaOp
{
public:
    struct TmpType
    {
#if defined(VT_SSE_SUPPORTED)
        TmpType()
            : ones(_mm_set1_ps(1.f))
        {
        }
        __m128 ones;
#endif
    };

public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef int ParamType;    

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE1 = 1;
    static const int NumGroupsPerOpNEON = 1;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType&)
    {
        pDst[0] = pSrc[0] * pSrc[3];
        pDst[1] = pSrc[1] * pSrc[3];
        pDst[2] = pSrc[2] * pSrc[3];
        pDst[3] = pSrc[3];    
    }
        
#if defined(VT_SSE_SUPPORTED)
    void EvalSSE1(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        // load
        __m128 x0 = _mm_loadu_ps(pSrc);
        // extract alpha - result will be | 1.f | a | a | a |
        __m128 x1 = _mm_shuffle_ps(tmp.ones, x0, _MM_SHUFFLE(3,3,3,3));
        x1 = _mm_shuffle_ps(x0, x1, _MM_SHUFFLE(0,3,3,3));
        // multiply
        x1 = _mm_mul_ps(x1, x0);
        // store
        _mm_storeu_ps(pDst, x1);
    }
#elif defined(VT_NEON_SUPPORTED)
    void EvalNEON(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType& tmp)
    {
        VT_USE(tmp);
        float32x4_t x0 = vld1q_f32(pSrc); // b,g,r,a
        float32x4_t x1 = vdupq_lane_f32(vget_high_f32(x0),1); // a,a,a,a
        x1 = vsetq_lane_f32(1.f,x1,3); // a,a,a,1
        x1 = vmulq_f32(x0,x1); // r*a,g*a,b*a,a
        vst1q_f32(pDst,x1);
    }
#endif

};

template<>
struct MultiplyAlphaOp<Byte, Byte>
{
public:
    struct TmpType
    {
#if defined(VT_SSE_SUPPORTED)
        TmpType()
        {
            x7 = _mm_set1_epi16(0x80);      // x7 = 0x80 by 8
#if defined(_MSC_VER)
            x6.m128i_u64[0] = x6.m128i_u64[1] = UINT64(0x00ff000000000000);
#else
            x6 = _mm_setr_epi64(__m64(0x00ff000000000000), __m64(0x00ff000000000000));
#endif
            x5 = _mm_set1_epi16(0);
        }
        
        __m128i x5, x6, x7;
#endif
    };

public:
    typedef Byte ImplSrcElType;
    typedef Byte ImplDstElType;
    typedef int ParamType;    

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 4;
    static const int NumGroupsPerOpNEON = 8;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType&)
    {
        float alpha;
        VtConv(&alpha, pSrc[3]);
        pDst[0] = (ImplDstElType)(F2I(float(pSrc[0]) * alpha));
        pDst[1] = (ImplDstElType)(F2I(float(pSrc[1]) * alpha));
        pDst[2] = (ImplDstElType)(F2I(float(pSrc[2]) * alpha));
        pDst[3] = pSrc[3];
    }

#if defined(VT_SSE_SUPPORTED)
    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst,
        ParamType*, TmpType& tmp)
    {
        // load
        __m128i x0 = _mm_loadu_si128 ((__m128i*)pSrc);

        // convert 8bit to 16bit
        __m128i x2 = _mm_unpackhi_epi8 (x0, tmp.x5);  // 2 instruc
        x0 = _mm_unpacklo_epi8 (x0, tmp.x5);    

        // extract alphas
        __m128i x1 = x0;
        x1 = _mm_shufflelo_epi16(x1, _MM_SHUFFLE(3,3,3,3));
        x1 = _mm_shufflehi_epi16(x1, _MM_SHUFFLE(3,3,3,3));
        __m128i x3 = x2;
        x3 = _mm_shufflelo_epi16(x3, _MM_SHUFFLE(3,3,3,3));
        x3 = _mm_shufflehi_epi16(x3, _MM_SHUFFLE(3,3,3,3));

        // set alpha multiply to 255
        x1 = _mm_or_si128(x1, tmp.x6);
        x3 = _mm_or_si128(x3, tmp.x6);

        // do integer approx of alpha mult  
        // t = (a * b + 0x80); result = ((t >> 8) + t) >> 8
        x0 = _mm_mullo_epi16(x0, x1);
        x0 = _mm_add_epi16(x0, tmp.x7);
        x0 = _mm_srli_epi16(_mm_add_epi16(x0,_mm_srli_epi16(x0, 8)),8);

        x2 = _mm_mullo_epi16(x2, x3);
        x2 = _mm_add_epi16(x2, tmp.x7);
        x2 = _mm_srli_epi16(_mm_add_epi16(x2,_mm_srli_epi16(x2, 8)),8);

        // convert to 8 bit
        x0 = _mm_packus_epi16(x0, x2);

        // store result
        _mm_storeu_si128((__m128i*)pDst, x0);
    }
#elif defined(VT_NEON_SUPPORTED)
	void EvalNEON(const ImplSrcElType* pSrc, ImplDstElType* pDst,
        ParamType*, TmpType& tmp)
	{
        VT_USE(tmp);
        uint8x8x4_t pix4 = vld4_u8(pSrc);
        uint8x16_t b = vmull_u8(pix4.val[0],pix4.val[3]);
        uint8x16_t g = vmull_u8(pix4.val[1],pix4.val[3]);
        uint8x16_t r = vmull_u8(pix4.val[2],pix4.val[3]);
        pix4.val[0] = vrshrn_n_u16(b,8);
        pix4.val[1] = vrshrn_n_u16(g,8);
        pix4.val[2] = vrshrn_n_u16(r,8);
        vst4_u8(pDst,pix4);
    }
#endif
};

template<>
struct MultiplyAlphaOp<UInt16, UInt16>
{
public:
    struct TmpType
    {
#if defined(VT_SSE_SUPPORTED)
        TmpType()
        {
            x7   = _mm_set_ps(1.f, 1.f/65535.f, 1.f/65535.f, 1.f/65535.f);
            x6i = _mm_set1_epi32(0x8000);
            x5i = _mm_set1_epi16(short(-0x8000));
            x4i = _mm_set1_epi16(0);
        }
        
        __m128i x4i, x5i, x6i;
        __m128 x7;        
#endif
    };

public:
    typedef UInt16 ImplSrcElType;
    typedef UInt16 ImplDstElType;
    typedef int ParamType;    

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 2;

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType*, TmpType&)
    {
        float alpha;
        VtConv(&alpha, pSrc[3]);
        pDst[0] = (ImplDstElType)(F2I(float(pSrc[0]) * alpha));
        pDst[1] = (ImplDstElType)(F2I(float(pSrc[1]) * alpha));
        pDst[2] = (ImplDstElType)(F2I(float(pSrc[2]) * alpha));
        pDst[3] = pSrc[3];
    }

#if defined(VT_SSE_SUPPORTED)
    void EvalSSE2(const ImplSrcElType* pSrc, ImplDstElType* pDst,
        ParamType*, TmpType& tmp)
    {
        // load 8 16bit values
        __m128i x0i = _mm_loadu_si128 ((__m128i*)pSrc);

        // convert 16bit to 32bit
        __m128i x1i = _mm_unpackhi_epi16 (x0i, tmp.x4i); // 2 insturc
        x0i = _mm_unpacklo_epi16 (x0i, tmp.x4i);

        // convert 32bit to float
        __m128 x0 = _mm_cvtepi32_ps(x0i);
        __m128 x2 = _mm_cvtepi32_ps(x1i);

        // extract alpha - result will be | 1.f | a | a | a |
        __m128 x1 = _mm_shuffle_ps(tmp.x7, x0, _MM_SHUFFLE(3,3,3,3));
        x1 = _mm_shuffle_ps(x0, x1, _MM_SHUFFLE(0,3,3,3));
        x1 = _mm_mul_ps(x1, tmp.x7);
        __m128 x3 = _mm_shuffle_ps(tmp.x7, x2, _MM_SHUFFLE(3,3,3,3));
        x3 = _mm_shuffle_ps(x2, x3, _MM_SHUFFLE(0,3,3,3));
        x3 = _mm_mul_ps(x3, tmp.x7);

        // multiply
        x0 = _mm_mul_ps(x0, x1);
        x2 = _mm_mul_ps(x2, x3);

        // 8x float to i32
        x0i = _mm_cvtps_epi32(x0);          
        x0i = _mm_sub_epi32(x0i, tmp.x6i);
        __m128i x2i = _mm_cvtps_epi32(x2);          
        x2i = _mm_sub_epi32(x2i, tmp.x6i);

        // 8x i32 to i16
        x0i = _mm_packs_epi32(x0i, x2i);
        x0i = _mm_add_epi16(x0i, tmp.x5i);

        // store
        _mm_storeu_si128((__m128i*)pDst, x0i);
    }
#endif
};

template<class T>
void VtMultiplyAlphaSpan(T* pDst, const T* pSrc, int iCnt)
{
    UnarySpanOp(pSrc, 1, pDst, 1, iCnt, MultiplyAlphaOp<T,T>());
}


//+-----------------------------------------------------------------------------
// 
// Function: VtLogImage
// 
//------------------------------------------------------------------------------
template<typename SrcElT, typename DstElT>
struct LogOp
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef float ParamType;
    typedef int TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;    

public:    
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        const ParamType* param, TmpType&)
    {
        const float s = *pSrc * float(ElTraits<SrcElT>::MaxVal());
        *pDst = (s > 0.f ? logf(s) : *param) / float(ElTraits<SrcElT>::MaxVal());
    }
};

//+-----------------------------------------------------------------------------
// 
// Function: VtLogImage
// 
//------------------------------------------------------------------------------

// A helper -- VtClip fails for some constellations involving inf
template<typename T>
inline float SafeLargeFloat(float val)
{
    return val > ElTraits<T>::MaxVal() ? float(ElTraits<T>::MaxVal()) : val;
}

template<>
inline float SafeLargeFloat<float>(float val)
{
    return val;
}    

template<>
inline float SafeLargeFloat<HALF_FLOAT>(float val)
{
    return val;
}    

template<typename SrcElT, typename DstElT>
struct ExpOp
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef int ParamType;
    typedef int TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;    

public:    
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        const ParamType*, TmpType&)
    {
        *pDst = SafeLargeFloat<DstElT>(
            expf(*pSrc * float(ElTraits<SrcElT>::MaxVal())) / 
            float(ElTraits<SrcElT>::MaxVal()));
    }
};


//+-----------------------------------------------------------------------------
// 
// Function: VtBlendImages
// 
//------------------------------------------------------------------------------
struct BlendOpParams
{
    BlendOpParams(float weight1, float weight2)
        : w1(weight1), w2(weight2)
    {
    }
    
    float w1, w2;
};

template<typename SrcElT, typename DstElT>
struct BlendOpBaseFloatFloat
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef BlendOpParams ParamType;
    struct TmpType 
    { 
#if defined(VT_SSE_SUPPORTED)
        __m128 x0, x1; 
#endif
    };

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE1 = 4;

public:
    void EvalGeneric(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType* param, TmpType&) 
    {
        pDst[0] = (*param).w1 * pSrc1[0] + (*param).w2 * pSrc2[0];
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE1(ParamType* param, TmpType& tmp)
    {
        tmp.x0 = _mm_set1_ps((*param).w1);
        tmp.x1 = _mm_set1_ps((*param).w2);
    }

    void EvalSSE1(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType& tmp) 
    {
        __m128 x6 = _mm_loadu_ps(pSrc1);      // 4 x row 0
        x6 = _mm_mul_ps(x6, tmp.x0);
        
        __m128 x7 = _mm_loadu_ps(pSrc2);      // 4 x row 1
        x7 = _mm_mul_ps(x7, tmp.x1);
        x6 = _mm_add_ps(x6, x7);
        
        _mm_storeu_ps(pDst, x6);
    }
#endif
};

template<typename SrcElT, typename DstElT>
struct BlendOp : public BlendOpBaseFloatFloat<SrcElT, DstElT> {};

template<>
struct BlendOp<float, Byte>
{
public:
    typedef float ImplSrcElType;
    typedef Byte ImplDstElType;
    typedef BlendOpParams ParamType;
    struct TmpType 
    { 
#if defined(VT_SSE_SUPPORTED)
        __m128 x0, x1;
#endif
        float w1, w2; 
    };

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 16;

public:
    void InitGeneric(ParamType* param, TmpType& tmp)
    {
         tmp.w1 = (*param).w1 * float(ElTraits<ImplDstElType>::MaxVal());
         tmp.w2 = (*param).w2 * float(ElTraits<ImplDstElType>::MaxVal());
    }

    void EvalGeneric(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType& tmp) 
    {
        VtClip(pDst, tmp.w1 * pSrc1[0] + tmp.w2 * pSrc2[0]);
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.x0 = _mm_set1_ps((*param).w1 * 
            float(ElTraits<ImplDstElType>::MaxVal()));
        tmp.x1 = _mm_set1_ps((*param).w2 * 
            float(ElTraits<ImplDstElType>::MaxVal()));
    }

    void EvalSSE2(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType& tmp) 
    {
        // 1st four results
        __m128 x4 = _mm_loadu_ps(pSrc1);      // 4 x row 0
        x4 = _mm_mul_ps(x4, tmp.x0);

        __m128 x5 = _mm_loadu_ps(pSrc2);      // 4 x row 1
        x5 = _mm_mul_ps(x5, tmp.x1);
        x4 = _mm_add_ps(x4, x5);
        pSrc1 += 4; pSrc2 += 4;

        // 2nd four results
        x5 = _mm_loadu_ps(pSrc1);      // 4 x row 0
        x5 = _mm_mul_ps(x5, tmp.x0);

        __m128 x6 = _mm_loadu_ps(pSrc2);      // 4 x row 1
        x6 = _mm_mul_ps(x6, tmp.x1);
        x5 = _mm_add_ps(x5, x6);
        pSrc1 += 4; pSrc2 += 4;

        __m128i x4i = _mm_cvtps_epi32(x4);          // 4x float to i32
        __m128i x5i = _mm_cvtps_epi32(x5);          // 4x float to i32
        x4i = _mm_packs_epi32(x4i, x5i);    // i32 to i16

        // 3rd four results
        x5 = _mm_loadu_ps(pSrc1);      // 4 x row 0
        x5 = _mm_mul_ps(x5, tmp.x0);

        x6 = _mm_loadu_ps(pSrc2);      // 4 x row 1
        x6 = _mm_mul_ps(x6, tmp.x1);
        x5 = _mm_add_ps(x5, x6);
        pSrc1 += 4; pSrc2 += 4;

        // 4th four results
        x6 = _mm_loadu_ps(pSrc1);      // 4 x row 0
        x6 = _mm_mul_ps(x6, tmp.x0);

        __m128 x7 = _mm_loadu_ps(pSrc2);      // 4 x row 1
        x7 = _mm_mul_ps(x7, tmp.x1);
        x6 = _mm_add_ps(x6, x7);        

        x5i = _mm_cvtps_epi32(x5);          // 4x float to i32
        __m128i x6i = _mm_cvtps_epi32(x6);          // 4x float to i32
        x5i = _mm_packs_epi32(x5i, x6i);    // i32 to i16
        x4i = _mm_packus_epi16(x4i, x5i);   // i16 to i8

        _mm_storeu_si128((__m128i*)pDst, x4i);
    }
#endif
};

template<>
struct BlendOp<float, UInt16>
{
public:
    typedef float ImplSrcElType;
    typedef UInt16 ImplDstElType;
    typedef BlendOpParams ParamType;
    struct TmpType 
    { 
#if defined(VT_SSE_SUPPORTED)
        __m128 x0, x1, x2; __m128i x3; 
#endif
        float w1, w2; 
    };

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 8;

public:
    void InitGeneric(ParamType* param, TmpType& tmp)
    {
         tmp.w1 = (*param).w1 * float(ElTraits<ImplDstElType>::MaxVal());
         tmp.w2 = (*param).w2 * float(ElTraits<ImplDstElType>::MaxVal());
    }

    void EvalGeneric(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType& tmp) 
    {
        VtClip(pDst, tmp.w1 * pSrc1[0] + tmp.w2 * pSrc2[0]);
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.x0 = _mm_set1_ps((*param).w1 * 
            float(ElTraits<ImplDstElType>::MaxVal()));
        tmp.x1 = _mm_set1_ps((*param).w2 * 
            float(ElTraits<ImplDstElType>::MaxVal()));
        tmp.x2 = _mm_set1_ps(0x8000);
        tmp.x3 = _mm_set1_epi16(short(-0x8000));
    }

    void EvalSSE2(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType& tmp) 
    {
        // 1st four results
        __m128 x4 = _mm_loadu_ps(pSrc1);    // 4 x row 0
        x4 = _mm_mul_ps(x4, tmp.x0);

        __m128 x5 = _mm_loadu_ps(pSrc2);    // 4 x row 1
        x5 = _mm_mul_ps(x5, tmp.x1);
        x4 = _mm_add_ps(x4, x5);
        pSrc1 += 4; pSrc2 += 4;

        // 2nd four results
        x5 = _mm_loadu_ps(pSrc1);            // 4 x row 0
        x5 = _mm_mul_ps(x5, tmp.x0);

        __m128 x6 = _mm_loadu_ps(pSrc2);    // 4 x row 1
        x6 = _mm_mul_ps(x6, tmp.x1);
        x5 = _mm_add_ps(x5, x6);
        
        x4  = _mm_sub_ps(x4, tmp.x2);       // shift to signed
        __m128i x4i = _mm_cvtps_epi32(x4);  // 4x float to i32

        x5  = _mm_sub_ps(x5, tmp.x2);       // shift to signed
        __m128i x5i = _mm_cvtps_epi32(x5);  // 4x float to i32
        
        x4i = _mm_packs_epi32(x4i, x5i);    // i32 to i16
        x4i = _mm_add_epi16(x4i, tmp.x3);   // shift back to unsign

        _mm_storeu_si128((__m128i*)pDst, x4i);
    }
#endif
};

template<>
struct BlendOp<Byte, Byte>
{
public:
    typedef Byte ImplSrcElType;
    typedef Byte ImplDstElType;
    typedef BlendOpParams ParamType;
    struct TmpType 
    {
#if defined(VT_SSE_SUPPORTED)
        __m128i x4, x5, x6, x7; 
#endif
        Byte uC1, uC2; 
    };

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;
    static const int NumGroupsPerOpSSE2 = 16;

public:
    void InitGeneric(ParamType* param, TmpType& tmp)
    {
        tmp.uC1 = static_cast<Byte>(F2I(256.f * (*param).w1));
        tmp.uC2 = static_cast<Byte>(F2I(256.f * (*param).w2));
    }

    void EvalGeneric(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType& tmp) 
    {
        pDst[0] = (tmp.uC1 * pSrc1[0] + tmp.uC2 * pSrc2[0] + 0x80) >> 8;
    }

#if defined(VT_SSE_SUPPORTED)
    void InitSSE2(ParamType* param, TmpType& tmp)
    {
        tmp.x7 = _mm_setzero_si128 ();   // x7 = 0
        tmp.x6 = _mm_set1_epi16(0x80);   // 0.5
        tmp.x5 = _mm_set1_epi16(static_cast<UInt16>(F2I(256.f * (*param).w1)));
        tmp.x4 = _mm_set1_epi16(static_cast<UInt16>(F2I(256.f * (*param).w2)));
    }

    void EvalSSE2(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
        ImplDstElType* pDst, ParamType*, TmpType& tmp) 
    {
         // load 2 lines of 16 bytes 
        __m128i x0 = _mm_loadu_si128 ((__m128i*)pSrc1);
        __m128i x1 = _mm_loadu_si128 ((__m128i*)pSrc2);
        // unpack low 8 bytes and do a mul/add/round - res in x2 
        __m128i x2 = x0;
        __m128i x3 = x1;
        x2 = _mm_unpacklo_epi8 (x2, tmp.x7);
        x3 = _mm_unpacklo_epi8 (x3, tmp.x7);
        x2 = _mm_mullo_epi16(x2, tmp.x5);
        x3 = _mm_mullo_epi16(x3, tmp.x4);
        x2 = _mm_add_epi16(x2, tmp.x6);
        x2 = _mm_add_epi16(x2, x3);
        x2 = _mm_srli_epi16(x2, 8);
        // unpack hi 8 bytes and do a mul/add - res in x0 
        x0 = _mm_unpackhi_epi8 (x0, tmp.x7);
        x1 = _mm_unpackhi_epi8 (x1, tmp.x7);
        x0 = _mm_mullo_epi16(x0, tmp.x5);
        x1 = _mm_mullo_epi16(x1, tmp.x4);
        x0 = _mm_add_epi16(x0, tmp.x6);
        x0 = _mm_add_epi16(x0, x1);
        x0 = _mm_srli_epi16(x0, 8);
        // pack result and write 
        x2 = _mm_packus_epi16(x2, x0);   // i16 to i8
        _mm_storeu_si128((__m128i*)pDst, x2);
    }
#endif
};

template<typename TI, typename TO>
void VtBlendSpan(TO* pDst, const TI* pSrc1, const TI* pSrc2, float w1, 
    float w2, int numEl)
{    
    VtBlendSpan(pDst, 1, pSrc1, pSrc2, 1, w1, w2, numEl); 
}

template<typename TI, typename TO>
void VtBlendSpan(TO* pDst, int numDstBands, const TI* pSrc1, 
    const TI* pSrc2, int numSrcBands, float w1, float w2, int numPix)
{
    BlendOpParams params(w1, w2);

    if (!(0.f <= w1 && w1 <= 1.f) ||
        !(0.f <= w2 && w2 <= 1.f) ||
        !(w1 + w2 <= 1.f + 1e-3))
    {
        BinarySpanOp(pSrc1, pSrc2, numSrcBands, pDst, numDstBands, 
                     numPix, BlendOpBaseFloatFloat<TI,TO>(), &params); 
    }
    else if (w1 == 1.f)
    {
        VtConvertSpan(pDst,  VT_IMG_MAKE_TYPE(ElTraits<TO>::ElFormat(), numDstBands), 
                      pSrc1, VT_IMG_MAKE_TYPE(ElTraits<TI>::ElFormat(), numSrcBands), 
                      numPix * numSrcBands);
    }
    else if (w2 == 1.f)
    {
        VtConvertSpan(pDst,  VT_IMG_MAKE_TYPE(ElTraits<TO>::ElFormat(), numDstBands), 
                      pSrc2, VT_IMG_MAKE_TYPE(ElTraits<TI>::ElFormat(), numSrcBands),
                      numPix * numSrcBands);
    }
    else
    {
        BinarySpanOp(pSrc1, pSrc2, numSrcBands, pDst, numDstBands, 
                     numPix, BlendOp<TI,TO>(), &params); 
    }
}

//+-----------------------------------------------------------------------------
//
// GammaMap
//
//------------------------------------------------------------------------------

class LookupTable
{
private:
    LookupTable(const LookupTable&);
    LookupTable& operator=(const LookupTable&);

public:
    LookupTable()
        : m_srcElFormat(0), m_dstElFormat(0), m_numEl(0), m_map(nullptr)
    {
    }

    ~LookupTable()
    {
        Deallocate();
    }
        
    void Reset()
    {
        m_srcElFormat = 0;
        m_dstElFormat = 0;
        Deallocate();
    }

    template<typename FuncType>
    HRESULT Initialize(int srcElFormat, int dstElFormat, FuncType func, 
        void* funcParams)
    {
        VT_HR_BEGIN();

        VT_ASSERT(IsValidSrcElFormat(srcElFormat));
        
        Reset();

        VT_HR_EXIT(Allocate(srcElFormat, dstElFormat));

        float maxVal = float(m_numEl - 1);

        switch (dstElFormat)
        {
        case EL_FORMAT_BYTE:
            for (size_t i = 0; i < m_numEl; ++i)
                VtConv(&Map<Byte>(i), func(float(i)/maxVal, funcParams)); 
            break;
        case EL_FORMAT_SHORT:
            for (size_t i = 0; i < m_numEl; ++i)
                VtConv(&Map<UInt16>(i), func(float(i)/maxVal, funcParams)); 
            break;
        case EL_FORMAT_FLOAT:
            for (size_t i = 0; i < m_numEl; ++i)
                VtConv(&Map<float>(i), func(float(i)/maxVal, funcParams)); 
            break;
        case EL_FORMAT_HALF_FLOAT:
            for (size_t i = 0; i < m_numEl; ++i)
                 VtConv(&Map<HALF_FLOAT>(i), func(float(i)/maxVal, funcParams)); 
            break;
        default:
            Deallocate();
            return E_INVALIDDST;
        }
        
        VT_HR_END();
    }

    bool IsInitialized(int srcElFormat, int dstElFormat) const
    {
        return m_srcElFormat == srcElFormat && 
            m_dstElFormat == dstElFormat &&
            m_map != nullptr;
    }

    static bool IsValidSrcElFormat(int format)
    {
        return format == EL_FORMAT_BYTE || format == EL_FORMAT_SHORT;
    }

    template<typename DstElType>
    const DstElType& Map(size_t index) const
    {
        VT_ASSERT(ElTraits<DstElType>::ElFormat() == m_dstElFormat);
        VT_ASSERT(index < m_numEl);
        VT_ASSERT(sizeof(DstElType) == VT_IMG_ELSIZE(m_dstElFormat));

        return reinterpret_cast<const DstElType*>(m_map)[index];
    }

    template<typename DstElType>
    DstElType& Map(size_t index) 
    {
        return const_cast<DstElType&>(
            static_cast<const LookupTable*>(this)->Map<DstElType>(index));
    }

private:
    HRESULT Allocate(int srcElFormat, int dstElFormat)
    {
        VT_HR_BEGIN();

        Deallocate();

        // printf_s("allocating\n");

        if (srcElFormat == EL_FORMAT_BYTE || srcElFormat == EL_FORMAT_SHORT)
            m_numEl = (size_t)1 << (8*VT_IMG_ELSIZE(srcElFormat));
        else
            VT_HR_EXIT(E_INVALIDSRC);

        m_map = VT_NOTHROWNEW char[m_numEl * VT_IMG_ELSIZE(dstElFormat)];
        
        if (m_map == nullptr)
            VT_HR_EXIT(E_OUTOFMEMORY);

        m_srcElFormat = srcElFormat;
        m_dstElFormat = dstElFormat;

        VT_HR_END();
    }
    
    void Deallocate()
    {
        delete[] m_map;
        m_map = nullptr;
        m_numEl = 0;
    }

private:
    int m_srcElFormat;
    int m_dstElFormat;
    size_t m_numEl;
    char* m_map;
};

////////////////////////////////////////////////////////////////////////////////

struct CACHED_MAP::Map
{
    static HRESULT Create(Map** map, int srcElFormat, int dstElFormat, 
                          float (*f)(float, void*), void* funcParams)
    {
        VT_HR_BEGIN();

        VT_HR_EXIT(map != nullptr ? S_OK : E_INVALIDARG);

        *map = VT_NOTHROWNEW CACHED_MAP::Map();
        VT_HR_EXIT(*map != nullptr ? S_OK : E_OUTOFMEMORY);

        (**map).func = f;
        (**map).funcParams = funcParams;
        (**map).table.Reset();

        if (!(**map).table.IsInitialized(srcElFormat, dstElFormat) &&         
            LookupTable::IsValidSrcElFormat(srcElFormat))
        {
            VT_HR_EXIT((**map).table.Initialize(srcElFormat, dstElFormat, 
                (**map).func, funcParams));
        }

        VT_HR_END();
    }

    float (*func)(float, void*);
    void* funcParams;
    LookupTable table;
};

template<typename SrcElT, typename DstElT>
struct MapOp
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef CACHED_MAP::Map ParamType;
    typedef int TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;    

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        *pDst = (*param).func(*pSrc, (*param).funcParams);
    }
};

template<typename SrcElT, typename DstElT>
struct MapOpLookupBase
{
public:
    typedef SrcElT ImplSrcElType;
    typedef DstElT ImplDstElType;
    typedef CACHED_MAP::Map ParamType;
    typedef int TmpType;

    static const int NumSrcBands = 0;
    static const int NumDstBands = 0;
    static const int NumSrcElPerGroup = 1;
    static const int NumDstElPerGroup = 1;
    static const int NumGroupsPerOpGeneric = 1;    

public:
    void InitGeneric(ParamType* param, TmpType&)
    {
        UNREFERENCED_PARAMETER(param);
        VT_ASSERT((*param).table.IsInitialized(
            ElTraits<ImplSrcElType>::ElFormat(),
            ElTraits<ImplDstElType>::ElFormat()));
    }

    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        *pDst = (*param).table.Map<ImplDstElType>(*pSrc);
    }    
};

// MapOp specializations for lookup impl
template<typename DstElT>
struct MapOp<Byte, DstElT> : public MapOpLookupBase<Byte, DstElT> {};
template<typename DstElT>
struct MapOp<UInt16, DstElT> : public MapOpLookupBase<UInt16, DstElT> {};

////////////////////////////////////////////////////////////////////////////////

template<typename SrcElT, typename DstElT>
struct MapColorOp
{
public:
    typedef float ImplSrcElType;
    typedef float ImplDstElType;
    typedef CACHED_MAP::Map ParamType;
    typedef int TmpType;

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;    

public:
    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        pDst[0] = (*param).func(pSrc[0], (*param).funcParams);
        pDst[1] = (*param).func(pSrc[1], (*param).funcParams);
        pDst[2] = (*param).func(pSrc[2], (*param).funcParams);
        pDst[3] = pSrc[3];
    }
};

template<typename SrcElT, typename DstElT>
struct MapColorOpLookupBase
{
public:
    typedef SrcElT ImplSrcElType;
    typedef DstElT ImplDstElType;
    typedef CACHED_MAP::Map ParamType;
    typedef int TmpType;

    static const int NumSrcBands = 4;
    static const int NumDstBands = 4;
    static const int NumSrcElPerGroup = 4;
    static const int NumDstElPerGroup = 4;
    static const int NumGroupsPerOpGeneric = 1;    

public:
    void InitGeneric(ParamType* param, TmpType&)
    {
        UNREFERENCED_PARAMETER(param);
        VT_ASSERT((*param).table.IsInitialized(
            ElTraits<ImplSrcElType>::ElFormat(),
            ElTraits<ImplDstElType>::ElFormat()));
    }

    void EvalGeneric(const ImplSrcElType* pSrc, ImplDstElType* pDst, 
        ParamType* param, TmpType&)
    {
        pDst[0] = (*param).table.Map<ImplDstElType>(pSrc[0]);
        pDst[1] = (*param).table.Map<ImplDstElType>(pSrc[1]);
        pDst[2] = (*param).table.Map<ImplDstElType>(pSrc[2]);
        VtConv(&pDst[3], pSrc[3]);
    }    
};

// MapColorOp specializations for lookup impl
template<typename DstElT>
struct MapColorOp<Byte, DstElT> : public MapColorOpLookupBase<Byte, DstElT> {};
template<typename DstElT>
struct MapColorOp<UInt16, DstElT> 
    : public MapColorOpLookupBase<UInt16, DstElT> {};

}