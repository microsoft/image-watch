//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Wrapper for common SIMD instructions
//
//  History:
//      2011/10/26-mafinch
//			Created
//
//------------------------------------------------------------------------
#pragma once

#if defined(_M_IX86) || defined(_M_AMD64)
#if !defined(__cplusplus_cli)

#include "vt_intrinsics.h"

// Simple class for encapsulating common SSE ops. Very minimal, some constructors to load
// the register, some common 4-float arithmetic ops, and then Unpack back into memory.
#ifndef __clang__
VT_DECLSPEC_ALIGN(16) struct simdReg
#else
struct VT_DECLSPEC_ALIGN(16) simdReg
#endif
{
public:
    // Constructors
    // Initialize from another simdReg.
    simdReg(const simdReg& src)
        : reg(src.reg)
    {
    }
    // Initialize from memory, will load 4 floats
    simdReg(const float* const pSrc)
    {
        reg = _mm_load_ps(pSrc);
    }
    // Load single float replicating into all 4 channels.
    simdReg(float f)
    {
        reg = _mm_set_ps1(f);
    }

    // Pull results back into memory
    void Unpack(float* pDst) const
    {
        _mm_store_ps(pDst, reg);
    }
    // Pull results back into memory
    static inline void Unpack(float* pDst, const simdReg& src)
    {
        src.Unpack(pDst);
    }

    // Arithmetic overloads
	friend simdReg operator +(const simdReg& lhs, const simdReg& rhs) { return _mm_add_ps(lhs.reg, rhs.reg); }
	friend simdReg operator -(const simdReg& lhs, const simdReg& rhs) { return _mm_sub_ps(lhs.reg, rhs.reg); }
	friend simdReg operator *(const simdReg& lhs, const simdReg& rhs) { return _mm_mul_ps(lhs.reg, rhs.reg); }
	friend simdReg operator /(const simdReg& lhs, const simdReg& rhs) { return _mm_div_ps(lhs.reg, rhs.reg); }

	simdReg& operator =(const simdReg& other) { reg = other.reg; return *this; }
	simdReg& operator +=(const simdReg& other) { return *this = _mm_add_ps(reg, other.reg); }
	simdReg& operator -=(const simdReg& other) { return *this = _mm_sub_ps(reg, other.reg); }
	simdReg& operator *=(const simdReg& other) { return *this = _mm_mul_ps(reg, other.reg); }
	simdReg& operator /=(const simdReg& other) { return *this = _mm_div_ps(reg, other.reg); }

private:
    // Initialize from another register.
    simdReg(__m128 v)
        : reg(v)
    {
    }
    simdReg& operator =(const __m128& other) { reg = other; return *this; }
    
    // This could be made public to allow mixing of above overloads and full set of sse ops.
    __m128 reg;
};

#else // Generic CPU, can serve as reference implementation.

struct simdReg
{
public:
    // Constructors
    // Initialize from another simdReg.
    simdReg(const simdReg& src)
    {
        block = src.block;
    }
    // Initialize from memory, will load 4 floats
    simdReg(const float* const pSrc)
    {
        block = *(const Block*)pSrc;
    }
    // Initialize from another register.
    // Load single float replicating into all 4 channels.
    simdReg(float f)
    {
        reg[3] = reg[2] = reg[1] = reg[0] = f;
    }

    // Pull results back into memory
    void Unpack(float* pDst) const
    {
        pDst[0] = reg[0];
        pDst[1] = reg[1];
        pDst[2] = reg[2];
        pDst[3] = reg[3];
    }
    // Pull results back into memory
    static inline void Unpack(float* pDst, const simdReg& src)
    {
        src.Unpack(pDst);
    }

    // Arithmetic overloads
	friend simdReg operator +(const simdReg& lhs, const simdReg& rhs) { return simdReg(lhs) += rhs; }
	friend simdReg operator -(const simdReg& lhs, const simdReg& rhs) { return simdReg(lhs) -= rhs; }
	friend simdReg operator *(const simdReg& lhs, const simdReg& rhs) { return simdReg(lhs) *= rhs; }
	friend simdReg operator /(const simdReg& lhs, const simdReg& rhs) { return simdReg(lhs) /= rhs; }

	simdReg& operator =(const simdReg& rhs) { block = rhs.block; return *this; }
	simdReg& operator +=(const simdReg& rhs) 
    { reg[0] += rhs.reg[0], reg[1] += rhs.reg[1], reg[2] += rhs.reg[2]; reg[3] += rhs.reg[3]; return *this; }
	simdReg& operator -=(const simdReg& rhs)
    { reg[0] -= rhs.reg[0], reg[1] -= rhs.reg[1], reg[2] -= rhs.reg[2]; reg[3] -= rhs.reg[3]; return *this; }
	simdReg& operator *=(const simdReg& rhs)
    { reg[0] *= rhs.reg[0], reg[1] *= rhs.reg[1], reg[2] *= rhs.reg[2]; reg[3] *= rhs.reg[3]; return *this; }
	simdReg& operator /=(const simdReg& rhs)
    { reg[0] /= rhs.reg[0], reg[1] /= rhs.reg[1], reg[2] /= rhs.reg[2]; reg[3] /= rhs.reg[3]; return *this; }

private:
    struct Block { float chunk[4]; };
    union 
    {
        float reg[4];
        Block block;
    };
};

#endif // C++/CLI
#endif // Generic CPU