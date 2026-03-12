//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Image classes
//
//  History:
//      2004/11/08-swinder
//			Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"

namespace vt {

#ifdef MAKE_DOXYGEN_WORK
void foo();
#endif

////////////////////////////////////////////////////////////////////////////////
//
// image type bitfield layout
//
//
// | 31 - 24 |     23    |    22    |   21-16    |   15-12  |  11-3 |  0-2   |
// | reserved| fixed-pix | fixed-el | pix-format | reserved | bands | eltype |
//

#define VT_IMG_ELFRMT_MASK 7

#define VT_IMG_BANDS_SHIFT 3
#define VT_IMG_BANDS_MAX   512
#define VT_IMG_BANDS_MASK  ((VT_IMG_BANDS_MAX-1) << VT_IMG_BANDS_SHIFT)

#define VT_IMG_PIXFRMT_SHIFT 16
#define VT_IMG_PIXFRMT_MAX   63
#define VT_IMG_PIXFRMT_MASK  (VT_IMG_PIXFRMT_MAX << VT_IMG_PIXFRMT_SHIFT)

#define VT_IMG_FIXED_ELFRMT_SHIFT 22
#define VT_IMG_FIXED_ELFRMT_MASK  (1 << VT_IMG_FIXED_ELFRMT_SHIFT)

#define VT_IMG_FIXED_PIXFRMT_SHIFT 23
#define VT_IMG_FIXED_PIXFRMT_MASK  (1 << VT_IMG_FIXED_PIXFRMT_SHIFT)

#define VT_IMG_BANDS(type) \
    ((((type) & VT_IMG_BANDS_MASK) >> VT_IMG_BANDS_SHIFT) + 1)

#define VT_IMG_ELSIZE(type) \
	(((type & VT_IMG_ELFRMT_MASK) == EL_FORMAT_HALF_FLOAT)? \
	    2: (1 << ((type & VT_IMG_ELFRMT_MASK) >> 1)))

#define VT_IMG_MAKE_TYPE(elfrmt, bands) \
    ((((bands-1) << VT_IMG_BANDS_SHIFT) & (VT_IMG_BANDS_MASK)) | elfrmt)

#define VT_IMG_MAKE_COMP_TYPE(pixfrmt, elfrmt, bands) \
    ((pixfrmt) | VT_IMG_MAKE_TYPE(elfrmt, bands))

#define VT_IMG_FIXED(t) ((t)|VT_IMG_FIXED_ELFRMT_MASK|VT_IMG_FIXED_PIXFRMT_MASK)

/// @name Format Macros
///@{ 
/// \def EL_FORMAT
/// \ingroup pixformat
/// \brief Macro for extracting element format from image type
#define EL_FORMAT(type)	 ((type) & VT_IMG_ELFRMT_MASK)
/// \def PIX_FORMAT
/// \ingroup pixformat
/// \brief Macro for extracting pixel format from image type
#define PIX_FORMAT(type) ((type) & VT_IMG_PIXFRMT_MASK)
///@}

#define EL_B_FORMAT(type)((type) & (VT_IMG_BANDS_MASK|VT_IMG_ELFRMT_MASK))

#define PIX_B_FORMAT(type)((type) & (VT_IMG_PIXFRMT_MASK|VT_IMG_BANDS_MASK))

#define IMG_FORMAT(type) ((type) & (VT_IMG_PIXFRMT_MASK|VT_IMG_BANDS_MASK|VT_IMG_ELFRMT_MASK))

#define VT_IMG_SAME_E(a, b)    (EL_FORMAT(a) == EL_FORMAT(b))

#define VT_IMG_SAME_BE(a, b)   (EL_B_FORMAT(a) == EL_B_FORMAT(b))

#define VT_IMG_SAME_PB(a, b)   (PIX_B_FORMAT(a) == PIX_B_FORMAT(b))

#define VT_IMG_SAME_PBE(a, b)  (IMG_FORMAT(a) == IMG_FORMAT(b))


#define VT_IMG_ISUNDEF(a)      (IMG_FORMAT(a)  == VT_IMG_PIXFRMT_MASK)

////////////////////////////////////////////////////////////////////////
// Element types
//
/// @name Element Format Constants
///@{ 
/// \def EL_FORMAT_BYTE
/// \ingroup pixformat
/// <summary>  Unsigned Byte (8 bit) format. </summary> 
#define EL_FORMAT_BYTE		 0

/// \def EL_FORMAT_SBYTE
/// \ingroup pixformat
/// <summary>  Signed Byte (8 bit) format. </summary> 
#define EL_FORMAT_SBYTE		 1

/// \def EL_FORMAT_SHORT
/// \ingroup pixformat
/// <summary> Unsigned Short int (16 bit) format. </summary> 
#define EL_FORMAT_SHORT		 2

/// \def EL_FORMAT_SSHORT
/// \ingroup pixformat
/// <summary> Signed Short int (16 bit) format. </summary> 
#define EL_FORMAT_SSHORT     3

/// \def EL_FORMAT_INT
/// \ingroup pixformat
/// <summary> Signed int (32 bit) format. </summary> 
#define EL_FORMAT_INT		 4

/// \def EL_FORMAT_FLOAT
/// \ingroup pixformat
/// <summary> Float (32 bit) format. </summary> 
#define EL_FORMAT_FLOAT		 5

/// \def EL_FORMAT_DOUBLE
/// \ingroup pixformat
/// <summary> Float (64 bit) format. </summary> 
#define EL_FORMAT_DOUBLE	 6

/// \def EL_FORMAT_HALF_FLOAT
/// \ingroup pixformat
/// <summary> Half float (16 bit) format. </summary> 
#define EL_FORMAT_HALF_FLOAT 7  
///@}

////////////////////////////////////////////////////////////////////////
// Pixel types
//
/// @name Pixel Format Constants
///@{ 
/// \def PIX_FORMAT_NONE
/// \ingroup pixformat
/// <summary> Unspecified pixel format </summary>
#define PIX_FORMAT_NONE		(0 << VT_IMG_PIXFRMT_SHIFT)
/// \def PIX_FORMAT_LUMA
/// \ingroup pixformat
/// <summary> 1-band luma pixel </summary>
#define PIX_FORMAT_LUMA	    (1 << VT_IMG_PIXFRMT_SHIFT)
/// \def PIX_FORMAT_UV
/// \ingroup pixformat
/// <summary> 2-banded packed UV pixels </summary>
#define PIX_FORMAT_UV 		(2 << VT_IMG_PIXFRMT_SHIFT)
/// \def PIX_FORMAT_RGB
/// \ingroup pixformat
/// <summary> 3-band RGB pixel </summary>
#define PIX_FORMAT_RGB 		(3 << VT_IMG_PIXFRMT_SHIFT)
/// \def PIX_FORMAT_RGBA
/// \ingroup pixformat
/// <summary> 4-band RGBA pixel </summary>
#define PIX_FORMAT_RGBA		(4 << VT_IMG_PIXFRMT_SHIFT)
/// \def PIX_FORMAT_YUV
/// \ingroup pixformat
/// <summary> 3-band YUV pixel </summary>
#define PIX_FORMAT_YUV		(5 << VT_IMG_PIXFRMT_SHIFT)
///@}
#define PIX_FORMAT_FLOW		(6 << VT_IMG_PIXFRMT_SHIFT)
#define PIX_FORMAT_COMPLEX	(7 << VT_IMG_PIXFRMT_SHIFT)

////////////////////////////////////////////////////////////////////////
// Image types
//
/// @name Composite Format Constants
///@{ 
/// \def OBJ_UNDEFINED 
/// \ingroup pixformat
/// <summary> Undefined type. An uninititialized CImg is of this type. </summary>
#define OBJ_UNDEFINED VT_IMG_PIXFRMT_MASK
/// \def OBJ_BYTEIMG
/// \ingroup pixformat
/// <summary> Byte elements, undefined pixel format </summary> 
#define OBJ_BYTEIMG	  VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_NONE, EL_FORMAT_BYTE, 1)
/// \def OBJ_FLOATIMG
/// \ingroup pixformat
/// <summary> Float elements, undefined pixel format </summary> 
#define OBJ_FLOATIMG  VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_NONE, EL_FORMAT_FLOAT, 1)
/// \def OBJ_SHORTIMG
/// \ingroup pixformat
/// <summary> Short int elements, undefined pixel format </summary> 
#define OBJ_SHORTIMG  VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_NONE, EL_FORMAT_SHORT, 1)
/// \def OBJ_HALFFLOATIMG
/// \ingroup pixformat
/// <summary> Half float elements, undefined pixel </summary> 
#define OBJ_HALFFLOATIMG VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_NONE, EL_FORMAT_HALF_FLOAT, 1)

/// \def OBJ_LUMAIMG
/// \ingroup pixformat
/// <summary> Byte elements, gray pixels </summary> 
#define OBJ_LUMAIMG		  VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_LUMA, EL_FORMAT_BYTE, 1)
/// \def OBJ_LUMAFLOATIMG
/// \ingroup pixformat
/// <summary> Float elements, gray pixels </summary> 
#define OBJ_LUMAFLOATIMG  VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_LUMA, EL_FORMAT_FLOAT, 1)
/// \def OBJ_LUMASHORTIMG
/// \ingroup pixformat
/// <summary> Short int elements, gray pixels </summary> 
#define OBJ_LUMASHORTIMG  VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_LUMA, EL_FORMAT_SHORT, 1)
/// \def OBJ_LUMAHALFFLOATIMG
/// \ingroup pixformat
/// <summary> Half float elements, gray pixels </summary> 
#define OBJ_LUMAHALFFLOATIMG VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_LUMA, EL_FORMAT_HALF_FLOAT, 1)

/// \def OBJ_UVIMG
/// \ingroup pixformat
/// <summary> Byte elements, UV pixels </summary> 
#define OBJ_UVIMG		VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_UV, EL_FORMAT_BYTE, 2)
/// \def OBJ_UVFLOATIMG
/// \ingroup pixformat
/// <summary> Float elements, UV pixels </summary> 
#define OBJ_UVFLOATIMG	VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_UV, EL_FORMAT_FLOAT, 2)
/// \def OBJ_UVSHORTIMG
/// \ingroup pixformat
/// <summary> Short int elements, UV pixels </summary> 
#define OBJ_UVSHORTIMG	VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_UV, EL_FORMAT_SHORT, 2)
/// \def OBJ_UVHALFFLOATIMG
/// \ingroup pixformat
/// <summary> Half float elements, UV pixels </summary> 
#define OBJ_UVHALFFLOATIMG	VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_UV, EL_FORMAT_HALF_FLOAT, 2)

/// \def OBJ_RGBIMG
/// \ingroup pixformat
/// <summary> Byte elements, RGB pixels </summary>
#define OBJ_RGBIMG		 VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_RGB, EL_FORMAT_BYTE, 3)
/// \def OBJ_RGBFLOATIMG
/// \ingroup pixformat
/// <summary> Float elements, RGB pixels </summary>
#define OBJ_RGBFLOATIMG	 VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_RGB, EL_FORMAT_FLOAT, 3)
/// \def OBJ_RGBSHORTIMG
/// \ingroup pixformat
/// <summary> Short int elements, RGB pixels </summary>
#define OBJ_RGBSHORTIMG	 VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_RGB, EL_FORMAT_SHORT, 3)
/// \def OBJ_RGBHALFFLOATIMG
/// \ingroup pixformat
/// <summary> Half float elements, RGB pixels </summary>
#define OBJ_RGBHALFFLOATIMG	 VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_RGB, EL_FORMAT_HALF_FLOAT, 3)

/// \def OBJ_RGBAIMG
/// \ingroup pixformat
/// <summary> Byte elements, RGBA pixels </summary>
#define OBJ_RGBAIMG		  VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_RGBA, EL_FORMAT_BYTE, 4)
/// \def OBJ_RGBAFLOATIMG
/// \ingroup pixformat
/// <summary> Float elements, RGBA pixels </summary>
#define OBJ_RGBAFLOATIMG  VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_RGBA, EL_FORMAT_FLOAT, 4)
/// \def OBJ_RGBASHORTIMG
/// \ingroup pixformat
/// <summary> Short int elements, RGBA pixels </summary>
#define OBJ_RGBASHORTIMG  VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_RGBA, EL_FORMAT_SHORT, 4)
/// \def OBJ_RGBAHALFFLOATIMG
/// \ingroup pixformat
/// <summary> Half float elements, RGBA pixels </summary>
#define OBJ_RGBAHALFFLOATIMG  VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_RGBA, EL_FORMAT_HALF_FLOAT, 4)
//@}

// undocumented image types
#define OBJ_INTIMG	   VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_NONE, EL_FORMAT_INT, 1)
#define OBJ_FLOWIMG	   VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_FLOW, EL_FORMAT_FLOAT, 2)
#define OBJ_COMPLEXIMG VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_COMPLEX, EL_FORMAT_FLOAT, 2)

// Traits for the different element types
template <class T> struct ElTraits;

template<> struct ElTraits<int8_t> 
{
	typedef Byte BLOB_T;
	 
	static int    ElFormat()   { return EL_FORMAT_SBYTE; }
    static int8_t   MaxVal()     { return 127; }
    static BLOB_T MaxValBlob() { return 0x7f; }
    // Max and min values supported by the type
    static int8_t   TypeMax()    { return 127; }
    static int8_t   TypeMin()    { return -128; }
};
template<> struct ElTraits<Byte> 
{
	typedef Byte BLOB_T;
	 
    static int    ElFormat()   { return EL_FORMAT_BYTE; }
    static Byte   MaxVal()     { return 255; }
    static BLOB_T MaxValBlob() { return 0xff; }
    // Max and min values supported by the type
    static Byte   TypeMax()    { return 255; }
    static Byte   TypeMin()    { return 0; }
};
template<> struct ElTraits<int16_t> 
{
	typedef UInt16 BLOB_T;

	static int     ElFormat()   { return EL_FORMAT_SSHORT; };
    static int16_t  MaxVal()     { return 32767; }
	static BLOB_T  MaxValBlob() { return 0x7fff; }
	static int16_t  TypeMax()     { return 32767; }
	static int16_t  TypeMin()     { return -32768; }
};
template<> struct ElTraits<UInt16> 
{
	typedef UInt16 BLOB_T;

	static int     ElFormat()   { return EL_FORMAT_SHORT; };
    static UInt16  MaxVal()     { return 65535; }
	static BLOB_T  MaxValBlob() { return 0xffff; }
	static UInt16  TypeMax()     { return 65535; }
	static UInt16  TypeMin()     { return 0; }
};
template<> struct ElTraits<int> 
{
	typedef unsigned int BLOB_T;

    static int    ElFormat()   { return EL_FORMAT_INT; }
    static int    MaxVal()     { return 0x7fffffff; }
    static BLOB_T MaxValBlob() { return 0x7fffffff; }
	static int    TypeMax()    { return 2147483647; }
	static int    TypeMin()    { return -2147483647-1; }
};
template<> struct ElTraits<float> 
{
	typedef unsigned int BLOB_T;

	static int    ElFormat()   { return EL_FORMAT_FLOAT; }
    static float  MaxVal()     { return 1.f; }
    static BLOB_T MaxValBlob() { return 0x3f800000; }
	static float  TypeMax() { return 3.402823466e+38f; } 
	static float  TypeMin() { return -3.402823466e+38f; }
};
template<> struct ElTraits<double> 
{
	typedef uint64_t BLOB_T;

	static int    ElFormat()   { return EL_FORMAT_DOUBLE; }
    static float  MaxVal()     { return 1.f; }
    //static BLOB_T MaxValBlob() // TODO: implement the following
	//static float  TypeMax() { return 3.402823466e+38f; } 
	//static float  TypeMin() { return -3.402823466e+38f; }
};
template<> struct ElTraits<HALF_FLOAT> 
{
	typedef unsigned short BLOB_T;

	static int    ElFormat()   { return EL_FORMAT_HALF_FLOAT; }
    static float  MaxVal()     { return 1.f; }
    static BLOB_T MaxValBlob() { return 0x3c00; }
    
	// bit 15 for sign, bits 14-10 for exponent, 
	// and bits 9-0 for mantissa.  Max exponent is 0xf, 
	// max mantissa is 0x3ff
    static HALF_FLOAT TypeMax() { HALF_FLOAT v; v.v=0x3fff; return v; }
    static HALF_FLOAT TypeMin() { HALF_FLOAT v; v.v=0x8000|0x3fff; return v; }
};

// Test for opaque alpha values for the element types

template <typename T>
bool IsOpaqueAlpha(T v)
{ return v == ElTraits<T>::MaxVal(); }

// for float opaque check have a tolerance which is within 1/2 a gray level of
// a 16 bit image - note as with all float comparisons this one is subject to
// float compare epsilons.  If a value is very close to the threshold being
// tested below, this test may fire in a non-deterministic way.
template <>
inline bool IsOpaqueAlpha(float v)
{
	return v > (ElTraits<float>::MaxVal() - 
				ElTraits<float>::MaxVal() / float(2*ElTraits<UInt16>::MaxVal()));
}

// do half-float comparisons by examining the which bits are set in the 
// half-float representation 
template <>
inline bool IsOpaqueAlpha(HALF_FLOAT v)
{ return (v.v & 0x8000) == 0 && v.v >= 0x3bff; }

// Pixel classes for composite pixel types

#define DEFINE_COMPOSITE_TYPE_BAND_ACCESSORS(CompositeType, FirstBandAddress) \
	const ELT* Ptr() const { return (FirstBandAddress); } \
	ELT* Ptr() { return const_cast<ELT*>(static_cast<const CompositeType&>(*this).Ptr()); } \
	const ELT& operator[](int i) const { VT_ASSERT(i>=0 && i<Bands()); return (FirstBandAddress)[i]; } \
	ELT& operator[](int i) { return const_cast<ELT&>(static_cast<const CompositeType&>(*this)[i]); }
	
// LumaPix
template <class TBase>
class LumaType
{
public:    
	typedef TBase ELT;
	static int PixFormat() { return PIX_FORMAT_LUMA; } 
    static int Bands()     { return 1; }
	
	DEFINE_COMPOSITE_TYPE_BAND_ACCESSORS(LumaType, &v)

    ELT v;
    LumaType() {}
    LumaType(ELT vi) : v(vi) {}
};

// UVPix
template <class TBase>
class UVType
{
public:
    typedef TBase ELT;
	static int PixFormat() { return PIX_FORMAT_UV; } 
    static int Bands()     { return 2; }

	DEFINE_COMPOSITE_TYPE_BAND_ACCESSORS(UVType, &u);

    ELT u, v;
    UVType() {}
    UVType(ELT ui, ELT vi) : u(ui), v(vi) {}
};

// RGBPix
template <class TBase>
class RGBType
{
public:
    typedef TBase ELT;
	static int PixFormat() { return PIX_FORMAT_RGB; } 
    static int Bands()     { return 3; }
	
	DEFINE_COMPOSITE_TYPE_BAND_ACCESSORS(RGBType, &b)

    ELT b, g, r;
    RGBType() {}
	RGBType(ELT rr, ELT gg, ELT bb) { r = rr; g = gg; b = bb; }
};

template <class TBase>
class RGBAType
{
public:
    typedef TBase ELT;
	static int PixFormat() { return PIX_FORMAT_RGBA; } 
    static int Bands()     { return 4; }

	DEFINE_COMPOSITE_TYPE_BAND_ACCESSORS(RGBAType, &b)

    ELT b, g, r, a;
    RGBAType() {}
	RGBAType(ELT rr, ELT gg, ELT bb, ELT aa = ElTraits<ELT>::MaxVal()) 
    { r = rr; g = gg; b = bb; a = aa;}

    bool IsOpaque() const
    { return IsOpaqueAlpha(a); }
};

#undef DEFINE_COMPOSITE_TYPE_BAND_ACCESSORS

/// @name Composite Format Typedefs
///@{

/// \ingroup pixformat
/// <summary> Luma byte pixel </summary>
typedef LumaType<Byte>    LumaPix;
/// <summary> Luma short pixel </summary>
typedef LumaType<UInt16>  LumaShortPix;
/// <summary> Luma float pixel </summary>
typedef LumaType<float>   LumaFloatPix;
/// <summary> Luma half float pixel </summary>
typedef LumaType<HALF_FLOAT>  LumaHalfFloatPix;

/// <summary> RGB byte pixel </summary>
typedef RGBType<Byte>    RGBPix;
/// <summary> RGB short pixel </summary>
typedef RGBType<UInt16>  RGBShortPix;
/// <summary> RGBA signed short pixel </summary>
typedef RGBAType<Int16> RGBASShortPix;
/// <summary> RGB float pixel </summary>
typedef RGBType<float>   RGBFloatPix;
/// <summary> RGB half float pixel </summary>
typedef RGBType<HALF_FLOAT>   RGBHalfFloatPix;

/// <summary> RGBA byte pixel </summary>
typedef RGBAType<Byte>   RGBAPix;
/// <summary> RGBA short pixel </summary>
typedef RGBAType<UInt16> RGBAShortPix;
/// <summary> RGBA float  pixel </summary>
typedef RGBAType<float>  RGBAFloatPix;
/// <summary> RGBA half float pixel </summary>
typedef RGBAType<HALF_FLOAT>  RGBAHalfFloatPix;

/// <summary> UV byte pixel </summary>
typedef UVType<Byte>   UVPix;
/// <summary> UV short pixel </summary>
typedef UVType<UInt16> UVShortPix;
/// <summary> UV float  pixel </summary>
typedef UVType<float>  UVFloatPix;
/// <summary> UV half float pixel </summary>
typedef UVType<HALF_FLOAT>  UVHalfFloatPix;

///@}

// Compile time tests for standard element and pixel types
template <typename T>
struct IsStandardElementType : std::false_type {};

#define DECLARE_STANDARD_ELEMENT_TYPE(etype)\
template <> struct IsStandardElementType<etype> : std::true_type {};

DECLARE_STANDARD_ELEMENT_TYPE(Byte);
DECLARE_STANDARD_ELEMENT_TYPE(UInt16);
DECLARE_STANDARD_ELEMENT_TYPE(HALF_FLOAT);
DECLARE_STANDARD_ELEMENT_TYPE(float);
#undef DECLARE_STANDARD_ELEMENT_TYPE

template <typename PT, typename ET = void>
struct IsStandardPixelType : std::false_type {};

#define DECLARE_STANDARD_PIXEL_TYPE(ptype)\
template <typename ET> struct IsStandardPixelType<ptype<ET>> : \
	std::conditional<IsStandardElementType<ET>::value, \
	std::true_type, std::false_type>::type {};

DECLARE_STANDARD_PIXEL_TYPE(LumaType);
DECLARE_STANDARD_PIXEL_TYPE(RGBType);
DECLARE_STANDARD_PIXEL_TYPE(RGBAType);
DECLARE_STANDARD_PIXEL_TYPE(UVType);
#undef DECLARE_STANDARD_PIXEL_TYPE

// Forward declarations
class CParams;

/// <summary> Holds header information of an image (dimensions, type, etc.) 
/// </summary>
class CImgInfo
{
public:
	/// <summary> Constructor </summary>
	CImgInfo() : type(OBJ_UNDEFINED), width(0), height(0)
    {}

	/// <summary> Constructor </summary>
	CImgInfo(int iW, int iH, int iType) : type(iType), width(iW), height(iH)
    {}

	/// <summary> Return image SIZE (width, height). </summary>
	SIZE Size() const { SIZE s = { width, height }; return s;}

	/// <summary> Return image RECT (0, 0, width, height). </summary>
	RECT Rect() const { RECT r = { 0, 0, width, height }; return r; }

	/// <summary> Return band count of the image (this is bits 11-3 of the type).
	/// </summary>
	int  Bands() const { return VT_IMG_BANDS(type); }

	/// <summary> Return element size of the image. </summary>
	int  ElSize() const { return VT_IMG_ELSIZE(type); }

	/// <summary> Return pixel size of the image. </summary>
	int  PixSize() const { return Bands()*ElSize(); }

public:
	/// <summary> Image type, see \link pixformat pixel formats \endlink </summary>
    int type;
	/// <summary> Image width </summary>
    int width;
	/// <summary> Image height </summary>
    int height;
};

#define VT_NON_CONST_PTR_ACCESS_IMG(AccessFunc, ConstAccessFuncCall)\
	Byte* AccessFunc { return const_cast<Byte*>( \
	static_cast<const CImg&>(*this).ConstAccessFuncCall); }

/// <summary> The basic image class. Base class for the CTypedImage and 
/// CCompositImage templates.</summary>
class CImg
{
public:
	/// <summary> Constructor </summary> 
	CImg(); 

	/// <summary> Destructor </summary> 
	virtual ~CImg();
	
#ifdef VT_IMG_COPY_ASSIGNABLE
public:
	CImg(const CImg& other) 
		: m_iStrideBytes(0), m_pbPtr(NULL), m_pMem(NULL), m_pMetaData(NULL)
	{ 
		other.Share(*this); 
	}
	
	CImg &operator=(const CImg& other) 
	{ 
		other.Share(*this); 
		return *this; 
	}
#else
private:
	// Copying is verboten	
	/// <summary> Copy constructor. See remarks below. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- CImg is not copyable by default
	///		- It is recommended to use 
	/// Share() and CopyTo() for shallow and deep copying,
	/// respectively. 
	///		- To enable the copy constructor, define 
	/// \code #define VT_IMG_COPY_ASSIGNABLE \endcode
	/// before including the VisionTools headers. This also enables assignment.
	//		- If enabled, the copy constructor calls Share() to make a shallow 
	// copy.
	CImg(const CImg&);

	/// <summary> Assignment operator. See remarks below. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- CImg is not assignable by default. 
	///		- It is recommended to use
	/// Share() and CopyTo() for shallow and deep copying,
	/// respectively. 
	///		- To enable assignment, define 
	/// \code #define VT_IMG_COPY_ASSIGNABLE \endcode
	/// before including the VisionTools headers. This also enables copying.
	//		- If enabled, the assignment operator calls Share() to make a shallow 
	// copy.
	CImg &operator=(const CImg&);
#endif

public:
	/// <summary> Initialize image, allocate pixel memory.  </summary>
	/// <returns> 
	///		- S_OK on success
	///		- E_INVALIDARG if requested properties are invalid
	///		- E_OUTOFMEMORY if memory allocation failed 
	/// </returns>
	HRESULT Create(int iW, int iH, int iType, AlignMode eAlign = DEFAULT_ALIGN)
	{ return CreateInternal(iW, iH, iType, eAlign, false); }

	/// <summary> Initialize image, allocate pixel memory.  </summary>
	/// <returns> 
	///		- S_OK on success
	///		- E_INVALIDARG if requested properties are invalid
	///		- E_OUTOFMEMORY if memory allocation failed 
	/// </returns>
	HRESULT Create(const CImgInfo& info, AlignMode eAlign = DEFAULT_ALIGN)
    { return Create(info.width, info.height, info.type, eAlign); }

	/// <summary> Initialize image, wrap existing pixel buffer.  </summary>
	/// <returns> 
	///		- S_OK on success
	///		- E_INVALIDARG if requested properties are invalid or if pbBuffer
	///	is NULL.
	/// </returns>
	/// <DL><DT> Remarks: </DT></DL>
	///		- No memory is allocated
	///		- Note that the pixel buffer is not const, since it may be modified 
	///	later via this CImg.
	///		- The wrapped memory resource is managed by the caller. Use Share() 
	///	judiciously with images created with this function.
	HRESULT Create(Byte *pbBuffer, int iW, int iH, int iStrideBytes, int iType);

	/// <summary> Invalidate image and release memory.  </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Decreases the pixel memory reference count. 
	///		- Deallocates pixel memory if no other references exist.
	void Deallocate();
	
	/// @name Pixel Memory access
	///@{
	
	/// <summary> Pixel memory access, top left </summary>
	const Byte *BytePtr() const { return m_pbPtr; }
	/// <summary> Pixel memory access, top left </summary>
	VT_NON_CONST_PTR_ACCESS_IMG(BytePtr(), BytePtr());
	
	/// <summary> Pixel memory access, first pixel in row iY </summary>
	const Byte *BytePtr(int iY) const 
    { 
        VT_ASSERT_BOUNDS( iY >= 0 && iY < Height() );
		// Added unsigned int to avoid 64 bit multiply.
		return m_pbPtr + iY * (UInt32)StrideBytes(); 
	} 
	/// <summary> Pixel memory access, first pixel in row iY </summary>
	VT_NON_CONST_PTR_ACCESS_IMG(BytePtr(int iY), BytePtr(iY));

	/// <summary> Pixel memory access, pixel at iX, iY </summary>
	const Byte *BytePtr(int iX, int iY) const 
    {
        VT_ASSERT_BOUNDS( iX >= 0 && iX < Width() );
		return BytePtr(iY) + iX * (UInt32)PixSize();
	}
	/// <summary> Pixel memory access, pixel at iX, iY </summary>
	VT_NON_CONST_PTR_ACCESS_IMG(BytePtr(int iX, int iY), BytePtr(iX, iY));

	/// <summary> Pixel memory access, pixel at iX, iY, band iBand </summary>
	const Byte *BytePtr(int iX, int iY, int iBand) const 
    { 
        VT_ASSERT_BOUNDS( iBand >= 0 && iBand < Bands() );
		return BytePtr(iX, iY) + iBand * (UInt32)ElSize(); 
	}
	/// <summary> Pixel memory access, pixel at iX, iY, band iBand </summary>
	VT_NON_CONST_PTR_ACCESS_IMG(BytePtr(int iX, int iY, int iBand), 
		BytePtr(iX, iY, iBand));

	/// <summary> Pixel memory access, pixel at pt </summary>
	const Byte *BytePtr(const POINT& pt) const { return BytePtr(pt.x, pt.y); }
	/// <summary> Pixel memory access, pixel at pt </summary>
	VT_NON_CONST_PTR_ACCESS_IMG(BytePtr(POINT pt), BytePtr(pt));

	/// <summary> Pixel memory access, pixel at pt, band iBand </summary>
	const Byte *BytePtr(const POINT& pt, int iBand) const 
	{ return BytePtr(pt.x, pt.y, iBand); }
	/// <summary> Pixel memory access, pixel at pt, band iBand </summary>
	VT_NON_CONST_PTR_ACCESS_IMG(BytePtr(POINT pt, int iBand), 
		BytePtr(pt, iBand));

	/// <summary> Pixel memory access, first pixel in row iY </summary>
	const Byte *operator[](int iY) const { return BytePtr(iY); }
	/// <summary> Pixel memory access, first pixel in row iY </summary>
	VT_NON_CONST_PTR_ACCESS_IMG(operator[](int iY), operator[](iY));
	
	/// <summary> Return stride length in bytes. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- The stride length is the memory offset between two image rows.
	///		- StrideBytes is at least PixSize() * Width(), but can be larger
	///	due to memory alignment
	int StrideBytes() const { return m_iStrideBytes; }	
	///@}
	
	/// @name Row Pointer Arithmetic 
	///@{

	/// <summary> Return pointer to pixel one row below </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes (pRow + StrideBytes()	
	const Byte *NextRow(const Byte *pRow) const 
	{ return pRow + StrideBytes(); }
	/// <summary> Return pointer to pixel one row below </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes (pRow + StrideBytes()	
	VT_NON_CONST_PTR_ACCESS_IMG(NextRow(Byte* pRow), NextRow(pRow));

	/// <summary> Return pointer to pixel one row above </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) - StrideBytes()	
	const Byte *PrevRow(const Byte *pRow) const 
	{ return pRow - StrideBytes(); }
	/// <summary> Return pointer to pixel one row above </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) - StrideBytes()	
	VT_NON_CONST_PTR_ACCESS_IMG(PrevRow(Byte* pRow), PrevRow(pRow));

	/// <summary> Return pointer to pixel N rows away </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) + numOffsetRows * StrideBytes()	
	const Byte *OffsetRow(const Byte *pRow, int numOffsetRows) const 
	{ return pRow + numOffsetRows * StrideBytes(); }
	/// <summary> Return pointer to pixel N rows away </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) + numOffsetRows * StrideBytes()	
	VT_NON_CONST_PTR_ACCESS_IMG(OffsetRow(Byte* pRow, int numOffsetRows), 
		OffsetRow(pRow, numOffsetRows));

	///@}

	/// @name Image properties
	///@{

	/// <summary> Return image type. </summary>
	int GetType() const { return IMG_FORMAT(m_info.type); }
	/// <summary> Return full un-masked image type. </summary>
	int GetFullType() const { return m_info.type; }
	/// <summary> Return image width in pixels. </summary>
	int Width() const { return m_info.width; }
	/// <summary> Return image heigth in pixels. </summary>
	int Height() const { return m_info.height; }
	/// <summary> Return number of bands. </summary>
	int Bands() const { return m_info.Bands(); }
	/// <summary> Return image SIZE (width, height). </summary>
	SIZE Size() const { SIZE s = { Width(), Height() }; return s;}
	/// <summary> Return image RECT (0, 0, width, height). </summary>
	RECT Rect() const { RECT r = { 0, 0, Width(), Height() }; return r; }
	/// <summary> Return element size in bytes, e.g. 4 for an RGB float image. </summary>	
	int ElSize() const  { return m_info.ElSize(); }
	/// <summary> Return pixel size in bytes, e.g. 12 for an RGB float image. </summary>	
	int PixSize() const { return m_info.PixSize(); }
	/// <summary> Return image info for this image. </summary>	
	CImgInfo GetImgInfo() const { return m_info; }
	///@}

	// undocumented: return rectangle prct clipped against image rectangle
	RECT ClipRect (const RECT *prct) const;
		
	// Memory
	/// <summary> Return if image is valid. </summary>
	///	<returns> 
	///		- true, if the image has been created successfully
	/// </returns>
	bool IsValid() const { return m_pbPtr!=NULL; }
	
	/// <summary> Return if image wraps memory. </summary>
	///	<returns> 
	///		- true, if the image was created using the wrap version of 
	///	Create()
	/// </returns>
	bool IsWrappingMemory() const 
    { return IsValid() && m_pMem==NULL; }
	
	// undocumented
	bool IsSSEAligned() const 
    { return IsValid() ? (IsAligned16(m_pbPtr) && (StrideBytes()&0xf)==0) 
	: false; }
	
	/// <summary> Make a deep copy of the image. </summary>
	/// <param name="cDest"> Destination image </param>
	/// <param name="prctSrc"> Sourc image region to copy. Optional. If NULL, 
	/// the entire image is copied. </param>
	///	<returns> 
	///		- S_OK on success
	///		- E_NOINIT if this image is invalid
	///		- E_INVALIDARG or E_OUTOFMEMORY if cDest cannot be created
	/// </returns>
	HRESULT CopyTo(CImg &cDest, const RECT *prctSrc = NULL) const;	

	/// <summary> Make a shallow copy of the image. </summary>
	/// <param name="cDest"> Destination image </param> 
	/// <param name="prctSrc"> Source image region to share. Optional. 
	/// If not NULL, it must not be empty. </param>
	/// <param name="bShareMetaData"> Set to true if Meta data should be shared. 
	/// Option. Default value: false. </param>
	///	<returns> 
	///		- S_OK on success
	///		- E_NOINIT if this image is invalid
	///		- E_INVALIDARG if cDest has an incompatible type
	///		- E_INVALIDARG if &cDest == *this or prctSrc is empty
	/// </returns>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Share() is an exception in terms of const correctness. It breaks 
	/// logical constness of this CImg to share data with a 
	/// non-const cDest image. If this was not allowed, too much existing code
	/// would break.
	HRESULT Share(CImg &cDest, const RECT *prctSrc = NULL, 
		bool bShareMetaData = false) const;
	
	/// <summary> Determine if this image is sharing memory with otherImg. </summary>	
	/// <param name="otherImg"> Test image </param> 
	/// <DL><DT> Remarks: </DT></DL>
	///		- Returns if memory blocks overlap 
	bool IsSharingMemory(const CImg& otherImg) const;
	
	/// @name Block Access
	///@{

	/// <summary> Paste contents of another image into this image. </summary>	
	/// <param name="iDstX"> Destination x coordinate </param>
	/// <param name="iDstY"> Destination y coordinate </param>
	/// <param name="cSrc"> Source image </param>
	/// <param name="prctSrc"> Region within the source image to paste. 
	/// If NULL, the entire source image is pasted. </param>
	///	<returns> 
	///		- S_OK on success
	///		- E_NOINIT if image is invalid
	///		- E_INVALIDARG if cSrc is invalid
	/// </returns>
	/// <DL><DT> Remarks: </DT></DL>
	///		- prctSrc can be anything, even a region outside the source image. 
	/// Outside pixels are pasted as zero.
	HRESULT Paste(int iDstX, int iDstY, const CImg &cSrc, 
		const RECT *prctSrc = NULL);

	/// <summary> Fill image with constant value. </summary>	
	/// <param name="pbValue"> Byte pointer to fill value </param>
	/// <param name="prctDst"> Region to fill. Optional. If NULL, the entire image
	/// is filled. </param>
	/// <param name="iBand"> Band to fill. Optional. If -1, all bands are filled. 
	/// </param>
	/// <param name="bReplicateBands"> If true, ElSize() sized value at pbValue 
	/// will be replicated across all bands. If false, PixSize() sized value 
	/// pbValue will be used for each pixel. </param>
	///	<returns> 
	///		- S_OK on success
	///		- E_NOINIT if image is invalid
	/// </returns>
	HRESULT Fill(const Byte *pbValue, const RECT *prctDst = NULL, int iBand = -1, 
                 bool bReplicateBands = false);
	
	/// <summary> Set pixel memory to zero. </summary>	
	/// <param name="prctDst"> Region to set to zero. Optional. If NULL, 
	/// the entire image is cleared. </param>
	///	<returns> 
	///		- S_OK on success
	///		- E_NOINIT if image is invalid
	/// </returns>
	HRESULT Clear(const RECT *prctDst = NULL);

	/// @name Load and Save
	///@{ 

	/// <summary> Load image in VisionTools format </summary>	
	/// <param name="pszFile"> Filename. The recommended extension for 
	/// VisionTools images is ".vti". </param>
	///	<returns> 
	///		- S_OK on success
	///		- E_INVALIDARG if the file cannot be opened
	///		- E_FAIL if there was an error reading from the file
	/// </returns>

	HRESULT Load(const WCHAR * pszFile); 
	/// <summary> Save image in VisionTools format. </summary>	
	/// <param name="pszFile"> Filename. The recommended extension for 
	/// VisionTools images is ".vti". </param>
	///	<returns> 
	///		- S_OK on success
	///		- E_NOINIT if image is invalid
	/// </returns>
	HRESULT Save(const WCHAR * pszFile) const; 
	///@}

	/// @name Metadata
	///@{

	/// <summary> Get meta data. </summary>	
	///	<returns> Pointer to CParams object holding meta data. Can be NULL.
	/// </returns>
	const CParams *GetMetaData() const { return m_pMetaData; }

    /// <summary> Set (merge) meta data. </summary>	
	/// <param name="pParams"> Meta data. NOTE: this functions merges pParams 
	/// with the image's existing meta data. Set pParams to NULL to clear meta data. 
	/// </param>
	///	<returns> 
	///		- S_OK on success
	///		- E_OUTOFMEMORY if meta data allocation failed 
	/// </returns>
	HRESULT SetMetaData(const CParams *pParams); 

	///@}

	/// <summary> Test if image can be created with a specific type. </summary>	
	/// <DL><DT> Remarks: </DT></DL>
	///		- This is to determine CImgs allowable types at run-time 
	///		- Always returns true if called on a CImg object
	///		- If called on a CImg reference/pointer referring to a CTypedImg 
	/// or CCompositeImg, it returns true iff iType is 
	/// compatible with the compile-time (template) type.
	///		- Example: \code CImg().IsCreatableAs(OBJ_RGBIMG) \endcode evaluates
	/// to true
	///		- Example: \code CTypedImg<Byte>().IsCreatableAs(OBJ_SHORTIMG) 
	/// \endcode evaluates to false
	///		- Example: \code CCompositeImg<LumaShortPix>().IsCreatableAs(OBJ_FLOATIMG) 
	/// \endcode evaluates to false
	bool IsCreatableAs(int iType) const;

protected:
	void CheckInvariant(int iType) const;

	void SetType(int iType)  { m_info.type = iType; }
 
private:
	HRESULT CreateInternal(int iW, int iH, int iType, AlignMode eAlign, 
						   bool keepExistingMemoryIfImgSizeMatches);

private:
	CImgInfo m_info;
	
	Byte *m_pbPtr;
	int m_iStrideBytes;

	CMemShare *m_pMem;
	CParams* m_pMetaData;
};

#define VT_NON_CONST_PTR_ACCESS_TYPED_IMG(T, ConstAccessFunc, ConstAccessFuncCall)\
	T* ConstAccessFunc { return const_cast<T*>( \
	static_cast<const CTypedImg<T>&>(*this).ConstAccessFuncCall); }

/// <summary> Class template for images containing N bands of element type T.
/// </summary>
template <class T>
class CTypedImg : public CImg
{
public:
	/// <summary> Pixel type typedef </summary>
	typedef T PixelType;

public:
	/// <summary> Constructor. </summary>
	CTypedImg() 
	{ CImg::SetType(TypeIs(1)); } 

	/// <summary> Constructor. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- This constructor allocates memory. This operation can fail,
	/// see CImg::Create().
	///		- Images constructed with this version of the constructor must 
	/// call IsValid() to check whether construction succeeded.
	CTypedImg(int iW, int iH, int iBands = 1, AlignMode eAlign = DEFAULT_ALIGN) 
	{ CImg::SetType(TypeIs(1)); Create(iW, iH, iBands, eAlign); }

	/// <summary> Constructor. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- This constructor wraps memory. This operation can fail, see
	/// CImg::Create()
	///		- Images constructed with this version of the constructor must 
	/// call IsValid() to check whether construction succeeded.
	CTypedImg(T *pbBuffer, int iW, int iH, int iBands, int iStrideBytes)
	{ CImg::SetType(TypeIs(1)); Create(pbBuffer, iW, iH, iBands, iStrideBytes); }

	~CTypedImg()
	{ CImg::CheckInvariant(TypeIs(1)); }

#ifdef VT_IMG_COPY_ASSIGNABLE
public:
	CTypedImg(const CTypedImg& other)
	{ 
		CImg::SetType(TypeIs(other.Bands())); 
		other.Share(*this); 
	}
	
	CTypedImg& operator=(const CTypedImg& other) 
	{ 
		CImg::SetType(TypeIs(other.Bands())); 
		other.Share(*this);
		return *this; 
	}
#else
private:
	// Copying is verboten	
	/// <summary> Copy constructor. See remarks below. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- CTypedImg is not copyable by default
	///		- It is recommended to use 
	/// Share() and CopyTo() for shallow and deep copying,
	/// respectively. 
	///		- To enable the copy constructor, define 
	/// \code #define VT_IMG_COPY_ASSIGNABLE \endcode
	/// before including the VisionTools headers. This also enables assignment.
	//		- If enabled, the copy constructor calls Share() to make a shallow 
	// copy.
	CTypedImg(const CTypedImg&);

	/// <summary> Assignment operator. See remarks below. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- CTypedImg is not assignable by default. 
	///		- It is recommended to use
	/// Share() and CopyTo() for shallow and deep copying,
	/// respectively. 
	///		- To enable assignment, define 
	/// \code #define VT_IMG_COPY_ASSIGNABLE \endcode
	/// before including the VisionTools headers. This also enables copying.
	//		- If enabled, the assignment operator calls Share() to make a shallow 
	// copy.
	CTypedImg& operator=(const CTypedImg&);
#endif

public:
	/// <summary> Initialize image, allocate pixel memory. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- see CImg::Create().
	HRESULT Create(int iW, int iH, int iBands = 1, AlignMode eAlign = DEFAULT_ALIGN) 
	{
		if( iBands > VT_IMG_BANDS_MAX )
		{
			return E_INVALIDARG;
		}
		return CImg::Create(iW, iH, TypeIs(iBands), eAlign); 
    }

	/// <summary> Initialize image, wrap existing pixel buffer. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- see CImg::Create().
	HRESULT Create(T *pbBuffer, int iW, int iH, int iBands, int iStrideBytes) 
	{ 
		if( iBands > VT_IMG_BANDS_MAX )
		{
			return E_INVALIDARG;
		}
		return CImg::Create((Byte*)pbBuffer, iW, iH, iStrideBytes, TypeIs(iBands)); 
	}

	/// @name Typed Pixel Access (Pointer)
	///@{

	/// <summary> Pixel pointer access, top left </summary>
	const T *Ptr() const { return reinterpret_cast<const T*>(BytePtr()); }
	/// <summary> Pixel pointer access, top left </summary>
	VT_NON_CONST_PTR_ACCESS_TYPED_IMG(T, Ptr(), Ptr());
	
	/// <summary> Pixel pointer access, row iY </summary>
	const T *Ptr(int iY) const { return reinterpret_cast<const T*>(BytePtr(iY)); }
	/// <summary> Pixel pointer access, row iY </summary>
	VT_NON_CONST_PTR_ACCESS_TYPED_IMG(T, Ptr(int iY), Ptr(iY));	
	
	/// <summary> Pixel pointer access, row iY, column iX </summary>
	const T *Ptr(int iX, int iY) const 
	{ return reinterpret_cast<const T*>(BytePtr(iX, iY)); }
	/// <summary> Pixel pointer access, row iY, column iX </summary>
	VT_NON_CONST_PTR_ACCESS_TYPED_IMG(T, Ptr(int iX, int iY), Ptr(iX, iY));		
	
	/// <summary> Pixel pointer access, row iY, column iX, band iBand </summary>
	const T *Ptr(int iX, int iY, int iBand) const 
	{ return reinterpret_cast<const T*>(BytePtr(iX, iY, iBand)); }
	/// <summary> Pixel pointer access, row iY, column iX, band iBand </summary>
	VT_NON_CONST_PTR_ACCESS_TYPED_IMG(T, Ptr(int iX, int iY, int iBands), 
		Ptr(iX, iY, iBands));
	
	/// <summary> Pixel pointer access at point pt</summary>
	const T *Ptr(const POINT& pt) const 
	{ return reinterpret_cast<const T*>(BytePtr(pt)); }
	/// <summary> Pixel pointer access at point pt</summary>
	VT_NON_CONST_PTR_ACCESS_TYPED_IMG(T, Ptr(const POINT& pt), Ptr(pt));

	/// <summary> Pixel pointer access at point pt, band iBand</summary>
	const T *Ptr(const POINT& pt, int iBand) const 
	{ return reinterpret_cast<const T*>(BytePtr(pt, iBand)); }
	/// <summary> Pixel pointer access at point pt, band iBand</summary>
	VT_NON_CONST_PTR_ACCESS_TYPED_IMG(T, Ptr(const POINT& pt, int iBand), 
		Ptr(pt, iBand));
	
	// operator[]
	/// <summary> Pixel pointer access, row iY </summary>
	const T *operator[](int iY) const { return Ptr(iY); }
	/// <summary> Pixel pointer access, row iY </summary>
	VT_NON_CONST_PTR_ACCESS_TYPED_IMG(T, operator[](int iY), operator[](iY));		
	///@}

	/// @name Typed Pixel Access (Reference)
	///@{

	/// <summary> Pixel reference access, row iY, column iX </summary>
	const T &Pix(int iX, int iY) const { return *Ptr(iX, iY); }
	/// <summary> Pixel reference access, row iY, column iX </summary>
	T &Pix(int iX, int iY) { return *Ptr(iX, iY); }
	
	/// <summary> Pixel reference access, row iY, column iX, band iBand </summary>
	const T &Pix(int iX, int iY, int iBand) const { return *Ptr(iX, iY, iBand); }
	/// <summary> Pixel reference access, row iY, column iX, band iBand </summary>
	T &Pix(int iX, int iY, int iBand) { return *Ptr(iX, iY, iBand); }

	/// <summary> Pixel reference access, point pt</summary>
	const T &Pix(POINT pt) const { return *Ptr(pt); }
	/// <summary> Pixel reference access, point pt </summary>
	T &Pix(POINT pt) { return *Ptr(pt); }

	/// <summary> Pixel reference access, point pt, band iBand </summary>
	const T &Pix(POINT pt, int iBand) const { return *Ptr(pt, iBand); }
	/// <summary> Pixel reference access, point pt, band iBand </summary>
	T &Pix(POINT pt, int iBand) { return *Ptr(pt, iBand); }
	///@}

	/// @name Typed Pixel Access (Reference, alternate syntax)
	///@{

	/// <summary> Pixel reference access, row iY, column iX </summary>
	const T &operator()(int iX, int iY) const { return Pix(iX, iY); }
	/// <summary> Pixel reference access, row iY, column iX </summary>
	T &operator()(int iX, int iY) { return Pix(iX, iY); }

	/// <summary> Pixel reference access, row iY, column iX, band iBand </summary>
	const T &operator()(int iX, int iY, int iBand) const 
	{ return Pix(iX, iY, iBand); }
	/// <summary> Pixel reference access, row iY, column iX, band iBand </summary>
	T &operator()(int iX, int iY, int iBand) 
	{ return Pix(iX, iY, iBand); }

	/// <summary> Pixel reference access, point pt </summary>
	const T &operator()(POINT pt) const { return Pix(pt); }
	/// <summary> Pixel reference access, point pt </summary>
	T &operator()(POINT pt) { return Pix(pt); }
	
	/// <summary> Pixel reference access, point pt, band iBand </summary>
	const T &operator()(POINT pt, int iBand) const { return Pix(pt, iBand); }
	/// <summary> Pixel reference access, point pt, band iBand </summary>
	T &operator()(POINT pt, int iBand) { return Pix(pt, iBand); }

	///@}

	/// @name Row Pointer Arithmetic 
	///@{
	
	/// <summary> Return pointer to pixel one row below </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) + StrideBytes()	
	const T *NextRow(const T *pRow) const 
	{ return reinterpret_cast<const T*>(
		CImg::NextRow(reinterpret_cast<const Byte*>(pRow))); }
	/// <summary> Return pointer to pixel one row below </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) + StrideBytes()	
	VT_NON_CONST_PTR_ACCESS_TYPED_IMG(T, NextRow(T* pRow), NextRow(pRow));

	/// <summary> Return pointer to pixel one row above </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) - StrideBytes()	
	const T *PrevRow(const T *pRow) const 
	{ return reinterpret_cast<const T*>(
		CImg::PrevRow(reinterpret_cast<const Byte*>(pRow))); }
	/// <summary> Return pointer to pixel one row above </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) - StrideBytes()	
	VT_NON_CONST_PTR_ACCESS_TYPED_IMG(T, PrevRow(T* pRow), PrevRow(pRow));

	/// <summary> Return pointer to pixel N rows away </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) + numOffsetRows * StrideBytes()	
	const T *OffsetRow(const T *pRow, int numOffsetRows) const 
	{ return reinterpret_cast<const T*>(
		CImg::OffsetRow(reinterpret_cast<const Byte*>(pRow), numOffsetRows)); }
	/// <summary> Return pointer to pixel N rows away </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) + numOffsetRows * StrideBytes()	
	VT_NON_CONST_PTR_ACCESS_TYPED_IMG(T, OffsetRow(T* pRow, int numOffsetRows), 
		OffsetRow(pRow, numOffsetRows));
	
	///@}

	/// @name Typed Fill
	///@{
	
	/// <summary> Fill image with constant value </summary>
	/// <param name="cValue"> Fill value of type T</param>
	/// <param name="prctDst"> Region to fill. Optional. If NULL, the entire image
	/// is filled. </param>
	/// <param name="iBand"> Band to fill. Optional. If -1, all bands are filled. 
	/// </param>
	///	<returns> 
	///		- S_OK on success
	///		- E_NOINIT if image is invalid
	/// </returns>
	HRESULT Fill(const T& cValue, const RECT *prctDst = NULL, int iBand = -1) 
	{ return CImg::Fill((Byte*)&cValue, prctDst, iBand, true); }

	///@}

public:
	static int TypeIs(int iBands)
	{
		VT_ASSERT(iBands <= VT_IMG_BANDS_MAX);
		return VT_IMG_FIXED_ELFRMT_MASK | 
			VT_IMG_MAKE_TYPE(ElTraits<T>::ElFormat(), iBands); 
    }
};

/// @name CTypedImg Typedefs
///@{
/// \ingroup pixformat
/// <summary> Byte image </summary>
typedef CTypedImg<Byte>       CByteImg;
/// <summary> Short image </summary>
typedef CTypedImg<UInt16>     CShortImg;
/// <summary> Float image </summary>
typedef CTypedImg<float>      CFloatImg;
/// <summary> Half float image </summary>
typedef CTypedImg<HALF_FLOAT> CHalfFloatImg;
///@}

typedef CTypedImg<int>        CIntImg;

#define VT_NON_CONST_PTR_ACCESS_COMP_IMG(U, ConstAccessFunc, ConstAccessFuncCall)\
	U* ConstAccessFunc { return const_cast<U*>( \
	static_cast<const CCompositeImg<T,TP>&>(*this).ConstAccessFuncCall); }

/// <summary> Class template for composite images. All bands are 
/// accessed together as one pixel of type T. E.g. the 4 bands of the CByteImg become 
/// RGBAPix structs in the CRGBAImg type. </summary>
template <class T, class TP=T>
class CCompositeImg : public CTypedImg<typename T::ELT>
{
public:
	/// <summary> Base image type </summary>
	typedef CTypedImg<typename T::ELT> BaseImageType;
	
	/// <summary> Base image pixel type </summary>
	typedef typename T::ELT BaseImagePixelType;

	/// <summary> Pixel type typedef </summary>
	typedef T CompositePixelType;


public:
	/// <summary> Constructor. </summary>
	CCompositeImg() { CImg::SetType(TypeIs()); }

	/// <summary> Constructor. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- This constructor allocates memory. This operation can fail,
	/// see CImg::Create().
	///		- Images constructed with this version of the constructor must 
	/// call IsValid() to check whether construction succeeded.
	CCompositeImg(int iW, int iH, AlignMode eAlign = DEFAULT_ALIGN)
	{ CImg::SetType(TypeIs()); CImg::Create(iW, iH, TypeIs(), eAlign); }

	/// <summary> Constructor. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- This constructor wraps memory. This operation can fail, see
	/// CImg::Create()
	///		- Images constructed with this version of the constructor must 
	/// call IsValid() to check whether construction succeeded.
	CCompositeImg(typename T::ELT *pbBuffer, int iW, int iH, int iStrideBytes) 
	{ CImg::SetType(TypeIs()); CImg::Create((Byte*)pbBuffer, iW, iH, iStrideBytes, TypeIs()); }

	~CCompositeImg()
	{ CImg::CheckInvariant(TypeIs()); }

#ifdef VT_IMG_COPY_ASSIGNABLE
public:
	CCompositeImg(const CCompositeImg& other) 
	{ 
		CImg::SetType(TypeIs());
		other.Share(*this); 
	}
	
	CCompositeImg& operator=(const CCompositeImg& other) 
	{ 
		CImg::SetType(TypeIs());
		other.Share(*this); 
		return *this; 
	}
#else
private:
	// Copying is verboten	
	/// <summary> Copy constructor. See remarks below. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- CCompositeImg is not copyable by default
	///		- It is recommended to use 
	/// Share() and CopyTo() for shallow and deep copying,
	/// respectively. 
	///		- To enable the copy constructor, define 
	/// \code #define VT_IMG_COPY_ASSIGNABLE \endcode
	/// before including the VisionTools headers. This also enables assignment.
	//		- If enabled, the copy constructor calls Share() to make a shallow 
	// copy.
	CCompositeImg(const CCompositeImg&);

	/// <summary> Assignment operator. See remarks below. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- CCompositeImg is not assignable by default. 
	///		- It is recommended to use
	/// Share() and CopyTo() for shallow and deep copying,
	/// respectively. 
	///		- To enable assignment, define 
	/// \code #define VT_IMG_COPY_ASSIGNABLE \endcode
	/// before including the VisionTools headers. This also enables copying.
	//		- If enabled, the assignment operator calls Share() to make a shallow 
	// copy.
	CCompositeImg& operator=(const CCompositeImg&);
#endif

public:
	/// <summary> Initialize image, allocate pixel memory. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- see CImg::Create().
	HRESULT Create(int iW, int iH, AlignMode eAlign = DEFAULT_ALIGN)
	{ return CImg::Create(iW, iH, TypeIs(), eAlign); }

	/// <summary> Initialize image, wrap existing pixel buffer. </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- see CImg::Create().
	HRESULT Create(Byte *pbBuffer, int iW, int iH, int iStrideBytes)
	{ return CImg::Create(pbBuffer, iW, iH, iStrideBytes, TypeIs()); }

	/// @name Typed Pixel Access (Pointer)
	///@{
	
	const TP *Ptr() const { return reinterpret_cast<const TP*>(CImg::BytePtr()); }
	VT_NON_CONST_PTR_ACCESS_COMP_IMG(TP, Ptr(), Ptr());

	const TP *Ptr(int iY) const { return reinterpret_cast<const TP*>(CImg::BytePtr(iY)); }
	VT_NON_CONST_PTR_ACCESS_COMP_IMG(TP, Ptr(int iY), Ptr(iY));

	const TP *Ptr(int iX, int iY) const { return reinterpret_cast<const TP*>(CImg::BytePtr(iX, iY)); }
	VT_NON_CONST_PTR_ACCESS_COMP_IMG(TP, Ptr(int iX, int iY), Ptr(iX, iY));

	const BaseImagePixelType *Ptr(int iX, int iY, int iBand) const { return BaseImageType::Ptr(iX, iY, iBand); }
	VT_NON_CONST_PTR_ACCESS_COMP_IMG(BaseImagePixelType, Ptr(int iX, int iY, int iBand), Ptr(iX, iY, iBand));

	const TP *Ptr(const POINT& pt) const 
	{ return reinterpret_cast<const TP*>(CImg::BytePtr(pt)); }
	VT_NON_CONST_PTR_ACCESS_COMP_IMG(TP, Ptr(const POINT& pt), Ptr(pt));

	const BaseImagePixelType *Ptr(const POINT& pt, int iBand) const 
	{ return BaseImageType::Ptr(pt); }
	VT_NON_CONST_PTR_ACCESS_COMP_IMG(BaseImagePixelType, Ptr(const POINT& pt, int iBand), Ptr(pt, iBand));

	const TP *operator[](int iY) const { return Ptr(iY); }
	VT_NON_CONST_PTR_ACCESS_COMP_IMG(TP, operator[](int iY), operator[](iY));
	
	///@}

	/// @name Typed Pixel Access (Reference)
	///@{
	
	const TP &Pix(int iX, int iY) const { return *Ptr(iX, iY); }
	TP &Pix(int iX, int iY) { return *Ptr(iX, iY); }

	const BaseImagePixelType& Pix(int iX, int iY, int iBand) const { return *Ptr(iX, iY, iBand); }
	BaseImagePixelType& Pix(int iX, int iY, int iBand) { return *Ptr(iX, iY, iBand); }
	
	const TP &Pix(POINT pt) const { return *Ptr(pt); }
	TP &Pix(POINT pt) { return *Ptr(pt); }

	const BaseImagePixelType& Pix(POINT pt, int iBand) const { return *Ptr(pt, iBand); }
	BaseImagePixelType& Pix(POINT pt, int iBand) { return *Ptr(pt, iBand); }
	///@}

	/// @name Typed Pixel Access (Reference, alternate syntax)
	///@{

	const TP &operator()(int iX, int iY) const { return *Ptr(iX, iY); }
	TP &operator()(int iX, int iY) { return *Ptr(iX, iY); }

	const BaseImagePixelType& operator()(int iX, int iY, int iBand) const { return *Ptr(iX, iY, iBand); }
	BaseImagePixelType& operator()(int iX, int iY, int iBand) { return *Ptr(iX, iY, iBand); }

	const TP &operator()(POINT pt) const { return *Ptr(pt); }
	TP &operator()(POINT pt) { return *Ptr(pt); }

	const BaseImagePixelType& operator()(POINT pt, int iBand) const { return *Ptr(pt, iBand); }
	BaseImagePixelType& operator()(POINT pt, int iBand) { return *Ptr(pt, iBand); }
	///@}

	/// @name Row Pointer Arithmetic 
	///@{

	/// <summary> Return pointer to pixel one row below </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) + StrideBytes()	
	const TP *NextRow(const TP *pRow) const 
	{ return reinterpret_cast<const TP*>(
		CImg::NextRow(reinterpret_cast<const Byte*>(pRow))); }
	/// <summary> Return pointer to pixel one row below </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) + StrideBytes()	
	VT_NON_CONST_PTR_ACCESS_COMP_IMG(TP, NextRow(TP* pRow), NextRow(pRow));

	/// <summary> Return pointer to pixel one row above </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) - StrideBytes()	
	const TP *PrevRow(const TP *pRow) const 
	{ return reinterpret_cast<const TP*>(
		CImg::PrevRow(reinterpret_cast<const Byte*>(pRow))); }
	/// <summary> Return pointer to pixel one row above  </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) - StrideBytes()	
	VT_NON_CONST_PTR_ACCESS_COMP_IMG(TP, PrevRow(TP* pRow), PrevRow(pRow));

	/// <summary> Return pointer to pixel N rows away </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) + numOffsetRows * StrideBytes()	
	const TP *OffsetRow(const TP *pRow, int numOffsetRows) const 
	{ return reinterpret_cast<const TP*>(
		CImg::OffsetRow(reinterpret_cast<const Byte*>(pRow), numOffsetRows)); }
	/// <summary> Return pointer to pixel N rows away </summary>
	/// <DL><DT> Remarks: </DT></DL>
	///		- Computes ((Byte*) pRow) + numOffsetRows * StrideBytes()	
	VT_NON_CONST_PTR_ACCESS_COMP_IMG(TP, OffsetRow(TP* pRow, int numOffsetRows), 
		OffsetRow(pRow, numOffsetRows));

	/// @name Typed Fill
	///@{

	/// <param name="cValue"> Fill value of type T</param>
	/// <param name="prctDst"> Region to fill. Optional. If NULL, the entire image
	/// is filled. </param>
	///	<returns> 
	///		- S_OK on success
	///		- E_NOINIT if image is invalid
	/// </returns>
	HRESULT Fill(const TP& cValue, const RECT *prctDst = NULL) 
	{ return CImg::Fill((Byte *)&cValue, prctDst, -1, false); }

	///@}

	static int TypeIs()
	{
		VT_ASSERT(T::Bands() <= VT_IMG_BANDS_MAX);
		return VT_IMG_FIXED_PIXFRMT_MASK | VT_IMG_FIXED_ELFRMT_MASK |
			VT_IMG_MAKE_COMP_TYPE(T::PixFormat(), ElTraits<BaseImagePixelType>::ElFormat(),
								  T::Bands());
	}
};

/// @name CCompositeImg Typedefs
///@{ 

/// \ingroup pixformat
/// <summary> Luma byte image</summary>
typedef CCompositeImg<LumaPix, LumaPix::ELT> CLumaImg;
/// <summary> Luma byte image</summary>
typedef CCompositeImg<LumaPix, LumaPix::ELT> CLumaByteImg;
/// <summary> Luma short image</summary>
typedef CCompositeImg<LumaShortPix, LumaShortPix::ELT>         CLumaShortImg;
/// <summary> Luma half float image</summary>
typedef CCompositeImg<LumaHalfFloatPix, LumaHalfFloatPix::ELT> CLumaHalfFloatImg;
/// <summary> Luma float image</summary>
typedef CCompositeImg<LumaFloatPix, LumaFloatPix::ELT>         CLumaFloatImg;
/// <summary> UV byte image</summary>
typedef CCompositeImg<UVPix>            CUVImg;
/// <summary> UV byte image</summary>
typedef CCompositeImg<UVPix>            CUVByteImg;
/// <summary> UV short image</summary>
typedef CCompositeImg<UVShortPix>       CUVShortImg;
/// <summary> UV half float image</summary>
typedef CCompositeImg<UVHalfFloatPix>   CUVHalfFloatImg;
/// <summary> UV float image</summary>
typedef CCompositeImg<UVFloatPix>       CUVFloatImg;
/// <summary> RGB byte image</summary>
typedef CCompositeImg<RGBPix>           CRGBImg;
/// <summary> RGB byte image</summary>
typedef CCompositeImg<RGBPix>           CRGBByteImg;
/// <summary> RGB short image</summary>
typedef CCompositeImg<RGBShortPix>      CRGBShortImg;
/// <summary> RGBA signed short image</summary>
typedef CCompositeImg<RGBASShortPix>     CRGBASShortImg;
/// <summary> RGB half float image</summary>
typedef CCompositeImg<RGBHalfFloatPix>  CRGBHalfFloatImg;
/// <summary> RGB float image</summary>
typedef CCompositeImg<RGBFloatPix>      CRGBFloatImg;
/// <summary> RGBA byte image</summary>
typedef CCompositeImg<RGBAPix>          CRGBAImg;
/// <summary> RGBA byte image</summary>
typedef CCompositeImg<RGBAPix>          CRGBAByteImg;
/// <summary> RGBA short image</summary>
typedef CCompositeImg<RGBAShortPix>     CRGBAShortImg;
/// <summary> RGBA half float image</summary>
typedef CCompositeImg<RGBAHalfFloatPix> CRGBAHalfFloatImg;
/// <summary> RGBA float image</summary>
typedef CCompositeImg<RGBAFloatPix>     CRGBAFloatImg;

#ifndef VT_NO_VECTOR_IMAGE_TYPEDEFS
/// \anchor CVec2fImgAnchor
/// <summary> CVec2f image </summary>
typedef CCompositeImg<CVec2f>           CVec2Img;
typedef CCompositeImg<CVec3f>           CVec3Img;

///@}

typedef CCompositeImg<CVec2f>           CFlowImg;
typedef CCompositeImg<Complexf>         CComplexImg;

#define CIMG_TYPEIS(s, t) template<> inline int s::TypeIs() \
    { return VT_IMG_FIXED_PIXFRMT_MASK | VT_IMG_FIXED_ELFRMT_MASK | t; }
CIMG_TYPEIS(CFlowImg,    OBJ_FLOWIMG)
CIMG_TYPEIS(CComplexImg, OBJ_COMPLEXIMG)
CIMG_TYPEIS(CCompositeImg<CVec3f>, VT_IMG_MAKE_COMP_TYPE(PIX_FORMAT_NONE, EL_FORMAT_FLOAT, 3))
#endif

/// \ingroup pixformat
/// <summary> Get element format string from composite type</summary>
/// <param name="iType"> Image type </param>
const wchar_t* VtElFormatStringFromType(int iType);

/// \ingroup pixformat
/// <summary> Get pixel format string from composite type</summary>
/// <param name="iType"> Image type </param>
/// <param name="bTrueOrder"> Return simple or true order format. For example, 
/// if an RGB image is actually stored as B, G, R, setting trueOrder to true will 
/// return BGR instead of RGB. </param> 
const wchar_t* VtPixFormatStringFromType(int iType, bool bTrueOrder);

//+-----------------------------------------------------------------------------
//
// Function: UpdateMutableTypeFields
// 
//------------------------------------------------------------------------------
int UpdateMutableTypeFields(int iType, int iUpdateType);

//+-----------------------------------------------------------------------------
//
// Function: CreateImageForTransform
// 
//------------------------------------------------------------------------------
HRESULT CreateImageForTransform(CImg& dst, int iWidth, int iHeight, int iDefType);

//+-----------------------------------------------------------------------------
//
// Function: Install image debug hooks
// 
//------------------------------------------------------------------------------
typedef bool (*CImgDebugHook)(const CImg&);
CImgDebugHook InstallDebugHookCImgConstructor(CImgDebugHook);
CImgDebugHook InstallDebugHookCImgDestructor(CImgDebugHook);

// forward declarations for ObjectMover specialization below
#if defined(_MSC_VER) && !defined(MSRVT_WINDOWS_BUILD) \
	&& !defined(VT_WINRT)
namespace imgdbg
{
	extern bool g_debuggerActive;

	bool SetValidFlag(const void* old_address, bool is_valid);
	bool MoveAddress(const void* old_address, 
		const void* new_address);
};
#endif

#ifndef VT_NO_VECTORSTRING
// specialization of vt::vector's ObjectMover template for CImg objects
namespace vectorhelpers
{
	template<typename T>
	struct ObjectMover<T, typename std::enable_if<
		std::is_base_of<vt::CImg, T>::value>::type>
	{
#if defined(_MSC_VER) && !defined(MSRVT_WINDOWS_BUILD) \
	&& !defined(VT_WINRT)
		static void PrepareMove(const T* src, const T*, size_t n_elem)
		{
			if (vt::imgdbg::g_debuggerActive)
			{
				for (size_t i = 0; i < n_elem; ++i)
					vt::imgdbg::SetValidFlag(src + i, false);					
			}
		}

		static void FinalizeMove(const T* src, const T* dst, size_t n_elem)
		{
			if (vt::imgdbg::g_debuggerActive)
			{
				for (size_t i = 0; i < n_elem; ++i)
				{
					vt::imgdbg::MoveAddress(src + i, dst + i);
					vt::imgdbg::SetValidFlag(dst + i, true);
				}
			}
		}
#else
		static void PrepareMove(const T*, const T*, size_t)
		{}
		
		static void FinalizeMove(const T*, const T*, size_t)
		{}
#endif
	};
};
#endif
};