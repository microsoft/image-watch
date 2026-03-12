#pragma once

#ifndef _WINDOWS_

#if defined(__clang__)
#define UNREFERENCED_PARAMETER(P) ((void)P)
#else
#define UNREFERENCED_PARAMETER(P) (P)
#endif

/// 
/// int ptr
///

#if defined(__clang__)
#include <stdint.h>
#define INT_PTR intptr_t
#endif
#else
#if defined(_WIN64)
    typedef __int64 INT_PTR;
#else
    typedef __w64 int INT_PTR;
#endif
#endif

#if defined(VT_OSX) || defined(VT_ANDROID)
typedef int HRESULT;
#else
typedef long HRESULT;
#endif

#define S_OK     ((HRESULT)0L)
#define S_FALSE  ((HRESULT)1L)
#define NOERROR  0

#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)

#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr)    (((HRESULT)(hr)) < 0)

#define E_NOTIMPL        ((HRESULT)0x80000001L)
#define E_OUTOFMEMORY    ((HRESULT)0x80000002L)
#define E_INVALIDARG     ((HRESULT)0x80000003L)
#define E_NOINTERFACE    ((HRESULT)0x00000004L)
#define E_POINTER        ((HRESULT)0x80000005L)
#define E_HANDLE         ((HRESULT)0x80000006L)
#define E_ABORT          ((HRESULT)0x80000007L)
#define E_FAIL           ((HRESULT)0x80000008L)
#define E_ACCESSDENIED   ((HRESULT)0x80000009L)
#define E_UNEXPECTED     ((HRESULT)0x8000FFFFL)

typedef wchar_t WCHAR, *PWCHAR;
typedef const wchar_t* LPCWSTR;
typedef char CHAR, *PCHAR;
typedef unsigned int UINT;
typedef unsigned short USHORT;
typedef unsigned char BYTE, *LPBYTE;
typedef void VOID, *PVOID, *LPVOID;

#if defined(__clang__)
typedef uint32_t ULONG;
typedef int32_t LONG, *LPLONG;
typedef uint32_t DWORD;
#else
typedef unsigned long ULONG;
typedef long LONG, *LPLONG;
typedef unsigned long DWORD;
#endif

#if defined(__clang__)
typedef uint8_t UINT8;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int64_t INT64;
typedef int32_t INT32;
#endif

typedef struct tagRECT
{
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT;

typedef struct tagPOINT
{
    LONG x;
    LONG y;
} POINT;

typedef struct tagSIZE
{
    LONG cx;
    LONG cy;
} SIZE;

typedef struct _GUID {
    ULONG  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;

//
// SAL
//

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#define _Printf_format_string_

//
// int ptr
//

#if defined(__clang__)
#include <stdint.h>
#define INT_PTR intptr_t
#else
#if defined(_WIN64)
    typedef __int64 INT_PTR;
#else
    typedef __w64 int INT_PTR;
#endif
#endif
