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

#pragma once

#include "vt_basetypes.h"
#include "vt_intrinsics.h"

namespace vt {

void *VtMemset(_Out_writes_bytes_(count) void *pDest, int c, size_t count, bool bBypassCache=false);

void *VtMemcpy(_Out_writes_bytes_(count) void *pDest, _In_reads_bytes_(count) const void *pSrc, size_t count, 
			   bool bBypassCache=false);

void *VtFillSpan(_Out_writes_bytes_(iCount*uiElSize) void *pDest,  _In_reads_bytes_(uiElSize) const void *pSrcEl, size_t uiElSize, int iCount,
				 bool bBypassCache=false);

/// \ingroup pixformat
/// \enum AlignMode
/// <summary> Memory alignment modes. </summary>
enum AlignMode 
{
	alignAny, 	///< don't force alignment
	align16Byte,   ///< pointer to image is aligned to 16 bytes
	align16ByteRows,    ///< pointer to start of each row is aligned to 16 bytes
    align64Byte,        ///< pointer to image is aligned to 64 bytes
    align64ByteRows     ///< pointer to start of each row is aligned to 64 bytes
};

/// \ingroup basicdefs
/// \var DEFAULT_ALIGN
/// <summary> Default memory alignment mode </summary>
const AlignMode DEFAULT_ALIGN = align64ByteRows; // best for image processing ops (sse & cache)

inline bool IsAligned4(const void *p) { return (((INT_PTR)(p))&3)==0; }

inline bool IsAligned16(INT_PTR p) { return (p & 15) == 0; }

inline bool IsAligned16(const void *p) { return IsAligned16((INT_PTR)(p)); }

inline bool IsAligned32(const void *p) { return (((INT_PTR)(p))&31)==0; }

inline bool IsAligned64(const void *p) { return (((INT_PTR)(p))&63)==0; }

class CMemShare
{
public:
	CMemShare() : m_pbData(NULL), m_pbAlloc(NULL), m_iRef(1), m_uiSize(0)
    {}

    ~CMemShare()
    { delete [] m_pbAlloc; }

    long AddRef()
	{ return VT_INTERLOCKED_INCREMENT(&m_iRef); }

    long Release()
    {
        long iRef = VT_INTERLOCKED_DECREMENT(&m_iRef);
        if(iRef == 0)
            delete this;
        return iRef;
    }

    long RefCount() const
    { return m_iRef; }

    Byte *Alloc(size_t uiBytes, AlignMode eAlign = DEFAULT_ALIGN, 
                bool bClearMem = false);

    void Use(Byte *pData);

    Byte *Ptr()
    { return m_pbData; }

    const Byte *Ptr() const 
    { return m_pbData; }

    bool IsWrappingMemory() const 
    { return m_pbAlloc==NULL && m_pbData!=NULL; }

private:
	Byte *m_pbData;
	Byte *m_pbAlloc;
	long m_iRef;
	size_t m_uiSize;
};

};
