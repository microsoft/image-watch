//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Helper interface for reading and writing to data stores.
//
//  History:
//      2011/2/16-v-hogood
//          Created
//
//------------------------------------------------------------------------
#pragma once

namespace vt {

//+---------------------------------------------------------------------------
// 
// Generic interfaces supported by various data stores 
//
//----------------------------------------------------------------------------
class IBlobCallback
{
public:
    virtual ~IBlobCallback() {};

    virtual void Callback(HRESULT hr) = 0;
};

class IBlobStore
{
public:
    virtual ~IBlobStore() {};

    // Returns the tract size for this type of blob store.
    virtual UINT GetTractSize() = 0;

    // CreateBlob creates a blob for reading and writing.
    // Initial size is 0 tracts; maximum is maxTracts (0 == unlimited).
    // Blob is delete on close if not named.
    virtual HRESULT CreateBlob(OUT LPVOID& pblob, LPCWSTR pwszName = NULL,
                               UINT maxTracts = 0) = 0;

    // Returns the number of tracts in this blob.
    virtual UINT GetBlobSize(LPVOID pblob) = 0;

    // Extend an existing blob by a number of tracts.
    virtual HRESULT ExtendBlob(LPVOID pblob, UINT numTracts) = 0;

    // Open an existing named blob for reading and writing.
    virtual HRESULT OpenBlob(OUT LPVOID& pblob, OUT UINT& numTracts,
                             LPCWSTR pwszName, bool bReadOnly = false) = 0;

    // Read tracts from the blob.
    virtual HRESULT ReadBlob(LPVOID pblob, UINT tractNum, UINT numTracts,
                             LPBYTE tractBuf, IBlobCallback* cbfn = NULL) = 0;

    // Write tracts to the blob.
    virtual HRESULT WriteBlob(LPVOID pblob, UINT tractNum, UINT numTracts,
                              const LPBYTE tractBuf, IBlobCallback* cbfn = NULL) = 0;

    // Close the blob (and delete if not named).
    virtual HRESULT CloseBlob(LPVOID pblob) = 0;

    // Delete the named blob.
    virtual HRESULT DeleteBlob(LPCWSTR pwszName) = 0;
};

};
