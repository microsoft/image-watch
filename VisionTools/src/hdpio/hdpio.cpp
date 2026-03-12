////////////////////////////////////////////////////////////////////////////////
//
//  hdpio.cpp: hd photo interface functions 
//
#include "stdafx.h"

#include "hdpstream.hpp"
#include "strcodec.h"

using namespace vt;

#define FailedDPK(err) ((err)<0)
#define CallDPK(exp) do { \
    if (FailedDPK(dpkerr = (exp))) {\
		hr = E_FAIL; \
        goto Exit; \
    } \
} while(0)

//+----------------------------------------------------------------------------
//
// Some useful data structs that map HDPhoto guids to VT types
//   TODO: this data struct is useful to both WIC and HDPhoto
// 
//----------------------------------------------------------------------------- 
struct HDP_PIXFRMT_MAP
{
    const GUID* pHDPPixFrmt;
    int         iMatchType;
    int         iPreferType;
    bool        bRGBToBGR;
	bool        bHasAlpha;
	int         iSrcBitsPerPixel;
};

struct HDP_CNTRFRMT_MAP
{
    const GUID*  pHDPContainerFrmt;
    const WCHAR* pExtension;
};

HDP_PIXFRMT_MAP g_HDPPixelFormatMap[] =
{
	// add the prefered vision-tools to HDP mappings, so that the search finds
	//   these first
    {&GUID_PKPixelFormat8bppGray,    OBJ_BYTEIMG,   OBJ_BYTEIMG, false, false, 8},
    {&GUID_PKPixelFormat16bppGray,   OBJ_SHORTIMG,  OBJ_SHORTIMG,false, false, 16},
    {&GUID_PKPixelFormat24bppBGR,    OBJ_RGBIMG,    OBJ_RGBIMG,  false, false, 24},
    {&GUID_PKPixelFormat32bppBGRA,   OBJ_RGBAIMG,   OBJ_RGBAIMG, false, true, 32},
    {&GUID_PKPixelFormat32bppGrayFloat,  OBJ_FLOATIMG,     OBJ_FLOATIMG,     false, false, 32},
    {&GUID_PKPixelFormat48bppRGB,        OBJ_RGBSHORTIMG,  OBJ_RGBSHORTIMG,  true, false, 48},
    {&GUID_PKPixelFormat64bppRGBA,       OBJ_RGBASHORTIMG, OBJ_RGBASHORTIMG, true, true, 64},
	{&GUID_PKPixelFormat96bppRGBFloat,   OBJ_RGBFLOATIMG,  OBJ_RGBFLOATIMG,  true, false, 96},
    {&GUID_PKPixelFormat128bppRGBAFloat, OBJ_RGBAFLOATIMG, OBJ_RGBAFLOATIMG, true, true, 128},

	// add others were we can do a simple conversion
    {&GUID_PKPixelFormat24bppRGB,    OBJ_RGBIMG,  OBJ_RGBIMG, true, false, 24},
	{&GUID_PKPixelFormat32bppBGR,    OBJ_RGBAIMG, OBJ_RGBIMG, false, false, 32},
    {&GUID_PKPixelFormat32bppPBGRA,  OBJ_RGBAIMG, OBJ_RGBAIMG,false, true, 32},
    {&GUID_PKPixelFormat64bppPRGBA,       OBJ_RGBASHORTIMG, OBJ_RGBASHORTIMG, true, true, 64},
	{&GUID_PKPixelFormat128bppPRGBAFloat, OBJ_RGBAFLOATIMG, OBJ_RGBAFLOATIMG, true, true, 128},
	{&GUID_PKPixelFormat128bppRGBFloat,   OBJ_RGBAFLOATIMG, OBJ_RGBFLOATIMG,  true, false, 128},

    {&GUID_PKPixelFormat16bppGrayHalf,  OBJ_HALFFLOATIMG, OBJ_HALFFLOATIMG, false, false, 16},
    {&GUID_PKPixelFormat64bppRGBAHalf,  OBJ_RGBAHALFFLOATIMG, OBJ_RGBAHALFFLOATIMG, true, true,  64},
    {&GUID_PKPixelFormat64bppRGBHalf,   OBJ_RGBAHALFFLOATIMG, OBJ_RGBHALFFLOATIMG,  true, false, 64},
    {&GUID_PKPixelFormat48bppRGBHalf,   OBJ_RGBHALFFLOATIMG,  OBJ_RGBHALFFLOATIMG,  true, false, 48},

	// the following require a PK convertor
	// TODO: support these at some point if they seem important
	//{&GUID_PKPixelFormat1bppIndexed, OBJ_UNDEFINED, OBJ_RGBAIMG, false, false, 1},
    //{&GUID_PKPixelFormat2bppIndexed, OBJ_UNDEFINED, OBJ_RGBAIMG, false, false, 2},
    //{&GUID_PKPixelFormat4bppIndexed, OBJ_UNDEFINED, OBJ_RGBAIMG, false, false, 4},
    //{&GUID_PKPixelFormat8bppIndexed, OBJ_UNDEFINED, OBJ_RGBAIMG, false, false, 8},
    {&GUID_PKPixelFormatBlackWhite,  OBJ_UNDEFINED, OBJ_BYTEIMG, false, false, 1},
    //{&GUID_PKPixelFormat2bppGray,    OBJ_UNDEFINED, OBJ_BYTEIMG, false, false, 2},
    //{&GUID_PKPixelFormat4bppGray,    OBJ_UNDEFINED, OBJ_BYTEIMG, false, false, 4},
    {&GUID_PKPixelFormat16bppRGB555, OBJ_UNDEFINED, OBJ_RGBIMG, true, false, 16},
    {&GUID_PKPixelFormat16bppRGB565, OBJ_UNDEFINED, OBJ_RGBIMG, true, false, 16},
	{&GUID_PKPixelFormat16bppGrayFixedPoint, OBJ_UNDEFINED, OBJ_FLOATIMG,     false, false, 16},
    {&GUID_PKPixelFormat32bppRGB101010,      OBJ_UNDEFINED, OBJ_RGBSHORTIMG,  true, false, 32},
	{&GUID_PKPixelFormat48bppRGBFixedPoint,  OBJ_UNDEFINED, OBJ_RGBFLOATIMG,  true, false, 48},
	{&GUID_PKPixelFormat64bppRGBAFixedPoint, OBJ_UNDEFINED, OBJ_RGBAFLOATIMG, true, true, 64},
	{&GUID_PKPixelFormat64bppRGBFixedPoint,  OBJ_UNDEFINED, OBJ_RGBFLOATIMG,  true, false, 64},
    {&GUID_PKPixelFormat96bppRGBFixedPoint,  OBJ_UNDEFINED, OBJ_RGBFLOATIMG,  true, false, 96},
    {&GUID_PKPixelFormat128bppRGBAFixedPoint,OBJ_UNDEFINED, OBJ_RGBAFLOATIMG, true, true, 128},
    {&GUID_PKPixelFormat128bppRGBFixedPoint, OBJ_UNDEFINED, OBJ_RGBFLOATIMG,  true, false, 128},
    {&GUID_PKPixelFormat32bppRGBE,           OBJ_UNDEFINED, OBJ_RGBFLOATIMG,  true, false, 32},
    {&GUID_PKPixelFormat32bppGrayFixedPoint, OBJ_UNDEFINED, OBJ_FLOATIMG,     false, false, 32}
};

const UINT g_cPixelFormatElements =
    sizeof(g_HDPPixelFormatMap)/sizeof(g_HDPPixelFormatMap[0]);

//+----------------------------------------------------------------------------
//
// Helper functions
// 
//-----------------------------------------------------------------------------

static const HDP_PIXFRMT_MAP*
FindPixFrmtMatch(const GUID& guid)
{
    for( UINT i = 0; i < g_cPixelFormatElements; i++ )
    {
        HDP_PIXFRMT_MAP& map = g_HDPPixelFormatMap[i];
        if( IsEqualGUID(guid, *map.pHDPPixFrmt) )
        {
            return &map;
        }
    }
    return NULL;
}

static const HDP_PIXFRMT_MAP*
FindPixFrmtMatch(int iType, bool bRGBOrder = false)
{
    // special case float RGBX float images
    if( VT_IMG_BANDS( iType ) == 3 && EL_FORMAT( iType ) == EL_FORMAT_FLOAT)
    {
        return FindPixFrmtMatch(GUID_PKPixelFormat128bppRGBFloat);
    }

    for( UINT i = 0; i < g_cPixelFormatElements; i++ )
    {
        const HDP_PIXFRMT_MAP& map = g_HDPPixelFormatMap[i];
        if( VT_IMG_SAME_BE(map.iMatchType, iType) )
        {
			if( map.bRGBToBGR != bRGBOrder)
			{
				// if the first match doesn't share the exact same order then
				// search for a subsequent one that does
				// 
				for( UINT j = i+1; j < g_cPixelFormatElements; j++ )
				{
					const HDP_PIXFRMT_MAP& map2 = g_HDPPixelFormatMap[j];
					if( VT_IMG_SAME_BE(map2.iMatchType, iType) &&
						map2.bRGBToBGR == bRGBOrder )
					{
						return &map2;
					}
                }
            }
			return &map;
        }
    }

    return NULL;
}

static HRESULT 
AddHDPhotoProperty(UINT uId, const DPKPROPVARIANT& prop, CParams& params)
{
	HRESULT hr = S_FALSE;

	CParamValue val;
	switch (prop.vt)
	{
	case DPKVT_UI1:                  // ParamType_Byte
		hr = val.Set(prop.VT.bVal);
		break;
	case DPKVT_UI2:                  // ParamType_UShort
		hr = val.Set(prop.VT.uiVal);
		break;
	case DPKVT_LPSTR:                // ParamType_ASCII
		hr = val.Set(prop.VT.pszVal);
		break;
	case DPKVT_LPWSTR:               // ParamType_String
		hr = val.Set((PWCHAR)(prop.VT.pwszVal));
		break;
	// TODO: DPKVT_BYREF | DPKVT_UI1 - this type has some implied size based
	//       on the ID
	case DPKVT_BYREF | DPKVT_UI1:
	case DPKVT_EMPTY:
		break;
	default:
		VT_ASSERT( 0 );
		break;
	}

	if( hr == S_OK )
	{
		VT_HR_EXIT( params.SetById(uId, 0, val) );
	}
	else if( hr == S_FALSE )
	{
		hr = S_OK;  // swallow empty props
	}

Exit:
	return hr;
}

static HRESULT 
VtCopyFromHDPhotoMetadata(PKImageDecode* pDecoder, OUT CParams& params)
{
	ERR dpkerr;
	HRESULT hr = S_OK;

	PKPixelFormatGUID guidPF;

	CallDPK( pDecoder->GetPixelFormat(pDecoder, &guidPF) );

	const HDP_PIXFRMT_MAP* pmap = FindPixFrmtMatch(guidPF);
    if( pmap == NULL )
    {
        VT_HR_EXIT( E_NOTIMPL );
    }

    // set some custom values that some apps find useful
    {
    CParamValue v;
    VT_HR_EXIT( v.Set(pmap->bHasAlpha) );
    VT_HR_EXIT( params.SetByName(L"FileHadAlpha", 0, v) );
    VT_HR_EXIT( v.Set(ULONG(pmap->iSrcBitsPerPixel)) );
    VT_HR_EXIT( params.SetByName(L"SourceBitsPerPixel", 0, v) );
    }

	// read the color profile
	U32 uColorContextByteCount;
	pDecoder->GetColorContext(pDecoder, NULL, &uColorContextByteCount);

	if( uColorContextByteCount != 0 )
	{
        vt::vector<U8> vecCC;
        VT_HR_EXIT( vecCC.resize(uColorContextByteCount) );

		CallDPK( pDecoder->GetColorContext(pDecoder, vecCC.begin(),
										   &uColorContextByteCount) );

		CParamValue v;
		VT_HR_EXIT( v.SetArray((BYTE*)vecCC.begin(), uColorContextByteCount) );
		VT_HR_EXIT( params.SetById(ImagePropertyICCProfile, 0, v) );
	}

	// read the additional metadata that HDP exposes
	DESCRIPTIVEMETADATA descMeta;
	if( !Failed(pDecoder->GetDescriptiveMetadata(pDecoder, &descMeta)) )
	{
		VT_HR_EXIT( AddHDPhotoProperty(ImagePropertyImageDescription, 
									   descMeta.pvarImageDescription, params) );
		VT_HR_EXIT( AddHDPhotoProperty(ImagePropertyEquipMake,
									   descMeta.pvarCameraMake, params) );
		VT_HR_EXIT( AddHDPhotoProperty(ImagePropertyEquipModel,
									   descMeta.pvarCameraModel, params) );
		VT_HR_EXIT( AddHDPhotoProperty(ImagePropertySoftwareUsed,
									   descMeta.pvarSoftware, params) );
		VT_HR_EXIT( AddHDPhotoProperty(ImagePropertyDateTime,
									   descMeta.pvarDateTime, params) );
		VT_HR_EXIT( AddHDPhotoProperty(ImagePropertyArtist,
									   descMeta.pvarArtist, params) );
		VT_HR_EXIT( AddHDPhotoProperty(ImagePropertyCopyright,
									   descMeta.pvarCopyright, params) );
		VT_HR_EXIT( AddHDPhotoProperty(ImagePropertySimpleRating,
                                       descMeta.pvarRatingStars, params) );
		VT_HR_EXIT( AddHDPhotoProperty(ImagePropertyRating,
                                       descMeta.pvarRatingValue, params) );
		VT_HR_EXIT( AddHDPhotoProperty(ImagePropertyTitle,
                                       descMeta.pvarCaption, params) );
	}

Exit:
    return hr;
}

static HRESULT
HDPCopyImage( PKCodecFactory* pCodecFactory, PKImageDecode* pDecoder,
			  const CRect* pRect, CImg& imgDst, const HDP_PIXFRMT_MAP& mapSrc)
{
	ERR dpkerr;
	HRESULT hr = S_OK;

    int iWidth  = (int)pDecoder->WMP.wmiI.cWidth;
    int iHeight = (int)pDecoder->WMP.wmiI.cHeight;

    vt::CRect rctFrame(0, 0, iWidth, iHeight);
    vt::CRect rctRequest = pRect? *pRect: rctFrame;
    rctRequest.IntersectRect(&rctRequest, &rctFrame);

	PKFormatConverter* pConverter = NULL;

    Byte* pbConv = NULL;

	HDP_PIXFRMT_MAP mapDst = mapSrc;

	// if necessary create a color transformer to convert from source
	// format to dest format
	if( mapSrc.iMatchType == OBJ_UNDEFINED )
	{
		// find the pixel format for the dst image
		const HDP_PIXFRMT_MAP* pFrmtDst = FindPixFrmtMatch(mapSrc.iPreferType,
														   mapSrc.bRGBToBGR);
		if( pFrmtDst == NULL )
		{
			VT_HR_EXIT( E_NOTIMPL );
		}
		mapDst = *pFrmtDst;

		CallDPK( pCodecFactory->CreateFormatConverter(&pConverter) );
		CallDPK( pConverter->Initialize(pConverter, pDecoder, ".jxr", 
									    *mapDst.pHDPPixFrmt) );
	}

	// create the image
    VT_HR_EXIT( CreateImageForTransform(
		imgDst, rctRequest.Width(), rctRequest.Height(), mapDst.iPreferType) );

	// tell the decoder what the ROI is
	pDecoder->WMP.wmiI.cROILeftX  = rctRequest.left;
	pDecoder->WMP.wmiI.cROITopY   = rctRequest.top;
	pDecoder->WMP.wmiI.cROIWidth  = rctRequest.Width();
	pDecoder->WMP.wmiI.cROIHeight = rctRequest.Height();

    // copy the entire image at once
    PKRect rc;
    rc.X      = rctRequest.left;
    rc.Y      = rctRequest.top;
    rc.Width  = rctRequest.Width();
    rc.Height = rctRequest.Height();

    if( mapSrc.iMatchType == OBJ_UNDEFINED )
	{
		VT_ASSERT( pConverter );

		// otherwise do a conversion
		if( VT_IMG_SAME_BE(mapSrc.iPreferType, imgDst.GetType()) &&
            imgDst.PixSize() >= (mapSrc.iSrcBitsPerPixel >> 3) )
		{
			CallDPK( pConverter->Copy(pConverter, &rc, imgDst.BytePtr(),
									 (U32)imgDst.StrideBytes()) );
		}
		else
		{
			CImg imgConv;
			int iElSize = VT_IMG_ELSIZE(mapSrc.iPreferType);

            // make sure image is large enough for either source or dest
            int iPixSize = VtMax(mapSrc.iSrcBitsPerPixel >> 3,
				                 iElSize * VT_IMG_BANDS(mapSrc.iPreferType));
            pbConv = VT_NOTHROWNEW Byte[imgDst.Width() * imgDst.Height() * iPixSize];
            VT_PTR_OOM_EXIT( pbConv );

            VT_HR_EXIT( imgConv.Create(pbConv, imgDst.Width(), imgDst.Height(),
                                       imgDst.Width() * iPixSize, mapSrc.iPreferType) );

			CallDPK( pConverter->Copy(pConverter, &rc, imgConv.BytePtr(),
									 (U32)imgConv.StrideBytes()) );
	
			// convert
			VT_HR_EXIT( VtConvertImage(imgDst, imgConv) );
		}
	}
	else if( EL_FORMAT(imgDst.GetType())    == EL_FORMAT(mapDst.iMatchType) &&
			 VT_IMG_BANDS(imgDst.GetType()) == VT_IMG_BANDS(mapDst.iMatchType) )
    {
        // if the pixel formats match then do a straight copy
		CallDPK( pDecoder->Copy(pDecoder, &rc, imgDst.BytePtr(),
							   (U32)imgDst.StrideBytes()) );			
    }
    else
    {
		CImg imgConv;
		int iElSize = VT_IMG_ELSIZE(mapSrc.iPreferType);
		VT_HR_EXIT( imgConv.Create(imgDst.Width(), imgDst.Height(), 
								   mapSrc.iMatchType, alignAny) );

		CallDPK( pDecoder->Copy(pDecoder, &rc, imgConv.BytePtr(),
							   (U32)imgConv.StrideBytes()) );

		// convert
		VT_HR_EXIT( VtConvertImage(imgDst, imgConv) );
    }

	// do an RGB swap if necessary
	if( mapDst.bRGBToBGR )
	{
		VtRGBColorSwapImage(imgDst);
	}

Exit:
    if( pConverter ) pConverter->Release(&pConverter);
    if( pbConv ) delete[] pbConv;
	return hr;
}

static HRESULT
HDPCopyImage(IImageReader* pReader, const CRect& rctSrc,
             const HDP_PIXFRMT_MAP& mapDst, 
			 PKImageEncode* pEncoder, CTaskProgress* pProgress)
{
	ERR dpkerr;
    HRESULT hr = S_OK;

    CImg imgSubSrc;

	// use the banded write API to avoid creating an entire 
    // second image
    CallDPK( pEncoder->WritePixelsBandedBegin(pEncoder, NULL) );

    // copy 16 scan-lines at a time
    for (CBlockIterator bi(BLOCKITER_INIT(rctSrc, rctSrc.Width(), MB_HEIGHT_PIXEL));
        !bi.Done(); bi.Advance())
    {
        CRect rctSubSrc = bi.GetRect();
        VT_HR_EXIT( CreateImageForTransform(imgSubSrc, rctSubSrc.Width(), rctSubSrc.Height(),
                                            mapDst.iMatchType) );

        // copy (and possibly convert) scanlines from source to buffer
        VT_HR_EXIT( pReader->ReadRegion(rctSubSrc, imgSubSrc) );

        // do an RGB swap if necessary
        if( mapDst.bRGBToBGR )
        {
            VT_HR_EXIT( VtRGBColorSwapImage(imgSubSrc) );
        }

        // write
		CallDPK( pEncoder->WritePixelsBanded(pEncoder, imgSubSrc.Height(),
									         imgSubSrc.BytePtr(), 
                                             (U32)imgSubSrc.StrideBytes(),
                                             rctSubSrc.bottom == rctSrc.bottom) );

        VT_HR_EXIT( CheckTaskProgressCancel(pProgress, 100.f * float(rctSubSrc.bottom) / float(rctSrc.bottom)) );
    }

    CallDPK( pEncoder->WritePixelsBandedEnd(pEncoder) );

Exit:
	return hr;
}

static HRESULT
HDPCopyRGBEImage(IImageReader* pReader, const CRect& rctSrc,
                 PKImageEncode* pEncoder, CTaskProgress* pProgress)
{
	ERR dpkerr;
    HRESULT hr = S_OK;

    CRGBFloatImg imgSubSrc;
	CByteImg imgSubDst;

	// use the banded write API to avoid creating an entire 
    // second image
    CallDPK( pEncoder->WritePixelsBandedBegin(pEncoder, NULL) );

    // copy 16 scan-lines at a time
    for (CBlockIterator bi(BLOCKITER_INIT(rctSrc, rctSrc.Width(), MB_HEIGHT_PIXEL));
        !bi.Done(); bi.Advance())
    {
        CRect rctSubSrc = bi.GetRect();

	    // create a temp destination image
	    VT_HR_EXIT( imgSubDst.Create( rctSubSrc.Width(), rctSubSrc.Height(), 4 ) );

		// if source is not RGBFloat then do an intermediate
		// float conversion
        VT_HR_EXIT( pReader->ReadRegion(rctSubSrc, imgSubSrc) );

        for( int y = 0; y < rctSubSrc.Height(); y++ )
			VtConvertSpanFloatToRGBE(imgSubDst.Ptr(y), imgSubSrc.Ptr(y),
                                     rctSubSrc.Width());

	    // write
	    CallDPK( pEncoder->WritePixelsBanded(pEncoder, rctSubSrc.Height(),
                                             imgSubDst.BytePtr(), 
                                             (U32)imgSubDst.StrideBytes(),
                                             rctSubSrc.bottom == rctSrc.bottom) );

        VT_HR_EXIT( CheckTaskProgressCancel(pProgress, 100.f * float(rctSubSrc.bottom) / float(rctSrc.bottom)) );
    }

    CallDPK( pEncoder->WritePixelsBandedEnd(pEncoder) );

Exit:
	return hr;
}

static
void HDPEncInitDefaultArgs(CWMIStrCodecParam& wmiSCP)
{
    memset(&wmiSCP, 0, sizeof(wmiSCP));

    wmiSCP.bVerbose      = FALSE;
    wmiSCP.cfColorFormat = YUV_444;
    wmiSCP.bdBitDepth    = BD_LONG;
    wmiSCP.bfBitstreamFormat  = FREQUENCY;
    wmiSCP.olOverlap          = OL_ONE;
    wmiSCP.cNumOfSliceMinus1H = wmiSCP.cNumOfSliceMinus1V = 0;
    wmiSCP.sbSubband  = SB_ALL;
    wmiSCP.uAlphaMode = 0;
    wmiSCP.uiDefaultQPIndex = 1;
    wmiSCP.uiDefaultQPIndexAlpha = 1;
    wmiSCP.bUseHardTileBoundaries = FALSE;
    wmiSCP.bProgressiveMode = TRUE;
}

// consider exposing these params
#define DEFAULT_ALPHA_QP 1

// Y, U, V, YHP, UHP, VHP
#if 1
// optimized for PSNR
int DPK_QPS_420[11][6] = {      // for 8 bit only
    { 66, 65, 70, 72, 72, 77 },
    { 59, 58, 63, 64, 63, 68 },
    { 52, 51, 57, 56, 56, 61 },
    { 48, 48, 54, 51, 50, 55 },
    { 43, 44, 48, 46, 46, 49 },
    { 37, 37, 42, 38, 38, 43 },
    { 26, 28, 31, 27, 28, 31 },
    { 16, 17, 22, 16, 17, 21 },
    { 10, 11, 13, 10, 10, 13 },
    {  5,  5,  6,  5,  5,  6 },
    {  2,  2,  3,  2,  2,  2 }
};

int DPK_QPS_8[12][6] = {
    { 67, 79, 86, 72, 90, 98 },
    { 59, 74, 80, 64, 83, 89 },
    { 53, 68, 75, 57, 76, 83 },
    { 49, 64, 71, 53, 70, 77 },
    { 45, 60, 67, 48, 67, 74 },
    { 40, 56, 62, 42, 59, 66 },
    { 33, 49, 55, 35, 51, 58 },
    { 27, 44, 49, 28, 45, 50 },
    { 20, 36, 42, 20, 38, 44 },
    { 13, 27, 34, 13, 28, 34 },
    {  7, 17, 21,  8, 17, 21 }, // Photoshop 100%
    {  2,  5,  6,  2,  5,  6 }
};
#else
// optimized for SSIM
int DPK_QPS_420[11][6] = {      // for 8 bit only
    { 67, 77, 80, 75, 82, 86 },
    { 58, 67, 71, 63, 74, 78 },
    { 50, 60, 64, 54, 66, 69 },
    { 46, 55, 59, 49, 60, 63 },
    { 41, 48, 53, 43, 52, 56 },
    { 35, 43, 48, 36, 44, 49 },
    { 29, 37, 41, 30, 38, 41 },
    { 22, 29, 33, 22, 29, 33 },
    { 15, 20, 26, 14, 20, 25 },
    {  9, 14, 18,  8, 14, 17 },
    {  4,  6,  7,  3,  5,  5 }
};

int DPK_QPS_8[12][6] = {
    { 67, 93, 98, 71, 98, 104 },
    { 59, 83, 88, 61, 89,  95 },
    { 50, 76, 81, 53, 85,  90 },
    { 46, 71, 77, 47, 79,  85 },
    { 41, 67, 71, 42, 75,  78 },
    { 34, 59, 65, 35, 66,  72 },
    { 30, 54, 60, 29, 60,  66 },
    { 24, 48, 53, 22, 53,  58 },
    { 18, 39, 45, 17, 43,  48 },
    { 13, 34, 38, 11, 35,  38 },
    {  8, 20, 24,  7, 22,  25 }, // Photoshop 100%
    {  2,  5,  6,  2,  5,   6 }
};
#endif

int DPK_QPS_16[11][6] = {
    { 197, 203, 210, 202, 207, 213 },
    { 174, 188, 193, 180, 189, 196 },
    { 152, 167, 173, 156, 169, 174 },
    { 135, 152, 157, 137, 153, 158 },
    { 119, 137, 141, 119, 138, 142 },
    { 102, 120, 125, 100, 120, 124 },
    {  82,  98, 104,  79,  98, 103 },
    {  60,  76,  81,  58,  76,  81 },
    {  39,  52,  58,  36,  52,  58 },
    {  16,  27,  33,  14,  27,  33 },
    {   5,   8,   9,   4,   7,   8 }
};

int DPK_QPS_16f[11][6] = {
    { 148, 177, 171, 165, 187, 191 },
    { 133, 155, 153, 147, 172, 181 },
    { 114, 133, 138, 130, 157, 167 },
    {  97, 118, 120, 109, 137, 144 },
    {  76,  98, 103,  85, 115, 121 },
    {  63,  86,  91,  62,  96,  99 },
    {  46,  68,  71,  43,  73,  75 },
    {  29,  48,  52,  27,  48,  51 },
    {  16,  30,  35,  14,  29,  34 },
    {   8,  14,  17,   7,  13,  17 },
    {   3,   5,   7,   3,   5,   6 }
};

int DPK_QPS_32f[11][6] = {
    { 194, 206, 209, 204, 211, 217 },
    { 175, 187, 196, 186, 193, 205 },
    { 157, 170, 177, 167, 180, 190 },
    { 133, 152, 156, 144, 163, 168 },
    { 116, 138, 142, 117, 143, 148 },
    {  98, 120, 123,  96, 123, 126 },
    {  80,  99, 102,  78,  99, 102 },
    {  65,  79,  84,  63,  79,  84 },
    {  48,  61,  67,  45,  60,  66 },
    {  27,  41,  46,  24,  40,  45 },
    {   3,  22,  24,   2,  21,  22 }
};

static
HRESULT HDPInitEncoder(PKImageEncode* pEncoder, WMPStream* pDstStream,
					   const CParams* pSaveParams, const GUID& guidFormatDst,
					   const HDP_PIXFRMT_MAP* pmap, int width, int height)
{
	ERR dpkerr;
    HRESULT hr = S_OK;

	// copy params
    CWMIStrCodecParam* pcodecParams = VT_NOTHROWNEW CWMIStrCodecParam;
    VT_PTR_OOM_EXIT( pcodecParams );
    CWMIStrCodecParam& codecParams = *pcodecParams;
	HDPEncInitDefaultArgs(codecParams);
    // use interleaved alpha (uAlphaMode == 3) to avoid planar alpha temp file
    // during banded encoding
    U8 uAlphaMode = pmap->bHasAlpha ? (VT_IMG_BANDS(pmap->iPreferType) == 1 ? 1 : 3) : 0;
    codecParams.uAlphaMode = uAlphaMode;

	if( pSaveParams )
	{
		const CParamValue* pval;
		if( pSaveParams->GetByName(&pval, L"CodecParams") == S_OK && 
			pval->GetDataSize() == sizeof(codecParams) )
		{
			memcpy(&codecParams, pval->GetDataPtr(), sizeof(codecParams));
		}
		else
		{
			float fQuality;
			if( pSaveParams->GetByName(&fQuality, L"ImageQuality") == S_OK )
			{
                if (fQuality < 0.f || fQuality > 1.f)
                    VT_HR_EXIT( E_INVALIDARG );

				//ImageQuality  Q (BD==1)  Q (BD==8)   Q (BD==16)  Q (BD==32F) Subsample   Overlap
	            //[0.0, 0.5)    8-IQ*5     (see table) (see table) (see table) 4:2:0       2
	            //[0.5, 1.0)    8-IQ*5     (see table) (see table) (see table) 4:4:4       1
				//[1.0, 1.0]    1          1           1           1           4:4:4       0
	
                if (fQuality == 1.0F)
				{
					codecParams.uiDefaultQPIndex = 1;
					codecParams.cfColorFormat = YUV_444;
					codecParams.olOverlap = OL_NONE;
				}
				else
				{
                    // Image width must be at least 2 MB wide for subsampled chroma and two levels of overlap!
		            if (fQuality >= 0.5F || width < 2 * MB_WIDTH_PIXEL)
						codecParams.olOverlap = OL_ONE;
					else
						codecParams.olOverlap = OL_TWO;

		            if (fQuality >= 0.5F || EL_FORMAT( pmap->iPreferType ) != EL_FORMAT_BYTE)
				        codecParams.cfColorFormat = YUV_444;
		            else
				        codecParams.cfColorFormat = YUV_420;

					if (IsEqualGUID(guidFormatDst, GUID_PKPixelFormatBlackWhite))
					{
						codecParams.uiDefaultQPIndex = (U8)(8 - 5.0F *
							fQuality + 0.5F);
					}
					else 
					{
                        // remap [0.8, 0.866, 0.933, 1.0] to [0.8, 0.9, 1.0, 1.1]
                        // to use 8-bit DPK QP table (0.933 == Photoshop JPEG 100)
                        if (fQuality > 0.8f && EL_FORMAT( pmap->iPreferType ) == EL_FORMAT_BYTE && 
                            codecParams.cfColorFormat != YUV_420 && codecParams.cfColorFormat != YUV_422)
                            fQuality = 0.8f + (fQuality - 0.8f) * 1.5f;

                        int qi = (int) (10.f * fQuality);
                        float qf = 10.f * fQuality - (float) qi;

                        int* pQPs =
                            (codecParams.cfColorFormat == YUV_420 || codecParams.cfColorFormat == YUV_422) ?
                                DPK_QPS_420[qi] :
                            (EL_FORMAT( pmap->iPreferType ) == EL_FORMAT_BYTE ? DPK_QPS_8[qi] :
                            (EL_FORMAT( pmap->iPreferType ) == EL_FORMAT_SHORT ? DPK_QPS_16[qi] :
                            (EL_FORMAT( pmap->iPreferType ) == EL_FORMAT_HALF_FLOAT ? DPK_QPS_16f[qi] :
                            DPK_QPS_32f[qi])));

                        codecParams.uiDefaultQPIndex = (U8) (0.5f +
                                (float) pQPs[0] * (1.f - qf) + (float) (pQPs + 6)[0] * qf);
                        codecParams.uiDefaultQPIndexU = (U8) (0.5f +
                                (float) pQPs[1] * (1.f - qf) + (float) (pQPs + 6)[1] * qf);
                        codecParams.uiDefaultQPIndexV = (U8) (0.5f +
                                (float) pQPs[2] * (1.f - qf) + (float) (pQPs + 6)[2] * qf);
                        codecParams.uiDefaultQPIndexYHP = (U8) (0.5f +
                                (float) pQPs[3] * (1.f - qf) + (float) (pQPs + 6)[3] * qf);
                        codecParams.uiDefaultQPIndexUHP = (U8) (0.5f +
                                (float) pQPs[4] * (1.f - qf) + (float) (pQPs + 6)[4] * qf);
                        codecParams.uiDefaultQPIndexVHP = (U8) (0.5f +
                                (float) pQPs[5] * (1.f - qf) + (float) (pQPs + 6)[5] * qf);
					}
				}
			}
		}
	}

	// TODO: figure out how to set color profile
    CallDPK( pEncoder->Initialize(pEncoder, pDstStream, &codecParams, 
								  sizeof(codecParams)) );

	// if encoder is capable of saving alpha then 
    if(pEncoder->WMP.wmiSCP.uAlphaMode != 0)
    {
        pEncoder->WMP.wmiSCP_Alpha.uiDefaultQPIndex = DEFAULT_ALPHA_QP;
        pEncoder->WMP.wmiSCP_Alpha.olOverlap = OL_NONE;
    }

	CallDPK( pEncoder->SetPixelFormat(pEncoder, guidFormatDst) );
	CallDPK( pEncoder->SetSize(pEncoder, width, height) );

	// TODO: are these necessary
	//CallDPK( pEncoder->SetResolution(pEncoder, rX, rY) );
	//pEncoder->WMP.oOrientation = args.orientation;

Exit:
    delete pcodecParams;
	return hr;
}

//+-----------------------------------------------------------------------------
//
//  Loading HD Photo
// 
//------------------------------------------------------------------------------
static HRESULT 
VtLoadHDPhoto_Internal(PKCodecFactory* pCodecFactory, PKImageDecode* pDecoder,
                       CImg &imgDst, const CRect* pRect, bool bLoadMetadata)
							   
{
	ERR dpkerr;
	HRESULT hr = S_OK;

	// determine if this pixel format is supported
	const HDP_PIXFRMT_MAP* match = FindPixFrmtMatch(pDecoder->guidPixFormat);
	if( match == NULL )
	{
		VT_HR_EXIT( E_INVALIDARG );
	}

	// setup the decoder
    pDecoder->WMP.wmiSCP.uAlphaMode = 2;  // image & alpha for formats with alpha
    pDecoder->WMP.wmiI.cPostProcStrength = 0;

	CallDPK( pDecoder->SelectFrame(pDecoder,0) );

	// call decoder/format convert
	VT_HR_EXIT( HDPCopyImage(pCodecFactory, pDecoder, pRect, imgDst, *match) );

    if( bLoadMetadata )
	{
		CParams p;
		VT_HR_EXIT( VtCopyFromHDPhotoMetadata(pDecoder, p ) );
		imgDst.SetMetaData(&p);
	}

Exit:
    return hr;
}

HRESULT VtLoadHDPhoto_Internal(const WCHAR * pszFileName, CImg &imgDst,
							   const CRect* pRect, bool bLoadMetadata, int iIndex)
{
	ERR dpkerr;
	HRESULT hr = S_OK;

	PKCodecFactory* pCodecFactory = NULL;
	PKImageDecode*  pDecoder      = NULL;

	// convert input to a multibyte
	vt::string strFileNameMB;
	VT_HR_EXIT( strFileNameMB.reserve(wcslen(pszFileName)+1) );
	VtWideCharToMultiByte(strFileNameMB, pszFileName);

    //================================    
    CallDPK( PKCreateCodecFactory(&pCodecFactory, WMP_SDK_VERSION) );
    CallDPK( pCodecFactory->CreateDecoderFromFile(strFileNameMB, &pDecoder) );

	VT_HR_EXIT( VtLoadHDPhoto_Internal(pCodecFactory, pDecoder, imgDst,
                                       pRect, bLoadMetadata) );

Exit: 
	if( pDecoder)       pDecoder->Release(&pDecoder);
	if( pCodecFactory ) pCodecFactory->Release(&pCodecFactory);

    return hr;
}

HRESULT vt::VtLoadHDPhoto(IStream* pStream, CImg &imgDst, const CRect* pRect,
                          bool bLoadMetadata)
{
	ERR dpkerr;
	HRESULT hr = S_OK;

	const PKIID*    pIID;
	PKCodecFactory* pCodecFactory = NULL;
	PKImageDecode*  pDecoder      = NULL;

	CWMPStreamConv streamConv(pStream);  

    //================================    
    CallDPK( PKCreateCodecFactory(&pCodecFactory, WMP_SDK_VERSION) );

	// get decode PKIID
	CallDPK( GetImageDecodeIID(".jxr", &pIID) );

	// Create decoder
	CallDPK( PKCodecFactory_CreateCodec(pIID, (void**)&pDecoder) );

	CallDPK( pDecoder->Initialize(pDecoder, streamConv.GetWMPStream()) ); 

	VT_HR_EXIT( VtLoadHDPhoto_Internal(pCodecFactory, pDecoder, imgDst,
                                       pRect, bLoadMetadata) );

Exit: 
	if( pDecoder)       pDecoder->Release(&pDecoder);
	if( pCodecFactory ) pCodecFactory->Release(&pCodecFactory);

    return hr;
}

HRESULT vt::VtLoadHDPhoto(const WCHAR * pszFile, CImg &imgDst, 
						  const CRect* pRect, bool bLoadMetadata, int iIndex)
{
    return VtLoadHDPhoto_Internal((const WCHAR *)pszFile, imgDst, 
								  pRect, bLoadMetadata, iIndex);
}

//+----------------------------------------------------------------------------
//
// Class CHDPWriter
// 
// Synopsis:
//     HP Photo writing routines
// 
//-----------------------------------------------------------------------------

CHDPWriter::CHDPWriter() : m_pvEncodeStream(NULL), m_pvStreamConv(NULL)
{
}

CHDPWriter::~CHDPWriter()
{
	CloseFile( );
}

HRESULT
CHDPWriter::Clone(IVTImageWriter **ppWriter)
{
    if (ppWriter == NULL)
        return E_INVALIDARG;

    if ((*ppWriter = VT_NOTHROWNEW CHDPWriter) == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
CHDPWriter::OpenFile( IStream* pStream, const WCHAR * pszFileName )
{
	ERR dpkerr;
	HRESULT hr = S_OK;

	CloseFile( );

	PKFactory*        pFactory = NULL;
	struct WMPStream* pEncodeStream = NULL;
	vt::string        strFileNameMB;

	CallDPK( PKCreateFactory(&pFactory, PK_SDK_VERSION) );

	// convert input to a multibyte
	VT_HR_EXIT( strFileNameMB.reserve(wcslen(pszFileName)+1) );
	VtWideCharToMultiByte(strFileNameMB, pszFileName);

	// create a stream from the filename
	if( pStream )
	{
		m_pvStreamConv = VT_NOTHROWNEW CWMPStreamConv(pStream);
	}
	else
	{
		CallDPK( pFactory->CreateStreamFromFilename(
			(WMPStream**)&m_pvEncodeStream, strFileNameMB, "wb") );
	}

Exit:
	if( pFactory ) pFactory->Release(&pFactory);

	return hr;
}

HRESULT
CHDPWriter::SetImage( IStream* pSrcStream, CRect& rect, const CParams* pSaveParams )
{
	ERR dpkerr;
	HRESULT hr = S_OK;

	const PKIID*    pIID;
    PKCodecFactory* pCodecFactory = NULL;
    PKImageEncode*  pEncoder      = NULL;
	PKImageDecode*  pDecoder      = NULL;
	PKPixelFormatGUID guidFormatSrc;
	CWMTranscodingParam transcodeParam;

	// wrap the source stream
	CWMPStreamConv streamConv(pSrcStream);  

	// determine which stream to use
	WMPStream* pDstStream = 
		m_pvStreamConv? ((CWMPStreamConv*)m_pvStreamConv)->GetWMPStream():
						(WMPStream*)m_pvEncodeStream;

	// create the decoder
	CallDPK( GetImageDecodeIID(".jxr", &pIID) );
	CallDPK( PKCodecFactory_CreateCodec(pIID, (void**)&pDecoder) );
	CallDPK( pDecoder->Initialize(pDecoder, streamConv.GetWMPStream()) ); 
	CallDPK( pDecoder->GetPixelFormat(pDecoder, &guidFormatSrc) );

	const HDP_PIXFRMT_MAP* pmap = FindPixFrmtMatch(guidFormatSrc);
    if( pmap == NULL )
    {
        VT_HR_EXIT( E_NOTIMPL );
    }
    U8 uAlphaMode = pmap->bHasAlpha ? (VT_IMG_BANDS(pmap->iPreferType) == 1 ? 1 : 2) : 0;

	// create and initialize the encoder
    CallDPK( PKCreateCodecFactory(&pCodecFactory, WMP_SDK_VERSION) );
    CallDPK( pCodecFactory->CreateCodec(&IID_PKImageWmpEncode,
										(void**)&pEncoder) );
	VT_HR_EXIT( HDPInitEncoder(pEncoder, pDstStream, pSaveParams,
        guidFormatSrc, pmap, rect.Width(), rect.Height()) );

	// perform the transcode
	ZeroMemory(&transcodeParam, sizeof(transcodeParam));
	transcodeParam.cLeftX  = rect.left;
	transcodeParam.cTopY   = rect.top;
	transcodeParam.cWidth  = rect.Width();
	transcodeParam.cHeight = rect.Height();
    transcodeParam.uAlphaMode = uAlphaMode;
	
    CallDPK( pEncoder->Transcode(pEncoder, pDecoder, &transcodeParam) );

Exit:
	if( pEncoder ) 
	{
		pEncoder->Release(&pEncoder);
		// encoder release closes the stream 
		m_pvEncodeStream = NULL;
	}
	if( pDecoder)       pDecoder->Release(&pDecoder);
	if( pCodecFactory ) pCodecFactory->Release(&pCodecFactory);

    return hr;
}

HRESULT
CHDPWriter::SetImage( const CImg & imgSrc, bool bSaveMetadata,
					  const CParams* pSaveParams,
                      CTaskProgress* pProgress )
{
    CImgReaderWriter<CImg> img; imgSrc.Share(img);
    return SetImage(&img, NULL, bSaveMetadata, pSaveParams, pProgress);
}

HRESULT
CHDPWriter::SetImage( IImageReader* pReader, const CRect* pRect,
                      bool bSaveMetadata,
					  const CParams* pSaveParams,
                      CTaskProgress* pProgress )
{
	ERR dpkerr;
	HRESULT hr = S_OK;

    CImgInfo& info = pReader->GetImgInfo();
    const CRect rctSrc = pRect == NULL ? info.Rect() :
        (*pRect & info.Rect());

    PKCodecFactory* pCodecFactory = NULL;
    PKImageEncode*  pEncoder      = NULL;

	// determine which stream to use
	WMPStream* pDstStream = 
		m_pvStreamConv? ((CWMPStreamConv*)m_pvStreamConv)->GetWMPStream():
						(WMPStream*)m_pvEncodeStream;

	// figure out what the destination pixel format is
	GUID guidFormatSrc, guidFormatDst;  
    const HDP_PIXFRMT_MAP* pmap = FindPixFrmtMatch(info.type);
    if( pmap == NULL )
    {
		// TODO: handle int images 
		// TODO: handle RGBFloat
        VT_HR_EXIT( E_NOTIMPL );
    }
    guidFormatSrc = *pmap->pHDPPixFrmt;

	if( pSaveParams != NULL &&
		pSaveParams->GetByName(&guidFormatDst, L"ImageFormat") == S_OK )
	{
		pmap = FindPixFrmtMatch( guidFormatDst );
		if( pmap == NULL )
		{
			VT_HR_EXIT( E_NOTIMPL );
		}
	}
	else
	{
		guidFormatDst = guidFormatSrc;
	}

	// create and initialize the encoder
    CallDPK( PKCreateCodecFactory(&pCodecFactory, WMP_SDK_VERSION) );
    CallDPK( pCodecFactory->CreateCodec(&IID_PKImageWmpEncode,
										(void**)&pEncoder) );

	VT_HR_EXIT( HDPInitEncoder(pEncoder, pDstStream, pSaveParams,
							   guidFormatDst, pmap,
                               rctSrc.Width(), rctSrc.Height()) );

	// write the frame

    if( IsEqualGUID(guidFormatDst, GUID_PKPixelFormat32bppRGBE) ) 
	{
		VT_HR_EXIT( HDPCopyRGBEImage(pReader, rctSrc, pEncoder, pProgress) );
	}
	else if( pmap->iMatchType == OBJ_UNDEFINED )
	{
		VT_HR_EXIT( E_NOTIMPL );
    }
    else
    {
        // no need to transform
        VT_HR_EXIT( HDPCopyImage(pReader, rctSrc, *pmap, pEncoder, pProgress) );
    }

Exit:
	if( pEncoder )
	{
		pEncoder->Release(&pEncoder);
		// encoder release closes the stream 
		m_pvEncodeStream = NULL;
	}

	if( pCodecFactory ) pCodecFactory->Release(&pCodecFactory);


    return hr;
}

HRESULT
CHDPWriter::CloseFile( )
{
	if( m_pvStreamConv )
	{
		delete (CWMPStreamConv*)m_pvStreamConv;
		m_pvStreamConv = NULL;
	}
 
	if( m_pvEncodeStream )
	{
		((WMPStream*)m_pvEncodeStream)->Close((WMPStream**)&m_pvEncodeStream);
		m_pvEncodeStream = NULL;
	}
	return S_OK;
}

HRESULT vt::VtSaveHDPhoto(IStream* pStream, const CImg &imgSrc, 
						  bool bSaveMetadata, int iIndex, 
						  CParams* pSaveParams)
{
	HRESULT hr;

	CHDPWriter wr;
	VT_HR_EXIT( wr.OpenFile(pStream, (WCHAR*)NULL) );
	VT_HR_EXIT( wr.SetImage(imgSrc, bSaveMetadata, pSaveParams) );

Exit:
	return hr;
}

//+-----------------------------------------------------------------------------
//
//  Saving HD Photo
// 
//------------------------------------------------------------------------------

HRESULT vt::VtSaveHDPhoto(const WCHAR * pszFile, const CImg &imgSrc, 
						  bool bSaveMetadata, int iIndex,
						  CParams* pSaveParams)
{
	HRESULT hr;

	CHDPWriter wr;
	VT_HR_EXIT( wr.OpenFile(pszFile) );
	VT_HR_EXIT( wr.SetImage(imgSrc, bSaveMetadata, pSaveParams) );

Exit:
	return hr;
}

HRESULT vt::VtSaveHDPhoto(const WCHAR * pszFile,
                          IImageReader* pReader, const CRect* pRect,
						  bool bSaveMetadata, int iIndex,
						  CParams* pSaveParams)
{
	HRESULT hr;

	CHDPWriter wr;
	VT_HR_EXIT( wr.OpenFile(pszFile) );
	VT_HR_EXIT( wr.SetImage(pReader, pRect, bSaveMetadata, pSaveParams) );

Exit:
	return hr;
}
