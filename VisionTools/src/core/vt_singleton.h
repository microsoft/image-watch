//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Base class for singleton objects.
// 
//  History:
//      2007/06/1-mattu
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"

namespace vt {

template <class T>
class CVTSingleton: public T
{
public:
	CVTSingleton() 
	{}
    ~CVTSingleton() 
    {}

	static HRESULT Startup()
    { return SingletonOp(eSingletonOpStartup); }

	static HRESULT Shutdown()
    { return SingletonOp(eSingletonOpShutdown); }

    static T* GetInstance()
    { return m_singletonInstance; }

protected:
    enum eSingletonOp
    {
        eSingletonOpStartup  = 0,
        eSingletonOpShutdown = 1
    };

    static HRESULT SingletonOp(eSingletonOp op);

protected:
	static LONG             m_uRefCount;
    static CVTSingleton<T>* m_singletonInstance;
};

template <class T>
HRESULT CVTSingleton<T>::SingletonOp(eSingletonOp op)
{
	HRESULT hr = S_OK;

	// The CreateMutex solution for the lock is described in the 
	// "Windows: Named-Mutex Solution" of this article:
	// http://drdobbs.com/article/print?articleId=199203083&siteSectionName=    
	//

    //
    // Create a named mutex.  there can be only on instance of this in 
    // the system. but since this only locks once when a VT app is starting
    // up it shouldn't cause perf. issues due to blocking on it.
    //
    // All named mutexs are system wide - no way to create one of these
    // just in the scope of the current process.  The Local\ name is a 
    // small optimization that limits the scope of the mutex to just the 
    // current user's session.
    //
    HANDLE hMutex = CSystem::CreateMutexW(L"Local\\Microsoft:VisionTools:GlobalsMutex");
    if (hMutex == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // wait for access.  if the wait is successful we have an exclusive
    // lock on the one 'VisionTools:Globals' mutex
    DWORD dwStatus = vt::CSystem::WaitForSingleObjectEx(hMutex, INFINITE, FALSE);
    if (dwStatus == WAIT_OBJECT_0)
    {
        if( op == eSingletonOpShutdown )
        {
            if( m_uRefCount == 0 )
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                m_uRefCount--;
                if( m_uRefCount == 0 )
                {
                    delete m_singletonInstance;
                    m_singletonInstance = NULL;
                }
            }
        }
        else
        {
            // check for that this is still NULL, if it isn't another thread
            // created the singleton first
            if (m_singletonInstance == NULL)
            {
                VT_ASSERT( m_uRefCount == 0 );

                CVTSingleton<T>* pS = VT_NOTHROWNEW CVTSingleton<T>();
                if (pS == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    hr = pS->StartupInternal();
                    if (hr == S_OK)
                    {
                        m_singletonInstance = pS;
                        m_uRefCount = 1;
                    }
                    else
                    {
                        delete pS;
                    }
                }
            }
            else
            {
                m_uRefCount++;
            }
        }
    }
    else
    {
        hr = (dwStatus == WAIT_FAILED) ?
            HRESULT_FROM_WIN32(GetLastError()) : E_UNEXPECTED;
    }

    // we are done with the mutex
    ReleaseMutex(hMutex);

    CloseHandle(hMutex);

	return hr;
}

};
