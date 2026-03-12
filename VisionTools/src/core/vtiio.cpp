//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Classes to access portions of blob (.vti file)
//
//  History:
//      2007/06/1-mattu
//          Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_image.h"
#include "vt_kernel.h"
#include "vt_convert.h"
#include "vt_layer.h"
#include "vt_fileblobstore.h"
#include "vt_pixeliterators.h"
#include "vt_separablefilter.h"
#include "vt_taskmanager.h"
#include "vt_transform.h"
#include "vt_pyramid.h"
#include "vt_utils.h"
#include "vtiio.h"

using namespace vt;

//+-----------------------------------------------------------------------------
//
// Class: VtiReaderWriter
// 
// Synopsis: an IIndexedImageReaderWriter that wraps a blob (.vti file)
//
//------------------------------------------------------------------------------
class CVtiBlobCallback : public IBlobCallback 
{
public:
    virtual void Callback(HRESULT hr);

    // if in use, pointer to the vti reader writer
    // otherwise, pointer to the next callback in empty list
    union 
    {
        CVtiReaderWriter *m_pThis;
        CVtiBlobCallback *m_pNext;
    };

    UINT *m_pPending;

    HANDLE m_hEvent;

    IImageWriter *m_pDst;

    CRect m_rct;
    CImg  m_img;

    UINT  m_cbAlloc;
    Byte *m_pbAlloc;

    CVtiBlobCallback() : m_pThis(NULL), m_pDst(NULL),
                         m_pPending(NULL), m_hEvent(NULL),
                         m_cbAlloc(0), m_pbAlloc(NULL) {};

    ~CVtiBlobCallback()
    {
        delete[] m_pbAlloc;
    }
};

void
CVtiBlobCallback::Callback(HRESULT /*hr*/)
{
    // Write desination block if read.
    if (m_pDst != NULL)
        m_pDst->WriteRegion(m_rct, m_img);

    (*m_pPending)--;

    SetEvent(m_hEvent);

    m_pThis->BlobIOCompletionRoutine(this);
}


HRESULT
CVtiReaderWriter::ComputeSourceSize()
{
    VT_HR_BEGIN()

    VT_HR_EXIT( m_vecBlocks.resize(m_vecInfos.size()) );

    // Compute size of source.
    for (UINT uIndex = 0; uIndex < static_cast<UINT>(m_vecInfos.size()); uIndex++)
    {
        UINT uBlockSizeX = m_vecInfos[uIndex].uBlockSizeX;
        UINT uBlockSizeY = m_vecInfos[uIndex].uBlockSizeY;

        if (uBlockSizeX == 0 || uBlockSizeY == 0 ||
            m_vecInfos[uIndex].uWidth == 0 || m_vecInfos[uIndex].uHeight == 0 ||
            m_vecInfos[uIndex].uNumLevels == 0 || m_vecInfos[uIndex].uType == OBJ_UNDEFINED)
            VT_HR_EXIT( E_INVALIDARG );

        // Initialize level vector for this image.
        CLayerImgInfo info = GetImgInfo(uIndex);
        VT_HR_EXIT( m_vecBlocks[uIndex].resize(m_vecInfos[uIndex].uNumLevels) );

        for (UINT uLevel = 0; uLevel < (UINT)m_vecBlocks[uIndex].size(); uLevel++)
        {
            info = GetImgInfo(uIndex, uLevel);

            // Initialize block vector for this level.
            UINT cBlocksX = (info.width  + uBlockSizeX - 1) / uBlockSizeX;
            UINT cBlocksY = (info.height + uBlockSizeY - 1) / uBlockSizeY;
            VT_HR_EXIT( m_vecBlocks[uIndex][uLevel].resize(cBlocksX * cBlocksY) );

            for (UINT uBlockY = 0; uBlockY < cBlocksY; uBlockY++)
            {
                int iHeight = VtMin(uBlockSizeY, info.height - uBlockY * uBlockSizeY);

                for (UINT uBlockX = 0; uBlockX < cBlocksX; uBlockX++)
                {
                    int iWidth = VtMin(uBlockSizeX, info.width - uBlockX * uBlockSizeX);

                    // Round up to multiple of tract size.
                    UINT uTractMask = m_uTractSize - 1;
                    UINT cbData = (((iWidth * info.PixSize() + 0x3f) & ~0x3f) *
                        iHeight + uTractMask) & ~uTractMask;
                    VT_ASSERT( cbData % m_uTractSize == 0 );
                    UINT uTracts = cbData / m_uTractSize;

                    m_vecBlocks[uIndex][uLevel][uBlockY * cBlocksX + uBlockX] = m_uNumTracts;
                    m_uNumTracts += uTracts;
                }
            }
        }
    }

    VT_HR_END()
}


CVtiReaderWriter::CVtiReaderWriter() : m_pBlobStore(NULL), m_pBlob(NULL),
                                       m_uTractSize(0), m_uNumTracts(0),
                                       m_pCallbacks(NULL)
{
    // From MSDN "File Buffering":
    // Therefore, in most situations, page-aligned memory will also be sector-aligned,
    // because the case where the sector size is larger than the page size is rare.
    SYSTEM_INFO si;
    vt::CSystem::GetSystemInfo(&si);
    m_uPageSize = si.dwPageSize;

    m_pFileStore = VT_NOTHROWNEW CFileBlobStore();
}

CVtiReaderWriter::~CVtiReaderWriter()
{
    if (m_pBlob != NULL)
        CloseBlob();

    delete m_pFileStore;

    while (m_pCallbacks != NULL)
    {
        CVtiBlobCallback* pCallback = (CVtiBlobCallback *) m_pCallbacks;
        m_pCallbacks = pCallback->m_pNext;

        delete pCallback;
    }
}


CLayerImgInfo
CVtiReaderWriter::GetImgInfo(UINT uIndex, UINT uLevel)
{
    if (uIndex < m_vecInfos.size() && uLevel < m_vecInfos[uIndex].uNumLevels)
    {
        VTI_INFO info = m_vecInfos[uIndex];

        CLayerImgInfo src(CImgInfo(info.uWidth, info.uHeight, info.uType),
						  info.uCompositeWidth, info.uCompositeHeight, 
						  CPoint(info.uOriginX, info.uOriginY),
						  (info.uFlags & COMPOSITE_WRAP_X) != 0x0, 
						  (info.uFlags & COMPOSITE_WRAP_Y) != 0x0);

        // Preserve layer info.
        return InfoAtLevel(src, uLevel);
    }
    else
        return CLayerImgInfo();
}


HRESULT
CVtiReaderWriter::ReadRegion(UINT uIndex, IN const CRect& region, OUT CImg& dst,
                             UINT uLevel)
{
    VT_HR_BEGIN()

    if (uIndex >= m_vecInfos.size() || uLevel >= m_vecInfos[uIndex].uNumLevels)
        VT_HR_EXIT( E_INVALIDARG );

    CLayerImgInfo info = GetImgInfo(uIndex, uLevel);

    // Clip region to level.
    CRect rect = region & info.Rect();

    // if the image is not pre-allocated to the correct size, then allocate it
    // here
    VT_HR_EXIT( CreateImageForTransform(dst, rect.Width(), rect.Height(),
                                        info.type) );

	CImgReaderWriter<CImg> img;
    VT_HR_EXIT( dst.Share(img) );

    VT_HR_EXIT( ReadRegion(uIndex, rect, &img, uLevel) );

    VT_HR_END()
}

HRESULT
CVtiReaderWriter::ReadRegion(UINT uIndex, IN const CRect& region, OUT IImageWriter* dst,
                             UINT uLevel)
{
    HANDLE hReadEvent = NULL;

    VT_HR_BEGIN()

    if (uIndex >= m_vecInfos.size() || uLevel >= m_vecInfos[uIndex].uNumLevels)
        VT_HR_EXIT( E_INVALIDARG );

    VT_PTR_EXIT( dst );

    VT_PTR_EXIT( m_pBlob );

    if (m_vecBlocks.empty())
        VT_HR_EXIT( E_NOINIT );

    CLayerImgInfo info = GetImgInfo(uIndex, uLevel);
    UINT uBlockSizeX = m_vecInfos[uIndex].uBlockSizeX;
    UINT uBlockSizeY = m_vecInfos[uIndex].uBlockSizeY;

    // Pad rect to block size.
    CRect rctPad;
    rctPad.left = (region.left / uBlockSizeX) * uBlockSizeX;
    rctPad.top  = (region.top  / uBlockSizeY) * uBlockSizeY;
    rctPad.right  = VtMin(info.Rect().right,  (LONG)
        (((region.right  + uBlockSizeX - 1) / uBlockSizeX) * uBlockSizeX));
    rctPad.bottom = VtMin(info.Rect().bottom, (LONG)
        (((region.bottom + uBlockSizeY - 1) / uBlockSizeY) * uBlockSizeY));

    UINT cBlocksX = (info.width  + uBlockSizeX - 1) / uBlockSizeX;

    UINT uPendingReads = 0;
    hReadEvent = CSystem::CreateEvent(FALSE, FALSE);
    VT_PTR_OOM_EXIT( hReadEvent );

    // Read blocks from the blob.
    for (CBlockIterator bi(BLOCKITER_INIT(rctPad, uBlockSizeX, uBlockSizeY));
        !bi.Done(); bi.Advance())
    {
        CRect rctBlk = bi.GetCompRect();

        // Round up to multiple of tract size.
        UINT uTractMask = m_uTractSize - 1;
        UINT uStrideBytes = (rctBlk.Width() * info.PixSize() + 0x3f) & ~0x3f;
        UINT cbData = (uStrideBytes * rctBlk.Height() + uTractMask) & ~uTractMask;
        VT_ASSERT( cbData % m_uTractSize == 0 );
        UINT uTracts = cbData / m_uTractSize;

        // Get callback.
        CVtiBlobCallback *pCallback;
        VT_HR_EXIT( GetCallback((LPVOID *) &pCallback, cbData) );
        pCallback->m_pDst = dst;
        pCallback->m_pPending = &uPendingReads;
        pCallback->m_hEvent = hReadEvent;

        // Round up to multiple of page size.
        UINT_PTR uPageMask = m_uPageSize - 1;
        Byte *pbData = (Byte *) (((UINT_PTR) pCallback->m_pbAlloc + uPageMask) & ~uPageMask);

        CImg imgBlk;
        VT_HR_EXIT( imgBlk.Create(pbData, rctBlk.Width(), rctBlk.Height(), uStrideBytes,
                                  info.type) );

        // Share block of destination image.
        pCallback->m_rct = (region & rctBlk) - region.TopLeft();

        // Share block of source image.
        CRect rctSrc = (region & rctBlk) - rctBlk.TopLeft();
        VT_HR_EXIT( imgBlk.Share(pCallback->m_img, &rctSrc) );

        UINT uBlockX = rctBlk.left / uBlockSizeX;
        UINT uBlockY = rctBlk.top  / uBlockSizeY;

        // Read source block.
        uPendingReads++;
        VT_HR_EXIT( m_pBlobStore->ReadBlob(m_pBlob,
            m_vecBlocks[uIndex][uLevel][uBlockY * cBlocksX + uBlockX] * (m_uTractSize / m_pBlobStore->GetTractSize()),
            uTracts * (m_uTractSize / m_pBlobStore->GetTractSize()), pbData, pCallback) );
    }

    // Wait for all pending reads to complete.
    while (uPendingReads > 0)
        if (vt::CSystem::WaitForSingleObjectEx(hReadEvent, INFINITE, TRUE) == WAIT_FAILED)
            VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

    VT_HR_EXIT_LABEL()

    if (hReadEvent != NULL)
        CloseHandle( hReadEvent );

    return hr;
}


HRESULT
CVtiReaderWriter::WriteRegion(UINT uIndex, IN const CRect& region, IN const CImg& src,
                              UINT uLevel)
{
    VT_HR_BEGIN()

    if (uIndex >= m_vecInfos.size() || uLevel >= m_vecInfos[uIndex].uNumLevels)
        VT_HR_EXIT( E_INVALIDARG );

    CLayerImgInfo info = GetImgInfo(uIndex, uLevel);

    // Clip region to level.
    CRect rect = region & info.Rect();

	// For write image size must match region size
	// (for read image will be re-created with region size).
	if (rect.Width() != src.Width() || rect.Height() != src.Height())
		VT_HR_EXIT( E_INVALIDARG );

	CImgReaderWriter<CImg> img;
    VT_HR_EXIT( src.Share(img) );

    VT_HR_EXIT( WriteRegion(uIndex, rect, &img, uLevel, NULL) );

    VT_HR_END()
}

HRESULT
CVtiReaderWriter::WriteRegion(UINT uIndex, IN const CRect& region, IN IImageReader* src,
                              UINT uLevel, const void* pbValue)
{
    HANDLE hWriteEvent = NULL;

    VT_HR_BEGIN()

    if (uIndex >= m_vecInfos.size() || uLevel >= m_vecInfos[uIndex].uNumLevels)
        VT_HR_EXIT( E_INVALIDARG );

    if ((src == NULL && pbValue == NULL) ||
        (src != NULL && pbValue != NULL))
        VT_HR_EXIT( E_INVALIDARG);

    VT_PTR_EXIT( m_pBlob );

    if (m_vecBlocks.empty())
        VT_HR_EXIT( E_NOINIT );

    CLayerImgInfo info = GetImgInfo(uIndex, uLevel);
    UINT uBlockSizeX = m_vecInfos[uIndex].uBlockSizeX;
    UINT uBlockSizeY = m_vecInfos[uIndex].uBlockSizeY;

    // Pad rect to block size.
    CRect rctPad;
    rctPad.left = (region.left / uBlockSizeX) * uBlockSizeX;
    rctPad.top  = (region.top  / uBlockSizeY) * uBlockSizeY;
    rctPad.right  = VtMin(info.Rect().right,  (LONG)
        (((region.right  + uBlockSizeX - 1) / uBlockSizeX) * uBlockSizeX));
    rctPad.bottom = VtMin(info.Rect().bottom, (LONG)
        (((region.bottom + uBlockSizeY - 1) / uBlockSizeY) * uBlockSizeY));

    UINT cBlocksX = (info.width  + uBlockSizeX - 1) / uBlockSizeX;

    UINT uPendingWrites = 0;
    hWriteEvent = CSystem::CreateEvent(FALSE, FALSE);
    VT_PTR_OOM_EXIT( hWriteEvent );

    // Write blocks to the blob.
    for (CBlockIterator bi(BLOCKITER_INIT(rctPad, uBlockSizeX, uBlockSizeY));
        !bi.Done(); bi.Advance())
    {
        CRect rctBlk = bi.GetCompRect();

        // Round up to multiple of tract size.
        UINT uTractMask = m_uTractSize - 1;
        UINT uStrideBytes = (rctBlk.Width() * info.PixSize() + 0x3f) & ~0x3f;
        UINT cbData = (uStrideBytes * rctBlk.Height() + uTractMask) & ~uTractMask;
        VT_ASSERT( cbData % m_uTractSize == 0 );
        UINT uTracts = cbData / m_uTractSize;

        // Get callback.
        CVtiBlobCallback *pCallback;
        VT_HR_EXIT( GetCallback((LPVOID *) &pCallback, cbData) );
        VtMemset(pCallback->m_pbAlloc, 0, pCallback->m_cbAlloc + m_uPageSize);
        pCallback->m_pDst = NULL;
        pCallback->m_pPending = &uPendingWrites;
        pCallback->m_hEvent = hWriteEvent;

        // Round up to multiple of page size.
        UINT_PTR uPageMask = m_uPageSize - 1;
        Byte *pbData = (Byte *) (((UINT_PTR) pCallback->m_pbAlloc + uPageMask) & ~uPageMask);

        CImg imgBlk;
        VT_HR_EXIT( imgBlk.Create(pbData, rctBlk.Width(), rctBlk.Height(), uStrideBytes,
                                  info.type) );

        // Share block of source image.
        CRect rctSrc = (region & rctBlk) - region.TopLeft();

        // Share block of destination image.
        CRect rctDst = (region & rctBlk) - rctBlk.TopLeft();
        CImg imgDst;
        VT_HR_EXIT( imgBlk.Share(imgDst, &rctDst) );

        UINT uBlockX = rctBlk.left / uBlockSizeX;
        UINT uBlockY = rctBlk.top  / uBlockSizeY;

        // If write doesn't fully overlap the block then we need to read the block from the blob.
        if ((region & rctBlk) != rctBlk)
            VT_HR_EXIT( m_pBlobStore->ReadBlob(m_pBlob,
                m_vecBlocks[uIndex][uLevel][uBlockY * cBlocksX + uBlockX] * (m_uTractSize / m_pBlobStore->GetTractSize()),
                uTracts * (m_uTractSize / m_pBlobStore->GetTractSize()), pbData) );

        // Read source block or fill destination block.
        if (src != NULL)
        {
            VT_HR_EXIT( src->ReadRegion(rctSrc, imgDst) );
        }
        else
        {
            VT_HR_EXIT( imgDst.Fill((Byte *) pbValue) );
        }

        // Write desination block.
        uPendingWrites++;
        VT_HR_EXIT( m_pBlobStore->WriteBlob(m_pBlob,
            m_vecBlocks[uIndex][uLevel][uBlockY * cBlocksX + uBlockX] * (m_uTractSize / m_pBlobStore->GetTractSize()),
            uTracts * (m_uTractSize / m_pBlobStore->GetTractSize()), pbData, pCallback) );
    }

    // Wait for all pending writes to complete.
    while (uPendingWrites > 0)
        if (vt::CSystem::WaitForSingleObjectEx(hWriteEvent, INFINITE, TRUE) == WAIT_FAILED)
            VT_HR_EXIT( HRESULT_FROM_WIN32( GetLastError() ) );

    VT_HR_EXIT_LABEL()

    if (hWriteEvent != NULL)
        CloseHandle( hWriteEvent );

    return hr;
}


HRESULT
CVtiReaderWriter::CreateBlob(const vector<CLayerImgInfo>& vecInfos,
                             int iMaxLevel, LPCWSTR pwszName, IBlobStore* pBlobStore,
                             UINT uTractSize)
{
    LPBYTE pHeader = NULL;

    VT_HR_BEGIN()

    VTI_HEADER* pHead = NULL;

    if (vecInfos.empty())
        VT_HR_EXIT( E_INVALIDARG );

    if (m_pBlob != NULL)
        CloseBlob();

    m_pBlobStore = pBlobStore == NULL ? m_pFileStore : pBlobStore;

    VT_PTR_EXIT( m_pBlobStore );

    if (uTractSize > 0 && uTractSize % m_pBlobStore->GetTractSize() != 0)
        VT_HR_EXIT( E_INVALIDARG );

    m_uTractSize = VtMax(uTractSize, m_pBlobStore->GetTractSize());

    // Copy infos and find block sizes.
    for (size_t uIndex = 0; uIndex < vecInfos.size(); uIndex++)
    {
        VTI_INFO info;

        // CImgInfo
		info.uType    = IMG_FORMAT(vecInfos[uIndex].type);
        info.uWidth   = vecInfos[uIndex].width;
        info.uHeight  = vecInfos[uIndex].height;

        if (info.uWidth == 0 || info.uHeight == 0 || 
			info.uType == OBJ_UNDEFINED )
            VT_HR_EXIT( E_INVALIDARG );

        // CLayerImgInfo
        info.uCompositeWidth  = vecInfos[uIndex].compositeWidth;
        info.uCompositeHeight = vecInfos[uIndex].compositeHeight;
        info.uOriginX         = vecInfos[uIndex].origin.x;
        info.uOriginY         = vecInfos[uIndex].origin.y;
        info.uFlags           =
            (vecInfos[uIndex].compositeWrapX ? COMPOSITE_WRAP_X : 0x0) |
            (vecInfos[uIndex].compositeWrapY ? COMPOSITE_WRAP_Y : 0x0); 

        info.uNumLevels = iMaxLevel >= 0 ? iMaxLevel + 1 :
            ComputePyramidLevelCount(info.uWidth, info.uHeight);

        // Increase block size to approach tract size.
		int pixsize = VT_IMG_BANDS(info.uType) * VT_IMG_ELSIZE(info.uType);
        
		info.uBlockSizeX = 256;
        info.uBlockSizeY = 256;
        while (info.uBlockSizeX * info.uBlockSizeY * pixsize <= (m_uTractSize >> 1))
        {
            info.uBlockSizeX <<= 1;
            if (info.uBlockSizeX * info.uBlockSizeY * pixsize <= (m_uTractSize >> 1))
               info.uBlockSizeY <<= 1;
        }

        VT_HR_EXIT( m_vecInfos.push_back(info) );
    }

    // Compute header size.
    UINT uHeadSize = sizeof(VTI_HEADER) +
        (UINT) vecInfos.size() * sizeof (VTI_INFO);
    UINT uHeadTracts = (uHeadSize + m_uTractSize - 1) / m_uTractSize;

    // Create header.
    pHeader = VT_NOTHROWNEW Byte[uHeadTracts * m_uTractSize + m_uPageSize];
    VT_PTR_OOM_EXIT( pHeader );
    VtMemset(pHeader, 0, uHeadTracts * m_uTractSize + m_uPageSize);

    // Fill header aligned to page size.
    UINT_PTR uPageMask = m_uPageSize - 1;
    pHead = (VTI_HEADER *) (((UINT_PTR) pHeader + m_uPageSize - 1) & ~uPageMask);
    memcpy(pHead->uMagic, VTI_HEADER().uMagic, sizeof(pHead->uMagic));
    pHead->uAlignment = m_uTractSize;
    pHead->uNumImages = (UINT) vecInfos.size();
    VtMemcpy(pHead + 1, m_vecInfos.begin(), sizeof(VTI_INFO) * m_vecInfos.size());

    // Compute header size.
    m_uNumTracts = (uHeadSize + m_uTractSize - 1) / m_uTractSize;

    VT_HR_EXIT( ComputeSourceSize() );

    // Try to create a blob.
    VT_HR_EXIT( m_pBlobStore->CreateBlob(m_pBlob, pwszName, m_uNumTracts *
        (m_uTractSize / m_pBlobStore->GetTractSize())) );

    // Set the blob size.
    VT_HR_EXIT( m_pBlobStore->ExtendBlob(m_pBlob, m_uNumTracts *
        (m_uTractSize / m_pBlobStore->GetTractSize())) );

    // Write header.
    VT_HR_EXIT( m_pBlobStore->WriteBlob(m_pBlob, 0, uHeadTracts *
        (m_uTractSize / m_pBlobStore->GetTractSize()), (LPBYTE) pHead) );

    VT_HR_EXIT_LABEL()
        
    delete[] pHeader;

    if (hr != S_OK)
    {
        if (m_pBlob != NULL)
            CloseBlob();
    }

    return hr;
}

HRESULT
CVtiReaderWriter::OpenBlob(LPCWSTR pwszName, IBlobStore* pBlobStore,
                           bool bReadOnly)
{
    LPBYTE pHeader = NULL;

    VT_HR_BEGIN()

    VTI_HEADER* pHead = NULL;
    UINT uNumTracts = 0;

    VT_PTR_EXIT( pwszName );

    if (m_pBlob != NULL)
        CloseBlob();

    m_pBlobStore = pBlobStore == NULL ? m_pFileStore : pBlobStore;

    VT_PTR_EXIT( m_pBlobStore );

    UINT uTractSize = m_pBlobStore->GetTractSize();

    // Attempt to open existing blob.
    VT_HR_EXIT( m_pBlobStore->OpenBlob(m_pBlob, uNumTracts, pwszName, bReadOnly) );

    // Create 1st tract of header.
    pHeader = VT_NOTHROWNEW Byte[uTractSize + m_uPageSize];
    VT_PTR_OOM_EXIT( pHeader );

    // Align 1st tract of header to page size.
    UINT_PTR uPageMask = m_uPageSize - 1;
    pHead = (VTI_HEADER *)
        (((UINT_PTR) pHeader + m_uPageSize - 1) & ~uPageMask);

    // Read 1st tract of header.
    VT_HR_EXIT( m_pBlobStore->ReadBlob(m_pBlob, 0, 1, (LPBYTE) pHead) );
    
    // Allow 0 magic for backwards compatibility.
    UINT8 uMagic[4] = { 0 };
    if ((0 != memcmp(pHead->uMagic, uMagic, sizeof(pHead->uMagic)) &&
         0 != memcmp(pHead->uMagic, VTI_HEADER().uMagic, sizeof(pHead->uMagic))) ||
        0 == pHead->uAlignment || pHead->uAlignment % uTractSize != 0)
        VT_HR_EXIT( E_UNEXPECTED );

    // Compute full header size.
    UINT uHeadSize = sizeof(VTI_HEADER) +
        pHead->uNumImages * sizeof(VTI_INFO);
    UINT uHeadTracts = (uHeadSize + uTractSize - 1) / uTractSize;

    if (uHeadTracts > 1)
    {
        // Create full header.
        delete[] pHeader;
        pHeader = VT_NOTHROWNEW Byte[uHeadTracts * uTractSize + m_uPageSize];
        VT_PTR_OOM_EXIT( pHeader );

        // Align full header to page size.
        pHead = (VTI_HEADER *)
            (((UINT_PTR) pHeader + m_uPageSize - 1) & ~uPageMask);

        // Read full header.
        VT_HR_EXIT( m_pBlobStore->ReadBlob(m_pBlob, 0,
            uHeadTracts, (LPBYTE) pHead) );
    }

    m_uTractSize = pHead->uAlignment;

    VT_HR_EXIT( m_vecInfos.resize(pHead->uNumImages) );
    VtMemcpy(m_vecInfos.begin(), pHead + 1, sizeof(VTI_INFO) * pHead->uNumImages);

    // Compute header size.
    m_uNumTracts = (uHeadSize + m_uTractSize - 1) / m_uTractSize;

    VT_HR_EXIT( ComputeSourceSize() );

    if (m_uNumTracts != uNumTracts )
        VT_HR_EXIT( E_UNEXPECTED );

    VT_HR_EXIT_LABEL()
        
    delete[] pHeader;

    if (hr != S_OK)
    {
        if (m_pBlob != NULL)
            CloseBlob();
    }

    return hr;
}

HRESULT
CVtiReaderWriter::CloseBlob()
{
    VT_HR_BEGIN()

    VT_PTR_EXIT( m_pBlobStore );

    VT_HR_EXIT( m_pBlobStore->CloseBlob(m_pBlob) );

    m_pBlob = NULL;

    m_uTractSize = 0;
    m_uNumTracts = 0;

    m_vecInfos.clear();

    VT_HR_END()
}


HRESULT
CVtiReaderWriter::GetCallback(LPVOID* ppCallback, UINT cbAlloc)
{
    VT_HR_BEGIN()

    VT_PTR_EXIT( ppCallback );

    CVtiBlobCallback* pCallback;

    // if there is an unused callback available use it otherwise allocate one
    if (m_pCallbacks != NULL)
    {
        pCallback = (CVtiBlobCallback *) m_pCallbacks;
        m_pCallbacks = ((CVtiBlobCallback *) m_pCallbacks)->m_pNext;
    }
    else
    {
        pCallback = VT_NOTHROWNEW CVtiBlobCallback;
        VT_PTR_OOM_EXIT( pCallback );
    }

    pCallback->m_pThis = this;

    if (pCallback->m_cbAlloc < cbAlloc)
    {
        delete[] pCallback->m_pbAlloc;

        // Add page size for start alignment.
        pCallback->m_pbAlloc = VT_NOTHROWNEW Byte[cbAlloc + m_uPageSize];
        VT_PTR_OOM_EXIT( pCallback->m_pbAlloc );

        pCallback->m_cbAlloc = cbAlloc;
    }

    *ppCallback = pCallback;

    VT_HR_END()
}

VOID
CVtiReaderWriter::BlobIOCompletionRoutine(LPVOID pCallback)
{
    // add to the callbacks
    ((CVtiBlobCallback *) pCallback)->m_pNext =
        (CVtiBlobCallback *) m_pCallbacks;
    m_pCallbacks = pCallback;
}
