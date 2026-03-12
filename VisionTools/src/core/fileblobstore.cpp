//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Helper class for reading and writing data store blobs.
//
//  History:
//      2011/2/16-v-hogood
//          Created
//
//------------------------------------------------------------------------
#include "stdafx.h"

#if !defined(VT_WINRT)

#include <shlwapi.h>

#include "vt_utils.h"
#include "vt_blobstore.h"
#include "vt_fileblobstore.h"

// macros from <intsafe.h>
#define LODWORD(_qw)    ((DWORD)(_qw))
#define HIDWORD(_qw)    ((DWORD)(((_qw) >> 32) & 0xffffffff))

using namespace vt;


typedef struct _Blob
{
    HANDLE hBlobFile;

    UINT uMaxTracts;
    UINT uBlobStoreIndex;
    FILE_BLOB_STORE BlobFile;

    _Blob() : hBlobFile(NULL), uBlobStoreIndex(UINT_MAX), uMaxTracts(0) {};
} Blob;


typedef struct _BLOBCALLBACK
{
    OVERLAPPED overlapped;

    // if in use, pointer to the user callback function
    // otherwise, pointer to the next callback in empty list
    union 
    {
        vt::IBlobCallback* cbfn;
        _BLOBCALLBACK*     next;
    };

    _BLOBCALLBACK() : cbfn(NULL)
    { memset(&overlapped, 0, sizeof(OVERLAPPED)); }
} BLOBCALLBACK;


VOID CALLBACK
VtFileIOCompletionRoutine(DWORD dwError, DWORD /*cbTransferred*/, 
                          LPOVERLAPPED lpOverlapped) 
{
    BLOBCALLBACK* pCallback = (BLOBCALLBACK *) lpOverlapped;
    
    if (pCallback->cbfn != NULL)
    {
        pCallback->cbfn->Callback(HRESULT_FROM_WIN32( dwError ));

        ((CFileBlobStore *)
            pCallback->overlapped.hEvent)->BlobIOCompletionRoutine(pCallback);
    }
    else
    {
        // Indicate synchronous IO is complete.
        SetEvent(pCallback->overlapped.hEvent);
    }
}


CFileBlobStore::CFileBlobStore() : m_pCallbacks(NULL), m_tractSize(0)
{
    // Set temp directory as default backing file path.
    wstring_b<MAX_PATH> wstrTempPath;
	if (CSystem::GetTempPath(MAX_PATH, wstrTempPath.get_buffer()) &&
        m_vecBlobFilePaths.resize(1) == S_OK)
    {
        m_vecBlobFilePaths[0].uBlobStoreTracts = 0;
        m_vecBlobFilePaths[0].strBlobStorePath = wstrTempPath;
    }

    // From MSDN "File Buffering":
    // Therefore, in most situations, page-aligned memory will also be sector-aligned,
    // because the case where the sector size is larger than the page size is rare.
    SYSTEM_INFO si;
    vt::CSystem::GetSystemInfo(&si);
    m_tractSize = si.dwPageSize;
    m_stripe = -1;
}

CFileBlobStore::~CFileBlobStore()
{
    while (m_pCallbacks != NULL)
    {
        BLOBCALLBACK* pCallback = (BLOBCALLBACK *) m_pCallbacks;
        m_pCallbacks = pCallback->next;

        delete pCallback;
    }
}


HRESULT
CFileBlobStore::ResetBlobStorePaths()
{
    m_vecBlobFilePaths.clear();
    if (m_stripe >= 0)
        m_stripe = 0;

	return S_OK;
}

HRESULT
CFileBlobStore::AddBlobStorePaths(const FILE_BLOB_STORE* pPaths, 
								  UINT uPathCount)
{
    VT_HR_BEGIN()

    if (pPaths == NULL || uPathCount == 0)
        VT_HR_EXIT( E_INVALIDARG );

    UINT uPreviousCount = (UINT) m_vecBlobFilePaths.size();
    UINT uFullCount =  uPreviousCount + uPathCount;
    VT_HR_EXIT( m_vecBlobFilePaths.resize(uFullCount) );

    for (UINT uPath = 0; uPath < uPathCount; uPath++)
        m_vecBlobFilePaths[uPreviousCount + uPath] = pPaths[uPath];

    VT_HR_END()
}

bool
CFileBlobStore::DoesBlobStoreHaveSpace(UINT64 uByteCount)
{
	UINT64 uFreeSpace = 0;
    for (size_t i = 0; i < m_vecBlobFilePaths.size(); i++)
    {
		const vt::wstring& path = m_vecBlobFilePaths[i].strBlobStorePath;
		bool isUniqueRoot = true;
		for (size_t j = 0; j < i; j++)
		{
			if (PathIsSameRootW(path, m_vecBlobFilePaths[j].strBlobStorePath))
			{
				isUniqueRoot = false;
				break;
			}
		}
		if (isUniqueRoot)
		{
			ULARGE_INTEGER liFreeBytesAvailable;
			if (GetDiskFreeSpaceExW(path, &liFreeBytesAvailable, NULL, NULL) != 0)
			{
				uFreeSpace = VtMax(uFreeSpace, liFreeBytesAvailable.QuadPart);
			}
		}
	}
	return uFreeSpace >= uByteCount;
}


UINT
CFileBlobStore::GetTractSize()
{
    return m_tractSize;
}

HRESULT
CFileBlobStore::CreateBlob(LPVOID& pblob, LPCWSTR pwszName, UINT maxTracts)
{
    Blob* blob = NULL;

    VT_HR_BEGIN()

    if (m_tractSize == 0)
        VT_HR_EXIT( E_NOINIT );

    blob = VT_NOTHROWNEW Blob;
    VT_PTR_OOM_EXIT(blob);

    blob->uMaxTracts = maxTracts;

    // Try to create a blob file in one of the blob file paths.
    for (UINT i = 0; i < (pwszName == NULL ? static_cast<UINT>(m_vecBlobFilePaths.size()) : 1); i++)
    {
        if (pwszName == NULL && m_stripe >= 0)
            i = (i + m_stripe) % m_vecBlobFilePaths.size();

        if (pwszName == NULL &&
            m_vecBlobFilePaths[i].uBlobStoreTracts > 0 &&
            m_vecBlobFilePaths[i].uBlobStoreTracts < maxTracts)
        {
            hr = HRESULT_FROM_WIN32( ERROR_DISK_FULL );
            continue;
        }

        wstring_b<MAX_PATH> wstrBackingFile;
        if (pwszName == NULL)
        {
            // Get a temporary file name.
            if (!CSystem::GetTempFileName(m_vecBlobFilePaths[i].strBlobStorePath,
										  L"IMG", 0, wstrBackingFile.get_buffer()))
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                continue;
            }
        }
        else if ((hr = wstrBackingFile.assign(pwszName)) != S_OK)
            continue;

        // Create a temporary file.
        HANDLE hBlobFile = CSystem::CreateFile(wstrBackingFile, GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                                      (pwszName == NULL ? FILE_FLAG_DELETE_ON_CLOSE : 0x0) |
                                      FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS,
                                      NULL);
        if (hBlobFile == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            continue;
        }

        blob->hBlobFile = hBlobFile;

        if (pwszName == NULL)
            blob->uBlobStoreIndex = i;
        else
            VT_HR_EXIT( blob->BlobFile.strBlobStorePath.assign(pwszName) );

        pblob = blob;
        if (pwszName == NULL && m_stripe >= 0)
            m_stripe = (m_stripe + 1) % m_vecBlobFilePaths.size();
        break;
    }

    VT_HR_EXIT_LABEL()

    if (hr != S_OK)
    {
        if (blob != NULL)
        {
            if (blob->hBlobFile != NULL)
                CloseHandle(blob->hBlobFile);
            delete blob;
        }
    }
        
    return hr;
}

UINT
CFileBlobStore::GetBlobSize(LPVOID pblob)
{
    Blob *blob = (Blob *) pblob;

    return blob == NULL ? 0 : blob->BlobFile.uBlobStoreTracts;
}

HRESULT
CFileBlobStore::ExtendBlob(LPVOID pblob, UINT numTracts)
{
    VT_HR_BEGIN()

    Blob *blob = (Blob *) pblob;

    if (blob == NULL || blob->hBlobFile == NULL)
        VT_HR_EXIT( E_NOINIT );

    if (numTracts == 0)
        VT_HR_EXIT( E_INVALIDARG );

    if (blob->uMaxTracts > 0 &&
        blob->uMaxTracts < blob->BlobFile.uBlobStoreTracts + numTracts)
        VT_HR_EXIT( E_ACCESSDENIED );

    if (blob->uBlobStoreIndex != UINT_MAX)
    {
        // Check if new tracts fit in blob path.
        if (m_vecBlobFilePaths[blob->uBlobStoreIndex].uBlobStoreTracts > 0 &&
            m_vecBlobFilePaths[blob->uBlobStoreIndex].uBlobStoreTracts < numTracts)
            VT_HR_EXIT( HRESULT_FROM_WIN32( ERROR_DISK_FULL ) );
    }

    LARGE_INTEGER liFileSize;
    liFileSize.QuadPart = (LONGLONG) m_tractSize *
        (blob->BlobFile.uBlobStoreTracts + numTracts);

    if (SetFilePointerEx(blob->hBlobFile, liFileSize, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
        !SetEndOfFile(blob->hBlobFile))
        VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

    if (blob->uBlobStoreIndex != UINT_MAX)
    {
        // Charge new tracts to blob path.
        if (m_vecBlobFilePaths[blob->uBlobStoreIndex].uBlobStoreTracts > 0)
            m_vecBlobFilePaths[blob->uBlobStoreIndex].uBlobStoreTracts -= numTracts;
    }

    blob->BlobFile.uBlobStoreTracts += numTracts;

    VT_HR_END()
}

HRESULT
CFileBlobStore::OpenBlob(LPVOID& pblob, UINT& numTracts, LPCWSTR pwszName,
                         bool bReadOnly)
{
    Blob* blob = NULL;

    VT_HR_BEGIN()

    if (pwszName == NULL)
        VT_HR_EXIT( E_INVALIDARG );

    blob = VT_NOTHROWNEW Blob;
    VT_PTR_OOM_EXIT(blob);

    // Open an existing file.
    HANDLE hBlobFile = CSystem::CreateFile(pwszName,
                                  GENERIC_READ | (bReadOnly ? 0x0 : GENERIC_WRITE),
                                  FILE_SHARE_READ | (bReadOnly ? 0x0 : FILE_SHARE_WRITE),
                                  NULL, OPEN_EXISTING,
                                  FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS,
                                  NULL);
    if (hBlobFile == INVALID_HANDLE_VALUE)
        VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

    LARGE_INTEGER liFileSize;
    if (!CSystem::GetFileSizeEx(hBlobFile, &liFileSize))
        VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

    VT_HR_EXIT( blob->BlobFile.strBlobStorePath.assign(pwszName) );

    blob->BlobFile.uBlobStoreTracts = (UINT) (liFileSize.QuadPart / m_tractSize);

    blob->hBlobFile = hBlobFile;

    pblob = blob;

    numTracts = blob->BlobFile.uBlobStoreTracts;

    VT_HR_EXIT_LABEL()

    if (hr != S_OK)
    {
        if (blob != NULL)
        {
            if (blob->hBlobFile != NULL)
                CloseHandle(blob->hBlobFile);
            delete blob;
        }
    }
        
    return hr;
}

HRESULT
CFileBlobStore::ReadBlob(LPVOID pblob, UINT tractNum, UINT numTracts,
                         LPBYTE tractBuf, IBlobCallback* cbfn)
{
    VT_HR_BEGIN()

    Blob *blob = (Blob *) pblob;

    if (blob == NULL || blob->hBlobFile == NULL)
        VT_HR_EXIT( E_NOINIT );

    if (blob->BlobFile.uBlobStoreTracts < tractNum + numTracts ||
        numTracts == 0 || tractBuf == NULL)
        VT_HR_EXIT( E_INVALIDARG );

    // Check tract buffer is multiple of tract size.
    UINT_PTR uTractMask = m_tractSize - 1;
    if ((UINT_PTR) tractBuf != (((UINT_PTR) tractBuf +
        uTractMask) & ~uTractMask))
        VT_HR_EXIT( E_POINTER );

    BLOBCALLBACK* pCallback;
    VT_HR_EXIT( GetCallback((LPVOID *) &pCallback) );

    UINT64 uiOffset = (UINT64) tractNum * m_tractSize;
    pCallback->overlapped.Offset = LODWORD(uiOffset);
    pCallback->overlapped.OffsetHigh = HIDWORD(uiOffset);

    // From MSDN "ReadFileEx":
    // The ReadFileEx function ignores the OVERLAPPED structure's hEvent member.
    // An application is free to use that member for its own purposes in the context of a ReadFileEx call.
    if ((pCallback->cbfn = cbfn) != NULL)
    {
        pCallback->overlapped.hEvent = (HANDLE) this;
    }
    else
    {
        pCallback->overlapped.hEvent = CSystem::CreateEvent(FALSE, FALSE);
        VT_PTR_OOM_EXIT( pCallback->overlapped.hEvent );
    }

    // Start asynchronous read.
    if (!ReadFileEx(blob->hBlobFile, tractBuf, numTracts * m_tractSize,
                    &pCallback->overlapped, VtFileIOCompletionRoutine))
        VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() )  );

    // If no callback make synchronous.
    if (cbfn == NULL)
    {
        DWORD dwError;
        while ((dwError = vt::CSystem::WaitForSingleObjectEx(pCallback->overlapped.hEvent, INFINITE, TRUE))
            != WAIT_OBJECT_0)
        {
            if (dwError == WAIT_FAILED)
                VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() )  );
        }

        CloseHandle(pCallback->overlapped.hEvent);

        BlobIOCompletionRoutine(pCallback);
    }

    VT_HR_END()
}

HRESULT
CFileBlobStore::WriteBlob(LPVOID pblob, UINT tractNum, UINT numTracts,
                          const LPBYTE tractBuf, IBlobCallback* cbfn)
{
    VT_HR_BEGIN()

    Blob *blob = (Blob *) pblob;

    if (blob == NULL || blob->hBlobFile == NULL)
        VT_HR_EXIT( E_NOINIT );

    if (blob->BlobFile.uBlobStoreTracts < tractNum + numTracts ||
        numTracts == 0 || tractBuf == NULL)
        VT_HR_EXIT( E_INVALIDARG );

    // Check tract buffer is multiple of tract size.
    UINT_PTR uTractMask = m_tractSize - 1;
    if ((UINT_PTR) tractBuf != (((UINT_PTR) tractBuf +
        uTractMask) & ~uTractMask))
        VT_HR_EXIT( E_POINTER );

    BLOBCALLBACK* pCallback;
    VT_HR_EXIT( GetCallback((LPVOID *) &pCallback) );

    UINT64 uiOffset = (UINT64) tractNum * m_tractSize;
    pCallback->overlapped.Offset = LODWORD(uiOffset);
    pCallback->overlapped.OffsetHigh = HIDWORD(uiOffset);

    // From MSDN "WriteFileEx":
    // The WriteFileEx function ignores the OVERLAPPED structure's hEvent member.
    // An application is free to use that member for its own purposes in the context of a WriteFileEx call.
    if ((pCallback->cbfn = cbfn) != NULL)
    {
        pCallback->overlapped.hEvent = (HANDLE) this;
    }
    else
    {
        pCallback->overlapped.hEvent = CSystem::CreateEvent(FALSE, FALSE);
        VT_PTR_OOM_EXIT( pCallback->overlapped.hEvent );
    }

    // Start asynchronous write.
    if (!WriteFileEx(blob->hBlobFile, tractBuf, numTracts * m_tractSize,
                     &pCallback->overlapped, VtFileIOCompletionRoutine))
        VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() )  );

    // If no callback make synchronous.
    if (cbfn == NULL)
    {
        DWORD dwError;
        while ((dwError = vt::CSystem::WaitForSingleObjectEx(pCallback->overlapped.hEvent, INFINITE, TRUE))
            != WAIT_OBJECT_0)
        {
            if (dwError == WAIT_FAILED)
                VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() )  );
        }

        CloseHandle(pCallback->overlapped.hEvent);

        BlobIOCompletionRoutine(pCallback);
    }

    VT_HR_END()
}

HRESULT
CFileBlobStore::CloseBlob(LPVOID pblob)
{
    VT_HR_BEGIN()

    Blob *blob = (Blob *) pblob;

    if (blob == NULL)
        VT_HR_EXIT( E_INVALIDARG );

    if (!CloseHandle(blob->hBlobFile))
        VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

    if (blob->uBlobStoreIndex != UINT_MAX)
    {
        // Credit blob tracts back to blob path.
        if (m_vecBlobFilePaths[blob->uBlobStoreIndex].uBlobStoreTracts > 0)
            m_vecBlobFilePaths[blob->uBlobStoreIndex].uBlobStoreTracts +=
                blob->BlobFile.uBlobStoreTracts;
    }

    delete blob;

    VT_HR_END()
}

HRESULT
CFileBlobStore::DeleteBlob(LPCWSTR pwszName)
{
    VT_HR_BEGIN()

    if (pwszName == NULL)
        VT_HR_EXIT( E_INVALIDARG );

    if (!DeleteFile(pwszName))
        VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

    VT_HR_END()
}


HRESULT
CFileBlobStore::GetCallback(LPVOID* pCallback)
{
    VT_HR_BEGIN()

    VT_PTR_EXIT( pCallback );

    // if there is an unused callback available use it otherwise allocate one
    if (m_pCallbacks != NULL)
    {
        *pCallback = m_pCallbacks;
        m_pCallbacks = ((BLOBCALLBACK *) m_pCallbacks)->next;
    }
    else
    {
        *pCallback = VT_NOTHROWNEW BLOBCALLBACK;
        VT_PTR_OOM_EXIT( *pCallback );
    }

    VT_HR_END()
}

VOID
CFileBlobStore::BlobIOCompletionRoutine(LPVOID pCallback)
{
    // add to the callbacks
    ((BLOBCALLBACK *) pCallback)->next =
        (BLOBCALLBACK *) m_pCallbacks;
    m_pCallbacks = pCallback;
}

#endif
