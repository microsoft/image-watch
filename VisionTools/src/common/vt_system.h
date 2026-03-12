//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Wrappers for operating system calls.
// 
//  History:
//      2012/05/23-ericsto
//          Created
//
//------------------------------------------------------------------------

#pragma once

// so far the functions in this header are only available on Windows
#ifdef _MSC_VER 

#include "vt_basetypes.h"
#include <Unknwn.h>

namespace vt 
{	
	class CSystem
	{
	public:
		// synchronization
		static HANDLE CreateMutexW(const WCHAR* name);	
#if defined(FORCE_ENABLE_DESKTOP_WINAPI)
#undef CreateEvent
#endif
		static HANDLE CreateEvent(BOOL bManualReset, BOOL bInitialState);
		static DWORD WaitForSingleObjectEx(HANDLE hHandle, DWORD dwMilliseconds, 
			BOOL bAlertable);
		static BOOL InitializeCriticalSection(CRITICAL_SECTION* lpCriticalSection,
			DWORD dwSpinCount);

		// COM
		static HRESULT CoCreateInstance(GUID rclsid, IUnknown* pUnkOuter,
			DWORD dwClsContext, GUID riid, void** ppv);

		// misc
		static void GetSystemInfo(SYSTEM_INFO* si);

		// file I/O
		static BOOL GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);
#if defined(FORCE_ENABLE_DESKTOP_WINAPI)
#undef GetTempPath
#endif
		static DWORD GetTempPath(_In_ DWORD nBufferLength, _Out_writes_to_opt_(nBufferLength, return + 1) LPWSTR lpBuffer);
#if defined(FORCE_ENABLE_DESKTOP_WINAPI)
#undef GetTempFileName
#endif
		static UINT GetTempFileName(_In_ LPCWSTR lpPathName, _In_ LPCWSTR lpPrefixString, _In_ UINT uUnique, _Out_writes_(MAX_PATH) LPWSTR lpTempFileName);
#if defined(FORCE_ENABLE_DESKTOP_WINAPI)
#undef CreateFile
#endif
		static HANDLE CreateFile(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
			LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
			DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

		// threading
		static HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize,
			LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
		static void Sleep(DWORD dwMilliseconds);

		// awaiting asynchronous tasks
#if defined(VT_WINRT)
		static void Await(Windows::Foundation::IAsyncAction^ asyncAction);

		template <typename TResult>
		static TResult Await(Windows::Foundation::IAsyncOperation<TResult>^ asyncOp);
#endif

	public:
		class CThreadPool
		{
		public:
			typedef void (__cdecl * THREAD_POOL_CALLBACK)(void *);

		public:
			CThreadPool(): m_pint(NULL)
			{}
	
			~CThreadPool()
			{ Shutdown(); }
			
			HRESULT Startup(DWORD maxthreads=0);
			HRESULT Shutdown(); 
			HRESULT SubmitWork(THREAD_POOL_CALLBACK pfnWorkItem,
							   void* pArg, LONG iCount);
	
		protected:
			class CThreadPoolInternal;
			CThreadPoolInternal* m_pint;
		};
	};
};
#endif
