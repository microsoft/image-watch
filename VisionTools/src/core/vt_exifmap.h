//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Define the exif IDS - this is mostly just copied from GDI+ headers
//
//  History:
//      2006/09/12-mattu
//          Created
//
//------------------------------------------------------------------------
#pragma once

#include "vtcommon.h"
#include <PropIdl.h>

namespace vt {

// converts one of the properties below to/from a string

const WCHAR* ImagePropertyToString(PROPID);
PROPID       StringToImageProperty(const WCHAR*);

#define ImagePropertyNull  0xffff

// These are copied from the GDI+ spec and renamed

#define ImagePropertyExifIFD             0x8769
#define ImagePropertyGpsIFD              0x8825

#define ImagePropertyNewSubfileType      0x00FE
#define ImagePropertySubfileType         0x00FF
#define ImagePropertyImageWidth          0x0100
#define ImagePropertyImageHeight         0x0101
#define ImagePropertyBitsPerSample       0x0102
#define ImagePropertyCompression         0x0103
#define ImagePropertyPhotometricInterp   0x0106
#define ImagePropertyThreshHolding       0x0107
#define ImagePropertyCellWidth           0x0108
#define ImagePropertyCellHeight          0x0109
#define ImagePropertyFillOrder           0x010A
#define ImagePropertyDocumentName        0x010D
#define ImagePropertyImageDescription    0x010E
#define ImagePropertyEquipMake           0x010F
#define ImagePropertyEquipModel          0x0110
#define ImagePropertyStripOffsets        0x0111
#define ImagePropertyOrientation         0x0112
#define ImagePropertySamplesPerPixel     0x0115
#define ImagePropertyRowsPerStrip        0x0116
#define ImagePropertyStripBytesCount     0x0117
#define ImagePropertyMinSampleValue      0x0118
#define ImagePropertyMaxSampleValue      0x0119
#define ImagePropertyXResolution         0x011A   // Image resolution in width direction
#define ImagePropertyYResolution         0x011B   // Image resolution in height direction
#define ImagePropertyPlanarConfig        0x011C   // Image data arrangement
#define ImagePropertyPageName            0x011D
#define ImagePropertyXPosition           0x011E
#define ImagePropertyYPosition           0x011F
#define ImagePropertyFreeOffset          0x0120
#define ImagePropertyFreeByteCounts      0x0121
#define ImagePropertyGrayResponseUnit    0x0122
#define ImagePropertyGrayResponseCurve   0x0123
#define ImagePropertyT4Option            0x0124
#define ImagePropertyT6Option            0x0125
#define ImagePropertyResolutionUnit      0x0128   // Unit of X and Y resolution
#define ImagePropertyPageNumber          0x0129
#define ImagePropertyTransferFunction    0x012D
#define ImagePropertySoftwareUsed        0x0131
#define ImagePropertyDateTime            0x0132
#define ImagePropertyArtist              0x013B
#define ImagePropertyHostComputer        0x013C
#define ImagePropertyPredictor           0x013D
#define ImagePropertyWhitePoint          0x013E
#define ImagePropertyPrimaryChromaticities 0x013F
#define ImagePropertyColorMap            0x0140
#define ImagePropertyHalftoneHints       0x0141
#define ImagePropertyTileWidth           0x0142
#define ImagePropertyTileLength          0x0143
#define ImagePropertyTileOffset          0x0144
#define ImagePropertyTileByteCounts      0x0145
#define ImagePropertySubIFDs             0x014A   // TIFF-EP
#define ImagePropertyInkSet              0x014C
#define ImagePropertyInkNames            0x014D
#define ImagePropertyNumberOfInks        0x014E
#define ImagePropertyDotRange            0x0150
#define ImagePropertyTargetPrinter       0x0151
#define ImagePropertyExtraSamples        0x0152
#define ImagePropertySampleFormat        0x0153
#define ImagePropertySMinSampleValue     0x0154
#define ImagePropertySMaxSampleValue     0x0155
#define ImagePropertyTransferRange       0x0156

#define ImagePropertyJPEGProc            0x0200
#define ImagePropertyJPEGInterFormat     0x0201
#define ImagePropertyJPEGInterLength     0x0202
#define ImagePropertyJPEGRestartInterval 0x0203
#define ImagePropertyJPEGLosslessPredictors  0x0205
#define ImagePropertyJPEGPointTransforms     0x0206
#define ImagePropertyJPEGQTables         0x0207
#define ImagePropertyJPEGDCTables        0x0208
#define ImagePropertyJPEGACTables        0x0209

#define ImagePropertyYCbCrCoefficients   0x0211
#define ImagePropertyYCbCrSubsampling    0x0212
#define ImagePropertyYCbCrPositioning    0x0213
#define ImagePropertyREFBlackWhite       0x0214

#define ImagePropertyICCProfile          0x8773   // This TAG is defined by ICC
                                                  // for embedded ICC in TIFF
#define ImagePropertyGamma               0x0301
#define ImagePropertyICCProfileDescriptor 0x0302
#define ImagePropertySRGBRenderingIntent 0x0303

#define ImagePropertyImageTitle          0x0320
#define ImagePropertyCopyright           0x8298

// Extra TAGs (Like Adobe Image Information tags etc.)

#define ImagePropertyResolutionXUnit           0x5001
#define ImagePropertyResolutionYUnit           0x5002
#define ImagePropertyResolutionXLengthUnit     0x5003
#define ImagePropertyResolutionYLengthUnit     0x5004
#define ImagePropertyPrintFlags                0x5005
#define ImagePropertyPrintFlagsVersion         0x5006
#define ImagePropertyPrintFlagsCrop            0x5007
#define ImagePropertyPrintFlagsBleedWidth      0x5008
#define ImagePropertyPrintFlagsBleedWidthScale 0x5009
#define ImagePropertyHalftoneLPI               0x500A
#define ImagePropertyHalftoneLPIUnit           0x500B
#define ImagePropertyHalftoneDegree            0x500C
#define ImagePropertyHalftoneShape             0x500D
#define ImagePropertyHalftoneMisc              0x500E
#define ImagePropertyHalftoneScreen            0x500F
#define ImagePropertyJPEGQuality               0x5010
#define ImagePropertyGridSize                  0x5011
#define ImagePropertyThumbnailFormat           0x5012  // 1 = JPEG, 0 = RAW RGB
#define ImagePropertyThumbnailWidth            0x5013
#define ImagePropertyThumbnailHeight           0x5014
#define ImagePropertyThumbnailColorDepth       0x5015
#define ImagePropertyThumbnailPlanes           0x5016
#define ImagePropertyThumbnailRawBytes         0x5017
#define ImagePropertyThumbnailSize             0x5018
#define ImagePropertyThumbnailCompressedSize   0x5019
#define ImagePropertyColorTransferFunction     0x501A
#define ImagePropertyThumbnailData             0x501B// RAW thumbnail bits in
                                                     // JPEG format or RGB format
                                                     // depends on
                                                     // ImagePropertyThumbnailFormat

// Thumbnail related TAGs

#define ImagePropertyThumbnailImageWidth       0x5020  // Thumbnail width
#define ImagePropertyThumbnailImageHeight      0x5021  // Thumbnail height
#define ImagePropertyThumbnailBitsPerSample    0x5022  // Number of bits per
                                                       // component
#define ImagePropertyThumbnailCompression      0x5023  // Compression Scheme
#define ImagePropertyThumbnailPhotometricInterp 0x5024 // Pixel composition
#define ImagePropertyThumbnailImageDescription 0x5025  // Image Tile
#define ImagePropertyThumbnailEquipMake        0x5026  // Manufacturer of Image
                                                       // Input equipment
#define ImagePropertyThumbnailEquipModel       0x5027  // Model of Image input
                                                       // equipment
#define ImagePropertyThumbnailStripOffsets     0x5028  // Image data location
#define ImagePropertyThumbnailOrientation      0x5029  // Orientation of image
#define ImagePropertyThumbnailSamplesPerPixel  0x502A  // Number of components
#define ImagePropertyThumbnailRowsPerStrip     0x502B  // Number of rows per strip
#define ImagePropertyThumbnailStripBytesCount  0x502C  // Bytes per compressed
                                                       // strip
#define ImagePropertyThumbnailResolutionX      0x502D  // Resolution in width
                                                       // direction
#define ImagePropertyThumbnailResolutionY      0x502E  // Resolution in height
                                                       // direction
#define ImagePropertyThumbnailPlanarConfig     0x502F  // Image data arrangement
#define ImagePropertyThumbnailResolutionUnit   0x5030  // Unit of X and Y
                                                       // Resolution
#define ImagePropertyThumbnailTransferFunction 0x5031  // Transfer function
#define ImagePropertyThumbnailSoftwareUsed     0x5032  // Software used
#define ImagePropertyThumbnailDateTime         0x5033  // File change date and
                                                       // time
#define ImagePropertyThumbnailArtist           0x5034  // Person who created the
                                                       // image
#define ImagePropertyThumbnailWhitePoint       0x5035  // White point chromaticity
#define ImagePropertyThumbnailPrimaryChromaticities 0x5036 
                                                       // Chromaticities of
                                                       // primaries
#define ImagePropertyThumbnailYCbCrCoefficients 0x5037 // Color space transforma-
                                                       // tion coefficients
#define ImagePropertyThumbnailYCbCrSubsampling 0x5038  // Subsampling ratio of Y
                                                       // to C
#define ImagePropertyThumbnailYCbCrPositioning 0x5039  // Y and C position
#define ImagePropertyThumbnailRefBlackWhite    0x503A  // Pair of black and white
                                                       // reference values
#define ImagePropertyThumbnailCopyRight        0x503B  // CopyRight holder

#define ImagePropertyLuminanceTable            0x5090
#define ImagePropertyChrominanceTable          0x5091

#define ImagePropertyFrameDelay                0x5100
#define ImagePropertyLoopCount                 0x5101

#define ImagePropertyPixelUnit         0x5110  // Unit specifier for pixel/unit
#define ImagePropertyPixelPerUnitX     0x5111  // Pixels per unit in X
#define ImagePropertyPixelPerUnitY     0x5112  // Pixels per unit in Y
#define ImagePropertyPaletteHistogram  0x5113  // Palette histogram

// EXIF specific tag

#define ImagePropertyExifExposureTime  0x829A
#define ImagePropertyExifFNumber       0x829D

#define ImagePropertyExifExposureProg  0x8822
#define ImagePropertyExifSpectralSense 0x8824
#define ImagePropertyExifISOSpeed      0x8827
#define ImagePropertyExifOECF          0x8828

#define ImagePropertyExifVer            0x9000
#define ImagePropertyExifDTOrig         0x9003 // Date & time of original
#define ImagePropertyExifDTDigitized    0x9004 // Date & time of digital data generation

#define ImagePropertyExifCompConfig     0x9101
#define ImagePropertyExifCompBPP        0x9102

#define ImagePropertyExifShutterSpeed   0x9201
#define ImagePropertyExifAperture       0x9202
#define ImagePropertyExifBrightness     0x9203
#define ImagePropertyExifExposureBias   0x9204
#define ImagePropertyExifMaxAperture    0x9205
#define ImagePropertyExifSubjectDist    0x9206
#define ImagePropertyExifMeteringMode   0x9207
#define ImagePropertyExifLightSource    0x9208
#define ImagePropertyExifFlash          0x9209
#define ImagePropertyExifFocalLength    0x920A
#define ImagePropertyExifMakerNote      0x927C
#define ImagePropertyExifUserComment    0x9286
#define ImagePropertyExifDTSubsec       0x9290  // Date & Time subseconds
#define ImagePropertyExifDTOrigSS       0x9291  // Date & Time original subseconds
#define ImagePropertyExifDTDigSS        0x9292  // Date & TIme digitized subseconds

#define ImagePropertyExifFPXVer         0xA000
#define ImagePropertyExifColorSpace     0xA001
#define ImagePropertyExifPixXDim        0xA002
#define ImagePropertyExifPixYDim        0xA003
#define ImagePropertyExifRelatedWav     0xA004  // related sound file
#define ImagePropertyExifInterop        0xA005
#define ImagePropertyExifFlashEnergy    0xA20B
#define ImagePropertyExifSpatialFR      0xA20C  // Spatial Frequency Response
#define ImagePropertyExifFocalXRes      0xA20E  // Focal Plane X Resolution
#define ImagePropertyExifFocalYRes      0xA20F  // Focal Plane Y Resolution
#define ImagePropertyExifFocalResUnit   0xA210  // Focal Plane Resolution Unit
#define ImagePropertyExifSubjectLoc     0xA214
#define ImagePropertyExifExposureIndex  0xA215
#define ImagePropertyExifSensingMethod  0xA217
#define ImagePropertyExifFileSource     0xA300
#define ImagePropertyExifSceneType      0xA301
#define ImagePropertyExifCfaPattern     0xA302

// New EXIF 2.2 properties

#define ImagePropertyExifCustomRendered           0xA401
#define ImagePropertyExifExposureMode             0xA402
#define ImagePropertyExifWhiteBalance             0xA403
#define ImagePropertyExifDigitalZoomRatio         0xA404
#define ImagePropertyExifFocalLengthIn35mmFilm    0xA405
#define ImagePropertyExifSceneCaptureType         0xA406
#define ImagePropertyExifGainControl              0xA407
#define ImagePropertyExifContrast                 0xA408
#define ImagePropertyExifSaturation               0xA409
#define ImagePropertyExifSharpness                0xA40A
#define ImagePropertyExifDeviceSettingDesc        0xA40B
#define ImagePropertyExifSubjectDistanceRange     0xA40C
#define ImagePropertyExifUniqueImageID            0xA420

#define ImagePropertyGpsVer             0x0000
#define ImagePropertyGpsLatitudeRef     0x0001
#define ImagePropertyGpsLatitude        0x0002
#define ImagePropertyGpsLongitudeRef    0x0003
#define ImagePropertyGpsLongitude       0x0004
#define ImagePropertyGpsAltitudeRef     0x0005
#define ImagePropertyGpsAltitude        0x0006
#define ImagePropertyGpsGpsTime         0x0007
#define ImagePropertyGpsGpsSatellites   0x0008
#define ImagePropertyGpsGpsStatus       0x0009
#define ImagePropertyGpsGpsMeasureMode  0x000A
#define ImagePropertyGpsGpsDop          0x000B  // Measurement precision
#define ImagePropertyGpsSpeedRef        0x000C
#define ImagePropertyGpsSpeed           0x000D
#define ImagePropertyGpsTrackRef        0x000E
#define ImagePropertyGpsTrack           0x000F
#define ImagePropertyGpsImgDirRef       0x0010
#define ImagePropertyGpsImgDir          0x0011
#define ImagePropertyGpsMapDatum        0x0012
#define ImagePropertyGpsDestLatRef      0x0013
#define ImagePropertyGpsDestLat         0x0014
#define ImagePropertyGpsDestLongRef     0x0015
#define ImagePropertyGpsDestLong        0x0016
#define ImagePropertyGpsDestBearRef     0x0017
#define ImagePropertyGpsDestBear        0x0018
#define ImagePropertyGpsDestDistRef     0x0019
#define ImagePropertyGpsDestDist        0x001A

// The remaining values are not in the GDI+ spec.
#define ImagePropertyGpsProcMethod      0x001B
#define ImagePropertyGpsAreaInformation 0x001C
#define ImagePropertyGpsDateStamp       0x001D
#define ImagePropertyGpsDifferential    0x001E
#define ImagePropertySimpleRating       0x4746
#define ImagePropertyRating             0x4749
#define ImagePropertyTitle              0x9C9B
#define ImagePropertyComment            0x9C9C
#define ImagePropertyAuthor             0x9C9D
#define ImagePropertyKeywords           0x9C9E
#define ImagePropertySubject            0x9C9F

};
