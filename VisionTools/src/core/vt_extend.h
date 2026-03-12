//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for image filtering
//
//  History:
//      2004/11/08-swinder
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_atltypes.h"

namespace vt {

#ifdef MAKE_DOXYGEN_WORK
void foo();
#endif

/// \ingroup geofunc
/// <summary> Describes the padding operation to perform when access pixels outside of
/// the image boundaries </summary>
enum ExtendMode 
{   
	Zero = 0,            ///< fill with Zeroes
	Wrap = 1,            ///< tile the source image on an infinite canvas, e.g. used for 360 panoramas
	Extend = 2,          ///< extend the nearest in bounds pixel value
	ExtendZeroAlpha = 3, ///< same as Extend except zero out the alpha channel for RGBA images 
	Constant = 4,        ///< fill with a constant value
    TypeMax = 5,         ///< fill with the max value supported by a particular type
    TypeMin = 6,         ///< fill with the min value supported by a particular type
//	Mirror = 7           ///< tile the source using reflected pixels
};

inline bool ExtendModeAllowsEmptySrc(ExtendMode ex)
{ return ex != Wrap && ex != Extend && ex != ExtendZeroAlpha; }

// Struct that holds the constant value for ExtendMode = Constant
struct EXTEND_CONSTVAL
{
	union
	{
		Byte  bv[8];
		Byte* pv;
	};
	UInt32 uSize;
    int iType;

	EXTEND_CONSTVAL() : pv(NULL), uSize(0), iType(OBJ_UNDEFINED)
	{}

	~EXTEND_CONSTVAL()
	{ Clear(); } 

	void* Val()
	{ return (uSize==0)? NULL: (uSize>8)? pv: bv;}

	const void* Val() const
	{ return (uSize==0)? NULL: (uSize>8)? pv: bv;}

	HRESULT Alloc(int type)
	{  
		HRESULT hr = S_OK;
		Clear();
        UInt32 uSizeInit = VT_IMG_BANDS(type) * VT_IMG_ELSIZE(type);
		if( uSizeInit==0 )
		{
			pv = NULL;
			uSize = 0;
		}
		else if( uSizeInit > 8 )
		{
			pv = VT_NOTHROWNEW Byte[uSizeInit];
			if( pv == NULL )
			{
				hr = E_OUTOFMEMORY;
			}
			else
			{
				uSize = uSizeInit;
			}
		}
		else
		{
			uSize = uSizeInit;
        }

        iType = type;
        return hr;
    }

public:
    /// \ingroup geofunc
    /// <summary> Initialize the EXTEND_CONSTVAL using a composite pixel type or 
	/// a simple element type. </summary>
    template <typename T>
    HRESULT Initialize(const T& value)
    { 
		static_assert(IsStandardElementType<T>::value || IsStandardPixelType<T>::value,
			"T must be a standard VisionTools element or pixel type");

		return InitializeDispatch(value, typename IsStandardPixelType<T>::type()); 
	}

    /// \ingroup geofunc
    /// <summary> Initialize the EXTEND_CONSTVAL using an array of simple 
	/// element types. </summary>
    template <typename T>
    HRESULT Initialize(const T* values, int bands)
    {
		static_assert(IsStandardElementType<T>::value,
			"T must be a standard VisionTools element type");

		int type = VT_IMG_MAKE_TYPE(ElTraits<T>::ElFormat(), bands);

		return InitializeInternal(values, type);
    }
		
	HRESULT Initialize(const EXTEND_CONSTVAL* pV)
	{ return InitializeInternal(pV->Val(), pV->iType); }

	void Clear()
	{
		if( uSize > 8 )
		{
			delete [] pv;
		}
		pv    = NULL;
		uSize = 0;
        iType = OBJ_UNDEFINED;
	}

	bool operator==(const EXTEND_CONSTVAL& other) const
	{
		return uSize == other.uSize && 0 == memcmp(Val(), other.Val(), uSize) &&
            VT_IMG_SAME_PBE(iType, other.iType);
	}

protected:

	// initialize with standard pixel type
    template <typename T>
    HRESULT InitializeDispatch(const T& value, const std::true_type&)
    { 
		static_assert(IsStandardPixelType<T>::value, 
			"internal logic error");

		int type = VT_IMG_MAKE_TYPE(ElTraits<typename T::ELT>::ElFormat(), 
									T::Bands());

		return InitializeInternal(&value, type); 
    }
    
	// initialize with standard element type
	template <typename T>
    HRESULT InitializeDispatch(const T& value, const std::false_type&)
    { 
		static_assert(IsStandardElementType<T>::value, 
			"internal logic error");

		return InitializeInternal(&value, ElTraits<T>::ElFormat()); 
	}

    HRESULT InitializeInternal(const void* pvInit, int type)
    {
		HRESULT hr = S_OK;
		if( pvInit == NULL )
		{
			Clear();
			pv       = NULL;
			uSize    = 0;
			iType    = OBJ_UNDEFINED;
		}
		else if( (hr = Alloc(type)) == S_OK )
        {
            memcpy(Val(), pvInit, VT_IMG_BANDS(type) * VT_IMG_ELSIZE(type));
		}
        return hr;
    }

private:
	// copy is verbotten
	EXTEND_CONSTVAL(const EXTEND_CONSTVAL&);
	EXTEND_CONSTVAL &operator=(const EXTEND_CONSTVAL&);
};

/// \ingroup geofunc
/// <summary> Holds the vertical and horizontal extend modes that
///           describe an image padding operation </summary>
struct IMAGE_EXTEND
{
	ExtendMode exHoriz;
	ExtendMode exVert;
	EXTEND_CONSTVAL excHoriz;
	EXTEND_CONSTVAL excVert;

	/// <summary> Default constructor, sets ExtendMode Zero horizontally and 
	/// vertically <summary>
	IMAGE_EXTEND() : exHoriz(Zero), exVert(Zero)
	{}
	  
	/// <summary> Constructor with same ExtendMode horizontally and vertically 
	/// <summary>
	explicit IMAGE_EXTEND(ExtendMode ex) : exHoriz(ex), exVert(ex)
	{}
	
	/// <summary> Constructor with different ExtendModes horizontally and 
	/// vertically <summary>
	IMAGE_EXTEND(ExtendMode exH, ExtendMode exV) : exHoriz(exH), exVert(exV)
	{} 

	/// <summary> Initialize with custom fill colors <summary>
	template<class T>
	HRESULT Initialize(ExtendMode exH, ExtendMode exV, 
					   const T& ClrH, const T& ClrV)
	{ 
		HRESULT hr = S_OK;
		if( (exHoriz=exH) == Constant )
		{
			hr = excHoriz.Initialize(ClrH);
		}
		if( hr == S_OK && ((exVert=exV) == Constant) )
		{
			hr = excVert.Initialize(ClrV);
		}
		return hr;
	}

	HRESULT Initialize(ExtendMode exH, ExtendMode exV)
	{
        exHoriz=exH;
        exVert=exV;
        excHoriz.Clear();
        excVert.Clear();
		return S_OK;
	}

	template<class T>
	HRESULT Initialize(ExtendMode exH, ExtendMode exV, 
					   const T* pClrHoriz, const T* pClrVert, int iBandCnt)
	{
		HRESULT hr = S_OK;
		if( (exHoriz=exH) == Constant )
		{
			hr = excHoriz.Initialize(pClrHoriz, iBandCnt);
		}
		if( hr == S_OK && ((exVert=exV) == Constant) )
		{
			hr = excVert.Initialize(pClrVert, iBandCnt);
		}
		return hr;
	}

	HRESULT Initialize(const IMAGE_EXTEND* pEx)
	{
		HRESULT hr = S_OK;
		if( pEx == NULL )
		{
			exHoriz = exVert = Zero;
			excHoriz.Clear();
			excVert.Clear();
		}
		else
		{
			exHoriz = pEx->exHoriz;
			exVert  = pEx->exVert;
			if( (hr = excHoriz.Initialize(&pEx->excHoriz))== S_OK )
			{
				hr = excVert.Initialize(&pEx->excVert);
			}
		}
		return hr;
	}

	bool IsVerticalSameAsHorizontal() const
	{
		if (exHoriz != Constant)
			return exHoriz == exVert;

		return exVert == Constant && excHoriz == excVert;
	}

    bool EmptySourceOk() const
    { 
        return ExtendModeAllowsEmptySrc(exHoriz) && 
               ExtendModeAllowsEmptySrc(exVert);
    }

    bool operator==(const IMAGE_EXTEND& other) const
	{ 
        return exHoriz==other.exHoriz   &&
               exVert==other.exVert     &&
               excHoriz==other.excHoriz &&
               excVert==other.excVert;
    }

private:
	IMAGE_EXTEND(const IMAGE_EXTEND&);
	IMAGE_EXTEND &operator=(const IMAGE_EXTEND&);
};

};
