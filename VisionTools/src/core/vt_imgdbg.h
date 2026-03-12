#pragma once

#if defined(MAKE_DOXYGEN_WORK) || !defined(VT_WINRT)

#include "errno.h"
#include "vt_image.h"
#include "vt_taskmanager.h"

namespace vt
{
	/// \ingroup debugging
	/// <summary>Enables image debugging. This function creates a global
	/// data structure with diagnostic information about vt::CImg objects in 
	/// the calling process. 
	/// This data is read by the Image Viewer tool, 
	/// which lets you view images in any running process where image debugging is enabled. 
	/// The Image Viewer does not
	/// invade the running process in any way, so you can safely use it while running 
	/// your application, or while debugging it with Visual Studio or WinDbg.
	/// </summary>	
	/// <param name="autoAdd"> If true, all subsequently constructed vt::CImg objects are automatically registered.
	/// If false, images must be added manually via VtImageDebuggerAddImage() </param>
	/// <returns> 
	///		- S_OK on success
	///		- E_FAIL on failure 
    /// </returns>
	/// <DL><DT> Remarks: </DT></DL>
	///	- The computational overhead is neglible, but not zero.
	///	- This function is not thread safe.
	///	- Security note: the debugging data structure is visible system-wide. 
	HRESULT VtImageDebuggerStart(bool autoAdd = true);

	/// \ingroup debugging
	/// <summary>Add image to list of debugged images. </summary>
	/// <param name="img"> Image to add </param>
	/// <returns> 
	///		- S_OK on success
	///		- E_NOINIT if image debugger is not initialized 
	///		- E_FAIL if image already added or data structure is full
    /// </returns>
	/// <DL><DT> Remarks: </DT></DL>
	///		- This function is thread safe.
	HRESULT VtImageDebuggerAddImage(const vt::CImg& img);

	/// \ingroup debugging
	/// <summary>Add image to list of debugged images. </summary>
	/// <param name="img"> Image to add </param>
	/// <param name="name"> Image name (optional)</param>
	/// <returns> 
	///		- S_OK on success
	///		- E_NOINIT if image debugger is not initialized 
	///		- E_FAIL if image already added or data structure is full
    /// </returns>
	/// <DL><DT> Remarks: </DT></DL>
	///		- This function is thread safe.
	HRESULT VtImageDebuggerAddImage(const vt::CImg& img, 
		const char* name);

	/// \ingroup debugging
	/// <summary>Add image to list of debugged images. </summary>
	/// <param name="img"> Image to add </param>
	/// <param name="name"> Image name for debugging (optional)</param>
	/// <returns> 
	///		- S_OK on success
	///		- E_NOINIT if image debugger is not initialized
	///		- E_INVALIDARG if wchar_t* to char* conversion failed
	///		- E_FAIL if image already added or data structure is full
    /// </returns>
	/// <DL><DT> Remarks: </DT></DL>
	///		- This function is thread safe.
	HRESULT VtImageDebuggerAddImage(const vt::CImg& img, 
		const wchar_t* name);
	
	/// \ingroup debugging
	/// <summary>Set image name for debugging. </summary>
	/// <param name="img"> Image </param>
	/// <param name="name"> Image name </param>
	/// <returns> 
	///		- S_OK on success
	///		- E_NOINIT if image debugger is not initialized
    /// </returns>
	HRESULT VtImageDebuggerSetImageName(const CImg& img, const char* name);

	/// \ingroup debugging
	/// <summary>Set image name for debugging. </summary>
	/// <param name="img"> Image </param>
	/// <param name="name"> Image name </param>
	/// <returns> 
	///		- S_OK on success
	///		- E_NOINIT if image debugger is not initialized
    ///		- E_INVALIDARG if wchar_t* to char* conversion failed
    /// </returns>
	HRESULT VtImageDebuggerSetImageName(const CImg& img, const wchar_t* name);

	/// \ingroup debugging
	/// <summary>Remove image from debugger. </summary>
	/// <param name="img"> Image to remove </param>
	/// <returns> 
	///		- S_OK on success
	///		- E_FAIL if image was not found
    /// </returns>
	HRESULT VtImageDebuggerRemoveImage(const vt::CImg& img);
	
	namespace imgdbg
	{
		const size_t MAX_NUM_IMAGES = 1 << 12;
		const size_t MAX_IMAGE_NAME_LENGTH = 32;
		const wchar_t* const MAPPED_FILE_NAME_BASE = L"IDV";
		const size_t STACK_WALK_DEPTH = 8;
		const wchar_t* const SKIP_SYMBOL_REGEX = 
			L"vt::CSystem::+|"
			L"vt::imgdbg::TargetComm::Add|"
			L"vt::VtImageDebuggerAddImage|"
			L"vt::CImg::CImg|"
			L"vt::vector<.+>::.+|"
			L"vt::CTypedImg<.+>::CTypedImg<.+>|"
			L"vt::CCompositeImg<.+>::CCompositeImg<.+>|"
			L"vt::CImgReaderWriter<.+>::CImgReaderWriter<.+>";

		struct TargetInfo
		{
			unsigned int process_id;
			unsigned int pointer_size;
			unsigned int num_images;
		};

		struct TargetEntryData
		{
			TargetEntryData()
			{
				name[0] = '\0';
			}

			intptr_t vtimage_addr;
			unsigned int constructor_time_stamp;
			unsigned int constructor_thread_id;
			intptr_t constructor_call_stack[STACK_WALK_DEPTH];
			char name[MAX_IMAGE_NAME_LENGTH];
		};

		struct TargetEntry
		{
			unsigned int is_valid;
			TargetEntryData data;
		};

		extern bool g_debuggerActive;
		extern bool g_debuggerAutoAdd;

		class ImgDbgCritSection : public vt::CCritSection
		{
		public:
			ImgDbgCritSection()
			{
				s_bInit = true;
			}

			~ImgDbgCritSection()
			{
				s_bInit = false;
			}

			static bool IsInitialized()
			{
				return s_bInit;
			}

		private:
			static bool s_bInit;
		};
		extern ImgDbgCritSection g_commLock;

		class AutoLock
		{
		public:
			AutoLock(ImgDbgCritSection& cs)
				: m_cs(cs)
			{
				m_cs.Enter();
			}

			~AutoLock()
			{
				m_cs.Leave();
			}

		private:
			AutoLock(const AutoLock&);
			AutoLock& operator=(const AutoLock&);

		private:
			ImgDbgCritSection& m_cs;
		};

		class TargetComm
		{
		public:
			static bool Touch()
			{
				AutoLock lock(g_commLock);
				GetInstance();
				return true;
			}

			static bool Add(const vt::CImg& img);
			static bool SetName(const vt::CImg& img, const char* name);
			static bool Delete(const vt::CImg& img);

			// introduced to accomodate vt::vector's copying behavior
			static bool SetValidFlag(const void* address, bool is_valid);
			static bool MoveAddress(const void* old_address, 
				const void* new_address);			

		private:
			static TargetComm& GetInstance()
			{
				static TargetComm instance;

				instance.Initialize();

				return instance;
			}

			static bool IsBadApp();

		private:
			TargetComm()
				: mapped_file_(nullptr), shared_memory_(nullptr), 
				initialized_(false)
			{
			}

			~TargetComm()
			{
				CleanUp();
			}

		private:
			TargetComm(const TargetComm&);
			TargetComm& operator=(const TargetComm&);

		private:
			void Initialize();
			void CleanUp();

		private:
			TargetInfo& GetInfo()
			{
				return *reinterpret_cast<TargetInfo*>(shared_memory_);
			}

			TargetEntry& GetEntry(size_t pos)
			{
				return *(reinterpret_cast<TargetEntry*>(shared_memory_ + 
					sizeof(TargetInfo)) + pos);
			}

			// returns num_images if not found
			size_t FindEntry(intptr_t vtimage_addr)
			{
				const size_t num = GetInfo().num_images;
				const TargetEntry* ptr = &GetEntry(0);

				for (size_t i = 0; i < num; ++i)
				{
					if (ptr[i].data.vtimage_addr == vtimage_addr)
						return i;
				}

				return num;
			}

		private:
			bool initialized_;
			HANDLE mapped_file_;
			unsigned char* shared_memory_;
			unsigned int constructor_counter_;
		};

		// introduced to accomodate vt::vector's copying behavior.
		// these are non-member versions of the corresponding TargetComm member
		// functions, so that they can be forward-declared
		bool SetValidFlag(const void* address, bool is_valid);
		bool MoveAddress(const void* old_address, const void* new_address);
	}
}

#endif
