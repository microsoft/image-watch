//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Interface to apply an operation (filter, saturation, brightness, etc)
//      to an image.
//
//  History:
//      2009/02/10 - mattu
//          Created
//
//------------------------------------------------------------------------
#include "stdafx.h"

#include "vt_pad.h"
#include "vt_transform.h"

using namespace vt;

class CTransformTaskTraverse;

// commonly occuring error pattern where the task needs to return before
// it was successfully submitted.  in this case we still have to signal 
// done.
#define TRNSFRM_PTR_OOM_RET(ptr, pStat)\
    if( ptr == NULL ) {\
		if( pStat ) {\
			pStat->SetTaskError(E_OUTOFMEMORY);\
		    pStat->SetDone();\
		}\
		return E_OUTOFMEMORY;\
	}\

#define TRNSFRM_HR_RET(call, pStat)\
	if( (hr = call) != S_OK ) {\
		if( pStat ) {\
			pStat->SetTaskError(hr);\
		    pStat->SetDone();\
		}\
		return hr;\
	}\

// forward declaration of internal function in taskmanager.cpp
extern HRESULT
PushTaskIntrnl(void* callback, void* pArg, ITaskState* pState,
			   CTaskStatus* pStatus, LONG count, ITaskWorkIdSequencer* pSeq,
			   bool bWait, const VT_TASK_OPTIONS* pOpts);

//+-----------------------------------------------------------------------------
//
// Class: CRasterBlockMap
// 
//------------------------------------------------------------------------------
void
CRasterBlockMap::Initialize(const BLOCKITER_INIT& bii, 
							int iBlkCntXInMaj, int iBlkCntYInMaj,
							int level)
{
	m_bii = bii;

	m_iLevel = level;

	m_iBlkCntXInMaj = iBlkCntXInMaj;
	m_iBlkCntYInMaj = iBlkCntYInMaj;

	m_iBlkCntX = (m_bii.region.Width()+m_bii.iBlockWidth-1)   / m_bii.iBlockWidth;
	m_iBlkCntY = (m_bii.region.Height()+m_bii.iBlockHeight-1) / m_bii.iBlockHeight;
	m_iTotalBlkCnt = m_iBlkCntX*m_iBlkCntY;
	   
	m_iMajCntX = (m_iBlkCntX+iBlkCntXInMaj-1) / m_iBlkCntXInMaj;
	m_iMajCntY = (m_iBlkCntY+iBlkCntYInMaj-1) / m_iBlkCntYInMaj;

	m_iBlkCntInMajRow = m_iBlkCntX * m_iBlkCntYInMaj;
	m_iBlkCntXInLastMajCol = m_iBlkCntX-(m_iMajCntX-1)*iBlkCntXInMaj;
}

HRESULT
CRasterBlockMap::GetBlockItems(CRect& rct, int& iPrefetchCount, int iBlockIndex)
{
	if( iBlockIndex >= GetTotalBlocks() )
	{
		rct = vt::CRect(0,0,0,0);
		iPrefetchCount = 0;
		return S_OK;
	}

	int iMajRow = iBlockIndex / m_iBlkCntInMajRow;

	int iBlkInMajRow = iBlockIndex - iMajRow * m_iBlkCntInMajRow;

	int iBlkCntInMaj = ( iMajRow < m_iMajCntY-1 )?  
	    m_iBlkCntXInMaj * m_iBlkCntYInMaj: 
		m_iBlkCntXInMaj * (m_iBlkCntY-iMajRow*m_iBlkCntYInMaj);   
	int iMajCol   = iBlkInMajRow / iBlkCntInMaj;
	int iBlkInMaj = iBlkInMajRow - iMajCol*iBlkCntInMaj;

	int iBlkCntXInMaj = (iMajCol < m_iMajCntX-1)?
		m_iBlkCntXInMaj: m_iBlkCntXInLastMajCol;
	int iBlkInMajY = iBlkInMaj / iBlkCntXInMaj;
	int iBlkInMajX = iBlkInMaj - iBlkInMajY*iBlkCntXInMaj;

	int l = (iMajCol*m_iBlkCntXInMaj+iBlkInMajX)*m_bii.iBlockWidth + 
		m_bii.region.left;
	int t = (iMajRow*m_iBlkCntYInMaj+iBlkInMajY)*m_bii.iBlockHeight +
		m_bii.region.top;
	rct = vt::CRect(l,t,l+m_bii.iBlockWidth,t+m_bii.iBlockHeight) & m_bii.region;
	rct = LayerRectAtLevel(rct, m_iLevel);

	iPrefetchCount = (iBlkInMajX==0 && iBlkInMajY==0 &&
					  (iMajCol!=m_iMajCntX-1 || iMajRow!=m_iMajCntY-1))? 1: 0;

	return S_OK;
}

CRect
CRasterBlockMap::GetPrefetchRect(int iBlockIndex, int iPrefetchIndex)
{
	UNREFERENCED_PARAMETER(iPrefetchIndex);

	VT_ASSERT( iBlockIndex < GetTotalBlocks() );
	VT_ASSERT( iPrefetchIndex == 0 );

	// compute maj row/col for this block
	int iMajRow = iBlockIndex / m_iBlkCntInMajRow;
	int iBlkInMajRow = iBlockIndex - iMajRow * m_iBlkCntInMajRow;
	int iBlkCntInMaj = ( iMajRow < m_iMajCntY-1 )?  
	    m_iBlkCntXInMaj * m_iBlkCntYInMaj: 
		m_iBlkCntXInMaj * (m_iBlkCntY-iMajRow*m_iBlkCntYInMaj);   
	int iMajCol = iBlkInMajRow / iBlkCntInMaj;

	// pre-fetch the next major row/col
	int iPreRow;
	int iPreCol = iMajCol+1;
	if( iPreCol == m_iMajCntX )
	{
		iPreCol = 0;
		iPreRow = iMajRow + 1;  
	}
	else
	{
		iPreRow = iMajRow;
	}

	vt::CRect rct;
	if( iPreRow>=m_iMajCntY )
	{
		rct = vt::CRect(0,0,0,0);
	}
	else
	{
		// prefetch an entire major block
		rct.left   = iPreCol*m_iBlkCntXInMaj*m_bii.iBlockWidth +
		    m_bii.region.left;
		rct.top    = iPreRow*m_iBlkCntYInMaj*m_bii.iBlockHeight +
		    m_bii.region.top;
		rct.right  = rct.left+m_iBlkCntXInMaj*m_bii.iBlockWidth;
		rct.bottom = rct.top+m_iBlkCntYInMaj*m_bii.iBlockHeight;
		rct &= m_bii.region;
	}
	return LayerRectAtLevel(rct, m_iLevel);
}

//+-----------------------------------------------------------------------------
//
// Class: CTransformTaskStatus
// 
// Synopsis: Implementation of CTaskStatus specific for CTransformTaskTraverse
//           This class is responsible for: 
//           1) collecting progress reports from the multi-threaded 
//              CTransformTaskTraverse
//           2) forwarding cancel/progress reports to the application provided
//              callback
//           3) deleting the task on SetDone()
// 
//------------------------------------------------------------------------------
class CTransformTaskStatus: public CTaskStatus
{
public:
	CTransformTaskStatus() : m_hr(S_OK)
	{ }

	void Initialize(CTransformTaskTraverse* pTask, 
					int  iBlkCnt,
					ITransformBlockMap* pDefBlockMap,
					bool bDoPrefetch,
					CTaskStatus*   pStatus,
					CTaskProgress* pProgress);

	// CTaskStatus implementation
	bool  GetCancel()
	{ return m_pUserProgress? m_pUserProgress->GetCancel(): false; }

	void  ReportProgress(float fPct)
	{ 
		if( m_pUserProgress )
		{
			m_pUserProgress->ReportProgress(fPct);
		}
	}

    // implemented below after definition of CTransformTaskTraverse
    HRESULT SetDone();

	void SetTaskError(HRESULT hr)
	{ 
		if(	m_pUserStatus )
		{
			m_pUserStatus->SetTaskError(hr);
			m_hr = m_pUserStatus->GetTaskError();
		}
		else if( m_hr == S_OK )
		{
			m_hr = hr;
		}
	}

	HRESULT GetTaskError()
	{ return m_pUserStatus? m_pUserStatus->GetTaskError(): m_hr; }

	// Progress Compute
	void BlockComplete(CTaskProgress* pProgress)
	{
		// TODO: 1) what should the report granularity be - below is every 
		//          4 blocks, perhaps this should be user configurable
		//       2) m_pUserStatus will now get hit from multiple threads, 
		//          the risk here is that status can get reported out-of-order
		//          this should be filtered somewhere into a single reporting thread
		//          ideally the framework would provide this functionality so
		//          that apps don't have to.
		LONG iCount = InterlockedIncrement(&m_iCompletedBlks);
		if( (iCount&0x3) == 0 )
		{
			pProgress->ReportProgress(float(iCount)*m_fProgressScale);
		}
	}

protected:
	bool  m_bDoPrefetch;
    float m_fProgressScale;
	CTransformTaskTraverse* m_pTask;
	ITransformBlockMap* m_pDefBlockMap;
	CTaskStatus*   m_pUserStatus;
	CTaskProgress* m_pUserProgress;
	LONG         m_iCompletedBlks;
	HRESULT      m_hr;
};

void
CTransformTaskStatus::Initialize(CTransformTaskTraverse* pTask,
								 int  iBlkCnt,
								 ITransformBlockMap* pDefBlockMap,
								 bool bDoPrefetch,
								 CTaskStatus* pStatus,
								 CTaskProgress* pProgress)
{
	m_hr             = S_OK;
	m_pTask          = pTask;
	m_pUserStatus    = pStatus;
	m_pUserProgress  = pProgress;
	m_iCompletedBlks = 0;
	m_bDoPrefetch    = bDoPrefetch;
	m_pDefBlockMap   = pDefBlockMap;
	m_fProgressScale = 100.f / float(iBlkCnt);
}

//+-----------------------------------------------------------------------------
//
// Class: CTransformTaskTraverse
// 
// Synopsis: traverses a transform graph - 
//           only used internally in PushTransformTask
// 
//------------------------------------------------------------------------------
class CTransformTaskTraverse: public ITaskState
{
public:
	CTransformTaskTraverse()
	{}

	~CTransformTaskTraverse()
	{ Deallocate(); }
	
	// CTransformTaskTraverse specific methods
public:
	HRESULT PushTask(const CTransformGraphNode*const* ppGraphHeads, 
					 int iHeadCount, 
					 CTaskStatus*   pUserStatus, 
					 CTaskProgress* pUserProgress, 
					 bool bWait,
					 const VT_TRANSFORM_TASK_OPTIONS* pOpts);

	// ITaskState
public:
	virtual bool RequiresCloneForConcurrency()
	{ return true;	}

	virtual HRESULT Clone(ITaskState** ppClone);

	virtual HRESULT Merge(const ITaskState* pClone);

protected: 
	struct IMG_POOL_EL
	{
		void*     pMemory;
		void*     pMemoryAlloc;
		size_t    uMemorySize;  
		UINT      uRefCount;

		IMG_POOL_EL() : pMemory(NULL), pMemoryAlloc(NULL), uMemorySize(0), 
			uRefCount(0)
		{}
	};

	// data initialized once per transform 
	// PERF: The elements (*) below are not required to be cloned.  An 
	//       optimization may be to share these across all clones.  This
	//       may be especially interesting for vecSource as this is a fair
	//       amount of data for nodes with many sources
	struct GRAPH_HEAD
	{
		UINT             uNodeIndex;     // (*)
		int              iNodeSrcIndex;  // (*)
		NODE_DEST_PARAMS writer;         // (*)
		vt::CPoint       ptWr;           // (*)

		vt::CRect rct;
		int iPrefetchCount;
	};
	struct GRAPH_NODE
	{
		IImageTransform*   pTransform;
		bool               bClonedTransform;
		int                iGraphHeadIndex;
		int                iTypeDst;       // (*)
		vt::vector<int>    vecTypeSrc;     // (*)
		vt::vector<UINT>   vecSourceIndex; // (*)

		// data updated on every block run through the transform, this needs to 
		// be stored in each node instead of in CTransformTaskTraverse because
		// it is filled in before child nodes are traversed.
		vt::vector<TRANSFORM_SOURCE_DESC> vecTSR;
		UINT uTransSrcCnt;

		GRAPH_NODE() : pTransform(NULL), bClonedTransform(false), iGraphHeadIndex(-1), 
			iTypeDst(OBJ_UNDEFINED)
		{}  
	};
	struct GRAPH_NODE_SOURCE
	{
		enum eType
		{
			eNode   = 0,
			eReader = 1,
            eIndexedReader = 2
		};

		eType type;
		union
		{   
			UINT                 uNodeIndex;
			IImageReader*        pReader;
			IIndexedImageReader* pIndexedReader;
		};
		// image reader only data
        int           level;
        int           idx;
		IMAGE_EXTEND  exReader;
		CLayerImgInfo infoReader;

		// runtime info on tiles allocated for this source
		UINT         uRefCount;
		vt::CRect    rct;
		short        iPoolId;
		bool         bGenerated;
	};

protected:
    void Deallocate();

	HRESULT InitializeGraph(OUT CRasterBlockMap** ppDefBlockMap,
							OUT int& iBlkCnt, 
							const CTransformGraphNode *pGraph,
							int iNodeSrcIndex);
	HRESULT InitializeNode(GRAPH_NODE& node, int iGraphHeadIndex,
						   IImageTransform* pTrans, bool bClone, UINT uSrcCount);
	HRESULT InitializeNodeSrc(GRAPH_NODE_SOURCE& src, 
							  const IMAGEREADER_SOURCE* pSrc);
	void InitializeNodeSrc(GRAPH_NODE_SOURCE& src, UINT uNodeIndex);

	// methods called once per task
	HRESULT CreateGraph(CRasterBlockMap** ppDefBlockMap, OUT int& iBlkCnt,
						const CTransformGraphNode*const* ppGraphHeads,
						UINT uHeadCount);
    void ResolveGraphEdges(GRAPH_NODE* pNode, int typeDst, bool bNoDst=false);

	// methods called per tile
	static HRESULT TaskCallback(ITaskState* pArg, LONG i, CTaskProgress* pProg)
	{ return ((CTransformTaskTraverse*)pArg)->TaskCallbackM(i, pProg); }

	HRESULT TaskCallbackM( UINT iTileId, CTaskProgress* pStatus );

	HRESULT ResolveActiveNodes(GRAPH_NODE* pNode, CRect rctOut, 
							   bool  bNodeVisited, int iTileId);
	HRESULT ExecuteGraph( GRAPH_NODE_SOURCE* pDst, GRAPH_NODE* pNode,
						  vt::CRect rctOut, int iTileId );
	HRESULT BeginTransform( OUT short* pDstPoolId, 
							OUT GRAPH_NODE_SOURCE* pDstSrc,
							const GRAPH_NODE* pNode, const vt::CRect& rctDst );
	HRESULT AllocTransformMemory( OUT UINT& uPoolId, const CRect& rct,
								  int iType, bool bCloseFitOnly );

protected:
	CTransformTaskStatus*         m_pStatus;
	vt::vector<GRAPH_HEAD>        m_vecHeadNodes;
	vt::vector<GRAPH_NODE>        m_vecNodes;
	vt::vector<GRAPH_NODE_SOURCE> m_vecNodeSources;
	vt::vector<IMG_POOL_EL>       m_vecImgMemPool;
	vt::vector<CImg>              m_vecImgPool;
	vt::vector<const CImg*>       m_vecImgSrcPtrPool;
};

HRESULT
CTransformTaskTraverse::PushTask(const CTransformGraphNode*const* ppGraphHeads, 
								 int iHeadCount,
								 CTaskStatus*   pUserStatus, 
								 CTaskProgress* pUserProgress, 
								 bool bWait,
								 const VT_TRANSFORM_TASK_OPTIONS* pOpts)
{
	// TODO: if a ITaskWorkIdSequencer is provided then get the default
	// block count from it

	// build the internal graph representation from the provided graph
	int iBlkCnt = -1;
	CRasterBlockMap* pDefBlockMap = NULL;
    HRESULT hr = CreateGraph(&pDefBlockMap, iBlkCnt, ppGraphHeads, iHeadCount);
	if( hr != S_OK )
	{
		delete pDefBlockMap;
		delete this;
		TRNSFRM_HR_RET(hr, pUserStatus);
	}

	// check if there is any work to do
	if( iBlkCnt <= 0 )
	{
		delete pDefBlockMap;
		delete this;
		if( pUserProgress ) { pUserProgress->ReportProgress(100.f); }
		if( pUserStatus )   { pUserStatus->SetDone(); }
		return S_OK;
	}

	// determine the pixel formats of the edges
	for( size_t i = 0; i < m_vecHeadNodes.size(); i++ )
	{
		const GRAPH_HEAD& gh = m_vecHeadNodes[i];
		GRAPH_NODE* pHead = m_vecNodes.begin() + gh.uNodeIndex;
		ResolveGraphEdges( pHead, OBJ_UNDEFINED, gh.writer.pWriter==NULL );

		// make sure that the graph edges resolved all the way up the graph
		if( gh.writer.pWriter != NULL )
		{
			if( pHead->iTypeDst == OBJ_UNDEFINED )
			{
				delete pDefBlockMap;
				delete this;
				TRNSFRM_HR_RET(E_INVALIDARG, pUserStatus);
			}
		}
		for( size_t j = 0; j < pHead->vecTypeSrc.size(); j++ )
		{
			if( pHead->vecTypeSrc[j] == OBJ_UNDEFINED )
			{
				delete pDefBlockMap;
				delete this;
				TRNSFRM_HR_RET(E_INVALIDARG, pUserStatus);
			}
		}
	}
	
	// create default options if necessary	
	VT_TRANSFORM_TASK_OPTIONS xfrmoptsdef;
	if( !pOpts )
	{
		pOpts = &xfrmoptsdef;
	}

	// create the task status 
	m_pStatus = VT_NOTHROWNEW CTransformTaskStatus();
    if( m_pStatus == NULL )
    {
		delete pDefBlockMap;
        delete this;
		TRNSFRM_HR_RET(E_OUTOFMEMORY, pUserStatus)
	}

	// if a custom sequencer is supplied then it controls the block count
	if( pOpts->pSeq )
	{
		iBlkCnt = pOpts->pSeq->GetTotalWorkItems();
	}

	m_pStatus->Initialize(this, iBlkCnt, pDefBlockMap, pOpts->bPrefetch, 
						  pUserStatus, pUserProgress);

	// start the task
	VT_TASK_OPTIONS taskopts; 
    if( pOpts->maxthreads == 0 )
	{
		// if user specified default then limit to numproc
		SYSTEM_INFO sysinfo;
		vt::CSystem::GetSystemInfo(&sysinfo);
		taskopts.maxthreads = sysinfo.dwNumberOfProcessors;
	}
	else
	{
		// use user-specified thread count
		taskopts.maxthreads = pOpts->maxthreads;
	}

    // restrict threads to a reasonable number
    taskopts.maxthreads = VtMin(taskopts.maxthreads, (DWORD) 16);

	return PushTaskIntrnl(TaskCallback, NULL, this, m_pStatus, iBlkCnt, 
						  pOpts->pSeq, bWait, &taskopts);
}

HRESULT 
CTransformTaskTraverse::Clone(ITaskState** ppClone)
{
	CTransformTaskTraverse* pClone = VT_NOTHROWNEW CTransformTaskTraverse;
	if( pClone == NULL )
	{
		*ppClone = NULL;
		return E_OUTOFMEMORY;
	}

	VT_HR_BEGIN()

	// clone the source structs
	UINT uNodeSrcCount = (UINT)m_vecNodeSources.size();
	VT_HR_EXIT( pClone->m_vecNodeSources.resize(uNodeSrcCount) );
	for( UINT i = 0; i < uNodeSrcCount; i++ )
	{
		const GRAPH_NODE_SOURCE& nodesrcOrig = m_vecNodeSources[i];
		GRAPH_NODE_SOURCE&       nodesrcClone = pClone->m_vecNodeSources[i];
		nodesrcClone.type = nodesrcOrig.type;
		nodesrcClone.uRefCount  = 0;
		nodesrcClone.iPoolId    = -1;
		nodesrcClone.bGenerated = false;
		if( nodesrcOrig.type == GRAPH_NODE_SOURCE::eNode ) 
		{
			nodesrcClone.uNodeIndex = nodesrcOrig.uNodeIndex;
		}
		else
		{
			nodesrcClone.pReader    = nodesrcOrig.pReader;
			nodesrcClone.idx        = nodesrcOrig.idx;
			nodesrcClone.level      = nodesrcOrig.level;
			nodesrcClone.infoReader = nodesrcOrig.infoReader;
			VT_HR_EXIT( nodesrcClone.exReader.Initialize(&nodesrcOrig.exReader) );
		}
	}

	// clone the node structs
	UINT uNodeCount = (UINT)m_vecNodes.size();
	VT_HR_EXIT( pClone->m_vecNodes.resize(uNodeCount) );
	for( UINT i = 0; i < uNodeCount; i++ )
	{
		const GRAPH_NODE& nodeOrig = m_vecNodes[i];
		UINT uSrcCount = (UINT)nodeOrig.vecSourceIndex.size();

		// clone the transform
		IImageTransform* pTransSrc = nodeOrig.pTransform;
		ITaskState* pTransNode;
		bool bClone = pTransSrc->RequiresCloneForConcurrency();
		if( bClone )
		{
			VT_HR_EXIT( pTransSrc->Clone(&pTransNode) );
		}
		else
		{
			pTransNode = pTransSrc;
		}

		// TODO: move initialization of writer into CreateGraph
		GRAPH_NODE& nodeClone = pClone->m_vecNodes[i];
		VT_HR_EXIT( InitializeNode(nodeClone, nodeOrig.iGraphHeadIndex, 
								   (IImageTransform*)pTransNode, 
								   bClone, uSrcCount) );
		 	
		nodeClone.iTypeDst = nodeOrig.iTypeDst;
		memcpy(nodeClone.vecTypeSrc.begin(), nodeOrig.vecTypeSrc.begin(),
			   sizeof(int)*uSrcCount);
		memcpy(nodeClone.vecSourceIndex.begin(), nodeOrig.vecSourceIndex.begin(),
			   sizeof(UINT)*uSrcCount);
	}

	VT_HR_EXIT( pClone->m_vecHeadNodes.resize(m_vecHeadNodes.size()) );
	memcpy(pClone->m_vecHeadNodes.begin(), m_vecHeadNodes.begin(),
		   sizeof(GRAPH_HEAD)*m_vecHeadNodes.size());


	pClone->m_pStatus = m_pStatus;

	VT_HR_EXIT_LABEL()

	if( hr != S_OK )
	{
		delete pClone;
		pClone = NULL;
	}
	*ppClone = pClone;

	return hr;
}

HRESULT 
CTransformTaskTraverse::Merge(const ITaskState* pClone)
{
	VT_HR_BEGIN()

	const CTransformTaskTraverse* pCloneX = (CTransformTaskTraverse*)pClone;

	//for each transform call merge from the clone
	for( size_t i = 0; i < m_vecNodes.size(); i++ )
	{
		const GRAPH_NODE& nodeClone = pCloneX->m_vecNodes[i];
		GRAPH_NODE& nodeOrig = m_vecNodes[i];
		VT_HR_EXIT( nodeOrig.pTransform->Merge(nodeClone.pTransform) );
	}

	VT_HR_END()
}

void
CTransformTaskTraverse::Deallocate()
{
	// delete graph nodes
	for( size_t i = 0; i < m_vecNodes.size(); i++ )
	{
		GRAPH_NODE& node = m_vecNodes[i];
		if( node.bClonedTransform )
		{
			delete node.pTransform;
		}
	}
	// delete tile memory
	bool bFirst = true;
	for( size_t i = 0; i < m_vecImgMemPool.size(); i++ )
	{
		IMG_POOL_EL& el = m_vecImgMemPool[i];
		delete [] el.pMemoryAlloc;
		if( el.uMemorySize != 0 )
		{
			if( bFirst )
			{
				// VT_DEBUG_LOG("Deleting blocks for transform\n");
				bFirst = false;
			}
			// VT_DEBUG_LOG("    delete block size %08x\n", el.uMemorySize);
		}
	}
	// reset the vectors
	m_vecHeadNodes.clear();
	m_vecNodes.clear();
    m_vecNodeSources.clear();
    m_vecImgMemPool.clear();
	m_vecImgPool.clear();
	m_vecImgSrcPtrPool.clear();
}

HRESULT
CTransformTaskTraverse::CreateGraph(CRasterBlockMap** ppDefBlockMap, 
									OUT int& iBlkCnt,
									const CTransformGraphNode*const* ppGraphHeads, 
									UINT uHeadCount)
{
	VT_HR_BEGIN()

	Deallocate();
 
	// walk the graph
	for( UINT i = 0; i < uHeadCount; i++ )
	{
		VT_HR_EXIT( InitializeGraph(ppDefBlockMap, iBlkCnt, ppGraphHeads[i], -1) );
	}

	// replace the temp use of pTransform with the correct pointer
	for( size_t i = 0; i < m_vecNodes.size(); i++ )
	{
		GRAPH_NODE& n = m_vecNodes[i];
		CTransformGraphNode *pG = (CTransformGraphNode*)n.pTransform;
		n.pTransform = pG->GetTransform();
		// every node must have a transform
		VT_PTR_EXIT( n.pTransform );
	}

	VT_HR_EXIT_LABEL()

	if( hr != S_OK )
	{
		Deallocate();
	}
	return hr;
}

HRESULT
CTransformTaskTraverse::InitializeGraph(OUT CRasterBlockMap** ppDefBlockMap,
										OUT int& iBlkCnt,
										const CTransformGraphNode *pGraph,
										int iNodeSrcIndex)
{
	VT_HR_BEGIN()

	// overload the node.pTransform to store a pointer to pGraph, this is
	// used to detect multiple refs to the same node.  the pTransform will
	// be correctly set in a second pass. 
	IImageTransform* pXPlaceHolder = (IImageTransform*)pGraph;

	// handle the writer/blockmap for the current node 
	int iCurBlkCnt = -1;
	vt::CPoint ptWr(0,0);
	NODE_DEST_PARAMS nd = pGraph->GetDest();
	if( nd.pBlockMap )
	{
		iCurBlkCnt = nd.pBlockMap->GetTotalBlocks();
	}

	if( nd.pWriter ) 
	{
		vt::CRect rctWr = nd.pWriter->GetImgInfo().LayerRect();
		ptWr = rctWr.TopLeft();
		if( nd.pBlockMap == NULL && !rctWr.IsRectEmpty() )
		{
			// TODO: in this case determine optimal block sizes based on 
			//       graph functionality
			if( *ppDefBlockMap == NULL )
			{
				BLOCKITER_INIT bii(rctWr, 256, 256);
				*ppDefBlockMap = VT_NOTHROWNEW CRasterBlockMap(bii, 4, 4);
				VT_PTR_OOM_EXIT( *ppDefBlockMap );
			}
			else if( (*ppDefBlockMap)->GetInitInfo().region != rctWr )
			{
				// can't deal with different size rects and a default blockmap
				VT_HR_EXIT( E_INVALIDARG );
			}
			nd.pBlockMap = *ppDefBlockMap;
			iCurBlkCnt = (*ppDefBlockMap)->GetTotalBlocks();
		}
	}

	if( iCurBlkCnt > 0 )
	{
		iBlkCnt = (iBlkCnt == -1)? iCurBlkCnt: VtMax(iBlkCnt, iCurBlkCnt);
		
		GRAPH_HEAD hd;
		hd.uNodeIndex    = (UINT)m_vecNodes.size();
		hd.iNodeSrcIndex = iNodeSrcIndex;
		hd.writer        = nd;
		hd.ptWr          = ptWr;
		VT_HR_EXIT(m_vecHeadNodes.push_back(hd));
	}
	else if( iNodeSrcIndex < 0 )
	{
		// this case is a head node with no work to do - so don't bother
		// descending
		return S_OK;
	}

	// init the node
	int iNodeIndex = (int)m_vecNodes.size();
	VT_HR_EXIT( m_vecNodes.resize(iNodeIndex+1) );
	UINT uSrcCnt = pGraph->GetSourceCount();
	VT_HR_EXIT( InitializeNode(m_vecNodes[iNodeIndex], 
							   (iCurBlkCnt > 0)? (int)m_vecHeadNodes.size()-1: -1 , 
							   pXPlaceHolder, false, uSrcCnt) );

	// initialize the node sources
	for( UINT i = 0; i < uSrcCnt; i++ )
	{
		CTransformGraphNode::eSourceType eSrcType = pGraph->GetSourceType(i); 
		void* pSrc = (eSrcType==CTransformGraphNode::eTransformSource)?
			(void*)(pGraph->GetTransformSource(i)): 
			(void*)(pGraph->GetReaderSource(i));

		// search all previous sources for previously visited parts of the graph
		bool bCommonSrc = false;
		UINT uPrevSrcIndex = UINT32_MAX;
		for( UINT j = 0; j < (UINT)m_vecNodeSources.size() && !bCommonSrc; j++ )
		{
			GRAPH_NODE_SOURCE& prevsrc = m_vecNodeSources[j];

			// note: see above about overloaded use of pTransform
			if( (prevsrc.type != GRAPH_NODE_SOURCE::eNode && 
				 prevsrc.pReader  == ((IMAGEREADER_SOURCE*)pSrc)->pReader &&
                 prevsrc.exReader == ((IMAGEREADER_SOURCE*)pSrc)->ex      &&
                 prevsrc.idx      == ((IMAGEREADER_SOURCE*)pSrc)->idx     &&
                 prevsrc.level    == ((IMAGEREADER_SOURCE*)pSrc)->level) ||
				(prevsrc.type == GRAPH_NODE_SOURCE::eNode && 
				 m_vecNodes[prevsrc.uNodeIndex].pTransform == pSrc) )
			{
				// found a common source
				bCommonSrc     = true;
				uPrevSrcIndex  = j;
			}
		}

		VT_ASSERT(bCommonSrc == false || uPrevSrcIndex != UINT32_MAX);
  
		if( bCommonSrc )
		{
			m_vecNodes[iNodeIndex].vecSourceIndex[i] = uPrevSrcIndex;
		}
		else
		{ 
			int iNextNodeSrcIndex = (int)m_vecNodeSources.size();
			m_vecNodes[iNodeIndex].vecSourceIndex[i] = iNextNodeSrcIndex;
			VT_HR_EXIT( m_vecNodeSources.resize(iNextNodeSrcIndex+1) );
			GRAPH_NODE_SOURCE& src = m_vecNodeSources.back();
			if( eSrcType == CTransformGraphNode::eTransformSource )
			{
				InitializeNodeSrc(src, (int)m_vecNodes.size());
				VT_HR_EXIT( InitializeGraph(ppDefBlockMap, iBlkCnt,
											(CTransformGraphNode*)pSrc,
							                iNextNodeSrcIndex) );
			}
			else
			{
				VT_HR_EXIT( InitializeNodeSrc(src, (IMAGEREADER_SOURCE*)pSrc) );
			}
		}
	}

	VT_HR_END()
}

HRESULT
CTransformTaskTraverse::InitializeNode(GRAPH_NODE& node,
									   int iGraphHeadIndex,
									   IImageTransform* pT, bool bClone, 
									   UINT uSrcCount)
{
	VT_HR_BEGIN()

	VT_HR_EXIT( node.vecTypeSrc.resize(uSrcCount) );
	VT_HR_EXIT( node.vecSourceIndex.resize(uSrcCount) );
	VT_HR_EXIT( node.vecTSR.resize(uSrcCount) );
	node.pTransform       = pT;
	node.bClonedTransform = bClone;
	node.iGraphHeadIndex  = iGraphHeadIndex;
	VT_HR_END()
}

HRESULT
CTransformTaskTraverse::InitializeNodeSrc(OUT GRAPH_NODE_SOURCE& src, 
										  const IMAGEREADER_SOURCE* pRdSrc)
{
    VT_HR_BEGIN()

	src.iPoolId    = -1;
	src.bGenerated = false;
	src.idx     = pRdSrc->idx;
    src.level   = pRdSrc->level;
    if( pRdSrc->type == IMAGEREADER_SOURCE::eReader )
    {
        src.type = GRAPH_NODE_SOURCE::eReader;
        src.pReader = pRdSrc->pReader;
        src.infoReader = src.pReader->GetImgInfo(src.level);
    }
    else
    {
        src.type = GRAPH_NODE_SOURCE::eIndexedReader;
        src.pIndexedReader = pRdSrc->pIndexedReader;
        src.infoReader     = src.pIndexedReader->GetImgInfo(src.idx, src.level);
    }

	VT_HR_EXIT( src.exReader.Initialize(&pRdSrc->ex) );

    VT_HR_END()
}

void
CTransformTaskTraverse::InitializeNodeSrc(OUT GRAPH_NODE_SOURCE& src,
										  UINT uNodeIndex)
{
	src.iPoolId    = -1;
	src.bGenerated = false;
	src.type       = GRAPH_NODE_SOURCE::eNode;
	src.uNodeIndex = uNodeIndex;
}

//+-----------------------------------------------------------------------------
//
// Function: ResolveGraphEdges
// 
// Synopsis: compute the pixel formats along the graph edges (in other words
//           the image types passed between the graph nodes)
// 
//------------------------------------------------------------------------------
void
CTransformTaskTraverse::ResolveGraphEdges(GRAPH_NODE* pNode, int iTypeDst,
										  bool bNoDst)
{
	// if this node has already been resolved then return right away
	if( pNode->iTypeDst != OBJ_UNDEFINED )
	{
		if( iTypeDst != OBJ_UNDEFINED && pNode->iTypeDst != iTypeDst )
		{
			// if pixel format conflicts then return undefined
			pNode->iTypeDst = OBJ_UNDEFINED;
		}
		return;
	}

	// Descend graph. if GetSrcPixFormat for the provided typeDst resolves 
	// to a fixed source pixel type then pin this source type and propogate the 
	// requirement down the graph
	UINT uSrcCnt = (UINT)pNode->vecSourceIndex.size();
    int* piTypeSrc = pNode->vecTypeSrc.begin();
	pNode->pTransform->GetSrcPixFormat(piTypeSrc, uSrcCnt, iTypeDst);
	for( UINT i = 0; i < uSrcCnt; i++, piTypeSrc++ )
	{
		const GRAPH_NODE_SOURCE& src = m_vecNodeSources[pNode->vecSourceIndex[i]];
        if( src.type == GRAPH_NODE_SOURCE::eNode )
		{
			GRAPH_NODE* pChild = m_vecNodes.begin()+src.uNodeIndex; 
			ResolveGraphEdges(pChild, *piTypeSrc);

			// if the child type couldn't be resolved then propogate error
			if( pChild->iTypeDst == OBJ_UNDEFINED )
			{
				pNode->iTypeDst = OBJ_UNDEFINED;
				return;
			}
            *piTypeSrc = pChild->iTypeDst;
		}
        else
		{				
			VT_ASSERT( src.type == GRAPH_NODE_SOURCE::eReader ||
                       src.type == GRAPH_NODE_SOURCE::eIndexedReader );
			CLayerImgInfo infoR = (src.type == GRAPH_NODE_SOURCE::eReader)? 
                src.pReader->GetImgInfo(src.level): 
                src.pIndexedReader->GetImgInfo(src.idx, src.level);
			int iReaderType = infoR.type;

			// special case half-float to appear as float for processing
			if( EL_FORMAT(iReaderType) == EL_FORMAT_HALF_FLOAT )
			{
				int iReaderBands = VT_IMG_BANDS(iReaderType);
				iReaderType = VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT(iReaderType), 
													EL_FORMAT_FLOAT, iReaderBands);
			}

			*piTypeSrc = VT_IMG_FIXED_PIXFRMT_MASK|VT_IMG_FIXED_ELFRMT_MASK|
				UpdateMutableTypeFields(*piTypeSrc, iReaderType);
		}
	}

	// Ascend graph.
	if( !bNoDst )
	{
		pNode->pTransform->GetDstPixFormat(
			pNode->iTypeDst, pNode->vecTypeSrc.begin(), uSrcCnt);
	
		// TODO: could imagine a mutable dest field but it doesn't come up
		//       very often 
		VT_ASSERT( (pNode->iTypeDst & (VT_IMG_FIXED_PIXFRMT_MASK|VT_IMG_FIXED_ELFRMT_MASK)) ==
				   (VT_IMG_FIXED_PIXFRMT_MASK|VT_IMG_FIXED_ELFRMT_MASK) ); 

		// if GetDstInfo returns undefined or if the typeDst is already defined and 
		// it disagrees with returned value then fail
		if( pNode->iTypeDst == OBJ_UNDEFINED || 
		   ((iTypeDst & VT_IMG_FIXED_ELFRMT_MASK)  && !VT_IMG_SAME_E(iTypeDst, pNode->iTypeDst)) ||
		   ((iTypeDst & VT_IMG_FIXED_PIXFRMT_MASK) && !VT_IMG_SAME_PB(iTypeDst, pNode->iTypeDst)) ) 
		{
			VT_ASSERT(!"Incompatible image types");
			pNode->iTypeDst = OBJ_UNDEFINED;
		}
	}
	else
	{
		pNode->iTypeDst = OBJ_UNDEFINED;
	}

	return;
}

HRESULT
CTransformTaskTraverse::TaskCallbackM(UINT iTileId, CTaskProgress* pTaskProg)
{  
	VT_HR_BEGIN()

	// determine what work to do
	int iPrefetchCount = 0;
	for( size_t i = 0; i < m_vecHeadNodes.size(); i++ )
	{
		GRAPH_HEAD& gh = m_vecHeadNodes[i];

		// every head node should have a blockmap
		VT_ASSERT( gh.writer.pBlockMap );

		VT_HR_EXIT( gh.writer.pBlockMap->GetBlockItems(gh.rct, gh.iPrefetchCount, 
													   iTileId) );
		iPrefetchCount = VtMax(iPrefetchCount, gh.iPrefetchCount);
	}

	// run prefetch if necessary
    for( int i = 0; i < iPrefetchCount; i++ )
	{
		// evaluate the graph to determine what to regions to prefetch
		for( size_t j = 0; j < m_vecHeadNodes.size(); j++ )
		{
			GRAPH_HEAD& gh = m_vecHeadNodes[j];
			if( gh.iPrefetchCount < i+1 )
			{
				continue;
			}

			vt::CRect rctPre = gh.writer.pBlockMap->GetPrefetchRect(iTileId, i);
			if( !rctPre.IsRectEmpty() )
			{
				GRAPH_NODE* pHead = m_vecNodes.begin() + gh.uNodeIndex;   
				if( gh.iNodeSrcIndex < 0 )
				{
					VT_HR_EXIT( ResolveActiveNodes(pHead, rctPre, false, iTileId) );
				}
				else 
				{
					GRAPH_NODE_SOURCE& s = m_vecNodeSources[gh.iNodeSrcIndex];
					if( s.uRefCount == 0 )
					{
						VT_HR_EXIT( ResolveActiveNodes(pHead, rctPre, false, iTileId) );
						s.rct = gh.rct;
					}
					s.uRefCount++;
				}
			}
		}

		// execute the prefetch on the readers
		for( size_t j = 0; j < m_vecNodeSources.size(); j++ )
		{
			GRAPH_NODE_SOURCE& s = m_vecNodeSources[j];
			if( s.uRefCount )
			{
                if( s.type == GRAPH_NODE_SOURCE::eReader )
                {
                    CLevelChangeReader rd(s.pReader, s.level);
                    VT_HR_EXIT( VtPrefetchPadded(s.rct, s.infoReader, &rd, 
                                                 s.exReader) );
                }
                else if( s.type == GRAPH_NODE_SOURCE::eIndexedReader )
                {
                    CIndexedImageReaderIterator rd(s.pIndexedReader, s.idx, s.level);
                    VT_HR_EXIT( VtPrefetchPadded(s.rct, s.infoReader, &rd, 
                                                 s.exReader) );
                }
			}
			s.uRefCount = 0;
		}
	}

	// determine the active nodes and their source tiles
	for( size_t i = 0; i < m_vecHeadNodes.size(); i++ )
	{
		GRAPH_HEAD& gh = m_vecHeadNodes[i];
		if( !gh.rct.IsRectEmpty() )
		{
			GRAPH_NODE* pHead = m_vecNodes.begin() + gh.uNodeIndex;   
			if( gh.iNodeSrcIndex < 0 )
			{
				VT_HR_EXIT( ResolveActiveNodes(pHead, gh.rct, false, iTileId) );
			}
			else 
			{
				GRAPH_NODE_SOURCE& s = m_vecNodeSources[gh.iNodeSrcIndex];
				if( s.uRefCount == 0 )
				{
					VT_HR_EXIT( ResolveActiveNodes(pHead, gh.rct, false, iTileId) );
					s.rct = gh.rct;
					s.bGenerated = false;
				}
				s.uRefCount++;
			}
		}
	}
	for( size_t i = 0; i < m_vecHeadNodes.size(); i++ )
	{
		// the execute graph traversal does not decrement head node refs, so
		// do that here
		GRAPH_HEAD& gh = m_vecHeadNodes[i];
		if( !gh.rct.IsRectEmpty() && gh.iNodeSrcIndex >= 0 )
		{
			m_vecNodeSources[gh.iNodeSrcIndex].uRefCount--;
		}
	}

	// execute the active graph transforms
	for( size_t i = 0; i < m_vecHeadNodes.size(); i++ )
	{
		// descend if the rect is not-empty and this is a head node or this is
		// an intermediate node and it has yet to be generated
		GRAPH_HEAD& gh = m_vecHeadNodes[i];
		if( !gh.rct.IsRectEmpty() )
		{
			GRAPH_NODE* pHead = m_vecNodes.begin() + gh.uNodeIndex;
			if( gh.iNodeSrcIndex < 0 )
			{
				VT_HR_EXIT( ExecuteGraph(NULL, pHead, gh.rct, iTileId) );
			}
			else
			{
				GRAPH_NODE_SOURCE& s = m_vecNodeSources[gh.iNodeSrcIndex];
				if( !s.bGenerated )
				{
				    if( s.uRefCount == 0 )
				    {
					    VT_HR_EXIT( ExecuteGraph(NULL, pHead, gh.rct, iTileId) );
				    }
				    else
				    {
						// resolve active nodes should have accounted for this
					    VT_ASSERT( s.rct.RectInRect(gh.rct) );  
					    VT_HR_EXIT( ExecuteGraph(&s, pHead, s.rct, iTileId) );
				    }
				}
			}
		}
	}

	// debug check that all pool and source refs are down to 0
#ifdef _DEBUG
	for( UINT i = 0; i < m_vecNodeSources.size(); i++ )
	{
		VT_ASSERT( m_vecNodeSources[i].uRefCount == 0 );
	}
	for( UINT i = 0; i < m_vecImgMemPool.size(); i++ )
	{
		VT_ASSERT( m_vecImgMemPool[i].uRefCount == 0 );
	}
#endif

	// increment the pStatus counter
	m_pStatus->BlockComplete(pTaskProg);

	VT_HR_EXIT_LABEL()

	return hr;
}

//+-----------------------------------------------------------------------------
//
// Function: ResolveActiveNodes
// 
// Synopsis: determine which sources are active for the current rect and
//           join any sources that are common to other nodes
// 
//------------------------------------------------------------------------------
HRESULT
CTransformTaskTraverse::ResolveActiveNodes(GRAPH_NODE* pNode, CRect rctOut, 
										   bool bNodeVisited, int iTileId)
{
	VT_HR_BEGIN()

	UINT uSrcCnt = (UINT)pNode->vecSourceIndex.size();

	// compute necessary child regions

	// NOTE: in cases where the sub-graph is being revisited, we are overwriting 
	//       the previous vecTSR.  The assumption here is that this is OK since
	//       rctOut is only growing, but transforms with the unlikely behavior
	//       that a rctOut that encompasses previous smaller rctOut requests
	//       causes said transform to request different sources - would be
	//       broken by this. 
	VT_HR_EXIT( pNode->pTransform->GetRequiredSrcRect( 
		pNode->vecTSR.begin(), pNode->uTransSrcCnt, uSrcCnt, rctOut) );

	VT_ASSERT( pNode->uTransSrcCnt <= uSrcCnt );
	 
	// TODO: determine #of passes and add a loop on passes

	for( UINT i = 0; i < pNode->uTransSrcCnt; i++ )
	{
		const TRANSFORM_SOURCE_DESC& tsr = pNode->vecTSR[i];
		if( !tsr.rctSrc.IsRectEmpty() )
		{
			GRAPH_NODE_SOURCE& src = 
				m_vecNodeSources[pNode->vecSourceIndex[tsr.uSrcIndex]];

			// set the iPoolId to -1 - this is a flag to the graph execution
			// process that this source has yet to be generated 
			src.iPoolId = -1;
			src.bGenerated = false;

			// determine if there is a child node
			GRAPH_NODE* pChild = (src.type == GRAPH_NODE_SOURCE::eNode)? 
				m_vecNodes.begin()+src.uNodeIndex: NULL; 

			// for each source union the rect and bump the refcount
			bool bChildNodeVisited;
			bool bDoResolve = (pChild != NULL);
			if( src.uRefCount == 0 )
			{
				src.rct = tsr.rctSrc;

				// if the child is a head node then union its requested rect
				if( pChild && pChild->iGraphHeadIndex != -1 )
				{
					src.rct |= m_vecHeadNodes[pChild->iGraphHeadIndex].rct;
				} 
				bChildNodeVisited = false;
			}
			else
			{
				if( src.rct.RectInRect(tsr.rctSrc) )
				{
					bDoResolve = false;
				}
				else
				{
					src.rct |= tsr.rctSrc;
				}
				bChildNodeVisited = true;
			}
	
			if( !bNodeVisited )
			{
				// bNodeVisited indicates that this part of the graph has been
				// previously visited so these sources don't have multiple refs
				// only sources referenced from previously unvisited portions
				// of the graph have multiple refs
				src.uRefCount++;
			}
			else
			{
				VT_ASSERT( src.uRefCount != 0 );
			}
			
			// visit the rest of the graph
			if( bDoResolve )
			{
				VT_HR_EXIT( ResolveActiveNodes(pChild, src.rct,
											   bChildNodeVisited, iTileId) );
			}
		}
	}

	VT_HR_END()
}

inline void
CreateImageFromMemPtr(CImg& img, Byte* pPoolMem, const vt::CRect& rctMem,
					  const vt::CRect& rctInMem, int iType)
{
	VT_ASSERT( rctInMem.left >=0 && rctInMem.top >=0 );

	int w = rctInMem.Width();
	int h = rctInMem.Height();
	int pxsize = VT_IMG_ELSIZE(iType) * VT_IMG_BANDS(iType);
	int stride = (rctMem.Width() * pxsize + 0xf) & ~0xf;
	Byte* pTL  = pPoolMem + rctInMem.top * stride + rctInMem.left*pxsize;
#if _DEBUG
	int hr = 
#endif
    img.Create(pTL, w, h, stride, iType);
#if _DEBUG
	VT_ASSERT(hr == S_OK);
#endif
}

HRESULT
CTransformTaskTraverse::ExecuteGraph(GRAPH_NODE_SOURCE* pDst, GRAPH_NODE* pNode,
									 vt::CRect rctDst, int iTileId)
{
	VT_HR_BEGIN()

	// descend
	// TODO: outer loop on passes 
	for( UINT i = 0; i < pNode->uTransSrcCnt; i++ )
	{
		const TRANSFORM_SOURCE_DESC& tsr = pNode->vecTSR[i];
		if( !tsr.rctSrc.IsRectEmpty() )
		{
			GRAPH_NODE_SOURCE& src = 
				m_vecNodeSources[pNode->vecSourceIndex[tsr.uSrcIndex]];
	
			// only descend if the source image hasn't been generated yet
			if( !src.bGenerated && src.type == GRAPH_NODE_SOURCE::eNode )
			{
				GRAPH_NODE* pChild = m_vecNodes.begin()+src.uNodeIndex; 
				VT_HR_EXIT( ExecuteGraph(&src, pChild, src.rct, iTileId) );
			}
		}
	}

	// TODO: pDst is available on all passes, but somehow don't reallocate it
	//       on pass > 0
    bool bNoDst = pDst==NULL;
	if( pNode->iGraphHeadIndex != -1 )
	{
		const GRAPH_HEAD& gh = m_vecHeadNodes[pNode->iGraphHeadIndex];
		bNoDst = bNoDst && gh.writer.pWriter==NULL;
	}

	// TODO: loop on passes

	// allocate images and necessary data structs for the transform
	short iDstPoolId = -1;
	VT_HR_EXIT( BeginTransform(bNoDst? NULL: &iDstPoolId, pDst, pNode, rctDst) );

	// alloc more source pointers if necessary - this will happen very rarely
	// only when a transform uses more sources than any other transform 
	// before it 
	UINT uImgCnt = pNode->uTransSrcCnt+(bNoDst? 0: 1);
	if( uImgCnt > (UINT)m_vecImgSrcPtrPool.size() )
	{
		VT_HR_EXIT( m_vecImgPool.resize(uImgCnt) );
		VT_HR_EXIT( m_vecImgSrcPtrPool.resize(uImgCnt) );
	}

	// read any sources necessary and setup the transform inputs
	bool bSrcOverwrite = false;
	vt::CRect rctSrcOver;
	for( UINT i = 0; i < pNode->uTransSrcCnt; i++ )
	{
		const TRANSFORM_SOURCE_DESC& tsr = pNode->vecTSR[i];
	   	GRAPH_NODE_SOURCE& src = m_vecNodeSources[pNode->vecSourceIndex[tsr.uSrcIndex]];
		int iTypeSrc = pNode->vecTypeSrc[tsr.uSrcIndex];

		// create an image that wraps memory allocated for this source
		CImg& imgSrc = m_vecImgPool[i];
		if( tsr.rctSrc.IsRectEmpty() )
		{
			CreateImageFromMemPtr(imgSrc, NULL, vt::CRect(0,0,0,0),
								  vt::CRect(0,0,0,0), iTypeSrc);
            // TODO: I think that this assert may be incorrect if two nodes
            //       are requesting a read but one is empty
			VT_ASSERT( src.uRefCount == 0 );
		}
		else
		{
			VT_ASSERT( src.iPoolId != -1 ); 
		    IMG_POOL_EL* pPoolEl   = m_vecImgMemPool.begin()+src.iPoolId; 
			VT_ASSERT( pPoolEl->pMemory != NULL );

			if( src.iPoolId == iDstPoolId )
			{
				bSrcOverwrite = true;
				rctSrcOver    = src.rct;
			}
			
            // get the requested rect inside of the source rect
			VT_ASSERT( src.rct.RectInRect(&tsr.rctSrc) );
			CreateImageFromMemPtr(imgSrc, (Byte*)pPoolEl->pMemory, src.rct,
								  tsr.rctSrc - src.rct.TopLeft(), iTypeSrc);

            // read the image if necessary
			if( !src.bGenerated && src.type != GRAPH_NODE_SOURCE::eNode )
			{
                CImg imgRd;
                imgRd.Create((Byte*)pPoolEl->pMemory, src.rct.Width(), src.rct.Height(),
                             imgSrc.StrideBytes(), iTypeSrc);
                if( src.type == GRAPH_NODE_SOURCE::eReader )
                {                
                    CLevelChangeReader rd(src.pReader, src.level);
                    VT_HR_EXIT( VtCropPadImage(imgRd, src.rct, src.infoReader, 
                                               &rd, src.exReader) );
                }
                else
                {
                    VT_ASSERT( src.type == GRAPH_NODE_SOURCE::eIndexedReader );
                    CIndexedImageReaderIterator rd(src.pIndexedReader, src.idx, src.level);
                    VT_HR_EXIT( VtCropPadImage(imgRd, src.rct, src.infoReader, 
                                               &rd, src.exReader) );
                }
				src.bGenerated = true;
			}
            			
			// while we are here decrease the source ref counts
			VT_ASSERT( src.uRefCount != 0 );
			VT_ASSERT( pPoolEl->uRefCount != 0 );
		    src.uRefCount--;
		    pPoolEl->uRefCount--;
		}

		// the transform needs a contiguous list of image pointers, so store 
		// the pointer to the image that will be provided to the transform
		m_vecImgSrcPtrPool[i] = &imgSrc;
	}

	CImg* pImgDst = NULL;
	if( iDstPoolId != -1 )
	{
		IMG_POOL_EL* pDstPoolEl = m_vecImgMemPool.begin()+iDstPoolId; 
		pImgDst = &m_vecImgPool[pNode->uTransSrcCnt];
		if( bSrcOverwrite && 
			(rctSrcOver.Width()  != rctDst.Width() || 
			 rctSrcOver.Height() != rctDst.Height()) )
		{
			VT_ASSERT( rctSrcOver.RectInRect(rctDst) );
			CreateImageFromMemPtr(*pImgDst, (Byte*)pDstPoolEl->pMemory, rctSrcOver, 
								  rctDst - rctSrcOver.TopLeft(),
								  pNode->iTypeDst);
			// update the pDst node source to indicate that that result is
			// shared inside a larger tile image
			if( pDst )
			{
				pDst->rct = rctSrcOver;
			}
		}
		else
		{ 
			CreateImageFromMemPtr(*pImgDst, (Byte*)pDstPoolEl->pMemory, rctDst, 
								  vt::CRect(0,0,rctDst.Width(), rctDst.Height()),
								  pNode->iTypeDst);
			if( bSrcOverwrite && pDst )
			{
				VT_ASSERT( pDst->rct.Width()  == rctSrcOver.Width() &&
						   pDst->rct.Height() == rctSrcOver.Height() );
			}
		}

		// release the transform's ref to the pool memory, a source should
		// still be holding a reference if this is an intermediate result
		
		// TODO: don't release until after lass pass
		pDstPoolEl->uRefCount--; 
	}

#if _DEBUG
    // make sure that we've set things up so that all images come out of the pool
    for( UINT i = 0; i < pNode->uTransSrcCnt+1; i++ )
    {
        const CImg* pT = (i == pNode->uTransSrcCnt)? pImgDst: m_vecImgSrcPtrPool[i];
		if( pT == NULL || (pT->BytePtr()==NULL && pT->Width()==0 && pT->Height()==0) )
        {
            continue;
        }
        UINT j = 0;
		int sz = pT->StrideBytes() * (pT->Height()-1) + pT->Width()*pT->PixSize();
		for( ; j < m_vecImgMemPool.size(); j++ ) 
        {
            const IMG_POOL_EL& el = m_vecImgMemPool[j];
            if( pT->BytePtr() >= el.pMemory &&
                (pT->BytePtr() + sz) <= (Byte*)el.pMemory+el.uMemorySize )
            {
                break;
            }
        }
        VT_ASSERT( j != m_vecImgMemPool.size() );
    }
#endif

	VT_HR_EXIT( pNode->pTransform->Transform( 
		pImgDst, rctDst, (CImg*const*)m_vecImgSrcPtrPool.begin(), 
		pNode->vecTSR.begin(), pNode->uTransSrcCnt) );

	// TODO: end loop on passes

	if( pDst )
	{
		pDst->bGenerated = true;
	}

	// make sure that transform didn't mess with the dest image dimensions
	VT_ASSERT( !pImgDst || (pImgDst->Width()  == rctDst.Width() &&
						    pImgDst->Height() == rctDst.Height()) );
 
	if( pNode->iGraphHeadIndex != -1 )
	{
		GRAPH_HEAD& gh = m_vecHeadNodes[pNode->iGraphHeadIndex];
		if( gh.writer.pWriter )
		{
			VT_ASSERT( rctDst.RectInRect(&gh.rct) );
			ANALYZE_ASSUME(pImgDst != NULL);
			CImg imgWrite;
			const CRect r(gh.rct - rctDst.TopLeft());
			pImgDst->Share(imgWrite, &r);

			VT_HR_EXIT( gh.writer.pWriter->WriteRegion(gh.rct-gh.ptWr, imgWrite) );
		}
	}

	VT_HR_END()
}

HRESULT
CTransformTaskTraverse::BeginTransform(OUT short* pDstPoolId, 
									   OUT   GRAPH_NODE_SOURCE* pDstSrc,
									   const GRAPH_NODE* pNode, 
									   const vt::CRect& rctDst)
{
	VT_HR_BEGIN()

	const TRANSFORM_SOURCE_DESC* pSrcReq = pNode->vecTSR.begin();

	if( pDstPoolId )
	{
		*pDstPoolId = -1;
	}

	// run in two passes - first pass tries to find 'ideal' tile reuse,
	// second pass tries any tile reuse
	for( UINT k = 0; k < 2; k++ )
	{
		// for each source
		for( UINT i = 0; i < pNode->uTransSrcCnt; i++ )
		{
			const TRANSFORM_SOURCE_DESC& tsr = pSrcReq[i];
			if( tsr.rctSrc.IsRectEmpty() )
			{
				continue;
			}

			GRAPH_NODE_SOURCE& src = 
				m_vecNodeSources[pNode->vecSourceIndex[tsr.uSrcIndex]];
		    int iTypeSrc = pNode->vecTypeSrc[tsr.uSrcIndex];

			// if the source has already been generated then continue;
			if( src.iPoolId != -1 )
			{
				VT_ASSERT( src.iPoolId != -1 );

				VT_ASSERT( src.uRefCount != 0 && (m_vecImgMemPool.begin() + 
					src.iPoolId)->uRefCount != 0 );
				VT_ASSERT( src.rct.RectInRect(&tsr.rctSrc) );
			}
			else
			{
				// can only get here if the source is a reader - sources that
				// are other nodes should have been allocated already   
				VT_ASSERT( src.type != GRAPH_NODE_SOURCE::eNode );
	
				UINT uPoolId;
				VT_HR_EXIT( AllocTransformMemory(uPoolId, src.rct, iTypeSrc,
												 k==0) );
	
				if( uPoolId < m_vecImgMemPool.size() )
				{
					IMG_POOL_EL *pPoolEl = m_vecImgMemPool.begin() + uPoolId;
					src.iPoolId = short(uPoolId);
					pPoolEl->uRefCount = src.uRefCount;
				}
				else
				{
					VT_ASSERT( k==0 );
				}
			}

			// determine if this can be used as the dest image
            VT_ASSERT( src.uRefCount > 0 );
			if( pDstPoolId != NULL && *pDstPoolId == -1 && 
				tsr.bCanOverWrite && src.iPoolId != -1 &&
				IMG_FORMAT(pNode->iTypeDst) == IMG_FORMAT(iTypeSrc) &&
				((rctDst.Width() == src.rct.Width() && 
				  rctDst.Height() == src.rct.Height()) ||
				 src.rct.RectInRect(&rctDst)) )
			{
                if (src.uRefCount > 1)
                {
		            // check for common sources
                    UINT uRefCount = 0;
		            for( UINT s = 0; s < pNode->uTransSrcCnt; s++ )
		            {
			            if (pSrcReq[s].bCanOverWrite &&
                            pNode->vecSourceIndex[pSrcReq[s].uSrcIndex] ==
                            pNode->vecSourceIndex[tsr.uSrcIndex])
                            uRefCount++;
                    }
                    if (uRefCount != src.uRefCount)
                        continue;
                }

				*pDstPoolId = src.iPoolId;
				IMG_POOL_EL *pDstPoolEl = m_vecImgMemPool.begin() + src.iPoolId;
				pDstPoolEl->uRefCount++;
				if( pDstSrc )
				{   
					pDstPoolEl->uRefCount += pDstSrc->uRefCount;
					pDstSrc->iPoolId = *pDstPoolId;
				}
			}
		}
	}

	// allocate the dest if it hasn't been yet in the bCanOverwrite test above
	if( pDstPoolId != NULL && *pDstPoolId == -1 )
	{
		for( UINT k = 0; k < 2; k++ )
		{
			UINT uPoolId;
			VT_HR_EXIT( AllocTransformMemory(
				uPoolId, rctDst, pNode->iTypeDst, k==0) );
			if( uPoolId < m_vecImgMemPool.size() )
			{
				*pDstPoolId = short(uPoolId);
				IMG_POOL_EL *pDstPoolEl = m_vecImgMemPool.begin()+uPoolId;
				pDstPoolEl->uRefCount = 1;
				if( pDstSrc )
				{   
					pDstPoolEl->uRefCount += pDstSrc->uRefCount;
					pDstSrc->iPoolId = *pDstPoolId; 
				}
				break;
			}
			else
			{
				VT_ASSERT( k==0 );
			}
		}
	}

    VT_HR_END()
}

HRESULT
CTransformTaskTraverse::AllocTransformMemory(OUT UINT& uPoolId, const CRect& rct,
											 int iType, bool bCloseFitOnly)
{
	VT_HR_BEGIN()

    VT_ASSERT( rct.Width() >= 0 && rct.Height() >= 0 );

	int w = rct.Width();
	int h = rct.Height();
	int stride = (w * VT_IMG_ELSIZE(iType) * VT_IMG_BANDS(iType) + 0xf) & ~0xf;

	size_t uMemory = h*stride;
	size_t uMemoryPad = uMemory + (uMemory>>3);  
	UINT uMinDiff     = UINT_MAX;

	uPoolId = (UINT)m_vecImgMemPool.size();
	bool bPoolFull = true;
	for( UINT i = 0; i < static_cast<UINT>(m_vecImgMemPool.size()); i++ )
	{
		IMG_POOL_EL& el = m_vecImgMemPool[i];
		if( el.uRefCount != 0 )
		{
			continue;
		}

		bPoolFull = false;
		if( bCloseFitOnly )
		{
			if( el.uMemorySize >= uMemory && el.uMemorySize <= uMemoryPad )
			{
				uPoolId = i;
				break;
			}
		}
		else if( el.uMemorySize >= uMemory )
		{
			size_t uDiff = el.uMemorySize - uMemory;
			if( uDiff < uMinDiff )
			{
				uPoolId  = i;
				uMinDiff = (UINT)uDiff;
			}
		}
	}

	if( uPoolId >= m_vecImgMemPool.size() && !bCloseFitOnly )
	{
        UINT i = 0;
		// if the pool is full then resize it - this should only happen
		// a small number of times at startup
		if( bPoolFull )
		{
			i = (UINT)m_vecImgMemPool.size();
			VT_HR_EXIT( m_vecImgMemPool.resize(m_vecImgMemPool.size()+1) );
		}

		// didn't find space so need to allocate a new block
		for( ; i < (UINT)m_vecImgMemPool.size(); i++ )
		{
			IMG_POOL_EL& el = m_vecImgMemPool[i];
			if( el.uRefCount == 0 )
			{
				if( uMemory != 0 )
				{
					delete [] el.pMemoryAlloc;
					el.pMemory = el.pMemoryAlloc = NULL;
					el.uMemorySize = 0;
					el.pMemoryAlloc = VT_NOTHROWNEW Byte[uMemory+64];
					VT_PTR_OOM_EXIT( el.pMemoryAlloc );
					el.pMemory = el.pMemoryAlloc;
					// always align memory to cache lines
					uintptr_t offset = (uintptr_t)el.pMemory & 63;
					if( offset )
					{
						el.pMemory = (uint8_t*)el.pMemory + (64-offset);
					}
					el.uMemorySize = uMemory;
					// VT_DEBUG_LOG("Allocate block size %08x\n",el.uMemorySize);
				}
				uPoolId = i;
				break;
			}
		}
	}

	VT_HR_END()
}

//+-----------------------------------------------------------------------------
//
// Function: PushTransformTask
// 
//------------------------------------------------------------------------------
inline HRESULT
PushTransformTaskInternal(const CTransformGraphNode*const* ppGraphHeads, 
						  UINT uHeadCount, 
						  CTaskStatus*   pUserStatus, 
						  CTaskProgress* pUserProgress,
						  bool bWait,
						  const VT_TRANSFORM_TASK_OPTIONS* pOpts)
{
	CTransformTaskTraverse *pTraversal = VT_NOTHROWNEW CTransformTaskTraverse();
	TRNSFRM_PTR_OOM_RET(pTraversal, pUserStatus)

	return pTraversal->PushTask(ppGraphHeads, uHeadCount, pUserStatus,
								pUserProgress, bWait, pOpts);
}

HRESULT
vt::PushTransformTask(const CTransformGraphNode*const* ppGraphHeads, 
					  UINT uHeadCount, 
					  CTaskStatus* pUserStatus, 
					  const VT_TRANSFORM_TASK_OPTIONS* pOpts)
{
	return PushTransformTaskInternal(ppGraphHeads, uHeadCount, pUserStatus,
									 pUserStatus, false, pOpts);
}

HRESULT
vt::PushTransformTaskAndWait(const CTransformGraphNode*const* ppGraphHeads, 
							 UINT uHeadCount, 
							 CTaskStatus* pUserStatus,
							 const VT_TRANSFORM_TASK_OPTIONS* pOpts)
{
	return PushTransformTaskInternal(ppGraphHeads, uHeadCount, pUserStatus, 
									 pUserStatus, true, pOpts);
}

HRESULT
vt::PushTransformTaskAndWait(const CTransformGraphNode*const* ppGraphHeads, 
							 UINT uHeadCount, 
							 CTaskProgress* pUserProgress, 
							 const VT_TRANSFORM_TASK_OPTIONS* pOpts)
{
	return PushTransformTaskInternal(ppGraphHeads, uHeadCount, NULL, 
									 pUserProgress, true, pOpts);
}

//+-----------------------------------------------------------------------------
//
// Class: CTransformTaskStatus
// 
//------------------------------------------------------------------------------
inline HRESULT 
CTransformTaskStatus::SetDone()
{ 
	HRESULT hr = S_OK;
	if( m_pUserProgress )
	{
		m_pUserProgress->ReportProgress(100.f);
	}
	if( m_pUserStatus )
	{
		hr = m_pUserStatus->SetDone();
	} 
	delete m_pTask;
	delete m_pDefBlockMap;
	delete this;
	return hr;
}

