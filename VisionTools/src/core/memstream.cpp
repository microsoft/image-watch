//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Implementation of memory object with IStream interface
//
//  History:
//      2005/1/30-swinder
//          Created
//
//------------------------------------------------------------------------

#include "stdafx.h"
#include "vt_memstream.h"

using namespace vt;

CMemStream::CMemStream()
: m_bDummy(0), m_bAlloc(true), m_cRef(1), m_uiBlockSize(MEMSTREAM_DEFAULT_BLOCKSIZE)
{
    m_stat.pwcsName        = NULL; 
    m_stat.type            = STGTY_STREAM;
    m_stat.cbSize.QuadPart = 0; // number of valid bytes in buffer
    m_stat.mtime = m_stat.ctime = m_stat.atime = CMSGetCurrentTime(); 
    m_stat.grfMode           = OF_READWRITE;
    m_stat.grfLocksSupported = 0;
    m_stat.clsid             = CLSID_NULL; 
    m_stat.grfStateBits      = 0;
    m_stat.reserved          = 0;

    m_rwCursor.QuadPart = 0; // position of read and write pointer
	m_cbAllocSize.QuadPart = 0; // number of bytes of memory allocated
}

CMemStream::~CMemStream()
{
    Free();
}

void CMemStream::Free()
{
    m_stat.cbSize.QuadPart = 0;
	m_cbAllocSize.QuadPart = 0; 

    int i;
    if(m_bAlloc) // delete memory blocks if we allocated them
        for(i=0; i<(int)m_vMem.size(); i++)
            if(m_vMem[i]!=NULL)
                delete [] m_vMem[i];

    m_bAlloc = true;
    m_vMem.clear();
    m_rwCursor.QuadPart = 0;
}

void CMemStream::Clear()
{
	m_stat.cbSize.QuadPart = 0;
	m_rwCursor.QuadPart = 0;
}

// this function is used to wrap a memory block which is typically full of data
// and you would like to read it from the beginning.
HRESULT CMemStream::Create(void *pMemory, size_t uiSize, size_t uiBlockSize)
{
    HRESULT hr = NOERROR;
    
    Free();

    if(pMemory==NULL)
        VT_HR_EXIT(E_POINTER);
    if(uiBlockSize<256)
        uiBlockSize = 256;

    m_stat.cbSize.QuadPart = uiSize; // we are wrapping so wrap full of valid bytes
	m_cbAllocSize.QuadPart = uiSize; // set bytes allocted
    m_stat.mtime = m_stat.ctime = m_stat.atime = CMSGetCurrentTime(); 
    m_bAlloc = false;// show that we are wrapping memory
    m_uiBlockSize = uiBlockSize;

    size_t uiCount;
    size_t uiBlocks = (uiSize + uiBlockSize - 1)/uiBlockSize;
    VT_HR_EXIT( m_vMem.resize(uiBlocks) );

    Byte *pb = (Byte *)pMemory;
    for(uiCount = 0; uiCount<uiBlocks; uiCount++, pb += uiBlockSize)
        m_vMem[(int)uiCount] = pb; // save array of pointers to blocks

Exit:
    return hr;
}

// this function is used to create and empty buffer with a initial size, which is
// dynamically grown as needed.
HRESULT CMemStream::Create(size_t uiSize, size_t uiBlockSize)
{
    HRESULT hr = NOERROR;
    
    Free();

    if(uiBlockSize<256)
        uiBlockSize = 256;
    
    m_stat.cbSize.QuadPart = 0; // create empty
	m_cbAllocSize.QuadPart = uiSize; // set number of bytes allocated
    m_stat.mtime = m_stat.ctime = m_stat.atime = CMSGetCurrentTime(); 
    m_uiBlockSize = uiBlockSize;

    if(uiSize>0)
    {
        size_t uiCount;
        size_t uiBlocks = (uiSize + uiBlockSize - 1)/uiBlockSize;
        VT_HR_EXIT( m_vMem.resize(uiBlocks) );

        memset(&m_vMem[0], 0, uiBlocks * sizeof(void *));

        for(uiCount = 0; uiCount<uiBlocks; uiCount++)
        {
            // Disabling C28193 here as it's a false positive from the compiler.
#pragma warning(push)
#pragma warning(disable: 28193)
            m_vMem[(int)uiCount] = VT_NOTHROWNEW Byte [uiBlockSize];
#pragma warning(pop)
            if(m_vMem[(int)uiCount]==NULL)
                VT_HR_EXIT( E_OUTOFMEMORY );
        }
    }

Exit:
    return hr;
}

Byte & CMemStream::operator [](size_t uiLoc)
{
    if(uiLoc >= m_cbAllocSize.QuadPart)
        return m_bDummy;

    size_t uiBlock = uiLoc / m_uiBlockSize;
    size_t uiOffset = uiLoc - uiBlock * m_uiBlockSize;

    if(uiBlock>=m_vMem.size() || m_vMem[(int)uiBlock]==NULL)
        return m_bDummy;

    return *(m_vMem[(int)uiBlock] + uiOffset);
}

HRESULT CMemStream::ReadFromFile(HANDLE h)
{
    HRESULT hr = NOERROR;
    DWORD dwRead;

	// the size that is read from the file is the size of the allocated memory buffer
	// which is typically set on the call to create
    if(m_cbAllocSize.QuadPart==0)
        goto Exit;

    if(m_bAlloc)
    {
        // read one block at a time
        int i;
        size_t uiCount = 0;
        for(i=0; i<(int)m_vMem.size(); i++, uiCount += m_uiBlockSize)
        {
            size_t uiRemainder = (size_t)m_cbAllocSize.QuadPart - uiCount;
            if(uiRemainder > m_uiBlockSize)
                uiRemainder = m_uiBlockSize;
            if(!ReadFile(h, m_vMem[i], (DWORD)uiRemainder, &dwRead, NULL))
                VT_HR_EXIT( HRESULT_FROM_WIN32(GetLastError()) );
            if(dwRead != uiRemainder)
                VT_HR_EXIT( E_READFAILED );
        }
    }
    else
    {
        // all blocks contiguous
        if(!ReadFile(h, m_vMem[0], (DWORD)m_cbAllocSize.QuadPart, &dwRead, NULL))
            VT_HR_EXIT( HRESULT_FROM_WIN32(GetLastError()) );
        if(dwRead != (DWORD)m_stat.cbSize.QuadPart)
            VT_HR_EXIT( E_READFAILED );
    }
	
	m_stat.cbSize.QuadPart = m_cbAllocSize.QuadPart; // set the number of valid bytes
Exit:
    return hr;
}

HRESULT CMemStream::WriteToFile(HANDLE h)
{
    HRESULT hr = NOERROR;
    DWORD dwWritten;

	// the size that is written to file is the size of the number of valid bytes
    if(m_stat.cbSize.QuadPart==0)
        goto Exit;

    if(m_bAlloc)
    {
        // write one block at a time
        int i;
        size_t uiCount = 0;
		size_t uiWriteSize;
        for(i=0; i<(int)m_vMem.size(); i++, uiCount += uiWriteSize)
        {
            uiWriteSize = (size_t)m_stat.cbSize.QuadPart - uiCount;
			if(uiWriteSize==0)
				break;

            if(uiWriteSize > m_uiBlockSize)
                uiWriteSize = m_uiBlockSize;

            if(!WriteFile(h, m_vMem[i], (DWORD)uiWriteSize, &dwWritten, NULL))
                VT_HR_EXIT( HRESULT_FROM_WIN32(GetLastError()) );

            if(dwWritten != uiWriteSize)
                VT_HR_EXIT( E_WRITEFAILED );
        }
    }
    else
    {
        // all blocks contiguous
        if(!WriteFile(h, m_vMem[0], (DWORD)m_stat.cbSize.QuadPart, &dwWritten, NULL))
            VT_HR_EXIT( HRESULT_FROM_WIN32(GetLastError()) );
        if(dwWritten != (DWORD)m_stat.cbSize.QuadPart)
            VT_HR_EXIT( E_READFAILED );
    }

Exit:
    return hr;
}

HRESULT CMemStream::QueryInterface ( REFIID riid, void **ppvObject )
{
    HRESULT hr = S_OK;
    if (ppvObject == 0)
    {
        hr = E_POINTER;
    }
    else if (riid == IID_IUnknown || riid == IID_IStream )
    {
        *ppvObject = this;
        AddRef();
    }
    else
    {
        *ppvObject = 0;
        hr = E_NOINTERFACE;
    }
    return hr;
}
    
ULONG CMemStream::AddRef()
{
    return InterlockedIncrement( &m_cRef );
}
    
ULONG CMemStream::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
        delete this;
    return cRef;
}

HRESULT CMemStream::Read (_Out_writes_bytes_to_(cb, *pcbRead)  void *pv,
						  _In_ ULONG cb,
						  _Out_opt_  ULONG *pcbRead )
{
    HRESULT hr = NOERROR;

    if( pv == NULL )
        VT_HR_EXIT(STG_E_INVALIDPOINTER);

    m_stat.atime = CMSGetCurrentTime();
        
	// dont read more than the number of valid bytes
    if( (m_rwCursor.QuadPart + cb) > m_stat.cbSize.QuadPart )
    {
		if(m_rwCursor.QuadPart > m_stat.cbSize.QuadPart)
			cb = 0;
		else
			cb = (ULONG)( m_stat.cbSize.QuadPart - m_rwCursor.QuadPart );
    }

    if(cb>0)
    {
        size_t uiBlock = (size_t)(m_rwCursor.QuadPart / m_uiBlockSize);
        size_t uiOffset = (size_t)(m_rwCursor.QuadPart - uiBlock * m_uiBlockSize);
        size_t cbtmp = cb;

        do {
            size_t uiCopyCount = m_uiBlockSize - uiOffset;
            uiCopyCount = VtMin(uiCopyCount, cbtmp);
            ANALYZE_ASSUME(uiCopyCount <= cbtmp);

            if(m_vMem.size()<=uiBlock || m_vMem[(int)uiBlock]==NULL)
            {
                // only happens if someone ignores the error state of a create call
                cb = 0;
                hr = S_FALSE;
                goto Fail;
            }
            memcpy(pv, m_vMem[(int)uiBlock] + uiOffset, uiCopyCount);
            cbtmp -= uiCopyCount;
            uiOffset += uiCopyCount;
            pv = (void *)((Byte *)pv + uiCopyCount);
            if(uiOffset==m_uiBlockSize)
            {
                uiOffset = 0;
                uiBlock++;
            }
        } while(cbtmp > 0);
        
        m_rwCursor.QuadPart += cb;
    }

Fail:
    if(pcbRead)
        *pcbRead = cb;

Exit:
    return hr;
}

HRESULT CMemStream::Write (_In_reads_bytes_(cb) const void *pv, 
  						   _In_  ULONG cb,
						   _Out_opt_  ULONG *pcbWritten )
{
    HRESULT hr = NOERROR;

    if( pv == NULL )
        VT_HR_EXIT(STG_E_INVALIDPOINTER);

    m_stat.atime = m_stat.mtime = CMSGetCurrentTime();

    if( (m_rwCursor.QuadPart + cb) > m_cbAllocSize.QuadPart )
    {
        if(m_bAlloc)
        {
            // allocate more memory
            VT_HR_EXIT( Grow((size_t)cb) );
        }
        else
		{
			if(m_rwCursor.QuadPart > m_cbAllocSize.QuadPart)
				cb = 0; // I dont think this can happen actually - swinder
			else
				// as many bytes as we can manage
				cb = (ULONG)( m_cbAllocSize.QuadPart - m_rwCursor.QuadPart );
		}
    }

    if(cb>0)
    {
        size_t uiBlock = (size_t)(m_rwCursor.QuadPart / m_uiBlockSize);
        size_t uiOffset = (size_t)(m_rwCursor.QuadPart - uiBlock * m_uiBlockSize);
        size_t cbtmp = cb;

        do {
            size_t uiCopyCount = m_uiBlockSize - uiOffset;
            uiCopyCount = VtMin(uiCopyCount, cbtmp);

            if(m_vMem.size()<=uiBlock || m_vMem[(int)uiBlock]==NULL)
            {
                // only happens if someone ignores the error state of a create call
                cb = 0;
                hr = STG_E_MEDIUMFULL;
                goto Fail;
            }
            memcpy(m_vMem[(int)uiBlock] + uiOffset, pv, uiCopyCount);
            cbtmp -= uiCopyCount;
            uiOffset += uiCopyCount;
            pv = (void *)((Byte *)pv + uiCopyCount);
            if(uiOffset==m_uiBlockSize)
            {
                uiOffset = 0;
                uiBlock++;
            }
        } while(cbtmp > 0);

        m_rwCursor.QuadPart += cb;

		// grow the valid size
		if(m_rwCursor.QuadPart > m_stat.cbSize.QuadPart)
			m_stat.cbSize.QuadPart = m_rwCursor.QuadPart;
    }

Fail:
    if(pcbWritten)
        *pcbWritten = cb;

Exit:
    return hr;
}

HRESULT CMemStream::Seek ( LARGE_INTEGER dlibMove,
						   DWORD dwOrigin,
						   _Out_opt_ ULARGE_INTEGER *plibNewPosition )
{
    HRESULT hr = S_OK;    
    ULARGE_INTEGER newCursor = m_rwCursor;

    switch( dwOrigin )
    {
    case STREAM_SEEK_SET:
        newCursor.QuadPart = dlibMove.QuadPart;
        break;
    case STREAM_SEEK_CUR:
        newCursor.QuadPart += dlibMove.QuadPart;
        break;
    case STREAM_SEEK_END:
        newCursor.QuadPart = m_stat.cbSize.QuadPart + dlibMove.QuadPart; 
        break;
    default:
        hr = STG_E_INVALIDFUNCTION;
        break;
    }

    // first check if pointer wrapped around 0
    if( dlibMove.QuadPart < 0 && 
        (newCursor.QuadPart > m_rwCursor.QuadPart || dwOrigin == STREAM_SEEK_SET) )
    {
        VT_HR_EXIT(STG_E_INVALIDFUNCTION);
    }

    if( newCursor.QuadPart > m_cbAllocSize.QuadPart )
    {
        // may need to resize memory
        if(m_bAlloc)
        {
            VT_HR_EXIT( Grow((size_t)(newCursor.QuadPart - m_cbAllocSize.QuadPart)) );
            m_rwCursor = newCursor;
        }
        else
            m_rwCursor.QuadPart = m_cbAllocSize.QuadPart; // hit the end
    }
    else
    {
        m_rwCursor = newCursor;
    }

	// grow the size? probably not on seek because seek doesnt add useful data
	// also do we put zeros in if we extend?
	//if(m_rwCursor.QuadPart > m_stat.cbSize.QuadPart)
	//	m_stat.cbSize.QuadPart = m_rwCursor.QuadPart;

    m_stat.atime = CMSGetCurrentTime();
    if( plibNewPosition )
    {
        plibNewPosition->QuadPart = m_rwCursor.QuadPart;
    }

Exit:
    return hr;
}

HRESULT CMemStream::Stat ( __RPC__out STATSTG *pstatstg, 
						   DWORD grfStatFlag )
{
    HRESULT hr;

    if( pstatstg == NULL )
    {
        hr = STG_E_INVALIDPOINTER;
    }
    else if( grfStatFlag != STATFLAG_DEFAULT &&
             grfStatFlag != STATFLAG_NONAME )
    {
        hr = STG_E_INVALIDFLAG;
    }
    else
    {
        *pstatstg = m_stat;
        if(grfStatFlag == STATFLAG_DEFAULT)
        {
            // should perhaps allocate a dummy name here with CoTaskMemAlloc
            // however its not clear that anyone cares about the name
            // and CoTaskMemFree can be passed a NULL pointer
        }
        hr = S_OK;
    }
    return hr;
}

FILETIME CMemStream::CMSGetCurrentTime()
{
    SYSTEMTIME systime;
    FILETIME   ftime;
    FILETIME   fzero = { 0 };
    GetSystemTime(&systime);
    return SystemTimeToFileTime(&systime, &ftime) ?
        ftime : fzero;
}

HRESULT CMemStream::Grow(size_t cb)
{
    HRESULT hr = NOERROR;

    if(m_bAlloc)
    {
        // excess bytes that we already have allocated
        size_t uiBytesSoFar = (size_t)(m_vMem.size() * m_uiBlockSize - m_cbAllocSize.QuadPart);
        if(uiBytesSoFar >= cb)
            goto Update; // no need to add to it

        size_t uiTarget = (size_t)(m_cbAllocSize.QuadPart + cb);
        uiTarget += uiTarget/7; // grow by about 14% more
        size_t uiBlocks = (uiTarget + m_uiBlockSize - 1)/m_uiBlockSize - m_vMem.size();
        
        size_t ui;
        for(ui=0; ui<uiBlocks; ui++)
        {
            Byte *p = VT_NOTHROWNEW Byte [m_uiBlockSize];
            if(p==NULL)
            {
                if(uiBytesSoFar >= cb) 
                    // dont signal error if we already have enough
                    goto Update;
                VT_HR_EXIT( STG_E_MEDIUMFULL );
            }
            if(FAILED(m_vMem.push_back(p)))
            {
                delete [] p;
                if(uiBytesSoFar >= cb)
                    goto Update;
                VT_HR_EXIT( STG_E_MEDIUMFULL );
            }
            uiBytesSoFar += m_uiBlockSize;
        }
Update:
        m_cbAllocSize.QuadPart += cb;
    }

Exit:
    return hr;
}
