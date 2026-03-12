//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     interface to Windows Imaging Components (WIC)
//
//  History:
//      2006/10/25-mattu
//            Created
//
//------------------------------------------------------------------------
#include "stdafx.h"

//#define DEBUG_PRINT_METADATA 1

#define SCANLINES 16

#include "vt_global.h"
#include "wicio.h"

#if defined(VT_WINRT)
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
#endif

using namespace vt;

#if DEBUG_PRINT_METADATA
void PrintMetadata(IWICMetadataQueryReader* pMetadata, const WCHAR* pPad);
#endif

EXTERN_C const GUID DECLSPEC_SELECTANY GUID_MetadataFormatChrominance = 
{0xF73D0DCF, 0xCEC6, 0x4F85, 0x9B, 0x0E, 0x1C, 0x39, 0x56, 0xB1, 0xBE, 0xF7};
EXTERN_C const GUID DECLSPEC_SELECTANY GUID_MetadataFormatLuminance = 
{0x86908007, 0xEDFC, 0x4860, 0x8D, 0x4B, 0x4E, 0xE6, 0xE8, 0x3E, 0x60, 0x58};

//+----------------------------------------------------------------------------
//
// Some useful data structs that map WIC guids to VT types
// 
//----------------------------------------------------------------------------- 
struct WIC_PIXFRMT_MAP
{
    const GUID* pWICPixFrmt;
    int         iMatchType;
    int         iPreferType;
    bool        bRGBToBGR;
};

struct WIC_CNTRFRMT_MAP
{
    const GUID*  pWICContainerFrmt;
    const WCHAR* pExtension;
};

WIC_PIXFRMT_MAP g_WICPixelFormatMap[] =
{
    // add the prefered vision-tools to WIC mappings, so that the search finds
    //   these first
    {&GUID_WICPixelFormat8bppGray,        OBJ_LUMAIMG,      OBJ_LUMAIMG, false},
    {&GUID_WICPixelFormat16bppGray,       OBJ_LUMASHORTIMG, OBJ_LUMASHORTIMG, false},
    {&GUID_WICPixelFormat24bppBGR,        OBJ_RGBIMG,       OBJ_RGBIMG, false},
    {&GUID_WICPixelFormat32bppBGRA,       OBJ_RGBAIMG,      OBJ_RGBAIMG, false},
    {&GUID_WICPixelFormat32bppGrayFloat,  OBJ_LUMAFLOATIMG, OBJ_LUMAFLOATIMG, false},
    {&GUID_WICPixelFormat48bppRGB,        OBJ_RGBSHORTIMG,  OBJ_RGBSHORTIMG,  true},
    {&GUID_WICPixelFormat64bppRGBA,       OBJ_RGBASHORTIMG, OBJ_RGBASHORTIMG, true},
    {&GUID_WICPixelFormat80bpp5Channels,  OBJ_SHORTIMG,     OBJ_SHORTIMG, false},
    {&GUID_WICPixelFormat128bppRGBAFloat, OBJ_RGBAFLOATIMG, OBJ_RGBAFLOATIMG, true},
    {&GUID_WICPixelFormat128bppRGBFloat,  OBJ_RGBAFLOATIMG, OBJ_RGBFLOATIMG,  true},
    {&GUID_WICPixelFormat64bppRGBAHalf,   OBJ_RGBAHALFFLOATIMG, OBJ_RGBAHALFFLOATIMG, true},
    {&GUID_WICPixelFormat64bppRGBHalf,    OBJ_RGBAHALFFLOATIMG, OBJ_RGBHALFFLOATIMG,  true},
    {&GUID_WICPixelFormat48bppRGBHalf,    OBJ_RGBHALFFLOATIMG,  OBJ_RGBHALFFLOATIMG,  true},
    {&GUID_WICPixelFormat16bppGrayHalf,   OBJ_LUMAHALFFLOATIMG, OBJ_LUMAHALFFLOATIMG, false},

    // add others were we can do a simple conversion
    {&GUID_WICPixelFormat24bppRGB,    OBJ_RGBIMG,  OBJ_RGBIMG,  true},
    {&GUID_WICPixelFormat32bppBGR,    OBJ_RGBAIMG, OBJ_RGBIMG,  false},
    {&GUID_WICPixelFormat32bppPBGRA,  OBJ_RGBAIMG, OBJ_RGBAIMG, false},
    {&GUID_WICPixelFormat64bppPRGBA,       OBJ_RGBASHORTIMG, OBJ_RGBASHORTIMG, true},
    {&GUID_WICPixelFormat128bppPRGBAFloat, OBJ_RGBAFLOATIMG, OBJ_RGBAFLOATIMG, true},

    // the following require a WIC convertor
    {&GUID_WICPixelFormat1bppIndexed, OBJ_UNDEFINED, OBJ_RGBAIMG, false}, // indexed formats need an alpha channel since palette can have transparency
    {&GUID_WICPixelFormat2bppIndexed, OBJ_UNDEFINED, OBJ_RGBAIMG, false},
    {&GUID_WICPixelFormat4bppIndexed, OBJ_UNDEFINED, OBJ_RGBAIMG, false},
    {&GUID_WICPixelFormat8bppIndexed, OBJ_UNDEFINED, OBJ_RGBAIMG, false},
    {&GUID_WICPixelFormatBlackWhite,  OBJ_UNDEFINED, OBJ_LUMAIMG,false},
    {&GUID_WICPixelFormat2bppGray,    OBJ_UNDEFINED, OBJ_LUMAIMG,false},
    {&GUID_WICPixelFormat4bppGray,    OBJ_UNDEFINED, OBJ_LUMAIMG,false},
    {&GUID_WICPixelFormat16bppBGR555, OBJ_UNDEFINED, OBJ_RGBIMG, false},
    {&GUID_WICPixelFormat16bppBGR565, OBJ_UNDEFINED, OBJ_RGBIMG, false},
	{&GUID_WICPixelFormat32bppCMYK, OBJ_UNDEFINED, OBJ_RGBIMG,false},
    {&GUID_WICPixelFormat16bppGrayFixedPoint, OBJ_UNDEFINED, OBJ_LUMASHORTIMG, false},
    {&GUID_WICPixelFormat32bppBGR101010,      OBJ_UNDEFINED, OBJ_RGBSHORTIMG,  false},
    {&GUID_WICPixelFormat48bppRGBFixedPoint,  OBJ_UNDEFINED, OBJ_RGBSHORTIMG,  true},
    {&GUID_WICPixelFormat64bppRGBAFixedPoint, OBJ_UNDEFINED, OBJ_RGBASHORTIMG, true},
    {&GUID_WICPixelFormat64bppRGBFixedPoint,  OBJ_UNDEFINED, OBJ_RGBASHORTIMG, true},
    // TODO( add int images): {&GUID_WICPixelFormat96bppRGBFixedPoint, 
    {&GUID_WICPixelFormat96bppRGBFixedPoint,  OBJ_UNDEFINED, OBJ_RGBAFLOATIMG, true},
    // TODO( add int images): {&GUID_WICPixelFormat128bppRGBAFixedPoint,
    {&GUID_WICPixelFormat128bppRGBAFixedPoint,OBJ_UNDEFINED, OBJ_RGBAFLOATIMG, true},
    // TODO( add int images): {&GUID_WICPixelFormat128bppRGBFixedPoint,
    {&GUID_WICPixelFormat128bppRGBFixedPoint, OBJ_UNDEFINED, OBJ_RGBAFLOATIMG, true},
    {&GUID_WICPixelFormat32bppRGBE,   OBJ_UNDEFINED, OBJ_RGBFLOATIMG, false},
    // TODO( add int images): {&GUID_WICPixelFormat32bppGrayFixedPoint,
    {&GUID_WICPixelFormat32bppGrayFixedPoint, OBJ_UNDEFINED, OBJ_LUMAFLOATIMG, false}
};

const UINT g_cPixelFormatElements =
    sizeof(g_WICPixelFormatMap)/sizeof(g_WICPixelFormatMap[0]);

WIC_CNTRFRMT_MAP g_WICContainerFormatMap[] =
{
    {&GUID_ContainerFormatJpeg, L".jpg"},
    {&GUID_ContainerFormatJpeg, L".jpeg"},
    {&GUID_ContainerFormatGif,  L".gif"},
    {&GUID_ContainerFormatTiff, L".tif"},
    {&GUID_ContainerFormatTiff, L".tiff"},
    {&GUID_ContainerFormatPng,  L".png"},
    {&GUID_ContainerFormatBmp,  L".bmp"},
    {&GUID_ContainerFormatWmp,  L".hdp"},
    {&GUID_ContainerFormatWmp,  L".wdp"},
	{&GUID_ContainerFormatWmp,  L".jxr"}
};

const UINT g_cContainerFormatElements =
    sizeof(g_WICContainerFormatMap)/sizeof(g_WICContainerFormatMap[0]);

//+----------------------------------------------------------------------------
//
// Helper functions
// 
//-----------------------------------------------------------------------------
HRESULT 
VtCreatePropVariantFromCParamValue(const CParamValue* pVtVal,
                                   PROPVARIANT &varValue);

static const WIC_PIXFRMT_MAP*
FindPixFrmtMatch(const GUID& guid)
{
    for( UINT i = 0; i < g_cPixelFormatElements; i++ )
    {
        WIC_PIXFRMT_MAP& map = g_WICPixelFormatMap[i];
        if( IsEqualGUID(guid, *map.pWICPixFrmt) )
        {
            return &map;
        }
    }
    return NULL;
}

static const WIC_PIXFRMT_MAP*
FindPixFrmtMatch(const CImgInfo& info)
{
    for( UINT i = 0; i < g_cPixelFormatElements; i++ )
    {
        WIC_PIXFRMT_MAP& map = g_WICPixelFormatMap[i];
		if( VT_IMG_SAME_BE(map.iMatchType, info.type) )
        {
            return &map;
        }
    }

    // special case float RGBX float images
    if( info.Bands() == 3 && EL_FORMAT(info.type) == EL_FORMAT_FLOAT)
    {
        return FindPixFrmtMatch(GUID_WICPixelFormat128bppRGBFloat);
    }

    return NULL;
}

static HRESULT
WICInitProperties(const CParams* pParams, IPropertyBag2* pPropertybag)
{
    HRESULT hr = S_OK;

    ULONG count = 0;

    VT_HR_RET( pPropertybag->CountProperties(&count) );

    for(ULONG i = 0; i < count; i++)
    {
        PROPBAG2 propBag;
        ULONG properties;

        VT_HR_RET( pPropertybag->GetPropertyInfo(i, 1, &propBag, &properties) );

        vt::CParamValue *pVal;
        hr = pParams->GetByName(&pVal, propBag.pstrName, 0);
        if(S_OK == hr)
        {
            PROPVARIANT varValue;
            PropVariantInit(&varValue);
            VT_HR_RET( VtCreatePropVariantFromCParamValue(pVal, varValue) );
            VT_HR_RET( pPropertybag->Write(1, &propBag, (VARIANT*)&varValue) );   
        }
    }

    return S_OK;
}

static HRESULT WICInitializeFactory(IWICImagingFactory** ppFactory)
{
	HRESULT hr = S_OK;
	VtStartCOM();
	if (*ppFactory == NULL)
	{
		hr = CSystem::CoCreateInstance(CLSID_WICImagingFactory, NULL,
			CLSCTX_INPROC_SERVER, IID_PPV_ARGS(ppFactory));
	}
	return hr;
}

static HRESULT WICGetFileExtensions(DWORD componentType, vector<wstring>& fileExtensions)
{
	VT_HR_BEGIN();

	fileExtensions.clear();

	VTComPtr<IWICImagingFactory> pFactory;
	VT_HR_EXIT(WICInitializeFactory(&pFactory));

	VTComPtr<IEnumUnknown> pEnumUnknown;
	VT_HR_EXIT(pFactory->CreateComponentEnumerator(componentType, WICComponentEnumerateDefault, &pEnumUnknown));

	// Enumerate all the encoders or decoders.
	while (hr == S_OK)
	{
		VTComPtr<IUnknown> pUnknown;
		ULONG count = 0;
		VT_HR_EXIT(pEnumUnknown->Next(1, &pUnknown, &count));
		if (count == 1)
		{
			// Get the codec information.
			VTComPtr<IWICBitmapCodecInfo> pCodecInfo;
			VT_HR_EXIT(pUnknown->QueryInterface(&pCodecInfo));

			// See how long the string of file extensions is.
			UINT bufferSize;
			VT_HR_EXIT(pCodecInfo->GetFileExtensions(0, NULL, &bufferSize));

			// Get the comma-separated list of supported file extensions for this codec.
			wstring joinedFileExtensions;
			joinedFileExtensions.resize(bufferSize);
			VT_HR_EXIT(pCodecInfo->GetFileExtensions(bufferSize, joinedFileExtensions.get_buffer(), &bufferSize));

			// Split the list at comma characters.
			WCHAR* fileExtensionStart = joinedFileExtensions.get_buffer();
			while (*fileExtensionStart != L'\0')
			{
				WCHAR* fileExtensionEnd = fileExtensionStart;
				while (*fileExtensionEnd != L',' && *fileExtensionEnd != L'\0')
				{
					fileExtensionEnd++;
				}
				bool isComma = *fileExtensionEnd == L',';
				if (fileExtensionEnd != fileExtensionStart)
				{
					*fileExtensionEnd = L'\0';
					wstring fileExtension;
					fileExtension.assign(fileExtensionStart);
					fileExtensions.push_back(fileExtension);
				}
				fileExtensionStart = fileExtensionEnd + (isComma ? 1 : 0);
			}
		}
	}

	VT_HR_END();
}

//+----------------------------------------------------------------------------
//
// function: CheckWIC
//
//-----------------------------------------------------------------------------
bool
vt::CheckWIC(vt::wstring& errmsg)
{
    IWICImagingFactory* pFactory = NULL;
    HRESULT hr = WICInitializeFactory(&pFactory);

    bool bOk = false;
    if( hr == S_OK && pFactory != NULL )
    {
        pFactory->Release();

        CWicWriter wr;
        CMemStream stream;
        if( (hr = wr.OpenFile(&stream, L".wdp")) == S_OK )
        {
            CRGBAImg imgTest;
            if( (hr = imgTest.Create(32, 32)) == S_OK )
            { 
                imgTest.Clear();
                bOk = ( (hr = wr.SetImage(imgTest, false)) == S_OK );   
            }
        }

        if( bOk == false )
        {
            errmsg.reserve(512);
            errmsg.format(L"Your system seems to have an out-dated version of Windows Imaging Component (hr= %08x).\n", hr);
            errmsg.append(L"Please download the most recent one from: http://www.microsoft.com/downloads\n");   
        }
    }
    else
    {
        errmsg.reserve(512);
        errmsg.format(L"This tool requires Windows Imaging Component to be present on the system.\n");
        errmsg.append(L"Please download it from: http://www.microsoft.com/downloads\n");
    }

    return bOk;
}

struct WIC_ERROR_MAP
{
    LPCWSTR wszWICError;
    HRESULT hr;
};

bool
vt::GetWICErrorString(HRESULT hr, wstring& errmsg)
{
    const static WIC_ERROR_MAP WICErrorMap[] =
    {
        {L"WINCODEC_ERR_GENERIC_ERROR",                    E_FAIL},
        {L"WINCODEC_ERR_INVALIDPARAMETER",                 E_INVALIDARG},
        {L"WINCODEC_ERR_OUTOFMEMORY",                      E_OUTOFMEMORY},
        {L"WINCODEC_ERR_NOTIMPLEMENTED",                   E_NOTIMPL},
        {L"WINCODEC_ERR_ABORTED",                          E_ABORT},
        {L"WINCODEC_ERR_ACCESSDENIED",                     E_ACCESSDENIED},
        {L"WINCODEC_ERR_VALUEOVERFLOW",                    INTSAFE_E_ARITHMETIC_OVERFLOW},
        {L"WINCODEC_ERR_WRONGSTATE",                       MAKE_WINCODECHR_ERR(0xf04)},
        {L"WINCODEC_ERR_VALUEOUTOFRANGE",                  MAKE_WINCODECHR_ERR(0xf05)},
        {L"WINCODEC_ERR_UNKNOWNIMAGEFORMAT",               MAKE_WINCODECHR_ERR(0xf07)},
        {L"WINCODEC_ERR_UNSUPPORTEDVERSION",               MAKE_WINCODECHR_ERR(0xf0B)},
        {L"WINCODEC_ERR_NOTINITIALIZED",                   MAKE_WINCODECHR_ERR(0xf0C)},
        {L"WINCODEC_ERR_ALREADYLOCKED",                    MAKE_WINCODECHR_ERR(0xf0D)},
        {L"WINCODEC_ERR_PROPERTYNOTFOUND",                 MAKE_WINCODECHR_ERR(0xf40)},
        {L"WINCODEC_ERR_PROPERTYNOTSUPPORTED",             MAKE_WINCODECHR_ERR(0xf41)},
        {L"WINCODEC_ERR_PROPERTYSIZE",                     MAKE_WINCODECHR_ERR(0xf42)},
        {L"WINCODEC_ERR_CODECPRESENT",                     MAKE_WINCODECHR_ERR(0xf43)},
        {L"WINCODEC_ERR_CODECNOTHUMBNAIL",                 MAKE_WINCODECHR_ERR(0xf44)},
        {L"WINCODEC_ERR_PALETTEUNAVAILABLE",               MAKE_WINCODECHR_ERR(0xf45)},
        {L"WINCODEC_ERR_CODECTOOMANYSCANLINES",            MAKE_WINCODECHR_ERR(0xf46)},
        {L"WINCODEC_ERR_INTERNALERROR",                    MAKE_WINCODECHR_ERR(0xf48)},
        {L"WINCODEC_ERR_SOURCERECTDOESNOTMATCHDIMENSIONS", MAKE_WINCODECHR_ERR(0xf49)},
        {L"WINCODEC_ERR_COMPONENTNOTFOUND",                MAKE_WINCODECHR_ERR(0xf50)},
        {L"WINCODEC_ERR_IMAGESIZEOUTOFRANGE",              MAKE_WINCODECHR_ERR(0xf51)},
        {L"WINCODEC_ERR_TOOMUCHMETADATA",                  MAKE_WINCODECHR_ERR(0xf52)},
        {L"WINCODEC_ERR_BADIMAGE",                         MAKE_WINCODECHR_ERR(0xf60)},
        {L"WINCODEC_ERR_BADHEADER",                        MAKE_WINCODECHR_ERR(0xf61)},
        {L"WINCODEC_ERR_FRAMEMISSING",                     MAKE_WINCODECHR_ERR(0xf62)},
        {L"WINCODEC_ERR_BADMETADATAHEADER",                MAKE_WINCODECHR_ERR(0xf63)},
        {L"WINCODEC_ERR_BADSTREAMDATA",                    MAKE_WINCODECHR_ERR(0xf70)},
        {L"WINCODEC_ERR_STREAMWRITE",                      MAKE_WINCODECHR_ERR(0xf71)},
        {L"WINCODEC_ERR_STREAMREAD",                       MAKE_WINCODECHR_ERR(0xf72)},
        {L"WINCODEC_ERR_STREAMNOTAVAILABLE",               MAKE_WINCODECHR_ERR(0xf73)},
        {L"WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT",           MAKE_WINCODECHR_ERR(0xf80)},
        {L"WINCODEC_ERR_UNSUPPORTEDOPERATION",             MAKE_WINCODECHR_ERR(0xf81)},
        {L"WINCODEC_ERR_INVALIDREGISTRATION",              MAKE_WINCODECHR_ERR(0xf8A)},
        {L"WINCODEC_ERR_COMPONENTINITIALIZEFAILURE",       MAKE_WINCODECHR_ERR(0xf8B)},
        {L"WINCODEC_ERR_INSUFFICIENTBUFFER",               MAKE_WINCODECHR_ERR(0xf8C)},
        {L"WINCODEC_ERR_DUPLICATEMETADATAPRESENT",         MAKE_WINCODECHR_ERR(0xf8D)},
        {L"WINCODEC_ERR_PROPERTYUNEXPECTEDTYPE",           MAKE_WINCODECHR_ERR(0xf8E)},
        {L"WINCODEC_ERR_UNEXPECTEDSIZE",                   MAKE_WINCODECHR_ERR(0xf8F)},
        {L"WINCODEC_ERR_INVALIDQUERYREQUEST",              MAKE_WINCODECHR_ERR(0xf90)},
        {L"WINCODEC_ERR_UNEXPECTEDMETADATATYPE",           MAKE_WINCODECHR_ERR(0xf91)},
        {L"WINCODEC_ERR_REQUESTONLYVALIDATMETADATAROOT",   MAKE_WINCODECHR_ERR(0xf92)},
        {L"WINCODEC_ERR_INVALIDQUERYCHARACTER",            MAKE_WINCODECHR_ERR(0xf93)},
        {L"WINCODEC_ERR_WIN32ERROR",                       MAKE_WINCODECHR_ERR(0xf94)},
        {L"WINCODEC_ERR_INVALIDPROGRESSIVELEVEL",          MAKE_WINCODECHR_ERR(0xf95)}
    };

    for (int i = 0; i < _countof(WICErrorMap); i++)
    {
        if (WICErrorMap[i].hr == hr)
        {
            errmsg.assign(WICErrorMap[i].wszWICError);
            return true;
        }
    }

    return false;
}

//+----------------------------------------------------------------------------
//
// Function GetWICDecoderFileExtensions
// 
// Synopsis:
//     Fills in a vector of file extensions for which decoders are available.
// 
//-----------------------------------------------------------------------------
HRESULT vt::GetWICDecoderFileExtensions(vt::vector<vt::wstring>& fileExtensions)
{
	return WICGetFileExtensions(WICDecoder, fileExtensions);
}

//+----------------------------------------------------------------------------
//
// Function GetWICEncoderFileExtensions
// 
// Synopsis:
//     Fills in a vector of file extensions for which encoders are available.
// 
//-----------------------------------------------------------------------------
HRESULT vt::GetWICEncoderFileExtensions(vt::vector<vt::wstring>& fileExtensions)
{
	return WICGetFileExtensions(WICEncoder, fileExtensions);
}

//+----------------------------------------------------------------------------
//
// metadata handling
//
//-----------------------------------------------------------------------------

HRESULT SetPropVectorParamValue(CParamValue& p, const PROPVARIANT& val);

HRESULT SetPropVariantParamValue(CParamValue& p, const PROPVARIANT& val)
{
    HRESULT hr;
    if ( val.vt & VT_VECTOR )
    {
        hr = SetPropVectorParamValue(p, val);
    }
    else
    {
        switch (val.vt)
        {
        case VT_UI1:                  // ParamType_Byte
            hr = p.Set(val.bVal);
            break;
        case VT_UI2:                  // ParamType_UShort
            hr = p.Set(val.uiVal);
            break;
        case VT_UI4:                  // ParamType_ULong
            hr = p.Set(val.ulVal);
            break;
        case VT_UI8:                  // ParamType_U8
            hr = p.Set(*(UINT64*)&val.uhVal);
            break;
        case VT_I1:                   // ParamType_SByte
            hr = p.Set(val.cVal);
            break;
        case VT_I2:                   // ParamType_SShort
            hr = p.Set(val.iVal);
            break;
        case VT_I4:                   // ParamType_SLong
            hr = p.Set(val.lVal);
            break;
        case VT_I8:                   // ParamType_I8
            hr = p.Set(*(INT64*)&val.hVal);
            break;
        case VT_R4:                   // ParamType_Float
            hr = p.Set(val.fltVal);
            break;
        case VT_R8:                   // ParamType_Double
            hr = p.Set(val.dblVal);
            break;
        case VT_BOOL:                 // ParamType_Bool
            {
                bool bv = (val.boolVal==0)? false: true;
                hr = p.Set(bv);
            }
            break;
        case VT_BLOB:                 // ParamType_Undefined
            hr = p.SetArray((void*)val.blob.pBlobData, val.blob.cbSize);
            break;
        case VT_LPSTR:                // ParamType_ASCII
            hr = p.Set(val.pszVal);
            break;
        case VT_LPWSTR:               // ParamType_String
            hr = p.Set(val.pwszVal);
            break;
        case VT_CLSID:                // ParamType_GUID
            hr = p.Set(*val.puuid);     
            break;
        default:
			VT_ASSERT(!"Warning: Invalid variant type"); 
            hr = S_FALSE;
            break;
        }
    }
    return hr;
}

HRESULT SetPropVectorParamValue(CParamValue& p, const PROPVARIANT& val)
{
    HRESULT hr;
    switch (val.vt)
    {
    case VT_VECTOR|VT_UI1:                  // ParamType_Byte
        hr = p.SetArray(val.caub.pElems, val.caub.cElems);
        break;
    case VT_VECTOR|VT_UI2:                  // ParamType_UShort
        hr = p.SetArray(val.caui.pElems, val.caui.cElems);
        break;
    case VT_VECTOR|VT_UI4:                  // ParamType_ULong
        hr = p.SetArray(val.caul.pElems, val.caul.cElems);
        break;
    case VT_VECTOR|VT_UI8:                  // ParamType_U8
        hr = p.SetArray((UINT64*)val.cauh.pElems, val.cauh.cElems);
        break;
    case VT_VECTOR|VT_I1:                   // ParamType_SByte
        hr = p.SetArray(val.cac.pElems, val.cac.cElems);
        break;
    case VT_VECTOR|VT_I2:                   // ParamType_SShort
        hr = p.SetArray(val.cai.pElems, val.cai.cElems);
        break;
    case VT_VECTOR|VT_I4:                   // ParamType_SLong
        hr = p.SetArray(val.cal.pElems, val.cal.cElems);
        break;
    case VT_VECTOR|VT_I8:                   // ParamType_I8
        hr = p.SetArray((INT64*)val.cah.pElems, val.cah.cElems);
        break;
    case VT_VECTOR|VT_R4:                   // ParamType_Float
        hr = p.SetArray(val.caflt.pElems, val.caflt.cElems);
        break;
    case VT_VECTOR|VT_R8:                   // ParamType_Double
        hr = p.SetArray(val.cadbl.pElems, val.cadbl.cElems);
        break;
    case VT_VECTOR|VT_CLSID:                // ParamType_GUID
        hr = p.SetArray(val.cauuid.pElems, val.cauuid.cElems);  
        break;
	case VT_VECTOR|VT_LPSTR:				// ParamType_ASCII
		// Note: We ignore all but the first string in a vector of strings.
		// This allows us to read GoPro metadata, which incorrectly reports
		// make, model, and software as vectors of strings.
		hr = p.Set(val.calpstr.cElems > 0 ? val.calpstr.pElems[0] : NULL);
		break;
    default:
        VT_ASSERT(!"Warning: Invalid vector variant type"); 
        hr = S_FALSE;
        break;
    }
    return hr;
}

static HRESULT 
VtCopyWICMetadataProp(LPCWSTR name, UINT id, PROPVARIANT vValue, CParams& params)
{
    HRESULT hr = NOERROR;

    CParamValue val;
	hr = SetPropVariantParamValue(val, vValue);
    if(FAILED(hr))
    {
        return hr;
    }
    if(hr == S_FALSE)
    {
        return S_OK;
    }

	if (name == NULL)
    {
        VT_HR_RET(params.SetById(id, 0, val));
    }
    else
    {
        VT_HR_RET(params.SetByName(name, id, val));
    }

    return hr;
}

static inline HRESULT 
VtCopyWICMetadataIfdTags(const PROPVARIANT &, const PROPVARIANT &rgeltId, 
                         const PROPVARIANT &rgeltValue, CParams& params)
{
    HRESULT hr = NOERROR;
    VT_HR_RET(VtCopyWICMetadataProp(NULL, rgeltId.ulVal, rgeltValue, params));
    return hr;
}

static inline HRESULT 
VtCopyWICMetadataAppXTags(const PROPVARIANT &, const PROPVARIANT &rgeltId, 
                          const PROPVARIANT &rgeltValue, CParams& params)
{
    HRESULT hr = NOERROR;
    VT_HR_EXIT(VtCopyWICMetadataProp(NULL, rgeltId.ulVal, rgeltValue, params));
Exit:
    return hr;
}

static inline HRESULT 
VtCopyWICMetadataThumbnailTags(const PROPVARIANT &, const PROPVARIANT &rgeltId, 
                               const PROPVARIANT &rgeltValue, CParams& params)
{
    HRESULT hr = NOERROR;

    ULONG id = ULONG(-1);
    switch(rgeltId.uiVal)
    {
    case 0:  // not set ThumbnailData
        id = 20507; //ThumbnailData
        break;
    case 259: // Compression
        id = 20515; // ThumbnailCompression
        break;
    case 282: // XResolution
        id = 20525; // ThumbnailResolutionX
        break;
    case 283: // YResolution
        id = 20526; // ThumbnailResolutionY
        break;
    case 296: // ResolutionUnit
        id = 20528;
        break;
    default:
        // Use whatever ls left;

        id = rgeltId.uiVal;
    }

    VT_HR_RET(VtCopyWICMetadataProp(NULL, id, rgeltValue, params));

    return hr;
}

static inline HRESULT 
VtCopyWICMetadataExifTags(const PROPVARIANT &, const PROPVARIANT &rgeltId, 
                          const PROPVARIANT &rgeltValue, CParams& params)
{
    HRESULT hr = NOERROR;
    VT_HR_RET(VtCopyWICMetadataProp(NULL, rgeltId.ulVal, rgeltValue, params));
    return hr;
}

static inline HRESULT 
VtCopyWICMetadataGpsTags(const PROPVARIANT &, const PROPVARIANT &rgeltId, 
                         const PROPVARIANT &rgeltValue, CParams& params)
{
    HRESULT hr = NOERROR;
    VT_HR_RET(VtCopyWICMetadataProp(NULL, rgeltId.ulVal, rgeltValue, params));
    return hr;
}

static inline HRESULT 
VtCopyWICMetadataChunktEXt(const PROPVARIANT &, const PROPVARIANT &rgeltId, 
                           const PROPVARIANT &rgeltValue, CParams& params)
{
    HRESULT hr = NOERROR;
    VT_HR_RET(VtCopyWICMetadataProp(NULL, rgeltId.ulVal, rgeltValue, params));
    return hr;
}

HRESULT 
VtCopyWICMetadataContainer(const PROPVARIANT , IWICMetadataReader* 
                           const pMetadataReader, CParams& params)
{
    HRESULT hr = NOERROR;

    GUID guidFormat;
    VT_HR_RET(pMetadataReader->GetMetadataFormat(&guidFormat));

    PROPVARIANT rgeltSchema;
    PROPVARIANT rgeltId;
    PROPVARIANT rgeltValue;

    PropVariantInit(&rgeltSchema);
    PropVariantInit(&rgeltId);
    PropVariantInit(&rgeltValue);

	UINT nCount = 0;
	VT_HR_RET(pMetadataReader->GetCount(&nCount));
	for (UINT nIndex = 0; nIndex < nCount; nIndex++)
    {
		// If any error occurs, the following code continues with the next metadata value.
		hr = pMetadataReader->GetValueByIndex(nIndex, &rgeltSchema, &rgeltId, &rgeltValue);
		if (SUCCEEDED(hr))
		{
			// Container can contain containers so need to be read recursivly.
			VTComPtr<IWICMetadataReader> pSubMetadataReader;
			if(rgeltValue.vt == VT_UNKNOWN && rgeltValue.punkVal->QueryInterface(&pSubMetadataReader) == S_OK)
			{
				hr = VtCopyWICMetadataContainer(rgeltId, pSubMetadataReader, params);
			}
			else if(IsEqualGUID(GUID_MetadataFormatIfd, guidFormat) || IsEqualGUID(GUID_MetadataFormatSubIfd, guidFormat))  
			{
				hr = VtCopyWICMetadataIfdTags(rgeltSchema, rgeltId, rgeltValue, params);
			}
			else if(IsEqualGUID(GUID_MetadataFormatApp1, guidFormat) || IsEqualGUID(GUID_MetadataFormatApp13, guidFormat))
			{
				hr = VtCopyWICMetadataAppXTags(rgeltSchema, rgeltId, rgeltValue, params);
			}
			else if(IsEqualGUID(GUID_MetadataFormatThumbnail, guidFormat))
			{
				hr = VtCopyWICMetadataThumbnailTags(rgeltSchema, rgeltId, rgeltValue, params);
			}
			else if(IsEqualGUID(GUID_MetadataFormatExif, guidFormat))
			{
				hr = VtCopyWICMetadataExifTags(rgeltSchema, rgeltId, rgeltValue, params);
			}
			else if(IsEqualGUID(GUID_MetadataFormatGps, guidFormat))
			{
				hr = VtCopyWICMetadataGpsTags(rgeltSchema, rgeltId, rgeltValue, params);
			}
			else if(IsEqualGUID(GUID_MetadataFormatChunktEXt, guidFormat))
			{
				hr = VtCopyWICMetadataChunktEXt(rgeltSchema, rgeltId, rgeltValue, params);
			}
			else if(IsEqualGUID(GUID_MetadataFormatChrominance, guidFormat))
			{
				hr = VtCopyWICMetadataProp(NULL, 20625, rgeltValue, params);
			}
			else if(IsEqualGUID(GUID_MetadataFormatLuminance, guidFormat))
			{
				hr = VtCopyWICMetadataProp(NULL, 20624, rgeltValue, params);
			}
			else
			{
				// This is an unsupported container so just skip it.
				hr = NOERROR;
			}

			PropVariantClear(&rgeltSchema);
			PropVariantClear(&rgeltId);
			PropVariantClear(&rgeltValue);
		}
    } 
    return NOERROR;
}

static HRESULT 
VtCopyWICFrameMetadata(IWICBitmapFrameDecode* const pBitmapFrame, CParams& params)
{
    HRESULT hr = NOERROR;

#if DEBUG_PRINT_METADATA
	{
		VTComPtr<IWICMetadataQueryReader> pMetadataQueryReader;
		HRESULT tempHr = pBitmapFrame->GetMetadataQueryReader(&pMetadataQueryReader);
		if (SUCCEEDED(tempHr))
		{
			PrintMetadata(pMetadataQueryReader, L"   ");
		}
	}
#endif

    VTComPtr<IWICMetadataBlockReader> pMetadataBlockReader;
    hr = pBitmapFrame->QueryInterface(&pMetadataBlockReader);
	if(FAILED(hr))
	{
		// If there is no metadata to read we are done.
		return hr == E_NOINTERFACE ? NOERROR : hr;
	}

    UINT count;
    VT_HR_RET(pMetadataBlockReader->GetCount(&count));

    for(UINT idx = 0; idx < count; idx++)
    {
		// Note that we ignore blocks that WIC cannot read, instead of returning a failure code.
        VTComPtr<IWICMetadataReader> pMetadataReader;
		if (SUCCEEDED(pMetadataBlockReader->GetReaderByIndex(idx, &pMetadataReader)))
		{
			PROPVARIANT rgeltEmpty;
			PropVariantInit(&rgeltEmpty);
			hr = VtCopyWICMetadataContainer(rgeltEmpty, pMetadataReader, params);
		}
    }

    return NOERROR;
}

static HRESULT 
VtCopyFromWICMetadata(IWICImagingFactory* const pFactory,
                      IWICBitmapDecoder* const ,
                      IWICBitmapFrameDecode* const pBitmapFrame, 
                      CParams& params)
{
    HRESULT hr = NOERROR;

    // first get the color profile
    UINT uCCCnt;
    HRESULT hr2 = pBitmapFrame->GetColorContexts(0, NULL, &uCCCnt);

    if ( hr2 == S_OK && uCCCnt != 0 )
    {
        vt::vector<VTComPtr<IWICColorContext>> vecCC;
        VT_HR_EXIT( vecCC.resize(uCCCnt) );

        for( UINT i = 0; i < uCCCnt; i++ )
        {
            VT_HR_EXIT( pFactory->CreateColorContext(&vecCC[i]) );
        }

        if ( pBitmapFrame->GetColorContexts(
            uCCCnt, (IWICColorContext**)vecCC.begin(), &uCCCnt) == S_OK )
        {
            for( UINT i = 0; i < uCCCnt; i++ )
            {
                VTComPtr<IWICColorContext>& pCC = vecCC[i];

                WICColorContextType type;
                VT_HR_EXIT( pCC->GetType(&type) );
                if( type != WICColorContextProfile )
                {
                    continue;
                }

                UINT uProfileByteCount; 
                VT_HR_EXIT( pCC->GetProfileBytes(0, NULL, &uProfileByteCount) );
                if( uProfileByteCount == 0  )
                {
                    continue;
                }

                vt::vector<BYTE> vecProfile;
                VT_HR_EXIT( vecProfile.resize(uProfileByteCount) );
                VT_HR_EXIT( pCC->GetProfileBytes(uProfileByteCount, 
                    vecProfile.begin(), &uProfileByteCount) );

                CParamValue val;
                VT_HR_EXIT( val.SetArray(vecProfile.begin(),
                    uProfileByteCount) );
                VT_HR_EXIT( params.SetById(ImagePropertyICCProfile,
                    0, val) );

                // quit after first valid context
                break;
            }
        }
    }

    VT_HR_EXIT(VtCopyWICFrameMetadata(pBitmapFrame, params));

Exit:
    return hr;
}

//+----------------------------------------------------------------------------
//
// helper functions to copy vision tools image to a WIC encoder
//
//-----------------------------------------------------------------------------
static HRESULT
VtGetResolution(const CParams& params, double* pDpiX, double* pDpiY)
{
    double dpiX = 0;
    double dpiY = 0;
    const CParamValue* paramValue;
    if (params.GetById(&paramValue, ImagePropertyXResolution) == S_OK)
    {
        RATIONAL dpiXRational;
        if (paramValue->Get(&dpiXRational) == S_OK)
        {
            dpiX = dpiXRational.AsDouble();
        }
    }
    if (params.GetById(&paramValue, ImagePropertyYResolution) == S_OK)
    {
        RATIONAL dpiYRational;
        if (paramValue->Get(&dpiYRational) == S_OK)
        {
            dpiY = dpiYRational.AsDouble();
        }
    }
    if (dpiX == 0 && dpiY == 0)
    {
        return S_FALSE;
    }

    if (params.GetById(&paramValue, ImagePropertyResolutionUnit) == S_OK)
    {
        USHORT resolutionUnit;
        if (paramValue->Get(&resolutionUnit) == S_OK && resolutionUnit == 3)
        {
            // Convert from pixels per centimeter to pixels per inch.
            dpiX *= 2.54;
            dpiY *= 2.54;
        }
    }

    *pDpiX = dpiX == 0 ? dpiY : dpiX;
    *pDpiY = dpiY == 0 ? dpiX : dpiY;
    return S_OK;
}

#pragma warning(push)
#pragma warning(disable:26036) // can't annotate varValue union string pointers but know it is null terminated already
HRESULT 
VtCreatePropVariantFromCParamValue(const CParamValue* pVtVal,
                                   PROPVARIANT &varValue)
{
	VT_HR_BEGIN()

    PropVariantInit(&varValue);

    switch(pVtVal->GetType())
    {
    case ParamType_None:  
        return E_NOTIMPL;
        break;
    case ParamType_Byte: // the hard-coded entries come from the exif spec
        varValue.vt = VT_UI1;
        break;
    case ParamType_ASCII:
        varValue.vt = VT_LPSTR;
        break;
    case ParamType_UShort:
        varValue.vt = VT_UI2;
        break;
    case ParamType_ULong:
        varValue.vt = VT_UI4;
        break;
    case ParamType_Rational:
        varValue.vt = VT_UI8;
        break;
    case ParamType_SByte:
        varValue.vt = VT_I1;
        break;
    case ParamType_Undefined:
        varValue.vt = VT_BLOB;
        break;
    case ParamType_SShort:
        varValue.vt = VT_I2;
        break;
    case ParamType_SLong:
        varValue.vt = VT_I4;
        break;
    case ParamType_SRational:
        varValue.vt = VT_I8;
        break;
    case ParamType_Float:
        varValue.vt = VT_R4;
        break;
    case ParamType_Double:
        varValue.vt = VT_R8;
        break;
        // These aren't standard types!!!!!
    case ParamType_Bool:
        varValue.vt = VT_BOOL;
        break;
    case ParamType_String:
        varValue.vt = VT_LPWSTR;
        break;
    case ParamType_U8:     // un-signed 64bit int 
        varValue.vt = VT_UI8;
        break;
    case ParamType_I8:     // signed 64bit int
        varValue.vt = VT_I8;
        break;
    case ParamType_GUID:
        varValue.vt = VT_CLSID;
        break;
    case ParamType_Params:
        // No support for nested types just yet.  Should we add it?
        VT_ASSERT(false);  // 
        return E_FAIL;
        break;
    }

    size_t cData = pVtVal->GetItemCount();
    size_t cSize = pVtVal->GetDataSize();
    if(varValue.vt == VT_LPSTR || varValue.vt == VT_LPWSTR)
    {
        varValue.pszVal = (char *)CoTaskMemAlloc(cSize);
		VT_PTR_OOM_EXIT( varValue.pszVal );
        memcpy(varValue.pszVal, pVtVal->GetDataPtr(), cSize); // result will be null terminated since source is
    }
    else if(varValue.vt == VT_BLOB)
    {
        varValue.blob.cbSize    = (ULONG)cSize;
        varValue.blob.pBlobData = (BYTE *)CoTaskMemAlloc(cSize);
		VT_PTR_OOM_EXIT( varValue.blob.pBlobData );
        memcpy(varValue.blob.pBlobData, pVtVal->GetDataPtr(), cSize);
    }
    else if(cData > 1)
    {
        varValue.vt |= VT_VECTOR;
        varValue.cac.cElems = (ULONG)cData;

        varValue.cac.pElems = (char *)CoTaskMemAlloc(cSize);
		VT_PTR_OOM_EXIT( varValue.cac.pElems );
        memcpy(varValue.cac.pElems, pVtVal->GetDataPtr(), cSize);
    }
    else
    {
        memcpy(&varValue.pcVal, pVtVal->GetDataPtr(), cSize);
    }

	VT_HR_END()
}
#pragma warning(pop)

struct VtWICMetadataMapEntry
{
    LPCWSTR szWicMetadataQuery;
    UINT id;
};

//+----------------------------------------------------------------------------
//
// Method: VtMapJpgWICMetadata
// 
// Synopsis:
//     Maps metadata to JFIF (JPEG) schema
// 
//-----------------------------------------------------------------------------
static HRESULT 
VtMapJpgWICMetadata(LPCWSTR pszVtName, UINT uVtId, wstring &name)
{
    static VtWICMetadataMapEntry aJpgWICMetadataMapTable[] = {
        // APP1 IFD GPS
        {L"/app1/ifd/gps/{ushort=0}", 0},		// GpsVersionID = 0x0000
        {L"/app1/ifd/gps/{ushort=1}", 1},		// GpsLatitudeRef = 0x0001
        {L"/app1/ifd/gps/{ushort=2}", 2},		// GpsLatitude = 0x0002
        {L"/app1/ifd/gps/{ushort=3}", 3},		// GpsLongitudeRef = 0x0003
        {L"/app1/ifd/gps/{ushort=4}", 4},		// GpsLongitude = 0x0004
        {L"/app1/ifd/gps/{ushort=5}", 5},		// GpsAltitudeRef = 0x0005
        {L"/app1/ifd/gps/{ushort=6}", 6},		// GpsAltitude = 0x0006
        {L"/app1/ifd/gps/{ushort=7}", 7},		// GpsTimeStamp = 0x0007
        {L"/app1/ifd/gps/{ushort=8}", 8},		// GpsSatellites = 0x0008
        {L"/app1/ifd/gps/{ushort=9}", 9},		// GpsStatus = 0x0009
        {L"/app1/ifd/gps/{ushort=10}", 10},		// GpsMeasureMode = 0x00A
        {L"/app1/ifd/gps/{ushort=11}", 11},		// GpsDOP = 0x000B
        {L"/app1/ifd/gps/{ushort=12}", 12},		// GpsSpeedRef = 0x000C
        {L"/app1/ifd/gps/{ushort=13}", 13},		// GpsSpeed = 0x000D
        {L"/app1/ifd/gps/{ushort=14}", 14},		// GpsTrackRef = 0x000E
        {L"/app1/ifd/gps/{ushort=15}", 15},		// GpsTrack = 0x000F
        {L"/app1/ifd/gps/{ushort=16}", 16},		// GpsImgDirectionRef = 0x0010
        {L"/app1/ifd/gps/{ushort=17}", 17},		// GpsImgDirection = 0x0011
        {L"/app1/ifd/gps/{ushort=18}", 18},		// GpsMapDatum = 0x0012
        {L"/app1/ifd/gps/{ushort=19}", 19},		// GpsDestLatitudeRef = 0x0013
        {L"/app1/ifd/gps/{ushort=20}", 20},		// GpsDestLatitude = 0x0014
        {L"/app1/ifd/gps/{ushort=21}", 21},		// GpsDestLongitudeRef = 0x0015
        {L"/app1/ifd/gps/{ushort=22}", 22},		// GpsDestLongitude = 0x0016
        {L"/app1/ifd/gps/{ushort=23}", 23},		// GpsDestBearingRef = 0x0017
        {L"/app1/ifd/gps/{ushort=24}", 24},		// GpsDestBearing = 0x0018
        {L"/app1/ifd/gps/{ushort=25}", 25},		// GpsDestDistanceRef = 0x0019
        {L"/app1/ifd/gps/{ushort=26}", 26},		// GpsDestDistance = 0x001A
        {L"/app1/ifd/gps/{ushort=27}", 27},		// GpsProcessingMethod = 0x001B
        {L"/app1/ifd/gps/{ushort=28}", 28},		// GpsAreaInformation = 0x001C
        {L"/app1/ifd/gps/{ushort=29}", 29},		// GpsDateStamp = 0x001D
        {L"/app1/ifd/gps/{ushort=30}", 30},		// GpsDifferential = 0x001E
        // APP1 IFD
        {L"/app1/ifd/{ushort=269}", 269},		// DocumentName
        {L"/app1/ifd/{ushort=270}", 270},		// ImageDescription
        {L"/app1/ifd/{ushort=271}", 271},		// Make
        {L"/app1/ifd/{ushort=272}", 272},		// Model
        {L"/app1/ifd/{ushort=282}", 282},		// XResolution
        {L"/app1/ifd/{ushort=283}", 283},		// YResolution
        {L"/app1/ifd/{ushort=285}", 285},		// PageName
        {L"/app1/ifd/{ushort=296}", 296},		// ResolutionUnit
        {L"/app1/ifd/{ushort=297}", 297},		// PageNumber
        {L"/app1/ifd/{ushort=305}", 305},		// Software
        {L"/app1/ifd/{ushort=306}", 306},		// DateTime
        {L"/app1/ifd/{ushort=315}", 315},		// Artist
        {L"/app1/ifd/{ushort=316}", 316},		// HostComputer
        {L"/app1/ifd/{ushort=18246}", 18246},	// SimpleRating
        {L"/app1/ifd/{ushort=18247}", 18247},	// Keywords
        {L"/app1/ifd/{ushort=18248}", 18248},	// WLPGPanoramaInfo
        {L"/app1/ifd/{ushort=18249}", 18249},	// Rating
        {L"/app1/ifd/{ushort=33432}", 33432},	// Copyright
        {L"/app1/ifd/{ushort=40091}", 40091},	// Title
        {L"/app1/ifd/{ushort=40092}", 40092},	// UserComment
        {L"/app1/ifd/{ushort=40093}", 40093},	// Author
        {L"/app1/ifd/{ushort=40094}", 40094},	// Keywords
        {L"/app1/ifd/{ushort=40095}", 40095},	// Subject
    };

    // No string metadata supported
    if(pszVtName != NULL)
    {
        return S_FALSE;
    }

    // We know exactly where know the exact path;
    for(int i = 0; i < _countof(aJpgWICMetadataMapTable); i++)
    {
        if(aJpgWICMetadataMapTable[i].id == uVtId)
        {
            name = aJpgWICMetadataMapTable[i].szWicMetadataQuery;
            return S_OK;
        }
    }

    // Unless is known drop all tags in the first 8 bits.  These
    // are almost always structual to the image content.
    if(uVtId > 200 && uVtId < 32768)
    {
        // Don't use this entry at all black listed
        return S_FALSE;
    }

    // All unknown WMP data goes in EXIF because the WMP writer will
    // create files that the read won't accept.
    return name.format_with_resize(L"/app1/ifd/exif/{ushort=%hu}", uVtId);
}

// IFD.   Global because its shared between TIFF and WMP
static VtWICMetadataMapEntry 
g_aIfdWICMetadataMapTable[] = {
    // IFD GPS
    {L"/ifd/gps/{ushort=0}", 0},		// GpsVersionID = 0x0000
    {L"/ifd/gps/{ushort=1}", 1},		// GpsLatitudeRef = 0x0001
    {L"/ifd/gps/{ushort=2}", 2},		// GpsLatitude = 0x0002
    {L"/ifd/gps/{ushort=3}", 3},		// GpsLongitudeRef = 0x0003
    {L"/ifd/gps/{ushort=4}", 4},		// GpsLongitude = 0x0004
    {L"/ifd/gps/{ushort=5}", 5},		// GpsAltitudeRef = 0x0005
    {L"/ifd/gps/{ushort=6}", 6},		// GpsAltitude = 0x0006
    {L"/ifd/gps/{ushort=7}", 7},		// GpsTimeStamp = 0x0007
    {L"/ifd/gps/{ushort=8}", 8},		// GpsSatellites = 0x0008
    {L"/ifd/gps/{ushort=9}", 9},		// GpsStatus = 0x0009
    {L"/ifd/gps/{ushort=10}", 10},		// GpsMeasureMode = 0x00A
    {L"/ifd/gps/{ushort=11}", 11},		// GpsDOP = 0x000B
    {L"/ifd/gps/{ushort=12}", 12},		// GpsSpeedRef = 0x000C
    {L"/ifd/gps/{ushort=13}", 13},		// GpsSpeed = 0x000D
    {L"/ifd/gps/{ushort=14}", 14},		// GpsTrackRef = 0x000E
    {L"/ifd/gps/{ushort=15}", 15},		// GpsTrack = 0x000F
    {L"/ifd/gps/{ushort=16}", 16},		// GpsImgDirectionRef = 0x0010
    {L"/ifd/gps/{ushort=17}", 17},		// GpsImgDirection = 0x0011
    {L"/ifd/gps/{ushort=18}", 18},		// GpsMapDatum = 0x0012
    {L"/ifd/gps/{ushort=19}", 19},		// GpsDestLatitudeRef = 0x0013
    {L"/ifd/gps/{ushort=20}", 20},		// GpsDestLatitude = 0x0014
    {L"/ifd/gps/{ushort=21}", 21},		// GpsDestLongitudeRef = 0x0015
    {L"/ifd/gps/{ushort=22}", 22},		// GpsDestLongitude = 0x0016
    {L"/ifd/gps/{ushort=23}", 23},		// GpsDestBearingRef = 0x0017
    {L"/ifd/gps/{ushort=24}", 24},		// GpsDestBearing = 0x0018
    {L"/ifd/gps/{ushort=25}", 25},		// GpsDestDistanceRef = 0x0019
    {L"/ifd/gps/{ushort=26}", 26},		// GpsDestDistance = 0x001A
    {L"/ifd/gps/{ushort=27}", 27},		// GpsProcessingMethod = 0x001B
    {L"/ifd/gps/{ushort=28}", 28},		// GpsAreaInformation = 0x001C
    {L"/ifd/gps/{ushort=29}", 29},		// GpsDateStamp = 0x001D
    {L"/ifd/gps/{ushort=30}", 30},		// GpsDifferential = 0x001E
    // IFD
    {L"/ifd/{ushort=269}", 269},		// DocumentName
    {L"/ifd/{ushort=270}", 270},		// ImageDescription
    {L"/ifd/{ushort=271}", 271},		// Make
    {L"/ifd/{ushort=272}", 272},		// Model
    {L"/ifd/{ushort=274}", 274},        // Orientation
    {L"/ifd/{ushort=280}", 280},        // MinSampleValue
    {L"/ifd/{ushort=281}", 281},        // MaxSampleValue
    {L"/ifd/{ushort=282}", 282},		// XResolution
    {L"/ifd/{ushort=283}", 283},		// YResolution
    {L"/ifd/{ushort=285}", 285},		// PageName
    {L"/ifd/{ushort=296}", 296},		// ResolutionUnit
    {L"/ifd/{ushort=297}", 297},		// PageNumber
    {L"/ifd/{ushort=305}", 305},		// Software
    {L"/ifd/{ushort=306}", 306},		// DateTime
    {L"/ifd/{ushort=315}", 315},		// Artist
    {L"/ifd/{ushort=316}", 316},		// HostComputer
    {L"/ifd/{ushort=339}", 339},        // SampleFormat
    {L"/ifd/{ushort=18246}", 18246},	// SimpleRating
    {L"/ifd/{ushort=18247}", 18247},	// Keywords
    {L"/ifd/{ushort=18248}", 18248},	// WLPGPanoramaInfo
    {L"/ifd/{ushort=18249}", 18249},	// Rating
    {L"/ifd/{ushort=33432}", 33432},	// Copyright = 0x8298
    {L"/ifd/{ushort=33550}", 33550},	// ModelPixelScaleTag = 0x830E
    {L"/ifd/{ushort=33920}", 33920},	// IntergraphMatrixTag = 0x8480
    {L"/ifd/{ushort=33922}", 33922},	// ModelTiepointTag = 0x8482
    {L"/ifd/{ushort=34264}", 34264},	// ModelTransformationTag = 0x85D8
    {L"/ifd/{ushort=34675}", 34675},	// InterColorProfile = 0x8773
    {L"/ifd/{ushort=34735}", 34735},	// GeoKeyDirectoryTag = 0x87AF
    {L"/ifd/{ushort=34736}", 34736},	// GeoDoubleParamsTag = 0x87B0
    {L"/ifd/{ushort=34737}", 34737},	// GeoAsciiParamsTag = 0x87B1
    {L"/ifd/{ushort=40091}", 40091},	// Title
    {L"/ifd/{ushort=40092}", 40092},	// UserComment
    {L"/ifd/{ushort=40093}", 40093},	// Author
    {L"/ifd/{ushort=40094}", 40094},	// Keywords
    {L"/ifd/{ushort=40095}", 40095},	// Subject
    // IFD EXIF
    {L"/ifd/exif/{ushort=33434}", 33434}, // ExposureTime = 0x829A, 
    {L"/ifd/exif/{ushort=33437}", 33437}, // FNumber = 0x829D,
    {L"/ifd/exif/{ushort=34850}", 34850}, // ExposureProgram = 0x8822,
    {L"/ifd/exif/{ushort=34855}", 34855}, // IsoSpeedRatings = 0x8827,
    {L"/ifd/exif/{ushort=36864}", 36864}, // ExifVersion = 0x9000,
    {L"/ifd/exif/{ushort=36867}", 36867}, // DateTimeOriginal = 0x9003, 
    {L"/ifd/exif/{ushort=36868}", 36868}, // DateTimeDigitized = 0x9004, 
    {L"/ifd/exif/{ushort=37121}", 37121}, // ComponentsConfiguration = 0x9101, 
    {L"/ifd/exif/{ushort=37122}", 37122}, // CompressedBitsPerPixel = 0x9102, 
    {L"/ifd/exif/{ushort=37377}", 37377}, // ShutterSpeedValue = 0x9201,
    {L"/ifd/exif/{ushort=37378}", 37378}, // ApertureValue = 0x9202,
    {L"/ifd/exif/{ushort=37379}", 37379}, // BrightnessValue = 0x9203,
    {L"/ifd/exif/{ushort=37380}", 37380}, // ExposureBiasValue = 0x9204,
    {L"/ifd/exif/{ushort=37381}", 37381}, // MaxApertureValue = 0x9205,
    {L"/ifd/exif/{ushort=37383}", 37383}, // MeteringMode = 0x9207,
    {L"/ifd/exif/{ushort=37384}", 37384}, // LightSource = 0x9208,
    {L"/ifd/exif/{ushort=37385}", 37385}, // Flash = 0x9209, 
    {L"/ifd/exif/{ushort=37386}", 37386}, // FocalLength = 0x920A,
    {L"/ifd/exif/{ushort=37500}", 37500}, // MakerNote = 0x927C,
    {L"/ifd/exif/{ushort=37510}", 37510}, // UserComment = 0x9286,
    {L"/ifd/exif/{ushort=40960}", 40960}, // FlashPixVersion = 0xA000,
    {L"/ifd/exif/{ushort=40961}", 40961}, // ColorSpace = 0xA001, 
    {L"/ifd/exif/{ushort=40962}", 40962}, // PixelXDimension = 0xA002,
    {L"/ifd/exif/{ushort=40963}", 40963}, // PixelYDimension = 0xA003,
    {L"/ifd/exif/{ushort=40965}", 40965}, // InteroperabilityIFDPointer = 0xA005, 
    {L"/ifd/exif/{ushort=41486}", 41486}, // FocalPlaneXResolution = 0xA20E,
    {L"/ifd/exif/{ushort=41487}", 41487}, // FocalPlaneYResolution = 0xA20F,
    {L"/ifd/exif/{ushort=41488}", 41488}, // FocalPlaneResolutionUnit = 0xA210,
    {L"/ifd/exif/{ushort=41495}", 41495}, // SensingMethod = 0xA217,
    {L"/ifd/exif/{ushort=41728}", 41728}, // FileSource = 0xA300,
    {L"/ifd/exif/{ushort=41729}", 41729}, // SceneType = 0xA301,
    {L"/ifd/exif/{ushort=41985}", 41985}, // CustomRendered = 0xA401,
    {L"/ifd/exif/{ushort=41986}", 41986}, // ExposureMode = 0xA402,
    {L"/ifd/exif/{ushort=41987}", 41987}, // WhiteBalance = 0xA403,
    {L"/ifd/exif/{ushort=41988}", 41988}, // DigitalZomeRatio = 0xA404,
    {L"/ifd/exif/{ushort=41990}", 41990}, // SceneCaptureType = 0xA406,
    {L"/ifd/exif/{ushort=41994}", 41994}, // Sharpness = 0xA40A,
    {L"/ifd/exif/{ushort=41996}", 41996}, // SubjectDistanceRange = 0xA40C,
};

//+----------------------------------------------------------------------------
//
// Method: VtMapTifWICMetadata
// 
// Synopsis:
//     Maps metadata to TIF IFD format
// 
//-----------------------------------------------------------------------------
static HRESULT 
VtMapTifWICMetadata(LPCWSTR pszVtName, UINT uId, wstring &name)
{
    // No string metadata supported by WIC at this point
    if(pszVtName != NULL)
    {
        return S_FALSE;
    }

    // We know exactly where to put the element
    for(int i = 0; i < _countof(g_aIfdWICMetadataMapTable); i++)
    {
        if(g_aIfdWICMetadataMapTable[i].id == uId)
        {
            name = g_aIfdWICMetadataMapTable[i].szWicMetadataQuery;
            return S_OK;
        }
    }

    // Unless its a "known good" drop all tags in the first 15 bits.  These
    // are almost always structual to the TIFF image content.
    if(uId > 200 && uId < 32768)
    {
        return S_FALSE;
    }

    // can take extension IFD's in the main directory
    return name.format_with_resize(L"/ifd/{ushort=%hu}", uId);
}

//+----------------------------------------------------------------------------
//
// Method: VtMapTifWICMetadata
// 
// Synopsis:
//     Maps metadata to WMP (HDPhoto) IFD format
// 
//-----------------------------------------------------------------------------
static HRESULT 
VtMapWmpWICMetadata(LPCWSTR pszVtName, UINT uVtId, wstring &name)
{
    // No string metadata supported
    if(pszVtName != NULL)
    {
        return S_FALSE;
    }

    // We know exactly where know the exact path;
    for(int i = 0; i < _countof(g_aIfdWICMetadataMapTable); i++)
    {
        if(g_aIfdWICMetadataMapTable[i].id == uVtId)
        {
            name = g_aIfdWICMetadataMapTable[i].szWicMetadataQuery;
            return S_OK;
        }
    }

    // Unless its a "known good" drop all tags in the first 15 bits.  These
    // are almost always structual to the TIFF image content.
    if(uVtId > 200 && uVtId < 32768)
    {
        // Don't use this entry at all black listed
        return S_FALSE;
    }

    // All unknown WMP data goes in EXIF because the WMP writer will
    // create files that the read won't read.
    return name.format_with_resize(L"/ifd/exif/{ushort=%hu}", uVtId);
}

//+----------------------------------------------------------------------------
//
// Method: WICCopySafeImageMetadata
// 
// Synopsis:
//     WIC metadata support
// 
//-----------------------------------------------------------------------------
static HRESULT
WICCopySafeImageMetadata(const CParams *pSourceMetadata,
                         IWICImagingFactory* pFactory,
                         IWICBitmapFrameEncode* pFrame)
{
    HRESULT hr = NOERROR;

    if(pSourceMetadata == NULL || pSourceMetadata->Size() == 0)
    {
        // Nothing to copy
        return S_OK;
    }

    // If there is a color profile, apply it to the bitmap frame.
    const CParamValue* pVtVal;
    if (pSourceMetadata->GetById(&pVtVal, ImagePropertyICCProfile) == S_OK &&
        pVtVal != NULL && pVtVal->GetDataPtr() != NULL && pVtVal->GetDataSize() > 0)
    {
        VTComPtr<IWICColorContext> pColorContext;
        if (SUCCEEDED( pFactory->CreateColorContext(&pColorContext) ))
        {
            if (SUCCEEDED( pColorContext->InitializeFromMemory((const BYTE*)pVtVal->GetDataPtr(), (UINT)pVtVal->GetDataSize()) ))
            {
                IWICColorContext* pContext = pColorContext;
                pFrame->SetColorContexts(1, &pContext); // can't operator& non-null VTComPtr
            }
        }
    }

    VTComPtr<IWICMetadataQueryWriter> pWICMetadataQueryWriter;
    hr = pFrame->GetMetadataQueryWriter(&pWICMetadataQueryWriter);
    if(S_OK != hr)
    {
        return S_OK;
    }

    GUID guidContainer;
    VT_HR_RET(pWICMetadataQueryWriter->GetContainerFormat(&guidContainer));

    // If we don't have a contain for the metadata just move on
    if(!(IsEqualGUID(GUID_ContainerFormatWmp, guidContainer) ||
        IsEqualGUID(GUID_ContainerFormatJpeg, guidContainer) ||
        IsEqualGUID(GUID_ContainerFormatTiff, guidContainer)))
    {
        // Maybe we should support GUID_ContainerFormatPng
        return S_OK;
    }

    const WCHAR* pszVtName;
    UINT uVtId;
    UINT uVtIndex;

    vt::PARAM_ENUM_TOKEN vtToken = pSourceMetadata->StartEnum();
    while(pSourceMetadata->NextEnum(vtToken, &pszVtName, &uVtId, &uVtIndex, &pVtVal))
    {
        wstring name;

        if(IsEqualGUID(GUID_ContainerFormatWmp, guidContainer)) 
        {
            hr = VtMapWmpWICMetadata(pszVtName, uVtId, name);
            if(FAILED(hr))
            {
                return hr;
            }
        }
        else if(IsEqualGUID(GUID_ContainerFormatTiff, guidContainer))
        {
            hr = VtMapTifWICMetadata(pszVtName, uVtId, name);
            if(FAILED(hr))
            {
                return hr;
            }
        }
        else if(IsEqualGUID(GUID_ContainerFormatJpeg, guidContainer))
        {
            hr = VtMapJpgWICMetadata(pszVtName, uVtId, name);
            if(FAILED(hr))
            {
                return hr;
            }
        }
        else
        {
            // The following should never happen so get l
            VT_ASSERT(0 && "Unsupported container got through");
            return E_FAIL;
        }

        // If the property isn't mapped don't set it.
        if(hr == S_FALSE)
        {
            hr = S_OK; // reset to S_OK, FIO checks fail S_FALSE
            continue;
        }

        PROPVARIANT varValue;
        PropVariantInit(&varValue);
        // Convert the token to ProVals
        VtCreatePropVariantFromCParamValue(pVtVal, varValue);

        hr = pWICMetadataQueryWriter->SetMetadataByName(name.get_constbuffer(), &varValue);
        VT_ASSERT(S_OK == hr);
        hr = S_OK; // reset to S_OK, FIO checks fail S_FALSE

        PropVariantClear(&varValue);		
    }

    return hr;

}

static HRESULT
WICCopyImage(IImageReader* pReader, const CRect& rctSrc,
             const WIC_PIXFRMT_MAP& map,
             IWICBitmapFrameEncode* pFrame,
             CTaskProgress* pProgress)
{
    HRESULT hr = S_OK;

    CImg imgSubSrc;

    // copy 16 scan-lines at a time
    for (CBlockIterator bi(BLOCKITER_INIT(CRect(rctSrc), rctSrc.Width(), SCANLINES));
        !bi.Done(); bi.Advance())
    {
        CRect rctSubSrc = bi.GetCompRect();
        VT_HR_EXIT( CreateImageForTransform(imgSubSrc, rctSubSrc.Width(), rctSubSrc.Height(),
                                             map.iMatchType) );

        // copy (and possibly convert) scanlines from source to buffer
        VT_HR_EXIT( pReader->ReadRegion(rctSubSrc, imgSubSrc) );

        // do an RGB swap if necessary
        if( map.bRGBToBGR )
        {
            VtRGBColorSwapImage(imgSubSrc);
        }

        // write
        VT_HR_EXIT( pFrame->WritePixels(imgSubSrc.Height(), (UINT) imgSubSrc.StrideBytes(),
                                         imgSubSrc.Height() * (UINT) imgSubSrc.StrideBytes(),
                                         imgSubSrc.BytePtr()) );

        VT_HR_EXIT( CheckTaskProgressCancel(pProgress, 100.f * float(rctSubSrc.bottom) / float(rctSrc.bottom)) );
    }

Exit:
    return hr;
}

static HRESULT
WICTransformCopyImage(IWICImagingFactory* pFactory,
                      IImageReader* pReader, const CRect& rctSrc,
                      const GUID& guidSrc, IWICBitmapFrameEncode* pDst,
                      const GUID& guidDst)
{
    HRESULT hr;
    VTComPtr<IWICFormatConverter> pFormatConvert;
    VTComPtr<IWICBitmap>          pBitmap;
    VTComPtr<IWICPalette>         pPalette;

    const CImgInfo& imgSrc = pReader->GetImgInfo();

    // create a bitmap that wraps the input
    VT_HR_EXIT( pFactory->CreateBitmap(rctSrc.Width(), rctSrc.Height(),
        guidSrc, WICBitmapCacheOnLoad, &pBitmap) );

    // create a color transformer to convert from source format to
    // dest format
    VT_HR_EXIT( pFactory->CreateFormatConverter(&pFormatConvert) );

    if( IsEqualGUID(guidDst, GUID_WICPixelFormat8bppIndexed) )
    {
        // TODO: create palette based on inputs
        WICColor colors[256];
        for( UINT i = 0; i < 256; i++ )
        {
            colors[i] = (0xff<<24)|(i<<16)|(i<<8)|i;
        }

        VT_HR_EXIT( pFactory->CreatePalette(&pPalette) );
        VT_HR_EXIT( pPalette->InitializeCustom(colors, 256) );
    }

    VT_HR_EXIT( pFormatConvert->Initialize(pBitmap, guidDst,
        WICBitmapDitherTypeNone,
        pPalette, 0.0, WICBitmapPaletteTypeCustom) );

#ifdef _DEBUG
	UINT uByteCount = UINT(rctSrc.Width() * imgSrc.PixSize());
#endif
    {
        // copy to the bitmap
        WICRect rcLock = { 0, 0, rctSrc.Width(), rctSrc.Height()};
        VTComPtr<IWICBitmapLock> pLock;
        VT_HR_EXIT( pBitmap->Lock(&rcLock, WICBitmapLockWrite, &pLock) );

        UINT cbStride = 0, cbBufferSize = 0;
        BYTE* pv;
        VT_HR_EXIT( pLock->GetStride(&cbStride) );
        VT_HR_EXIT( pLock->GetDataPointer(&cbBufferSize, &pv) );

#ifdef _DEBUG
        VT_ASSERT( cbStride >= uByteCount );
        VT_ASSERT( cbBufferSize >= uByteCount * rctSrc.Height() );
#endif

        // copy the image
        CImg imgDst;
        VT_HR_EXIT( imgDst.Create(pv, rctSrc.Width(), rctSrc.Height(),
                                  cbStride, imgSrc.type) );
        VT_HR_EXIT( pReader->ReadRegion(rctSrc, imgDst) );

        pLock = NULL; // release the lock

        // copy and transform the result
        VT_HR_EXIT( pDst->WriteSource(pFormatConvert, NULL) );
    }

Exit:
    return hr;
}

//+----------------------------------------------------------------------------
//
// Class CWicReader
// 
// Synopsis:
//     WIC reading routines
// 
//-----------------------------------------------------------------------------
CWicReader::CWicReader()
{
#if defined(MSRVT_WINDOWS_BUILD)
    // Apollo does not support STA/APARTMENTTHREADED
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif
    m_bComStarted = SUCCEEDED(hr);

    m_pBitmapFrame = NULL;
	m_pDecoder     = NULL;
    m_pFactory     = NULL;
#if defined(VT_WINRT)
	m_pBufferByteAccess = NULL;
#endif
}

CWicReader::~CWicReader()
{
    CloseFile();
    if( m_pFactory )
    {
        m_pFactory->Release();
    }
    if( m_bComStarted )
    {
        CoUninitialize();
    }
}

HRESULT
CWicReader::OpenFile( const WCHAR * pszFile )
{
    HRESULT hr;
    CParamValue val;
    VTComPtr<IPropertyStore> pPropertyStore;

    CloseFile( );

	VT_HR_EXIT( WICInitializeFactory(&m_pFactory) );

    VT_HR_EXIT( m_pFactory->CreateDecoderFromFilename(pszFile, NULL, GENERIC_READ, 
		WICDecodeMetadataCacheOnDemand, 
        &m_pDecoder) );

    VT_HR_EXIT( LoadFrame() );

    VT_HR_EXIT( val.Set((PWCHAR)pszFile));
    VT_HR_EXIT( m_params.SetByName(L"Filename", 0, val) );

Exit:
    return hr;
}

#if defined(VT_WINRT)
HRESULT CWicReader::OpenFile(IStorageFile^ storageFile)
{
	VT_HR_BEGIN();

	try
	{
		// Open the file.
		IRandomAccessStream^ stream = CSystem::Await(storageFile->OpenAsync(FileAccessMode::Read));

		VT_HR_EXIT( OpenFile(stream) );

	}
	catch (...)
	{
		hr = E_FAIL;
	}

	VT_HR_EXIT_LABEL();
	return hr;
}

HRESULT CWicReader::OpenFile(IRandomAccessStream^ stream)
{
	VT_HR_BEGIN();

	try
	{
		// Load data from the stream.
		unsigned int streamSize = (unsigned int)stream->Size;
		DataReader^ dataReader = ref new DataReader(stream->GetInputStreamAt(0));
		unsigned int bytesLoaded = CSystem::Await(dataReader->LoadAsync(streamSize));

		// Put the data into a buffer.
		IBuffer^ buffer = dataReader->ReadBuffer(bytesLoaded);

		// Get access to the bytes in the buffer.
		unsigned int bufferLength = buffer->Length;
		VTComPtr<IUnknown> comBuffer(reinterpret_cast<IUnknown*>(buffer));
		VTComPtr<IBufferByteAccess> bufferByteAccess;
		VT_HR_EXIT(comBuffer->QueryInterface(&bufferByteAccess));
		byte* data;
		VT_HR_EXIT(bufferByteAccess->Buffer(&data));

		// Initialize the WIC factory.
		VT_HR_EXIT(WICInitializeFactory(&m_pFactory));

		// Create a WIC stream.
		VTComPtr<IWICStream> wicStream;
		VT_HR_EXIT(m_pFactory->CreateStream(&wicStream));
		VT_HR_EXIT(wicStream->InitializeFromMemory(data, bufferLength));

		// Open the WIC stream.
		VT_HR_EXIT(OpenStream(wicStream));

		// Do not release the buffer byte access until the reader is closed.
		m_pBufferByteAccess = bufferByteAccess.Detach();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	VT_HR_EXIT_LABEL();
	return hr;
}
#endif

HRESULT
CWicReader::OpenStream( IStream *pStream )
{
    HRESULT hr;

    CloseFile( );

	VT_HR_EXIT( WICInitializeFactory(&m_pFactory) );

    VT_HR_EXIT( m_pFactory->CreateDecoderFromStream( 
        pStream, 
        NULL, 
        WICDecodeMetadataCacheOnDemand, 
        &m_pDecoder) );

    VT_HR_EXIT( LoadFrame() );

Exit:
    return hr;
}


HRESULT
CWicReader::CloseFile( )
{
    if( m_pBitmapFrame )
    {
        m_pBitmapFrame->Release();
        m_pBitmapFrame = NULL;
    }
    if( m_pDecoder )
    {
        m_pDecoder->Release();
        m_pDecoder = NULL;
    }
#if defined(VT_WINRT)
	if (m_pBufferByteAccess)
	{
		m_pBufferByteAccess->Release();
		m_pBufferByteAccess = NULL;
	}
#endif

    m_info = CImgInfo();
	return S_OK;
}

HRESULT
CWicReader::GetImage( CImg & imgDst, const CRect* pRect, bool bLoadMetadata )
{
    HRESULT hr;

	CImgReaderWriter<CImg> img;
    
	CRect rctFrame = m_info.Rect();
    CRect rctRequest = pRect? *pRect: rctFrame;
    rctRequest.IntersectRect(&rctRequest, &rctFrame);

    VT_HR_EXIT( CreateImageForTransform(imgDst, rctRequest.Width(), 
		                                rctRequest.Height(), m_info.type) );

	imgDst.Share(img);
    VT_HR_EXIT( LoadPixels(&img, NULL, &rctRequest) );

    if( bLoadMetadata )
    {
        VT_HR_EXIT( imgDst.SetMetaData(&m_params) );
    }

Exit:
    return hr;
}

HRESULT
CWicReader::GetImage( IImageWriter* pWriter, const CPoint* ptDst,
                      const CRect* pRect, bool bLoadMetadata )
{
    HRESULT hr;

    VT_HR_EXIT( LoadPixels(pWriter, ptDst, pRect) );

    if( bLoadMetadata )
    {
        VT_HR_EXIT( pWriter->SetMetaData(&m_params) );
    }

Exit:
    return hr;
}

// check if format is one of the indexed types; if so, then try to load the
// palette and check if it has an alpha channel; used to select between
// allocating and returning RGB vs RGBA, and only return true if it can be
// confirmed that the palette does not have an alpha channel
inline bool FrameIsIndexedAndHasNoAlphaChannel(
    WICPixelFormatGUID& guidFormat,
    IWICImagingFactory* pFactory,
    IWICBitmapFrameDecode* pBitmapFrame)
{
    HRESULT hr;
    if ( IsEqualGUID(GUID_WICPixelFormat1bppIndexed, guidFormat) ||
         IsEqualGUID(GUID_WICPixelFormat2bppIndexed, guidFormat) ||
         IsEqualGUID(GUID_WICPixelFormat4bppIndexed, guidFormat) ||
         IsEqualGUID(GUID_WICPixelFormat8bppIndexed, guidFormat) )
    {
        VTComPtr<IWICPalette> pPalette;
        hr = pFactory->CreatePalette(&pPalette);
        if (hr != S_OK) { return false; }
        hr = pBitmapFrame->CopyPalette(pPalette);
        if (hr == WINCODEC_ERR_PALETTEUNAVAILABLE) { return false; }
        else
        {
            BOOL bHasAlpha = true;
            hr = pPalette->HasAlpha(&bHasAlpha);
            if (hr != S_OK) { return false; }
            if (!bHasAlpha) { return true; }
        }
    }
    return false;
}

HRESULT 
CWicReader::LoadFrame()
{
    HRESULT hr = NOERROR;

	ANALYZE_ASSUME(m_pFactory);
	ANALYZE_ASSUME(m_pDecoder);
    VT_ASSERT( m_pFactory != NULL );
    VT_ASSERT( m_pDecoder != NULL );
    VT_ASSERT( m_pBitmapFrame == NULL );

    VT_HR_EXIT( m_pDecoder->GetFrame(0, &m_pBitmapFrame) );

    UINT uWidth, uHeight;
    VT_HR_EXIT( m_pBitmapFrame->GetSize(&uWidth, &uHeight) );

    WICPixelFormatGUID guidFormat;
    VT_HR_EXIT( m_pBitmapFrame->GetPixelFormat(&guidFormat) );

    const WIC_PIXFRMT_MAP* pmap = FindPixFrmtMatch(guidFormat);
    if( pmap == NULL )
    {
        VT_HR_EXIT( E_NOTIMPL );
    }

    int iType = pmap->iPreferType;
    // special case RGBX formats
    if( IsEqualGUID(GUID_WICPixelFormat128bppRGBFloat, guidFormat) )
    {
        iType = OBJ_RGBFLOATIMG;
    }
    // special case indexed formats to check for alpha channel
    else if ( FrameIsIndexedAndHasNoAlphaChannel(guidFormat, m_pFactory,m_pBitmapFrame) )
    {
        iType = OBJ_RGBIMG;
    }
    m_info = CImgInfo((int)uWidth, (int)uHeight, iType);

    VT_HR_EXIT( VtCopyFromWICMetadata(m_pFactory, m_pDecoder, m_pBitmapFrame, m_params) );

Exit:
    return hr;
}

HRESULT
CWicReader::LoadPixels(IImageWriter* pWriter,
                       const CPoint* ptDst,
                       const CRect* pRect)
{
    HRESULT hr = NOERROR;
    CImg imgSubDst;
    CRect rctFrame;
    CRect rctRequest;

   	ANALYZE_ASSUME(m_pFactory);
	ANALYZE_ASSUME(m_pDecoder);
	ANALYZE_ASSUME(m_pBitmapFrame);
    VT_ASSERT( m_pFactory != NULL );
    VT_ASSERT( m_pDecoder != NULL );
    VT_ASSERT( m_pBitmapFrame != NULL );

    VTComPtr<IWICBitmapSource> pBitmapSource = m_pBitmapFrame;

    WICPixelFormatGUID guidPixFrmt;
    VT_HR_EXIT( m_pBitmapFrame->GetPixelFormat(&guidPixFrmt) );

    const WIC_PIXFRMT_MAP* pMap = FindPixFrmtMatch(guidPixFrmt);
    if( pMap == NULL )
    {  
        VT_HR_EXIT( E_NOTIMPL );
    }

    // create the image
    UINT uWidth, uHeight;
    VT_HR_RET( m_pBitmapFrame->GetSize(&uWidth, &uHeight) );

    rctFrame.SetRect(0, 0, uWidth, uHeight);
    rctRequest = pRect? *pRect: rctFrame;
    rctRequest.IntersectRect(&rctRequest, &rctFrame);

    // if necessary create a color transformer to convert from source
    // format to dest format
    if ( pMap->iMatchType == OBJ_UNDEFINED )
    {
        int iType = pMap->iPreferType;
        // special case indexed formats to check for alpha channel
        if ( FrameIsIndexedAndHasNoAlphaChannel(guidPixFrmt, m_pFactory,m_pBitmapFrame) )
        {
            iType = OBJ_RGBIMG; 
        }
        // convert to the preferred format; let VisionTools convert to the dest format
        CImgInfo imgDst(uWidth, uHeight, iType);

        // find the pixel format for the dst image
        const WIC_PIXFRMT_MAP* pFrmtDst = FindPixFrmtMatch(imgDst);
        if ( pFrmtDst == NULL )
        {
            VT_HR_EXIT( E_NOTIMPL );
        }
        pMap = pFrmtDst;

        VTComPtr<IWICFormatConverter> pFormatConvert;
        VT_HR_EXIT( m_pFactory->CreateFormatConverter(&pFormatConvert) );

        VT_HR_EXIT( pFormatConvert->Initialize(m_pBitmapFrame, 
            *pFrmtDst->pWICPixFrmt,
            WICBitmapDitherTypeNone,
            NULL, 0.0, 
            WICBitmapPaletteTypeCustom) );
        pBitmapSource = pFormatConvert;
    }

    // copy 16 scan-lines at a time
    for (CBlockIterator bi(BLOCKITER_INIT(CRect(rctRequest), rctRequest.Width(), SCANLINES));
        !bi.Done(); bi.Advance())
    {
        const CRect rctSubSrc = bi.GetCompRect();
        const CRect rctSubDst = bi.GetRect() + (ptDst != NULL ? *ptDst : CPoint(0,0));

        VT_HR_EXIT( CreateImageForTransform(imgSubDst, rctSubDst.Width(), rctSubDst.Height(),
                                             pMap->iMatchType) );

        // read
        WICRect rc = { rctSubSrc.left, rctSubSrc.top, rctSubSrc.Width(), rctSubSrc.Height() };
        VT_HR_EXIT( pBitmapSource->CopyPixels(&rc, (UINT) imgSubDst.StrideBytes(),
                                               imgSubDst.Height() * (UINT) imgSubDst.StrideBytes(),
                                               imgSubDst.BytePtr()) );

        // do an RGB swap if necessary
        if( pMap->bRGBToBGR )
        {
            VtRGBColorSwapImage(imgSubDst);
        }

        // copy (and possibly convert) scanlines from buffer to destination
        VT_HR_EXIT( pWriter->WriteRegion(rctSubDst, imgSubDst) );
    }

Exit:
    return hr;
}

//+----------------------------------------------------------------------------
//
// Class CWicWriter
// 
// Synopsis:
//     WIC writing
// 
//-----------------------------------------------------------------------------
CWicWriter::CWicWriter()
{
#if defined(MSRVT_WINDOWS_BUILD)
    // Apollo does not support STA/APARTMENTTHREADED
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif
    m_bComStarted = SUCCEEDED(hr);

	m_pStream  = NULL;
    m_pFactory = NULL;
    m_pEncoder = NULL;
}

CWicWriter::~CWicWriter()
{
    CloseFile();
    if( m_pFactory )
    {
        m_pFactory->Release();
    }
    if( m_bComStarted )
    {
        CoUninitialize();
    }
}

HRESULT
CWicWriter::Clone(IVTImageWriter **ppWriter)
{
    if (ppWriter == NULL)
        return E_INVALIDARG;

    if ((*ppWriter = VT_NOTHROWNEW CWicWriter) == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
CWicWriter::OpenFile( IStream* pStream, const WCHAR * pszFile )
{
    CloseFile( );

	return OpenFileInternal( pStream, pszFile );
}

HRESULT
CWicWriter::OpenFileInternal( IStream* pStream, const WCHAR * pszFile )
{
    HRESULT hr;

    VT_HR_EXIT( m_strFileName.assign(pszFile) );

    const WCHAR* pExt = wcsrchr(m_strFileName, L'.');

    // switch on the filename extension to determine the container guid
    const CLSID* pcontainerguid = NULL;
    for( UINT i = 0; i < g_cContainerFormatElements; i++ )
    {
        if( _wcsicmp(g_WICContainerFormatMap[i].pExtension, pExt) == 0 )
        {
            pcontainerguid = g_WICContainerFormatMap[i].pWICContainerFrmt;
            break;
        }
    }
    if( pcontainerguid == NULL )
    {
        return E_NOTIMPL;
    }

	VT_HR_EXIT( WICInitializeFactory(&m_pFactory) );

    VT_HR_EXIT( m_pFactory->CreateStream(&m_pStream) );

    if( pStream )
    {
        VT_HR_EXIT( m_pStream->InitializeFromIStream(pStream) );
    }
    else
    {
        VT_HR_EXIT( m_pStream->InitializeFromFilename(pszFile, GENERIC_WRITE) );
    }

    // setup the appropriate encoder
    VT_HR_EXIT( m_pFactory->CreateEncoder(*pcontainerguid, NULL, &m_pEncoder) );

    VT_HR_EXIT( m_pEncoder->Initialize(m_pStream, WICBitmapEncoderNoCache) );

Exit:
    if( hr != S_OK )
    {
        CloseFile();
    }

    return hr;
}

HRESULT
CWicWriter::CloseFile( )
{
	HRESULT hr = S_OK;

    if( m_pEncoder )
    {
        // TODO: add a check to was an image set here
        m_pEncoder->Commit();

        m_pEncoder->Release();
        m_pEncoder = NULL;
    }
    if( m_pStream )
    {
        m_pStream->Release();
        m_pStream = NULL;
    }

    m_strFileName.assign(L"");
	return hr;
}

HRESULT
CWicWriter::SetImage( IStream* pStream, CRect& rect, const CParams* pParams )
{
    HRESULT hr;

    VTComPtr<IWICBitmapFrameEncode> pBitmapFrame;
    VTComPtr<IPropertyBag2>         pPropertybag;
    VTComPtr<IWICBitmapDecoder>     pDecoder;
    VTComPtr<IWICBitmapFrameDecode> pFrameDecoder;

    WICRect wrc = {rect.left, rect.top, rect.Width(), rect.Height() };

    VT_HR_EXIT( m_pEncoder->CreateNewFrame(&pBitmapFrame, &pPropertybag) );

    if( pParams )
    {
        VT_HR_EXIT( WICInitProperties(pParams, pPropertybag) );
    }

    VT_HR_EXIT( pBitmapFrame->Initialize(pPropertybag) );

    // create a decoder from the stream
    VT_HR_EXIT( m_pFactory->CreateDecoderFromStream(pStream, NULL, 
        WICDecodeMetadataCacheOnDemand, &pDecoder) );

    VT_HR_EXIT( pDecoder->GetFrame(0, &pFrameDecoder) );

    // call the encoder on the bitmap frame and the rect
    WICPixelFormatGUID guidFormatSrc;
    VT_HR_EXIT( pFrameDecoder->GetPixelFormat(&guidFormatSrc) );
    double dpiX, dpiY;
    VT_HR_EXIT( pFrameDecoder->GetResolution(&dpiX, &dpiY) );

    VT_HR_EXIT( pBitmapFrame->SetSize(wrc.Width, wrc.Height) );
    VT_HR_EXIT( pBitmapFrame->SetResolution(dpiX, dpiY) );
    VT_HR_EXIT( pBitmapFrame->SetPixelFormat(&guidFormatSrc) );
    VT_HR_EXIT( pBitmapFrame->WriteSource(pFrameDecoder, &wrc) );

    VT_HR_EXIT( pBitmapFrame->Commit() );

Exit:
    return hr;
}

HRESULT
CWicWriter::SetImage(const CImg & imgSrc, bool bSaveMetadata,
                     const CParams* pParams,
                     CTaskProgress* pProgress)
{
    CImgReaderWriter<CImg> img;
	imgSrc.Share(img, NULL, true);
    return SetImage(&img, NULL, bSaveMetadata, pParams, pProgress);
}

HRESULT
CWicWriter::SetImage(IImageReader* pReader, const CRect* pRect,
                     bool bSaveMetadata, const CParams* pParams,
                     CTaskProgress* pProgress)
{
    HRESULT hr;

    VTComPtr<IWICBitmapFrameEncode> pBitmapFrame;
    VTComPtr<IPropertyBag2>         pPropertybag;

    GUID guidFormatSrc, guidFormatDst;

    const CImgInfo& imgSrc = pReader->GetImgInfo();
    const CRect rctSrc = pRect == NULL ? imgSrc.Rect() :
        (*pRect & imgSrc.Rect());

    CParams params;
    VT_HR_EXIT( pReader->GetMetaData(params) );

    VT_HR_EXIT( m_pEncoder->CreateNewFrame(&pBitmapFrame, &pPropertybag) );

    if( pParams )
    {
        VT_HR_EXIT( WICInitProperties(pParams, pPropertybag) );
    }
    else
    {
        VT_HR_EXIT( WICInitProperties(&params, pPropertybag) );
    }

    VT_HR_EXIT( pBitmapFrame->Initialize(pPropertybag) );

	// Note that WIC's encoder for JPEG (but not TIFF or HD Photo) requires
	// that the metadata be set *before* copying pixels.
	if( bSaveMetadata )
	{
		VT_HR_EXIT( WICCopySafeImageMetadata(&params, m_pFactory, pBitmapFrame) );
	}

    // search for the best match pixel format
    const WIC_PIXFRMT_MAP* pmap = FindPixFrmtMatch(imgSrc);
    if( pmap == NULL )
    {
        // TODO: handle int images
        VT_HR_EXIT( E_NOTIMPL );
    }

    guidFormatSrc = guidFormatDst = *pmap->pWICPixFrmt;
    VT_HR_EXIT( pBitmapFrame->SetPixelFormat(&guidFormatDst) );

    // the call above may have changed the pixel format so search the map
    // again
    pmap = FindPixFrmtMatch(guidFormatDst);

    VT_HR_EXIT( pBitmapFrame->SetSize(rctSrc.Width(), rctSrc.Height()) );

    // Set the resolution if we have it.
    double dpiX, dpiY;
    if (VtGetResolution(params, &dpiX, &dpiY) == S_OK)
    {
        VT_HR_EXIT( pBitmapFrame->SetResolution(dpiX, dpiY) );
    }

    // write the frame
    if( pmap == NULL || pmap->iMatchType == OBJ_UNDEFINED )
    {
        // use the WIC color transformer
        // TODO: Report progress.
        VT_HR_EXIT( WICTransformCopyImage(m_pFactory, pReader, rctSrc,
            guidFormatSrc, pBitmapFrame, guidFormatDst) );
    }
    else
    {
        // no need to transform
        VT_HR_EXIT( WICCopyImage(pReader, rctSrc, *pmap, pBitmapFrame, pProgress) );
    }

    VT_HR_EXIT( pBitmapFrame->Commit() );

Exit:
    return hr;
}


//+----------------------------------------------------------------------------
//
// Class CWicMetadataReader
// 
//-----------------------------------------------------------------------------
typedef struct {
    UInt16 ID;
    UInt16 Version;
    UInt32 IFDOffset;
} TiffHeader;

#define VT_TIFF_MAGIC      0x4949
#define VT_TIFF_BIG_ENDIAN 0x4D4D
#define VT_TIFF_VERSION    0x002A

CWicMetadataReader::CWicMetadataReader()
{
#if defined(MSRVT_WINDOWS_BUILD)
    // Apollo does not support STA/APARTMENTTHREADED
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif
    m_bComStarted = SUCCEEDED(hr);

    m_pFactory = NULL;
    m_pStream = NULL;
}

CWicMetadataReader::~CWicMetadataReader()
{
    if (m_pFactory != NULL)
        m_pFactory->Release();

    if (m_bComStarted)
        CoUninitialize();
}

HRESULT
CWicMetadataReader::OpenStream(IStream* pStream)
{
    VT_HR_BEGIN()

    CloseStream();

    if (m_pFactory == NULL)
    {
        VT_HR_EXIT( CSystem::CoCreateInstance(CLSID_WICImagingFactory,
                                     NULL, CLSCTX_INPROC_SERVER,
									 IID_PPV_ARGS(&m_pFactory)));
    }       

    VT_HR_EXIT( m_pFactory->CreateStream(&m_pStream) );

    VT_HR_EXIT( m_pStream->InitializeFromIStream(pStream) );

    VT_HR_EXIT( LoadMetadata() );

    VT_HR_END()
}

VOID
CWicMetadataReader::CloseStream()
{
    m_pStream = NULL;
    m_params.DeleteAll();
}

HRESULT
CWicMetadataReader::GetMetadata(const CParams** ppParams)
{
    VT_HR_BEGIN()

    VT_PTR_EXIT( ppParams );

    *ppParams = &m_params;

    VT_HR_END()
}

HRESULT
CWicMetadataReader::LoadMetadata()
{
    VT_HR_BEGIN()

    VT_ASSERT( m_pStream != NULL );
    ANALYZE_ASSUME( m_pStream != NULL );

    IWICMetadataReader* pReader = NULL;

    LARGE_INTEGER liZero;

    // read the offset of IFD0 from the tiff header
    TiffHeader hdr;
    VT_HR_EXIT( m_pStream->Read(&hdr, sizeof(TiffHeader), NULL) );
    if (hdr.ID != VT_TIFF_MAGIC && hdr.ID != VT_TIFF_BIG_ENDIAN)
        VT_HR_EXIT( E_INVALIDARG );
    if (hdr.ID == VT_TIFF_BIG_ENDIAN)
        hdr.Version = (hdr.Version >> 8) | (hdr.Version << 8);
    if (hdr.Version != VT_TIFF_VERSION)
        VT_HR_EXIT( E_INVALIDARG ) ;
    if (hdr.ID == VT_TIFF_BIG_ENDIAN)
        hdr.IFDOffset = (hdr.IFDOffset >> 24) | ((hdr.IFDOffset & 0xff0000) >> 8) |
                        ((hdr.IFDOffset & 0xff00) << 8) | (hdr.IFDOffset << 24);

    // seek to IFD0
    liZero.QuadPart = hdr.IFDOffset;
    VT_HR_EXIT( m_pStream->Seek(liZero, 0, NULL) );

    // Create a metadata reader
    VT_HR_EXIT( m_pFactory->CreateMetadataReader(
        GUID_MetadataFormatIfd, NULL, hdr.ID == VT_TIFF_BIG_ENDIAN ?
        WICPersistOptionBigEndian : WICMetadataCreationDefault,
        m_pStream, &pReader) );

    PROPVARIANT rgeltEmpty;
    PropVariantInit(&rgeltEmpty);
    VT_HR_RET( VtCopyWICMetadataContainer(rgeltEmpty, pReader, m_params) );

    VT_HR_END()
}

//+----------------------------------------------------------------------------
//
// Class CWicMetadataWriter
// 
//-----------------------------------------------------------------------------
CWicMetadataWriter::CWicMetadataWriter()
{
#if defined(MSRVT_WINDOWS_BUILD)
    // Apollo does not support STA/APARTMENTTHREADED
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif
    m_bComStarted = SUCCEEDED(hr);

    m_pFactory = NULL;
    m_pStream = NULL;
}

CWicMetadataWriter::~CWicMetadataWriter()
{
    if (m_pFactory != NULL)
        m_pFactory->Release();

    if (m_bComStarted)
        CoUninitialize();
}

HRESULT
CWicMetadataWriter::OpenStream(IStream* pStream)
{
    VT_HR_BEGIN()

    CloseStream();

    VT_PTR_EXIT( pStream );

    if( m_pFactory == NULL )
    {
        VT_HR_EXIT( CSystem::CoCreateInstance(CLSID_WICImagingFactory,
                                    NULL, CLSCTX_INPROC_SERVER,
									IID_PPV_ARGS(&m_pFactory)));
    }       

    m_pStream = pStream;

    VT_HR_END()
}

VOID
CWicMetadataWriter::CloseStream()
{
    m_pStream = NULL;
    m_params.DeleteAll();
}

HRESULT
CWicMetadataWriter::SetMetadata(const CParams* pParams)
{
    VT_HR_BEGIN()

    VT_PTR_EXIT( pParams );

    if (m_pStream == NULL)
        VT_HR_EXIT( E_NOINIT );

    m_params = *pParams;

    VT_HR_EXIT( SaveMetadata() );

    VT_HR_END()
}

HRESULT
CWicMetadataWriter::SaveMetadata()
{
    VT_HR_BEGIN()

    VT_ASSERT( m_pStream != NULL );
    ANALYZE_ASSUME( m_pStream != NULL );

    TiffHeader hdr;
    hdr.ID = VT_TIFF_MAGIC;
    hdr.Version = VT_TIFF_VERSION;
    hdr.IFDOffset = sizeof(TiffHeader);

    VT_HR_EXIT( m_pStream->Write(&hdr, sizeof(hdr), NULL) );

    // Create a metadata writer
    IWICMetadataWriter* pWriter = NULL;
    VT_HR_EXIT( m_pFactory->CreateMetadataWriter(
        GUID_MetadataFormatIfd, NULL, WICMetadataCreationDefault, 
        &pWriter) );
    
    // copy the metadata via WIC             
    const WCHAR* pszVtName;
    UINT uVtId;
    UINT uVtIndex;
    const CParamValue* pVtVal;
    PARAM_ENUM_TOKEN vtToken = m_params.StartEnum();    
    while (m_params.NextEnum(vtToken, &pszVtName, &uVtId, &uVtIndex, &pVtVal))
    {
        PROPVARIANT varId, varValue;
        PropVariantInit(&varId);
        PropVariantInit(&varValue);
    
        varId.vt = VT_UI4;
        varId.uintVal = uVtId;
    
        // Convert the tokens to PropVals
        if (VtCreatePropVariantFromCParamValue(pVtVal, varValue) != S_OK)
            continue;
    
        if (pWriter->SetValue(NULL, &varId, &varValue) != S_OK)
            continue;
    }
    
    VTComPtr<IWICPersistStream> pWICStream;
    VT_HR_EXIT( pWriter->QueryInterface(
        IID_PPV_ARGS(&pWICStream)));
    
    VT_HR_EXIT( pWICStream->SaveEx(m_pStream, WICPersistOptionDefault, true) );

    VT_HR_END()
}


//+-----------------------------------------------------------------------------
//
// Debug Utilities
// 
//------------------------------------------------------------------------------

#if DEBUG_PRINT_METADATA
void PrintMetadata(IWICMetadataQueryReader* pMetadata, const WCHAR* pPad)
{
    VTComPtr<IEnumString> pEnum;

    if( pMetadata->GetEnumerator(&pEnum) == S_OK )
    {
        LPOLESTR pStr;
        while( pEnum->Next(1, &pStr, NULL) == S_OK )
        {
            if( pStr )
            {
                PROPVARIANT value;
                PropVariantClear(&value);
                HRESULT hr = pMetadata->GetMetadataByName(pStr, &value);
                if( value.vt == VT_UNKNOWN )
                {
                    VT_DPF(("%S%S BEGIN ENUM\n", pPad, pStr));

                    VTComPtr<IWICMetadataQueryReader> pMetaNested;
                    value.punkVal->QueryInterface(IID_IWICMetadataQueryReader,
                        (void**)&pMetaNested);
                    if( pMetaNested )
                    {
                        vt::wstring_b<1024> nestpad;
                        nestpad.format(L"    %s%s",pPad,pStr);
                        PrintMetadata(pMetaNested, nestpad);
                    }

                    VT_DPF(("%S%S DONE ENUM\n", pPad, pStr));
                }
                else if( value.vt == VT_LPWSTR )
                {
                    VT_DPF(("%S%S, LPWSTR= %S\n", pPad, pStr, value.pwszVal));
                }
                else
                {
                    VT_DPF(("%S%S, type = %d\n", pPad, pStr, value.vt));
                }
            }
        }
    }
}
#endif

