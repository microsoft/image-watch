//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Implementation of a sting class
//
//  History:
//      2003/11/12-mattu
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vt_basetypes.h"
#include "vt_intrinsics.h"
#include "vt_dbg.h"
#include "vt_new.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdarg.h>

namespace vt {

inline int VtStringCopy(
    __out_ecount(numDstChars) char* dst, 
    size_t numDstChars, 
    const char* src)
{
#ifdef _MSC_VER
	return strcpy_s(dst, numDstChars, src);
#else
	strcpy(dst, src);
	return 0;
#endif
}

inline int VtStringCopy(
    __out_ecount(numDstChars) char* dst, 
    size_t numDstChars, 
    const char* src, 
	size_t maxNumChars)
{
#ifdef _MSC_VER
	return strncpy_s(dst, numDstChars, src, maxNumChars);
#else
	strncpy(dst, src, maxNumChars);
	return 0;
#endif
}

inline int VtStringCopy(
    __out_ecount(numDstChars) wchar_t* dst, 
    size_t numDstChars, 
    const wchar_t* src)
{
#ifdef _MSC_VER
	return wcscpy_s(dst, numDstChars, src);
#else
	wcscpy(dst, src);
	return 0;
#endif
}

inline int VtStringCopy(
    __out_ecount(numDstChars) wchar_t* dst, 
    size_t numDstChars, 
    const wchar_t* src, 
	size_t maxNumChars)
{
#ifdef _MSC_VER
	return wcsncpy_s(dst, numDstChars, src, maxNumChars);
#else
	wcsncpy(dst, src, maxNumChars);
	return 0;
#endif
}

inline int VtStringPrintf(
    __out_ecount(numDstChars) char* dst, 
    size_t numDstChars, 
    const char* fmt, ...)
{
	int numChars = -1;

	va_list args;
	va_start(args, fmt);
#ifdef _MSC_VER
	numChars = vsprintf_s(dst, numDstChars, fmt, args);
#else
	numChars = vsprintf(dst, fmt, args);
#endif
	va_end(args);

	return numChars;
}

inline int VtStringPrintfLength(const char* fmt, ...)
{
	int numChars = -1;

	va_list args;
	va_start(args, fmt);
#ifdef _MSC_VER
	numChars = _vscprintf(fmt, args);
#else
	VT_ASSERT(!"Not implemented");
#endif
	va_end(args);

	return numChars;
}

inline int VtStringPrintf(
    __out_ecount(numDstChars) wchar_t* dst, 
    size_t numDstChars, 
    const wchar_t* fmt, ...)
{
	int numChars = -1;
	
	va_list args;
	va_start(args, fmt);
#ifdef _MSC_VER
	numChars = vswprintf_s(dst, numDstChars, fmt, args);
#else
	numChars = vswprintf(dst, numDstChars, fmt, args);
#endif
	va_end(args);

	return numChars;
}

inline int VtStringPrintfLength(const wchar_t* fmt, ...)
{
	int numChars = -1;

	va_list args;
	va_start(args, fmt);
#ifdef _MSC_VER
	numChars = _vscwprintf(fmt, args);
#else
	VT_ASSERT(!"Not implemented");
#endif
	va_end(args);

	return numChars;
}

// _string_t is a no-nonsense, STL-like, lightweight string class template.
// string and wstring are instances of the template for strings of 
// char and WCHAR respectivly. In the simplest use, the template is merely 
// managing dynamic memory for a string and thus has minimal or no overhead 
// compared to manually allocation/freeing dynamic memory.
// _string_t implements small string optimization - small strings are stored in 
// an internal buffer rather than in a buffer allocated from heap.

#if 0
/**/    // 1. contruction and assigment
/**/
/**/    string str;
/**/    string wstr;
/**/
/**/    str = "string";
/**/    wstr = L"string";
/**/
/**/    string str1("string1");
/**/    wstring wstr2(wstr);
/**/
/**/    str = str1;
/**/
/**/    // 2. comparison
/**/
/**/    if(str == str1)
/**/        // ...
/**/    
/**/    if(str == "string")
/**/        // ...
/**/    
/**/    if(L"string" != wstr)
/**/        // ...
/**/    
/**/    // 3. append and assign can be used in place of = or += when you need
/**/    //    to check for outofmemory
/**/
/**/    return str.append(psz, 3))      // append first 3 characters from psz
/**/
/**/    return str.assign("123", 2, 2)  // assing "12" to the string starting at 
/**/                                    // position 2
/**/
/**/    // 4. getting C string
/**/
/**/    void foo(const char* psz);
/**/    void bar(WCHAR *ps, int n);
/**/
/**/    foo(str); // automatic conversion to const char*
/**/    
/**/    // reserve space for 100 characters and pass a writable buffer to a 
/**/    // function
/**/    wstr.reserve(100);
/**/    bar(wstr, wstr.capacity());
/**/
/**/    // 5. you can define custom instances of _string_t template
/**/    
/**/    // wide character string that uses internal buffer for strings up to 
/**/    // 128 characters
/**/    typedef _string_t<WCHAR, 128>  wstring;
/**/
/**/    // case insensitive string
/**/    template<class T>
/**/    struct case_insensitive_traits<T>
/**/    {
/**/        // provide traits methods with case insensitive logic
/**/    };
/**/
/**/    typedef _string_t<char, 16, case_insensitive_traits<char> >  stringi;
/**/
#endif

// AUTOEXP.DAT
// 
// Add following line to your AUTOEXP.DAT to tell debugger how to display
// objects of this class in watch windows.
//
// _string_t<*>=<_Ptr>

template <class T>
struct char_traits;

// char_traits for char
template<>
struct char_traits<char>
{
    static int compare(const char* _U, const char* _V)
        {return _U ? (_V ? strcmp(_U, _V) : 1) : -1; }

    static size_t length(const char* _U)
        {return _U ? strlen(_U) : 0; }

    static void copy(_Out_writes_(_N) char* _U, size_t /*_NU*/, 
		             _In_reads_(_N) const char* _V, size_t _N)
        {memcpy(_U, _V, _N); }

    static const char* find(const char* _U, const char& _C)
        {return strchr(_U, _C); }

    static HRESULT format(char* dst, size_t numDstChars,
                           const char* fmt, va_list args)
    {	
	int numChars = -1;	
#ifdef _MSC_VER
	numChars = vsprintf_s(dst, numDstChars, fmt, args);
#else
	numChars = vsprintf(dst, fmt, args);
#endif	
	return numChars > 0 ? S_OK : E_FAIL;
	}

	static int format_buffer_size(const char* fmt, va_list args)
	{
	int numChars = -1;	
#ifdef _MSC_VER
	numChars = _vscprintf(fmt, args);
#else
    va_list args_copy; 
    va_copy(args_copy, args);
    numChars = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
#endif	
	return numChars;
	}
};


// char_traits for WCHAR
template<>
struct char_traits<WCHAR>
{
    static int compare(const WCHAR* _U, const WCHAR* _V)
        {return _U ? (_V ? wcscmp(_U, _V) : 1) : -1; }

    static size_t length(const WCHAR* _U)
        {return _U ? wcslen(_U) : 0; }

    static void copy(WCHAR* _U, size_t _NU, 
                             const WCHAR* _V, size_t _N)
        {VtStringCopy(_U, _NU < _N+1 ? _NU : _N+1, _V, _N); }

    static const WCHAR* find(const WCHAR* _U, const WCHAR& _C)
        {return wcschr(_U, _C); }

    static HRESULT format(WCHAR* dst, size_t numDstChars,
                           const WCHAR* fmt, va_list args)
    {
	int numChars = -1;
#ifdef _MSC_VER
	numChars = vswprintf_s(dst, numDstChars, fmt, args);
#else
	numChars = vswprintf(dst, numDstChars, fmt, args);
#endif
	return numChars > 0 ? S_OK : E_FAIL;
	}

	static int format_buffer_size(const WCHAR* fmt, va_list args)
	{
	int numChars = -1;
#ifdef _MSC_VER
	numChars = _vscwprintf(fmt, args);
#else
    va_list args_copy; 
    va_copy(args_copy, args);
    wchar_t buf[4096]; // clang llvm lib vswprintf does not return correct character count unless provided with a buffer
	numChars = vswprintf(buf, 4096, fmt, args_copy);
    va_end(args_copy);
#endif
	return numChars;
	}
};


// _string_t
template <class T, unsigned _BUF_SIZE = 16, class _Tr = char_traits<T> >
class _string_t
{
protected:
    class _refmem;

public:
    typedef _string_t<T, _BUF_SIZE, _Tr>    _Myt;
    typedef size_t                          size_type;

    // ctors
    _string_t()
        {_Init(); }

    _string_t(const _Myt& _X)
        {_Init(); assign(_X); }

    _string_t(const T* _S)
        {_Init(); assign(_S); }
        
    explicit _string_t(size_type _N)
        {_Init(); _Grow(_N); }

    // dtor
    ~_string_t()
    {
        VT_ASSERT(length() <= _Res);
        VT_ASSERT(_Ptr()[_Res] == T(0));
        
        if(_RefMem)
        {
            _RefMem->_Release();
        }
    }
    
    // length
    size_type length() const
        {return _Tr::length(_Ptr()); }
        
    // size
    size_type size() const
        {return length(); } 
        
    // capacity
    size_type capacity() const
        {return _Res; } 
        
    // empty
    bool empty() const   
        {return length() == 0; }

    // reserve
    HRESULT reserve(size_type _N)
        {return _Grow(_N); }

    // operator const T*
    operator const T*() const
        {return _Ptr(); }

    // operator=
    _Myt& operator=(const T* _S)
        {assign(_S); return *this; }

    _Myt& operator=(const _Myt& _X)
        {assign(_X); return *this; }

    // operator+=
    _Myt& operator+=(const _Myt& _X)
        {append(_X); return *this; }

    _Myt& operator+=(const T* _S)
        {append(_S); return *this; }

    // compare
    int compare(const _Myt& _X) const
        {return compare(_X._Ptr()); }

    int compare(const T* _S) const
        {return _Tr::compare(_Ptr(), _S); }
    
    // append
    HRESULT append(const T& _C)
        {return append(&_C, 1); }

    HRESULT append(const T* _S)
        {return append(_S, _Tr::length(_S)); }

    HRESULT append(const T* _S, size_type _N)
        { return _Assign(_S, _N, length()); }

    HRESULT append(const _Myt& _X)
        { return _Assign(_X._Ptr(), _X.length(), length() ); }
    
    // assign
    HRESULT assign(const T& _C, size_type _P = 0)
        {return _Assign(&_C, 1, _P); }
    
    HRESULT assign(const T* _S, size_type _P = 0)
        {return _Assign(_S, _Tr::length(_S), _P); }

    HRESULT assign(const _Myt& _X, size_type _P = 0)
    {
        if( _X._RefMem == NULL || _P != 0 )
            return _Assign(_X._Ptr(), _X.length(), _P);
        else
        {
            _X._RefMem->_AddRef();
            if( _RefMem )
            {
                _RefMem->_Release();
            }

            _RefMem = _X._RefMem;
            _Res    = _X._Res;
        }
        return S_OK;
    }

    // copyto
    template <unsigned B> 
    HRESULT copyto(_string_t<T, B>& _X) const
    {
        return _X._Assign(_Ptr(), length(), 0);
    }

    // safe string printf
    HRESULT format(_Printf_format_string_ const T* pszFmt, ...)
    {
        HRESULT hr = _ReallocateSharedBuffer(); 
        if( hr == S_OK )
        {
            va_list Args;
            va_start(Args, pszFmt);
            hr = _Tr::format(_Ptr(), _Res, pszFmt, Args);
            va_end(Args);

            _Eos(length());
        }

        return hr;
    }

	// resize buffer to appropriate size and safe string printf
    HRESULT format_with_resize(_Printf_format_string_ const T* pszFmt, va_list args)
    {
        HRESULT hr;
        va_list args_copy1; 
        va_copy(args_copy1, args);
        size_t bufferlen = (size_t)_Tr::format_buffer_size(pszFmt, args_copy1) + 1;
        va_end(args_copy1);
        if( _Res < bufferlen )
        {
            hr = _Grow(bufferlen);
        }
        else
        {
            hr = _ReallocateSharedBuffer(); 
        }
        if( hr == S_OK )
        {
            va_list args_copy2; 
            va_copy(args_copy2, args);
            hr = _Tr::format(_Ptr(), _Res, pszFmt, args_copy2);
            va_end(args_copy2);
            _Eos(length());
        }
        return hr;
    }
    HRESULT format_with_resize(_Printf_format_string_ const T* pszFmt, ...)
    {
        HRESULT hr;
        va_list Args;
        va_start(Args, pszFmt);
        hr = format_with_resize(pszFmt, Args);
        va_end(Args);
        return hr;
    }

    // resize
    HRESULT resize(size_type _N)
    {
        HRESULT hr = _Grow(_N);
        if( hr == S_OK )
        {
            _Eos(_N);
        }   
        return hr;
    }
    
    // XXX: what to do about failure here?
    T* get_buffer()
        {_ReallocateSharedBuffer(); return _Ptr(); }

    const T* get_constbuffer() const
        {return _Ptr(); }

protected:
    void _Init()
    {
        _Res    = _BUF_SIZE;
        _RefMem = NULL;
        _Buf[_Res] = T(0);
        _Eos(0);
    }

    T* _Ptr()
        {return _RefMem? _RefMem->_buf(): _Buf;}

    const T* _Ptr() const
        {return _RefMem? _RefMem->_buf(): _Buf;}

    // _Grow
    HRESULT _Grow(size_type _N)
    {
// Note: I'm 99% sure that the /analyze warning here is a false positive so
//       disabling it.  When a new version of the tools come out we should check
//       if this warning is still generated.
#pragma warning( push )
#pragma warning ( disable : 6385 )

        if(_Res < _N)
        {
            if( _RefMem == NULL )
            {
                _RefMem = VT_NOTHROWNEW _refmem();
                if(!_RefMem || _RefMem->_grow(_N+1)==false)
                    return E_OUTOFMEMORY;
                memcpy(_RefMem->_buf(), _Buf, _Res*sizeof(T));
            }
            else
            {
                if( !_RefMem->_grow(_N+1) )
                    return E_OUTOFMEMORY;
            }

            _Res = _N;
            _RefMem->_buf()[_Res] = T(0);
        }

        return S_OK;

#pragma warning( pop )
    }

	// _ReallocateSharedBuffer
    HRESULT _ReallocateSharedBuffer()
    {
        if( _RefMem )
        {
            if ( _RefMem->_Refs() > 1 )
            {       
                _refmem* pnew = VT_NOTHROWNEW _refmem();
                if (!pnew || !pnew->_grow(_Res+1) )
                {
                    if( pnew ) 
                        pnew->_Release();
                    _RefMem->_Release();
                    _Init();
                    return E_OUTOFMEMORY;
                }
    
                _Tr::copy(pnew->_buf(), _Res+1, _RefMem->_buf(), _Res+1);
                _RefMem->_Release();
                _RefMem = pnew;
            }
        }

        return S_OK;
    }

    HRESULT _Assign(const T* _S, size_type _N, size_type _P)
    {

        VT_ASSERT(_S != NULL);
        
        // in the case of shared memory, create a new copy and release the old
        if( _RefMem && _RefMem->_Refs() > 1 )
        {
            // XXX: refmem is released before we are finished with oldbuf
            T* oldbuf = _RefMem->_buf();
            _RefMem->_Release();
            _Init();
            
            if( _P != 0  )
            {
                // allocate _N+_P and copy the old _P
                _Res = _N+_P;
                if( _Res > _BUF_SIZE )
                {                    
                    _refmem* pnew = VT_NOTHROWNEW _refmem();
                    if (!pnew )
                        return E_OUTOFMEMORY;
    
                    if ( !pnew->_grow(_Res+1) )
                    {
                        pnew->_Release();
                        return E_OUTOFMEMORY;
                    }
    
                    _Tr::copy(pnew->_buf(), _Res+1, oldbuf, _P);
                    _RefMem = pnew;
                }
                else
                {
                    _Tr::copy(_Buf, _BUF_SIZE+1, oldbuf, _P);
                }
                _Eos(_Res);
            }
        }
        
        HRESULT hr = _Grow(_N + _P);
        if( hr == S_OK)
        {
            _Tr::copy(_Ptr() + _P, _Res+1-_P, _S, _N);
            _Eos(_N + _P);
        }

        return hr;
    }

    // _Eos
    void _Eos(size_type _N)
    {
        VT_ASSERT(_N <= _Res);
        
        _Ptr()[_N] = T(0);
    }

protected:
    T           _Buf[_BUF_SIZE + 1];
    _refmem*    _RefMem;
    size_type   _Res;

protected:
    class _refmem
    {
    public:
        _refmem() : _RefCount(1), _Buffer(NULL), _Size(0)
            {}

        ~_refmem()
            { _clear(); }

        long _AddRef()
            {return VT_INTERLOCKED_INCREMENT(&_RefCount);}

        long _Release()
        {
			long cRef = VT_INTERLOCKED_DECREMENT(&_RefCount);
            if(cRef == 0)
            {
                delete this;
                return 0;
            }
            return cRef;
        }

        long _Refs()
            { return _RefCount; }

        T* _buf() const
            { return _Buffer; }

        bool _grow(size_type _N)
        {   
            if( _Size < _N )
            {
                VT_ASSERT(_N > 17);
                T* _New = VT_NOTHROWNEW T[_N];

                if(!_New)
                    return false;
            
                if( _Buffer )
                {
                    memcpy(_New, _Buffer, _Size*sizeof(T));
                    delete [] _Buffer;
                }

                _Buffer = _New;
                _Size   = _N;
            }
            return true;
        }

        void _clear()
            { if( _Buffer ) {delete [] _Buffer; _Buffer = NULL; _Size = 0; } }

        size_type size() const
            { return _Size; }

    private:
        long  _RefCount;
        T*    _Buffer;
        size_type _Size;   
    };
};

// operator==
template <class T, unsigned _BUF_SIZE, class _Tr>
inline bool operator==(const _string_t<T, _BUF_SIZE, _Tr>& _L, 
                       const _string_t<T, _BUF_SIZE, _Tr>& _R)
    {return _L.compare(_R) == 0; }

template <class T, unsigned _BUF_SIZE, class _Tr>
inline bool operator==(const _string_t<T, _BUF_SIZE, _Tr>& _L, const T* _R)
    {return _L.compare(_R) == 0; }

template <class T, unsigned _BUF_SIZE, class _Tr>
inline bool operator==(const T* _L, const _string_t<T, _BUF_SIZE, _Tr>& _R)
    {return _R.compare(_L) == 0; }

// operator!=
template <class T, unsigned _BUF_SIZE, class _Tr>
inline bool operator!=(const _string_t<T, _BUF_SIZE, _Tr>& _L, 
                       const _string_t<T, _BUF_SIZE, _Tr>& _R)
    {return !(_L == _R); }

template <class T, unsigned _BUF_SIZE, class _Tr>
inline bool operator!=(const _string_t<T, _BUF_SIZE, _Tr>& _L, const T* _R)
    {return !(_L == _R); }

template <class T, unsigned _BUF_SIZE, class _Tr>
inline bool operator!=(const T* _L, const _string_t<T, _BUF_SIZE, _Tr>& _R)
    {return !(_L == _R); }

// instantiate specific string types
typedef _string_t<char>    string;
typedef _string_t<WCHAR> wstring;

// define a string_b (string with a user defined internal buffer of size B), 
// this could also be done with the _string_t class but string_b is a 
// convenient short-hand
template <unsigned B> class string_b:  public _string_t<char, B> {};
template <unsigned B> class wstring_b: public _string_t<WCHAR, B> {};

// define conversion operations between these types

// MultiByteToWideChar 
template <unsigned _BUF_SIZE>
int VtMultiByteToWideChar( _string_t<WCHAR, _BUF_SIZE>& wstr, const char* pC )
{
    if( pC == NULL )
    {
        return 0;
    }

    int len = (int)strlen(pC);
#ifdef _MSC_VER
    size_t cchWstr = ::MultiByteToWideChar(CP_ACP, 0, pC, len, NULL, 0);
#else
	size_t cchWstr = len;
#endif

    if( wstr.resize( cchWstr ) != S_OK )
    {
        return 0;
    }

#ifdef _MSC_VER
    return ::MultiByteToWideChar( CP_ACP, 0, pC, len, wstr.get_buffer(), 
                                  (int)cchWstr );
#else
	return mbstowcs(wstr.get_buffer(), pC, wstr.size());
#endif
}

// WideCharToMultiByte
template <unsigned _BUF_SIZE>
int VtWideCharToMultiByte( _string_t<char, _BUF_SIZE>& str, const WCHAR* pWC )
{
    if( pWC == NULL )
    {
        return 0;
    }

	int len = (int)wcslen(pWC);
#ifdef _MSC_VER	
	size_t cchStr = ::WideCharToMultiByte(CP_ACP, 0, pWC, len, NULL, 0, 
                                          NULL, NULL);
#else
	size_t cchStr = len;
#endif

    if( str.resize(cchStr) != S_OK )
    {
        return 0;
    }

#ifdef _MSC_VER
    return ::WideCharToMultiByte(CP_ACP, 0, pWC, len, str.get_buffer(), 
                                 (int)str.capacity(), NULL, NULL);
#else
	return wcbtombs(str.get_buffer(), pWC, str.size());
#endif
}

};
