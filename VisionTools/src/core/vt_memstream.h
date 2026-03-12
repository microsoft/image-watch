//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Implementation of a memory object with IStream interface
//
//  History:
//      2005/1/31-swinder
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include <ObjIdl.h>

namespace vt {

#define MEMSTREAM_DEFAULT_BLOCKSIZE 65536

class CMemStream : public IStream
{
public:
    CMemStream();
    ~CMemStream();
    
    HRESULT Create(void *pMemory, size_t uiSize, size_t uiBlockSize = MEMSTREAM_DEFAULT_BLOCKSIZE);
    HRESULT Create(size_t uiSize = 0, size_t uiBlockSize = MEMSTREAM_DEFAULT_BLOCKSIZE);
    void Free();
	void Clear(); // clears data and sets pointers to start, keeps memory allocation
    HRESULT ReadFromFile(HANDLE h); // read entire memory block from file
    HRESULT WriteToFile(HANDLE h); // write entire memory block to file
    size_t Size() { return (size_t)m_stat.cbSize.QuadPart; }
    Byte & operator[] (size_t uiLoc); // do not make a pointer to this (memory blocks are not contiguous)

// IStream
    STDMETHODIMP           QueryInterface ( REFIID riid, void **ppvObject );
    STDMETHODIMP_(ULONG)   AddRef  ( );
    STDMETHODIMP_(ULONG)   Release ( );

	STDMETHODIMP Read(_Out_writes_bytes_to_(cb, *pcbRead)  void *pv,
					  _In_  ULONG cb,
					  _Out_opt_  ULONG *pcbRead);

    STDMETHODIMP Write (_In_reads_bytes_(cb)  const void *pv,
						_In_  ULONG cb,
						_Out_opt_  ULONG *pcbWritten );

    STDMETHODIMP Seek ( /* [in] */ LARGE_INTEGER dlibMove,
                        /* [in] */ DWORD dwOrigin,
                        /* [annotation] */ 
                        _Out_opt_  ULARGE_INTEGER *plibNewPosition );

    STDMETHODIMP Stat ( /* [out] */ __RPC__out STATSTG *pstatstg,
                        /* [in] */ DWORD grfStatFlag );

    STDMETHODIMP SetSize ( /* [in] */ ULARGE_INTEGER /*libNewSize*/ )
    { return E_NOTIMPL; }

    STDMETHODIMP CopyTo ( /* [annotation][unique][in] */ 
		                  _In_  IStream*,
                          /* [in] */ ULARGE_INTEGER,
						  /* [annotation] */ 
						  _Out_opt_  ULARGE_INTEGER*,
						  /* [annotation] */ 
						  _Out_opt_  ULARGE_INTEGER* )
    { return E_NOTIMPL; }

    STDMETHODIMP Commit ( /* [in] */ DWORD /*grfCommitFlags*/ )
    { return E_NOTIMPL; }

    STDMETHODIMP Revert ( )
    { return E_NOTIMPL; }

    STDMETHODIMP LockRegion ( /* [in] */ ULARGE_INTEGER /*libOffset*/,
                              /* [in] */ ULARGE_INTEGER /*cb*/,
                              /* [in] */ DWORD /*dwLockType*/ )
    { return E_NOTIMPL; }

    STDMETHODIMP UnlockRegion ( /* [in] */ ULARGE_INTEGER /*libOffset*/,
                                /* [in] */ ULARGE_INTEGER /*cb*/,
                                /* [in] */ DWORD /*dwLockType*/ )
    { return E_NOTIMPL; }

    STDMETHODIMP Clone ( /* [out] */ __RPC__deref_out_opt IStream ** /*ppstm*/ )
    { return E_NOTIMPL; }

private:
    FILETIME CMSGetCurrentTime();
    HRESULT Grow(size_t cb);

    bool m_bAlloc;
    Byte m_bDummy;
    size_t m_uiBlockSize;
    LONG           m_cRef;
    vector<Byte *> m_vMem;
    ULARGE_INTEGER m_rwCursor;
	ULARGE_INTEGER m_cbAllocSize;
    STATSTG        m_stat;
};

};
