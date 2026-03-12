//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//------------------------------------------------------------------------
#include "stdafx.h"

#if defined(MAKE_DOXYGEN_WORK) || !defined(VT_WINRT)

#include "vt_imgdbg.h"

bool vt::imgdbg::g_debuggerActive = false;
vt::imgdbg::ImgDbgCritSection vt::imgdbg::g_commLock;
bool vt::imgdbg::ImgDbgCritSection::s_bInit = false;

HRESULT vt::VtImageDebuggerStart(bool autoAdd)
{
	VT_HR_BEGIN();

	if (FAILED(vt::imgdbg::g_commLock.Startup()))
		return E_FAIL;

	if (autoAdd)
		vt::InstallDebugHookCImgConstructor(vt::imgdbg::TargetComm::Add);

	vt::InstallDebugHookCImgDestructor(vt::imgdbg::TargetComm::Delete);

	VT_HR_EXIT(vt::imgdbg::TargetComm::Touch() ? S_OK : E_FAIL);

	vt::imgdbg::g_debuggerActive = true;

	VT_HR_END();
}

HRESULT vt::VtImageDebuggerSetImageName(const vt::CImg& img, 
	const wchar_t* name)
{
	VT_HR_BEGIN();

	vt::string str;
	if (FAILED(vt::VtWideCharToMultiByte(str, name)))
		return E_INVALIDARG;

	VT_HR_EXIT(vt::VtImageDebuggerSetImageName(img, str.get_constbuffer()));

	VT_HR_END();
}

HRESULT vt::VtImageDebuggerSetImageName(const vt::CImg& img, const char* name)
{
	VT_HR_BEGIN();

	if (!vt::imgdbg::g_debuggerActive)
		return E_NOINIT;

	VT_HR_EXIT(vt::imgdbg::TargetComm::SetName(img, name) ? S_OK : E_FAIL);

	VT_HR_END();
}

HRESULT vt::VtImageDebuggerAddImage(const vt::CImg& img)
{
	return vt::VtImageDebuggerAddImage(img, (wchar_t*)NULL);
}

HRESULT vt::VtImageDebuggerAddImage(const vt::CImg& img, const wchar_t* name)
{
	VT_HR_BEGIN();

	vt::string str;
	if (FAILED(vt::VtWideCharToMultiByte(str, name)))
		return E_INVALIDARG;

	VT_HR_EXIT(vt::VtImageDebuggerAddImage(img, str.get_constbuffer()));

	VT_HR_END();
}

HRESULT vt::VtImageDebuggerAddImage(const vt::CImg& img, const char* name)
{
	VT_HR_BEGIN();

	VT_HR_EXIT(vt::imgdbg::g_debuggerActive ? S_OK : E_NOINIT);

	VT_HR_EXIT(vt::imgdbg::TargetComm::Add(img) ? S_OK : E_FAIL);

	VT_HR_EXIT(vt::VtImageDebuggerSetImageName(img, name));

	VT_HR_END();
}

HRESULT vt::VtImageDebuggerRemoveImage(const vt::CImg& img)
{
	VT_HR_BEGIN();

	VT_HR_EXIT(vt::imgdbg::g_debuggerActive ? S_OK : E_NOINIT);

	VT_HR_EXIT(vt::imgdbg::TargetComm::Delete(img) ? S_OK : E_FAIL);

	VT_HR_END();
}

void vt::imgdbg::TargetComm::Initialize()
{
	if (initialized_)
		return;

	// Log(L"TargetComm init");

	wchar_t file_name[256];
	swprintf_s(file_name, L"_%s_%u_", MAPPED_FILE_NAME_BASE, 
		GetCurrentProcessId());

	mapped_file_ = CreateFileMappingW(INVALID_HANDLE_VALUE, 
		NULL, PAGE_READWRITE, 0, 
		sizeof(TargetInfo) + sizeof(TargetEntry) * MAX_NUM_IMAGES, 
		file_name);

	VT_ASSERT(GetLastError() != ERROR_ALREADY_EXISTS);
	VT_ASSERT(mapped_file_ != nullptr);

	if (mapped_file_ == nullptr || 
		GetLastError() == ERROR_ALREADY_EXISTS)
	{ 
		VT_DEBUG_LOG(L"Error in CreateFileMapping()\n");
		CleanUp();
		return;
	}

	shared_memory_ = reinterpret_cast<unsigned char*>(MapViewOfFile(mapped_file_, 
		FILE_MAP_ALL_ACCESS, 0, 0, 0));

	VT_ASSERT(shared_memory_ != nullptr);

	if (shared_memory_ == nullptr)
	{
		VT_DEBUG_LOG(L"Error in MapViewOfFile()\n");
		CleanUp();
		return;
	}

	// Note: race condition between target and debugger here: if TargetInfo 
	// partially initialized while debugger is reading. Not sure if this 
	// justifies a system-wide lock.
	TargetInfo& ti = GetInfo();
	ti.process_id = GetCurrentProcessId();
	ti.pointer_size = sizeof(intptr_t);
	ti.num_images = 0;

	constructor_counter_ = 0;

	initialized_ = true;
}

void vt::imgdbg::TargetComm::CleanUp()
{
	if (!initialized_)
		return;

	// Log(L"TargetComm exit");

	if (shared_memory_ != nullptr)
		UnmapViewOfFile(shared_memory_);

	if (mapped_file_ != nullptr)
		CloseHandle(mapped_file_);

	initialized_ = false;
}

#define IMGDBG_GET_SAFE_INST() \
	if (!g_commLock.IsInitialized()) \
	{ \
		VT_DEBUG_LOG("WARNING: image debugger critical section not initialized! "\
		"This can happen if global image objects get destroyed AFTER "\
		"the global image debugger runtime has shut down.\n");\
		return false;\
	}\
	AutoLock lock(g_commLock); \
	VT_ASSERT(!IsBadApp());\
	if (IsBadApp()) return false;\
	TargetComm& inst = GetInstance();

bool vt::imgdbg::TargetComm::Add(const vt::CImg& img)
{	
	IMGDBG_GET_SAFE_INST();

	unsigned int& num = inst.GetInfo().num_images;

	VT_ASSERT(num < MAX_NUM_IMAGES);
	if (num >= MAX_NUM_IMAGES)
	{
		VT_DEBUG_LOG(L"IMGDBG: Image buffer full\n");
		return false;
	}

	const intptr_t ptr = reinterpret_cast<intptr_t>(&img);
	if (inst.FindEntry(ptr) < num)
	{
		// VT_ASSERT(!"Image already added");
		VT_DEBUG_LOG(L"IMGDBG: Image already added\n");
		return false;
	}

	TargetEntry& new_entry = inst.GetEntry(num);
	new_entry.data.constructor_thread_id = GetCurrentThreadId();
	new_entry.data.constructor_time_stamp = 
		inst.constructor_counter_++;

	for (size_t i=0; i<STACK_WALK_DEPTH; ++i)
		new_entry.data.constructor_call_stack[i] = 0;

	CaptureStackBackTrace(0, STACK_WALK_DEPTH,
		reinterpret_cast<void**>(
		new_entry.data.constructor_call_stack), 
		NULL);

	new_entry.data.vtimage_addr = reinterpret_cast<intptr_t>(&img);
	new_entry.data.name[0] = '\0';

	new_entry.is_valid = 1;		
	++num;

	// VT_DEBUG_LOG(L"%08X inserted. %d entries", 
	// inst.GetEntry(num-1).vtimage_addr, num);

	return true;
}

bool vt::imgdbg::TargetComm::SetName(const vt::CImg& img, const char* name)
{
	IMGDBG_GET_SAFE_INST();

	// Just return if not found. It could be e.g. VtLoadImage()
	// that tries to set the name in non-autoAdd mode on an
	// image that hasnt been explicitly added
	size_t pos = inst.FindEntry(reinterpret_cast<intptr_t>(&img));
	if (pos >= inst.GetInfo().num_images)
		return false;

	if (name != NULL)
	{
		errno_t e = strncpy_s(inst.GetEntry(pos).data.name, name, 
			_TRUNCATE);

		if (e != 0 && e != STRUNCATE)
		{						
			VT_DEBUG_LOG("IMGDBG: Error setting image name\n");						
			return false;
		}
	}	
	else
	{
		inst.GetEntry(pos).data.name[0] = '\0';
	}

	return true;
}

bool vt::imgdbg::TargetComm::Delete(const vt::CImg& img)
{
	IMGDBG_GET_SAFE_INST();

	TargetInfo& ti = inst.GetInfo();

	const intptr_t del = reinterpret_cast<intptr_t>(&img);
	size_t del_pos = inst.FindEntry(del);
	// "not found" case must be ignored since the deletion hook
	// in CImg::~CImg() calls this for all images, even once that
	// were not added, e.g. global static images, or any image not 
	// specifically added while in non-autoAdd mode
	if (del_pos >= ti.num_images)
		return false;

	TargetEntry& src = inst.GetEntry(ti.num_images - 1);
	TargetEntry& dst = inst.GetEntry(del_pos);
	// mark invalid and copy data
	dst.is_valid = 0;
	dst.data = src.data;
	// Note: race condition between target and debugger: at this 
	// point there are two copies of the same entry. 
	// -> debugger client must check the is_valid flag!
	src.is_valid = 0;
	src.data.name[0] = '\0';
	--ti.num_images;

	// VT_DEBUG_LOG(L"%08X deleted. %d entries", del, 
	// ti.num_images);

	return true;
}

// introduced to accomodate vt::vector's copying behavior
bool vt::imgdbg::TargetComm::SetValidFlag(const void* address, bool is_valid)
{
	IMGDBG_GET_SAFE_INST();

	TargetInfo& ti = inst.GetInfo();

	const intptr_t mv = 
		reinterpret_cast<const intptr_t>(address);
	size_t mv_pos = inst.FindEntry(mv);
	// "not found" is ignored
	if (mv_pos >= ti.num_images)
		return false;

	TargetEntry& img = inst.GetEntry(mv_pos);
	img.is_valid = is_valid ? 1 : 0;

	return true;

}

// introduced to accomodate vt::vector's copying behavior
bool vt::imgdbg::TargetComm::MoveAddress(const void* old_address, 
	const void* new_address)
{
	IMGDBG_GET_SAFE_INST();

	TargetInfo& ti = inst.GetInfo();

	const intptr_t mv = 
		reinterpret_cast<const intptr_t>(old_address);
	size_t mv_pos = inst.FindEntry(mv);
	// "not found" is ignored
	if (mv_pos >= ti.num_images)
		return false;

	TargetEntry& img = inst.GetEntry(mv_pos);
	// note: debugger/debuggee race condition: debugger could
	// already be reading the changing vtimage_addr when we mark
	// it invalid. hopefully doesnt happen often, though.
	img.data.vtimage_addr = 
		reinterpret_cast<const intptr_t>(new_address);

	return true;
}

bool vt::imgdbg::TargetComm::IsBadApp()
{
	static bool is_bad = true;
	static bool already_checked = false;

	if (!already_checked)
	{
		wchar_t buf[256];
		GetModuleFileNameW(NULL, buf, _countof(buf));
		_wcslwr_s(buf);
		const wchar_t* p = wcsstr(buf, L"blend.exe"); 
		is_bad = p!=NULL && (p==buf || p[-1]==L'\\' || p[-1]==L'/');
		already_checked = true;
	}

	return is_bad;
}

#undef IMGDBG_GET_SAFE_INST

bool vt::imgdbg::SetValidFlag(const void* address, bool is_valid)
{
	return vt::imgdbg::TargetComm::SetValidFlag(address, is_valid);
}

bool vt::imgdbg::MoveAddress(const void* old_address, const void* new_address)
{
	return vt::imgdbg::TargetComm::MoveAddress(old_address, new_address);
}

#endif