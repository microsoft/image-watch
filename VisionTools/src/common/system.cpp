// BEGIN RAZZLE STL 110 BUG WORKAROUND (build team referred to this bug: http://bugcheck/bugs/Windows8Bugs/38025)
#include <crtdefs.h>
#   ifndef __STR2WSTR 
#     define __STR2WSTR(str)    L##str
#   endif
#   ifndef _STR2WSTR
#     define _STR2WSTR(str)     __STR2WSTR(str)
#   endif
#   ifndef __FILEW__
#     define __FILEW__          _STR2WSTR(__FILE__)
#   endif
#   ifndef __FUNCTIONW__
#     define __FUNCTIONW__          _STR2WSTR(__FUNCTION__)
#   endif

_CRTIMP __declspec(noreturn)
void __cdecl _invoke_watson(_In_opt_z_ const wchar_t *, _In_opt_z_ const wchar_t *, _In_opt_z_ const wchar_t *, _In_ unsigned int, _In_ uintptr_t);

#ifdef _DEBUG 
 #ifndef _CRT_SECURE_INVALID_PARAMETER
  #define _CRT_SECURE_INVALID_PARAMETER(expr) ::_invalid_parameter(__STR2WSTR(#expr), __FUNCTIONW__, __FILEW__, __LINE__, 0)
#endif
#else
/* By default, _CRT_SECURE_INVALID_PARAMETER in retail invokes _invalid_parameter_noinfo_noreturn(),
  * which is marked __declspec(noreturn) and does not return control to the application. Even if 
  * _set_invalid_parameter_handler() is used to set a new invalid parameter handler which does return
  * control to the application, _invalid_parameter_noinfo_noreturn() will terminate the application and
  * invoke Watson. You can overwrite the definition of _CRT_SECURE_INVALID_PARAMETER if you need.
  *
  * _CRT_SECURE_INVALID_PARAMETER is used in the Standard C++ Libraries and the SafeInt library.
  */
#ifndef _CRT_SECURE_INVALID_PARAMETER
  #define _CRT_SECURE_INVALID_PARAMETER(expr) ::_invalid_parameter_noinfo_noreturn()
#endif
#endif
// END RAZZLE WORKAROUND

#if defined(FORCE_ENABLE_DESKTOP_WINAPI)
#undef WINAPI_FAMILY
#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP
#endif

#if defined(VT_WINRT)

#if !defined(FORCE_ENABLE_DESKTOP_WINAPI)
#include <thread>
#endif

#include <ppltasks.h>

#else

#include <concrt.h>

#endif

#ifdef _MSC_VER 

#include "vt_dbg.h"
#include "vt_error.h"
#include "vt_new.h"
#include "vt_system.h"

#pragma comment(lib, "ole32")
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY!=WINAPI_FAMILY_PHONE_APP)
#pragma comment(lib, "advapi32")
#endif

HANDLE vt::CSystem::CreateMutexW(const WCHAR* name)
{
#if defined(VT_WINRT)
	// CreateMutexEx is available on Vista and later; required for WinRT.
	HANDLE hMutex = ::CreateMutexExW(NULL, name, 0, SYNCHRONIZE);
#else
	HANDLE hMutex = ::CreateMutexW(NULL, FALSE, name);
#endif
	return hMutex;
}

HANDLE vt::CSystem::CreateEvent(BOOL bManualReset, BOOL bInitialState)
{
#if defined(VT_WINRT)
	// CreateEventEx is available on Vista and later; required for WinRT.
	DWORD dwFlags = (bManualReset ? CREATE_EVENT_MANUAL_RESET : 0) | 
		(bInitialState ? CREATE_EVENT_INITIAL_SET : 0);
	HANDLE hEvent = ::CreateEventEx(NULL, NULL, dwFlags, SYNCHRONIZE | 
		EVENT_MODIFY_STATE);
#else
	HANDLE hEvent = ::CreateEvent(NULL, bManualReset, bInitialState, 
		NULL);
#endif
	return hEvent;
}

DWORD vt::CSystem::WaitForSingleObjectEx(HANDLE hHandle, 
	DWORD dwMilliseconds, BOOL bAlertable)
{
	// Available XP through Win8
	return ::WaitForSingleObjectEx(hHandle, dwMilliseconds, bAlertable);
}

BOOL vt::CSystem::InitializeCriticalSection(CRITICAL_SECTION* lpCriticalSection,
			DWORD dwSpinCount)
{
#ifdef VT_WINRT
		return InitializeCriticalSectionEx(lpCriticalSection, dwSpinCount, 0);
#else
		return InitializeCriticalSectionAndSpinCount(lpCriticalSection, 
			dwSpinCount);
#endif
}

HRESULT vt::CSystem::CoCreateInstance(GUID rclsid, IUnknown* pUnkOuter,
	DWORD dwClsContext, GUID riid, void** ppv)
{

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP)
	// the RPAL SDK replaces CoCreateInstance in a specific way so call it directly 
	// instead of the wrapper
	HRESULT  hr = ::CoCreateInstance(rclsid, (IUnknown*) pUnkOuter,
		dwClsContext, riid, (LPVOID*) ppv);
#else
	MULTI_QI mqi;
	mqi.hr = 0;
	mqi.pItf = NULL;
	mqi.pIID = &riid;

#if defined(VT_WINRT)
	HRESULT hr = ::CoCreateInstanceFromApp(rclsid, (IUnknown*)pUnkOuter, 
		dwClsContext, NULL, 1, &mqi);
#else
	HRESULT hr = ::CoCreateInstanceEx(rclsid, (IUnknown*)pUnkOuter, 
		dwClsContext, NULL, 1, &mqi);
#endif
	*ppv = mqi.pItf;

#endif

	

	return hr;
}

void vt::CSystem::GetSystemInfo(SYSTEM_INFO* si)
{
	return ::GetNativeSystemInfo(si);
}

BOOL vt::CSystem::GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize)
{
#if defined(VT_WINRT)
	FILE_STANDARD_INFO fileStandardInfo;
	BOOL succeeded = ::GetFileInformationByHandleEx(hFile, FileStandardInfo, &fileStandardInfo, sizeof(FILE_STANDARD_INFO));
	*lpFileSize = fileStandardInfo.EndOfFile;
	return succeeded;
#else
	return ::GetFileSizeEx(hFile, lpFileSize);
#endif
}

DWORD vt::CSystem::GetTempPath(_In_ DWORD nBufferLength, _Out_writes_to_opt_(nBufferLength, return + 1) LPWSTR lpBuffer)
{
#if defined(VT_WINRT)
	if (lpBuffer == NULL)
	{
		SetLastError(ERROR_BAD_ARGUMENTS);
		return 0;
	}
#if defined(WINAPI_FAMILY_PHONE_APP) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
	// Windows Phone 8 does not have a temporary folder, so we use the local folder.
	Platform::String^ path = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
#else
	Platform::String^ path = Windows::Storage::ApplicationData::Current->TemporaryFolder->Path;
#endif
	wcscpy_s(lpBuffer, nBufferLength, path->Data());
	return path->Length();
#else
	return ::GetTempPath(nBufferLength, lpBuffer);
#endif
}

UINT vt::CSystem::GetTempFileName(_In_ LPCWSTR lpPathName, _In_ LPCWSTR lpPrefixString, _In_ UINT uUnique, _Out_writes_(MAX_PATH) LPWSTR lpTempFileName)
{
#if defined(VT_WINRT)
	if (lpPathName == NULL || lpPrefixString == NULL || lpTempFileName == NULL)
	{
		if (lpTempFileName)
		{
			*lpTempFileName = 0;
		}
		SetLastError(ERROR_BAD_ARGUMENTS);
		return 0;
	}

	if (uUnique > 0)
	{
		WCHAR* format = L"%s\\%s%04x.TMP";
		int count = _scwprintf(format, lpPathName, lpPrefixString, uUnique);
		if (count >= MAX_PATH)
		{
			*lpTempFileName = 0;
			SetLastError(ERROR_BUFFER_OVERFLOW);
			return 0;
		}
		else
		{
			if (_snwprintf_s(lpTempFileName, MAX_PATH, MAX_PATH, format, lpPathName, lpPrefixString, uUnique) < 0)
			{
				return 0;
			}
			return 1;
		}
	}
	else
	{
		GUID guid = { 0 };
		CoCreateGuid(&guid); 
		WCHAR* format = L"%s\\%s%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x";
		int count = _scwprintf(format, lpPathName, lpPrefixString, guid.Data1, guid.Data2, guid.Data3, UINT(guid.Data4[0]), UINT(guid.Data4[1]),
			UINT(guid.Data4[2]), UINT(guid.Data4[3]), UINT(guid.Data4[4]), UINT(guid.Data4[5]), UINT(guid.Data4[6]), UINT(guid.Data4[7]));
		if (count >= MAX_PATH)
		{
			*lpTempFileName = 0;
			SetLastError(ERROR_BUFFER_OVERFLOW);
			return 0;
		}
		else
		{
			if (_snwprintf_s(lpTempFileName, MAX_PATH, MAX_PATH, format, lpPathName, lpPrefixString, guid.Data1, guid.Data2, guid.Data3, UINT(guid.Data4[0]), UINT(guid.Data4[1]),
				UINT(guid.Data4[2]), UINT(guid.Data4[3]), UINT(guid.Data4[4]), UINT(guid.Data4[5]), UINT(guid.Data4[6]), UINT(guid.Data4[7])) < 0)
			{
				return 0;
			}
			return 1;
		}
	}
#else
	return ::GetTempFileName(lpPathName, lpPrefixString, uUnique, lpTempFileName);
#endif
}

HANDLE vt::CSystem::CreateFile(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
#if defined(VT_WINRT)
	CREATEFILE2_EXTENDED_PARAMETERS createExParams;
	createExParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	createExParams.dwFileAttributes = dwFlagsAndAttributes & 0x0000FFFF;
	createExParams.dwFileFlags = dwFlagsAndAttributes & 0xFFF80000;
	createExParams.dwSecurityQosFlags = dwFlagsAndAttributes & 0x001F0000;	
	createExParams.lpSecurityAttributes = lpSecurityAttributes; 
	createExParams.hTemplateFile = hTemplateFile;
	return ::CreateFile2(lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, &createExParams);
#else
	return ::CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
#endif
}

HANDLE vt::CSystem::CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize,
	LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
{
#if defined(VT_WINRT)
	// Ignore the following parameters:
	lpThreadAttributes;
	dwStackSize;
	dwCreationFlags;

	// Create an event handle (since we cannot get a thread handle).
	HANDLE hEvent = CSystem::CreateEvent(true, false);

	// Run the user's function on a thread pool thread, then set the event.
    auto workItemDelegate = [lpThreadId, lpStartAddress, lpParameter, hEvent](Windows::Foundation::IAsyncAction^ workItem)
    {
		if (lpThreadId != NULL)
		{
			*lpThreadId = GetCurrentThreadId();
		}
		lpStartAddress(lpParameter);
		SetEvent(hEvent);
	};
    auto handler = ref new Windows::System::Threading::WorkItemHandler(workItemDelegate);
	auto priority = Windows::System::Threading::WorkItemPriority::Normal;
	auto options = Windows::System::Threading::WorkItemOptions::None;
    auto workItem = Windows::System::Threading::ThreadPool::RunAsync(handler, priority, options);

	// Return the event handle.
	return hEvent;
#else
	return ::CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
#endif
}

void vt::CSystem::Sleep(DWORD dwMilliseconds)
{
#if !defined(FORCE_ENABLE_DESKTOP_WINAPI) && defined(VT_WINRT)
	std::chrono::milliseconds duration(dwMilliseconds);
	std::this_thread::sleep_for(duration);
#else
	::Sleep(dwMilliseconds);
#endif
}


#if defined(VT_WINRT)
void vt::CSystem::Await(Windows::Foundation::IAsyncAction^ asyncAction)
{
	// Create an event handle.
	HANDLE hEvent = CSystem::CreateEvent(true, false);
	Platform::Exception^ exception = nullptr;

	// Run the user's function on a thread pool thread, then set the event.
	auto workItemDelegate = [&asyncAction, &hEvent, &exception](Windows::Foundation::IAsyncAction^ workItem)
    {
		Concurrency::create_task(asyncAction).then([&hEvent, &exception](Concurrency::task<void> previousTask)
		{
			try
			{
				previousTask.get();
			}
			catch (Platform::Exception^ innerException)
			{
				exception = innerException;
			}
			SetEvent(hEvent);
		});
	};
    auto handler = ref new Windows::System::Threading::WorkItemHandler(workItemDelegate);
	auto priority = Windows::System::Threading::WorkItemPriority::Normal;
	auto options = Windows::System::Threading::WorkItemOptions::None;
    auto workItem = Windows::System::Threading::ThreadPool::RunAsync(handler, priority, options);

	// Wait for the event.
	CSystem::WaitForSingleObjectEx(hEvent, INFINITE, TRUE);
	CloseHandle(hEvent);

	// If an exception was thrown on the thread pool thread, re-throw it now.
	if (exception != nullptr)
	{
		throw exception;
	}
}

template <typename TResult>
TResult vt::CSystem::Await(Windows::Foundation::IAsyncOperation<TResult>^ asyncOp)
{
	// Create an event handle.
	HANDLE hEvent = CSystem::CreateEvent(true, false);
	TResult result;
	Platform::Exception^ exception = nullptr;

	// Run the user's function on a thread pool thread, then set the event.
    auto workItemDelegate = [&asyncOp, &hEvent, &result, &exception](Windows::Foundation::IAsyncAction^ workItem)
    {
		Concurrency::create_task(asyncOp).then([&hEvent, &result, &exception](Concurrency::task<TResult> previousTask)
		{
			try
			{
				result = previousTask.get();
			}
			catch (Platform::Exception^ innerException)
			{
				exception = innerException;
			}
			SetEvent(hEvent);
		});

	};
    auto handler = ref new Windows::System::Threading::WorkItemHandler(workItemDelegate);
	auto priority = Windows::System::Threading::WorkItemPriority::Normal;
	auto options = Windows::System::Threading::WorkItemOptions::None;
    auto workItem = Windows::System::Threading::ThreadPool::RunAsync(handler, priority, options);

	// Wait for the event.
	CSystem::WaitForSingleObjectEx(hEvent, INFINITE, TRUE);
	CloseHandle(hEvent);

	// If an exception was thrown on the thread pool thread, re-throw it now.
	if (exception != nullptr)
	{
		throw exception;
	}

	return result;
}

// Explicit template function instantiations:
template Windows::Storage::Streams::IRandomAccessStream^ vt::CSystem::Await(Windows::Foundation::IAsyncOperation<Windows::Storage::Streams::IRandomAccessStream^>^ asyncOp);
template Windows::Storage::StorageFile^ vt::CSystem::Await(Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFile^>^ asyncOp);
template Windows::Storage::StorageFolder^ vt::CSystem::Await(Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFolder^>^ asyncOp);
template Platform::String^ vt::CSystem::Await(Windows::Foundation::IAsyncOperation<Platform::String^>^ asyncOp);
template unsigned int vt::CSystem::Await(Windows::Foundation::IAsyncOperation<unsigned int>^ asyncOp);

#endif


//+-----------------------------------------------------------------------------
//
// Class: CThreadPool
// 
// Synopsis: the threadpool API is a thin wrapper on the Concurrency Runtime
//
//------------------------------------------------------------------------------
class vt::CSystem::CThreadPool::CThreadPoolInternal
{
public:
	CThreadPoolInternal()
#if !defined(VT_WINRT)
		: m_pScheduler(NULL)
#endif
	{}

	~CThreadPoolInternal()
	{ Shutdown(); }

	HRESULT Startup(DWORD maxthreads=0);
	HRESULT Shutdown(); 
	HRESULT SubmitWork(
		vt::CSystem::CThreadPool::THREAD_POOL_CALLBACK pfnWorkItem, void* pArg,
		LONG iCount);

protected:
#if !defined(VT_WINRT)
	Concurrency::Scheduler* m_pScheduler; 
#endif
};

//  forwarding functions from wrapper class
HRESULT
vt::CSystem::CThreadPool::Startup(DWORD maxthreads)
{
	if( m_pint == NULL )
	{
		m_pint = VT_NOTHROWNEW CThreadPoolInternal();
	}

	VT_HR_BEGIN()

	VT_PTR_OOM_EXIT( m_pint );

	VT_HR_EXIT( m_pint->Startup(maxthreads) );

	VT_HR_END()
}

HRESULT
vt::CSystem::CThreadPool::Shutdown()
{
	HRESULT hr = (m_pint==NULL)? S_OK: m_pint->Shutdown();
	delete m_pint;
    m_pint = nullptr;
	return hr;
}
	
HRESULT 
vt::CSystem::CThreadPool::SubmitWork(THREAD_POOL_CALLBACK pfnWorkItem, void* pArg, 
									 LONG iCount)
{
	return m_pint->SubmitWork(pfnWorkItem, pArg, iCount);
}

// implementation of the thread pool
#ifdef VT_WINRT

HRESULT
vt::CSystem::CThreadPool::CThreadPoolInternal::Startup(DWORD maxthreads)
{
	// we cannot control the threadpool in WINRT, so assert that maxthreads is 
	// the default
	VT_ASSERT( maxthreads == 0 ); 
#ifndef _DEBUG
    maxthreads;
#endif
	return S_OK;
}

HRESULT 
vt::CSystem::CThreadPool::CThreadPoolInternal::Shutdown()
{ return S_OK; }

HRESULT
vt::CSystem::CThreadPool::CThreadPoolInternal::SubmitWork(
	THREAD_POOL_CALLBACK pfnWorkItem, void* pArg, LONG iCount)
{
	using namespace Windows::System::Threading;
    using namespace Windows::Foundation;

	HRESULT hr = S_OK;
	try
	{  
	    for( int i = 0; i < iCount; i++ )
		{
			ThreadPool::RunAsync(
				ref new WorkItemHandler([=] (IAsyncAction^ operation) {pfnWorkItem(pArg);},
			    Platform::CallbackContext::Any));
	        //auto t = Concurrency::create_task([=](){ pfnWorkItem(pArg); });
		}
	}
	catch(...)
	{
		// TODO: translate exceptions to HRs
		hr = E_FAIL;
	}
	return hr;
}

#else

HRESULT
vt::CSystem::CThreadPool::CThreadPoolInternal::Startup(DWORD maxthreads)
{ 
	VT_HR_BEGIN()

	VT_HR_EXIT( Shutdown() );

	VT_ASSERT( m_pScheduler == NULL );

	try
	{  
		Concurrency::SchedulerPolicy policy;
		policy.SetConcurrencyLimits(
			1, (maxthreads==0)? Concurrency::MaxExecutionResources: maxthreads);
		m_pScheduler = Concurrency::Scheduler::Create(policy);  
    } 
	catch(...)
	{
		// TODO: translate exceptions to HRs
		hr = E_FAIL;
	}
	
	VT_HR_END()
}

HRESULT 
vt::CSystem::CThreadPool::CThreadPoolInternal::Shutdown()
{
	if( m_pScheduler )
	{
		m_pScheduler->Release();
		m_pScheduler = NULL;
    }
	return S_OK;
}

struct POOL_CALLBACK_INFO
{
	vt::CSystem::CThreadPool::THREAD_POOL_CALLBACK pfnWorkItem;
	void* pArg;
	Concurrency::Context* pContext;
	LONG  iCount;
	LONG  iFirst;
};

static void PoolTask(void *pArg)
{
	POOL_CALLBACK_INFO* pInfo = (POOL_CALLBACK_INFO*)pArg;

	if( InterlockedIncrement(&pInfo->iFirst)==1 )
	{
		VT_ASSERT( pInfo->pContext );
		ANALYZE_ASSUME( pInfo->pContext );
		pInfo->pContext->Unblock();
	}

	pInfo->pfnWorkItem(pInfo->pArg);

	if( InterlockedDecrement(&pInfo->iCount)==0 )
	{
		delete pInfo;
	}
}
	
HRESULT
vt::CSystem::CThreadPool::CThreadPoolInternal::SubmitWork(
	THREAD_POOL_CALLBACK pfnWorkItem, void* pArg, LONG iCount)
{
	VT_HR_BEGIN()

	VT_HR_EXIT( (m_pScheduler==NULL)? E_NOINIT: S_OK );

	try
	{
		// if we submit work on a thread that is a member of this threadpool
		// then add another thread to the pool (assumption is that the submitting
		// thread is not going to do much work). This is necessary to avoid
		// a deadlock. The extra thread is added via the Oversubscribe calls
		// below.
		// 
		// TODO: if ScheduleTask fails (stepping through there seem to be
		//       several exceptions potentially thrown) then needs to
		//       wait on those tasks that were submitted
		if( Concurrency::CurrentScheduler::Get() != m_pScheduler )
		{
			for( int i = 0; i < iCount; i++ )
			{
				m_pScheduler->ScheduleTask(pfnWorkItem, pArg);
			}
		}
		else
		{
			POOL_CALLBACK_INFO* pInfo = new POOL_CALLBACK_INFO;
	
			Concurrency::Context* pContext = Concurrency::Context::CurrentContext();
	
			pInfo->pContext    = pContext;
			pInfo->iFirst      = 0;
			pInfo->iCount      = iCount;
			pInfo->pfnWorkItem = pfnWorkItem;
			pInfo->pArg        = pArg;
	
			Concurrency::Context::Oversubscribe(true);

			for( int i = 0; i < iCount; i++ )
			{
				m_pScheduler->ScheduleTask(PoolTask, pInfo);
			}
	
			if( pContext )
			{
				// block waits until the first task has started (that's when
				// we know that Oversubscribe() has been succesful.
				pContext->Block();
				Concurrency::Context::Oversubscribe(false);
			}
		}
	} 
	catch(...)
	{
		// TODO: cleanup pInfo
		
		// TODO: translate exceptions to HRs
		hr = E_FAIL;
	}

	VT_HR_END()
}
#endif

#endif