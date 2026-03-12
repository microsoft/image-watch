//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Exif properties
//
//  History:
//      2004/11/08-swinder
//			Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_exifmap.h"

using namespace vt;

// ImagePropertyNull must be the last item

#define MAKE_PROPS(n) {ImageProperty##n, L#n}

struct EXIF_PROPERTY_MAP 
{
    PROPID id;
    WCHAR *pszName;
};

static EXIF_PROPERTY_MAP g_sExifMap[] = {
	MAKE_PROPS(ExifIFD                       ),    //8769
	MAKE_PROPS(GpsIFD                        ),    //8825
	MAKE_PROPS(NewSubfileType                ),    //00FE
	MAKE_PROPS(SubfileType                   ),    //00FF
	MAKE_PROPS(ImageWidth                    ),    //0100
	MAKE_PROPS(ImageHeight                   ),    //0101
	MAKE_PROPS(BitsPerSample                 ),    //0102
	MAKE_PROPS(Compression                   ),    //0103
	MAKE_PROPS(PhotometricInterp             ),    //0106
	MAKE_PROPS(ThreshHolding                 ),    //0107
	MAKE_PROPS(CellWidth                     ),    //0108
	MAKE_PROPS(CellHeight                    ),    //0109
	MAKE_PROPS(FillOrder                     ),    //010A
	MAKE_PROPS(DocumentName                  ),    //010D
	MAKE_PROPS(ImageDescription              ),    //010E
	MAKE_PROPS(EquipMake                     ),    //010F
	MAKE_PROPS(EquipModel                    ),    //0110
	MAKE_PROPS(StripOffsets                  ),    //0111
	MAKE_PROPS(Orientation                   ),    //0112
	MAKE_PROPS(SamplesPerPixel               ),    //0115
	MAKE_PROPS(RowsPerStrip                  ),    //0116
	MAKE_PROPS(StripBytesCount               ),    //0117
	MAKE_PROPS(MinSampleValue                ),    //0118
	MAKE_PROPS(MaxSampleValue                ),    //0119
	MAKE_PROPS(XResolution                   ),    //011A
	MAKE_PROPS(YResolution                   ),    //011B
	MAKE_PROPS(PlanarConfig                  ),    //011C
	MAKE_PROPS(PageName                      ),    //011D
	MAKE_PROPS(XPosition                     ),    //011E
	MAKE_PROPS(YPosition                     ),    //011F
	MAKE_PROPS(FreeOffset                    ),    //0120
	MAKE_PROPS(FreeByteCounts                ),    //0121
	MAKE_PROPS(GrayResponseUnit              ),    //0122
	MAKE_PROPS(GrayResponseCurve             ),    //0123
	MAKE_PROPS(T4Option                      ),    //0124
	MAKE_PROPS(T6Option                      ),    //0125
	MAKE_PROPS(ResolutionUnit                ),    //0128
	MAKE_PROPS(PageNumber                    ),    //0129
	MAKE_PROPS(TransferFunction              ),    //012D
	MAKE_PROPS(SoftwareUsed                  ),    //0131
	MAKE_PROPS(DateTime                      ),    //0132
	MAKE_PROPS(Artist                        ),    //013B
	MAKE_PROPS(HostComputer                  ),    //013C
	MAKE_PROPS(Predictor                     ),    //013D
	MAKE_PROPS(WhitePoint                    ),    //013E
	MAKE_PROPS(PrimaryChromaticities         ),    //013F
	MAKE_PROPS(ColorMap                      ),    //0140
	MAKE_PROPS(HalftoneHints                 ),    //0141
	MAKE_PROPS(TileWidth                     ),    //0142
	MAKE_PROPS(TileLength                    ),    //0143
	MAKE_PROPS(TileOffset                    ),    //0144
	MAKE_PROPS(TileByteCounts                ),    //0145
	MAKE_PROPS(InkSet                        ),    //014C
	MAKE_PROPS(InkNames                      ),    //014D
	MAKE_PROPS(NumberOfInks                  ),    //014E
	MAKE_PROPS(DotRange                      ),    //0150
	MAKE_PROPS(TargetPrinter                 ),    //0151
	MAKE_PROPS(ExtraSamples                  ),    //0152
	MAKE_PROPS(SampleFormat                  ),    //0153
	MAKE_PROPS(SMinSampleValue               ),    //0154
	MAKE_PROPS(SMaxSampleValue               ),    //0155
	MAKE_PROPS(TransferRange                 ),    //0156
	MAKE_PROPS(JPEGProc                      ),    //0200
	MAKE_PROPS(JPEGInterFormat               ),    //0201
	MAKE_PROPS(JPEGInterLength               ),    //0202
	MAKE_PROPS(JPEGRestartInterval           ),    //0203
	MAKE_PROPS(JPEGLosslessPredictors        ),    //0205
	MAKE_PROPS(JPEGPointTransforms           ),    //0206
	MAKE_PROPS(JPEGQTables                   ),    //0207
	MAKE_PROPS(JPEGDCTables                  ),    //0208
	MAKE_PROPS(JPEGACTables                  ),    //0209
	MAKE_PROPS(YCbCrCoefficients             ),    //0211
	MAKE_PROPS(YCbCrSubsampling              ),    //0212
	MAKE_PROPS(YCbCrPositioning              ),    //0213
	MAKE_PROPS(REFBlackWhite                 ),    //0214
	MAKE_PROPS(ICCProfile                    ),    //8773
	MAKE_PROPS(Gamma                         ),    //0301
	MAKE_PROPS(ICCProfileDescriptor          ),    //0302
	MAKE_PROPS(SRGBRenderingIntent           ),    //0303
	MAKE_PROPS(ImageTitle                    ),    //0320
	MAKE_PROPS(Copyright                     ),    //8298
	MAKE_PROPS(ResolutionXUnit               ),    //5001
	MAKE_PROPS(ResolutionYUnit               ),    //5002
	MAKE_PROPS(ResolutionXLengthUnit         ),    //5003
	MAKE_PROPS(ResolutionYLengthUnit         ),    //5004
	MAKE_PROPS(PrintFlags                    ),    //5005
	MAKE_PROPS(PrintFlagsVersion             ),    //5006
	MAKE_PROPS(PrintFlagsCrop                ),    //5007
	MAKE_PROPS(PrintFlagsBleedWidth          ),    //5008
	MAKE_PROPS(PrintFlagsBleedWidthScale     ),    //5009
	MAKE_PROPS(HalftoneLPI                   ),    //500A
	MAKE_PROPS(HalftoneLPIUnit               ),    //500B
	MAKE_PROPS(HalftoneDegree                ),    //500C
	MAKE_PROPS(HalftoneShape                 ),    //500D
	MAKE_PROPS(HalftoneMisc                  ),    //500E
	MAKE_PROPS(HalftoneScreen                ),    //500F
	MAKE_PROPS(JPEGQuality                   ),    //5010
	MAKE_PROPS(GridSize                      ),    //5011
	MAKE_PROPS(ThumbnailFormat               ),    //5012
	MAKE_PROPS(ThumbnailWidth                ),    //5013
	MAKE_PROPS(ThumbnailHeight               ),    //5014
	MAKE_PROPS(ThumbnailColorDepth           ),    //5015
	MAKE_PROPS(ThumbnailPlanes               ),    //5016
	MAKE_PROPS(ThumbnailRawBytes             ),    //5017
	MAKE_PROPS(ThumbnailSize                 ),    //5018
	MAKE_PROPS(ThumbnailCompressedSize       ),    //5019
	MAKE_PROPS(ColorTransferFunction         ),    //501A
	MAKE_PROPS(ThumbnailData                 ),    //501B
	MAKE_PROPS(ThumbnailImageWidth           ),    //5020
	MAKE_PROPS(ThumbnailImageHeight          ),    //5021
	MAKE_PROPS(ThumbnailBitsPerSample        ),    //5022
	MAKE_PROPS(ThumbnailCompression          ),    //5023
	MAKE_PROPS(ThumbnailPhotometricInterp    ),    //5024
	MAKE_PROPS(ThumbnailImageDescription     ),    //5025
	MAKE_PROPS(ThumbnailEquipMake            ),    //5026
	MAKE_PROPS(ThumbnailEquipModel           ),    //5027
	MAKE_PROPS(ThumbnailStripOffsets         ),    //5028
	MAKE_PROPS(ThumbnailOrientation          ),    //5029
	MAKE_PROPS(ThumbnailSamplesPerPixel      ),    //502A
	MAKE_PROPS(ThumbnailRowsPerStrip         ),    //502B
	MAKE_PROPS(ThumbnailStripBytesCount      ),    //502C
	MAKE_PROPS(ThumbnailResolutionX          ),    //502D
	MAKE_PROPS(ThumbnailResolutionY          ),    //502E
	MAKE_PROPS(ThumbnailPlanarConfig         ),    //502F
	MAKE_PROPS(ThumbnailResolutionUnit       ),    //5030
	MAKE_PROPS(ThumbnailTransferFunction     ),    //5031
	MAKE_PROPS(ThumbnailSoftwareUsed         ),    //5032
	MAKE_PROPS(ThumbnailDateTime             ),    //5033
	MAKE_PROPS(ThumbnailArtist               ),    //5034
	MAKE_PROPS(ThumbnailWhitePoint           ),    //5035
	MAKE_PROPS(ThumbnailPrimaryChromaticities),    //5036
	MAKE_PROPS(ThumbnailYCbCrCoefficients    ),    //5037
	MAKE_PROPS(ThumbnailYCbCrSubsampling     ),    //5038
	MAKE_PROPS(ThumbnailYCbCrPositioning     ),    //5039
	MAKE_PROPS(ThumbnailRefBlackWhite        ),    //503A
	MAKE_PROPS(ThumbnailCopyRight            ),    //503B
	MAKE_PROPS(LuminanceTable                ),    //5090
	MAKE_PROPS(ChrominanceTable              ),    //5091
	MAKE_PROPS(FrameDelay                    ),    //5100
	MAKE_PROPS(LoopCount                     ),    //5101
	MAKE_PROPS(PixelUnit                     ),    //5110
	MAKE_PROPS(PixelPerUnitX                 ),    //5111
	MAKE_PROPS(PixelPerUnitY                 ),    //5112
	MAKE_PROPS(PaletteHistogram              ),    //5113
	MAKE_PROPS(ExifExposureTime              ),    //829A
	MAKE_PROPS(ExifFNumber                   ),    //829D
	MAKE_PROPS(ExifExposureProg              ),    //8822
	MAKE_PROPS(ExifSpectralSense             ),    //8824
	MAKE_PROPS(ExifISOSpeed                  ),    //8827
	MAKE_PROPS(ExifOECF                      ),    //8828
	MAKE_PROPS(ExifVer                       ),    //9000
	MAKE_PROPS(ExifDTOrig                    ),    //9003
	MAKE_PROPS(ExifDTDigitized               ),    //9004
	MAKE_PROPS(ExifCompConfig                ),    //9101
	MAKE_PROPS(ExifCompBPP                   ),    //9102
	MAKE_PROPS(ExifShutterSpeed              ),    //9201
	MAKE_PROPS(ExifAperture                  ),    //9202
	MAKE_PROPS(ExifBrightness                ),    //9203
	MAKE_PROPS(ExifExposureBias              ),    //9204
	MAKE_PROPS(ExifMaxAperture               ),    //9205
	MAKE_PROPS(ExifSubjectDist               ),    //9206
	MAKE_PROPS(ExifMeteringMode              ),    //9207
	MAKE_PROPS(ExifLightSource               ),    //9208
	MAKE_PROPS(ExifFlash                     ),    //9209
	MAKE_PROPS(ExifFocalLength               ),    //920A
	MAKE_PROPS(ExifMakerNote                 ),    //927C
	MAKE_PROPS(ExifUserComment               ),    //9286
	MAKE_PROPS(ExifDTSubsec                  ),    //9290
	MAKE_PROPS(ExifDTOrigSS                  ),    //9291
	MAKE_PROPS(ExifDTDigSS                   ),    //9292
	MAKE_PROPS(ExifFPXVer                    ),    //A000
	MAKE_PROPS(ExifColorSpace                ),    //A001
	MAKE_PROPS(ExifPixXDim                   ),    //A002
	MAKE_PROPS(ExifPixYDim                   ),    //A003
	MAKE_PROPS(ExifRelatedWav                ),    //A004
	MAKE_PROPS(ExifInterop                   ),    //A005
	MAKE_PROPS(ExifFlashEnergy               ),    //A20B
	MAKE_PROPS(ExifSpatialFR                 ),    //A20C
	MAKE_PROPS(ExifFocalXRes                 ),    //A20E
	MAKE_PROPS(ExifFocalYRes                 ),    //A20F
	MAKE_PROPS(ExifFocalResUnit              ),    //A210
	MAKE_PROPS(ExifSubjectLoc                ),    //A214
	MAKE_PROPS(ExifExposureIndex             ),    //A215
	MAKE_PROPS(ExifSensingMethod             ),    //A217
	MAKE_PROPS(ExifFileSource                ),    //A300
	MAKE_PROPS(ExifSceneType                 ),    //A301
	MAKE_PROPS(ExifCfaPattern                ),    //A302
	MAKE_PROPS(ExifWhiteBalance              ),    //A403
	MAKE_PROPS(ExifFocalLengthIn35mmFilm     ),    //A405
    MAKE_PROPS(ExifCustomRendered            ),    //A401
    MAKE_PROPS(ExifExposureMode              ),    //A402
    MAKE_PROPS(ExifWhiteBalance              ),    //A403
    MAKE_PROPS(ExifDigitalZoomRatio          ),    //A404
    MAKE_PROPS(ExifFocalLengthIn35mmFilm     ),    //A405
    MAKE_PROPS(ExifSceneCaptureType          ),    //A406
    MAKE_PROPS(ExifGainControl               ),    //A407
    MAKE_PROPS(ExifContrast                  ),    //A408
    MAKE_PROPS(ExifSaturation                ),    //A409
    MAKE_PROPS(ExifSharpness                 ),    //A40A
    MAKE_PROPS(ExifDeviceSettingDesc         ),    //A40B
    MAKE_PROPS(ExifSubjectDistanceRange      ),    //A40C
    MAKE_PROPS(ExifUniqueImageID             ),    //A420
	MAKE_PROPS(GpsVer                        ),    //0000
	MAKE_PROPS(GpsLatitudeRef                ),    //0001
	MAKE_PROPS(GpsLatitude                   ),    //0002
	MAKE_PROPS(GpsLongitudeRef               ),    //0003
	MAKE_PROPS(GpsLongitude                  ),    //0004
	MAKE_PROPS(GpsAltitudeRef                ),    //0005
	MAKE_PROPS(GpsAltitude                   ),    //0006
	MAKE_PROPS(GpsGpsTime                    ),    //0007
	MAKE_PROPS(GpsGpsSatellites              ),    //0008
	MAKE_PROPS(GpsGpsStatus                  ),    //0009
	MAKE_PROPS(GpsGpsMeasureMode             ),    //000A
	MAKE_PROPS(GpsGpsDop                     ),    //000B
	MAKE_PROPS(GpsSpeedRef                   ),    //000C
	MAKE_PROPS(GpsSpeed                      ),    //000D
	MAKE_PROPS(GpsTrackRef                   ),    //000E
	MAKE_PROPS(GpsTrack                      ),    //000F
	MAKE_PROPS(GpsImgDirRef                  ),    //0010
	MAKE_PROPS(GpsImgDir                     ),    //0011
	MAKE_PROPS(GpsMapDatum                   ),    //0012
	MAKE_PROPS(GpsDestLatRef                 ),    //0013
	MAKE_PROPS(GpsDestLat                    ),    //0014
	MAKE_PROPS(GpsDestLongRef                ),    //0015
	MAKE_PROPS(GpsDestLong                   ),    //0016
	MAKE_PROPS(GpsDestBearRef                ),    //0017
	MAKE_PROPS(GpsDestBear                   ),    //0018
	MAKE_PROPS(GpsDestDistRef                ),    //0019
	MAKE_PROPS(GpsDestDist                   ),    //001A
	MAKE_PROPS(GpsProcMethod                 ),    //001B
	MAKE_PROPS(GpsAreaInformation            ),    //001C
	MAKE_PROPS(GpsDateStamp                  ),    //001D
	MAKE_PROPS(GpsDifferential               ),    //001E
	MAKE_PROPS(SimpleRating                  ),    //4746
	MAKE_PROPS(Rating                        ),    //4749
	MAKE_PROPS(Title                         ),    //9C9B
	MAKE_PROPS(Comment                       ),    //9C9C
	MAKE_PROPS(Author                        ),    //9C9D
	MAKE_PROPS(Keywords                      ),    //9C9E
	MAKE_PROPS(Subject                       ),    //9C9F
	MAKE_PROPS(Null) // FFFF
};

const WCHAR* vt::ImagePropertyToString(PROPID id)
{
    UINT i;
    for(i=0; g_sExifMap[i].id != ImagePropertyNull; i++)
    {
        if( g_sExifMap[i].id == id )
            return g_sExifMap[i].pszName;
    }
    return L"";
}

PROPID vt::StringToImageProperty(const WCHAR* pszName)
{
    int i;
    for(i=0; g_sExifMap[i].id != ImagePropertyNull; i++)
    {
        if(wcscmp(pszName, g_sExifMap[i].pszName)==0)
            break;
    }
    return g_sExifMap[i].id;
}


