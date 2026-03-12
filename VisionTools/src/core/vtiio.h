//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Interfaces to access portions of an image or image list
//
//  History:
//      2007/06/1-mattu
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_readerwriter.h"
#include "vt_blobstore.h"

namespace vt {

//+-----------------------------------------------------------------------------
//
// Class: CVtiReader
// 
// Synopsis: an IIndexedImageReader that wraps a blob (.vti file)
//
//------------------------------------------------------------------------------
__declspec(align(4)) struct VTI_HEADER
{
    UINT8  uMagic[4];
    UINT32 uAlignment;
    UINT32 uNumImages;

    VTI_HEADER() : uAlignment(0), uNumImages(0)
    {
        uMagic[0] = 'V';
        uMagic[1] = 'T';
        uMagic[2] = 'I';
        uMagic[3] = 0;  // version
    }
};

__declspec(align(4)) struct VTI_INFO
{
    // CImgInfo
    UINT32 uType;
    UINT32 uWidth;
    UINT32 uHeight;

    // CLayerImgInfo
    UINT32 uCompositeWidth;
    UINT32 uCompositeHeight;
    UINT32 uOriginX;
    UINT32 uOriginY;
#define COMPOSITE_WRAP_X 0x1
#define COMPOSITE_WRAP_Y 0x2
    UINT32 uFlags; 

    UINT32 uNumLevels;

    UINT32 uBlockSizeX;
    UINT32 uBlockSizeY;

    VTI_INFO() : uType(OBJ_UNDEFINED), uWidth(0), uHeight(0),
        uCompositeWidth(0), uCompositeHeight(0),
        uOriginX(0), uOriginY(0), uFlags(0x0),
        uNumLevels(0), uBlockSizeX(0), uBlockSizeY(0) {}
};

class CVtiReaderWriter : public IIndexedImageReaderWriter
{
public:
    CVtiReaderWriter();
    virtual ~CVtiReaderWriter();

	// IIndexedImageReader implementation
	virtual UINT GetFrameCount()
	{ return (UINT) m_vecInfos.size(); }

	virtual CLayerImgInfo GetImgInfo(UINT uIndex, UINT uLevel = 0);

	virtual HRESULT GetMetaData(UINT /*uIndex*/, OUT CParams& /*params*/)
	{ return E_NOTIMPL;	}

	virtual HRESULT ReadImg(UINT uIndex, OUT CImg& dst, UINT uLevel = 0)
	{ return ReadRegion(uIndex,
        CRect(0, 0, m_vecInfos[uIndex].uWidth, m_vecInfos[uIndex].uHeight), dst, uLevel); }

	virtual HRESULT ReadRegion(UINT uIndex, IN const CRect& region, OUT CImg& dst,
							   UINT uLevel = 0);

	virtual HRESULT Prefetch(UINT /*uIndex*/, IN const CRect& /*region*/, 
							 UINT /*uLevel*/)
	{ return S_OK; }

	// IIndexedImageWriter implementation
	virtual HRESULT SetMetaData(UINT /*uIndex*/, IN const CParams* /*pParams*/)
	{ return E_NOTIMPL;	}

	virtual HRESULT WriteImg(UINT uIndex, IN const CImg& src, UINT uLevel = 0)
	{ return WriteRegion(uIndex,
        CRect(0, 0, m_vecInfos[uIndex].uWidth, m_vecInfos[uIndex].uHeight), src, uLevel); }

	virtual HRESULT WriteRegion(UINT uIndex, IN const CRect& region, IN const CImg& src,
							    UINT uLevel = 0);

	virtual HRESULT Fill(UINT uIndex, const void* pbValue,
						 const RECT *prct = NULL, UINT uLevel = 0)
    { return WriteRegion(uIndex, prct != NULL ? *prct : GetImgInfo(uIndex, uLevel).Rect(),
                         NULL, uLevel, pbValue); }

public:
	HRESULT CreateBlob(const vector<CLayerImgInfo>& vecInfos,
                       int iMaxLevel = -1, LPCWSTR pwszName = NULL,
                       IBlobStore* pBlobStore = NULL,
                       UINT uTractSize = 0);

	HRESULT OpenBlob(LPCWSTR pwszName, IBlobStore* pBlobStore = NULL,
                     bool bReadOnly = false);

    HRESULT CloseBlob();

    UINT    GetLevelCount(UINT uIndex)
    { return uIndex < m_vecBlocks.size() ? (UINT) m_vecBlocks[uIndex].size() : 0; }

    UINT    GetTractSize()
    { return m_uTractSize; }

    LPVOID  GetBlob()
    { return m_pBlob; }

    VOID BlobIOCompletionRoutine(LPVOID pCallback);

protected:
	HRESULT ReadRegion(UINT uIndex, IN const CRect& region, OUT IImageWriter* dst,
                       UINT uLevel);

	HRESULT WriteRegion(UINT uIndex, IN const CRect& region, IN IImageReader* src,
						UINT uLevel, const void* pbValue);

    HRESULT GetCallback(LPVOID* pCallback, UINT cbData);

    HRESULT ComputeSourceSize();

    IBlobStore* m_pFileStore;
    IBlobStore* m_pBlobStore;
    LPVOID      m_pBlob;

    UINT        m_uPageSize;
    UINT        m_uTractSize;
    UINT        m_uNumTracts;

    vt::vector<VTI_INFO> m_vecInfos;

    vt::vector<vt::vector<vt::vector<UINT>>> m_vecBlocks;

    // Empty callbacks to reduce memory allocations.
    LPVOID m_pCallbacks;
};

};
