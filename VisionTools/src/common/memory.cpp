//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Class to represent reference counted memory blocks
//
//  History:
//      2004/11/08-swinder
//			Created
//
//------------------------------------------------------------------------

#include "vt_dbg.h"
#include "vt_new.h"
#include "vt_memory.h"
#include "vt_cpudetect.h"
#include "vt_intrinsics.h"
#include <string.h>

using namespace vt;

template<bool BypassCache>
void *VtMemsetIntrnl(_Out_writes_bytes_(count) void *pDest, int c, size_t count)
{
	Byte *pbDest = (Byte *)pDest;

	if(count>=128)
	{
		if(g_SupportSSE2())
		{
#if (defined(_M_IX86) || defined(_M_AMD64))
			// align 16-byte
			for(; count>0 && ((Int64)pbDest & 15); count--)
				*pbDest++ = (Byte)c;

			__m128i x0i = _mm_set1_epi8((char)c);

			for(; count>=16; count-=16, pbDest+=16)
			{
				StoreIntrinsicSSE<BypassCache>::StoreAligned(pbDest, x0i);
			}
#endif
		}
		else
		{
			// align 4-byte
			for(; count>0 && ((Int64)pbDest & 3); count--)
				*pbDest++ = (Byte)c;

			Byte bc = (Byte)c;
			unsigned int u = bc | (bc<<8) | (bc<<16) | (bc<<24);

			for(; count>=16; count-=16, pbDest+=16)
			{
				((unsigned int *)pbDest)[0] = u;
				((unsigned int *)pbDest)[1] = u;
				((unsigned int *)pbDest)[2] = u;
				((unsigned int *)pbDest)[3] = u;
			}
		}
	}

	for(; count>0; count--)
		*pbDest++ = (Byte)c;

	return pDest;
}

void *vt::VtMemset(_Out_writes_bytes_(count) void *pDest, int c, size_t count, bool bBypassCache)
{
	return bBypassCache?
		VtMemsetIntrnl<true>(pDest, c, count):
		VtMemsetIntrnl<false>(pDest, c, count);
}

void *VtMemcpyBypassCache(_Out_writes_bytes_(count) void *pDest, _In_reads_bytes_(count) const void *pSrc, size_t count)
{
#if (defined(_M_IX86) || defined(_M_AMD64))
	Byte *pbDest = (Byte *)pDest;
	const Byte *pbSrc = (Byte *)pSrc;

	if(g_SupportSSE2() && (count>=16))
	{
		// align 16-byte
		for(; count>0 && (INT_PTR(pbDest) & 15); count--)
			*pbDest++ = *pbSrc++;

        if( (INT_PTR(pbSrc) & 0xf) != 0 )
        {
            for(; count>=16; count-=16, pbDest+=16, pbSrc+=16)
            {
                __m128i x0i = _mm_loadu_si128((__m128i *)pbSrc);
                StoreIntrinsicSSE<true>::StoreAligned(pbDest, x0i);
            }
        }
        else
        {
            for(; count>=16; count-=16, pbDest+=16, pbSrc+=16)
            {
                __m128i x0i = _mm_load_si128((__m128i *)pbSrc);
                StoreIntrinsicSSE<true>::StoreAligned(pbDest, x0i);
            }
        }
    	for(; count>0; count--)
	    	*pbDest++ = *pbSrc++;
    }
	else
#endif
	{
// TODO: on Arm, force use of Neon copy to bypass L0 cache
        memcpy(pDest, pSrc, count);
	}

	return pDest;
}

void *vt::VtMemcpy(_Out_writes_bytes_(count) void *pDest, _In_reads_bytes_(count) const void *pSrc, size_t count, bool bBypassCache)
{
    if (bBypassCache) 
    {
		return VtMemcpyBypassCache(pDest, pSrc, count);
    }
    else
    {
        return memcpy(pDest, pSrc, count);
    }
}

#pragma warning(push)
#pragma warning(disable:22105)
#pragma warning(disable:26019)
// suppress false positive overflow warnings in OACR and VS:
#pragma warning(disable:26015)
#pragma warning(disable:6386)
template<bool BypassCache>
void *VtFillSpanIntrnl(_Out_writes_bytes_(iCount*uiElSize) void *pDest,
                       _In_reads_bytes_(uiElSize) const void *pSrcEl,
                       size_t uiElSize,
					   int iCount)
{
    VT_ASSERT(iCount >= 0);
    Byte *pOut = (Byte *)pDest;
    if( iCount == 0 )
        return pDest;

	if( g_SupportSSE2() )
	{   
#if (defined(_M_IX86) || defined(_M_AMD64))
        int iByteCount;
        __m128i x0i;

        switch( uiElSize )
        {
        case 1:
            x0i = _mm_set1_epi8(*(Byte *)pSrcEl);
            iByteCount = iCount;
            iCount &= 0xf;
            break;
        case 2:
            x0i = _mm_set1_epi16(*(short *)pSrcEl);
            iByteCount = iCount*2;
            iCount &= 0x7;
            break;
        case 4:
            x0i = _mm_set1_epi32(*(int *)pSrcEl);
            iByteCount = iCount*4;
            iCount &= 0x3;
            break;
        case 8:
            x0i = _mm_loadl_epi64((__m128i *)pSrcEl);
            x0i = _mm_unpacklo_epi64(x0i, x0i);
            iByteCount = iCount*8;
            iCount &= 0x1;
            break;
        case 16:
            x0i = _mm_loadu_si128((__m128i *)pSrcEl);
            iByteCount = iCount*16;
            iCount = 0;
            break;
        default:
            goto skipsse;
            break;
        }

        if( !IsAligned16(pOut) )
        {
            while( iByteCount >= 16 )
            {
                _mm_storeu_si128((__m128i*)pOut, x0i);
                pOut += 16;
                iByteCount -= 16;
            }
        }
        else
        {
            while( iByteCount >= 16 )
            {
                StoreIntrinsicSSE<BypassCache>::StoreAligned(pOut, x0i);
                pOut += 16;
                iByteCount -= 16;
            }
        }
#endif
	}

    switch( uiElSize )
    {
    case 1:
        memset(pOut, *(Byte *)pSrcEl, iCount);
        break;
    case 2:
        while( iCount )
        {
            *(short *)pOut = *(short *)pSrcEl;
            pOut += 2;
            --iCount;
        }
        break;
    case 4:
        while( iCount )
        {
            *(int *)pOut = *(int *)pSrcEl;
            pOut += 4;
            --iCount;
        }
        break;
    case 8:
        while( iCount )
        {
            ((int *)pOut)[0] = ((int *)pSrcEl)[0];
            ((int *)pOut)[1] = ((int *)pSrcEl)[1];
            pOut += 8;
            --iCount;
        }
        break;
    case 16:
        while( iCount )
        {
            ((int *)pOut)[0] = ((int *)pSrcEl)[0];
            ((int *)pOut)[1] = ((int *)pSrcEl)[1];
            ((int *)pOut)[2] = ((int *)pSrcEl)[2];
            ((int *)pOut)[3] = ((int *)pSrcEl)[3];
            pOut += 16;
            --iCount;
        }
        break;
    default:
#if (defined(_M_IX86) || defined(_M_AMD64))
    skipsse:
#endif
        while( iCount )
        {
            memcpy(pOut, pSrcEl, uiElSize);
            pOut += uiElSize;
            --iCount;
        }
        break;
    }

	return pDest;
}
#pragma warning(pop)

void *vt::VtFillSpan(_Out_writes_bytes_(iCount*uiElSize) void *pDest,  _In_reads_bytes_(uiElSize) const void *pSrcEl, size_t uiElSize,
					 int iCount, bool bBypassCache)
{
	return bBypassCache? 
		VtFillSpanIntrnl<true>(pDest, pSrcEl, uiElSize, iCount):
		VtFillSpanIntrnl<false>(pDest, pSrcEl, uiElSize, iCount);
}

////////////////////////////////////////////////////////////////////////
// CMemShare
////////////////////////////////////////////////////////////////////////

Byte *CMemShare::Alloc(size_t uiBytes, AlignMode eAlign, bool bClearMem)
{
	size_t uiTotalBytes;
	if(eAlign==align64Byte || eAlign==align64ByteRows)
		uiTotalBytes = ((uiBytes + 0x3f) & (~0x3f)) + 0x40; // round up to multiple of 64 and add 64 for start alignment
    else if(eAlign==align16Byte || eAlign==align16ByteRows)
        uiTotalBytes = ((uiBytes + 0xf) & (~0xf)) + 0x10; // round up to multiple of 16 and add 16 for start alignment
	else
		uiTotalBytes = uiBytes;
	
	// avoid reallocation if nothing changes
	if(m_pbAlloc==NULL || m_uiSize!=uiTotalBytes)
	{
		Byte *pbAlloc = VT_NOTHROWNEW Byte [uiTotalBytes];
		if(!pbAlloc)
			return NULL;
		if(m_pbAlloc)
			delete [] m_pbAlloc;
		m_uiSize = uiTotalBytes;
		m_pbAlloc = pbAlloc;
	}

	if(eAlign==align64Byte || eAlign==align64ByteRows)
        m_pbData = (Byte *)((INT_PTR(m_pbAlloc) + 0x3f) & (~0x3f)); // align pointer to 64 byte boundary
	else if(eAlign==align16Byte || eAlign==align16ByteRows)
		m_pbData = (Byte *)((INT_PTR(m_pbAlloc) + 0xf) & (~0xf)); // align pointer to 16 byte boundary
	else
		m_pbData = m_pbAlloc;

	if(bClearMem)
		VtMemset(m_pbData, 0, uiBytes * sizeof(Byte));

	return m_pbData;
}

void CMemShare::Use(Byte *pData)
{
	delete [] m_pbAlloc;
	m_pbAlloc = NULL;
	m_pbData = pData;
	m_uiSize = 0;
}



