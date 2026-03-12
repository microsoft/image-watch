//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Helper class for reading and writing Flat Data Store blobs.
//
//  History:
//      2011/2/16-v-hogood
//          Created
//
//------------------------------------------------------------------------
#pragma once

#if !defined(VT_WINRT)

#include "vtcommon.h"
#include "vt_blobstore.h"

namespace vt {

struct FILE_BLOB_STORE
{
	UINT        uBlobStoreTracts;
	vt::wstring strBlobStorePath;

    FILE_BLOB_STORE() : uBlobStoreTracts(0) {};
};

class CFileBlobStore : public vt::IBlobStore
{
public:
	CFileBlobStore();
    virtual ~CFileBlobStore();

    //+-------------------------------------------------------------------------
    //
    //  Method:  ResetBlobStorePaths
    //
    //  Synopsis: Removes all path(s) for the blob store 
	//            
	//--------------------------------------------------------------------------
	HRESULT ResetBlobStorePaths();

    //+-------------------------------------------------------------------------
    //
    //  Method:  AddBlobStorePaths
    //
    //  Synopsis: Adds the path(s) for the blob store 
	//            
	//     pPaths - a list of paths to use, includes a path name and the 
	//              maximum amonut of memory to use on that store
	//     uPathCount - the number of paths indicated by the pPaths argument
	//
	//--------------------------------------------------------------------------
	HRESULT AddBlobStorePaths(const FILE_BLOB_STORE* pPaths, 
                              UINT uPathCount);

    //+-------------------------------------------------------------------------
    //
    //  Method:  SetStriping
    //
    //  Synopsis: Turn striping on or off for the blob store 
	//            
	//     bStripe - if true, each CreateBlob() goes to next blob store path
    //               in round-robin fashion
    //               if false, each CreateBlob() goes to current blob store path
    //               until maxed out
	//
	//--------------------------------------------------------------------------
	void SetStriping(bool bStripe)
    { m_stripe = bStripe ? 0 : -1; }

    //+-------------------------------------------------------------------------
    //
    //  Method:  DoesBlobStoreHaveSpace()
    //
    //  Synopsis: Returns true if the blob store disks have enough free
	//            space to store the given number of bytes
    //
	//     uByteCount - the number of bytes of storage space in question
    //
    //--------------------------------------------------------------------------
	bool DoesBlobStoreHaveSpace(UINT64 uByteCount);

    //+-------------------------------------------------------------------------
    //
    // From IBlobStore:
    //

    // Returns the tract size for this type of blob store.
    virtual UINT GetTractSize();

    // CreateBlob creates a blob for reading and writing.
    // Initial size is 0 tracts; maximum is maxTracts (0 == unlimited).
    // Blob is delete on close if not named.
    virtual HRESULT CreateBlob(OUT LPVOID& pblob, LPCWSTR pwszName = NULL,
                               UINT maxTracts = 0);

    // Returns the number of tracts in this blob.
    virtual UINT GetBlobSize(LPVOID pblob);

    // Extend an existing blob by a number of tracts.
    virtual HRESULT ExtendBlob(LPVOID pblob, UINT numTracts);

    // Open an existing named blob for reading and writing.
    virtual HRESULT OpenBlob(OUT LPVOID& pblob, OUT UINT& numTracts,
                             LPCWSTR pwszName, bool bReadOnly = false);

    // Read tracts from the blob.
    virtual HRESULT ReadBlob(LPVOID pblob, UINT tractNum, UINT numTracts,
                             LPBYTE tractBuf, IBlobCallback* cbfn = NULL);

    // Write tracts to the blob.
    virtual HRESULT WriteBlob(LPVOID pblob, UINT tractNum, UINT numTracts,
                              const LPBYTE tractBuf, IBlobCallback* cbfn = NULL);

    // Close the blob (and delete if not named).
    virtual HRESULT CloseBlob(LPVOID pblob);

    // Delete the named blob.
    virtual HRESULT DeleteBlob(LPCWSTR pwszName);

    VOID BlobIOCompletionRoutine(LPVOID pCallback);

	inline const WCHAR* GetLastErrorStr() const
	{ return m_strLastError; }

private:
    HRESULT GetCallback(LPVOID* pCallback);

    UINT m_tractSize;

    vt::vector<FILE_BLOB_STORE> m_vecBlobFilePaths;

    vt::wstring_b<512> m_strLastError; 

    // Empty callbacks to reduce memory allocations.
    LPVOID m_pCallbacks;

    int m_stripe;   // -1 == no striping, else current blob file path for striping
};

};

#endif
