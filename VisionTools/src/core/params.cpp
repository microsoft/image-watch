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

#include "stdafx.h"

#if defined(VT_OSX)
#include <limits.h>
#include <libkern/OSAtomic.h>
#define InterlockedIncrement OSAtomicIncrement32
#define InterlockedDecrement OSAtomicDecrement32
#elif defined(VT_ANDROID)
#define InterlockedIncrement(__pVal__) __sync_add_and_fetch(__pVal__,1)
#define InterlockedDecrement(__pVal__) __sync_sub_and_fetch(__pVal__,1)
#endif

#include "vt_params.h"

using namespace vt;

//+----------------------------------------------------------------------------
//
//  Class: CParamValue
// 
//-----------------------------------------------------------------------------
size_t
CParamValue::SizeOfType() const
{
#define CASETYPE(E) \
    case E: return ParamTypeTraits<E>::SizeOf();

    switch(m_eType)
    {
    CASETYPE(ParamType_Byte)
    CASETYPE(ParamType_UShort)
    CASETYPE(ParamType_ULong)
    CASETYPE(ParamType_Rational)
    CASETYPE(ParamType_SByte)
    case ParamType_Undefined:
        return 1;
        break;
    CASETYPE(ParamType_SShort)
    CASETYPE(ParamType_SLong)
    CASETYPE(ParamType_SRational)
    CASETYPE(ParamType_Float)
    CASETYPE(ParamType_Double)
    CASETYPE(ParamType_Bool)
    CASETYPE(ParamType_ASCII)
    CASETYPE(ParamType_String)
    CASETYPE(ParamType_U8)
    CASETYPE(ParamType_I8)
    CASETYPE(ParamType_GUID)
    CASETYPE(ParamType_Params)
    case ParamType_None:
    default:
        break;
    }

#undef CASESET
    return 0;
}

size_t
CParamValue::GetItemCount() const
{
    size_t count = 0;
    if( IsValid() )
    {
        switch( m_eType )
        {
        case ParamType_ASCII:
        case ParamType_String:
            count = 1;
            break;
        default:
            count = m_uiSize/SizeOfType();
            break;
        }
    }
    return count;
}

void CParamValue::Invalidate()
{
    if( IsValid() )
    {
        switch( m_eType )
        {
        case ParamType_Params:
            delete [] (CParams*)m_pvData;
            break;
        default:
            delete [] m_pvData;
            break;
        }

        m_pvData = NULL;
        m_uiSize = 0;
        m_eType = ParamType_None;
    }
}

#pragma warning(push)
#pragma warning(disable:22103) // no way to annotate *val buffer size since it depends on a runtime variable
HRESULT 
CParamValue::GetGeneric(void* val, ParamType type, size_t element, 
                        size_t count) const
{
    // encodes the rules of allowed conversions from the stored type 
    // to the requested type
    if( (element+count) > GetItemCount() )
    {
        return E_INVALIDARG;
    }

    #define CASECOPY(s, d) case s:\
        memcpy(val, (BYTE*)m_pvData + element*sizeof(ParamTypeTraits<s>::Type),\
               count*sizeof(ParamTypeTraits<s>::Type));\
        hr = S_OK;\
        break;

    #define CASECONV(s, d) case s:\
        for( size_t i = 0; i < count; i++ )\
            ((ParamTypeTraits<d>::Type*)val)[i] =\
                ((ParamTypeTraits<s>::Type*)m_pvData)[i+element];\
        hr = S_OK;\
        break;

    HRESULT hr = S_FALSE;
    switch(type)
    {
    case ParamType_Byte:
        switch( m_eType )
        {
        CASECOPY(ParamType_Byte, ParamType_Byte)
        }
        break;
    case ParamType_UShort:
        switch( m_eType )
        {
        CASECONV(ParamType_Byte,   ParamType_UShort)
        CASECOPY(ParamType_UShort, ParamType_UShort)
        }
        break;
    case ParamType_ULong:
        switch( m_eType )
        {
        CASECONV(ParamType_Byte,   ParamType_ULong)
        CASECONV(ParamType_UShort, ParamType_ULong)
        CASECOPY(ParamType_ULong,  ParamType_ULong)
        }
        break;
    case ParamType_SByte:
        switch( m_eType )
        {
        CASECOPY(ParamType_SByte, ParamType_SByte)
        }
        break;
    case ParamType_SShort:
        switch( m_eType )
        {
        CASECONV(ParamType_SByte,  ParamType_SShort)
        CASECOPY(ParamType_SShort, ParamType_SShort)
        }
        break;
    case ParamType_SLong:
        switch( m_eType )
        {
        CASECONV(ParamType_SByte,  ParamType_SLong)
        CASECONV(ParamType_SShort, ParamType_SLong)
        CASECOPY(ParamType_SLong,  ParamType_SLong)
        }
        break;
    case ParamType_U8:
        switch( m_eType )
        {
        CASECONV(ParamType_Byte,   ParamType_U8)
        CASECONV(ParamType_UShort, ParamType_U8)
        CASECONV(ParamType_ULong,  ParamType_U8)
        CASECOPY(ParamType_U8,     ParamType_U8)
        }
        break;
    case ParamType_I8:
        switch( m_eType )
        {
        CASECONV(ParamType_SByte,  ParamType_I8)
        CASECONV(ParamType_SShort, ParamType_I8)
        CASECONV(ParamType_SLong,  ParamType_I8)
        CASECOPY(ParamType_I8,     ParamType_I8)
        }
        break;
    case ParamType_Float:
        switch( m_eType )
        {
        CASECONV(ParamType_SByte,  ParamType_Float)
        CASECONV(ParamType_SShort, ParamType_Float)
        CASECONV(ParamType_Byte,   ParamType_Float)
        CASECONV(ParamType_UShort, ParamType_Float)
        CASECOPY(ParamType_Float,  ParamType_Float)
        }
        break;
    case ParamType_Double:
        switch( m_eType )
        {
        CASECONV(ParamType_SByte,  ParamType_Double)
        CASECONV(ParamType_SShort, ParamType_Double)
        CASECONV(ParamType_SLong,  ParamType_Double)
        CASECONV(ParamType_Byte,   ParamType_Double)
        CASECONV(ParamType_UShort, ParamType_Double)
        CASECONV(ParamType_ULong,  ParamType_Double)
        CASECONV(ParamType_Float,  ParamType_Double)
        CASECOPY(ParamType_Double, ParamType_Double)
        }
        break;
    case ParamType_Rational:
        switch( m_eType )
        {
        // sometimes rationals are stored as I8s, U8s
        CASECOPY(ParamType_Rational, ParamType_Rational)
        CASECOPY(ParamType_U8,       ParamType_Rational)
        CASECOPY(ParamType_I8,       ParamType_Rational)
        }
        break;
    case ParamType_SRational:
        switch( m_eType )
        {
        // sometimes rationals are stored as I8s, U8s
        CASECOPY(ParamType_SRational, ParamType_SRational)
        CASECOPY(ParamType_I8,        ParamType_Rational)
        CASECOPY(ParamType_U8,        ParamType_Rational)
        }
        break;
    case ParamType_Bool:
        switch( m_eType )
        {
        CASECOPY(ParamType_Bool, ParamType_Bool)
        }
        break;
    case ParamType_GUID:
        switch( m_eType )
        {
        CASECOPY(ParamType_GUID, ParamType_GUID)
        }
        break;
    default:
        VT_ASSERT(0);
        break;
    }

    if( hr == S_FALSE )
    {
		VT_ASSERT(!"Warning: invalid param");
	}
    
    #undef CASECONV
    #undef CASECOPY

    return hr;
}
#pragma warning(pop)

HRESULT CParamValue::SetData(ParamType e, size_t uiSz, const void *pv)
{
    Invalidate();

    m_eType  = e;
    m_uiSize = uiSz;
    if( e == ParamType_None )
    {
        return NOERROR;
    }

    VT_ASSERT( pv != NULL );

    m_pvData = (void *)VT_NOTHROWNEW Byte [m_uiSize];
    if(m_pvData == NULL)
    {
        m_eType = ParamType_None;
        return E_OUTOFMEMORY;
    }
    memcpy(m_pvData, pv, m_uiSize);
    return NOERROR;
}

HRESULT CParamValue::Set(const CParamValue& val)
{
    if( val.GetType() == ParamType_Params )
    {
        return SetArray((CParams*)val.GetDataPtr(), val.GetItemCount());
    }
    return SetData( val.GetType(), val.GetDataSize(), val.GetDataPtr() );
}

//+----------------------------------------------------------------------------
//
//  Class: CParams
// 
//-----------------------------------------------------------------------------
CParams::CParams()
    : m_pCreateListTail(NULL), m_iCount(0), m_pRef(NULL)
{
    int i;
    for(i=0; i<PARX_HASHSIZE; i++)
        m_rgpHashList[i] = NULL;
}

CParams::CParams(const CParams &cParams)
    : m_pCreateListTail(NULL), m_iCount(0), m_pRef(NULL)
{
    int i;
    for(i=0; i<PARX_HASHSIZE; i++)
        m_rgpHashList[i] = NULL;
    CParams::operator=(cParams);
}

CParams::~CParams()
{
    DeleteAll();
}

HRESULT 
CParams::GetById(CParamValue** ppVal, UINT uId, UINT uIndex) const
{
    return Get(ppVal, NULL, uId, uIndex);
}

HRESULT 
CParams::GetById(const CParamValue** ppVal, UINT uId, UINT uIndex) const
{
    return GetById(const_cast<CParamValue**>(ppVal), uId, uIndex);
}

HRESULT
CParams::GetByName(CParamValue** ppVal, const WCHAR *pszName, 
                   UINT uIndex) const
{
    if( pszName == NULL )
    {
        return E_INVALIDARG;
    }
    return Get(ppVal, pszName, UINT_MAX, uIndex);
}

HRESULT
CParams::GetByName(const CParamValue** ppVal, const WCHAR *pszName, 
                   UINT uIndex) const
{
    return GetByName(const_cast<CParamValue**>(ppVal), pszName, uIndex);
}

HRESULT 
CParams::Get(CParamValue** ppVal, const WCHAR *pszName, UINT uId, 
             UINT uIndex) const
{
    if( ppVal == NULL )
    {
        return E_POINTER;
    }

    int iHash;
    PARAM_ENTRY *pEntry = FindHashEntry(pszName, uId, uIndex, &iHash);
    if(pEntry==NULL)
    {
        *ppVal = NULL;
        return S_FALSE;
    }

    *ppVal = &pEntry->sVal;
    return S_OK;
}

HRESULT 
CParams::SetById(UINT uId, UINT uIndex, const CParamValue &sEntry)
{
    return Set(NULL, uId, uIndex, sEntry);
}

HRESULT 
CParams::SetByName(const WCHAR *pszName, UINT uIndex, const CParamValue &sEntry)
{
    if( pszName == NULL )
    {
        return E_INVALIDARG;
    }
    return Set(pszName, UINT_MAX, uIndex, sEntry);
}

HRESULT
CParams::Reset()
{
    HRESULT hr = S_OK;

    if (m_pRef == NULL)
    {
        // create new reference
        m_pRef = VT_NOTHROWNEW LONG;
        VT_PTR_OOM_EXIT( m_pRef );
        *m_pRef = 1;
    }
    else if (InterlockedExchange(m_pRef, *m_pRef) > 1)
    {
        // if more than one reference need deep copy

        // save old hash list headers
        PARAM_ENTRY *rgpHashList[PARX_HASHSIZE];
        for(int i=0; i<PARX_HASHSIZE; i++)
            rgpHashList[i] = m_rgpHashList[i];
        PARAM_ENTRY *pCreateListTail = m_pCreateListTail;
        int iCount = m_iCount;

        // initialize new hash list headers
        for(int i=0; i<PARX_HASHSIZE; i++)
            m_rgpHashList[i] = NULL;
        m_pCreateListTail = NULL;
        m_iCount = 0;

        // deep copy, without checking refcount re-entrantly
        VT_HR_EXIT( Merge(pCreateListTail, false) );

        // release our reference to old copy
        if (InterlockedDecrement(m_pRef) == 0)
        {
            // oops, new copy no longer needed
            DeleteAllEntries();

            // use old hash list headers
            for(int i=0; i<PARX_HASHSIZE; i++)
                m_rgpHashList[i] = rgpHashList[i];
            m_pCreateListTail = pCreateListTail;
            m_iCount = iCount;
        }
        else
        {
            // create new reference
            m_pRef = VT_NOTHROWNEW LONG;
            VT_PTR_OOM_EXIT( m_pRef );
        }
        *m_pRef = 1;
    }

Exit:
    return hr;
}

HRESULT 
CParams::Set(const WCHAR *pszName, UINT uId, UINT uIndex, 
             const CParamValue &sEntry, bool bReset)
{
    size_t uL(0);
#if !defined(VT_ANDROID)
    // validate the name
    if( pszName != NULL )
    {
        uL = wcslen(pszName);
        for(size_t i=0; i<uL; i++)
        {
            if( !isgraph (pszName[i]))
            {
                return E_BADFORMAT;
            }
        }
    }
#endif
    HRESULT hr = NOERROR;

    // make sure we aren't sharing hash list
    if (bReset)
        VT_HR_EXIT( Reset() );

    int iHash;
    PARAM_ENTRY *pEntry = FindHashEntry(pszName, uId, uIndex, &iHash);

    if(pEntry == NULL)
    {
        // create new entry at iHash
        pEntry = VT_NOTHROWNEW PARAM_ENTRY;
		VT_PTR_OOM_EXIT( pEntry );
        pEntry->pszName = NULL;
        pEntry->uId = UINT_MAX;

        if( pszName == NULL )
        {
            pEntry->uId = uId;
        }
        else
        {
            pEntry->pszName = VT_NOTHROWNEW WCHAR [uL + 1];
            if (NULL == pEntry->pszName)
			{
				hr = E_OUTOFMEMORY;
				goto E2;
			}

			if (0 != VtStringCopy(pEntry->pszName, uL+1, pszName))
			{
				hr = E_FAIL;
				goto E2;
			}
        }
        
        pEntry->uIndex = uIndex;

        // copy data
        if (FAILED(hr = pEntry->sVal.Set(sEntry)))
			goto E2;
        
        // add to hash list at head
        pEntry->pNextHash = m_rgpHashList[iHash];
        pEntry->pPrevHash = NULL;
        m_rgpHashList[iHash] = pEntry;
        if(pEntry->pNextHash)
            pEntry->pNextHash->pPrevHash = pEntry;

        // add to create list at tail
        pEntry->pPrevCreate = m_pCreateListTail;
        pEntry->pNextCreate = NULL;
        m_pCreateListTail = pEntry;
        if(pEntry->pPrevCreate)
            pEntry->pPrevCreate->pNextCreate = pEntry;

        m_iCount++;

    E2:
        if ( hr != S_OK )
        {
            delete [] pEntry->pszName;
            delete pEntry;
        }
    }
    else // already exists
    {
        // copy data
        VT_HR_EXIT( pEntry->sVal.Set(sEntry) );
    }

Exit:
    return hr;
}

HRESULT
CParams::DeleteById(UINT uId, UINT uIndex)
{
    return Delete(NULL, uId, uIndex);
}

HRESULT
CParams::DeleteByName(const WCHAR *pszName, UINT uIndex)
{
    if( pszName == NULL )
    {
        return E_INVALIDARG;
    }
    return Delete(pszName, UINT_MAX, uIndex);
}

HRESULT 
CParams::Delete(const WCHAR *pszName, UINT uId, UINT uIndex)
{
    HRESULT hr = NOERROR;

    // make sure we aren't sharing hash list
    VT_HR_EXIT( Reset() );

    int iHash;
    PARAM_ENTRY *pEntry = FindHashEntry(pszName, uId, uIndex, &iHash);
    if(pEntry==NULL)
        return E_NOTFOUND;
    
    DeleteHashEntry(pEntry, iHash);

Exit:
    return hr;
}

PARAM_ENUM_TOKEN CParams::StartEnum() const
{
    PARAM_ENTRY* pToken = m_pCreateListTail;
    if( pToken )
    {
        while( pToken->pPrevCreate)
            pToken = pToken->pPrevCreate;
    }

    PARAM_ENUM_TOKEN tok;
    tok.ptr = pToken;

    return tok;
}

bool CParams::NextEnum(PARAM_ENUM_TOKEN& tok,
                       const WCHAR **ppszNameRtn, UINT *puIdRtn, 
                       UINT *puIndexRtn, const CParamValue **ppValueRtn) const
{
    PARAM_ENTRY* pEnum = (PARAM_ENTRY*)tok.ptr;
    if( pEnum == NULL )
        return false;

    if(ppszNameRtn)
        *ppszNameRtn = pEnum->pszName;
    if(puIdRtn)
        *puIdRtn = pEnum->uId;
    if(puIndexRtn)
        *puIndexRtn = pEnum->uIndex;
    if(ppValueRtn)
        *ppValueRtn = &pEnum->sVal;

    tok.ptr = pEnum->pNextCreate;
    return true;
}

//+-----------------------------------------------------------------------
//
//  Member:    CParams::operator=
//
//  Synopsis:  assignment operator. this doesn't copy any params, just
//              increments a shared reference count
//
//  Returns:   new params
//
//------------------------------------------------------------------------

CParams & CParams::operator=(const CParams &cParams)
{
    if(&cParams == this)
        return *this;

    DeleteAll();

    if (cParams.m_pRef != NULL)
    {
        m_pRef = cParams.m_pRef;
        InterlockedIncrement(m_pRef);

        // shallow copy
        for(int i=0; i<PARX_HASHSIZE; i++)
            m_rgpHashList[i] = cParams.m_rgpHashList[i];
        m_pCreateListTail = cParams.m_pCreateListTail;
        m_iCount = cParams.m_iCount;
    }

    return *this;
}

void CParams::DeleteAll()
{
    if (m_pRef == NULL || InterlockedDecrement(m_pRef) > 0)
    {
        // shallow delete
        for(int i=0; i<PARX_HASHSIZE; i++)
            m_rgpHashList[i] = NULL;
        m_pCreateListTail = NULL;
        m_iCount = 0;
    }
    else
    {
        // deep delete
        DeleteAllEntries();
        delete m_pRef;
    }
    m_pRef = NULL;
}

void CParams::DeleteAllEntries()
{
    int i;
    for(i=0; i<PARX_HASHSIZE; i++)
    {
        PARAM_ENTRY *pEntry;
        while((pEntry = m_rgpHashList[i])!=NULL)
            DeleteHashEntry(pEntry, i);
    }
    m_iCount = 0;
}

HRESULT CParams::Merge(const CParams *pParams)
{
    if(pParams==NULL || pParams==this)
        return NOERROR;

    return Merge(pParams->m_pCreateListTail);
}

HRESULT CParams::Merge(const PARAM_ENTRY *pEnum, bool bReset)
{
    if(pEnum==NULL)
        return NOERROR;

    while(pEnum->pPrevCreate)
        pEnum = pEnum->pPrevCreate;

    HRESULT hr = NOERROR;

    CParamValue val;

    for(; pEnum; pEnum = pEnum->pNextCreate)
    {
        VT_HR_EXIT( Set(pEnum->pszName, pEnum->uId, pEnum->uIndex, 
                        pEnum->sVal, bReset) );
    }

Exit:
    return hr;
}

void CParams::DeleteHashEntry(PARAM_ENTRY *pEntry, int iHash)
{
    // unlink from hash chain
    if(pEntry->pPrevHash)
        pEntry->pPrevHash->pNextHash = pEntry->pNextHash;
    else
        m_rgpHashList[iHash] = pEntry->pNextHash;
    if(pEntry->pNextHash)
        pEntry->pNextHash->pPrevHash = pEntry->pPrevHash;

    // unlink from create chain
    if(pEntry->pPrevCreate)
        pEntry->pPrevCreate->pNextCreate = pEntry->pNextCreate;
    if(pEntry->pNextCreate)
        pEntry->pNextCreate->pPrevCreate = pEntry->pPrevCreate;
    else
        m_pCreateListTail = pEntry->pPrevCreate;

    VT_ASSERT( (pEntry->pszName == NULL && pEntry->uId != UINT_MAX) ||
               (pEntry->pszName != NULL && pEntry->uId == UINT_MAX) );

    // delete
    delete [] pEntry->pszName;
    delete pEntry;

    m_iCount--;
}

PARAM_ENTRY *CParams::FindHashEntry(const WCHAR *pszName, UINT uId, UINT uIndex, 
                                   int *piHashRtn) const
{
    const WCHAR *pszPtr;
    int iHash = (uIndex + 2) % PARX_HASHSIZE;

    PARAM_ENTRY *pEntry;
    if( pszName == NULL )
    {
        iHash = (iHash * 17 + uId) % PARX_HASHSIZE;

        pEntry = m_rgpHashList[iHash];
        for(; pEntry!=NULL; pEntry = pEntry->pNextHash)
        {
            if(uIndex==pEntry->uIndex && pEntry->pszName==NULL && 
               uId == pEntry->uId )
            {
                break;
            }
        }
    }
    else
    {
        for(pszPtr = pszName; *pszPtr; pszPtr++)
            iHash = (iHash * 17 + *pszPtr) % PARX_HASHSIZE;

        pEntry = m_rgpHashList[iHash];
        for(; pEntry!=NULL; pEntry = pEntry->pNextHash)
        {
            if(uIndex==pEntry->uIndex && 
               (pEntry->pszName!=NULL && wcscmp(pszName, pEntry->pszName)==0))
            {
                break;
            }
        }
    }

    *piHashRtn = iHash;

    return pEntry;
}
