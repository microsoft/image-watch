//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Classes for holding name-value parameters e.g. metadata
//
//  History:
//      2004/11/08-swinder
//          Created
//
//------------------------------------------------------------------------
#pragma once

#include "vtcommon.h"

namespace vt {

#define	PARX_HASHSIZE	11

//+----------------------------------------------------------------------------
//
//  Define the types of params
// 
//-----------------------------------------------------------------------------
typedef enum {
    ParamType_None    = 0,  
    ParamType_Byte    = 1, // the hard-coded entries come from the exif spec
    ParamType_ASCII   = 2,
    ParamType_UShort  = 3, 
    ParamType_ULong   = 4,
    ParamType_Rational  = 5,
    ParamType_SByte     = 6,
    ParamType_Undefined = 7,
    ParamType_SShort    = 8,
    ParamType_SLong     = 9,
    ParamType_SRational = 10,
    ParamType_Float  = 11,
    ParamType_Double = 12,
    ParamType_Bool,
    ParamType_String,
    ParamType_U8,     // un-signed 64bit int 
    ParamType_I8,     // signed 64bit int
    ParamType_GUID,
    ParamType_Params
} ParamType;

struct RATIONAL
{
    ULONG numerator;
    ULONG denominator;

    float AsFloat() const { return (float)AsDouble(); }
    double AsDouble() const { return double(numerator)/double(denominator); }
    
    RATIONAL() {}

    RATIONAL(ULONG numerator_, ULONG denominator_)
        : numerator(numerator_), denominator(denominator_)
    {}
};

struct SRATIONAL
{
    LONG numerator;
    LONG denominator;

    float AsFloat() const { return (float)AsDouble(); }
    double AsDouble() const { return double(numerator)/double(denominator); }

    SRATIONAL() {}

    SRATIONAL(LONG numerator_, LONG denominator_)
        : numerator(numerator_), denominator(denominator_)
    {}
};

//+----------------------------------------------------------------------------
//
//  Class: CParamValue
// 
//  Synopsis: The class that holds a param value, used for metadata in images 
// 
//-----------------------------------------------------------------------------

class CParamValue
{
public:
    CParamValue() : m_eType(ParamType_None) {}
    ~CParamValue() { Invalidate(); }

    void      Invalidate();
    bool      IsValid() const   { return m_eType!=ParamType_None; }
    ParamType GetType() const   { return m_eType; }
    size_t    GetItemCount() const;
 
    HRESULT Set(const CParamValue& val);
    template <class T> HRESULT Set(const T& val);
    template <class T> HRESULT SetArray(const T* val, size_t count);
    template <class T> HRESULT Get(T* val, size_t element = 0, 
                                   size_t count = 1) const;
    const void*  GetDataPtr()  const { return m_pvData; }
    size_t       GetDataSize() const { return m_uiSize; }

protected:
    size_t  SizeOfType() const;
    HRESULT GetGeneric(void* val, ParamType type, size_t element,
                       size_t count) const;
    HRESULT SetData(ParamType e, size_t uiSz, const void *pv);

protected:
    ParamType m_eType;
    size_t    m_uiSize;
    void     *m_pvData;
};

//+----------------------------------------------------------------------------
//
//  Class: CParams
// 
//  Synopsis: The class that holds a list of parameters
// 
//-----------------------------------------------------------------------------
struct PARAM_ENTRY
{
    WCHAR *pszName;  // if name is NULL then this param is specified by uID
    UINT   uId;
    UINT   uIndex;
    CParamValue sVal;
    PARAM_ENTRY *pPrevCreate;
    PARAM_ENTRY *pNextCreate;
    PARAM_ENTRY *pPrevHash;
    PARAM_ENTRY *pNextHash;
};

struct PARAM_ENUM_TOKEN
{
    const void* ptr;
    PARAM_ENUM_TOKEN() : ptr(NULL) {}
};

class CParams
{
public:
    CParams();
	CParams(const CParams &cParams);
    ~CParams();

    template <class T> HRESULT GetById(T* val, UINT uId, UINT uIndex = 0) const;
    HRESULT GetById(CParamValue** pVal, UINT uId, UINT uIndex = 0) const;
    HRESULT GetById(const CParamValue** pVal, UINT uId, UINT uIndex = 0) const;
    HRESULT SetById(UINT uId, UINT uIndex, const CParamValue &sEntry);
    HRESULT DeleteById(UINT uId, UINT uIndex = 0);

    template <class T> HRESULT GetByName(T* val, const WCHAR *pszName,
                                         UINT uIndex = 0) const;
    HRESULT GetByName(CParamValue** pVal, const WCHAR *pszName, 
                      UINT uIndex = 0) const;
    HRESULT GetByName(const CParamValue** pVal, const WCHAR *pszName, 
                      UINT uIndex = 0) const;
    HRESULT SetByName(const WCHAR *pszName, UINT uIndex, 
                      const CParamValue &sEntry);
    HRESULT DeleteByName(const WCHAR *pszName, UINT uIndex = 0);

    PARAM_ENUM_TOKEN StartEnum() const;
    bool NextEnum(PARAM_ENUM_TOKEN& token,
                  const WCHAR **ppszNameRtn, UINT *pIdRtn, UINT *puIndexRtn, 
                  const CParamValue **ppValueRtn = NULL) const;

	CParams &operator=(const CParams &cParams);

    int Size() const { return m_iCount; }
    void DeleteAll();
    HRESULT Merge(const CParams *pParams);

protected:
    HRESULT Get(CParamValue** ppVal, const WCHAR *pszName, UINT uID, 
                UINT uIndex = 0) const;
    HRESULT Set(const WCHAR *pszName, UINT uID, UINT uIndex,
                const CParamValue &sEntry, bool bReset = true);
    HRESULT Delete(const WCHAR *pszName, UINT uID, UINT uIndex = 0);

    void DeleteHashEntry(PARAM_ENTRY *pEntry, int iHash);
    PARAM_ENTRY *FindHashEntry(const WCHAR *pszName, UINT uId, UINT uIndex, 
                              int *piHashRtn) const;
    HRESULT Merge(const PARAM_ENTRY *pTail, bool bReset = true);
    HRESULT Reset();
    void DeleteAllEntries();

    LPLONG m_pRef;
    PARAM_ENTRY *m_pCreateListTail;
    PARAM_ENTRY *m_rgpHashList[PARX_HASHSIZE];
    int m_iCount;
};

//+----------------------------------------------------------------------------
// 
//  Helper templates for param types
// 
//-----------------------------------------------------------------------------
template <class T>     struct ParamTraits;
template <ParamType E> struct ParamTypeTraits;
#define DEF_TRAITS(T, E) \
    template<> struct ParamTraits<T>\
    {\
        static ParamType Type() { return E; }\
    };\
    template<> struct ParamTypeTraits<E>\
    {\
        typedef T Type;\
        static size_t SizeOf() { return sizeof(T); }\
    };

DEF_TRAITS(Byte,  ParamType_Byte)
DEF_TRAITS(char*, ParamType_ASCII)
DEF_TRAITS(USHORT, ParamType_UShort)
DEF_TRAITS(ULONG,  ParamType_ULong)
// note ParamType_Undefined doesn't have traits entry 
DEF_TRAITS(RATIONAL,  ParamType_Rational)
DEF_TRAITS(char,      ParamType_SByte)
DEF_TRAITS(short,     ParamType_SShort)
DEF_TRAITS(LONG,      ParamType_SLong)
DEF_TRAITS(SRATIONAL, ParamType_SRational)
DEF_TRAITS(float,  ParamType_Float)
DEF_TRAITS(double, ParamType_Double)
DEF_TRAITS(bool,   ParamType_Bool)
DEF_TRAITS(WCHAR*, ParamType_String)
DEF_TRAITS(UINT64, ParamType_U8)
DEF_TRAITS(INT64,  ParamType_I8)
DEF_TRAITS(GUID,   ParamType_GUID)
DEF_TRAITS(CParams, ParamType_Params)

template <class T>
inline ParamType TypeIs(const T&) { return ParamTraits<T>::Type(); }

//+----------------------------------------------------------------------------
// 
//  Implementation of the Set/Get members of the CParam class
// 
//-----------------------------------------------------------------------------
template <class T>
inline HRESULT CParamValue::Set(const T& val)
{
    return SetData(TypeIs(val), sizeof(T), (const void*)&val);
}

template <class T>
inline HRESULT CParamValue::SetArray(const T* val, size_t count)
{
    return SetData(TypeIs(*val), sizeof(T)*count, (const void*)val);
}

template <>
inline HRESULT CParamValue::SetArray(const CParams* val, size_t count)
{
    m_eType = TypeIs(*val);
    m_pvData = VT_NOTHROWNEW CParams[count];
    if( m_pvData == NULL )
    {
        return E_OUTOFMEMORY;
    }

    m_uiSize = count * sizeof(CParams);
    for( size_t i = 0 ; i < count; i++ )
    {
        HRESULT hr = ((CParams*)m_pvData)[i].Merge(val+i);
        if( hr != S_OK )
        {
            return hr;
        }
    }
    return S_OK;
}

template <>
inline HRESULT CParamValue::Set(const CParams& val)
{
    return SetArray(&val, 1);
}

template <>
inline HRESULT CParamValue::Set(const char * const & val)
{
    return SetData(ParamType_ASCII, (strlen(val)+1)*sizeof(char), val);
}

template <>
inline HRESULT CParamValue::Set(const WCHAR * const & val)
{ 
    return SetData(ParamType_String, (wcslen((WCHAR*)val)+1)*sizeof(WCHAR), 
                   val);
}

template <>
inline HRESULT CParamValue::Set(char * const & val)
{
    return SetData(ParamType_ASCII, (strlen(val)+1)*sizeof(char), val);
}

template <>
inline HRESULT CParamValue::Set(WCHAR * const & val)
{ 
    return SetData(ParamType_String, (wcslen((WCHAR*)val)+1)*sizeof(WCHAR), 
                   val);
}

template <>
inline HRESULT CParamValue::SetArray(const void* val, size_t count)
{
    return SetData(ParamType_Undefined, count, val);
}

template <>
inline HRESULT CParamValue::Set(const PVOID& val)
{
    UNREFERENCED_PARAMETER(val);

    return E_NOTIMPL;
}

template <class T>
inline HRESULT CParamValue::Get(T* val, size_t element, size_t count) const
{
    return GetGeneric(val, TypeIs(*val), element, count);
}

template <>
inline HRESULT CParamValue::Get(PCHAR* val, size_t element, size_t count) const
{
    if( ParamType_ASCII != m_eType || element != 0 || count != 1 )
    {
        *val = NULL;
        return E_INVALIDARG;
    }
    *val = (PCHAR)m_pvData;
    return S_OK;
}

template <>
inline HRESULT CParamValue::Get(PWCHAR* val, size_t element, size_t count) const
{
    if( ParamType_String != m_eType || element != 0 || count != 1 )
    {
        *val = NULL;
        return E_INVALIDARG;
    }
    *val = (PWCHAR)m_pvData;
    return S_OK;
}

template <>
inline HRESULT CParamValue::Get(_Out_writes_bytes_(count) void* val, size_t element, size_t count) const
{
#pragma warning(suppress:22013)
    if( ParamType_Undefined != m_eType || (element+count) > m_uiSize )
    {
        return E_INVALIDARG;
    }
    memcpy(val, (BYTE*)m_pvData+element, count);
    return S_OK;
}

template <>
inline HRESULT CParamValue::Get(CParams* val, size_t element, size_t count) const
{
    if( TypeIs(*val) != m_eType || (element+count) > GetItemCount() )
    {
        return E_INVALIDARG;
    }
    for( size_t i = 0; i < count; i++ )
    {
        val[i].Merge(((CParams*)m_pvData+(element+i)));
    }
    return S_OK;
}

template <class T> 
inline HRESULT CParams::GetById(T* val, UINT uId, UINT uIndex) const
{
    const CParamValue* pVal;
    HRESULT hr = GetById(&pVal, uId, uIndex);
    if( hr == S_OK )
    {
        hr = pVal->Get(val);
    }
    return hr;
}

template <class T> 
inline HRESULT CParams::GetByName(T* val, const WCHAR *pszName, 
                                  UINT uIndex) const
{
    const CParamValue* pVal;
    HRESULT hr = GetByName(&pVal, pszName, uIndex);
    if( hr == S_OK )
    {
        hr = pVal->Get(val);
    }
    return hr;
}

};


