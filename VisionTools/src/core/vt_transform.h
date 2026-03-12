//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Interface to apply an transform (filter, saturation, brightness, etc)
//      to an image.
//
//  History:
//      2007/06/1-mattu
//          Created
//
//------------------------------------------------------------------------------
#pragma once

#include "vtcommon.h"
#include "vt_taskmanager.h"
#include "vt_readerwriter.h"
#include "vt_extend.h"
#include "vt_pixeliterators.h"

#pragma warning(disable:28204) // disable requirement that base and derived class methods all have the same annotations

namespace vt {

//+-----------------------------------------------------------------------------
//
// Class: ITransformBlockMap
//
/// \ingroup transforms
/// <summary>The base interface for generating destination regions while 
/// stepping through a transform work item, evaluating transforms.
/// </summary>
//
//------------------------------------------------------------------------------
class ITransformBlockMap
{
public:
	virtual ~ITransformBlockMap() = 0
	{}

	virtual int     GetTotalBlocks() = 0;

	virtual HRESULT GetBlockItems(vt::CRect& rct, int& iPrefetchCount, 
								  int iBlockIndex) = 0;

	virtual CRect   GetPrefetchRect(int iBlockIndex, int iPrefetchIndex) = 0;
};


//+-----------------------------------------------------------------------------
//
// Class: CRasterBlockMap
// 
// Synopsis: Implementation of ITransformBlockMap. Traverses the destination
//           image one block at a time.  By default the traversal is raster
//           scan in blocks. There is a configurable major block size.  If this
//           is choosen, the traversal will be raster in major blocks, then
//           within that raster in minor blocks.
// 
//------------------------------------------------------------------------------
class CRasterBlockMap: public ITransformBlockMap
{
public:
	virtual HRESULT GetBlockItems(CRect& rct, int& iPrefetchCount, int iBlockIndex);
	  
	virtual CRect GetPrefetchRect(int iBlockIndex, int iPrefetchIndex);

	// TODO: do we need int64 here?
	virtual int GetTotalBlocks()
	{ return m_iTotalBlkCnt; }

public:
	CRasterBlockMap() : m_iTotalBlkCnt(0)
	{}

	CRasterBlockMap(const BLOCKITER_INIT& bii, 
					int iBlkCntXInMaj=4, int iBlkCntYInMaj=4, int level=0)
	{ Initialize(bii, iBlkCntXInMaj, iBlkCntYInMaj, level); }

	CRasterBlockMap(const vt::CRect& region, int iBlockWidth=256, int iBlockHeight=256,
					int iBlkCntXInMaj=4, int iBlkCntYInMaj=4, int level=0)
	{ 
		Initialize(BLOCKITER_INIT(region,iBlockWidth,iBlockHeight),
	               iBlkCntXInMaj, iBlkCntYInMaj, level);
	}

	void Initialize(const BLOCKITER_INIT& bii, 
					int iBlkCntXInMaj=4, int iBlkCntYInMaj=4, int level=0);

	BLOCKITER_INIT GetInitInfo()
	{ return m_bii; }

	int GetBlockCols()
	{ return m_iBlkCntX; }

	int GetBlockRows()
	{ return m_iBlkCntY; }

protected:
	BLOCKITER_INIT m_bii;
	int m_iLevel;
	int m_iBlkCntX;
	int m_iBlkCntY;
	int m_iTotalBlkCnt;
	int m_iBlkCntXInMaj;
	int m_iBlkCntYInMaj;
	int m_iBlkCntInMajRow;
	int m_iBlkCntXInLastMajCol;
	int m_iMajCntX;
	int m_iMajCntY; 
};

//+-----------------------------------------------------------------------------
//
// Class: CSparseBlockMap
// 
// Synopsis: Like CRasterBlockMap but returns empty rects for "empty" blocks.
// 
//------------------------------------------------------------------------------
class CSparseBlockMap: public CRasterBlockMap
{
public:
	CSparseBlockMap()
	{ m_iTotalBlkCnt = 0; }

	CSparseBlockMap(const BLOCKITER_INIT& bii, int level=0)
	{ Initialize(bii, 1, 1, level); }

	CSparseBlockMap(const vt::CRect& region, int iBlockWidth=256, int iBlockHeight=256,
                    int level=0)
	{ 
		Initialize(BLOCKITER_INIT(region,iBlockWidth,iBlockHeight),
	               1, 1, level);
	}

    HRESULT SetRects(const vector<vt::CRect>& vecRects, const vt::CPoint* origin = NULL)
    {
        HRESULT hr;
        if ((int) vecRects.size() != m_iTotalBlkCnt)
            return E_INVALIDARG;
        if ((hr = m_vecRects.resize(m_iTotalBlkCnt)) != S_OK)
            return hr;
        vt::CPoint pt = origin == NULL ? vt::CPoint(0,0) : *origin;
        for (int i = 0; i < m_iTotalBlkCnt; i++)
            m_vecRects[i] = vecRects[i] + pt;
        return S_OK;
    }

	virtual HRESULT GetBlockItems(CRect& rct, int& iPrefetchCount, int iBlockIndex)
    {
        HRESULT hr = CRasterBlockMap::GetBlockItems(rct, iPrefetchCount, iBlockIndex);
        if (!rct.IsRectEmpty() && !m_vecRects.empty())
            rct &= m_vecRects[iBlockIndex];
        return hr;
    }
	  
	virtual CRect GetPrefetchRect(int iBlockIndex, int iPrefetchIndex)
    {
        CRect rct = CRasterBlockMap::GetPrefetchRect(iBlockIndex, iPrefetchIndex);
        if (!rct.IsRectEmpty() && !m_vecRects.empty())
            rct &= m_vecRects[iBlockIndex + 1];
        return rct;
    }

protected:
    vector<CRect> m_vecRects;
};

//+-----------------------------------------------------------------------------
//
// struct: TRANSFORM_SOURCE_DESC
// 
// Synopsis: returned from image transforms to describe which inputs and the 
//           regions within those inputs that are required to generate an
//           output region
// 
//------------------------------------------------------------------------------
struct TRANSFORM_SOURCE_DESC
{
    UINT  uSrcIndex;
	CRect rctSrc;
	// TODO: add a 'pass#' value
	bool  bCanOverWrite;
};

//+-----------------------------------------------------------------------------
//
// Class: IImageTransform
// 
/// \ingroup transforms
/// <summary>The base interface for all image transforms.  Image transforms
/// are composable Vision Tools objects that enable tile-based concurrent
/// (for example multi-core) processing of images.
/// </summary>
// 
//------------------------------------------------------------------------------
class IImageTransform: public ITaskState
{
public:
    virtual ~IImageTransform() = 0
	{} 

	/// <summary>
	/// Given the destination pixel format, return the required format for
	/// each source. This is only called once during the graph setup, 
	/// format resolution stage (TODO: link to reference docs).
	/// </summary>
	/// <param name="ptypeSrcs">The required image type for each of the sources.
	/// If the transform has no preference then it can return the default
	/// OBJ_UNDEFINED
	/// </param>
	/// <param name="uSrcCnt">The number of sources for this transform, which
	/// corresponds to the number of elements in the first argument ptypeSrcs 
	/// array.</param>
	/// <param name="typeDst">The current destination pixel format, might
	/// be unresolved if no parent transforms in the graph have specified a 
	/// prefered type.
	/// </param>
	virtual void    GetSrcPixFormat(_Out_writes_(uSrcCnt) int* ptypeSrcs, 
									IN UINT  uSrcCnt,
									IN int typeDst) = 0;

	/// <summary>
	/// Given the resolved pixel formats for each source, return the 
	/// destination format for this transform. This is only called once during 
	/// the graph setup, format resolution stage (TODO: link to reference docs).
	/// </summary>
	/// <param name="typeDst">The destination image type. Must return a defined 
    /// type, cannot be OBJ_UNDEFINED. Most transforms don't do format 
	/// conversion, so the implementation of GetDstPixFormat simply copies the 
	/// format of one of their sources.
	/// </param>
	/// <param name="ptypeSrcs">The resolved format for each of the sources.
	/// </param>
	/// <param name="uSrcCnt">The number of sources for this transform, which
	/// corresponds to the number of elements in the second argument ptypeSrcs 
	/// array.</param>
	virtual void    GetDstPixFormat(OUT int& typeDst,
									_In_reads_(uSrcCnt) const int* ptypeSrcs, 
									IN  UINT  uSrcCnt) = 0;

	/// <summary>
	/// Given a destination rect, return the source rects required to
	/// generate the destination. This is called for every tile during 
	/// transform graph evaluation(TODO: link to reference docs).
	/// </summary>
	/// <param name="pSrcReq">An array of 
	/// /link TRANSFORM_SOURCE_DESC TRANSFORM_SOURCE_DESC /endlink allocated
	/// by the caller.  The size of the array will be equal to uSrcCnt. Given
	/// the rctLayerDst, the transform will write to this array to return the 
	/// active sources and their rects.</param>
	/// <param name="uSrcReqCount">An output parameter. In this parameter the
	/// transform returns the count of active sources for the destination rect.
	/// This must be less than or equal to uSrcCnt.</param>
	/// <param name="uSrcCnt">The count of sources assigned to this 
	/// transform.</param>
	/// <param name="rctLayerDst">The destination rect for the current tile.
	/// </param>

	virtual HRESULT GetRequiredSrcRect(_Out_writes_(uSrcCnt) TRANSFORM_SOURCE_DESC* pSrcReq,
									   OUT UINT& uSrcReqCount,
									   IN  UINT  uSrcCnt,
									   IN  const CRect& rctLayerDst
									   ) = 0;

	/// <summary>Returns the affected rect in the destination given an dirty
	/// rect in one of the sources.</summary>
	/// <param name="rctDst">The affected rect in the transform detination.
	/// </param>
	/// <param name="rctSrc">The dirty rect in one of the source images</param>
	/// <param name="uSrcIndex">The index within the sources currently bound to
	/// this transform.</param>
	/// <param name="uSrcCnt">The total number of sources currently bound to
	/// the transform.</param>
	/// <DL><DT> Remarks: </DT></DL>
	/// This method is not called during transform graph evaluation. It is 
	/// normally called by applications where a sub-region of a source is 
	/// changed and the application needs to determine the minimum destination
	/// region to re-evaluate.

	virtual HRESULT GetAffectedDstRect(OUT CRect& rctDst,
									   IN  const CRect& rctSrc,
									   IN  UINT uSrcIndex,
									   IN  UINT uSrcCnt) = 0;

	/// <summary>Determine the size of a destination rect given a source rect.
	/// </summary>
	/// <param name="rctDst">The destination rect.</param>
	/// <param name="rctSrc">The source rect.</param>
	/// <param name="uSrcIndex">The index within the sources currently bound to
	/// this transform.</param>
	/// <param name="uSrcCnt">The total number of sources currently bound to
	/// the transform.</param>
	/// This method is not called during transform graph evaluation. Applications
	/// use this to determine the size of a destination image given a source
	/// rect. It behaves very much like GetAffectedDstRect except that it trims
	/// the rect for certain geotransforms to eliminate filter support pixels.

	virtual HRESULT GetResultingDstRect(OUT CRect& rctDst,
										IN  const CRect& rctSrc,
										IN  UINT uSrcIndex,
										IN  UINT uSrcCnt) = 0;

	/// <summary>Generates the pixels in a tile of the destination image given 
	/// a set of tiles in the source images. This is where all the pixel 
	/// processing is done and is called for every tile during transform graph 
	/// evaluation(TODO: link to reference docs).</summary>
	/// <param name="pimgDstRegion">The output tile. This can be NULL if the
	/// transform is not producing an image, (for example stats transforms that
	/// compute historgrams).</param>
	/// <param name="rctLayerDst">The position of the output tile within the
	/// larger destination image.</param>
	/// <param name="ppimgSrcRegions">The source tiles required to generate the
	/// output tile.  These will have been deteremined by a previous call to
	/// \link GetRequiredSrcRect() GetRequiredSrcRect \endlink .</param>
	/// <param name="pSrcDesc">The positions and indecies of each of the 
	/// source tiles. This will be a copy of the array returned by 
	/// \link GetRequiredSrcRect() GetRequiredSrcRect \endlink</param>
	/// <param name="uSrcCnt">The count of images in the ppimgSrcRegions
	/// array and descriptions in the pSrcDesc array.</param>

	// TODO: add uPassCnt parameter
	virtual HRESULT Transform(OUT CImg* pimgDstRegion, 
							  IN  const CRect& rctLayerDst,
							  _In_reads_(uSrcCnt) CImg *const *ppimgSrcRegions,
							  _In_reads_(uSrcCnt) const TRANSFORM_SOURCE_DESC* pSrcDesc,
							  IN  UINT  uSrcCnt
							  ) = 0;
};

//+-----------------------------------------------------------------------------
//
// struct: IMAGEREADER_SOURCE
// 
// Synopsis: returned from GetReaderSource method on CTransformGraphNode
//           contains the reader interface and the extend mode
// 
//------------------------------------------------------------------------------
struct IMAGEREADER_SOURCE
{ 
    enum eType
    {
        eReader = 0,
        eIndexedReader = 1
    };

    eType type;
    union
    {	
        IImageReader*        pReader;
        IIndexedImageReader* pIndexedReader;
    };
    
    int level;
    int idx;
	IMAGE_EXTEND  ex;

	IMAGEREADER_SOURCE() : pReader(NULL), level(0), idx(0)
	{}

	void Initialize(IImageReader* pReaderInit, int levelInit)
	{
        type    = eReader;
        pReader = pReaderInit;
        level   = levelInit;
        idx     = 0;
		ex.Initialize(Zero, Zero);
	}

	HRESULT Initialize(IImageReader* pReaderInit, const IMAGE_EXTEND* pEx,
                       int levelInit)
	{
        type    = eReader;
        level   = levelInit;
		pReader = pReaderInit;
        idx     = 0;
		return ex.Initialize(pEx);
	}

	void Initialize(IIndexedImageReader* pReaderInit, int idxInit, int levelInit)
	{
        type    = eIndexedReader;
        pIndexedReader = pReaderInit;
        level   = levelInit;
        idx     = idxInit;
		ex.Initialize(Zero, Zero);
	}

	HRESULT Initialize(IIndexedImageReader* pReaderInit, int idxInit,
                       const IMAGE_EXTEND* pEx, int levelInit)
	{
        type    = eIndexedReader;
        level   = levelInit;
		pIndexedReader = pReaderInit;
        idx     = idxInit;
		return ex.Initialize(pEx);
	}

private:
	IMAGEREADER_SOURCE(const IMAGEREADER_SOURCE&);
	IMAGEREADER_SOURCE& operator=(const IMAGEREADER_SOURCE&);
};

//+-----------------------------------------------------------------------------
//
// struct: NODE_DEST_PARAMS
// 
// Synopsis: contains the writer and optional block map for a transform
//           graph node
// 
//------------------------------------------------------------------------------
struct NODE_DEST_PARAMS
{
	IImageWriter*       pWriter;
	ITransformBlockMap* pBlockMap; // block generator for this node

	NODE_DEST_PARAMS() : pWriter(NULL), pBlockMap(NULL) 
	{}

	NODE_DEST_PARAMS(IImageWriter* pWriterI, ITransformBlockMap* pBlockMapI=NULL) :
		pWriter(pWriterI), pBlockMap(pBlockMapI)
	{}
};

//+-----------------------------------------------------------------------------
//
// Class: CTransformGraphNode
// 
// Synopsis: A node in a transform graph.  This allows transforms to be chained
//           together and bound to input readers.  In addition this holds the
//           information for how to generate pad pixels (the extend mode) for
//           transforms that require pixels outside of its source image 
//           boundaries
// 
//------------------------------------------------------------------------------
class CTransformGraphNode
{
public:
	enum eSourceType
	{
		eReaderSource    = 0,
		eTransformSource = 1
	};

public:
	CTransformGraphNode() : m_pTransform(NULL)
	{} 

	CTransformGraphNode(IImageTransform* pTransform, IImageWriter* pDst=NULL) : 
		m_pTransform(pTransform), m_dest(pDst)
	{}

	virtual ~CTransformGraphNode() = 0
	{} 

	virtual void SetDest(IImageWriter* p)
	{ m_dest = NODE_DEST_PARAMS(p); }

	virtual void SetDest(const NODE_DEST_PARAMS& p)
	{ m_dest = p; }

	virtual NODE_DEST_PARAMS GetDest() const
	{ return m_dest; }

	virtual void SetTransform(IImageTransform* pTransform)
	{ m_pTransform = pTransform; }

	virtual IImageTransform* GetTransform() const
	{ return m_pTransform; }

	virtual UINT                       GetSourceCount() const = 0;
	virtual eSourceType                GetSourceType(UINT uIndex) const = 0;
	virtual const IMAGEREADER_SOURCE*  GetReaderSource(UINT uIndex) const = 0;  
	virtual const CTransformGraphNode* GetTransformSource(UINT uIndex) const = 0;
 
protected:
	struct SOURCE_DESC
	{
        SOURCE_DESC() : type(eReaderSourceIntrnl)
        {}

        ~SOURCE_DESC()
        { Clear(); }

		enum eSourceTypeInternal
		{
			eReaderSourceIntrnl    = 0,
			eTransformSourceIntrnl = 1,
		};

		eSourceTypeInternal GetType() const
		{ return type; }

		void SetReader(IImageReader* pReaderInit, int level)
		{
			Clear();
			type = eReaderSourceIntrnl;
			reader.Initialize(pReaderInit, level);
		}

		HRESULT SetReader(IImageReader* pReaderInit, const IMAGE_EXTEND* pEx, int level)
		{
			Clear();
			type = eReaderSourceIntrnl;
			return reader.Initialize(pReaderInit, pEx, level);
		}

		void SetIndexedReader(IIndexedImageReader* pReaderInit, int idx, int level)
		{
			Clear();
			type = eReaderSourceIntrnl;
			reader.Initialize(pReaderInit, idx, level);
		}

		HRESULT SetIndexedReader(IIndexedImageReader* pReaderInit, int idx,
                                 const IMAGE_EXTEND* pEx, int level)
		{
			Clear();
			type = eReaderSourceIntrnl;
			return reader.Initialize(pReaderInit, idx, pEx, level);
		}

		const IMAGEREADER_SOURCE* GetReader() const
		{ return &reader; }  

		void SetTransform(CTransformGraphNode* pNodeInit)
		{
			Clear();
			type = eTransformSourceIntrnl;
			pNode = pNodeInit;   
		}

		const CTransformGraphNode* GetTransform() const
		{ return pNode; }  

	protected:
		void Clear()
		{
            reader.level   = 0;
			reader.pReader = NULL;
			pNode          = NULL;
			type = eReaderSourceIntrnl;
		}

	protected:
		eSourceTypeInternal  type;
		IMAGEREADER_SOURCE   reader; 
		CTransformGraphNode* pNode;

	private:
		SOURCE_DESC(const SOURCE_DESC&);
		SOURCE_DESC& operator=(const SOURCE_DESC&);
	};

protected:
    NODE_DEST_PARAMS m_dest; 
	IImageTransform* m_pTransform;
};

//+-----------------------------------------------------------------------------
//
// Class: CTransformGraphNoSrcNode
// 
// Synopsis: a CTransformGraphNode with no source reader input
// 
//------------------------------------------------------------------------------
class CTransformGraphNoSrcNode: public CTransformGraphNode
{
public:
	// CTransformGraphNode implementation
    virtual UINT                       GetSourceCount() const
    { return 0; }
    virtual eSourceType                GetSourceType(UINT) const
    { return eReaderSource; }
    virtual const IMAGEREADER_SOURCE*  GetReaderSource(UINT) const
    { return NULL; }
    virtual const CTransformGraphNode* GetTransformSource(UINT) const
    { return NULL; }
public:
    CTransformGraphNoSrcNode() 
    {} 

    CTransformGraphNoSrcNode(IImageTransform* pTransform, IImageWriter* pDst=NULL): 
        CTransformGraphNode(pTransform, pDst)
    {}
};

//+-----------------------------------------------------------------------------
//
// Class: CTransformGraphUnaryNode
// 
// Synopsis: a CTransformGraphNode with a single input
// 
//------------------------------------------------------------------------------
class CTransformGraphUnaryNode: public CTransformGraphNode
{
public:
	// CTransformGraphNode implementation
	virtual UINT GetSourceCount() const
	{ return 1; }

	virtual eSourceType GetSourceType(UINT uIndex) const
	{ 
		UNREFERENCED_PARAMETER(uIndex);
		VT_ASSERT( uIndex == 0 );
        return (m_source.GetType()==SOURCE_DESC::eTransformSourceIntrnl)? 
			eTransformSource: eReaderSource;
    }

	virtual const IMAGEREADER_SOURCE* GetReaderSource(UINT uIndex) const
	{
		UNREFERENCED_PARAMETER(uIndex);
		VT_ASSERT( uIndex == 0 );
		VT_ASSERT( GetSourceType(0) == eReaderSource );
		return m_source.GetReader();
	}

	virtual const CTransformGraphNode* GetTransformSource(UINT uIndex) const
	{
		UNREFERENCED_PARAMETER(uIndex);
		VT_ASSERT( uIndex == 0 );
		VT_ASSERT( GetSourceType(0) == eTransformSource );
		return m_source.GetTransform();
	}

private:
	CTransformGraphUnaryNode(const CTransformGraphUnaryNode&);
	CTransformGraphUnaryNode& operator=(const CTransformGraphUnaryNode&);

public:
	// Specific CTransformGraphUnaryNode implementation
    CTransformGraphUnaryNode()
    {}

	CTransformGraphUnaryNode(IImageTransform* pTransform, 
							 IImageReader*    pSrc=NULL,
							 IImageWriter*    pDst=NULL) :
	    CTransformGraphNode(pTransform, pDst)
	{ m_source.SetReader(pSrc, 0); } 

	CTransformGraphUnaryNode(IImageTransform* pTransform, 
							 CTransformGraphNode* pNode,
							 IImageWriter*    pDst=NULL) :
		CTransformGraphNode(pTransform, pDst)
	{ m_source.SetTransform(pNode); }

	HRESULT BindSourceToReader( IImageReader* pSrc,
								const IMAGE_EXTEND* pex = NULL, int level = 0 )
	{ return m_source.SetReader(pSrc, pex, level); }

	HRESULT BindSourceToIndexedReader( IIndexedImageReader* pSource,
								       UINT uReaderIndex,
									   const IMAGE_EXTEND* pex = NULL,
                                       int level = 0)
	{
	    VT_ASSERT( uReaderIndex < pSource->GetFrameCount() );
		return m_source.SetIndexedReader(pSource, uReaderIndex, pex, level);
	}

	HRESULT BindSourceToTransform( CTransformGraphNode* pNode )
	{
		m_source.SetTransform(pNode);
		return S_OK;
	}

protected:
	SOURCE_DESC m_source;
};

//+-----------------------------------------------------------------------------
//
// Class: CTransformGraphNaryNode
// 
// Synopsis: a CTransformGraphNode with multiple inputs
// 
//------------------------------------------------------------------------------
class CTransformGraphNaryNode: public CTransformGraphNode
{
public:
	// CTransformGraphNode implementation	
	virtual UINT GetSourceCount() const
	{ return (UINT)m_vecSources.size(); }

	virtual eSourceType GetSourceType(UINT uIndex) const
	{ 
		VT_ASSERT( uIndex < GetSourceCount() );
		const SOURCE_DESC& source = m_vecSources[uIndex];
        return (source.GetType()==SOURCE_DESC::eTransformSourceIntrnl)? 
			eTransformSource: eReaderSource;
    }

	virtual const IMAGEREADER_SOURCE* GetReaderSource(UINT uIndex) const
	{
		VT_ASSERT( uIndex < GetSourceCount() );
		VT_ASSERT( GetSourceType(uIndex) == eReaderSource );
		return m_vecSources[uIndex].GetReader();
	}

	virtual const CTransformGraphNode* GetTransformSource(UINT uIndex) const
	{
	    VT_ASSERT( uIndex < GetSourceCount() );
		VT_ASSERT( GetSourceType(uIndex) == eTransformSource );
        return m_vecSources[uIndex].GetTransform();
	}

public:
	// Specific CTransformGraphNaryNode implementation
    CTransformGraphNaryNode(IImageTransform* pTransform = NULL,
		                    IImageWriter* pDst=NULL) :
		CTransformGraphNode(pTransform, pDst)
    {}

	HRESULT SetSourceCount(UINT uCnt)
	{ return m_vecSources.resize(uCnt); }

	HRESULT BindSourceToReader(UINT uIndex, IImageReader* pSource,
							   const IMAGE_EXTEND* pex = NULL, int level = 0)
	{
	    VT_ASSERT( uIndex < GetSourceCount() );
		SOURCE_DESC& source = m_vecSources[uIndex];
		return source.SetReader(pSource, pex, level);
	}

	HRESULT BindSourcesToIndexedReader(UINT uIndex, IIndexedImageReader* pSource,
								       UINT uReaderIndex, UINT uReaderCnt,
									   const IMAGE_EXTEND* pex = NULL,
                                       int level = 0)
	{
		HRESULT hr = S_OK;
	    VT_ASSERT( uIndex + uReaderCnt <= GetSourceCount() );
	    VT_ASSERT( uReaderIndex + uReaderCnt <= pSource->GetFrameCount() );
		for( UINT i = 0; i < uReaderCnt && hr == S_OK; i++ )
		{
            SOURCE_DESC& source = m_vecSources[uIndex+i];
            hr = source.SetIndexedReader(pSource, uReaderIndex+i, pex, level);  
		}
		return hr;
	}

	HRESULT BindSourceToTransform(UINT uIndex, CTransformGraphNode* pNode)
	{
	    VT_ASSERT( uIndex < GetSourceCount() );
		m_vecSources[uIndex].SetTransform(pNode);
		return S_OK;
	}

protected:
	vt::vector<SOURCE_DESC> m_vecSources;
};

//+-----------------------------------------------------------------------------
//
// Function: PushTransformTask
// 
// Synopsis: Executes the specified transform graph. Will potnetially issue the
//           chain on multiple concurrent threads.
// 
//------------------------------------------------------------------------------
struct VT_TRANSFORM_TASK_OPTIONS
{
	DWORD maxthreads;
	bool  bPrefetch;
	ITaskWorkIdSequencer* pSeq;

	VT_TRANSFORM_TASK_OPTIONS() : maxthreads(0), bPrefetch(true), pSeq(NULL)
	{}
};


HRESULT	PushTransformTask(const CTransformGraphNode*const* ppGraphHeadList,
						  UINT  uHeadCount,
						  CTaskStatus* pStatus,
						  const VT_TRANSFORM_TASK_OPTIONS* pOpts = NULL);

inline HRESULT	PushTransformTask(const CTransformGraphNode* pGraphHead, 
								  CTaskStatus* pStatus,
								  const VT_TRANSFORM_TASK_OPTIONS* pOpts = NULL)
{ return PushTransformTask(&pGraphHead, 1, pStatus, pOpts); }


HRESULT	PushTransformTaskAndWait(const CTransformGraphNode*const* ppGraphHeadList, 
								 UINT  uHeadCount,
								 CTaskStatus* pStatus,
								 const VT_TRANSFORM_TASK_OPTIONS* pOpts = NULL);

HRESULT PushTransformTaskAndWait(const CTransformGraphNode*const* ppGraphHeads, 
								 UINT uHeadCount, 
								 CTaskProgress* pUserProgress = NULL, 
								 const VT_TRANSFORM_TASK_OPTIONS* pOpts = NULL);

inline HRESULT	PushTransformTaskAndWait(const CTransformGraphNode* pGraphHead, 
										 CTaskStatus* pStatus,
										 const VT_TRANSFORM_TASK_OPTIONS* pOpts = NULL)
{ return PushTransformTaskAndWait(&pGraphHead, 1, pStatus, pOpts); }

inline HRESULT	PushTransformTaskAndWait(const CTransformGraphNode* pGraphHead, 
										 CTaskProgress* pUserProgress = NULL,
										 const VT_TRANSFORM_TASK_OPTIONS* pOpts = NULL)
{ return PushTransformTaskAndWait(&pGraphHead, 1, pUserProgress, pOpts); }

//+-----------------------------------------------------------------------------
//
// Class: CImageTransformUnaryPoint
// 
// Synopsis: a unary image transform that needs as input only the exactly
//           corresponding rect in the source
// 
//------------------------------------------------------------------------------
template <class Derived, bool bRequiresCloneForConcurrency>
class CImageTransformUnaryPoint : public IImageTransform
{
	// IImageTransform implementation
public:
	virtual bool RequiresCloneForConcurrency()
	{ return bRequiresCloneForConcurrency;	}

	// by default unary point transforms don't change the pixel type or 
	// dimensions, so just copy  
	virtual void    GetSrcPixFormat(IN OUT int* ptypeSrcs, 
									IN UINT  /*uSrcCnt*/,
									IN int typeDst)
	{ ptypeSrcs[0] = typeDst; }

	virtual void    GetDstPixFormat(OUT int& typeDst,
									IN  const int* ptypeSrcs, 
									IN  UINT  /*uSrcCnt*/)
	{ typeDst = ptypeSrcs[0]; }

	virtual HRESULT GetRequiredSrcRect(OUT TRANSFORM_SOURCE_DESC* pSrcReq,
									  OUT UINT& uSrcReqCount,
									  IN  UINT  /*uSrcCnt*/,
									  IN  const CRect& rctLayerDst
									  )
	{
		pSrcReq->bCanOverWrite = true;
		pSrcReq->rctSrc        = rctLayerDst;
		pSrcReq->uSrcIndex     = 0;
		uSrcReqCount           = 1;
		return S_OK;
	}

	virtual HRESULT GetAffectedDstRect(OUT CRect& rctDst,
									  IN  const CRect& rctSrc,
									  IN  UINT /*uSrcIndex*/,
									  IN  UINT /*uSrcCnt*/)
	{
		rctDst = rctSrc;
		return S_OK;
	}

	virtual HRESULT GetResultingDstRect(OUT CRect& rctDst,
	                                    IN  const CRect& rctSrc,
	                                    IN  UINT /*uSrcIndex*/,
										IN  UINT /*uSrcCnt*/)
	{
		rctDst = rctSrc;
		return S_OK;
	}

	virtual HRESULT Transform(OUT CImg* pimgDstRegion, 
							  IN  const CRect& rctLayerDst,
							  IN  CImg *const *ppimgSrcRegions,
							  IN  const TRANSFORM_SOURCE_DESC* /*pSrcDesc*/,
							  IN  UINT  /*uSrcCnt*/
							  )
	{ 
		return ((Derived*)this)->Transform(pimgDstRegion, rctLayerDst,
										   *ppimgSrcRegions[0]);
	}
};

//+-----------------------------------------------------------------------------
//
// Class: CImageTransformPoint
// 
// Synopsis: a multi-source image transform that requires only exactly the 
//           corresponding rect in each of the sources
// 
//------------------------------------------------------------------------------
template <class Derived, bool bRequiresCloneForConcurrency>
class CImageTransformPoint : public IImageTransform
{
	// IImageTransform implementation
public:
	virtual bool RequiresCloneForConcurrency()
	{ return bRequiresCloneForConcurrency;	}

	virtual void    GetSrcPixFormat(_In_reads_(uSrcCnt) int* ptypeSrcs, 
									IN UINT  uSrcCnt,
									IN int typeDst)
	{ 
		for( UINT i = 0; i < uSrcCnt; i++ )
		{
			ptypeSrcs[i] = typeDst; 
		}
    }

	// by default this transform uses the type of the first source as the 
	// output type
	virtual void    GetDstPixFormat(OUT int& typeDst,
									IN  const int* ptypeSrcs, 
									IN  UINT  /*uSrcCnt*/)
	{ typeDst = ptypeSrcs[0]; }

	virtual HRESULT GetRequiredSrcRect(OUT TRANSFORM_SOURCE_DESC* pSrcReq,
									  OUT UINT& uSrcReqCount,
									  IN  UINT  uSrcCnt,
									  IN  const CRect& rctLayerDst
									  )
	{
		for( UINT i = 0; i < uSrcCnt; i++, pSrcReq++ )
		{
			pSrcReq->bCanOverWrite = true;
			pSrcReq->rctSrc        = rctLayerDst;
			pSrcReq->uSrcIndex     = i;
		}
		uSrcReqCount = uSrcCnt;;
		return S_OK;
	}

	virtual HRESULT GetAffectedDstRect(OUT CRect& rctDst,
									  IN  const CRect& rctSrc,
									  IN  UINT /*uSrcIndex*/,
									  IN  UINT /*uSrcCnt*/)
	{
		rctDst = rctSrc;
		return S_OK;
	}

	virtual HRESULT GetResultingDstRect(OUT CRect& rctDst,
	                                    IN  const CRect& rctSrc,
	                                    IN  UINT /*uSrcIndex*/,
										IN  UINT /*uSrcCnt*/)
	{
		rctDst = rctSrc;
		return S_OK;
	}
		
	virtual HRESULT Transform(OUT CImg* pimgDstRegion, 
							  IN  const CRect& rctLayerDst,
							  IN  CImg *const *ppimgSrcRegions,
							  IN  const TRANSFORM_SOURCE_DESC* /*pSrcDesc*/,
							  IN  UINT  uSrcCnt
							  )
	{
		 return ((Derived*)this)->Transform(pimgDstRegion, rctLayerDst,
											ppimgSrcRegions, uSrcCnt); 
	}
};

//+-----------------------------------------------------------------------------
//
// Class: CImageTransformUnaryGeo
// 
// Synopsis: a Geo(metric) image transform that has a single input
// 
//------------------------------------------------------------------------------
template <class Derived, bool bRequiresCloneForConcurrency>
class CImageTransformUnaryGeo : public IImageTransform
{
	// IImageTransform implementation
public:
	virtual bool RequiresCloneForConcurrency()
	{ return bRequiresCloneForConcurrency;	}

	// by default we use the pixel format of the dest
	virtual void    GetSrcPixFormat(IN OUT int* ptypeSrcs, 
									IN UINT  /*uSrcCnt*/,
									IN int typeDst)
	{ ptypeSrcs[0] = typeDst; }

	virtual void    GetDstPixFormat(OUT int& typeDst,
									IN  const int* ptypeSrcs, 
									IN  UINT  /*uSrcCnt*/)
	{ typeDst = ptypeSrcs[0]; }

	virtual HRESULT GetRequiredSrcRect(OUT TRANSFORM_SOURCE_DESC* pSrcReq,
									  OUT UINT& uSrcReqCount,
									  IN  UINT  /*uSrcCnt*/,
									  IN  const CRect& rctLayerDst
									  )
	{
        if (!pSrcReq) { uSrcReqCount = 0; return S_OK; }
		pSrcReq->bCanOverWrite = false;
		pSrcReq->rctSrc = ((Derived*)this)->GetRequiredSrcRect(rctLayerDst);
		pSrcReq->uSrcIndex     = 0;
		uSrcReqCount           = 1;
		return S_OK;
	}

	virtual HRESULT GetAffectedDstRect(OUT CRect& rctDst,
									  IN  const CRect& rctSrc,
									  IN  UINT /*uSrcIndex*/,
									  IN  UINT /*uSrcCnt*/)
	{
		rctDst = ((Derived*)this)->GetAffectedDstRect(rctSrc);
		return S_OK;
	}

	virtual HRESULT GetResultingDstRect(OUT CRect& rctDst,
	                                    IN  const CRect& rctSrc,
	                                    IN  UINT /*uSrcIndex*/,
										IN  UINT /*uSrcCnt*/)
	{		
		rctDst = ((Derived*)this)->GetResultingDstRect(rctSrc);
		return S_OK;
	}

	virtual HRESULT Transform(OUT CImg* pimgDstRegion, 
							  IN  const CRect& rctLayerDst,
							  IN  CImg *const *ppimgSrcRegions,
							  IN  const TRANSFORM_SOURCE_DESC* pSrcDesc,
							  IN  UINT  /*uSrcCnt*/
							  )
	{ 
		return ((Derived*)this)->Transform(pimgDstRegion, rctLayerDst,
										   ppimgSrcRegions?(*ppimgSrcRegions[0]):(m_CImg), 
										   pSrcDesc?(pSrcDesc[0].rctSrc.TopLeft()):(vt::CPoint(0,0)));
	}
private:
    CImg m_CImg;
};

};
