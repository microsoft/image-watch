//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for converting between image types
//
//  History:
//      2004/11/08-mattu/swinder
//			Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_convert.h"
#include "vt_convert.inl"
#include "vt_imgmath.h"
#include "vt_imgmath.inl"
#include "vt_function.h"

#if defined(VT_WINRT)
#include <robuffer.h>
#pragma warning(push)
#pragma warning(disable:26135) // wrl 'corewrappers.h' has missing lock annotations
#pragma warning(disable:26007) // wrl 'module.h' has missing buffer annotations
#include <wrl.h>
#pragma warning(pop)

using namespace Microsoft::WRL;
using namespace Platform;
using namespace Windows::Storage::Streams;
#if (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)
using namespace Windows::UI::Xaml::Media::Imaging;
#endif
#endif

using namespace vt;

///////////////////////////////////////////////////////////////////////////////
//
// HALF_FLOAT statics
//
///////////////////////////////////////////////////////////////////////////////
const float vt::HALF_FLOAT_CONVERT_STATICS::fminFloatVal = -131008.f;
const float vt::HALF_FLOAT_CONVERT_STATICS::fmaxFloatVal = 131008.f;
const float vt::HALF_FLOAT_CONVERT_STATICS::powf2to112 = powf(2.0f, 112.0f);
const float vt::HALF_FLOAT_CONVERT_STATICS::oneOverPowf2to112 = 1.0f / powf(2.0f, 112.0f);

const vt::HALF_FLOAT_CONVERT_STATICS vt::g_hfcconst = 
	vt::HALF_FLOAT_CONVERT_STATICS(); // GCC wants copy construction here

///////////////////////////////////////////////////////////////////////////////
//
// utilities
//
///////////////////////////////////////////////////////////////////////////////
bool vt::IsValidConvertPair(int typeA, int typeB)
{
	// memcpy case
	if (!VT_IMG_ISUNDEF(typeA) && VT_IMG_SAME_PBE(typeA,typeB))
		return true;

	int iElA  = EL_FORMAT(typeA);
    int iElB  = EL_FORMAT(typeB);
	int bandsA = VT_IMG_BANDS(typeA);
	int bandsB = VT_IMG_BANDS(typeB);

	// int/double special case
	if (iElA == EL_FORMAT_INT || iElA == EL_FORMAT_DOUBLE || 
		iElA == EL_FORMAT_FLOAT)
	{
		if (bandsA == bandsB && 
			(iElB == EL_FORMAT_INT || iElB == EL_FORMAT_DOUBLE ||
			iElB == EL_FORMAT_FLOAT))
			return true;
	}	
	
	// sbyte special case
	if (iElA == EL_FORMAT_SBYTE)
	{
		if (bandsA == bandsB && 
			(iElB == EL_FORMAT_SBYTE || iElB == EL_FORMAT_SSHORT 
			|| iElB == EL_FORMAT_FLOAT))
			return true;
	}

	// sshort special case
	if (iElA == EL_FORMAT_SSHORT)
	{
		if (bandsA == bandsB && 
			(iElB == EL_FORMAT_SSHORT || iElB == EL_FORMAT_FLOAT))
			return true;
	}

	// float to sbyte/sshort
	else if (iElA == EL_FORMAT_FLOAT)
	{
		if (bandsA == bandsB && 
			(iElB == EL_FORMAT_SBYTE || iElB == EL_FORMAT_SSHORT))
			return true;
	}

	// check for standard format
	if( !(iElA==EL_FORMAT_BYTE  || iElA==EL_FORMAT_SHORT ||
          iElA==EL_FORMAT_FLOAT || iElA==EL_FORMAT_HALF_FLOAT) )
    {
        return false;
    }

    if( !(iElB==EL_FORMAT_BYTE  || iElB==EL_FORMAT_SHORT ||
          iElB==EL_FORMAT_FLOAT || iElB==EL_FORMAT_HALF_FLOAT) )
    {
        return false;
    }

    // check for valid convert band counts
    if( !(bandsA == bandsB || ((bandsA==1||bandsA==3||bandsA==4) && 
							   (bandsB==1||bandsB==3||bandsB==4))) )
    {
        return false;
    }

    return true;
}

bool vt::VtIsValidConvertImagePair(const CImg& imgA, const CImg& imgB)
{ return IsValidConvertPair(imgA.GetType(), imgB.GetType()); }

HRESULT vt::VtConvertSpan(void* pDst, int iDstType, const void* pSrc, 
						  int iSrcType, int iSrcElCount, bool bBypassCache)
{
    VT_HR_BEGIN();

	int iDstBands = VT_IMG_BANDS(iDstType);
	int iSrcBands = VT_IMG_BANDS(iSrcType);
	
	VT_HR_EXIT((iSrcElCount % iSrcBands == 0) ? S_OK : E_INVALIDARG);

	VT_HR_EXIT(IsValidConvertPair(iSrcType, iDstType)? S_OK : E_INVALIDARG);
    
	// memcpy case 
	if (iSrcType == iDstType)
	{
		VtMemcpy(pDst, pSrc, iSrcElCount * VT_IMG_ELSIZE(iSrcType), 
				 bBypassCache);
		
		return S_OK;
	}

	#define DEF_CASE() default: hr = E_NOTIMPL; break;

    #define DO_CONV(i, TD, TS) \
    	case i: hr = VtConvertSpanBands((TD*)pDst, iDstBands, (const TS*)pSrc, \
			                            iSrcBands, iSrcElCount, bBypassCache); \
		    break;

	#define DO_CONV_SAME_BANDS(i, TD, TS) \
    	case i: VT_ASSERT(iSrcBands == iDstBands); \
		VtConvertSpan((TD*)pDst, (const TS*)pSrc, iSrcElCount, bBypassCache); \
			break;

    switch (EL_FORMAT(iSrcType))
    {
	case EL_FORMAT_SBYTE:
        switch (EL_FORMAT(iDstType))
        {
            DO_CONV_SAME_BANDS(EL_FORMAT_SBYTE, int8_t, int8_t);
            DO_CONV_SAME_BANDS(EL_FORMAT_SSHORT, int16_t, int8_t);
    		DO_CONV_SAME_BANDS(EL_FORMAT_FLOAT, float, int8_t);
            DEF_CASE();
        }
        break;
	case EL_FORMAT_BYTE:
        switch (EL_FORMAT(iDstType))
        {
            DO_CONV(EL_FORMAT_BYTE, Byte, Byte);
            DO_CONV(EL_FORMAT_SHORT, UInt16, Byte);
    		DO_CONV(EL_FORMAT_HALF_FLOAT, HALF_FLOAT, Byte);
    		DO_CONV(EL_FORMAT_FLOAT, float, Byte);
            DEF_CASE();
        }
        break;
	case EL_FORMAT_SSHORT:
        switch (EL_FORMAT(iDstType))
        {
            DO_CONV_SAME_BANDS(EL_FORMAT_SSHORT, int16_t, int16_t);
            DO_CONV_SAME_BANDS(EL_FORMAT_FLOAT, float, int16_t);
            DEF_CASE();
        }
        break;
    case EL_FORMAT_SHORT:
        switch (EL_FORMAT(iDstType))
        {
            DO_CONV(EL_FORMAT_BYTE, Byte, UInt16);
            DO_CONV(EL_FORMAT_SHORT, UInt16, UInt16);
            DO_CONV(EL_FORMAT_HALF_FLOAT, HALF_FLOAT, UInt16);
            DO_CONV(EL_FORMAT_FLOAT, float, UInt16);
            DEF_CASE();
        }
        break;
    case EL_FORMAT_HALF_FLOAT:
        switch (EL_FORMAT(iDstType))
        {
            DO_CONV(EL_FORMAT_BYTE,  Byte, HALF_FLOAT);
            DO_CONV(EL_FORMAT_SHORT, UInt16, HALF_FLOAT);
            DO_CONV(EL_FORMAT_HALF_FLOAT, HALF_FLOAT, HALF_FLOAT);
            DO_CONV(EL_FORMAT_FLOAT, float, HALF_FLOAT);
            DEF_CASE();
        }
        break;
    case EL_FORMAT_FLOAT:
        switch (EL_FORMAT(iDstType))
        {
            DO_CONV(EL_FORMAT_BYTE,  Byte, float);
            DO_CONV(EL_FORMAT_SHORT, UInt16, float);
            DO_CONV(EL_FORMAT_HALF_FLOAT, HALF_FLOAT, float);
            DO_CONV(EL_FORMAT_FLOAT, float, float);
            DO_CONV_SAME_BANDS(EL_FORMAT_INT, int, float);
            DO_CONV_SAME_BANDS(EL_FORMAT_DOUBLE, double, float);
            DO_CONV_SAME_BANDS(EL_FORMAT_SBYTE, int8_t, float);
            DO_CONV_SAME_BANDS(EL_FORMAT_SSHORT, int16_t, float);
            DEF_CASE();
        }
        break;
    case EL_FORMAT_INT:
        switch (EL_FORMAT(iDstType))
        {
			DO_CONV_SAME_BANDS(EL_FORMAT_INT, int, int);
			DO_CONV_SAME_BANDS(EL_FORMAT_FLOAT, float, int);
            DO_CONV_SAME_BANDS(EL_FORMAT_DOUBLE, double, int);
            DEF_CASE();
        }
        break;
    case EL_FORMAT_DOUBLE:
        switch (EL_FORMAT(iDstType))
        {
            DO_CONV_SAME_BANDS(EL_FORMAT_FLOAT, float, double);
            DO_CONV_SAME_BANDS(EL_FORMAT_INT, int, double);
            DEF_CASE();
        }
        break;
	DEF_CASE()
    }

	#undef DO_CONV
	#undef DO_CONV_SAME_BANDS
	#undef DEF_CASE

	VT_HR_END();
}

HRESULT vt::VtConvertImage(CImg &imgDst, const CImg &imgSrc, bool bBypassCache)
{
	// if an identical image is passed for source and dest then simply return
	if( imgSrc.BytePtr() == imgDst.BytePtr() &&
		EL_FORMAT(imgSrc.GetType()) == EL_FORMAT(imgDst.GetType()) &&
		imgSrc.Width()   == imgDst.Width()  &&
		imgSrc.Height()  == imgDst.Height() &&
		imgSrc.Bands()   == imgDst.Bands() )
	{
		return S_OK;
	}

	VT_HR_BEGIN();

	VT_HR_EXIT(imgSrc.IsSharingMemory(imgDst) ? E_INVALIDARG : S_OK);

	VT_HR_EXIT(PrepareUnaryImgOp(imgSrc, imgDst));

	if (imgSrc.GetType() == imgDst.GetType() )
	{
		for (int y = 0; y < imgDst.Height(); ++y)
		{
			VtMemcpy(imgDst.BytePtr(y), imgSrc.BytePtr(y),
                     imgSrc.Width() * imgSrc.PixSize(), 
					 bBypassCache);
		}
	}
	else
	{
		for (int y = 0; y < imgDst.Height(); ++y)
		{
			VT_HR_EXIT(VtConvertSpan(imgDst.BytePtr(y), imgDst.GetType(), 
									 imgSrc.BytePtr(y), imgSrc.GetType(), 
									 imgSrc.Width() * imgSrc.Bands(), 
									 bBypassCache));
		}
	}
	
	VT_HR_END();
}

static HRESULT ConvertImgToComp(CImg &imgDst, const CImg &imgSrc, 
								int pixfrmt, int bands, bool bBypassCache)
{
    VT_HR_BEGIN();

    // Try to create luminance image of source type first, then dest type.
    int iDstType = VT_IMG_MAKE_COMP_TYPE(pixfrmt, EL_FORMAT(imgSrc.GetType()), 
										 bands);
    if ( !imgDst.IsCreatableAs(iDstType) )
	{
		iDstType = VT_IMG_MAKE_COMP_TYPE(
			pixfrmt, EL_FORMAT(imgDst.GetType()), bands);
		if (!imgDst.IsCreatableAs(iDstType))
		{
			VT_HR_EXIT( E_INVALIDDST );
		}
	}

	if( imgDst.Width() != imgSrc.Width() || imgDst.Height() != imgSrc.Height() ||
		imgDst.GetType() != iDstType )
	{
		VT_HR_EXIT( imgDst.Create(imgSrc.Width(), imgSrc.Height(), iDstType) );
	}

    VT_HR_EXIT( VtConvertImage(imgDst, imgSrc, bBypassCache) );

    VT_HR_END();
}

#if (defined(VT_WINRT) && defined(_WIN32_WINNT_WINBLUE))
HRESULT vt::VtConvertImageToWriteableBitmap(Windows::UI::Xaml::Media::Imaging::WriteableBitmap^& writeableBitmap, const CImg& imgSrc, bool bBypassCache)
{
	VT_HR_BEGIN();

	// Make sure we have a writeable bitmap of the correct size.
	int width = imgSrc.Width();
	int height = imgSrc.Height();
	if (writeableBitmap == nullptr || writeableBitmap->PixelWidth != width || writeableBitmap->PixelHeight != height)
	{
		writeableBitmap = ref new Windows::UI::Xaml::Media::Imaging::WriteableBitmap(width, height);
	}

	// Get a pointer to the writeable bitmap's pixel buffer.
	IBuffer^ pixelBuffer = writeableBitmap->PixelBuffer;
	ComPtr<IUnknown> comBuffer(reinterpret_cast<IUnknown*>(pixelBuffer));
	ComPtr<IBufferByteAccess> byteBuffer;
	comBuffer.As(&byteBuffer);
	byte* data;
	VT_HR_EXIT(byteBuffer->Buffer(&data));

	// Wrap a Vision Tools RGBA image around the writeable bitmap's pixel buffer.
	CRGBAImg wrappingImage;
	VT_HR_EXIT(wrappingImage.Create(data, width, height, 4 * width));

	// Convert the pixels in the given Vision Tools image to the pixels in the writeable bitmap.
	VT_HR_EXIT(VtConvertImage(wrappingImage, imgSrc, bBypassCache));

	// Notify the writeable bitmap that the pixels have been modified.
	writeableBitmap->Invalidate();

	VT_HR_END();
}

HRESULT vt::VtConvertImageToByteArray(Array<byte>^& byteArray, const CImg& imgSrc, bool bBypassCache)
{
	VT_HR_BEGIN();

	// Make sure we have an array of the correct size.
	int width = imgSrc.Width();
	int height = imgSrc.Height();
	if (byteArray == nullptr || byteArray->Length != 4 * (unsigned)width * (unsigned)height)
	{
		byteArray = ref new Array<byte>(4 * width * height);
	}

	// Wrap a Vision Tools RGBA image around the writeable bitmap's pixel buffer.
	CRGBAImg wrappingImage;
	VT_HR_EXIT(wrappingImage.Create(byteArray->Data, width, height, 4 * width));

	// Convert the pixels in the given Vision Tools image to the pixels in the writeable bitmap.
	VT_HR_EXIT(VtConvertImage(wrappingImage, imgSrc, bBypassCache));

	// Swap the red and blue channels.
	VT_HR_EXIT(VtRGBColorSwapImage(wrappingImage));

	VT_HR_END();
}
#endif

HRESULT vt::VtConvertImageToLuma(CImg &imgDst, const CImg &imgSrc, bool bBypassCache)
{ return ConvertImgToComp(imgDst, imgSrc, PIX_FORMAT_LUMA, 1, bBypassCache); }

HRESULT vt::VtConvertImageToRGB(CImg &imgDst, const CImg &imgSrc, bool bBypassCache)
{ return ConvertImgToComp(imgDst, imgSrc, PIX_FORMAT_RGB, 3, bBypassCache); }

HRESULT vt::VtConvertImageToRGBA(CImg &imgDst, const CImg &imgSrc, bool bBypassCache)
{ return ConvertImgToComp(imgDst, imgSrc, PIX_FORMAT_RGBA, 4, bBypassCache); }

HRESULT vt::VtConvertBands(CImg& cDst, const CImg& cSrc, int numDstBands, 
						   const BandIndexType* peBandSet, const void* pFillVals)
{
	VT_HR_BEGIN();

	if (!cSrc.IsValid())
		VT_HR_EXIT(E_INVALIDSRC);
	
	if (cSrc.IsSharingMemory(cDst))
		VT_HR_EXIT(E_INVALIDARG);

	if (peBandSet == NULL)
		VT_HR_EXIT(E_INVALIDARG);

	VT_HR_EXIT(InitDst(cDst, cSrc.Width(), cSrc.Height(), 
		VT_IMG_MAKE_TYPE(EL_FORMAT(cSrc.GetType()), numDstBands)));

	VT_ASSERT(cDst.Bands() == numDstBands);
	
	for (int i = 0; i < cDst.Bands(); ++i)
	{
		if (peBandSet[i] < BandIndexFill || peBandSet[i] >= cSrc.Bands())
			return E_INVALIDARG;
	}
	
	#define DO_CONV(TO, TINAME, TI) case TINAME:\
				vt::VtConvertBandsSpan((TO*)cDst.BytePtr(iY), cDst.Bands(),\
				(const TI*)cSrc.BytePtr(iY), cSrc.Bands(), cDst.Width(),\
				peBandSet, (const TO*)pFillVals);\
				break;

	for (int iY = 0; iY < cDst.Height(); ++iY)
	{
		switch (EL_FORMAT(cDst.GetType()))
		{
		case EL_FORMAT_SBYTE:
			switch (EL_FORMAT(cSrc.GetType()))
			{
				DO_CONV(int8_t, EL_FORMAT_SBYTE, int8_t);				
				DO_CONV(int8_t, EL_FORMAT_FLOAT, float);				
			default:
				VT_HR_EXIT(E_NOTIMPL);
			}
			break;
		case EL_FORMAT_BYTE:
			switch (EL_FORMAT(cSrc.GetType()))
			{
				DO_CONV(Byte, EL_FORMAT_BYTE, Byte);
				DO_CONV(Byte, EL_FORMAT_FLOAT, float);
				DO_CONV(Byte, EL_FORMAT_HALF_FLOAT, HALF_FLOAT);
				DO_CONV(Byte, EL_FORMAT_SHORT, UInt16);
			default:
				VT_HR_EXIT(E_NOTIMPL);
			}
			break;
		case EL_FORMAT_FLOAT:
			switch (EL_FORMAT(cSrc.GetType()))
			{
				DO_CONV(float, EL_FORMAT_SBYTE, int8_t);
				DO_CONV(float, EL_FORMAT_BYTE, Byte);
				DO_CONV(float, EL_FORMAT_FLOAT, float);
				DO_CONV(float, EL_FORMAT_HALF_FLOAT, HALF_FLOAT);
				DO_CONV(float, EL_FORMAT_SSHORT, int16_t);
				DO_CONV(float, EL_FORMAT_SHORT, UInt16);
				DO_CONV(float, EL_FORMAT_INT, int);
				DO_CONV(float, EL_FORMAT_DOUBLE, double);
			default:
				VT_HR_EXIT(E_NOTIMPL);
			}
			break;
		case EL_FORMAT_HALF_FLOAT:
			switch (EL_FORMAT(cSrc.GetType()))
			{
				DO_CONV(HALF_FLOAT, EL_FORMAT_BYTE, Byte);
				DO_CONV(HALF_FLOAT, EL_FORMAT_FLOAT, float);
				DO_CONV(HALF_FLOAT, EL_FORMAT_HALF_FLOAT, HALF_FLOAT);
				DO_CONV(HALF_FLOAT, EL_FORMAT_SHORT, UInt16);
			default:
				VT_HR_EXIT(E_NOTIMPL);
			}
			break;
		case EL_FORMAT_SSHORT:
			switch (EL_FORMAT(cSrc.GetType()))
			{
				DO_CONV(int16_t, EL_FORMAT_SBYTE, int8_t);
				DO_CONV(int16_t, EL_FORMAT_SSHORT, int16_t);
				DO_CONV(int16_t, EL_FORMAT_FLOAT, float);
			default:
				VT_HR_EXIT(E_NOTIMPL);
			}
			break;
		case EL_FORMAT_SHORT:
			switch (EL_FORMAT(cSrc.GetType()))
			{
				DO_CONV(UInt16, EL_FORMAT_BYTE, Byte);
				DO_CONV(UInt16, EL_FORMAT_FLOAT, float);
				DO_CONV(UInt16, EL_FORMAT_HALF_FLOAT, HALF_FLOAT);
				DO_CONV(UInt16, EL_FORMAT_SHORT, UInt16);
			default:
				VT_HR_EXIT(E_NOTIMPL);
			}
			break;
		case EL_FORMAT_INT:
			switch (EL_FORMAT(cSrc.GetType()))
			{
				DO_CONV(int, EL_FORMAT_INT, int);
				DO_CONV(int, EL_FORMAT_FLOAT, float);
				DO_CONV(int, EL_FORMAT_DOUBLE, double);
			default:
				VT_HR_EXIT(E_NOTIMPL);
			}
			break;
		case EL_FORMAT_DOUBLE:
			switch (EL_FORMAT(cSrc.GetType()))
			{
				DO_CONV(double, EL_FORMAT_DOUBLE, double);
				DO_CONV(double, EL_FORMAT_FLOAT, float);
				DO_CONV(double, EL_FORMAT_INT, int);
			default:
				VT_HR_EXIT(E_NOTIMPL);
			}
			break;
		default:
			VT_HR_EXIT(E_NOTIMPL);
		}
	}

	VT_HR_END();
}

//+-----------------------------------------------------------------------------
//
// BGR to RGB swap routines
//
//------------------------------------------------------------------------------
HRESULT vt::VtRGBColorSwapImage(CImg& img)
{
    VT_HR_BEGIN();
	
	if (!img.IsValid())
		VT_HR_EXIT(E_INVALIDSRC);
		
	if (!IsColorImage(img))
	    VT_HR_EXIT(E_INVALIDSRC);
	
	for( int y = 0; y < img.Height(); y++)
	{
        VT_HR_EXIT(VtRGBColorSwapSpan(img.BytePtr(y), img.GetType(), 
									  img.BytePtr(y), img.GetType(), img.Width()));
	}
	
	VT_HR_END();
}

HRESULT
vt::VtRGBColorSwapImage(CImg& imgDst, const CImg& imgSrc)
{
    VT_HR_BEGIN();
	
	if (!imgSrc.IsValid())
		VT_HR_EXIT(E_INVALIDSRC);
	
	if (!IsColorImage(imgSrc))
		VT_HR_EXIT(E_INVALIDSRC);
	
	if (imgSrc.IsSharingMemory(imgDst))
		VT_HR_EXIT(E_INVALIDARG);

	VT_HR_EXIT(InitDst(imgDst, imgSrc));
			
	for( int y = 0; y < imgDst.Height(); y++)
	{
        VT_HR_EXIT(VtRGBColorSwapSpan(imgDst.BytePtr(y), imgDst.GetType(),
									  imgSrc.BytePtr(y), imgSrc.GetType(),
									  imgSrc.Width()));
	}
	
	VT_HR_END();
}

HRESULT vt::VtRGBColorSwapSpan(Byte* pDst, int iDstType, 
							   const Byte* pSrc, int iSrcType, int pixcnt)
{
	int iDstBands = VT_IMG_BANDS(iDstType);
	int iSrcBands = VT_IMG_BANDS(iSrcType);

    VT_ASSERT((iDstBands == 3 || iDstBands == 4) && 
		(iSrcBands == 3 || iSrcBands == 4));
    
    VT_HR_BEGIN();
	
	Byte buffer[1024];

    int   iProcPixCnt;
    Byte* pProc;
    if( iDstType!=iSrcType || iDstBands!=iSrcBands )
    {
        iProcPixCnt = sizeof(buffer) / (VT_IMG_ELSIZE(iSrcType)*iSrcBands);
        pProc       = buffer;
    }
    else
    {
        iProcPixCnt = pixcnt;
        pProc       = pDst;
    }
    int iDstByteCnt = iProcPixCnt*VT_IMG_ELSIZE(iDstType)*iDstBands;
    int iSrcByteCnt = iProcPixCnt*VT_IMG_ELSIZE(iSrcType)*iSrcBands;

    for( int i = 0; i < pixcnt; i+=iProcPixCnt, pDst+=iDstByteCnt, pSrc+=iSrcByteCnt )
    {   
        // process source
        int iCurPixCnt = VtMin(iProcPixCnt, pixcnt-i);
        if ( iSrcBands == 3)
        {
            switch ( EL_FORMAT(iSrcType) )
            {
            case EL_FORMAT_BYTE:
                VtRGBColorSwapSpan((RGBType<Byte>*)pProc, (RGBType<Byte>*)pSrc, 
                                   iCurPixCnt);
                break;
            case EL_FORMAT_SHORT:
                VtRGBColorSwapSpan((RGBType<UInt16>*)pProc, (RGBType<UInt16>*)pSrc, 
                                   iCurPixCnt);
                break;
            case EL_FORMAT_HALF_FLOAT:
                VtRGBColorSwapSpan((RGBType<HALF_FLOAT>*)pProc, 
                                   (RGBType<HALF_FLOAT>*)pSrc, iCurPixCnt);
                break;
            case EL_FORMAT_FLOAT:
                VtRGBColorSwapSpan((RGBType<float>*)pProc, (RGBType<float>*)pSrc,
                                   iCurPixCnt);
                break;
			default:
				return E_NOTIMPL;
            }
        }
        else
        {
            switch ( EL_FORMAT(iSrcType) )
            {
            case EL_FORMAT_BYTE:
                VtRGBColorSwapSpan((RGBAType<Byte>*)pProc, (RGBAType<Byte>*)pSrc,
                                   iCurPixCnt);
                break;
            case EL_FORMAT_SHORT:
                VtRGBColorSwapSpan((RGBAType<UInt16>*)pProc, (RGBAType<UInt16>*)pSrc,
                                   iCurPixCnt);
                break;
            case EL_FORMAT_HALF_FLOAT:
                VtRGBColorSwapSpan((RGBAType<HALF_FLOAT>*)pProc,
                                   (RGBAType<HALF_FLOAT>*)pSrc, iCurPixCnt);
                break;
            case EL_FORMAT_FLOAT:
                VtRGBColorSwapSpan((RGBAType<float>*)pProc, (RGBAType<float>*)pSrc, 
                                   iCurPixCnt);
                break;
			default:
				return E_NOTIMPL;
            }
        }

        // convert to dest type if necessary
        if( iDstType!=iSrcType || iDstBands!=iSrcBands )
        {
            VT_HR_EXIT(VtConvertSpan(pDst, iDstType, pProc, iSrcType,
                                     iCurPixCnt*iSrcBands));
        }
    }

    VT_HR_END();
}

template <>
float* vt::VtConvertSpanARGBTo1BandSSE(float* pOut, const float* pIn, int inspan, int band)
{
    float *pOutTmp = pOut;
	// not really more efficient to implement this in sse
	int i = 0;
	for(i=0; i<inspan-15; i+=16, pIn+=16, pOut+=4)
	{
		pOut[0] = pIn[band];
		pOut[1] = pIn[band+4];
		pOut[2] = pIn[band+8];
		pOut[3] = pIn[band+12];
	}
	for( ; i<inspan; i+=4, pIn+=4, pOut++)
	{
		pOut[0] = pIn[band];
	}

	return pOutTmp;
}

template <>
float* vt::VtConvertSpanARGBTo1BandSSE(float* pOut, const Byte* pIn, 
									   int inspan, int band)
{
	VT_ASSERT( g_SupportSSE2() );
	
    float *pOutTmp = pOut;

	int i;

	// handle un-aligned output pointer
    for(i=0 ; !IsAligned16(pOut) && i < inspan; i+=4, pIn += 4 )
        VtConv(pOut++, pIn[band]);

#if (defined(_M_IX86) || defined(_M_AMD64))
	// mask = 0x000000ff000000ff000000ff000000ff
	__m128i x_mask = _mm_set1_epi32(0xff);
	__m128 x6 = _mm_set1_ps(1.f/float(ElTraits<Byte>::MaxVal()));

	if(band==0)
	{
		for( ; i <inspan-15; i+=16, pIn+=16, pOut+=4)
		{   
			__m128i x1 = _mm_loadu_si128((__m128i *)pIn);
			__m128i x3 = _mm_and_si128(x1, x_mask);
			__m128 x4 = _mm_cvtepi32_ps(x3);
			x4 = _mm_mul_ps(x4, x6);
			_mm_store_ps(pOut, x4);
		}
	}
	else if(band==1)
	{
		for( ; i <inspan-15; i+=16, pIn+=16, pOut+=4)
		{
			__m128i x1 = _mm_loadu_si128((__m128i *)pIn);
			__m128i x2 = _mm_srli_si128(x1, 1);
			__m128i x3 = _mm_and_si128(x2, x_mask);
			__m128 x4 = _mm_cvtepi32_ps(x3);
			x4 = _mm_mul_ps(x4, x6);
			_mm_store_ps(pOut, x4);
		}
	}
	else if(band==2)
	{
		for( ; i <inspan-15; i+=16, pIn+=16, pOut+=4)
		{
			__m128i x1 = _mm_loadu_si128((__m128i *)pIn);
			__m128i x2 = _mm_srli_si128(x1, 2);
			__m128i x3 = _mm_and_si128(x2, x_mask);
			__m128 x4 = _mm_cvtepi32_ps(x3);
			x4 = _mm_mul_ps(x4, x6);
			_mm_store_ps(pOut, x4);
		}
	}
	else
	{
		for( ; i <inspan-15; i+=16, pIn+=16, pOut+=4)
		{
			__m128i x1 = _mm_loadu_si128((__m128i *)pIn);
			__m128i x2 = _mm_srli_si128(x1, 3);
			__m128i x3 = _mm_and_si128(x2, x_mask);
			__m128 x4 = _mm_cvtepi32_ps(x3);
			x4 = _mm_mul_ps(x4, x6);
			_mm_store_ps(pOut, x4);
		}
	}
#endif

	// finish off row
    for( ; i < inspan; i+=4, pIn += 4 )
		VtConv(pOut++, pIn[band]);

	return pOutTmp;
}

template <>
float* vt::VtConvertSpanARGBTo1BandSSE(float* pOut, const UInt16* pIn,
									   int inspan, int band)
{
    float *pOutTmp = pOut;

	// not really more efficient to implement this in sse
	int i;
	for(i=0; i<inspan-15; i+=16, pIn+=16, pOut+=4)
	{
		VtConv(pOut, pIn[band]);
		VtConv(pOut+1, pIn[band+4]);
		VtConv(pOut+2, pIn[band+8]);
		VtConv(pOut+3, pIn[band+12]);
	}
	for( ; i<inspan; i+=4, pIn+=4, pOut++)
	{
		VtConv(pOut, pIn[band]);
	}

	return pOutTmp;
}

template <>
int vt::VtRGBColorSwapSpanSSE(RGBAType<float>* pDst, const RGBAType<float>* pSrc,
							  int pixcnt)
{
    int i = 0;

#if (defined(_M_IX86) || defined(_M_AMD64))
    if ( g_SupportSSE1() )
    {
        if ( IsAligned16(pSrc) && IsAligned16(pDst) )
        {
            for ( ; i < pixcnt; i++, pSrc++, pDst++ )
            {
                __m128 x0 = _mm_load_ps((float*)pSrc);  // load 4 floats
                x0 = _mm_shuffle_ps(x0, x0, _MM_SHUFFLE(3, 0, 1, 2));
                _mm_store_ps((float*)pDst, x0);
            }
        }
        else
        {
            for ( ; i < pixcnt; i++, pSrc++, pDst++ )
            {
                __m128 x0 = _mm_loadu_ps((float*)pSrc);  // load 4 floats
                x0 = _mm_shuffle_ps(x0, x0, _MM_SHUFFLE(3, 0, 1, 2));
                _mm_storeu_ps((float*)pDst, x0);
            }
        }
    }
#else
    VT_USE(pDst);
	VT_USE(pSrc);
	VT_USE(pixcnt);
#endif
    return i;
}

template <>
int vt::VtRGBColorSwapSpanSSE(RGBAType<UInt16>* pDst, const RGBAType<UInt16>* pSrc,
							  int pixcnt)
{
    int i = 0;

#if (defined(_M_IX86) || defined(_M_AMD64))
    if ( g_SupportSSE2() )
    {
        if ( IsAligned16(pSrc) && IsAligned16(pDst) )
        {
            for ( ; i < pixcnt-1; i+=2, pSrc+=2, pDst+=2 )
            {
                __m128i x0 = _mm_load_si128 ((__m128i*)pSrc);   // load 8 words
                x0 = _mm_shufflelo_epi16(x0, _MM_SHUFFLE(3, 0, 1, 2));
                x0 = _mm_shufflehi_epi16(x0, _MM_SHUFFLE(3, 0, 1, 2));
                _mm_store_si128((__m128i*)pDst, x0);
            }
        }
        else
        {
            for ( ; i < pixcnt-1; i+=2, pSrc+=2, pDst+=2 )
            {
                __m128i x0 = _mm_loadu_si128 ((__m128i*)pSrc);  // load 8 words
                x0 = _mm_shufflelo_epi16(x0, _MM_SHUFFLE(3, 0, 1, 2));
                x0 = _mm_shufflehi_epi16(x0, _MM_SHUFFLE(3, 0, 1, 2));
                _mm_storeu_si128((__m128i*)pDst, x0);
            }
        }
    }
#else
	VT_USE(pDst);
	VT_USE(pSrc);
	VT_USE(pixcnt);
#endif

    return i;
}

template <>
inline int vt::VtRGBColorSwapSpanSSE(RGBAType<HALF_FLOAT>* pDst, 
									 const RGBAType<HALF_FLOAT>* pSrc, int pixcnt)
{
    return VtRGBColorSwapSpanSSE((RGBAType<UInt16>*) pDst, 
								 (const RGBAType<UInt16>*) pSrc, pixcnt);
}

template <>
int vt::VtRGBColorSwapSpanSSE(RGBAType<Byte>* pDst, const RGBAType<Byte>* pSrc,
						  int pixcnt)
{
    int i = 0;

#if (defined(_M_IX86) || defined(_M_AMD64))
    if ( g_SupportSSE2() )
    {
        __m128i x7 = _mm_setzero_si128 ();   // x7 = 0

        for ( ; i < pixcnt-3; i+=4, pSrc+=4, pDst+=4 )
        {
            // load 16 bytes
            __m128i x0 = _mm_loadu_si128 ((__m128i*)pSrc);
            __m128i x1 = x0;

            // swap
            x0 = _mm_unpacklo_epi8(x0, x7);
            x0 = _mm_shufflelo_epi16(x0, _MM_SHUFFLE(3, 0, 1, 2));
            x0 = _mm_shufflehi_epi16(x0, _MM_SHUFFLE(3, 0, 1, 2));
            x1 = _mm_unpackhi_epi8(x1, x7);
            x1 = _mm_shufflelo_epi16(x1, _MM_SHUFFLE(3, 0, 1, 2));
            x1 = _mm_shufflehi_epi16(x1, _MM_SHUFFLE(3, 0, 1, 2));
            x0 = _mm_packus_epi16(x0, x1);

            // store 16 bytes
            _mm_storeu_si128((__m128i*)pDst, x0);
        }
    }
#else
	VT_USE(pDst);
	VT_USE(pSrc);
	VT_USE(pixcnt);
#endif

    return i;
}


//+-----------------------------------------------------------------------------
// RGB to HSL
//------------------------------------------------------------------------------
HRESULT vt::VtConvertImageRGBToHSB(CImg &imgDst, const CImg& imgSrc)
{
    VT_HR_BEGIN();
	
	if (!imgSrc.IsValid())
		VT_HR_EXIT(E_INVALIDSRC);
	
	if (!IsColorImage(imgSrc))
		VT_HR_EXIT(E_INVALIDSRC);
	
	HRESULT createHR = imgDst.Create(imgSrc.Width(), imgSrc.Height(), 
		VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, imgSrc.Bands()));
	if (createHR == E_INVALIDARG)
		createHR = E_INVALIDDST;
	VT_HR_EXIT(createHR);

	CRGBAFloatImg rgbaBuf;
	if (imgSrc.GetType() != OBJ_RGBAFLOATIMG)
		VT_HR_EXIT(rgbaBuf.Create(imgSrc.Width(), 1));

	CFloatImg hsbaBuf;
	if (imgDst.Bands() != 4)
		VT_HR_EXIT(hsbaBuf.Create(imgSrc.Width(), 1, 4));

	for (int y = 0; y < imgDst.Height(); ++y)
    {
		const RGBAFloatPix* ptrRGBA = NULL;
		if (imgSrc.GetType() == OBJ_RGBAFLOATIMG)
		{
			ptrRGBA = reinterpret_cast<const RGBAFloatPix*>(imgSrc.BytePtr(y));
		}
		else
		{
			VT_HR_EXIT(VtConvertSpan(rgbaBuf.BytePtr(), OBJ_RGBAFLOATIMG, 
									 imgSrc.BytePtr(y), imgSrc.GetType(),
									 imgSrc.Width() * imgSrc.Bands()));

			ptrRGBA = rgbaBuf.Ptr();
		}

		if (imgDst.Bands() == 4)
		{
			VtConvertSpanRGB32ToHSB(reinterpret_cast<float*>(imgDst.BytePtr(y)),
				ptrRGBA, imgDst.Width());		
		}
		else
		{
			VtConvertSpanRGB32ToHSB(hsbaBuf.Ptr(), ptrRGBA, hsbaBuf.Width());
			VtConvertSpanMBandTo3Band(reinterpret_cast<float*>(imgDst.BytePtr(y)),
				hsbaBuf.Ptr(), 4 * imgDst.Width(), 1);
		}
    }

	VT_HR_END();
}

float* vt::VtConvertSpanRGB32ToHSB(float* pD, const RGBAFloatPix* pS, 
                                   int pixcnt)
{
    float *pDOrig = pD;

    int x;
    for( x = 0; x < pixcnt; x++, pD+=4, pS++ )
    {
        pD[3] = pS->a; // copy alpha

        float fMax, fMin;
        float fHue;
        float fHueAdj;
        if( pS->b > pS->g && pS->b > pS->r )
        {
            // B Max
            fMax = pS->b;
            fHue = pS->r - pS->g;
            fMin = VtMin(pS->r, pS->g);
            fHueAdj = 240.f;
        }
        else if( pS->g > pS->r )
        {
            // G Max
            fMax = pS->g;
            fHue = pS->b - pS->r;
            fMin = VtMin(pS->r, pS->b);
            fHueAdj = 120.f;
        }
        else
        {
            // R Max
            fMax = pS->r;
            fHue = pS->g - pS->b;
            fMin = VtMin(pS->g, pS->b);
            fHueAdj = (fHue >= 0.f ? 0.f : 360.f);
        }

        float delta = fMax - fMin;
        float fB    = fMax;
        if( fMax == 0.f || delta == 0.f )
        {
            pD[0] = pD[1] = 0;
            pD[2] = fB;
            continue;
        }

        fHue = 60.f*fHue/delta + fHueAdj;
        float fSat = delta / fMax;

        // store
        pD[0] = fHue;
        pD[1] = fSat;
        pD[2] = fB;
    }

    return pDOrig;
}

HRESULT vt::VtConvertImageHSBToRGB(CImg &imgDst, const CImg &imgSrc)
{
	VT_HR_BEGIN();
	
	if (!imgSrc.IsValid())
		VT_HR_EXIT(E_INVALIDSRC);

	if (imgSrc.Bands() != 3 && imgSrc.Bands() != 4)
		VT_HR_EXIT(E_INVALIDSRC);

	if (EL_FORMAT(imgSrc.GetType()) != EL_FORMAT_FLOAT)
		VT_HR_EXIT(E_INVALIDSRC);

	if (PIX_FORMAT(imgSrc.GetType()) != PIX_FORMAT_NONE)
		VT_HR_EXIT(E_INVALIDSRC);

	VT_HR_EXIT(InitDstColor(imgDst, imgSrc));
	
	CFloatImg hsbaBuf;
	if (imgSrc.Bands() != 4)
		VT_HR_EXIT(hsbaBuf.Create(imgSrc.Width(), 1, 4));

	CRGBAFloatImg rgbaBuf;
	if (imgDst.Bands() != 4 || EL_FORMAT(imgDst.GetType()) != EL_FORMAT_FLOAT)
		VT_HR_EXIT(rgbaBuf.Create(imgDst.Width(), 1));

	for (int y = 0; y < imgDst.Height(); ++y)
    {
		const float* ptrHSBA = NULL;
		if (imgSrc.Bands() == 4)
		{
			ptrHSBA = reinterpret_cast<const float*>(imgSrc.BytePtr(y));
		}
		else
		{
			VtConvertSpanRGBToRGBA(reinterpret_cast<RGBAFloatPix*>(hsbaBuf.Ptr()), 
				reinterpret_cast<const RGBFloatPix*>(imgSrc.BytePtr(y)), hsbaBuf.Width());

			ptrHSBA = hsbaBuf.Ptr();
		}

		if (imgDst.Bands() == 4 && EL_FORMAT(imgDst.GetType()) == EL_FORMAT_FLOAT)
		{
			VtConvertSpanHSBToRGB32(reinterpret_cast<RGBAFloatPix*>(imgDst.BytePtr(y)),
				ptrHSBA, imgDst.Width());
		}
		else
		{
			VtConvertSpanHSBToRGB32(rgbaBuf.Ptr(), ptrHSBA, imgDst.Width());
			VT_HR_EXIT(VtConvertSpan(imgDst.BytePtr(y), imgDst.GetType(),
									 rgbaBuf.Ptr(), rgbaBuf.GetType(),
									 imgDst.Width() * 4));
		}
    }

	VT_HR_END();
}

float* vt::VtModifySpanHSB(float* pD, const float* pS, float fHueAdj, 
    float fSatAdj, float fLightAdj, int iSpan)
{
    float* pDOrig = pD;

    // apply the adjustments
    RGBAFloatPix scale(fLightAdj, fSatAdj, 1.f, 1.f);
    RGBAFloatPix offset(0, 0, fHueAdj, 0);

    vt::VtScaleOffsetColorSpan((RGBAFloatPix*)pD, (const RGBAFloatPix*)pS, 
		scale, offset, iSpan);

    // clip hue
    int x;
    for( x = 0; x < iSpan; x++, pD+=4 )
    {
        if( pD[0] > 360.f )
        {
            pD[0] -= 360.f;
        }
        else if ( pD[0] < 0 )
        {
            pD[0] += 360.f;
        }
    }

    return pDOrig;
}

HRESULT vt::VtRGBModifyHSB(CImg& cDst, const CImg& cSrc, float fHueAdj, 
	float fSatAdj, float fBrtAdj)
{
	VT_HR_BEGIN();
	
	if (!cSrc.IsValid())
		VT_HR_EXIT(E_INVALIDSRC);
	
	if (!IsColorImage(cSrc))
		VT_HR_EXIT(E_INVALIDSRC);

	VT_HR_EXIT(InitDstColor(cDst, cSrc));

	CFloatImg hsbaBuf;
	VT_HR_EXIT(hsbaBuf.Create(cSrc.Width(), 1, 4));

	CRGBAFloatImg rgbaBuf;
	if (cSrc.GetType() != OBJ_RGBAFLOATIMG || 
		cDst.GetType() != OBJ_RGBAFLOATIMG)
	{
		VT_HR_EXIT(rgbaBuf.Create(cSrc.Width(), 1));
	}

	for (int y = 0; y < cDst.Height(); ++y)
    {
		const RGBAFloatPix* ptrRGBA = NULL;
		if (cSrc.GetType() == OBJ_RGBAFLOATIMG)
		{
			ptrRGBA = reinterpret_cast<const RGBAFloatPix*>(cSrc.BytePtr(y));
		}
		else
		{
			VtConvertSpan(rgbaBuf.BytePtr(), OBJ_RGBAFLOATIMG,
						  cSrc.BytePtr(y), cSrc.GetType(),
						  cSrc.Width() * cSrc.Bands());

			ptrRGBA = rgbaBuf.Ptr();
		}

		VtConvertSpanRGB32ToHSB(hsbaBuf.Ptr(), ptrRGBA, hsbaBuf.Width());

		VtModifySpanHSB(hsbaBuf.Ptr(), hsbaBuf.Ptr(), fHueAdj, fSatAdj, fBrtAdj,
			            cDst.Width());

		if (cDst.GetType() == OBJ_RGBAFLOATIMG)
		{
			VtConvertSpanHSBToRGB32(reinterpret_cast<RGBAFloatPix*>(cDst.BytePtr(y)),
				                    hsbaBuf.Ptr(), cDst.Width());
		}
		else
		{
			VtConvertSpanHSBToRGB32(reinterpret_cast<RGBAFloatPix*>(rgbaBuf.Ptr()),
				                    hsbaBuf.Ptr(), rgbaBuf.Width());

			VtConvertSpan(cDst.BytePtr(y), cDst.GetType(),
						  rgbaBuf.Ptr(), OBJ_RGBAFLOATIMG, rgbaBuf.Width() * 4);
		}
    }

	VT_HR_END();
}

