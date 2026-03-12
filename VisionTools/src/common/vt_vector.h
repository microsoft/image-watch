//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      STL like vector class
//
//  History:
//      2004/11/8-mattu
//			Created
//
//------------------------------------------------------------------------

#pragma once

#include "vt_new.h"
#include "vt_dbg.h"
#include "vt_basetypes.h"

#include <string.h>

// special support for vectors of types that cannot be "memmoved" bitwise
namespace vt
{
	// fallback implementation
	namespace vectorhelpers
	{
		template<typename T, typename Enable = void>
		struct ObjectMover
		{
			static void PrepareMove(const T* /*src*/, const T* /*dst*/, 
				size_t /*n_elem*/)
			{
			}
			static void FinalizeMove(const T* /*src*/, const T* /*dst*/, 
				size_t /*n_elem*/)
			{
			}		
		};
	};
};

namespace vt {

// vector is a no-nonsense STL like vector class template.
// The template merely manages dynamic memory for a vector
// and thus has minimal or no overhead compared to manually allocation/freeing 
// dynamic memory.
// It keeps all items in a consecutive memory block so in case it needs to grow 
// memory it must copy items which might include calling copy ctor and dtor. 
// You may reserve some memory upfront to minimize reallocations.

#if 0
/**/    // 1. can be used with simple types or classes (must have copy ctor)
/**/
/**/    vector<int>         vectorInt;
/**/    vector<string>      vectorStr;
/**/    vector<POINT>		vectorPoint;
/**/
/**/    // 2. reserve memory to avoid reallocations
/**/
/**/    vectorInt.reserve(100); // reserve space for 100 items
/**/
/**/    vectorInt[99] = 99;
/**/    
/**/    // 3. adding items
/**/
/**/    vectorStr.push_back(str);
/**/
/**/    // 4. removing items
/**/
/**/    vectorInt.pop_back(); // remove last item
/**/
/**/    vectorInt.clear(); // remove all items and clears memory
/**/    
/**/	// 5. iterating	the STL way
/**/
/**/	for(vector<POINT>::iterator it = vectorPoint.begin(), itEnd = vectorPoint.end(); it != itEnd; ++it)
/**/		if(it->x == 10)
/**/			break;
#endif

// vector template class
template <typename T, unsigned int uAlign4 = 0>
class vector
{
public:
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef vector<T>  _Myt;
    typedef size_t size_type;
    
    vector()
        : _Mem(NULL), _First(NULL), _Last(NULL), _End(NULL)
        {}

    // copy ctor
    vector(const _Myt& _X)
        : _Mem(NULL), _First(NULL), _Last(NULL), _End(NULL)
    {  
        operator=(_X);
    }

    ~vector()
    {
        clear();
    }
    
    // begin
    const_iterator begin() const
        {return _First; }

    iterator begin()
        {return _First; }
    
    // end
    const_iterator end() const
        {return _Last; }

    iterator end()
        {return _Last; }
    
    // back
    const T& back() const
        {return *(_Last-1); }

    T& back()
        {return *(_Last-1); }

    // front
    const T& front() const
        {return *_First; }

    T& front()
        {return *_First; }

    // const operator[]
    const T& operator[](size_type i) const
    {
        VT_ASSERT_BOUNDS( i < size() );
        return *(_First + i);
    }
    
    // operator[]
    T& operator[](size_type i)
    {
        VT_ASSERT_BOUNDS( i < size() );
        return *(_First + i);
    }

    // operator =
    _Myt& operator= (const _Myt& _X)
    {
        _Destroy(_First, _Last);
        
        size_t szdst = capacity();
        size_t szsrc = _X.size();

        if( szsrc > szdst )
        {
            if( !_Grow(szsrc-szdst) ) return *this;
        }
        _Last = _Copy(_X.begin(), _X.end(), _First);
        return *this;
    }

    // swap
    void swap(_Myt& _X)
    {
        iterator _Temp;
        _Temp     = _Mem;
        _Mem      = _X._Mem;
        _X._Mem   = _Temp;
        _Temp     = _First;
        _First    = _X._First;
        _X._First = _Temp;
        _Temp     = _Last;
        _Last     = _X._Last;
        _X._Last  = _Temp;
        _Temp     = _End;
        _End      = _X._End;
        _X._End   = _Temp;
    }

    // reserve
    HRESULT reserve(size_type n)
    {
        if( n > capacity() )
        {
            _Grow(n - capacity());
        }
        
        return (n <= capacity())? S_OK: E_OUTOFMEMORY;
    }

    // resize
    HRESULT resize(size_type n)
    {
        if( n > size() )
        {
            size_type cap = capacity();  
            if( n > cap )
            {
                // always grow by at least 12.5% or 4 if current capacity is 0
				if( !_Grow( VtMax(n-cap, (cap==0) ? 4: (cap+7)/8) ) )
                    return E_OUTOFMEMORY;
            }

			T* _P = _Last;
            for (; _P != _First+n; ++_P)
                VT_PLACENEW(_P) T;
            _Last = _P;
        }
        else if( n < size() )
        {
            erase(_First+n, _Last);
        }
        return S_OK;
    }

    // clear
    void clear()
    {
        // destroy all elements in vector
        _Destroy(_First, _Last);

        // deallocate memory
        delete [] (char*)_Mem;

        _Mem = _Last = _First = _End = NULL;
    }

    // erase
    iterator erase(iterator _Where)
    {
        return erase(_Where, _Where+1);
    }

    iterator erase(iterator _F, iterator _L)
    {
        VT_ASSERT_BOUNDS( _F >= _First && _F <  _Last );
        VT_ASSERT_BOUNDS( _L >= _First && _L <= _Last && _L > _F );
        
		// XXX: call destroy here
        iterator _P = _F;
        for (; _P < _L && _P < _Last; ++_P)  // remove designated elements
			_P->~T();
		
        _MoveElements(_P, _F, (_Last-_P));   // copy remaining elements
        
        _Last = _F + (_Last-_P);

        return _F;
    }

	// insert
	HRESULT insert(size_type i, const T& x)
	{
        VT_ASSERT_BOUNDS( i <= size() );

        if(_Last == _End)
        {
            size_type sz = size();
            _Grow((sz==0)? 4: (sz+7)/8);  // grow by 12.5%
        }

		if(_Last == _End)
            return E_OUTOFMEMORY; // couldn't allocate memory

		// move later elements
		iterator p = _First + i;
		_MoveElements(p, p + 1, (_Last - p));
		_Last++;

		// insert new element
		VT_PLACENEW(p) T(x);

		return S_OK;
	}

    // size
    size_type size() const
        {return _Last - _First; }

    // capacity
    size_t capacity() const
        {return _End - _First; }

    // empty
    bool empty() const
        {return (size() == 0); }
    
    // push_back
    HRESULT push_back(const T& x)
    {
        if(_Last == _End)
        {
            size_type sz = size();
            _Grow((sz==0)? 4: (sz+7)/8);  // grow by 12.5%
        }
        
        if(_Last == _End)
            return E_OUTOFMEMORY; // couldn't allocate memory

        // insert element at the end of vector
		VT_PLACENEW(_Last++) T(x);

        return S_OK;
    }

    // pop_back
    void pop_back()
        {_Destroy(_Last - 1, _Last); --_Last; }

protected:
	static void _MoveElements(const T* src, T* dst, size_t n_elem)
	{
		vectorhelpers::ObjectMover<T>::PrepareMove(src, dst, n_elem);
		::memmove((void*)dst, (void*)src, n_elem * sizeof(T));
/*
        // We should do a C++ safe move 
        // Unfortunately, VisionTools doesn't compile if we do it like this
        if(dst <= src)
        {
            std::copy(src, src + n_elem, dst);
        }
        else
        {
            std::copy_backward(src, src + n_elem, dst + n_elem);
        }
*/
		vectorhelpers::ObjectMover<T>::FinalizeMove(src, dst, n_elem);
	}

    bool _Grow(size_type _N)
    {
        _N += capacity();
        
        // allocate memory
        size_type szAllocSize = _N * sizeof(T);
        size_type szAlign  = (4<<uAlign4);
        if( szAlign > 4 )
        {
            szAllocSize += szAlign-4;
        }

        iterator _New = 
            reinterpret_cast<iterator>(VT_NOTHROWNEW char[szAllocSize]);
		if(_New == NULL)
			return false;

        iterator _NewFirst = _New;

        // align
        size_type szAlignMask = INT_PTR(_NewFirst)&(szAlign-1);
        if( szAlignMask != 0 )
        {
            VT_ASSERT( uAlign4 > 0 );
            _NewFirst = (iterator)(((char*)_NewFirst) + szAlign - szAlignMask);
        }

		// copy
        _MoveElements(_First, _NewFirst, size());
		
        // deallocate old memory
        delete [] (char*)_Mem;

        _Mem   = _New;
		_Last  = _NewFirst + size();
        _End   = _NewFirst + _N;
		_First = _NewFirst;
		return true;
    }

    // destroy elements from _F to _L
    void _Destroy(iterator _F, iterator _L)
    {
        for (; _F != _L; ++_F)
			_F->~T();
    }

    // copy elements from _F to _L to new location starting at _P
    iterator _Copy(const_iterator _F, const_iterator _L, iterator _P)
	{
        for (; _F != _L; ++_P, ++_F)
		    VT_PLACENEW(_P) T(*_F);
		
        return (_P);
    }
	
private:
    iterator _Mem, _First, _Last, _End;
};

};
