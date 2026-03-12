//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Implementation of tasks management for visiontools applications.
//      The very basic description is that the taks manager consumes tasks
//      from user threads and farms them out perhaps in parallel to a set
//      of worker threads maintained by the task manager.
//
//  History:
//      2007/06/1-mattu
//          Created
//
//------------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_taskmanager.h"
#if defined(_MSC_VER)
#include "vt_singleton.h"
#endif

using namespace vt;

// commonly occuring error pattern where the task needs to return before
// it was successfully submitted.  in this case we still have to signal 
// done.
#define TASK_PTR_RET(ptr, pStat, err)\
    if( ptr == NULL ) {\
		if( pStat ) {\
			pStat->SetTaskError(err);\
		    pStat->SetDone();\
		}\
        VT_DEBUG_LOG_HR(err);\
		return err;\
	}\

#define TASK_HR_RET(call, pStat)\
	if( (hr = call) != S_OK ) {\
		if( pStat ) {\
			pStat->SetTaskError(hr);\
		    pStat->SetDone();\
		}\
        VT_DEBUG_LOG_HR(hr);\
		return hr;\
	}\


//+-----------------------------------------------------------------------------
//
// Class: CPhasedTaskProgressImpl
// 
//------------------------------------------------------------------------------
template<class Base>
void CPhasedTaskProgressImpl<Base>::BeginPhase(const char* phaseName, float fPct)
{
	VT_ASSERT( fPct >= 0 );
	VT_ASSERT( fPct <= 100.1f );
	VT_ASSERT( m_fBase + m_fPhasePct*100.f + fPct <= 100.1f );

	if( phaseName )
	{
		//VT_DPF(("Beginning phase: %s\n", phaseName));
	}
	m_fBase += m_fPhasePct*100.f;
	m_fPhasePct = fPct/100.f;
	m_fProgressInPhase = 0;
}

template<class Base>
void CPhasedTaskProgressImpl<Base>::ReportProgress(float fPct)
{
	VT_ASSERT( fPct >= 0 );
	VT_ASSERT( fPct <= 100.1f );

	m_fProgressInPhase = fPct;
	if( m_pTaskProgOuter )
	{
		m_pTaskProgOuter->ReportProgress(m_fBase + 
										 m_fPhasePct*m_fProgressInPhase);   
	}
}

namespace vt {
template class CPhasedTaskProgressImpl<CTaskProgress>;
#if defined(_MSC_VER)
template class CPhasedTaskProgressImpl<CTaskStatus>;
#endif
};

#if defined(_MSC_VER)
//+-----------------------------------------------------------------------------
//
// Class: CTaskStatusEvent
// 
//------------------------------------------------------------------------------

HRESULT CTaskStatusEvent::Create()
{
	if( m_hTaskIdle == NULL )
	{
		m_hTaskIdle = CSystem::CreateEvent(TRUE, TRUE);
		if( m_hTaskIdle == NULL )
		{
			return HRESULT_FROM_WIN32( GetLastError() );
		}
	}
	return S_OK;
}

HRESULT CTaskStatusEvent::Begin()
{
	HRESULT hr;
	if( (hr = Create()) == S_OK )
	{
        m_bDone   = false;
		m_bCancel = false;
		m_fProgressPct = 0;
		m_hrTask    = S_OK;

		if( !ResetEvent(m_hTaskIdle) )
		{
			hr = HRESULT_FROM_WIN32( GetLastError() );
		}
	}
	return hr;
}

void CTaskStatusEvent::ReportProgress(float fPct)
{
	VT_ASSERT( fPct >= 0 && fPct <= 100.1f );

	m_fProgressPct = fPct;
}

HRESULT CTaskStatusEvent::SetDone()
{ 
    m_bDone = true;

	if( !SetEvent(m_hTaskIdle) )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}
	return S_OK;
}

HRESULT CTaskStatusEvent::WaitForDone()
{
	if( m_hTaskIdle )
	{
		// TODO: could do a timeout loop checking for error condition
		DWORD status = vt::CSystem::WaitForSingleObjectEx(m_hTaskIdle, INFINITE, FALSE);
		return (status==WAIT_FAILED)? 
			HRESULT_FROM_WIN32(GetLastError()): S_OK;
	}
	return E_UNEXPECTED;
}
#endif

//+-----------------------------------------------------------------------------
//
// Class: CCritSection
// 
//------------------------------------------------------------------------------

HRESULT CCritSection::Startup(int spincount)
{
	if( !m_bInit )
	{
#if defined(__clang__)
        m_bInit = (pthread_mutex_init( &m_mutex, NULL ) == 0);
        return m_bInit ? S_OK: E_FAIL;
#else
		m_bInit = vt::CSystem::InitializeCriticalSection(&m_cs, spincount) ? 
			true : false;
			
		return m_bInit?	S_OK: HRESULT_FROM_WIN32( GetLastError() );
#endif
    }
	return S_OK;
}

void CCritSection::Shutdown()
{
	if( m_bInit )
	{
#if defined(__clang__)
        pthread_mutex_destroy( &m_mutex );
#else
        DeleteCriticalSection(&m_cs);
#endif
		m_bInit = false;
	}
}

_Acquires_lock_(m_cs) void CCritSection::Enter()
{
#if defined(__clang__)
    pthread_mutex_lock( &m_mutex );
#else
    EnterCriticalSection(&m_cs);
#endif
}

_Releases_lock_(m_cs) void CCritSection::Leave()
{
#if defined(__clang__)
    pthread_mutex_unlock( &m_mutex );
#else
    LeaveCriticalSection(&m_cs);
#endif
}

#if defined(_MSC_VER)
//+-----------------------------------------------------------------------------
//
// Class: CTaskManager
// 
//------------------------------------------------------------------------------
class CTaskManager
{
public:
	HRESULT PushTask(void* callback, void* pArg, ITaskState* pState,
					 CTaskStatus* pStatus, 
					 LONG count, ITaskWorkIdSequencer* pSeq,
					 bool bWait, const VT_TASK_OPTIONS* pOpts = NULL);
	
	//+-------------------------------------------------------------------------
	//
	// Class: CTaskStatusDefault
	// 
	// Synopsis: a default implementation of CTaskStatus that is used when 
	//           users don't supply a task status callback
	// 
	//--------------------------------------------------------------------------
protected:
	class CTaskStatusDefault: public CTaskStatusEvent
	{
	public:
		virtual HRESULT Begin(CTaskStatus* pCaller = NULL)
		{ 
			m_pTaskProgCaller = pCaller;
			m_bCancel = pCaller? pCaller->GetCancel(): false;
			if( m_bCancel )
			{
				return E_ABORT;
			}
			return CTaskStatusEvent::Begin();
		}

		virtual bool  GetCancel()
		{ return m_bCancel; }

		virtual HRESULT WaitForDone()
		{
			HRESULT hr = E_NOINIT;
			if( m_hTaskIdle )
			{
				// this loop insures that progress is always issued on the 
				// calling thread 
				// TODO: do I also need to add error checking to the loop?
				//       e.g. is there an error condition where done doesn't get
				//       set as a byproduct?
				DWORD status, timeout = m_pTaskProgCaller? 100: INFINITE;
				while( (status = vt::CSystem::WaitForSingleObjectEx(m_hTaskIdle, timeout, FALSE)) == 
					   WAIT_TIMEOUT )
				{
					if( m_pTaskProgCaller )
					{
						m_pTaskProgCaller->ReportProgress(m_fProgressPct);
						m_bCancel = m_pTaskProgCaller->GetCancel();
					}
				}

                hr = (status==WAIT_FAILED)? 
                    HRESULT_FROM_WIN32(GetLastError()): S_OK;

                if( m_pTaskProgCaller )
			    {
                    HRESULT hr2 = m_pTaskProgCaller->SetDone();
                    if( hr == S_OK )
                    {
                        hr = hr2;
                    }
				    // can't use m_pTaskProgCaller after SetDone()
				    m_pTaskProgCaller = NULL;
			    }
			}
			return hr;
		}

		virtual HRESULT SetDone()
        { return CTaskStatusEvent::SetDone(); }

		virtual void SetTaskError(HRESULT hr)
		{
			 if( m_pTaskProgCaller )
			 {
				 m_pTaskProgCaller->SetTaskError(hr);
				 CTaskStatusEvent::SetTaskError(
					 m_pTaskProgCaller->GetTaskError());
			 }
			 else
			 {
				 CTaskStatusEvent::SetTaskError(hr);
			 }
		}

	public:
		CTaskStatusDefault(): m_pTaskProgCaller(NULL), m_bCancel(false)
		{}

	protected:  
		CTaskStatus* m_pTaskProgCaller;
		volatile bool m_bCancel;
	};
	
	//+-------------------------------------------------------------------------
	//
	// Class: CTask
	// 
	// Synopsis: The task manager holds a list of these.  One entry in the list 
	//           per active task.  Internally there is one TASK_THREAD_INFO
	//           per active thread.
	// 
	//--------------------------------------------------------------------------
protected:
	class CTask
	{
	public:
		CTask(LONG iActive) : m_pThreadInfoHead(NULL), m_iIsActive(iActive)
		{}

		~CTask()
		{ Shutdown(); }

		HRESULT Startup()
		{ return m_csThreads.Startup(); }

		void    Shutdown();
		bool    TryMakeActive();
		HRESULT TaskBegin(CSystem::CThreadPool* pPool, 
						  void* pCallback, void* pArg, ITaskState* pState, 
						  CTaskStatus* pStatus, LONG iCount,
						  ITaskWorkIdSequencer* pSeq,
						  const VT_TASK_OPTIONS* pOpts, bool bWait);

	protected:
		struct TASK_THREAD_INFO
	    {
			volatile LONG  iThreadId;  // set to 0 for uninitialized
			ITaskState* pCallbackState;
			TASK_THREAD_INFO* pNext;

			TASK_THREAD_INFO() : iThreadId(0), pCallbackState(NULL), pNext(NULL)
			{}

			~TASK_THREAD_INFO()
			{ 
				// make sure things got shut-down correctly
				VT_ASSERT(pCallbackState == NULL && iThreadId == 0); 
			}
		};

	protected:
		TASK_THREAD_INFO* GetInfoForThread(LONG iThreadIndex);

		static void __cdecl WorkCallback(PVOID Context)
		{ ((CTask*)Context)->WorkCallback();  }
		
        void  WorkCallback();
		
        void  TaskEnd(HRESULT hr);

	protected:
		CCritSection                     m_csThreads;
		CTaskManager::CTaskStatusDefault m_statusDefault;
		CSimpleCounterWorkIdSequencer    m_sequencerDefault;

		bool m_bWait;

		volatile LONG m_iIsActive;  

		volatile LONG m_iThreadIndex;
		volatile LONG m_iThreadFinished;
		ITaskWorkIdSequencer* m_pIdSequencer;

		void*         m_pCallback;
		void*         m_pTaskArg;
		ITaskState*   m_pTaskOrigState;
		CTaskStatus*  m_pTaskStatusCallback;
	    TASK_THREAD_INFO* volatile m_pThreadInfoHead;
	};

	//+-------------------------------------------------------------------------
	//
	// Struct: TASK_LIST_EL
	// 
	// Synopsis: elements of a linked list stored by the taskmanager.  
	// 
	//--------------------------------------------------------------------------
protected:
	struct TASK_LIST_EL
	{
		CTaskManager::CTask task;
		TASK_LIST_EL*       pNext;

		TASK_LIST_EL(LONG iActive) : pNext(NULL), task(iActive)
		{}  
	};

protected:
	CTaskManager() : m_pTaskList(NULL)
	{}
	~CTaskManager();

	HRESULT StartupInternal();

    HRESULT GetTask(CTaskManager::CTask** ppTask);

protected:
	vt::CCritSection	     m_csTasks;
	vt::CSystem::CThreadPool m_pool;
	TASK_LIST_EL* volatile   m_pTaskList;
};

//+-------------------------------------------------------------------------
//
// Class: CTaskManager
// 
//--------------------------------------------------------------------------
CTaskManager::~CTaskManager()
{
	// threading - will only be called from one thread

	while( m_pTaskList )
	{
		TASK_LIST_EL* pNext = m_pTaskList->pNext;
		delete m_pTaskList;
		m_pTaskList = pNext;
	}
}

HRESULT
CTaskManager::PushTask(void* callback, void* pArg, ITaskState* pState,
					   CTaskStatus* pStatus, 
					   LONG count, ITaskWorkIdSequencer* pSeq,
					   bool bWait, const VT_TASK_OPTIONS* pOpts)
{
	// threading - can be called concurrently 
	VT_ASSERT( callback );

	// get the count from the sequencer if necessary
	if( pSeq != NULL )
	{
		count = pSeq->GetTotalWorkItems();
	}

	// exit early for 0 count
	if( count == 0 )
	{
		if( pStatus )
		{
			pStatus->SetDone();
		}
		return S_OK;
	}

	// get a task from the CTask pool
	CTaskManager::CTask* pTask;
	HRESULT hr;
	TASK_HR_RET( GetTask(&pTask), pStatus );

	return pTask->TaskBegin(&m_pool, callback, pArg, pState, pStatus, count,
							pSeq, pOpts, bWait);
}

HRESULT 
CTaskManager::StartupInternal()
{ 
	VT_HR_BEGIN()

	VT_HR_EXIT( m_pool.Startup() );
	VT_HR_EXIT( m_csTasks.Startup() );

	VT_HR_END() 
}

HRESULT 
CTaskManager::GetTask(CTask** ppTask)
{
	// threading - can be called concurrently

	// try to reuse an existing task slot
	TASK_LIST_EL* pTaskEl = m_pTaskList; MemoryBarrier();
	for( ; pTaskEl; pTaskEl=pTaskEl->pNext )
	{
		if( pTaskEl->task.TryMakeActive() )
		{
			*ppTask = &(pTaskEl->task);
			return S_OK;
		}
	}

	// didn't find an available task so create a new one and add it to the list
	// TODO: it may be more efficient to create ThreadInfos on the CTask
	//       now instead of on demand - this will limit heap contention.  
	//       Probably want to pass in a task/processor count and alloc the min 
	//       of that number of threadinfos
	if( (pTaskEl = VT_NOTHROWNEW TASK_LIST_EL(1)) == NULL )
	{
		return E_OUTOFMEMORY;
	}
 
	m_csTasks.Enter();

	pTaskEl->pNext = m_pTaskList; 
	MemoryBarrier();

	m_pTaskList = pTaskEl;

	m_csTasks.Leave();

	*ppTask = &(pTaskEl->task);
	return S_OK;
}

//+-----------------------------------------------------------------------------
//
// Class: CTaskManager::CTask
// 
//------------------------------------------------------------------------------
void
CTaskManager::CTask::Shutdown()
{
	// threading - will only be called from one thread
	while( m_pThreadInfoHead )
	{
		// NOTE: for now users need to wait for task completion before
		//       shutting down.  it may be better to block here until
		//       task completion
		VT_ASSERT( m_iIsActive == 0 );
		TASK_THREAD_INFO* pNext = m_pThreadInfoHead->pNext;
		delete m_pThreadInfoHead;
		m_pThreadInfoHead = pNext;
	}
}

bool
CTaskManager::CTask::TryMakeActive()
{
	// threading - can be called concurrently

	bool bMadeActive = false;
	if( 0 == m_iIsActive &&
		0 == InterlockedCompareExchange(&m_iIsActive, 1, 0) )
	{
		bMadeActive = true;
	}
	return bMadeActive;
}

HRESULT
CTaskManager::CTask::TaskBegin(CSystem::CThreadPool* pPool,
							   void* pCallback, void* pTaskArg, 
							   ITaskState* pTaskState,	CTaskStatus* pStatus, 
							   LONG iCount, ITaskWorkIdSequencer* pSeq,
							   const VT_TASK_OPTIONS* pOpts, bool bWait)
{
	VT_ASSERT( pPool && pCallback && iCount );
	VT_ASSERT( !pTaskState || (pTaskState && !pTaskArg) );
	ANALYZE_ASSUME( pPool ); 

	// threading - will only be called from one thread for an instance of CTask
	VT_HR_BEGIN()

	// store task state pointers and counters
	m_iThreadIndex        = 0;
	m_pCallback           = pCallback;
	m_pTaskArg            = pTaskArg;
	m_pTaskOrigState      = pTaskState;
	m_bWait               = bWait;
	m_pIdSequencer        = (pSeq==NULL)? &m_sequencerDefault: pSeq;
	m_pTaskStatusCallback = (pStatus==NULL || m_bWait)? &m_statusDefault: pStatus;

    VT_HR_EXIT( Startup() );

	// if pStatus is NULL use the default or we are waiting internally then
	// use the default
	if( pStatus==NULL || m_bWait )
	{
		VT_HR_EXIT( m_statusDefault.Begin(pStatus) );
	}
	else
	{
		VT_HR_EXIT( CheckTaskCancel(pStatus) );
	}

	// setup the default sequencer if necessary
	if( pSeq == NULL )
	{
		m_sequencerDefault.Initialize(iCount);
	}

	// determine how many calls to make to submit. The work item callbacks 
	// will process multiple work items each, so if the user has requested 
	// a limit on number of threads then limit the submitted work items to that.
	m_iThreadFinished = iCount;
	if( pOpts && pOpts->maxthreads != 0 )
	{
		m_iThreadFinished = VtMin(m_iThreadFinished, (LONG)pOpts->maxthreads);
	}

	// submit the work to the specified threadpool
	VT_HR_EXIT( pPool->SubmitWork(WorkCallback, this, m_iThreadFinished) );
	
	VT_HR_EXIT_LABEL()

	// failure here can only happen if the task failed to start - so it is
	// safe to call TaskEnd because the class variables it updates aren't being
	// touched in worker threads.  Normally TaskEnd is called from the last
	// worker thread to complete work.
	if( hr != S_OK )
	{
		if( pStatus )
		{
			pStatus->SetTaskError(hr);
		}
		TaskEnd(hr);
		VT_HR_RET(hr);
	}

	// if we get here means task was succesfully started

	// wait if the caller requested
	if( m_bWait )
	{  
		m_statusDefault.WaitForDone();
		hr = m_statusDefault.GetTaskError();
        if( hr != S_OK )
        {
            VT_DEBUG_LOG_HR(hr);
        }
		VT_ASSERT( m_iIsActive );

		// if we are waiting, then can't reset iIsActive flag until control
		// returns to this thread and we are done with m_statusDefault
		MemoryBarrier();
		m_iIsActive = 0;
	}

	// if we got here it means that the task was sucessfully submitted, in the
	// case of wait, it also completed so return the hr of the task
	return hr;
}

void
CTaskManager::CTask::TaskEnd(HRESULT hr)
{
	// threading - will only be called from one thread
 
	// call merge into source on all concurrent state unless an error has
	// occurred 
	if( m_pTaskOrigState && m_pTaskOrigState->RequiresCloneForConcurrency() )
	{
		// TODO: only need to step through the thread infos that we actually
		//       used.  In some cases a very large list of infos may be on 
		//       this task (from previous tasks), but they may not have been
		//       used on this task.
		// 
		UINT uThreadCount = 0;
		UINT uThreadCountActive = 0;
		TASK_THREAD_INFO* pInfo = m_pThreadInfoHead; MemoryBarrier();
		while( pInfo )
		{
			if( hr == S_OK && pInfo->pCallbackState )
			{
				hr = m_pTaskOrigState->Merge((ITaskState*)pInfo->pCallbackState);
				if( hr != S_OK )
				{
					m_pTaskStatusCallback->SetTaskError(hr);
				}
			}

			uThreadCount++;
			if( pInfo->pCallbackState  )
			{
				uThreadCountActive++;
			}

			// NOTE: this info list is only being accessed in this one thread at
			//       this point so these vars can be updated without barriers, etc   
			delete pInfo->pCallbackState;
			pInfo->pCallbackState = NULL;
			pInfo->iThreadId = 0;
	
			pInfo = pInfo->pNext;
		}
		//VT_DPF(("   Task Done, used %d threads, out of %d thread slots\n",
		//	   uThreadCountActive, uThreadCount));
	}
		
	if( m_bWait )
	{
		// if the task is waiting in the call thread then we can simply indicate 
		// done here and the calling thread will perform the task shutdown 
		// operations that are present the in the else clause here
	    m_pTaskStatusCallback->SetDone();
	}
	else
	{
		CTaskStatus* pStatus = m_pTaskStatusCallback;

		// need a memorybarrier here because everything up to this point must have
		// completed before setting the task inactive - once this happens all member
		// variables might be overwritten - so no further access to them should be done
		MemoryBarrier();
		m_iIsActive = 0;

		// on taskend indicate to caller that task is done - this needs to 
		// be the last thing called and all previous load/stores above must
		// have completed by this point.  SetDone can cause a user to destroy 
		// the CTaskManager
		MemoryBarrier();
		pStatus->SetDone();
	}
}

CTaskManager::CTask::TASK_THREAD_INFO*
CTaskManager::CTask::GetInfoForThread(LONG iThreadIndex)
{
	// should only call this when it is needed
	VT_ASSERT( m_pTaskOrigState && 
			   m_pTaskOrigState->RequiresCloneForConcurrency() );
    ANALYZE_ASSUME( m_pTaskOrigState );

	// find the info for this thread
	TASK_THREAD_INFO* pInfo = m_pThreadInfoHead; MemoryBarrier();
	for( ; pInfo; pInfo=pInfo->pNext )
	{
		if( pInfo->iThreadId == iThreadIndex )
		{
			return pInfo;
		}
	}

	// otherwise find an empty slot for this thread
	pInfo = m_pThreadInfoHead; MemoryBarrier();
	for( ; pInfo; pInfo=pInfo->pNext )
	{
		if( 0 == pInfo->iThreadId &&
			0 == InterlockedCompareExchange(&(pInfo->iThreadId), iThreadIndex, 0) )
		{
			VT_ASSERT( pInfo->pCallbackState == NULL );

			// *** Enter Threads Critsection (in case Clone isn't threadsafe)
			m_csThreads.Enter();

			HRESULT hr = m_pTaskOrigState->Clone(&(pInfo->pCallbackState));
			if( hr != S_OK )
			{
				pInfo = NULL;
				m_pTaskStatusCallback->SetTaskError(hr);
			}
			m_csThreads.Leave();

			// *** Leave Threads Critsection

			return pInfo;
		}
	}

	// if still no pInfo allocate a new one
	pInfo = VT_NOTHROWNEW TASK_THREAD_INFO();
	if( pInfo == NULL )
	{
		m_pTaskStatusCallback->SetTaskError(E_OUTOFMEMORY);
	}
	else
	{
		VT_ASSERT( pInfo->pCallbackState == NULL );

		pInfo->iThreadId = iThreadIndex;
			
		// *** Enter Threads Critsection
		m_csThreads.Enter();
		HRESULT hr = m_pTaskOrigState->Clone(&(pInfo->pCallbackState));
		if( hr != S_OK )
		{
			delete pInfo;
			pInfo = NULL;
			m_pTaskStatusCallback->SetTaskError(hr);
		}
		else
		{
			pInfo->pNext = m_pThreadInfoHead; MemoryBarrier();
			m_pThreadInfoHead = pInfo;
		}
		m_csThreads.Leave();
		// *** Leave Threads Critsection
	}

	return pInfo;
}

void
CTaskManager::CTask::WorkCallback()
{
	// threading - will be called from multiple concurrent threads
	LONG iCurThreadIndex = InterlockedIncrement(&m_iThreadIndex); 

	TASK_THREAD_INFO* pInfo = NULL;
	if( m_pTaskOrigState && m_pTaskOrigState->RequiresCloneForConcurrency() )
	{
		pInfo = GetInfoForThread(iCurThreadIndex);
		// on failure (pInfo is NULL) then GetInfoForThread will have called
		// SetTaskError
	}

	// remainder of code uses a zero-based thread index
	iCurThreadIndex--;

	for (;;)
	{
		// don't submit new work if the task is in an error or cancel state
		if( m_pTaskStatusCallback->GetTaskError() != S_OK )
		{
			break;
		}

		// get the next work id
		bool bDone;
		int  iCurrentId;
		HRESULT hr = m_pIdSequencer->Advance(bDone, iCurrentId, iCurThreadIndex);
		if( hr != S_OK )
		{
			m_pTaskStatusCallback->SetTaskError(hr);
			break;
		}
		if( bDone )
		{
			break;
		}
		VT_ASSERT( iCurrentId < m_pIdSequencer->GetTotalWorkItems() );

		if(	m_pTaskStatusCallback->GetCancel() )
		{
			hr = E_ABORT;
		}
		else
		{
			if( m_pTaskOrigState )
			{
				if( m_pTaskOrigState->RequiresCloneForConcurrency() )
				{
					VT_ASSERT( pInfo != NULL );
					ANALYZE_ASSUME( pInfo != NULL );
					hr = ((TASK_WITH_STATE_CALLBACK)m_pCallback)(
											  pInfo->pCallbackState, iCurrentId, 
											  m_pTaskStatusCallback);
				}
				else
				{
					hr = ((TASK_WITH_STATE_CALLBACK)m_pCallback)(
					    m_pTaskOrigState, iCurrentId, m_pTaskStatusCallback);
				}
			}
			else 
			{
				hr = ((TASK_CALLBACK)m_pCallback)(m_pTaskArg, iCurrentId, 
												  m_pTaskStatusCallback);
			}
		}
	
		if( hr != S_OK )
		{
			m_pTaskStatusCallback->SetTaskError(hr);
			break;
		}	
	}

	if ( InterlockedDecrement(&m_iThreadFinished) == 0 )
	{
		TaskEnd(m_pTaskStatusCallback->GetTaskError());
	}
}

//+-----------------------------------------------------------------------------
//
// the single instance of CTaskManager manager for an application
//
//------------------------------------------------------------------------------
typedef CVTSingleton<CTaskManager> CTaskManagerSingle;
CTaskManagerSingle* CTaskManagerSingle::m_singletonInstance = NULL;
LONG CTaskManagerSingle::m_uRefCount = 0;

// functions that handle startup and shutdown of the task manager
HRESULT vt::TaskManagerStartup()
{
	return CTaskManagerSingle::Startup();
}

HRESULT vt::TaskManagerShutdown()
{
	return CTaskManagerSingle::Shutdown();
}

//+-----------------------------------------------------------------------------
//
// Function: PushTask
// 
//------------------------------------------------------------------------------
HRESULT
PushTaskIntrnl(void* callback, void* pArg, ITaskState* pState,
		       CTaskStatus* pStatus, LONG count, ITaskWorkIdSequencer* pSeq,
		       bool bWait, const VT_TASK_OPTIONS* pOpts)
{
	CTaskManager* pTM = CTaskManagerSingle::GetInstance();
	TASK_PTR_RET(pTM, pStatus, E_NOINIT);
	return pTM->PushTask(callback, pArg, pState, pStatus, count, pSeq, bWait, 
						 pOpts);
}

HRESULT
vt::PushTask(TASK_CALLBACK callback, void* pArg, CTaskStatus* pStatus, 
             LONG count, const VT_TASK_OPTIONS* pOpts)
{ return PushTaskIntrnl(callback, pArg, NULL, pStatus, count, NULL, false, pOpts); }

HRESULT
vt::PushTaskAndWait(TASK_CALLBACK callback, void* pArg, CTaskStatus* pStatus,
				    LONG count, const VT_TASK_OPTIONS* pOpts)
{ return PushTaskIntrnl(callback, pArg, NULL, pStatus, count, NULL, true, pOpts); }

HRESULT
vt::PushTask(TASK_WITH_STATE_CALLBACK callback, ITaskState* pArg, 
			 CTaskStatus* pStatus, LONG count, const VT_TASK_OPTIONS* pOpts)
{ return PushTaskIntrnl(callback, NULL, pArg, pStatus, count, NULL, false, pOpts); }

HRESULT
vt::PushTaskAndWait(TASK_WITH_STATE_CALLBACK callback, ITaskState* pArg,	
					CTaskStatus* pStatus, LONG count, const VT_TASK_OPTIONS* pOpts)
{ return PushTaskIntrnl(callback, NULL, pArg, pStatus, count, NULL, true, pOpts); }


HRESULT
vt::PushTask(TASK_CALLBACK callback, void* pArg, CTaskStatus* pStatus, 
             ITaskWorkIdSequencer* pSeq, const VT_TASK_OPTIONS* pOpts)
{ return PushTaskIntrnl(callback, pArg, NULL, pStatus, 0, pSeq, false, pOpts); }

HRESULT
vt::PushTaskAndWait(TASK_CALLBACK callback, void* pArg, CTaskStatus* pStatus,
				    ITaskWorkIdSequencer* pSeq, const VT_TASK_OPTIONS* pOpts)
{ return PushTaskIntrnl(callback, pArg, NULL, pStatus, 0, pSeq, true, pOpts); }

HRESULT
vt::PushTask(TASK_WITH_STATE_CALLBACK callback, ITaskState* pArg, 
			 CTaskStatus* pStatus, ITaskWorkIdSequencer* pSeq, const VT_TASK_OPTIONS* pOpts)
{ return PushTaskIntrnl(callback, NULL, pArg, pStatus, 0, pSeq, false, pOpts); }

HRESULT
vt::PushTaskAndWait(TASK_WITH_STATE_CALLBACK callback, ITaskState* pArg,	
					CTaskStatus* pStatus, ITaskWorkIdSequencer* pSeq, 
					const VT_TASK_OPTIONS* pOpts)
{ return PushTaskIntrnl(callback, NULL, pArg, pStatus, 0, pSeq, true, pOpts); }

#endif