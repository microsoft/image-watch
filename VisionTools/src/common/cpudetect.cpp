//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for detecting processor capabilities
//
//  History:
//      2004/11/08-swinder
//			Created
//
//------------------------------------------------------------------------

#include "vt_dbg.h"
#include "vt_cpudetect.h"

#if defined(_MSC_VER)
#if (defined(_M_IX86)||defined(_M_AMD64))
#include "immintrin.h"
#endif
#elif defined(__clang__)
#if (defined(_M_IX86)||defined(_M_AMD64))
#include <cpuid.h>
#endif
#else
#error "unknown compiler"
#endif

#if defined(VT_ANDROID)
#include <cpu-features.h>
#endif

using namespace vt;

//***************************************************************************
// Global Variables
//***************************************************************************
static int g_iCPUDetected = 0;

#if defined(VT_ANDROID)
AndroidCpuFamily g_androidCpuFamily;
uint64_t g_androidCpuFeatures;
#elif defined(VT_OSX)
// TODO
#else
// CPUID EAX = 0
static int g_iCPUID_MaxLvl = 0;
static int g_iCPUID_VendorID_ebx = 0;
static int g_iCPUID_VendorID_edx = 0;
static int g_iCPUID_VendorID_ecx = 0;

// CPUID EAX = 1
static int g_iCPUID_Signature = 0;
static int g_iCPUID_FeatureFlags = 0;
static int g_iCPUID_FeatureFlags2 = 0;
static int g_iCPUID_BrandID = 0;
#endif

/***********************************************************************/
// Function CPUIDInstruction() is effectively the intrinsic __cpuid().
//
// Parameters:
//     iRegValOut [in]:  An integer array accepting the values of EAX, EBX,
//                       ECX and EDX registers after calling CPUID.
//     iEAXValIn [in]:   The integer value stored in EAX register before
//                       calling CPUID instruction.
// Return Values:
//     None.
// Comment:
//     In theory this function can be fully replaced with the intrinsic
//     __cpuid() for X64.  However, there seems to be a bug that the intrinsic
//     does not clear ECX register before calling CPUID (uninitialized), and on
//     some Intel processors this could be a problem.  Before that an X64
//     Assembly CPUID_ECX_Clearedasm() is implemented as a temporary fix.
//
//     Intel published the document "CPUID for x64 Platforms and Microsoft
//     Visual Studio* .NET 2005" 
//     in http://www.intel.com/cd/ids/developer/asmo-na/eng/257129.htm
//     on this issue.
/***********************************************************************/
#if defined(_M_AMD64) && defined(_MSC_VER)
// Temporary fix for the intrnsic __cpuid().
extern "C" {
	void __stdcall CPUID_ECX_Clearedasm(int dRegValue[], int dEAXValue);
}

static void CPUIDInstruction(int iRegValOut[], int iEAXValIn)
{
#if defined(_M_AMD64)
	//	__cpuid(iRegValOut, iEAXValIn);
	CPUID_ECX_Clearedasm(iRegValOut, iEAXValIn);
#elif defined(_M_IX86)
	_asm {
		MOV			EAX, dword ptr iEAXValIn
			MOV			EDI, dword ptr iRegValOut
			XOR			ECX, ECX
			CPUID
			MOV			dword ptr [EDI], EAX
			MOV			dword ptr [EDI +  4], EBX
			MOV			dword ptr [EDI +  8], ECX
			MOV			dword ptr [EDI + 12], EDX
	}
#endif
}
#endif

//***************************************************************************
// Function: prvDetectCPU
//
// Purpose:
//   This function detects the presence of CPUID and fills out a number of
// global variables accordingly. The global variables should be interpreted
// as from an Intel processor. If the processor does not support CPUID (Cyrix)
// and/or conflicts with Intel's implementation, this function must map the
// non-Intel CPUID results to the Intel CPUID result space (ie, fake an
// Intel-compatible CPUID result). At the moment all processors which support
// CPUID EAX=1 return the same feature flags, so no translation is required.
// The only time we need to translate is for Cyrix, which can support MMX
// without supporting CPUID. If in future we ever decide to support AMD 3DNow
// extensions, we will also need to map the extended feature flags
// (CPUID EAX=0x80000001) into g_iCPUID_FeatureFlags, or create a new variable
// to hold the values.
//
// Note that this function is not thread safe. The first call to this function
// should be done in a non-multithreaded environment. Once that first call is
// complete, subsequent calls to this function, even if made in a multithreaded
// environment, should be safe.
//***************************************************************************
static void prvDetectCPU(void)
{
#if defined( _M_AMD64) && defined(_MSC_VER)
	int	aFeature[4];
#endif  // _M_AMD64

	// The g_iCPUID* vars must be able to hold 32-bit register
	VT_ASSERT(4 == sizeof(int));

	// Check if CPU detection already performed
	if (g_iCPUDetected)
		return;

#if defined(_M_IX86) && defined(_MSC_VER)
	// CPU detection not performed, go ahead
	_asm
	{
		// Save all registers that may be affected by CPUID
		push    eax
			push    ebx
			push    ecx
			push    edx

			// Assume user is running at least a 486 processor: check for that first
			pushfd                  // Get original EFLAGS
			pop     eax
			mov     ecx, eax
			xor     eax, 200000h     // Flip ID bit in EFLAGS
			push    eax             // Save new EFLAGS value on stack
			popfd                   // Replace current EFLAGS value
			pushfd                  // Get new EFLAGS
			pop     eax             // Store new EFLAGS in EAX
			xor     eax, ecx         // Did ID bit toggle?

			// If bit did not toggle, this is either a 486 or a Cyrix
			jz      noCPUID

			// CPUID, EAX = 0: Get and save Max CPUID level and vendor ID
			mov     eax, 0
			cpuid
			mov     g_iCPUID_MaxLvl, eax        // Save for future reference
			mov     g_iCPUID_VendorID_ebx, ebx  // Save for future reference
			mov     g_iCPUID_VendorID_edx, edx  // Save for future reference
			mov     g_iCPUID_VendorID_ecx, ecx  // Save for future reference
			cmp     eax, 1          // See if we can go to the next stage
			jl      exit_cpuid

			// CPUID, EAX = 1: Get and save feature flags
			mov     eax, 1
			cpuid
			mov     g_iCPUID_Signature, eax     // Save for future reference
			mov     g_iCPUID_FeatureFlags, edx  // Save for future reference
			mov		g_iCPUID_FeatureFlags2, ecx
			mov     g_iCPUID_BrandID, ebx       // Save for future reference

			// We don't care about the rest of CPUID
			jmp     exit_cpuid

			noCPUID :
		// Apparently it is possible for Cyrix processor 6x86 to support MMX
		// without supporting CPUID instruction. Since this processor is no 
		// longer supported for desktop/CE, we will ignore that case.
		jmp     exit_cpuid

			exit_cpuid :
		pop edx
			pop ecx
			pop ebx
			pop eax
	}
#elif defined (_M_AMD64) && defined(_MSC_VER)
	CPUIDInstruction(aFeature, 0);
	g_iCPUID_MaxLvl = aFeature[0];
	g_iCPUID_VendorID_ebx = aFeature[1];
	g_iCPUID_VendorID_ecx = aFeature[2];
	g_iCPUID_VendorID_edx = aFeature[3];

	CPUIDInstruction(aFeature, 1);
	g_iCPUID_Signature = aFeature[0];
	g_iCPUID_BrandID = aFeature[1];
	g_iCPUID_FeatureFlags2 = aFeature[2];
	g_iCPUID_FeatureFlags = aFeature[3];
#elif defined(VT_GCC) && (defined(_M_IX86) || defined(_M_AMD64))
	if (__get_cpuid(0, (unsigned int *)&g_iCPUID_MaxLvl, (unsigned int *)&g_iCPUID_VendorID_ebx, (unsigned int *)&g_iCPUID_VendorID_ecx, (unsigned int *)&g_iCPUID_VendorID_edx))
		__get_cpuid(1, (unsigned int *)&g_iCPUID_Signature, (unsigned int *)&g_iCPUID_BrandID, (unsigned int *)&g_iCPUID_FeatureFlags2, (unsigned int *)&g_iCPUID_FeatureFlags)
#elif defined(_M_ARM)
    // MS compiler arm - nothing to detect (basic Neon always supported)
#elif defined(VT_ANDROID)
    // this detection is for both x86 and Arm
    g_androidCpuFamily = android_getCpuFamily();
    g_androidCpuFeatures = android_getCpuFeatures();
#elif defined(VT_IOS)
    // Neon always supported
#else
	#pragma message("Missing CPU detection implementation for this target platform")
#endif

	VT_ASSERT(0 == g_iCPUDetected); // There can only be one
	g_iCPUDetected = 1;
}

////////////////////////////////////////////////////////////////////////////////
// wkienzle 5/4/2011: work around EBX compiler warning 
// When inlined, prvDetectCPU generates a warning that EBX is modified (Release 
// build, 32bit). Calling prvDetectCPU through a pointer prevents inlining and
// disables the warning.
void (*prvDetectCPUPtr)() = prvDetectCPU;

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_AMD64))
bool vt::g_SupportMMX (void)
{
	if (!g_iCPUDetected)
		prvDetectCPUPtr();
	return !!(g_iCPUID_FeatureFlags & 0x00800000);
}

bool vt::g_SupportSSE1 (void)
{
	if (!g_iCPUDetected)
		prvDetectCPUPtr();
	return !!(g_iCPUID_FeatureFlags & 0x02000000);
}

bool vt::g_SupportSSE2 (void)
{
	if (!g_iCPUDetected)
		prvDetectCPUPtr();
	return !!(g_iCPUID_FeatureFlags & 0x04000000);
}

bool vt::g_SupportSSE3 (void)
{
	if (!g_iCPUDetected)
		prvDetectCPUPtr();
	return !!(g_iCPUID_FeatureFlags2 & 0x00000001);
}

bool vt::g_SupportSSSE3 (void)
{
	if (!g_iCPUDetected)
		prvDetectCPUPtr();
	return !!(g_iCPUID_FeatureFlags2 & 0x00000200);
}

bool vt::g_SupportSSE4_1 (void)
{
	if (!g_iCPUDetected)
		prvDetectCPUPtr();
	return !!(g_iCPUID_FeatureFlags2 & 0x00080000);
}

bool vt::g_SupportSSE4_2 (void)
{
	if (!g_iCPUDetected)
		prvDetectCPUPtr();
	return !!(g_iCPUID_FeatureFlags2 & 0x00100000);
}

bool vt::g_SupportPOPCNT (void)
{
	if (!g_iCPUDetected)
		prvDetectCPUPtr();
	return !!(g_iCPUID_FeatureFlags2 & 0x00800000);
}

bool vt::g_SupportCMOV (void)
{
	if (!g_iCPUDetected)
		prvDetectCPUPtr();
	return !!(g_iCPUID_FeatureFlags & 0x00008000);
}

bool vt::g_SupportAVX(void)
{
#if (defined(_M_IX86)||defined(_M_AMD64)) && defined(_MSC_VER) && !defined(MSRVT_WINDOWS_BUILD)
    if (!g_iCPUDetected)
		prvDetectCPUPtr();
	return ((g_iCPUID_FeatureFlags2 & 0x18000000) == 0x18000000) &&
		   ((_xgetbv(_XCR_XFEATURE_ENABLED_MASK) & 0x6) == 0x6);
#else
    return false;
#endif
}

bool vt::g_SupportAVXwF16C (void)
{
#if (defined(_M_IX86)||defined(_M_AMD64)) && defined(_MSC_VER) && !defined(MSRVT_WINDOWS_BUILD)
    if (!g_iCPUDetected)
		prvDetectCPUPtr();
	return ((g_iCPUID_FeatureFlags2 & 0x38000000) == 0x38000000) &&
		   ((_xgetbv(_XCR_XFEATURE_ENABLED_MASK) & 0x6) == 0x6);
#else
    return false;
#endif
}

bool vt::cpuidIsIntelP5(void)
{
	prvDetectCPUPtr();

	// This function used to return g_iCPUID_MaxLvl. If this was "1", then the
	// caller assumed the CPU was an Intel P5. This is not true as AMD Athlons
	// also return "1". Fixed so this function returns a bool, and only returns
	// true for Intels.

	if (0x756E6547 == g_iCPUID_VendorID_ebx &&
		0x49656E69 == g_iCPUID_VendorID_edx &&
		0x6C65746E == g_iCPUID_VendorID_ecx &&
		1 == g_iCPUID_MaxLvl)
		return true;
	else
		return false;
}
#elif defined(VT_ANDROID)

bool vt::g_SupportMMX (void)        { if (!g_iCPUDetected) { prvDetectCPU(); } return (g_androidCpuFamily == ANDROID_CPU_FAMILY_X86); }
bool vt::g_SupportSSE1 (void)       { if (!g_iCPUDetected) { prvDetectCPU(); } return (g_androidCpuFamily == ANDROID_CPU_FAMILY_X86); }
bool vt::g_SupportSSE2 (void)       { if (!g_iCPUDetected) { prvDetectCPU(); } return (g_androidCpuFamily == ANDROID_CPU_FAMILY_X86); }
bool vt::g_SupportSSE3 (void)       { if (!g_iCPUDetected) { prvDetectCPU(); } return (g_androidCpuFamily == ANDROID_CPU_FAMILY_X86); }
bool vt::g_SupportSSSE3 (void)      { if (!g_iCPUDetected) { prvDetectCPU(); } return (g_androidCpuFamily == ANDROID_CPU_FAMILY_X86) && (g_androidCpuFeatures & ANDROID_CPU_X86_FEATURE_SSSE3);; }
bool vt::g_SupportSSE4_1 (void)     { if (!g_iCPUDetected) { prvDetectCPU(); } return false; }
bool vt::g_SupportSSE4_2 (void)     { if (!g_iCPUDetected) { prvDetectCPU(); } return false; }
bool vt::g_SupportAVX (void)        { if (!g_iCPUDetected) { prvDetectCPU(); } return false; }
bool vt::g_SupportAVXwF16C (void)   { if (!g_iCPUDetected) { prvDetectCPU(); } return false; }
bool vt::g_SupportPOPCNT (void)     { if (!g_iCPUDetected) { prvDetectCPU(); } return (g_androidCpuFamily == ANDROID_CPU_FAMILY_X86) && (g_androidCpuFeatures & ANDROID_CPU_X86_FEATURE_POPCNT); }
bool vt::g_SupportMVI (void)        { if (!g_iCPUDetected) { prvDetectCPU(); } return false; }
bool vt::g_SupportCMOV (void)       { if (!g_iCPUDetected) { prvDetectCPU(); } return false; }
bool vt::cpuidIsIntelP5 (void)      { if (!g_iCPUDetected) { prvDetectCPU(); } return false; }

#else

// TODO: OSX SSE support
                                   
#endif
