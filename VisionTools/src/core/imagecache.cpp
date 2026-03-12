//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      the image cache class
//
//  History:
//      2004/11/8-mattu
//          Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_image.h"
#include "vt_kernel.h"
#include "vt_convert.h"
#include "vt_layer.h"
#include "vt_readerwriter.h"
#include "vt_pixeliterators.h"
#include "vt_separablefilter.h"
#include "vt_taskmanager.h"
#include "vt_transform.h"
#include "vt_pyramid.h"
#include "vt_blobstore.h"
#include "vt_fileblobstore.h"
#include "vt_imagecache.h"
#include "vt_singleton.h"
#include "vt_utils.h"
#include "vtiio.h"

using namespace vt;
	
// disabling for now but opening VisionTools bug 659
#pragma warning ( disable : 26110 )

// the single instance of CImageCache cache for an application
typedef CVTSingleton<CImageCache> CImageCacheSingle;
CImageCacheSingle* CImageCacheSingle::m_singletonInstance = NULL;
LONG CImageCacheSingle::m_uRefCount = 0;

CImageCache* vt::VtGetImageCacheInstance()
{
    return CImageCacheSingle::GetInstance();
}

// functions that handle startup and shutdown of the image cache
HRESULT vt::ImageCacheStartup()
{
	return CImageCacheSingle::Startup();
}

HRESULT vt::ImageCacheShutdown()
{
	return CImageCacheSingle::Shutdown();
}

// implementation of CImageCache
void
CImageCache::CICBlobCallback::Callback(HRESULT hr)
{
    VtGetImageCacheInstance()->HandleBlock(this, hr);
}

HRESULT 
CImageCache::StartupInternal()
{
	HRESULT hr = S_OK;

    wstring_b<MAX_PATH> wstrTempPath;

	VT_HR_EXIT( HRESULT_FROM_WIN32(m_cs.Startup(8000)) );

#if defined(_M_AMD64) && !defined(VT_WINRT)
    if ((m_hHeap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0)) == NULL)
#else
    if ((m_hHeap = GetProcessHeap()) == NULL)
#endif
        VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

    if ((m_hBackingEvent =
        CSystem::CreateEvent(FALSE, FALSE)) == NULL)
        VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

    DWORD dwThreadId;
    if ((m_hBackingThread =
        CSystem::CreateThread(NULL, 0, StaticBackingProc,
                              this, 0, &dwThreadId)) == NULL)
        VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

    // Set Windows file system as default backing store.
    m_pFileStore = VT_NOTHROWNEW CFileBlobStore;
    VT_PTR_OOM_EXIT( m_pFileStore ); 
    VT_HR_EXIT( SetBackingStore(m_pFileStore) );

    // From MSDN "File Buffering":
    // Therefore, in most situations, page-aligned memory will also be sector-aligned,
    // because the case where the sector size is larger than the page size is rare.
    SYSTEM_INFO si;
    vt::CSystem::GetSystemInfo(&si);
    m_uPageSize = si.dwPageSize;

Exit:
    return hr;
}

// Named blobs are allocated all at one; un-named are incremented 1 GB at a time.
#define BLOB_SIZE_INCR 0x40000000

//+----------------------------------------------------------------------------
//
//  Class:     CImageCache
//
//-----------------------------------------------------------------------------
CImageCache::CImageCache()
{
    m_pHeadBlocks = m_pTailBlocks = m_pHeadReads = m_pTailReads =
        m_pEmptyBlocks = NULL;
    m_pbEmptyAlloc = NULL;
    m_cbEmptyAlloc = 0;
    m_pCallbacks = NULL;
    m_cBackingReads = 0;
    m_cBackingWrites = 0;

    m_hHeap = NULL;
    m_pBlobStore = NULL;
    m_pFileStore = NULL;
    m_uPageSize = 0;

    m_uMaxCacheSystemMemory = m_uHighWater = _UI64_MAX;
    m_uMemoryUsed = m_uMemoryPending = 0;
    m_pFlushSource = NULL;
}


CImageCache::~CImageCache()
{
    for (UINT i = 0; i < static_cast<UINT>(m_vecSources.size()); i++)
    {
        if (m_vecSources[i] != NULL)
            DeleteSource( i );
    }

    m_vecSources.clear();

    VT_ASSERT( m_uMemoryUsed == 0 && m_uMemoryPending == 0 &&
               m_cBackingReads == 0 && m_cBackingWrites == 0 );

    while (m_pEmptyBlocks != NULL)
    {
        VT_ASSERT( m_pEmptyBlocks->pbAlloc == NULL &&
                   m_pEmptyBlocks->pbData == NULL &&
                   m_pEmptyBlocks->cbAlloc == 0 &&
                   m_pEmptyBlocks->cbData == 0 );

        LINK* pBlock = m_pEmptyBlocks;
        m_pEmptyBlocks = m_pEmptyBlocks->pNext;

        delete pBlock;
    }

    if (m_pbEmptyAlloc != NULL)
    {
        VT_ASSERT( m_cbEmptyAlloc != 0 );
        HeapFree(m_hHeap, 0, m_pbEmptyAlloc);
    }

    while (m_pCallbacks != NULL)
    {
        CICBlobCallback* pCallback = m_pCallbacks;
        m_pCallbacks = (CICBlobCallback *) m_pCallbacks->m_pNext;

        delete pCallback;
    }

    // Tell backing thread to exit.
    HANDLE hBackingThread = m_hBackingThread;
    m_hBackingThread = NULL;
    SetEvent(m_hBackingEvent);
    vt::CSystem::WaitForSingleObjectEx(hBackingThread, INFINITE, FALSE);
    CloseHandle(hBackingThread);

    CloseHandle(m_hBackingEvent);

    if (m_pBlobStore != m_pFileStore)
        delete m_pBlobStore;
    delete m_pFileStore;

#if defined(_M_AMD64) && !defined(VT_WINRT)
    HeapDestroy(m_hHeap);
#endif
}


HRESULT
CImageCache::SetMaxMemoryUsage( UINT64 uBytes )
{
    HRESULT hr = S_OK;

	m_cs.Enter();

    UINT64 uMaxCacheSystemMemory = uBytes == 0 ? _UI64_MAX : uBytes;

    if (m_uMaxCacheSystemMemory != uMaxCacheSystemMemory)
    {
        m_uMaxCacheSystemMemory = uMaxCacheSystemMemory;

        if (uBytes == 0)
        {
            // If 0 never use backing store.
            m_uHighWater = _UI64_MAX;
        }
        else
        {
            // High water mark at 3/4 of cache.
            m_uHighWater =
                (m_uMaxCacheSystemMemory >> 1) +
                (m_uMaxCacheSystemMemory >> 2);
        }

        if (m_uMemoryUsed > m_uMaxCacheSystemMemory ||
            m_uMemoryUsed - m_uMemoryPending > m_uHighWater)
            VT_HR_EXIT( CheckForMemory() );
    }

Exit:
	m_cs.Leave();

    return hr;
}


HRESULT
CImageCache::SetBackingStore(IBlobStore* pBlobStore)
{
    HRESULT hr = S_OK;

	m_cs.Enter();

    if (pBlobStore == NULL)
        VT_HR_EXIT( E_INVALIDARG );

    // Don't reset backing store if sources outstanding.
    if (!m_vecSources.empty())
        VT_HR_EXIT( E_ACCESSDENIED );

    if (m_pBlobStore != m_pFileStore)
        delete m_pBlobStore;

    m_pBlobStore = pBlobStore;

Exit:
	m_cs.Leave();

    return hr;
}

IBlobStore*
CImageCache::GetBackingStore()
{
    return m_pBlobStore;
}


UINT64
CImageCache::GetMaxMemoryUsage()
{
    return m_uMaxCacheSystemMemory;
}

UINT64
CImageCache::GetMemoryUsage()
{
    return m_uMemoryUsed;
}


void
CImageCache::FlushCache()
{
    m_cs.Enter();

    while (m_uMemoryUsed > 0)
    {
        // Temporarily set high water to 0.
        UINT64 uHighWater = m_uHighWater;

        m_uHighWater = 0;

        CheckForMemory(true);

        if (m_uHighWater == 0)
            m_uHighWater = uHighWater;
    }

    m_cs.Leave();
}


DWORD
CImageCache::BackingStoreProc()
{
    HRESULT hr = S_OK;

    while (m_hBackingThread != NULL)
    {
        if (vt::CSystem::WaitForSingleObjectEx(m_hBackingEvent, INFINITE, TRUE) == WAIT_FAILED)
            VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

        m_cs.Enter();

        // Do pending reads.
        for (LINK* pBlock = m_pTailReads; pBlock != NULL &&
            m_cBackingReads < MAXIMUM_WAIT_OBJECTS; pBlock = m_pTailReads)
        {
            // Remove block from read list.
            m_pTailReads = pBlock->pPrev;
            if (m_pTailReads != NULL)
                m_pTailReads->pNext = NULL;
            else
                m_pHeadReads = NULL;
            pBlock->pNext = pBlock->pPrev = NULL;

            HRESULT hrSource = pBlock->pSource->hrBackingResult;
            if (hrSource == S_OK)
                hrSource = BlobBlock(pBlock, false);
            // Note block can be removed during BlobBlock().
            if (hrSource != S_OK && pBlock->pCallback != NULL)
            {
                // Clean up block's callback and put on tail of MRU list for eventual removal.
                m_cBackingReads++;
                HandleBlock(pBlock->pCallback, pBlock->pSource->hrBackingResult);
            }
            if (pBlock->pSource != NULL)
                pBlock->pSource->hrBackingResult = hrSource;
        }

        // Do writes if necessary.
        for (LINK* pBlock = m_pTailBlocks; pBlock != NULL &&
            (m_pFlushSource != NULL ||
             m_uMemoryUsed > m_uMaxCacheSystemMemory ||
             m_uMemoryUsed - m_uMemoryPending > m_uHighWater) &&
            m_cBackingWrites < MAXIMUM_WAIT_OBJECTS; pBlock = pBlock->pPrev)
        {
            VT_ASSERT( pBlock->pSource != NULL );
            ANALYZE_ASSUME( pBlock->pSource != NULL );
            if (m_pFlushSource == pBlock->pSource ||
                m_uMemoryUsed > m_uMaxCacheSystemMemory ||
                m_uMemoryUsed - m_uMemoryPending > m_uHighWater)
            {
                // check if there is only one reference to block
                if (pBlock->dwThreadId == 0 && pBlock->dwThreadCount == 0)
                {
                    // If no pending backing file read or write can back up block.
                    if (pBlock->pCallback == NULL)
                    {
                        if ((pBlock->pBlock->uTractNum & BLOCK_IN_STORE) == 0x0 &&
                            pBlock->pSource->bUseBackingStore)
                        {
                            HRESULT hrSource = pBlock->pSource->hrBackingResult;
                            if (hrSource == S_OK)
                                hrSource = BackupBlock(pBlock);
                            // Note block can be removed during BackupBlock().
                            if (pBlock->pSource != NULL)
                                pBlock->pSource->hrBackingResult = hrSource;
                        }
                    }
                }
            }
        }

        m_cs.Leave();
    }

Exit:
    return hr == S_OK ? 1 : 0;
}


HRESULT
CImageCache::AddSource(const wchar_t* pwszBackingBlob, OUT UINT& uSourceId,
                       bool bReadOnly)
{
    HRESULT hr = S_OK;

    SOURCE* pSource = NULL;
    CVtiReaderWriter* pBackingReaderWriter = NULL;
    vector<CLayerImgInfo> vecInfos;

	if (pwszBackingBlob==NULL || wcslen(pwszBackingBlob)==0 )
        VT_HR_EXIT( E_INVALIDARG );

    // Attempt to open existing backing blob.
    pBackingReaderWriter = VT_NOTHROWNEW CVtiReaderWriter;
    VT_PTR_OOM_EXIT( pBackingReaderWriter );

    VT_HR_EXIT( pBackingReaderWriter->OpenBlob(pwszBackingBlob, m_pBlobStore, bReadOnly) );

    UINT uCount = pBackingReaderWriter->GetFrameCount();

    for (UINT i = 0; i < uCount; i++)
        VT_HR_EXIT( vecInfos.push_back(pBackingReaderWriter->GetImgInfo(i)) );
    const CLayerImgInfo* infos = vecInfos.begin();

    pSource = VT_NOTHROWNEW SOURCE;
    VT_PTR_OOM_EXIT( pSource );
    VT_HR_EXIT( pSource->vecImages.resize(uCount) );

    VT_HR_EXIT( pSource->wstrBackingBlob.assign(pwszBackingBlob) );
    pSource->pBackingReaderWriter = pBackingReaderWriter;
    pSource->pBackingBlob = pBackingReaderWriter->GetBlob();
    pSource->bAutoLevelGenerate = false;
    pSource->iAlignment = pBackingReaderWriter->GetTractSize();
    pSource->bReadOnly = bReadOnly;

    // Alignment must be multiple of blob store tract size.
    if (pSource->iAlignment > 0 &&
        pSource->iAlignment % m_pBlobStore->GetTractSize() != 0)
        VT_HR_EXIT( E_INVALIDARG );

    VT_HR_EXIT( AddSource(pSource, uSourceId, uCount, infos, NULL) );

Exit:
    if (hr != S_OK)
    {
        delete pSource;
        delete pBackingReaderWriter;
    }

    return hr;
}

HRESULT
CImageCache::AddSource(const CLayerImgInfo& info, UINT uCount, OUT UINT& uSourceId,
                       const IMG_CACHE_SOURCE_PROPERTIES* pProps)
{
    HRESULT hr = S_OK;

    SOURCE* pSource = NULL;

	if (uCount == 0 || info.type==OBJ_UNDEFINED )
        VT_HR_EXIT( E_INVALIDARG );

    pSource = VT_NOTHROWNEW SOURCE;
    VT_PTR_OOM_EXIT( pSource );
    VT_HR_EXIT( pSource->vecImages.resize(uCount) );

    VT_HR_EXIT( AddSource(pSource, uSourceId, 1, &info, pProps) );

Exit:
    if (hr != S_OK)
        delete pSource;

    return hr;
}

HRESULT
CImageCache::AddSource(const CLayerImgInfo* infos, UINT uCount, OUT UINT& uSourceId,
                       const IMG_CACHE_SOURCE_PROPERTIES* pProps)
{
    HRESULT hr = S_OK;

    SOURCE* pSource = NULL;

    if (infos == NULL || uCount == 0)
        VT_HR_EXIT( E_INVALIDARG );

    pSource = VT_NOTHROWNEW SOURCE;
    VT_PTR_OOM_EXIT( pSource );
    VT_HR_EXIT( pSource->vecImages.resize(uCount) );

    VT_HR_EXIT( AddSource(pSource, uSourceId, uCount, infos, pProps) );

Exit:
    if (hr != S_OK)
        delete pSource;

    return hr;
}

HRESULT
CImageCache::AddSource(IImageReader* pReader, OUT UINT& uSourceId,
                       bool bUseBackingStore, const wchar_t* pBackingBlob)
{
    HRESULT hr = S_OK;

    SOURCE* pSource = NULL;

    IMG_CACHE_SOURCE_PROPERTIES props;

    if (pReader == NULL)
        VT_HR_EXIT( E_INVALIDARG );

    pSource = VT_NOTHROWNEW SOURCE;
    VT_PTR_OOM_EXIT( pSource );
    VT_HR_EXIT( pSource->vecImages.resize(1) );

    pSource->pImageReader = pReader;
    pSource->bUseBackingStore = bUseBackingStore;

    props.bAutoLevelGenerate = false;
    props.pBackingBlob = pBackingBlob;
    {
        const CLayerImgInfo info = pReader->GetImgInfo();
        VT_HR_EXIT( AddSource(pSource, uSourceId, 1, &info, &props) );
    }

Exit:
    if (hr != S_OK)
        delete pSource;

    return hr;
}

HRESULT
CImageCache::AddSource(IIndexedImageReader* pIdxReader, OUT UINT& uSourceId,
                       bool bUseBackingStore, const wchar_t* pBackingBlob)
{
    HRESULT hr = S_OK;

    SOURCE* pSource = NULL;

    IMG_CACHE_SOURCE_PROPERTIES props;

    vector<CLayerImgInfo> vecInfos;

    if (pIdxReader == NULL)
        VT_HR_EXIT( E_INVALIDARG );

    pSource = VT_NOTHROWNEW SOURCE;
    VT_PTR_OOM_EXIT( pSource );
    VT_HR_EXIT( pSource->vecImages.resize(pIdxReader->GetFrameCount()) );

    pSource->pIdxImageReader = pIdxReader;
    pSource->bUseBackingStore = bUseBackingStore;

    UINT uCount = pIdxReader->GetFrameCount();
    VT_HR_EXIT( vecInfos.resize(uCount) );
    for (UINT i = 0; i < uCount; i++)
        vecInfos[i] = pIdxReader->GetImgInfo(i);

    props.bAutoLevelGenerate = false;
    props.pBackingBlob = pBackingBlob;
    VT_HR_EXIT( AddSource(pSource, uSourceId, uCount, vecInfos.begin(), &props) );

Exit:
    if (hr != S_OK)
        delete pSource;

    return hr;
}

HRESULT
CImageCache::AddSource(SOURCE* source, OUT UINT& uSourceId,
                       UINT cInfos, const CLayerImgInfo* infos,
                       const IMG_CACHE_SOURCE_PROPERTIES* pProps)
{
    HRESULT hr = S_OK;

    UINT uId = UINT_MAX;

    bool bInBlob = !source->wstrBackingBlob.empty();

    if (pProps != NULL)
    {
        source->bAutoLevelGenerate = pProps->bAutoLevelGenerate;

        if (pProps->pBackingBlob != NULL)
            VT_HR_EXIT( source->wstrBackingBlob.assign(pProps->pBackingBlob) );

        source->iMaxLevel = pProps->iMaxLevel;

        // Alignment must be multiple of blob store tract size.
        if (pProps->iAlignment > 0 &&
            pProps->iAlignment % m_pBlobStore->GetTractSize() != 0)
            VT_HR_EXIT( E_INVALIDARG );

        source->iAlignment = pProps->iAlignment;
    }

    source->iAlignment = VtMax((UINT) source->iAlignment, m_pBlobStore->GetTractSize());

    if (source->bAutoLevelGenerate)
    {
        C14641Kernel kernel;

        // Initialize even and odd filter kernel sets.
        VT_HR_EXIT( source->ks.Create(1, 2) );
        VT_HR_EXIT( source->ks1.Create(1, 2) );

        const C1dKernel* pKernel = pProps != NULL && pProps->pKernel != NULL ?
			pProps->pKernel : &kernel.AsKernel();
        VT_HR_EXIT( source->ks.Set(0, -pKernel->Center(), *pKernel) );
        VT_HR_EXIT( source->ks1.Set(0, -pKernel->Center() + 1, *pKernel) );
    }

    for (size_t uIndex = 0; uIndex < source->vecImages.size(); uIndex++)
    {
        IMAGE& image = source->vecImages[uIndex];

        VT_HR_EXIT( image.vecLevels.resize(1) );

        CLayerImgInfo info = cInfos == 1 ? infos[0] : infos[uIndex];
		// return error if info.type is unreasonable
        if (info.type == OBJ_UNDEFINED || 
			(info.type & ~(VT_IMG_ELFRMT_MASK|VT_IMG_BANDS_MASK|
			               VT_IMG_PIXFRMT_MASK|VT_IMG_FIXED_ELFRMT_MASK|
						   VT_IMG_FIXED_PIXFRMT_MASK)) != 0 )
		{
            VT_HR_EXIT( E_INVALIDARG );
		}
        image.vecLevels[0].info = info;

        // Set default clear value (0th entry of vecClearVals) to 0.
        VT_HR_EXIT( image.vecClearVals.resize(1) );
        image.vecClearVals[0] = VT_NOTHROWNEW Byte[info.PixSize()];
        VT_PTR_OOM_EXIT( image.vecClearVals[0] );
        VtMemset(image.vecClearVals[0], 0, info.PixSize());

        // Increase block size to approach tract size.
        if (pProps != NULL)
        {
            image.uBlockSizeX = pProps->uBlockSizeX;
            image.uBlockSizeY = pProps->uBlockSizeY;
        }
        while (image.uBlockSizeX * image.uBlockSizeY * infos[uIndex].PixSize() <=
            (UINT) (source->iAlignment >> 1))
        {
            image.uBlockSizeX <<= 1;
            if (image.uBlockSizeX * image.uBlockSizeY * infos[uIndex].PixSize() <=
                (UINT) (source->iAlignment >> 1))
               image.uBlockSizeY <<= 1;
        }
    }

    // Set blob size to source size.
    VT_HR_EXIT( ComputeSourceSize(source, bInBlob) );

    m_cs.Enter();

    // Find empty source.
    for (uId = 0; uId < (UINT)m_vecSources.size(); uId++)
    {
        if (m_vecSources[uId] == NULL)
            break;
    }
    // If no empty source found add one.
    if (uId < (UINT)m_vecSources.size())
        m_vecSources[uId] = source;
    else
        hr = m_vecSources.push_back(source);
 
    if (hr == S_OK)
    {
        // Create blob now if named, else wait until if/when needed.
        if (!bInBlob && !source->wstrBackingBlob.empty())
        {
            // Check if any other source is using this name.
            for (size_t i = 0; i < m_vecSources.size(); i++)
            {
                if (m_vecSources[i] != NULL &&
                    m_vecSources[i] != source &&
                    m_vecSources[i]->pBackingReaderWriter != NULL &&
                    m_vecSources[i]->wstrBackingBlob.compare(source->wstrBackingBlob) == 0)
                {
                    hr = HRESULT_FROM_WIN32( ERROR_FILE_EXISTS );
                    break;
                }
            }
        }
    }

    m_cs.Leave();

    VT_HR_EXIT( hr );

    // Create blob now if named, else wait until if/when needed.
    if (!bInBlob && !source->wstrBackingBlob.empty())
    {
        // Try to create a blob.
        VT_HR_EXIT( CreateBlob(source) );
    }

    uSourceId = uId;

Exit:
    if (hr != S_OK)
    {
        if (uId < UINT_MAX)
        {
            // Own the source.
            source->lRef++;
            // Delete from the source vector.
            DeleteSource(uId);
        }
    }

    return hr;
}

HRESULT
CImageCache::GetSourceProperties(UINT uSourceId, IMG_CACHE_SOURCE_PROPERTIES& Props)
{
    HRESULT hr = S_OK;

    m_cs.Enter();

    if( uSourceId >= m_vecSources.size() ||
        m_vecSources[uSourceId] == NULL)
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    SOURCE* source = m_vecSources[uSourceId];

    Props = IMG_CACHE_SOURCE_PROPERTIES();
    Props.iMaxLevel = source->iMaxLevel;
    Props.iAlignment = source->iAlignment;

Exit:
    m_cs.Leave();

    return hr;
}

HRESULT
CImageCache::GetSourceStatistics(UINT uSourceId, IMG_CACHE_SOURCE_STATISTICS& Stats)
{
    HRESULT hr = S_OK;

    m_cs.Enter();

    if( uSourceId >= m_vecSources.size() ||
        m_vecSources[uSourceId] == NULL)
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    SOURCE* source = m_vecSources[uSourceId];

    Stats = source->Stats;

Exit:
    m_cs.Leave();

    return hr;
}


HRESULT
CImageCache::FlushSource(UINT uSourceId, CTaskProgress* pProg)
{
    HRESULT hr = S_OK;

    m_cs.Enter();

    if( uSourceId >= m_vecSources.size() ||
        m_vecSources[uSourceId] == NULL)
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    SOURCE* source = m_vecSources[uSourceId];

    VT_HR_EXIT( FlushSource(source, pProg) );

Exit:
    m_cs.Leave();

    return hr;
}

HRESULT
CImageCache::ClearSource(UINT uSourceId, IN const void* pClearVal)
{
    HRESULT hr = S_OK;

    m_cs.Enter();

    if( uSourceId >= m_vecSources.size() ||
        m_vecSources[uSourceId] == NULL)
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    SOURCE* source = m_vecSources[uSourceId];

    UINT uClearVal = 0;
    for(size_t uIndex = 0; uIndex < source->vecImages.size(); uIndex++)
    {
        IMAGE& image = source->vecImages[uIndex];

        if (pClearVal != NULL)
        {
            // If clear value specified then all pixel sizes must match.
            if (image.vecLevels[0].info.PixSize() !=
                source->vecImages[0].vecLevels[0].info.PixSize())
                VT_HR_EXIT( E_INVALIDARG );

            VT_HR_EXIT( AddClearValue(image, pClearVal, uClearVal) );
        }
    }

    VT_HR_EXIT( ClearSource(m_vecSources[uSourceId], uClearVal) );

Exit:
    m_cs.Leave();

    return hr;
}

HRESULT
CImageCache::DeleteSource(UINT uSourceId)
{
    HRESULT hr = S_OK;

    m_cs.Enter();

    if( uSourceId >= m_vecSources.size() ||
        m_vecSources[uSourceId] == NULL)
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    SOURCE* source = m_vecSources[uSourceId];
    m_vecSources[uSourceId] = NULL;

    // Check if all sources have been deleted and clear vector if so.
    for (uSourceId = 0; uSourceId < (UINT)m_vecSources.size(); uSourceId++)
        if (m_vecSources[uSourceId] != NULL)
            break;
    if (uSourceId == m_vecSources.size())
        m_vecSources.clear();

    if (!source->wstrBackingBlob.empty() && source->pBackingReaderWriter != NULL)
        VT_HR_EXIT( FlushSource(source) );

    VT_HR_EXIT( ClearSource(source) );

    if (!source->wstrBackingBlob.empty())
    {
        // Remove backing blob if named.
        delete source->pBackingReaderWriter;
        source->pBackingReaderWriter = NULL;
        source->pBackingBlob = NULL;
    }

    // Check no outstanding reads/writes to deleted source.
    if (--source->lRef == 0)
        delete source;

Exit:
    m_cs.Leave();

    return hr;
}

HRESULT
CImageCache::FlushSource(SOURCE* source, CTaskProgress* pProg)
{
    HRESULT hr = S_OK;

    // Start flushing this source to backing store.
    m_pFlushSource = source;

    UINT cBlocksInBlob, cBlocks;
    do
    {
        // Check the backing store thread.
        VT_HR_EXIT( source->hrBackingResult );

        // Kick the backing store thread.
        if (!SetEvent(m_hBackingEvent))
            VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

        // Write through all the levels.
        for(UINT uIndex = 0; uIndex < static_cast<UINT>(source->vecImages.size()); uIndex++)
        {
            IMAGE& image = source->vecImages[uIndex];

            for(UINT uLevel = 0; uLevel < static_cast<UINT>(image.vecLevels.size()); uLevel++)
            {
                LEVEL& level = image.vecLevels[uLevel];

                // Write through level.
                VT_HR_EXIT( DoRegion(source, uIndex, level.info.Rect(), NULL, uLevel,
                                     false, true, 0, false) );
            }
        }

        // Make sure all the blocks are into the backing store.
        cBlocksInBlob = cBlocks = 0;
        for (size_t uIndex = 0; uIndex < source->vecImages.size(); uIndex++)
        {
            IMAGE& image = source->vecImages[uIndex];

            for (size_t uLevel = 0; uLevel < image.vecLevels.size(); uLevel++)
            {
                LEVEL& level = image.vecLevels[uLevel];

                for (size_t uBlock = 0; uBlock < level.vecBlocks.size(); uBlock++)
                {
                    BLOCK& block = level.vecBlocks[uBlock];

                    cBlocks++;
                    if ((block.uTractNum & BLOCK_IN_STORE) != 0x0)
                        cBlocksInBlob++;
                }
            }
        }

        if (pProg != NULL)
            pProg->ReportProgress(cBlocksInBlob * 100.f / cBlocks);

        if (cBlocksInBlob < cBlocks)
        {
            // Kick the backing store thread.
            if (!SetEvent(m_hBackingEvent))
                VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

            // Give other threads a chance to give up blocks.
            m_cs.Leave();
            CSystem::Sleep(1);
            m_cs.Enter();
        }
    }
    while (cBlocksInBlob < cBlocks);

Exit:
    // Stop flushing this source to backing store.
    m_pFlushSource = NULL;

    return hr;
}

HRESULT
CImageCache::ClearSource(SOURCE* source, UINT uClearVal)
{
    HRESULT hr = S_OK;

    bool bBlocksInBlob;
    do
    {
        // Clear all the levels.
        for( UINT uIndex = 0; uIndex < static_cast<UINT>(source->vecImages.size()); uIndex++)
        {
            IMAGE& image = source->vecImages[uIndex];

            for( UINT uLevel = 0; uLevel < static_cast<UINT>(image.vecLevels.size()); uLevel++)
            {
                LEVEL& level = image.vecLevels[uLevel];

                // Clear level.
                VT_HR_EXIT( DoRegion(source, uIndex, level.info.Rect(), NULL, uLevel,
                                     true, false, uClearVal) );
            }
        }

        // Make sure all the blocks are out of backing store.
        bBlocksInBlob = false;
        for( size_t uIndex = 0; uIndex < source->vecImages.size() && !bBlocksInBlob; uIndex++)
        {
            IMAGE& image = source->vecImages[uIndex];

            for( size_t uLevel = 0; uLevel < image.vecLevels.size() && !bBlocksInBlob; uLevel++)
            {
                LEVEL& level = image.vecLevels[uLevel];

                for( size_t uBlock = 0; uBlock < level.vecBlocks.size() && !bBlocksInBlob; uBlock++)
                {
                    BLOCK& block = image.vecLevels[uLevel].vecBlocks[uBlock];

                    if ((block.uTractNum & BLOCK_IN_STORE) != 0x0)
                        bBlocksInBlob = true;
                    else if (source->wstrBackingBlob.empty())
                        block.uTractNum = TRACT_MASK;
                }
            }
        }
    }
    while (bBlocksInBlob);

    if (source->wstrBackingBlob.empty())
    {
        // Remove backing blob if un-named.
        if (m_pBlobStore != NULL && source->pBackingBlob != NULL)
            VT_HR_EXIT( m_pBlobStore->CloseBlob(source->pBackingBlob) );
        source->pBackingBlob = NULL;

        source->uTractNum = 0;
    }

Exit:
    return hr;
}

HRESULT
CImageCache::AddClearValue(IMAGE& image,
                           IN const void* pClearVal, OUT UINT& uClearVal)
{
    HRESULT hr = S_OK;

    CImgInfo info = image.vecLevels[0].info;

    UINT uVal = 1;
    for (; uVal < (UINT)image.vecClearVals.size(); uVal++)
        if (memcmp(image.vecClearVals[uVal], pClearVal, info.PixSize()) == 0)
            break;

    // If no matching clear value found add one.
    if (uVal == (UINT)image.vecClearVals.size())
    {
        VT_HR_EXIT( image.vecClearVals.resize(uVal + 1) );
// Disabling C28193 here as it's a false positive from the compiler.
#pragma warning(push)
#pragma warning(disable: 28193)
        image.vecClearVals[uVal] = VT_NOTHROWNEW BYTE[info.PixSize()];
#pragma warning(pop)
        VT_PTR_OOM_EXIT( image.vecClearVals[uVal] );
        memcpy(image.vecClearVals[uVal], pClearVal, info.PixSize());
    }

    uClearVal = uVal;

Exit:
    return hr;
}


UINT
CImageCache::GetFrameCount(UINT uSourceId)
{
    UINT uFrameCount = 0;

    m_cs.Enter();

    if (uSourceId < m_vecSources.size() &&
        m_vecSources[uSourceId] != NULL)
        uFrameCount = (UINT) m_vecSources[uSourceId]->vecImages.size();

    m_cs.Leave();

    return uFrameCount;
}

VOID
CImageCache::GetBlockSize(UINT uSourceId, UINT uIndex, UINT& uBlockSizeX, UINT& uBlockSizeY)
{
    m_cs.Enter();

    if (uSourceId < m_vecSources.size() &&
        m_vecSources[uSourceId] != NULL &&
        uIndex < m_vecSources[uSourceId]->vecImages.size())
    {
        uBlockSizeX = m_vecSources[uSourceId]->vecImages[uIndex].uBlockSizeX;
        uBlockSizeY = m_vecSources[uSourceId]->vecImages[uIndex].uBlockSizeY;
    }

    m_cs.Leave();
}

CLayerImgInfo
CImageCache::GetImgInfo(UINT uSourceId, UINT uIndex, UINT uLevel)
{
    m_cs.Enter();

    CLayerImgInfo info;

    if( uSourceId < m_vecSources.size() &&
        m_vecSources[uSourceId] != NULL )
    {
        info = GetImgInfo(m_vecSources[uSourceId], uIndex, uLevel);
    }

    m_cs.Leave();

    return info;
}

CLayerImgInfo
CImageCache::GetImgInfo(SOURCE* source, UINT uIndex, UINT uLevel)
{
    CLayerImgInfo info;

    if( uIndex < source->vecImages.size() )
    {
        IMAGE& image = source->vecImages[uIndex];

		VT_ASSERT( image.vecLevels.size() != 0 );

        if (uLevel < image.vecLevels.size())
            info = image.vecLevels[uLevel].info;
        else
        {
            // Set composite width and height for 0-sized image.
            if (source->pImageReader != NULL)
                info = source->pImageReader->GetImgInfo(uLevel);
            else if (source->pIdxImageReader != NULL)
                info = source->pIdxImageReader->GetImgInfo(uIndex, uLevel);
            else
                info = InfoAtLevel(image.vecLevels[0].info, uLevel);
        }
    }

    return info;
}


HRESULT
CImageCache::GetMetaData(UINT uSourceId, UINT uIndex, CParams& params)
{
    HRESULT hr = S_OK;

    m_cs.Enter();

    if( uSourceId >= m_vecSources.size() ||
        m_vecSources[uSourceId] == NULL ||
        uIndex >= m_vecSources[uSourceId]->vecImages.size() )
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    SOURCE* source = m_vecSources[uSourceId];

    IMAGE& image = source->vecImages[uIndex];

    if (image.params != NULL)
        params = *image.params;
    else if (source->pImageReader != NULL)
        VT_HR_EXIT( source->pImageReader->GetMetaData(params) );
    else if (source->pIdxImageReader != NULL)
        VT_HR_EXIT( source->pIdxImageReader->GetMetaData(uIndex, params) );

Exit:
    m_cs.Leave();

    return hr;
}


void
CImageCache::CancelRead(LINK *pBlock)
{
    VT_ASSERT( pBlock->pCallback != NULL );

    // Check if still in the read list.
    if (pBlock->pNext == NULL && pBlock->pPrev == NULL &&
        pBlock != m_pHeadReads)
    {
        VT_ASSERT( pBlock != m_pTailReads );
        return;
    }

    // remove this block from its list
    if (pBlock == m_pHeadReads)
        m_pHeadReads = pBlock->pNext;
    else if (pBlock->pPrev != NULL)
        pBlock->pPrev->pNext = pBlock->pNext;
    if (pBlock == m_pTailReads)
        m_pTailReads = pBlock->pPrev;
    else if (pBlock->pNext != NULL)
        pBlock->pNext->pPrev = pBlock->pPrev;

    // add to the callbacks
    pBlock->pCallback->m_pNext = m_pCallbacks;
    m_pCallbacks = pBlock->pCallback;

    // Indicate asynchronous file operation over.
    pBlock->pCallback = NULL;
}

void
CImageCache::HandleBlock(CICBlobCallback* pCallback, HRESULT hr)
{
    m_cs.Enter();

    LINK *pBlock = pCallback->m_pLink;

    VT_ASSERT( pBlock->pCallback->m_pLink == pBlock );

    // add to the callbacks
    pCallback->m_pNext = m_pCallbacks;
    m_pCallbacks = pBlock->pCallback;

    // Indicate asynchronous file operation over.
    pBlock->pCallback = NULL;

    if ((pBlock->pBlock->uTractNum & BLOCK_IN_STORE) == 0x0)
    {
        // Subtract from backing writes.
        VT_ASSERT( m_cBackingWrites > 0 );
        m_cBackingWrites--;

        // Backing store is now valid.
        if (hr == S_OK)
        {
            pBlock->pBlock->uTractNum |= BLOCK_IN_STORE;

            m_uMemoryPending += pBlock->cbAlloc;
            VT_ASSERT( m_uMemoryPending <= m_uMemoryUsed );
        }
    }
    else
    {
        // Subtract from backing reads.
        VT_ASSERT( m_cBackingReads > 0 );
        m_cBackingReads--;

        // If read make it the most recently used block.
        VT_ASSERT( pBlock->pNext == NULL && pBlock->pPrev == NULL );

        if (hr == S_OK)
            MakeMRUBlock(pBlock);
        else
            MakeLRUBlock(pBlock);
    }

    // If no WAIT_IO_COMPLETION need to set the backing event instead.
    if (!SetEvent(m_hBackingEvent))
        hr = HRESULT_FROM_WIN32( GetLastError() );

    if (pBlock->pSource->hrBackingResult == S_OK)
        pBlock->pSource->hrBackingResult = hr;

    pBlock->pSource->lRef--;

    m_cs.Leave();
}

HRESULT
CImageCache::GetCallback(LINK* pBlock)
{
    HRESULT hr = S_OK;

    // if there is an unused callback available use it otherwise allocate one
    if (m_pCallbacks != NULL)
    {
        pBlock->pCallback = m_pCallbacks;
        m_pCallbacks = m_pCallbacks->m_pNext;
    }
    else
    {
        pBlock->pCallback = VT_NOTHROWNEW CICBlobCallback;
        VT_PTR_OOM_EXIT( pBlock->pCallback );
    }

    pBlock->pCallback->m_pLink = pBlock;

Exit:
    return hr;
}

HRESULT
CImageCache::BlobBlock(LINK* pBlock, bool bWrite)
{
    HRESULT hr = S_OK;

    VT_ASSERT( pBlock->pBlock != NULL && pBlock->pCallback != NULL );
    ANALYZE_ASSUME( pBlock->pCallback );

    VT_ASSERT( bWrite ?
        ((pBlock->pBlock->uTractNum & BLOCK_IN_STORE) == 0x0) :
        ((pBlock->pBlock->uTractNum & BLOCK_IN_STORE) != 0x0) );

    UINT uTractNum =
        (pBlock->pBlock->uTractNum & TRACT_MASK) *
        (pBlock->pSource->iAlignment / m_pBlobStore->GetTractSize());

    UINT uNumTracts = pBlock->cbData / m_pBlobStore->GetTractSize();

    // Own the source.
    pBlock->pSource->lRef++;

    m_cs.Leave();

    // Start asynchronous read/write.
    if (bWrite)
    {
        // Count pending backing blob writes.
        VT_ASSERT( m_cBackingWrites < MAXIMUM_WAIT_OBJECTS );
        m_cBackingWrites++;

        hr = m_pBlobStore->WriteBlob(pBlock->pSource->pBackingBlob,
                                     uTractNum, uNumTracts, pBlock->pbData,
                                     pBlock->pCallback);
    }
    else
    {
        // Count pending backing blob reads.
        VT_ASSERT( m_cBackingReads < MAXIMUM_WAIT_OBJECTS );
        m_cBackingReads++;

        hr = m_pBlobStore->ReadBlob(pBlock->pSource->pBackingBlob,
                                    uTractNum, uNumTracts, pBlock->pbData,
                                    pBlock->pCallback);
    }

    m_cs.Enter();

    VT_HR_EXIT( hr );

Exit:
    if (hr != S_OK && pBlock->pCallback != NULL)
    {
        // Clean up block's callback and put on tail of MRU list for eventual removal.
        HandleBlock(pBlock->pCallback, hr);
    }

    return hr;
}

HRESULT
CImageCache::ReadImg(UINT uSourceId, UINT uIndex, OUT CImg& dst, UINT uLevel)
{
    return DoRegion(uSourceId, uIndex, NULL, &dst, uLevel, false, false);
}

HRESULT
CImageCache::ReadRegion(UINT uSourceId, UINT uIndex, IN const CRect& region,
                        OUT CImg& dst, UINT uLevel)
{
    return DoRegion(uSourceId, uIndex, &region, &dst, uLevel, false, false);
}

HRESULT
CImageCache::Prefetch(UINT uSourceId, UINT uIndex, IN const CRect& region,
                      UINT uLevel)
{
    return DoRegion(uSourceId, uIndex, &region, NULL, uLevel, false, false);
}

HRESULT
CImageCache::DoRegion(UINT uSourceId, UINT uIndex, IN const CRect* region,
                      CImg* img, UINT uLevel, bool bWrite, bool bWriteThrough,
                      const void* pClearVal)
{
    HRESULT hr = S_OK;

    m_cs.Enter();

    CImgInfo info;
    CRect rect;

    SOURCE* source = NULL;

    if( uSourceId >= m_vecSources.size() ||
        m_vecSources[uSourceId] == NULL ||
        uIndex >= m_vecSources[uSourceId]->vecImages.size() )
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    source = m_vecSources[uSourceId];

    // Can't write if using reader.
    if (bWrite && (source->bReadOnly ||
        source->pImageReader != NULL || source->pIdxImageReader != NULL))
    {
        VT_HR_EXIT( E_ACCESSDENIED );
    }

    // Initialize level vector for this image.
    if (uLevel >= source->vecImages[uIndex].vecLevels.size())
    {
        VT_HR_EXIT( E_INVALIDARG );
    }
    info = source->vecImages[uIndex].vecLevels[uLevel].info;

    // Clip region to level.
    rect = region == NULL ? info.Rect(): (*region & info.Rect());

    UINT uClearVal = 0; // 0th entry is source's clear value; default 0
    if (bWrite)
    {
		// For write image size must match region size
		// (for read image will be re-created with region size).
		if ( img != NULL && 
			 (rect.Width() != img->Width() || rect.Height() != img->Height()) )
		{
			VT_HR_EXIT( E_INVALIDARG );
		}

        if (pClearVal != NULL)
        {
            // Find matching clear value.
            VT_HR_EXIT( AddClearValue(source->vecImages[uIndex], pClearVal, uClearVal) );
        }

		if( source->bAutoLevelGenerate )
		{
			// If the cache is auto-gen pyramid levels then clear affected 
			// region in lower levels.
			VT_HR_EXIT( ClearRegion(source, uIndex, rect, uLevel, uClearVal) );
		}
    }
    else
    {
        VT_ASSERT( pClearVal == NULL );

        // If prefetch and no backing blob and no reader, then there is nothing to do.
        if (img == NULL && source->pBackingBlob == NULL &&
            source->pImageReader == NULL && source->pIdxImageReader == NULL)
            goto Exit;
    }

    VT_HR_EXIT( DoRegion(source, uIndex, rect, img, uLevel, bWrite, bWriteThrough, uClearVal) );

Exit:
    m_cs.Leave();

    return hr;
}

HRESULT
CImageCache::DoRegion(SOURCE* source, UINT uIndex, IN const CRect& rect,
                      CImg* img, UINT uLevel, bool bWrite, bool bWriteThrough,
                      UINT uClearVal, bool bWait)
{
    HRESULT hr = S_OK;

    // Own the source.
    source->lRef++;

#define QUEUE_SIZE 32
#define QUEUE_MASK (QUEUE_SIZE - 1)
    // Initial queue is on stack (no malloc).
    CRect rctQueue[QUEUE_SIZE];
    CRect *rctPending = rctQueue;
    UINT cPending = 0;

    CRect rctBlk;

    IMAGE& image = source->vecImages[uIndex];
    LEVEL& level = image.vecLevels[uLevel];
    CImgInfo info = level.info;

    if (img != NULL && !bWrite )
    {
        // if the image is not pre-allocated to the correct size, then allocate it
        // here
        VT_HR_EXIT( CreateImageForTransform(*img, rect.Width(), rect.Height(),
                                            info.type) );
    }

    // Pad rect to block size.
    rctBlk.left = (rect.left / image.uBlockSizeX) * image.uBlockSizeX;
    rctBlk.top  = (rect.top  / image.uBlockSizeY) * image.uBlockSizeY;
    rctBlk.right  = VtMin(info.Rect().right,  (LONG)
        (((rect.right  + image.uBlockSizeX - 1) / image.uBlockSizeX) * image.uBlockSizeX));
    rctBlk.bottom = VtMin(info.Rect().bottom, (LONG)
        (((rect.bottom + image.uBlockSizeY - 1) / image.uBlockSizeY) * image.uBlockSizeY));

    // determine if level is in the cache.
    for (CBlockIterator bi(BLOCKITER_INIT(rctBlk, image.uBlockSizeX, image.uBlockSizeY));
        !bi.Done(); bi.Advance())
    {
        if ((hr = DoBlock(source, uIndex, rect, img, uLevel,
                          bWrite, bWriteThrough, uClearVal, bi.GetCompRect())) == S_FALSE && bWait)
        {
            // Grow queue by QUEUE_SIZE if necessary.
            if (cPending >= QUEUE_SIZE && (cPending & QUEUE_MASK) == 0)
            {
                CRect *rctNew = VT_NOTHROWNEW CRect[cPending + QUEUE_SIZE];
                VT_PTR_OOM_EXIT( rctNew );
                memcpy(rctNew, rctPending, cPending * sizeof(CRect));
                if (rctPending != rctQueue)
                    delete[] rctPending;
                rctPending = rctNew;
            }

            // Push block on queue.
            rctPending[cPending++] = bi.GetCompRect();
        }
        else if (hr != S_OK)
            VT_HR_EXIT( hr );
    }

    while (cPending > 0)
    {
        // Kick the backing store thread.
        if (!SetEvent(m_hBackingEvent))
            VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

        // Give other threads a chance to give up blocks.
        m_cs.Leave();
        CSystem::Sleep(1);
        m_cs.Enter();

        for (int iBlock = (int) cPending - 1; iBlock >= 0; iBlock--)
        {
            if ((hr = DoBlock(source, uIndex, rect, img, uLevel,
                              bWrite, bWriteThrough, uClearVal, rctPending[iBlock])) == S_OK)
            {
                // Erase block from queue.
                for (UINT i = iBlock; i < cPending - 1; i++)
                    rctPending[i] = rctPending[i + 1];
                cPending--;
            }
            else if (hr != S_FALSE)
                VT_HR_EXIT( hr );
        }
    }

Exit:
    if (rctPending != rctQueue)
        delete[] rctPending;

    if (--source->lRef == 0)
    {
        // Source was deleted while we were reading/writing region.
        delete source;

        hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT
CImageCache::DoBlock(SOURCE* source, UINT uIndex, IN const CRect& rect,
                     CImg* img, UINT uLevel, bool bWrite, bool bWriteThrough,
                     UINT uClearVal, IN const CRect& rctBlk)
{
    HRESULT hr = S_OK;

    // We enter in critical section.
    bool bCriticalSection = true;
    bool bOwned = false;

    DWORD dwThreadId = GetCurrentThreadId();

    IMAGE& image = source->vecImages[uIndex];
    LEVEL& level = image.vecLevels[uLevel];
    CImgInfo info = level.info;

    // Round up for align64ByteRows.
    UINT uStrideBytes = (rctBlk.Width() * info.PixSize() + 0x3f) & ~0x3f;

    UINT cBlocksX = (info.width  + image.uBlockSizeX - 1) / image.uBlockSizeX;
    // UINT cBlocksY = (info.height + source->uBlockSizeY - 1) / source->uBlockSizeY;
    UINT uBlockX = rctBlk.left / image.uBlockSizeX;
    UINT uBlockY = rctBlk.top  / image.uBlockSizeY;
    BLOCK& block = level.vecBlocks[uBlockY * cBlocksX + uBlockX];

    if (bWrite)
        block.uUniquenessValue++;

    info.width = rctBlk.Width();
    info.height = rctBlk.Height();

    // Share block of source image.
    CImg imgSrc, imgBlk;
    CRect rctSrc = (rect & rctBlk) - rect.TopLeft();
    if (img != NULL)
        VT_HR_EXIT( img->Share(imgSrc, &rctSrc) );

    // Update cache request stats.
    UINT uBytes = rctSrc.Width() * rctSrc.Height() * info.PixSize();
    if (bWrite)
        if (img != NULL)
            source->Stats.writes.request_bytes += uBytes;
        else
            source->Stats.clears.request_bytes += uBytes;
    else
        if (img != NULL)
            source->Stats.reads.request_bytes += uBytes;
        else
            source->Stats.prefetches.request_bytes += uBytes;

    // Check if write fully overlaps the block.
    BOOL bOverlap = (rect & rctBlk) == rctBlk;

    bool bNeedRead = false;

    const void* pBlockVal = NULL;

    // Determine if block is in the cache.
    if ((block.uTractNum & BLOCK_IN_CACHE) == 0x0)
    {
        // Current clear value for block.
        pBlockVal = block.pClearVal;

        // If write fully overlaps the block
        // then we don't need to read it into cache.
        bNeedRead = !bWrite || !bOverlap;

        // Is data is available from reader or higher levels?
        bool bCanRead = 
            source->pImageReader != NULL || source->pIdxImageReader != NULL ||
             (source->bAutoLevelGenerate && uLevel > 0);

        // Need space for image in cache if writing,
        // and not a completely overlapped clear,
        // or reading or prefetching from backing store and not write though,
        // or reading (not prefetching)
        // and data is available from reader or higher levels.
        if ((bWrite && !(img == NULL && bOverlap)) ||
           (!bWrite && ((((block.uTractNum & BLOCK_IN_STORE) == 0x0) == bWriteThrough) ||
            (img != NULL && bCanRead))))
        {
            // Round up to multiple of tract size.
            UINT uTractMask = source->iAlignment - 1;
            UINT cbData = (((rctBlk.Width() * info.PixSize() + 0x3f) & ~0x3f) *
                rctBlk.Height() + uTractMask) & ~uTractMask;

            // set up the data structs
            VT_HR_EXIT( AssignBlock(source, block, cbData) );
            ANALYZE_ASSUME( block.pLink != NULL );
            VT_ASSERT( block.pLink != NULL );

            // clear the least recently used stuff to make room for the new images
            VT_HR_EXIT( CheckForMemory() );

            if (bNeedRead)
            {
                // read from backing store if possible
                if ((block.uTractNum & BLOCK_IN_STORE) != 0x0)
                {
                    // Update cache miss stats.
                    if (bWrite)
                        if (img != NULL)
                            source->Stats.writes.blob_miss_bytes += block.pLink->cbData;
                        else
                            source->Stats.clears.blob_miss_bytes += block.pLink->cbData;
                    else
                        if (img != NULL)
                            source->Stats.reads.blob_miss_bytes += block.pLink->cbData;
                        else
                            source->Stats.prefetches.blob_miss_bytes += block.pLink->cbData;

                    // Load block from backing file.
                    VT_HR_EXIT( GetCallback(block.pLink) );

                    block.pLink->pNext = m_pHeadReads;
                    if (m_pHeadReads != NULL)
                        m_pHeadReads->pPrev = block.pLink;
                    else
                        m_pTailReads = block.pLink;
                    m_pHeadReads = block.pLink;

                    // Check the backing store thread.
                    VT_HR_EXIT( source->hrBackingResult );

                    // Kick the backing store thread.
                    if (!SetEvent(m_hBackingEvent))
                        VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

                    bNeedRead = false;
                }
                else
                {
                    // Don't own block if can't fill it.
                    VT_ASSERT( block.pLink->dwThreadId == dwThreadId );
                    if (!bCanRead && pBlockVal == NULL)
                        block.pLink->dwThreadId = 0;
                }
            }
        }
    }

    // Try to own this block unless just doing prefetch.
    if ((bWrite || bWriteThrough || img != NULL) &&
        (block.uTractNum & BLOCK_IN_CACHE) != 0x0)
    {
        if (block.pLink->dwThreadId != 0)
        {
            if (block.pLink->dwThreadId != dwThreadId)
            {
                // Someone else owns this block.  Give up and try again later.
                hr = S_FALSE;
                goto Exit;
            }
        }

        // Check the backing store thread.
        VT_HR_EXIT( source->hrBackingResult );

        // Check if pending read/write to/from backing store.
        if (block.pLink->pCallback != NULL)
        {
            if ((block.uTractNum & BLOCK_IN_STORE) == 0x0)
            {
                // If writing, wait for pending write since backing data now invalid.
                if (bWrite)
                {
                    hr = S_FALSE;
                    goto Exit;
                }
            }
            else
            {
                // If writing over whole block, cancel pending read since data not needed.
                if (bWrite && bOverlap)
                {
                    // Remove from read list.
                    CancelRead(block.pLink);
                }
            }
        }

        // Check if still pending read.
        if (block.pLink->pCallback != NULL &&
            (block.uTractNum & BLOCK_IN_STORE) != 0x0)
        {
            // Wait for read to complete.
            hr = S_FALSE;
            goto Exit;
        }
        else if (bWrite && !imgSrc.IsValid() && bOverlap)
        {
            // Remove block if completely overlapped clear.
            VT_ASSERT( block.pLink->dwThreadId == 0 );
            if (block.pLink->dwThreadCount != 0)
            {
                // Someone else owns this block.  Give up and try again later.
                hr = S_FALSE;
                goto Exit;
            }
            VT_HR_EXIT( RemoveBlock(block.pLink) );
        }
        else
        {
            // If not in read list can set place in MRU list.
            if (bWriteThrough)
                MakeLRUBlock(block.pLink);
            else
                MakeMRUBlock(block.pLink);

            VT_HR_EXIT( imgBlk.Create(block.pLink->pbData, rctBlk.Width(), rctBlk.Height(),
                                      uStrideBytes, info.type) );

            // Own this block.
            bOwned = true;
            block.pLink->dwThreadCount++;
            VT_ASSERT( block.pLink->dwThreadCount > 0 );

            if (bWrite)
                block.uTractNum |= BLOCK_IN_WRITE;
        }
    }

    if (bWrite)
    {
        if ((block.uTractNum & (BLOCK_IN_STORE | BLOCK_IN_CACHE)) ==
            (BLOCK_IN_STORE | BLOCK_IN_CACHE))
        {
            // If write subtract from pending memory.
            VT_ASSERT( m_uMemoryPending >= block.pLink->cbAlloc );
            m_uMemoryPending -= block.pLink->cbAlloc;
        }

        // Mark data as invalid but keep its place in backing file.
        block.uTractNum &= ~BLOCK_IN_STORE;

        if (!imgSrc.IsValid() && bOverlap)
        {
            VT_ASSERT( (block.uTractNum & BLOCK_IN_CACHE) == 0x0 );

            // Set clear value if completely overlapped clear.
            block.pClearVal = image.vecClearVals[uClearVal];
        }
    }

    // Now we own this block.
    bCriticalSection = false;
    m_cs.Leave();

    if (bNeedRead)
    {
        if (source->pImageReader != NULL)
        {
            VT_ASSERT( !bWrite );

            // Load block from image reader.
            if (!imgBlk.IsValid())
            {
                // Update cache miss stats.
                source->Stats.prefetches.reader_miss_bytes += block.pLink->cbData;
                VT_HR_EXIT( source->pImageReader->Prefetch(rctBlk, uLevel) );
            }
            else
            {
                // Update cache miss stats.
                source->Stats.reads.reader_miss_bytes += block.pLink->cbData;
                VT_HR_EXIT( source->pImageReader->ReadRegion(rctBlk, imgBlk, uLevel) );
            }
        }
        else if (source->pIdxImageReader != NULL)
        {
            VT_ASSERT( !bWrite );

            // Load block from indexed image reader.
            if (!imgBlk.IsValid())
            {
                // Update cache miss stats.
                source->Stats.prefetches.reader_miss_bytes += block.pLink->cbData;
                VT_HR_EXIT( source->pIdxImageReader->Prefetch(uIndex, rctBlk, uLevel) );
            }
            else
            {
                // Update cache miss stats.
                source->Stats.reads.reader_miss_bytes += block.pLink->cbData;
                VT_HR_EXIT( source->pIdxImageReader->ReadRegion(uIndex, rctBlk, imgBlk, uLevel) );
            }
        }
        else if (uLevel > 0 && source->bAutoLevelGenerate)
        {
            // Update cache miss stats.
            if (bWrite)
                if (img != NULL)
                    source->Stats.writes.autogen_miss_bytes += block.pLink->cbData;
                else
                    source->Stats.clears.autogen_miss_bytes += block.pLink->cbData;
            else
                if (img != NULL)
                    source->Stats.reads.autogen_miss_bytes += block.pLink->cbData;
                else
                    VT_ASSERT( source->Stats.prefetches.autogen_miss_bytes == 0 );

            // Generate pyramid to get block for this level.
            VT_HR_EXIT( GenerateRegion(source, uIndex, uLevel, rctBlk, imgBlk) );
        }
        else if (imgBlk.IsValid() && pBlockVal != NULL)
        {
            // Update cache miss stats.
            if (bWrite)
                if (img != NULL)
                    source->Stats.writes.clear_miss_bytes += block.pLink->cbData;
                else
                    source->Stats.clears.clear_miss_bytes += block.pLink->cbData;
            else
                if (img != NULL)
                    source->Stats.reads.clear_miss_bytes += block.pLink->cbData;
                else
                    source->Stats.prefetches.clear_miss_bytes += block.pLink->cbData;

            VT_HR_EXIT( imgBlk.Fill((const Byte *) pBlockVal) );
        }
        // else user is reading uninitialized data from level; do nothing
    }

    // If write or read (not prefetch) need to actually do something.
    if (bWrite || img != NULL)
    {
        // Share block of dest image.
        CRect rctDst = (rect & rctBlk) - rctBlk.TopLeft();
        CImg imgDst;
        if (imgBlk.IsValid())
            VT_HR_EXIT( imgBlk.Share(imgDst, rctDst) );

        // Copy or convert or clear block image.
        if (bWrite)
        {
			// On writes set the bCacheBypass flag for memset, fillspan, memcpy,
			// and ConvertImg. This copies the pixels straight to main memory
			// bypassing the CPU caches - assumption here is that after a write
			// this block is not likely to be needed again soon. 
            if (!imgSrc.IsValid())
            {
                // Clear the block's image with input clear value
                // unless completely overlapped (where it will be removed.)
                if (!bOverlap)
                {
                    const void* pClearVal = image.vecClearVals[uClearVal];
                    VT_HR_EXIT( imgDst.Fill((const Byte *) pClearVal) );
                }
            }
            else
                VT_HR_EXIT( VtConvertImage(imgDst, imgSrc, true) );
        }
        else if (imgSrc.IsValid())
        {
            if (!imgDst.IsValid())
            {
                // Clear the output image with block's clear value.
                if (pBlockVal != NULL)
                {
                    // May need to convert the clear value on output.
                    if (EL_B_FORMAT( imgSrc.GetType() ) != EL_B_FORMAT( info.type ))
                    {
                        CImg imgFill, imgBlock;
                        VT_HR_EXIT( imgBlock.Create((Byte *) pBlockVal, 1, 1, info.PixSize(), info.type) );
                        VT_HR_EXIT( imgFill.Create(1, 1, imgSrc.GetType()) );
                        VT_HR_EXIT( VtConvertImage(imgFill, imgBlock) );
                        VT_HR_EXIT( imgSrc.Fill(imgFill.BytePtr()) );
                    }
                    else
                        VT_HR_EXIT( imgSrc.Fill((const Byte*) pBlockVal) );
                }
            }
            else
                VT_HR_EXIT( VtConvertImage(imgSrc, imgDst) );
        }
    }

    m_cs.Enter();
    bCriticalSection = true;

    if ((block.uTractNum & BLOCK_IN_CACHE) != 0x0)
    {
        VT_ASSERT( block.pLink != NULL );
        ANALYZE_ASSUME( block.pLink != NULL );

        // Give up block if we owned it.
        if (block.pLink->dwThreadId == dwThreadId)
            block.pLink->dwThreadId = 0;
        if (bOwned)
        {
            VT_ASSERT( block.pLink->dwThreadCount > 0 );
            block.pLink->dwThreadCount--;

            if (block.pLink->dwThreadCount == 0)
                block.uTractNum &= ~BLOCK_IN_WRITE;
        }
    }

Exit:
    // We leave in critical section.
    if (!bCriticalSection)
    {
        // Should only be here if error outside critical section.
        VT_ASSERT( hr != S_OK );

        m_cs.Enter();

        if ((block.uTractNum & BLOCK_IN_CACHE) != 0x0 && hr != S_FALSE)
        {
            // Give up block if we owned it.
            if (block.pLink->dwThreadId == dwThreadId)
                block.pLink->dwThreadId = 0;
            if (bOwned)
            {
                VT_ASSERT( block.pLink->dwThreadCount > 0 );
                block.pLink->dwThreadCount--;

                if (block.pLink->dwThreadCount == 0)
                    block.uTractNum &= ~BLOCK_IN_WRITE;
            }
        }
    }

    return hr;
}


HRESULT
CImageCache::SetMetaData(UINT uSourceId, UINT uIndex, const CParams& params,
						 bool bMerge)
{
    HRESULT hr = S_OK;

    m_cs.Enter();

    if( uSourceId >= m_vecSources.size() ||
        m_vecSources[uSourceId] == NULL ||
        uIndex >= m_vecSources[uSourceId]->vecImages.size() )
    {
        VT_HR_EXIT( E_INVALIDARG );
    }

    SOURCE* source = m_vecSources[uSourceId];

    // Can't set metadata if using reader.
    if (source->pImageReader != NULL || source->pIdxImageReader != NULL)
    {
        VT_HR_EXIT( E_ACCESSDENIED );
    }

    IMAGE& image = source->vecImages[uIndex];

    if (image.params == NULL)
    {
        image.params = VT_NOTHROWNEW CParams;
        VT_PTR_OOM_EXIT( image.params );
        *image.params = params;
    }
    else if (!bMerge)
        *image.params = params;
    else
        VT_HR_EXIT( image.params->Merge(&params) );

Exit:
    m_cs.Leave();

    return hr;
}


HRESULT
CImageCache::WriteImg(UINT uSourceId, UINT uIndex, IN const CImg& src,
                      UINT uLevel, bool bWriteThrough)
{
    return DoRegion(uSourceId, uIndex, NULL, (CImg*) &src, uLevel, true, bWriteThrough);
}

HRESULT
CImageCache::WriteRegion(UINT uSourceId, UINT uIndex, IN const CRect& region,
                         IN const CImg& src, UINT uLevel, bool bWriteThrough)
{
    return DoRegion(uSourceId, uIndex, &region, (CImg*) &src, uLevel, true, bWriteThrough);
}


HRESULT
CImageCache::ClearImg(UINT uSourceId, UINT uIndex, UINT uLevel, IN const void* pClearVal)
{
    return DoRegion(uSourceId, uIndex, NULL, NULL, uLevel, true, false, pClearVal);
}

HRESULT
CImageCache::ClearRegion(UINT uSourceId, UINT uIndex, IN const CRect& region,
                         UINT uLevel, IN const void* pClearVal)
{
    return DoRegion(uSourceId, uIndex, &region, NULL, uLevel, true, false, pClearVal);
}


HRESULT
CImageCache::GenerateLevels(UINT uSourceId, UINT uIndex, UINT uEndLev,
                            IN const CRect* pRgnInEndLev)
{
    return DoRegion(uSourceId, uIndex, pRgnInEndLev,
                    NULL, uEndLev, false, false, NULL);
}


HRESULT
CImageCache::GenerateRegion(SOURCE* source, UINT uIndex, UINT uLevel,
                            IN const CRect& region, OUT CImg& imgDst)
{
    HRESULT hr = S_OK;

    VT_ASSERT( source != NULL &&
               uIndex < source->vecImages.size() &&
               uLevel > 0 && uLevel <= source->vecImages[uIndex].vecLevels.size() );

    IMAGE& image = source->vecImages[uIndex];

    CLayerImgInfo info = image.vecLevels[uLevel - 1].info;

    CImg imgSrc;

    // Calculate size of source region needed.
    int iX, iWidth, iY, iHeight;
    C1dKernelSet* ksx = (info.origin.x & 1) == 0 ? &source->ks : &source->ks1;
    C1dKernelSet* ksy = (info.origin.y & 1) == 0 ? &source->ks : &source->ks1;
    ksx->GetSourceRegion(info.width, region.left, region.Width(),
                         iX, iWidth);
    ksy->GetSourceRegion(info.height, region.top, region.Height(),
                         iY, iHeight);

    // Load source region necessary for filter
    // (may call GenerateBlock() recursively).
    m_cs.Enter();

    hr = DoRegion(source, uIndex,
                  CRect(iX, iY, iX + iWidth, iY + iHeight),
                  !imgDst.IsValid() ? NULL : &imgSrc, uLevel - 1, false,
                  !imgDst.IsValid());

    m_cs.Leave();

    VT_HR_EXIT( hr );

    if (imgDst.IsValid())
    {
        // Filter source region into destinatation.
	    VT_HR_EXIT( VtSeparableFilter(imgDst, region, imgSrc, CPoint(iX, iY),
                                      *ksx, *ksy) );

        if (EL_FORMAT(imgSrc.GetType()) == EL_FORMAT_HALF_FLOAT ||
            EL_FORMAT(imgSrc.GetType()) == EL_FORMAT_FLOAT)
        {
    	    // suppress large ringing in float format
            VtSuppressPyramidRinging(imgDst, region.TopLeft(),
                                     imgSrc, vt::CPoint(iX, iY));
        }
    }

Exit:
    return hr;
}


HRESULT
CImageCache::ClearRegion(SOURCE* source, UINT uIndex,
                         IN const CRect& region, UINT uLevel,
                         UINT uClearVal)
{
    HRESULT hr = S_OK;

    VT_ASSERT( source != NULL &&
               uIndex < source->vecImages.size() );

    IMAGE& image = source->vecImages[uIndex];

    CRect rect = region;

    // Clear affected blocks in levels below passed in region/level.
    for (uLevel = uLevel + 1; uLevel < (UINT)image.vecLevels.size(); uLevel++)
    {
        LEVEL& level = image.vecLevels[uLevel];

        if (level.vecBlocks.empty())
            continue;

        CLayerImgInfo info = level.info;

        // Calculate width and height of affected rectangle on this level
        // given rectangle on previous level.
        int iX, iWidth, iY, iHeight;
        C1dKernelSet* ksx = (info.origin.x & 1) == 0 ? &source->ks : &source->ks1;
        C1dKernelSet* ksy = (info.origin.y & 1) == 0 ? &source->ks : &source->ks1;
        ksx->GetDestinationRegion(rect.left, rect.Width(), iX, iWidth);
        ksy->GetDestinationRegion(rect.top, rect.Height(), iY, iHeight);

        // Rectangle of blocks to remove for this level.
        rect = CRect(CPoint(iX, iY), CSize(iWidth, iHeight));

        // Pad rect to block size.
        rect.left = (rect.left / image.uBlockSizeX) * image.uBlockSizeX;
        rect.top  = (rect.top  / image.uBlockSizeY) * image.uBlockSizeY;
        rect.right  = VtMin(info.Rect().right,  (LONG)
            (((rect.right  + image.uBlockSizeX - 1) / image.uBlockSizeX) * image.uBlockSizeX));
        rect.bottom = VtMin(info.Rect().bottom, (LONG)
            (((rect.bottom + image.uBlockSizeY - 1) / image.uBlockSizeY) * image.uBlockSizeY));

        // Clear blocks.
        VT_HR_EXIT( DoRegion(source, uIndex, rect, NULL, uLevel, true, false, uClearVal) );
    }

Exit:
    return hr;
}


HRESULT
CImageCache::CheckForMemory(bool bFlush)
{
    HRESULT hr = S_OK;

    LINK* pBlock = m_pTailBlocks;
    bool bRef = true;

    if (m_uMemoryUsed - m_uMemoryPending > m_uHighWater)
    {
        // Kick the backing store thread.
        if (!SetEvent(m_hBackingEvent))
            VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );
    }

    // Traverse whole list for flush.
    while (pBlock != NULL && (bFlush ||
        m_uMemoryUsed > m_uMaxCacheSystemMemory))
    {
        LINK* pPrevBlock = pBlock->pPrev;

        // check if there is only one reference to block
        if (pBlock->dwThreadId == 0 && pBlock->dwThreadCount == 0)
        {
            bRef = false;

            // If no pending backing file read or write can remove block.
            if (pBlock->pCallback == NULL)
            {
                if ((pBlock->pBlock->uTractNum & BLOCK_IN_STORE) != 0x0 ||
                    !pBlock->pSource->bUseBackingStore ||
                    pBlock->pSource->hrBackingResult != S_OK)
                {
                    VT_HR_EXIT( RemoveBlock(pBlock) );
                }
            }
        }

        pBlock = pPrevBlock;

        // If still too much memory and still unowned blocks try again.
        if (pBlock == NULL && !bRef && (bFlush ||
            m_uMemoryUsed > m_uMaxCacheSystemMemory))
        {
            // Kick the backing store thread.
            if (!SetEvent(m_hBackingEvent))
                VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

            // Give other threads a chance to give up blocks.
            m_cs.Leave();
            CSystem::Sleep(1);
            m_cs.Enter();

            pBlock = m_pTailBlocks;
            bRef = true;
        }
    }

Exit:
    return hr;
}

void
CImageCache::MakeMRUBlock(LINK* pBlock)
{
    if (pBlock != m_pHeadBlocks)
    {
        if (pBlock == m_pTailBlocks)
            m_pTailBlocks = pBlock->pPrev;
        if (m_pTailBlocks == NULL)
            m_pTailBlocks = pBlock;

        if (pBlock->pNext != NULL)
            pBlock->pNext->pPrev = pBlock->pPrev;
        if (pBlock->pPrev != NULL)
            pBlock->pPrev->pNext = pBlock->pNext;  

        pBlock->pPrev = NULL;
        pBlock->pNext = m_pHeadBlocks;

        if (m_pHeadBlocks != NULL)
            m_pHeadBlocks->pPrev = pBlock;
        m_pHeadBlocks = pBlock;
    }
}

void
CImageCache::MakeLRUBlock(LINK* pBlock)
{
    if (pBlock != m_pTailBlocks)
    {
        if (pBlock == m_pHeadBlocks)
            m_pHeadBlocks = pBlock->pNext;
        if (m_pHeadBlocks == NULL)
            m_pHeadBlocks = pBlock;

        if (pBlock->pPrev != NULL)
            pBlock->pPrev->pNext = pBlock->pNext;
        if (pBlock->pNext != NULL)
            pBlock->pNext->pPrev = pBlock->pPrev;  

        pBlock->pNext = NULL;
        pBlock->pPrev = m_pTailBlocks;

        if (m_pTailBlocks != NULL)
            m_pTailBlocks->pNext = pBlock;
        m_pTailBlocks = pBlock;
    }
}

HRESULT
CImageCache::RemoveBlock(LINK* pBlock)
{
    HRESULT hr = S_OK;

    VT_ASSERT( pBlock->pBlock != NULL && pBlock->pCallback == NULL && pBlock->pSource != NULL &&
               pBlock->dwThreadId == 0 && pBlock->dwThreadCount == 0);

    pBlock->pSource = NULL;

    // remove this block from its list (if in one)
    if (pBlock == m_pHeadBlocks)
        m_pHeadBlocks = pBlock->pNext;
    else if (pBlock->pPrev != NULL)
        pBlock->pPrev->pNext = pBlock->pNext;
    if (pBlock == m_pTailBlocks)
        m_pTailBlocks = pBlock->pPrev;
    else if (pBlock->pNext != NULL)
        pBlock->pNext->pPrev = pBlock->pPrev;

    // add to the empty blocks
    pBlock->pPrev = NULL;
    pBlock->pNext = m_pEmptyBlocks;
    m_pEmptyBlocks = pBlock;

    // Save image if larger (i.e. more useful) than current empty image.
    if (pBlock->cbAlloc > m_cbEmptyAlloc)
    {
        if (m_pbEmptyAlloc != NULL)
        {
            if (!HeapFree(m_hHeap, 0, m_pbEmptyAlloc))
                VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError()) );
        }

        m_pbEmptyAlloc = pBlock->pbAlloc;
        m_cbEmptyAlloc = pBlock->cbAlloc;
    }
    else
    {
        if (!HeapFree(m_hHeap, 0, pBlock->pbAlloc))
            VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError()) );
    }
        
    pBlock->pbAlloc = NULL;
    pBlock->pbData = NULL;

    // release its memory and subtract from total mem used.
    VT_ASSERT( m_uMemoryPending <= m_uMemoryUsed );
    VT_ASSERT( m_uMemoryUsed >= pBlock->cbAlloc );
    m_uMemoryUsed -= pBlock->cbAlloc;
    if ((pBlock->pBlock->uTractNum & BLOCK_IN_STORE) != 0x0)
    {
        VT_ASSERT( m_uMemoryPending >= pBlock->cbAlloc );
        m_uMemoryPending -= pBlock->cbAlloc;
    }
    pBlock->cbAlloc = 0;
    pBlock->cbData = 0;

    pBlock->pBlock->uTractNum &= ~BLOCK_IN_CACHE;
    pBlock->pBlock->pLink = NULL;
    pBlock->pBlock = NULL;

Exit:
    return hr;
}

HRESULT
CImageCache::ComputeSourceSize(SOURCE *source, bool bInBlob)
{
    HRESULT hr = S_OK;

    VT_PTR_EXIT( source );

    // Compute header size.
    UINT uHeadSize = sizeof(VTI_HEADER) +
        (UINT) source->vecImages.size() * sizeof (VTI_INFO);
    source->uNumTracts = (uHeadSize + source->iAlignment - 1) / source->iAlignment;

    // Compute size of source.
    for (UINT uIndex = 0; uIndex < static_cast<UINT>(source->vecImages.size()); uIndex++)
    {
        IMAGE& image = source->vecImages[uIndex];

        // Initialize level vector for this image.
        CLayerImgInfo info = image.vecLevels[0].info;
        int iLevels = 0;
        if (source->pImageReader != NULL)
        {
            while (source->pImageReader->GetImgInfo(iLevels).width > 0 &&
                source->pImageReader->GetImgInfo(iLevels).height > 0)
                iLevels++;
        }
        else if (source->pIdxImageReader != NULL)
        {
            while (source->pIdxImageReader->GetImgInfo(uIndex, iLevels).width > 0 &&
                source->pIdxImageReader->GetImgInfo(uIndex, iLevels).height > 0)
                iLevels++;
        }
        else if (source->pBackingReaderWriter != NULL)
            iLevels = ((CVtiReaderWriter *) source->pBackingReaderWriter)->GetLevelCount(uIndex);
        else
            iLevels = ComputePyramidLevelCount(info.width, info.height);

        if (source->iMaxLevel < 0)
            source->iMaxLevel = VtMax(0, iLevels - 1);
        VT_HR_EXIT( image.vecLevels.resize(source->iMaxLevel + 1) );

        for (UINT uLevel = 0; uLevel < static_cast<UINT>(image.vecLevels.size()); uLevel++)
        {
            LEVEL& level = image.vecLevels[uLevel];

            // Generate intermediate level infos.
            if (source->pImageReader != NULL)
                level.info = source->pImageReader->GetImgInfo(uLevel);
            else if (source->pIdxImageReader != NULL)
                level.info = source->pIdxImageReader->GetImgInfo(uIndex, uLevel);
            else if (source->pBackingReaderWriter != NULL)
                level.info = source->pBackingReaderWriter->GetImgInfo(uIndex, uLevel);
            else
                level.info = InfoAtLevel(info, uLevel);

            // Initialize block vector for this level.
            UINT cBlocksX = (level.info.width  + image.uBlockSizeX - 1) / image.uBlockSizeX;
            UINT cBlocksY = (level.info.height + image.uBlockSizeY - 1) / image.uBlockSizeY;
            VT_HR_EXIT( level.vecBlocks.resize(cBlocksX * cBlocksY) );

            for (UINT uBlockY = 0; uBlockY < cBlocksY; uBlockY++)
            {
                int iHeight = VtMin(image.uBlockSizeY, level.info.height - uBlockY * image.uBlockSizeY);

                for (UINT uBlockX = 0; uBlockX < cBlocksX; uBlockX++)
                {
                    int iWidth = VtMin(image.uBlockSizeX, level.info.width - uBlockX * image.uBlockSizeX);

                    // Round up to multiple of tract size.
                    UINT uTractMask = source->iAlignment - 1;
                    UINT cbData = (((iWidth * level.info.PixSize() + 0x3f) & ~0x3f) *
                        iHeight + uTractMask) & ~uTractMask;
                    VT_ASSERT( cbData % source->iAlignment == 0 );
                    UINT uTracts = cbData / source->iAlignment;

                    BLOCK& block = level.vecBlocks[uBlockY * cBlocksX + uBlockX];
                    // If not named blob is not allocated all at once--don't assign block offsets.
                    block.uTractNum = source->wstrBackingBlob.empty() ? TRACT_MASK :
                        (source->uNumTracts | (bInBlob ? BLOCK_IN_STORE : 0x0));
                    source->uNumTracts += uTracts;
                }
            }
        }
    }

Exit:
    return hr;
}

HRESULT
CImageCache::CreateBlob(SOURCE* source)
{
    HRESULT hr = S_OK;

    CVtiReaderWriter *pBackingReaderWriter = NULL;
    vector<CLayerImgInfo> vecInfos;

    VT_PTR_EXIT( source );

    VT_ASSERT( !source->wstrBackingBlob.empty() );

    for (size_t i = 0; i < source->vecImages.size(); i++)
        VT_HR_EXIT( vecInfos.push_back(source->vecImages[i].vecLevels[0].info) );

    pBackingReaderWriter = VT_NOTHROWNEW CVtiReaderWriter;
    VT_PTR_OOM_EXIT( pBackingReaderWriter );

    // Try to create a blob.
    VT_HR_EXIT( pBackingReaderWriter->CreateBlob(vecInfos, source->iMaxLevel,
        source->wstrBackingBlob.get_constbuffer(), m_pBlobStore, source->iAlignment) );

    source->pBackingReaderWriter = pBackingReaderWriter;
    source->pBackingBlob = pBackingReaderWriter->GetBlob();

Exit:
    if (hr != S_OK)
        delete pBackingReaderWriter;

    return hr;
}

HRESULT
CImageCache::BackupBlock(LINK* pBlock)
{
    HRESULT hr = S_OK;

    VT_ASSERT( (pBlock->pBlock->uTractNum & BLOCK_IN_CACHE) != 0x0 &&
               (pBlock->pBlock->uTractNum & BLOCK_IN_STORE) == 0x0 );

    VT_ASSERT( pBlock->pSource != NULL );

    SOURCE* source = pBlock->pSource;

    VT_HR_EXIT( GetCallback(pBlock) );

    // Check if block has not previously been written to backing blob.
    if ((pBlock->pBlock->uTractNum & TRACT_MASK) == TRACT_MASK)
    {
        UINT uBlockSize = pBlock->cbData / m_pBlobStore->GetTractSize();
        UINT uTractNum = source->uTractNum *
            (pBlock->pSource->iAlignment / m_pBlobStore->GetTractSize());
        UINT uNumTracts = source->uNumTracts *
            (pBlock->pSource->iAlignment / m_pBlobStore->GetTractSize());

        // Check if source has backing blob or needs increment.
        if (source->pBackingBlob == NULL || uTractNum + uBlockSize >
            m_pBlobStore->GetBlobSize(source->pBackingBlob))
        {
            // Own the source.
            source->lRef++;

            m_cs.Leave();

            // Should only create on demand for un-named blobs.
            VT_ASSERT( source->wstrBackingBlob.empty() );

            if (source->pBackingBlob == NULL)
            {
                // Try to create a blob that can grow up to the source size.
                hr = m_pBlobStore->CreateBlob(source->pBackingBlob, NULL, uNumTracts);
            }

            if (hr == S_OK)
            {
                // Try to extend blob by 1 GB or up to the source size, whichever is less.
                hr = m_pBlobStore->ExtendBlob(source->pBackingBlob,
                    VtMin(BLOB_SIZE_INCR / m_pBlobStore->GetTractSize(), uNumTracts -
                          m_pBlobStore->GetBlobSize(source->pBackingBlob)));
            }

            m_cs.Enter();

            source->lRef--;

            VT_HR_EXIT( hr );
        }
       
        pBlock->pBlock->uTractNum = BLOCK_IN_CACHE | source->uTractNum;
        source->uTractNum += pBlock->cbData / source->iAlignment;

        // Update cache flush stats.
        source->Stats.flushed_bytes += pBlock->cbData;
    }
    else
    {
        // Update cache flush stats.
        source->Stats.reflush_bytes += pBlock->cbData;
    }

    VT_HR_EXIT( BlobBlock(pBlock, true) );

Exit:
    return hr;
}

HRESULT
CImageCache::AssignBlock(SOURCE* source, BLOCK& block, UINT cbData)
{
    HRESULT hr = S_OK;

    VT_ASSERT( source != NULL &&
               (block.uTractNum & BLOCK_IN_CACHE) == 0x0 );

    // if there is an unused image available use it otherwise allocate one
    Byte *pbAlloc = NULL;
    UINT cbAlloc = 0;
    if (cbData <= m_cbEmptyAlloc)
    {
        pbAlloc = m_pbEmptyAlloc;
        cbAlloc = m_cbEmptyAlloc;
        m_pbEmptyAlloc = NULL;
        m_cbEmptyAlloc = 0;
    }
    else
    {
        // Add page size for start alignment.
        cbAlloc = cbData;
        pbAlloc = (Byte *) HeapAlloc(m_hHeap, 0, cbAlloc + m_uPageSize);
        VT_PTR_OOM_EXIT( pbAlloc );
    }

    // Round up to multiple of page size.
    UINT_PTR uPageMask = m_uPageSize - 1;
    Byte *pbData = (Byte *) (((UINT_PTR) pbAlloc +
        uPageMask) & ~uPageMask);

    // if there is an unused MRU link available use it otherwise allocate one
    LINK* pBlock = NULL;
    if (m_pEmptyBlocks != NULL)
    {
        pBlock = m_pEmptyBlocks;
        m_pEmptyBlocks = m_pEmptyBlocks->pNext;
    }
    else
    {
        pBlock = VT_NOTHROWNEW LINK();
        if (pBlock == NULL)
        {
            if (!HeapFree(m_hHeap, 0, pbAlloc))
                VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );
        }
        VT_PTR_OOM_EXIT( pBlock );
    }

    pBlock->pSource = source;
    pBlock->pbAlloc = pbAlloc;
    pBlock->pbData  = pbData;
    pBlock->cbAlloc = cbAlloc;
    pBlock->cbData  = cbData;

    // insert this at the head and add the memory being used
    pBlock->pNext = pBlock->pPrev = NULL;

    pBlock->dwThreadId = GetCurrentThreadId();
    pBlock->dwThreadCount = 0;

    VT_ASSERT( m_uMemoryPending <= m_uMemoryUsed );
    m_uMemoryUsed += cbAlloc;
    if ((block.uTractNum & BLOCK_IN_STORE) != 0x0)
        m_uMemoryPending += cbAlloc;

    pBlock->pBlock = &block;
    block.pLink = pBlock;

    block.uTractNum |= BLOCK_IN_CACHE;

Exit:
    return hr;
}

