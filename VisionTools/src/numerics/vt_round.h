//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Support to use the fpu round modes
//
//  History:
//      2002/5/1-mattu
//			Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"

#include <float.h>

#if defined(__clang__)
#include <cfenv>
#if defined(__i386) || defined(__x86_64__)
#include <x86intrin.h>
#endif
#endif

#if defined(_M_ARM) || defined(_M_ARM64)
#include <math.h> // for floorf
#endif

namespace vt {

///////////////////////////////////////////////////////////////////////////
//
// DESCRIPTION
//
//  Use these rountines to efficiently round floating point numbers to
//  integers.  The cast to int of a float causes the compiler to emit
//  a call to the function _ftol2.  This function is horribly inefficient.
//  A better way to do this is to set the X86 FPU to truncate rounding mode
//  then use fistp opcode to store a float as an int, and finally reset the
//  rounding mode to its original state.  This isn't a big win for a single
//  cast, however if you can amortize the cost of setting and restoring the
//  round mode over many float to int conversions then it is about a 4x
//  speedup.  Care must be taken not to do any other floating point ops
//  while round mode is truncate as their results will change from the 
//  default rounding mode.     
//
///////////////////////////////////////////////////////////////////////////

//
// Do not use this class in global scope, it is not thread safe.
// Rather use it as a local variable.
//
// Anywhere you intend to use truncate rounding mode insert an 
// instantiate of the CFPUChop class.

// Instead of using an int cast to convert float to int, call the
// F2I function. 

// After doing all your float to int conversions be careful to restore
// the FPU control using the CFPUChop::Reset function.  Alternatively
// when the CFPUChop class goes out of scope, the round mode is restored
// to its original value.

// If you want to do default rounding (round to nearest away from 0) then you
// don't need to store and restore the FPU, simply call the F2I function.
// For example if your code contains b = BYTE(f + 0.5) where f are positive
// floating point values, this can be replaced with b = F2I(f).  The F2I
// function is much more efficient than the cast.

#if defined(__clang__)

  class CFloatRoundMode {

  public:

    enum RoundMode {
      RM_NEAREST = FE_TONEAREST,
      RM_UP = FE_UPWARD,
      RM_DOWN = FE_DOWNWARD,
      RM_TRUNCATE = FE_TOWARDZERO
    };

    CFloatRoundMode(RoundMode mode)
    : old_(std::fegetround())
    {
      std::fesetround(mode);
    }

    ~CFloatRoundMode()
    {
      std::fesetround(old_);
    }

  private:

    int old_;

  };

inline int F2I(float f)
{
#ifdef __FAST_MATH__
  return __builtin_rintf(f);
#elif defined(__SSE__)
  return _mm_cvtss_si32(_mm_set1_ps(f));
#else
#if !defined(VT_ANDROID)
#warning poor rounding implementation
#endif
  return f + 0.5;
#endif
}

#elif defined(__cplusplus_cli)

inline int F2I(float f)
{
	return int(f + 0.5f);
}

#else

#if ( _MSC_VER < 1400 )

inline unsigned int SetControlFP(unsigned int uVal, unsigned int uMask)
{
    unsigned int uPrevVal;
    uPrevVal = _controlfp(0, 0); 
    _controlfp(uVal, uMask);
    return uPrevVal;
}

#else

inline unsigned int SetControlFP(unsigned int uVal, unsigned int uMask)
{
    unsigned int uNewVal = 0, uPrevVal = 0;
    _controlfp_s(&uPrevVal, 0, 0);
    _controlfp_s(&uNewVal, uVal, uMask);
    return uPrevVal;
}

#endif

//=============================================================================
// CFloatRoundMode
//
// Allows control of the round mode for both x87 FPUs (under x86) and
// SSE2 (under x64). Round mode is restored on destruction.
//=============================================================================
class CFloatRoundMode
{
public:
    //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    // Constants
    //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    enum RoundMode
    {
        RM_NEAREST = 0,
        RM_UP,
        RM_DOWN,
        RM_TRUNCATE
    };

    //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    // Constructor & destructor
    //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    inline CFloatRoundMode(RoundMode rm)
    {
        Set(rm);
    }

    //-------------------------------------------------------------------------
    inline ~CFloatRoundMode()
    {
        Reset();
    }

private:
    //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    // Implementation
    //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    inline void Set(RoundMode rm)
    {
#if defined(_M_AMD64)
        // Save old SSE2 round mode bits in class storage and set
        // rounding mode to a new mode
        m_ulOldRoundBits = _mm_getcsr() & MXCSR_ROUND_MASK;
        unsigned int newMxCSR = _mm_getcsr() & (~MXCSR_ROUND_MASK);
        switch (rm)
        {
        case RM_NEAREST:    newMxCSR |= MXCSR_ROUND_NEAREST; break;
        case RM_UP:         newMxCSR |= MXCSR_ROUND_UP; break;
        case RM_DOWN:       newMxCSR |= MXCSR_ROUND_DOWN; break;
        case RM_TRUNCATE:   newMxCSR |= MXCSR_ROUND_TRUNCATE; break;
        default:
            // Unexpected round mode
            VT_ASSERT(false);
        }
        _mm_setcsr(newMxCSR);
#elif defined(_M_IX86)
        // Save old FPU control word in class storage and set
        // rounding mode to a new mode
        unsigned int mode = 0;
        switch (rm)
        {
        case RM_NEAREST:    mode = RC_NEAR; break;
        case RM_UP:         mode = RC_UP; break;
        case RM_DOWN:       mode = RC_DOWN; break;
        case RM_TRUNCATE:   mode = RC_CHOP; break;
        default:
            // Unexpected round mode
            VT_ASSERT(false);
        }
        m_ulFPU = SetControlFP(mode & _MCW_RC, _MCW_RC);
#elif defined(_M_ARM) || defined(_M_ARM64)
        // TODO: ARM implementation
        rm;
#else
#error Unexpected CPU architecture
#endif
    }

    //-------------------------------------------------------------------------
    inline void Reset()
    {
#if defined(_M_AMD64)
        unsigned int newMxCSR = (_mm_getcsr() & (~MXCSR_ROUND_MASK)) |
            m_ulOldRoundBits;
        _mm_setcsr(newMxCSR);
#elif defined(_M_IX86)
        // Get old rounding mode and restore
        SetControlFP(m_ulFPU & _MCW_RC, _MCW_RC);
#elif defined(_M_ARM) || defined(_M_ARM64)
        // TODO: arm implementation		
#else
#error Unexpected CPU architecture
#endif
    }

private:
    //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    // Private constants
    //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    enum
    {
        MXCSR_ROUND_MASK =       3 << 13,
        MXCSR_ROUND_NEAREST =    0 << 13,
        MXCSR_ROUND_DOWN =       1 << 13,
        MXCSR_ROUND_UP =         2 << 13,
        MXCSR_ROUND_TRUNCATE =   3 << 13
    };

    //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    // Data members
    //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#if defined(_M_AMD64)
    unsigned int m_ulOldRoundBits;
#elif defined(_M_IX86)
    UINT32 m_ulFPU;
#elif defined(_M_ARM) || defined(_M_ARM64)
    // TODO: arm implementation		
#else
#error Unexpected CPU architecture
#endif
};

//
// Convert a float to int using the current FPU/SSE rounding mode. The default
// rounding mode is round to nearest.  If this is the desired conversion then
// simply call this function.  Otherwise use the class above to store & change
// the rounding mode before calling F2I then restore the rounding mode.
//

inline int F2I(float f)
{
#if defined(_M_AMD64)
    // Use SSE intrinsics to round appropriately
    __m128 fp = _mm_load_ss(&f);
    __m128i io = _mm_cvtps_epi32(fp);
    return io.m128i_i32[0];
#elif defined(_M_IX86)
    volatile int n;
    __asm {
        fld   f   // Load float
        fistp n   // Store integer (and pop)
    }
    return n;
#elif defined(_M_ARM) || defined(_M_ARM64)
	return (int)floorf(f + 0.5f);
#else
#error Unexpected CPU architecture
	return (int)floorf(f + 0.5f);
#endif
}

#endif

};

