//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      HRESULTS etc
//
//  History:
//      2004/11/08-swinder
//			Created
//
//------------------------------------------------------------------------

#pragma once

#include "vt_basetypes.h"
#include "vt_dbg.h"

namespace vt {

#define S_EOF			_HRESULT_TYPEDEF_(0x27ff0001L)
#define E_NOINIT		_HRESULT_TYPEDEF_(0xa7ff0009L)
#define E_READFAILED	_HRESULT_TYPEDEF_(0xa7ff0002L)
#define E_WRITEFAILED	_HRESULT_TYPEDEF_(0xa7ff0003L)
#define E_BADFORMAT		_HRESULT_TYPEDEF_(0xa7ff0004L)
#define E_TYPEMISMATCH	_HRESULT_TYPEDEF_(0xa7ff0005L)
#define E_NOTFOUND		_HRESULT_TYPEDEF_(0xa7ff0006L)
#define E_NOCOMPRESSOR	_HRESULT_TYPEDEF_(0xa7ff0007L)
#define E_TOOCOMPLEX	_HRESULT_TYPEDEF_(0xa7ff0008L)
#define E_INVALIDSRC	_HRESULT_TYPEDEF_(0xa7ff0010L)
#define E_INVALIDDST	_HRESULT_TYPEDEF_(0xa7ff0011L)

// Error macros
#pragma warning (disable: 4102) // no warning when function has no _EXIT instances
#define VT_HR_BEGIN() HRESULT hr = S_OK; {
#define VT_HR_EXIT_LABEL() } Exit:
#pragma warning (disable: 4003) // no warning when macro does not have any params
#define VT_HR_END(code_block) } Exit: { code_block } return hr;

// Support for multi-line macros (like VT_HR_EXIT) to force use of a semicolon but not trigger clang 'dangling else' warning
#define VT_MULTI_LINE_MACRO_BEGIN do {  
#if defined(_MSC_VER)
#define VT_MULTI_LINE_MACRO_END \
    __pragma(warning(push)) \
    __pragma(warning(disable:4127)) \
    } while(0) \
    __pragma(warning(pop))  
#else
#define VT_MULTI_LINE_MACRO_END \
    } while(0)
#endif

#define VT_HR_RET(expression) \
VT_MULTI_LINE_MACRO_BEGIN \
if FAILED(hr = (expression)) \
    { VT_DEBUG_LOG_HR(hr); return hr; } \
VT_MULTI_LINE_MACRO_END

#define VT_HR_EXIT(expression) \
VT_MULTI_LINE_MACRO_BEGIN \
if FAILED(hr = (expression)) \
    { VT_DEBUG_LOG_HR(hr); goto Exit;} \
VT_MULTI_LINE_MACRO_END

#define VT_PTR_OOM_EXIT(expression) \
VT_MULTI_LINE_MACRO_BEGIN \
if (NULL == (expression)) \
    {hr = E_OUTOFMEMORY; VT_DEBUG_LOG_HR(hr); goto Exit;} \
VT_MULTI_LINE_MACRO_END

#define VT_PTR_EXIT(expression) \
VT_MULTI_LINE_MACRO_BEGIN \
if (NULL == (expression)) \
    {hr = E_POINTER; VT_DEBUG_LOG_HR(hr); goto Exit;} \
VT_MULTI_LINE_MACRO_END

/// \ingroup error
/// <summary> Return common HRESULT as string </summary>
/// <param name="hr"> HRESULT from a VisionTools operation </param>
/// <param name="buf"> Buffer to write the error text to </param>
/// <param name="numBufElem"> Buffer size, in characters </param>
/// <returns> Pointer to the beginning of the text buffer </returns>
const wchar_t* VtErrorToString(
    HRESULT hr, 
    __in_ecount(numBufElem) wchar_t* buf, 
    int numBufElem);

class CErrorBase
{
public:
	CErrorBase() : m_hr(NOERROR) {}

	HRESULT GetError() const { return m_hr; }
	bool    IsError()  const { return FAILED(m_hr); }
	HRESULT SetHR(HRESULT hr) { m_hr = hr; return hr; } // for setting S_EOF
	HRESULT SetError(HRESULT hr) { if(FAILED(hr)) m_hr = hr; return hr; } // only set error on failure
	HRESULT ClearError()         { m_hr = NOERROR; return m_hr; }

protected:
    HRESULT ClearError()         const { m_hr = NOERROR; return m_hr; }
	HRESULT SetError(HRESULT hr) const { m_hr = hr; return hr; }

private:
	mutable HRESULT m_hr;
};

};
