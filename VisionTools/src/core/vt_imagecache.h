//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Class to manage a large set of input images in a given amount of
//      memory
//
//  History:
//      2004/11/8-mattu
//          Created
//
//------------------------------------------------------------------------
#pragma once

#include "vtcommon.h"

namespace vt {

struct IMG_CACHE_SOURCE_PROPERTIES
{
	bool  bAutoLevelGenerate; // if true the cache will automatically generate 
	const C1dKernel* pKernel; // levels using the provided kernel, if kernel
	                          // is NULL then the C14641Kernel is used

    const wchar_t* pBackingBlob;  // backing store blob name
    int   iMaxLevel;              // max image level allowed in cache (-1 = all)
    int   iAlignment;             // backing store blob tract size (0 = default)

    UINT  uBlockSizeX;            // block width for this source (256 = default)
    UINT  uBlockSizeY;            // block height for this source (256 = default)
	
    IMG_CACHE_SOURCE_PROPERTIES() :
        bAutoLevelGenerate(true), pKernel(NULL),
        pBackingBlob(NULL), iMaxLevel(-1), iAlignment(0),
        uBlockSizeX(256), uBlockSizeY(256)
	{ }

	IMG_CACHE_SOURCE_PROPERTIES(bool bGenLev, const C1dKernel* k,
                                int iLevel = -1, int iAlign = 0) : 
		bAutoLevelGenerate(bGenLev), pKernel(k),
        pBackingBlob(NULL), iMaxLevel(iLevel), iAlignment(iAlign),
        uBlockSizeX(256), uBlockSizeY(256)
	{ }
};

struct IMG_CACHE_SOURCE_STATISTICS
{
    struct COUNTS
    {
        uint64_t request_bytes;      // = byte count of read/write/prefetch/clear requests - note NOT padded to block size
        uint64_t blob_miss_bytes;    // = byte count of misses fulfilled by backing store blob - note IS padded to block size
        uint64_t reader_miss_bytes;  // = byte count of misses fulfilled by source reader - note IS padded to block size
        uint64_t autogen_miss_bytes; // = byte count of misses fulfilled by pyramid autogen - note IS padded to block size
        uint64_t clear_miss_bytes;   // = byte count of misses fulfilled by clear value - note IS padded to block size

        COUNTS() : request_bytes(0), blob_miss_bytes(0), reader_miss_bytes(0),
                   autogen_miss_bytes(0), clear_miss_bytes(0)
        { }

        COUNTS operator- (const COUNTS& Counts) const
        {
            COUNTS This;
            This.request_bytes = request_bytes - Counts.request_bytes;
            This.blob_miss_bytes = blob_miss_bytes - Counts.blob_miss_bytes;
            This.reader_miss_bytes = reader_miss_bytes - Counts.reader_miss_bytes;
            This.autogen_miss_bytes = autogen_miss_bytes - Counts.autogen_miss_bytes;
            This.clear_miss_bytes = clear_miss_bytes - Counts.clear_miss_bytes;
            return This;
        }

        COUNTS operator+ (const COUNTS& Counts) const
        {
            COUNTS This;
            This.request_bytes = request_bytes + Counts.request_bytes;
            This.blob_miss_bytes = blob_miss_bytes + Counts.blob_miss_bytes;
            This.reader_miss_bytes = reader_miss_bytes + Counts.reader_miss_bytes;
            This.autogen_miss_bytes = autogen_miss_bytes + Counts.autogen_miss_bytes;
            This.clear_miss_bytes = clear_miss_bytes + Counts.clear_miss_bytes;
            return This;
        }

        COUNTS& operator-= (const COUNTS& Counts)
        {
            return *this = *this - Counts;
        }

        COUNTS& operator+= (const COUNTS& Counts)
        {
            return *this = *this + Counts;
        }
    };

    uint64_t flushed_bytes;          // = byte count of blocks flushed from the cache initially
    uint64_t reflush_bytes;          // = byte count of blocks flushed from the cache more than once

    COUNTS reads;      // requests and misses caused by reads
    COUNTS writes;     // requests and misses caused by writes
    COUNTS prefetches; // requests and misses caused by prefetches
    COUNTS clears;     // requests and misses caused by clears

    IMG_CACHE_SOURCE_STATISTICS() : flushed_bytes(0), reflush_bytes(0)
    { }

    IMG_CACHE_SOURCE_STATISTICS operator-
        (const IMG_CACHE_SOURCE_STATISTICS& Stats) const
    {
        IMG_CACHE_SOURCE_STATISTICS This;
        This.flushed_bytes = flushed_bytes - Stats.flushed_bytes;
        This.reflush_bytes = reflush_bytes - Stats.reflush_bytes;
        This.reads = reads - Stats.reads;
        This.writes = writes - Stats.writes;
        This.prefetches = prefetches - Stats.prefetches;
        This.clears = clears - Stats.clears;
        return This;
    }

    IMG_CACHE_SOURCE_STATISTICS operator+
        (const IMG_CACHE_SOURCE_STATISTICS& Stats) const
    {
        IMG_CACHE_SOURCE_STATISTICS This;
        This.flushed_bytes = flushed_bytes + Stats.flushed_bytes;
        This.reflush_bytes = reflush_bytes + Stats.reflush_bytes;
        This.reads = reads + Stats.reads;
        This.writes = writes + Stats.writes;
        This.prefetches = prefetches + Stats.prefetches;
        This.clears = clears + Stats.clears;
        return This;
    }

    IMG_CACHE_SOURCE_STATISTICS& operator-=
        (const IMG_CACHE_SOURCE_STATISTICS& Stats)
    {
        return *this = *this - Stats;
    }

    IMG_CACHE_SOURCE_STATISTICS& operator+=
        (const IMG_CACHE_SOURCE_STATISTICS& Stats)
    {
        return *this = *this + Stats;
    }
};

//+----------------------------------------------------------------------------
//
//  Class:  CImageCache
//
//  Synopsis:  This is the class that handles memory managment for the 
//             VisionTools applications.
// 
//             Notes: 
//             (1) all methods are multi-thread safe
//             (2) CImgInfo, CLayerImgInfo, and CParams are always resident in 
//                 the cache, so getting them will never generate a disk access
// 
// 
// TODO: - CParams needs to be shareable like CImg is
//       - Add a property for the pyramid generation filter - per source
//       - Add a pre-fetch mechanism - probably a queue
//
//-----------------------------------------------------------------------------
class CImageCache
{
public: 
    //+-------------------------------------------------------------------------
    //
    //  Method:  SetMemoryUsage
    //
    //  Synopsis: sets the memory limit on the image cache
	//            
	//     uBytes - The maximum amount of system memory (in bytes) to use
	//              before backing images to disk.
	//
	//--------------------------------------------------------------------------
	HRESULT SetMaxMemoryUsage( UINT64 uBytes );

    //+-------------------------------------------------------------------------
    //
    //  Method:  SetBackingStore
    //
    //  Synopsis: sets the backing store for the image cache
	//            
	//--------------------------------------------------------------------------
    HRESULT SetBackingStore( IBlobStore *pBlobStore );

    IBlobStore* GetBackingStore();

    //+-------------------------------------------------------------------------
    //
    //  Method:  GetMaxMemoryUsage()
    //
    //  Synopsis: Returns the current maximum memory available to the cache
    //
    //--------------------------------------------------------------------------
	UINT64 GetMaxMemoryUsage();

    //+-------------------------------------------------------------------------
    //
    //  Method:  GetMemoryUsage()
    //
    //  Synopsis: Returns the current memory usage of the cache.  This should
	//            always be <= uMaxCacheSystemMemory
    //
    //--------------------------------------------------------------------------
	UINT64 GetMemoryUsage();

	//+-------------------------------------------------------------------------
	//
	//  Method:  FlushCache()
	//
	//  Synopsis: Purges all data from the system memory cache and resets the
	//            system memory usage to 0.
	//
	//--------------------------------------------------------------------------
	void FlushCache();

	//+-------------------------------------------------------------------------
	//
	//  Method:  AddSource()
	//
	//  Synopsis: Various methods for adding a source of data into the cache.
	//            A source can have one or more images.  Once a source is added
	//            the cache assigns an 'Id' for it that will be used to reference
	//            it in subsequent calls.
	// 
	//      AddSource(const CImgInfo& info, UINT uCount, OUT UINT& uSourceId)
	//      - adds uCount number of images each of dimensions specified by
	//        corresponding CImgInfo, returns uSourceId for the allocated image
    //      - can be one info or one per image
    //
	//      AddSource(IImageReader* pReader, OUT UINT& uSourceId)
	//      - adds a single (layer) image whose source is pReader, 
	//        returns uSourceId for the allocated image
	// 
	//      AddSource(IIndexedImageReader* pReader,  OUT UINT& uSourceId)
	//      - adds an indexed list of (layer) images whose source is pReader, 
	//        returns uSourceId for the allocated images
	// 
    //      UseBackingStore indicates to use cache backing store;
    //      otherwise go back to the Reader.
	//--------------------------------------------------------------------------
	HRESULT AddSource(const wchar_t* pBackingBlob, OUT UINT& uSrcId,
                      bool bReadOnly = false);
	HRESULT AddSource(const CLayerImgInfo& info, UINT uCount, OUT UINT& uSrcId, 
					  const IMG_CACHE_SOURCE_PROPERTIES* pProps = NULL);
	HRESULT AddSource(const CLayerImgInfo* infos, UINT uCount, OUT UINT& uSrcId, 
					  const IMG_CACHE_SOURCE_PROPERTIES* pProps = NULL);
	HRESULT AddSource(IImageReader* pReader, OUT UINT& uSourceId,
                      bool bUseBackingStore = true,
                      const wchar_t* pBackingBlob = NULL);
	HRESULT AddSource(IIndexedImageReader* pReader, OUT UINT& uSourceId,
                      bool bUseBackingStore = true,
                      const wchar_t* pBackingBlob = NULL);

    HRESULT GetSourceProperties(UINT uSrcId, IMG_CACHE_SOURCE_PROPERTIES& Props);

    HRESULT GetSourceStatistics(UINT uSrcId, IMG_CACHE_SOURCE_STATISTICS& Stats);

	//+-------------------------------------------------------------------------
	//
	//  Method:  FlushSource()
	// 
	//  Synopsis: Flushes the source specified by uSourceId from the cache
    //            to the source's backing store blob.
	//
	//--------------------------------------------------------------------------
	HRESULT FlushSource(UINT uSourceId, CTaskProgress* pProg = NULL);

	//+-------------------------------------------------------------------------
	//
	//  Method:  ClearSource()
	// 
	//  Synopsis: Clears the source specified by uSourceId from the cache
    //            and removes the source's backing store blob.
	//
	//--------------------------------------------------------------------------
	HRESULT ClearSource(UINT uSourceId, IN const void* pClearVal = NULL);

	//+-------------------------------------------------------------------------
	//
	//  Method:  DeleteSource()
	// 
	//  Synopsis: Removes the source specified by uSourceId from the cache. 
	//
	//--------------------------------------------------------------------------
	HRESULT DeleteSource(UINT uSourceId);

	//+-------------------------------------------------------------------------
	//
	//  Method:  GetFrameCount()
	// 
	//  Synopsis: Returns the number of images associated with a particular 
	//            uSourceId.  This will return 1 when AddSource was called
	//            with a uCount=1 or a non-indexed reader, otherwise it will
	//            return the length of the source list.
	//
	//--------------------------------------------------------------------------
	UINT GetFrameCount(UINT uSourceId);

    VOID GetBlockSize(UINT uSourceId, UINT uIndex,
                      UINT& uBlockSizeX, UINT& uBlockSizeY);

	//+-------------------------------------------------------------------------
	//
	//  Method:  GetImgInfo
	// 
	//  Synopsis: returns the CLayerImgInfo for an image defined by: uSourceId,
	//            index within that source, and level within the image. 
	//
	//--------------------------------------------------------------------------
	CLayerImgInfo GetImgInfo( UINT uSourceId, UINT uIndex = 0, UINT uLevel = 0 );

    //+-------------------------------------------------------------------------
	//
	//  Method: ReadImg
	// 
	//  Synopsis: returns the entire image specified by uSourceId, uIndex, 
	//            and uLevel.  If the image is not in cache then it will be 
	//            retrievied from either (a) on first access the source
	//            specified during AddSource (b) on subsequent accesses from
	//            the disk backing blob.  If the CImg argument has a
	//            specified type that is different from the cache type then 
	//            a convert will be done.
	//
	//--------------------------------------------------------------------------
	HRESULT  ReadImg(UINT uSourceId, UINT uIndex, OUT CImg& dst, 
					 UINT uLevel = 0);

	//+-------------------------------------------------------------------------
	//
	//  Method:  GetMetaData
	// 
	//  Synopsis: returns the metadata for an image defined by: uSourceId, and
	//            index within that source
	//
	//--------------------------------------------------------------------------
	HRESULT GetMetaData( UINT uSourceId, UINT uIndex, OUT CParams& params);

	//+-------------------------------------------------------------------------
	//
	//  Method: ReadRegion
	// 
	//  Synopsis: identical to ReadImg except that it only fetches a specified
	//            sub-region of the source.
	//
	//--------------------------------------------------------------------------
	HRESULT  ReadRegion(UINT uSourceId, UINT uIndex, IN const CRect& region,
                        OUT CImg& dst, UINT uLevel = 0);

	//+-------------------------------------------------------------------------
	//
	//  Method: Prefetch
	// 
	//  Synopsis: starts a prefetch into the cache.
	//
	//--------------------------------------------------------------------------
	HRESULT  Prefetch(UINT uSourceId, UINT uIndex, IN const CRect& region,
                      UINT uLevel = 0);

    //+-------------------------------------------------------------------------
	//
	//  Method: SetMetaData
	// 
	//  Synopsis: Updates the metadata for an image defined by uSourceId and
	//            uIndex.  If bMerge is set then the metadata is merged in
	//            with the metadata in cache.  Otherwise the metadata in cache
	//            is replaced.
	//
	//--------------------------------------------------------------------------
	HRESULT SetMetaData( UINT uSourceId, UINT uIndex, const CParams& params,
						 bool bMerge = true);

	//+-------------------------------------------------------------------------
	//
	//  Method: WriteImg
	// 
	//  Synopsis: Updates the pixels for an entire image in the cache.  If CImg
	//            is of a different type than the image in cache then a convert 
	//            is performed.
	//            This will fail if the source was defined as an IImageReader.
	//
	//--------------------------------------------------------------------------
	HRESULT WriteImg(UINT uSourceId, UINT uIndex, IN const CImg& src,
					 UINT uLevel = 0, bool bWriteThrough = false);

	HRESULT WriteRegion(UINT uSourceId, UINT uIndex, IN const CRect& region,
						IN const CImg& src, UINT uLevel = 0, 
						bool bWriteThrough = false);

    //+-------------------------------------------------------------------------
	//
	//  Method: ClearImg
	// 
	//  Synopsis: Clears the pixels for an entire image in the cache to a
	//            specified value (default is the source's clear value).
	//            This will fail if the source was defined as an IImageReader.
	//
	//--------------------------------------------------------------------------
	HRESULT ClearImg(UINT uSourceId, UINT uIndex, UINT uLevel = 0,
                     IN const void* pClearVal = NULL);

	HRESULT ClearRegion(UINT uSourceId, UINT uIndex, IN const CRect& region,
                        UINT uLevel = 0, const void* pClearVal = NULL);

    //+-------------------------------------------------------------------------
    //
    //  Method:  GenerateLevels()
    //
    //  Synopsis: Constructs a region RgnInEndLevel in level EndLev,
    //            and all matching regions in all levels above it.
    //
    //--------------------------------------------------------------------------
    HRESULT GenerateLevels(UINT uSourceId, UINT uIndex, UINT uEndLev,
                           IN const CRect* pRgnInEndLev = NULL);

protected:
    CImageCache();

    ~CImageCache();

    HRESULT StartupInternal();

private:
    struct LINK;

    class CICBlobCallback : public IBlobCallback 
    {
    public:
        virtual void Callback(HRESULT hr);

        // if in use, pointer to the block
        // otherwise, pointer to the next callback in empty list
        union 
        {
            LINK *m_pLink;
            CICBlobCallback *m_pNext;
        };

        CICBlobCallback() : m_pLink(NULL) {};
    };

    struct BLOCK
    {
        UINT uTractNum;
#define BLOCK_IN_STORE  0x80000000
#define BLOCK_IN_CACHE  0x40000000
#define BLOCK_IN_WRITE  0x20000000
#define TRACT_MASK (UINT_MAX & ~(BLOCK_IN_STORE | BLOCK_IN_CACHE | BLOCK_IN_WRITE))
        UINT uUniquenessValue;

        // if in cache, pointer to the block in the MRU list
        // otherwise, pointer to the clear value
        union 
        {
            LINK *pLink;
            const void *pClearVal;
        };

        BLOCK() : uTractNum(0), uUniquenessValue(0), pLink(NULL) {};
    };

    struct LEVEL
    {
        // info for this level
        CLayerImgInfo info;

        // blocks of this level
        vt::vector<BLOCK> vecBlocks;
    };

    struct IMAGE
    {
        // params for this image
        CParams *params;

        // levels of this image
        vt::vector<LEVEL> vecLevels;

        // keep track of clear values
        vt::vector<void *> vecClearVals;

	    UINT uBlockSizeX;
	    UINT uBlockSizeY;

        IMAGE() : params(NULL), uBlockSizeX(256), uBlockSizeY(256) {};
 
        ~IMAGE()
        {
            if (params != NULL)
                delete params;

            // Delete clear values.
            for (size_t uVal = 0; uVal < vecClearVals.size(); uVal++)
                delete[] vecClearVals[uVal];
        };
    };

    struct SOURCE
    {
        IMG_CACHE_SOURCE_STATISTICS Stats;

        // this source
        IImageReader        *pImageReader;
        IIndexedImageReader *pIdxImageReader;

        // if false use reader as backing store
        bool bUseBackingStore;

        // backing store blob name
        wstring wstrBackingBlob;
        // max image level allowed in cache (-1 = all)
        int iMaxLevel;
        // backing store blob alignment (0 = default)
        int iAlignment;

        // current tract number in un-named backing blob for next block write
        UINT uTractNum;
        UINT uNumTracts;

    	bool bReadOnly;
        IIndexedImageReaderWriter *pBackingReaderWriter;
        LPVOID pBackingBlob;
        HRESULT hrBackingResult;

        // images of this source
        vt::vector<IMAGE> vecImages;

        // if true the source will automatically generate
        bool bAutoLevelGenerate; 

        // reference count
        LONG lRef;

        // pyramid filter kernels (for even and odd layer origins)
        C1dKernelSet ks, ks1;

        SOURCE() : pImageReader(NULL), pIdxImageReader(NULL),
                   bUseBackingStore(true), pBackingBlob(NULL),
                   pBackingReaderWriter(NULL),
                   iMaxLevel(-1), iAlignment(0), bReadOnly(false),
                   uTractNum(0), uNumTracts(0),
                   hrBackingResult(S_OK),
                   lRef(1), bAutoLevelGenerate(true) {};
    };

    struct LINK
    {
        SOURCE *pSource;

        Byte  *pbAlloc;
        Byte  *pbData;
        UINT  cbAlloc;
        UINT  cbData;

        BLOCK *pBlock;

        // pointers to the links in the MRU list
        LINK  *pPrev;
        LINK  *pNext;

        DWORD dwThreadId;
        DWORD dwThreadCount;

        CICBlobCallback* pCallback;

        LINK() : pSource(NULL), pbAlloc(NULL), pbData(NULL), cbAlloc(0), cbData(0),
                 pBlock(NULL), pPrev(NULL), pNext(NULL),
                 dwThreadId(0), dwThreadCount(0), pCallback(NULL) {};
    };

private:
    CLayerImgInfo GetImgInfo(SOURCE* source, UINT uIndex, UINT uLevel = 0);

    HRESULT AddSource(SOURCE* source,  OUT UINT& uSourceId,
                      UINT cInfos = 0,
                      const CLayerImgInfo* infos = NULL,
                      const IMG_CACHE_SOURCE_PROPERTIES* pProps = NULL);
    HRESULT FlushSource(SOURCE* source, CTaskProgress* pProg = NULL);
    HRESULT ClearSource(SOURCE* source, UINT uClearVal = 0);
    HRESULT AddClearValue(IMAGE& image, IN const void* pClearVal, OUT UINT& uClearVal);

	HRESULT DoRegion(UINT uSourceId, UINT uIndex, IN const CRect* region,
                     CImg* img, UINT uLevel, bool bWrite, bool bWriteThrough,
                     const void* pClearVal = NULL);
	HRESULT DoRegion(SOURCE* source, UINT uIndex, IN const CRect& rect,
                     CImg* img, UINT uLevel, bool bWrite, bool bWriteThrough,
                     UINT uClearVal = 0, bool bWait = true);
	HRESULT DoBlock(SOURCE* source, UINT uIndex, IN const CRect& rect,
                    CImg* img, UINT uLevel, bool bWrite, bool bWriteThrough,
                    UINT uClearVal, IN const CRect& rctBlk);

    HRESULT GenerateRegion(SOURCE* source, UINT uIndex, UINT uLevel,
                           IN const CRect& region, OUT CImg& imgDst);
    HRESULT ClearRegion(SOURCE* source, UINT uIndex,
                        IN const CRect& region, UINT uLevel,
                        UINT uClearVal);

    DWORD   BackingStoreProc();

    static DWORD WINAPI StaticBackingProc(LPVOID lpParameter)
    {
        return ((CImageCache *) lpParameter)->BackingStoreProc();
    }

    HRESULT ComputeSourceSize(SOURCE* source, bool bInBlob);
    HRESULT CreateBlob(SOURCE* source);
    HRESULT CheckForMemory(bool bFlush = false);
    void    MakeMRUBlock(LINK* pBlock);
    void    MakeLRUBlock(LINK* pBlock);
    HRESULT GetCallback(LINK* pBlock);
    HRESULT BackupBlock(LINK* pBlock);
    HRESULT RemoveBlock(LINK* pBlock);
    HRESULT AssignBlock(SOURCE* source, BLOCK& block, UINT cbData);
    HRESULT BlobBlock(LINK* pBlock, bool bWrite);
    void    CancelRead(LINK* pBlock);

public:
    void    HandleBlock(CICBlobCallback* pCallback, HRESULT hr);

private:
    CCritSection		 m_cs;

    HANDLE               m_hHeap;

    IBlobStore           *m_pBlobStore;
    IBlobStore           *m_pFileStore;
    UINT                 m_uPageSize;

    vt::vector<SOURCE *> m_vecSources;

    HANDLE               m_hBackingThread;
    HANDLE               m_hBackingEvent;
    UINT                 m_cBackingReads;
    UINT                 m_cBackingWrites;

    UINT64               m_uMaxCacheSystemMemory;
    UINT64               m_uHighWater;
    UINT64               m_uMemoryUsed;
    UINT64               m_uMemoryPending;
    SOURCE*              m_pFlushSource;

    // Cache MRU list.
    LINK                 *m_pHeadBlocks;
    LINK                 *m_pTailBlocks;

    // Backing store read list.
    LINK                 *m_pHeadReads;
    LINK                 *m_pTailReads;

    // Empty blocks/image/callbacks to reduce memory allocations.
    LINK                 *m_pEmptyBlocks;
    Byte                 *m_pbEmptyAlloc;
    UINT                 m_cbEmptyAlloc;
    CICBlobCallback      *m_pCallbacks;
};

CImageCache* VtGetImageCacheInstance();

//+----------------------------------------------------------------------------
//
//  Class:  CImgInCache
//
//  Synopsis:  An implementation of IImageReaderWriter that wraps an image 
//             in the cache.
//
//-----------------------------------------------------------------------------
class CImgInCache: public IImageReaderWriter
{
public:
	CImgInCache() : m_pIC(NULL), m_uSourceId(UINT_MAX) 
	{}

	~CImgInCache()
	{ Deallocate();}

public:
	// IImageReader
	virtual CLayerImgInfo GetImgInfo(UINT uLevel = 0)
	{ return m_pIC? m_pIC->GetImgInfo(m_uSourceId, 0, uLevel): CLayerImgInfo(); }

	virtual HRESULT GetMetaData(CParams &params)
    { return m_pIC? m_pIC->GetMetaData(m_uSourceId, 0, params): E_NOINIT; }

	virtual HRESULT ReadImg(OUT CImg& dst, UINT uLevel=0)
	{ return m_pIC? m_pIC->ReadImg(m_uSourceId, 0, dst, uLevel): E_NOINIT; }

	virtual HRESULT  ReadRegion(IN const CRect& r, OUT CImg& dst, 
								UINT uLevel = 0)
	{ return m_pIC? m_pIC->ReadRegion(m_uSourceId, 0, r, dst, uLevel): E_NOINIT;}

	virtual HRESULT Prefetch(IN const CRect& region, UINT uLevel = 0)
	{ return m_pIC? m_pIC->Prefetch(m_uSourceId, 0, region, uLevel): E_NOINIT; }

public:
	// IImageWriter
	virtual HRESULT  WriteImg(IN const CImg& src, UINT uLevel=0)
	{ return m_pIC? m_pIC->WriteImg(m_uSourceId, 0, src, uLevel): E_NOINIT; }

	virtual HRESULT  WriteRegion(IN const CRect& r, IN const CImg& src,
								 UINT uLvl=0)
	{ return m_pIC? m_pIC->WriteRegion(m_uSourceId, 0, r, src, uLvl): E_NOINIT; }

	virtual HRESULT Fill(const void* pbValue, const RECT *prct = NULL,
                         UINT uLevel = 0)
	{ 
		HRESULT hr = E_NOINIT;
		if( m_pIC )
		{
			hr = ( prct == NULL )? 
                m_pIC->ClearImg(m_uSourceId, 0, uLevel, pbValue):
				m_pIC->ClearRegion(m_uSourceId, 0, *prct, uLevel, pbValue);   
		}
		return hr;
	}

	virtual HRESULT  SetMetaData(const CParams *pParams)
	{
		HRESULT hr = E_NOINIT;
		if( m_pIC )
		{
			hr = pParams? m_pIC->SetMetaData(m_uSourceId, 0, *pParams, true):
				          m_pIC->SetMetaData(m_uSourceId, 0, CParams(), false);
		}
		return hr;
	}

public:
	// Implementation specific methods
	HRESULT Create(const CImgInfo& info,
                   const IMG_CACHE_SOURCE_PROPERTIES* pProps = NULL)
	{ return Create(CLayerImgInfo(info), pProps);}

	HRESULT Create(const CLayerImgInfo& info,
                   const IMG_CACHE_SOURCE_PROPERTIES* pProps = NULL)
	{
		m_pIC = VtGetImageCacheInstance();
		Deallocate();
		return m_pIC? m_pIC->AddSource(info, 1, m_uSourceId, pProps) : E_NOINIT;
	}

	HRESULT Create(const CImg& src,
                   const IMG_CACHE_SOURCE_PROPERTIES* pProps = NULL)
    {
        HRESULT hr = Create(src.GetImgInfo(), pProps);
		if( hr == S_OK )
		{
			hr = m_pIC->WriteImg(m_uSourceId, 0, src); 
		}
		return hr;
	}

	HRESULT Create(const wchar_t* pBackingBlob, bool bReadOnly = false)
	{
		m_pIC = VtGetImageCacheInstance();
		Deallocate();
		return m_pIC? m_pIC->AddSource(pBackingBlob, m_uSourceId, bReadOnly) : E_NOINIT;
	}

    HRESULT GetSourceProperties(IMG_CACHE_SOURCE_PROPERTIES& Props)
    {
        return m_pIC->GetSourceProperties(m_uSourceId, Props);
    }

	void Deallocate()
	{
		if ( m_pIC && m_uSourceId != UINT_MAX )
		{
			m_pIC->DeleteSource(m_uSourceId);
			m_uSourceId = UINT_MAX;
		}
	}

	virtual HRESULT GenerateLevels(UINT uEndLev, IN const CRect* pRgnInEndLev = NULL)
	{ return m_pIC? m_pIC->GenerateLevels(m_uSourceId, 0, uEndLev, pRgnInEndLev): E_NOINIT;}

	UINT GetCacheSourceId()
	{ return m_uSourceId; }

protected:
	CImageCache* m_pIC;
	UINT m_uSourceId;
};

//+----------------------------------------------------------------------------
//
//  Class:  CTypedImgInCache
//
//  Synopsis:  An implementation of CImgInCache that has a fixed pixel type
//
//-----------------------------------------------------------------------------
template <class T>
class CTypedImgInCache: public CImgInCache
{
public:
	// Implementation specific methods
	HRESULT Create(int iW, int iH, int iBands = 1,
                   const IMG_CACHE_SOURCE_PROPERTIES* pProps = NULL)
	{
        return CImgInCache::Create(
			CImgInfo(iW, iH, VT_IMG_MAKE_TYPE(ElTraits<T>::ElFormat(), iBands)), 
			pProps);
	}

	HRESULT Create(int iW, int iH, int iBands, int iCompositeW, int iCompositeH, 
				   const CPoint& origin, bool bCompWrapX, bool bCompWrapY,
                   const IMG_CACHE_SOURCE_PROPERTIES* pProps = NULL)
	{
        return CImgInCache::Create(
			CLayerImgInfo(CImgInfo(iW, iH, VT_IMG_MAKE_TYPE(ElTraits<T>::ElFormat(), iBands)),
						  iCompositeW, iCompositeH, origin, bCompWrapX, bCompWrapY),
            pProps);
	}
};

typedef CTypedImgInCache<Byte>       CByteImgInCache;
typedef CTypedImgInCache<UInt16>     CShortImgInCache;
typedef CTypedImgInCache<HALF_FLOAT> CHalfFloatImgInCache;
typedef CTypedImgInCache<float>      CFloatImgInCache;

//+-----------------------------------------------------------------------------
//
//  Class:    CCompositeImgInCache
//
//  Synopsis:  analagous to CCompositeImg except stored in the cache
//
//------------------------------------------------------------------------------
template <class T, class TP=T>
class CCompositeImgInCache : public CTypedImgInCache<typename T::ELT>
{
public:
	typename typedef T CompositePixelType;

public:
	HRESULT Create(int iW, int iH,
                   const IMG_CACHE_SOURCE_PROPERTIES* pProps = NULL)
	{
		return CImgInCache::Create(CImgInfo(iW, iH, CCompositeImg<T, TP>::TypeIs()), 
			                                pProps);
	}
	
	HRESULT Create(int iW, int iH, int iCompositeW, int iCompositeH, 
				   const CPoint& origin, bool bCompWrapX=false, bool bCompWrapY=false,
                   const IMG_CACHE_SOURCE_PROPERTIES* pProps = NULL)
	{
        return CImgInCache::Create(
			CLayerImgInfo(CImgInfo(iW, iH, CCompositeImg<T, TP>::TypeIs()), 
			iCompositeW, iCompositeH, origin, bCompWrapX, bCompWrapY),
            pProps);
	}

	HRESULT Fill(T cValue, const RECT *prct = NULL, UINT uLevel = 0) 
	{ return CImgInCache::Fill((Byte *)&cValue, prct, uLevel); }
};

typedef CCompositeImgInCache<LumaPix, LumaPix::ELT>           CLumaImgInCache;
typedef CCompositeImgInCache<LumaShortPix, LumaShortPix::ELT> CLumaShortImgInCache;
typedef CCompositeImgInCache<LumaFloatPix, LumaFloatPix::ELT> CLumaFloatImgInCache;
typedef CCompositeImgInCache<UVPix, UVPix::ELT>               CUVImgInCache;
typedef CCompositeImgInCache<UVShortPix, UVShortPix::ELT>     CUVShortImgInCache;
typedef CCompositeImgInCache<UVFloatPix, UVFloatPix::ELT>     CUVFloatImgInCache;
typedef CCompositeImgInCache<RGBPix>       CRGBImgInCache;
typedef CCompositeImgInCache<RGBShortPix>  CRGBShortImgInCache;
typedef CCompositeImgInCache<RGBFloatPix>  CRGBFloatImgInCache;
typedef CCompositeImgInCache<RGBAPix>      CRGBAImgInCache;
typedef CCompositeImgInCache<RGBAShortPix> CRGBAShortImgInCache;
typedef CCompositeImgInCache<RGBAFloatPix> CRGBAFloatImgInCache;
typedef CCompositeImgInCache<RGBHalfFloatPix>  CRGBHalfFloatImgInCache;
typedef CCompositeImgInCache<RGBAHalfFloatPix> CRGBAHalfFloatImgInCache;

//+----------------------------------------------------------------------------
//
//  Class:  CImgListInCache
//
//  Synopsis:  Maintains a list of images in the cache
//
//-----------------------------------------------------------------------------
class CImgListInCache: public IIndexedImageReaderWriter
{
public:
	CImgListInCache() : m_pIC(NULL), m_uSourceId(UINT_MAX)
	{}

	~CImgListInCache()
	{ Deallocate(); }

public:
	// IIndexedImageReader
	virtual CLayerImgInfo GetImgInfo(UINT uIndex, UINT uLevel = 0)
	{ 
        UINT uSourceId;
		HRESULT hr = Ok(uIndex, uSourceId);
		return (hr==S_OK)? 
			m_pIC->GetImgInfo(uSourceId, uIndex, uLevel): CLayerImgInfo();
	}   

	virtual HRESULT GetMetaData(UINT uIndex, OUT CParams& params)
	{ 
        UINT uSourceId;
		HRESULT hr = Ok(uIndex, uSourceId);
		return (hr==S_OK)? m_pIC->GetMetaData(uSourceId, uIndex, params): hr;
    }

	virtual HRESULT ReadImg(UINT uIndex, OUT CImg& dst, UINT uLevel=0)
	{
        UINT uSourceId;
		HRESULT hr = Ok(uIndex, uSourceId);
		return (hr==S_OK)? m_pIC->ReadImg(uSourceId, uIndex, dst, uLevel): hr; 		 
	}

	virtual HRESULT ReadRegion(UINT uIndex, IN const CRect& region,
							   OUT CImg& dst, UINT uLevel = 0)
	{ 
        UINT uSourceId;
		HRESULT hr = Ok(uIndex, uSourceId);
		return (hr==S_OK)? 
			m_pIC->ReadRegion(uSourceId, uIndex, region, dst, uLevel): hr; 
	}

	virtual HRESULT Prefetch(UINT uIndex, IN const CRect& region, UINT uLevel=0)
	{ 
        UINT uSourceId;
		HRESULT hr = Ok(uIndex, uSourceId);
		return(hr==S_OK)? m_pIC->Prefetch(uSourceId, uIndex, region, uLevel): hr; 
	}

public:
	// IIndexedImageWriter
	virtual HRESULT  WriteImg(UINT uIndex, IN const CImg& src, UINT uLevel=0)
	{ 
        UINT uSourceId;
		HRESULT hr = Ok(uIndex, uSourceId);
		return (hr==S_OK)? m_pIC->WriteImg(uSourceId, uIndex, src, uLevel): hr; 
	}

	virtual HRESULT  WriteRegion(UINT uIndex, IN const CRect& region, 
								 IN const CImg& src, UINT uLevel=0)
	{ 
        UINT uSourceId;
		HRESULT hr = Ok(uIndex, uSourceId);
		return (hr==S_OK)? 
			m_pIC->WriteRegion(uSourceId, uIndex, region, src, uLevel): hr; 
	}

	virtual HRESULT Fill(UINT uIndex, const void* pbValue, const RECT *prct = NULL,
                         UINT uLevel = 0)
	{ 
		HRESULT hr = E_NOINIT;
		if( m_pIC )
		{
            if (!m_vSourceIds.empty())
			    hr = ( prct == NULL )? m_pIC->ClearImg(m_vSourceIds[uIndex], 0, uLevel, pbValue):
				    m_pIC->ClearRegion(m_vSourceIds[uIndex], 0, *prct, uLevel, pbValue);   
            else
			    hr = ( prct == NULL )? m_pIC->ClearImg(m_uSourceId, uIndex, uLevel, pbValue):
				    m_pIC->ClearRegion(m_uSourceId, uIndex, *prct, uLevel, pbValue);   
		}
		return hr;
	}

	virtual HRESULT  SetMetaData(UINT uIndex, const CParams *pParams)
	{
        UINT uSourceId;
		HRESULT hr = Ok(uIndex, uSourceId);
		if( hr == S_OK )
		{
			hr = pParams? m_pIC->SetMetaData(uSourceId, uIndex, *pParams, true):
				          m_pIC->SetMetaData(uSourceId, uIndex, CParams(), false);
		}
		return hr;
	}

	virtual UINT    GetFrameCount()
	{ return m_pIC && m_uSourceId != UINT_MAX ?
        m_pIC->GetFrameCount(m_uSourceId) : (UINT)m_vSourceIds.size();}

public:
	//+-------------------------------------------------------------------------
	//  Method:   Create
	//  Synopsis: creates an image list in the global cache
	//--------------------------------------------------------------------------
	HRESULT Create(const CLayerImgInfo* infos, UINT uCount,
                   const IMG_CACHE_SOURCE_PROPERTIES* prop=NULL)
	{
        if (!m_vSourceIds.empty())
            return E_ACCESSDENIED;

		m_pIC = VtGetImageCacheInstance();

		HRESULT hr = E_NOINIT;
		if ( m_pIC )
			hr = m_pIC->AddSource(infos, uCount, m_uSourceId, prop);
		return hr;
	}

	HRESULT Create(IIndexedImageReader* pReader)
	{
        if (!m_vSourceIds.empty())
            return E_ACCESSDENIED;

		m_pIC = VtGetImageCacheInstance();

		HRESULT hr = E_NOINIT;
		if ( m_pIC )
			hr = m_pIC->AddSource(pReader, m_uSourceId);
		return hr;
	}

	HRESULT Create(const wstring wstrBackingBlob, bool bReadOnly = false)
	{
        if (!m_vSourceIds.empty())
            return E_ACCESSDENIED;

		m_pIC = VtGetImageCacheInstance();

		HRESULT hr = E_NOINIT;
		if ( m_pIC )
			hr = m_pIC->AddSource(wstrBackingBlob, m_uSourceId, bReadOnly);
		return hr;
	}

    HRESULT GetSourceProperties(UINT uIndex, IMG_CACHE_SOURCE_PROPERTIES& Props)
    {
        return m_pIC->GetSourceProperties(GetCacheSourceId(uIndex), Props);
    }

public:
	//+-------------------------------------------------------------------------
	//  Method:   PushBack
	//  Synopsis: creates an image in the global cache and assigns it the last
	//            index in this list       
	//--------------------------------------------------------------------------
	HRESULT PushBack(const CImgInfo& info, 
					 const IMG_CACHE_SOURCE_PROPERTIES* prop=NULL)
	{ return PushBack(CLayerImgInfo(info), prop);}

	HRESULT PushBack(const CLayerImgInfo& info,
					 const IMG_CACHE_SOURCE_PROPERTIES* prop=NULL)
	{
        if (m_uSourceId != UINT_MAX)
            return E_ACCESSDENIED;

		m_pIC = VtGetImageCacheInstance();

		HRESULT hr = E_NOINIT;
		if ( m_pIC )
		{
			UINT uSourceId;
			if( (hr = m_pIC->AddSource(info, 1, uSourceId, prop)) == S_OK )
			{
			    if ( (hr = m_vSourceIds.push_back(uSourceId)) != S_OK )
			    {
				    m_pIC->DeleteSource(uSourceId);
			    }
			}
		}
		return hr;
	}

	HRESULT PushBack(const CImg& src, 
					 const IMG_CACHE_SOURCE_PROPERTIES* prop=NULL)
	{ 
		HRESULT hr = PushBack(src.GetImgInfo(), prop);
		if( hr == S_OK )
		{
			hr = m_pIC->WriteImg(m_vSourceIds.back(), 0, src);
		}
		return hr;
	}

	HRESULT PushBack(IImageReader* pReader)
	{
        if (m_uSourceId != UINT_MAX)
            return E_ACCESSDENIED;

		m_pIC = VtGetImageCacheInstance();

		HRESULT hr = E_NOINIT;
		if ( m_pIC )
		{
    		UINT uSourceId;
            if( (hr = m_pIC->AddSource(pReader, uSourceId)) == S_OK )
            {
                if ( (hr = m_vSourceIds.push_back(uSourceId)) != S_OK )
			    {
				    m_pIC->DeleteSource(uSourceId);
			    }
            }
		}
		return hr;
	}

	HRESULT PushBack(const wchar_t* wstrBackingBlob, bool bReadOnly = false)
	{
        if (m_uSourceId != UINT_MAX)
            return E_ACCESSDENIED;

		m_pIC = VtGetImageCacheInstance();

		HRESULT hr = E_NOINIT;
		if ( m_pIC )
		{
			UINT uSourceId;
			if( (hr = m_pIC->AddSource(wstrBackingBlob, uSourceId, bReadOnly)) == S_OK )
			{
			    if ( (hr = m_vSourceIds.push_back(uSourceId)) != S_OK )
			    {
				    m_pIC->DeleteSource(uSourceId);
			    }
			}
		}
		return hr;
	}

	//+-------------------------------------------------------------------------
	//  Method:   Deallocate()
	//  Synopsis: deletes all images in the cache that this reader refers to
	//--------------------------------------------------------------------------
	void Deallocate()
	{ 
		if ( m_pIC )
		{
            if (m_uSourceId != UINT_MAX)
                m_pIC->DeleteSource(m_uSourceId);

			for ( size_t i = 0; i < m_vSourceIds.size(); i++ )
			{
				m_pIC->DeleteSource(m_vSourceIds[i]);
			}
		}
        m_uSourceId = UINT_MAX;
		m_vSourceIds.resize(0);
	}

	//+-------------------------------------------------------------------------
	//  Method:   GetCacheSourceId(UINT uIndex)
	//  Synopsis: return the source id of this image in the cache
	//--------------------------------------------------------------------------
	UINT GetCacheSourceId(UINT uIndex)
	{ return (uIndex<m_vSourceIds.size())? m_vSourceIds[uIndex]: m_uSourceId; }

protected:
	HRESULT Ok(UINT& uIndex, UINT& uSourceId)
	{
        if (!m_pIC)
            return E_NOINIT;
        if (!m_vSourceIds.empty())
        {
            if (uIndex >= m_vSourceIds.size())
                return E_INVALIDARG;
            uSourceId = m_vSourceIds[uIndex];
            uIndex = 0;
        }
        else
        {
            if (uIndex >= m_pIC->GetFrameCount(m_uSourceId))
                return E_INVALIDARG;
            uSourceId = m_uSourceId;
        }
        return S_OK;
    }

protected:
	CImageCache* m_pIC;

    // can be one source for all images, or one source per image
    UINT m_uSourceId;
	vt::vector<UINT> m_vSourceIds;
};


//+----------------------------------------------------------------------------
//
//  Class:  CTypedImgListInCache
//
//  Synopsis:  Maintains a list of images in the cache
//
//-----------------------------------------------------------------------------
template <class T>
class CTypedImgListInCache: public CImgListInCache
{
public:
	//+-------------------------------------------------------------------------
	//  Method:   PushBack
	//  Synopsis: creates an image in the global cache and assigns it the last
	//            index in this list       
	//--------------------------------------------------------------------------
	HRESULT PushBack(int iW, int iH, int iBands = 1,
					 const IMG_CACHE_SOURCE_PROPERTIES* prop=NULL)
	{
		return CImgListInCache::PushBack(
			CImgInfo(iW, iH, iBands, sizeof(T), CTypedImg<T>::TypeIs()), prop);
	}

	HRESULT PushBack(int iW, int iH, int iCompositeW, int iCompositeH, 
					 const CPoint& origin, bool bCompWrapX=false, bool bCompWrapY=false,
                     int iBands = 1, const IMG_CACHE_SOURCE_PROPERTIES* prop=NULL)
	{
        return CImgListInCache::PushBack(
			CLayerImgInfo(CImgInfo(iW, iH, CTypedImg<T>::TypeIs(iBands)),
						  iCompositeW, iCompositeH, origin, bCompWrapX, bCompWrapY), 
            prop);
	}
};

typedef CTypedImgListInCache<Byte>       CByteImgListInCache;
typedef CTypedImgListInCache<UInt16>     CShortImgListInCache;
typedef CTypedImgListInCache<HALF_FLOAT> CHalfFloatListInCache;
typedef CTypedImgListInCache<float>      CFloatImgListInCache;


//+-----------------------------------------------------------------------
//
//  Class:    CCompositeImgListInCache
//
//  Synopsis:  A list of CCompositeImg stored in the cache 
//
//------------------------------------------------------------------------
template <class T>
class CCompositeImgListInCache : public CTypedImgListInCache<typename T::ELT>
{
public:
	typename typedef T CompositePixelType;

public:
	HRESULT PushBack(int iW, int iH, 
					 const IMG_CACHE_SOURCE_PROPERTIES* prop=NULL)
	{ 
        return CImgListInCache::PushBack( CImgInfo(iW, iH,
                                          CCompositeImg<T>::TypeIs()), prop);
    }

	HRESULT PushBack(int iW, int iH, int iCompositeW, int iCompositeH, 
					 const CPoint& origin, 
					 bool bCompWrapX=false, bool bCompWrapY=false,
					 const IMG_CACHE_SOURCE_PROPERTIES* prop=NULL)
	{ 
        return CImgListInCache::PushBack( 
			CLayerImgInfo( CImgInfo(iW, iH, CCompositeImg<T>::TypeIs()),
						   iCompositeW, iCompositeH, origin, bCompWrapX, bCompWrapY ), 
            prop );
    }
};

typedef CCompositeImgListInCache<LumaPix>      CLumaImgListInCache;
typedef CCompositeImgListInCache<LumaShortPix> CLumaShortImgListInCache;
typedef CCompositeImgListInCache<LumaFloatPix> CLumaFloatImgListInCache;
typedef CCompositeImgListInCache<RGBPix>       CRGBImgListInCache;
typedef CCompositeImgListInCache<RGBShortPix>  CRGBShortImgListInCache;
typedef CCompositeImgListInCache<RGBFloatPix>  CRGBFloatImgListInCache;
typedef CCompositeImgListInCache<RGBAPix>      CRGBAImgListInCache;
typedef CCompositeImgListInCache<RGBAShortPix> CRGBAShortImgListInCache;
typedef CCompositeImgListInCache<RGBAFloatPix> CRGBAFloatImgListInCache;

};