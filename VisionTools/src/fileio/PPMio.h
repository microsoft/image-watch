//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//    Read / write Portable Pixmap / Graymap / Floatmap (PPM, PGM, PFM) images
//
//    Only a subset of the PPM / PFM files are supported, namely:
//      raw (not ASCII) portable graymap  (.pgm),  8 bpp, monochrome, magic header = P5
//      raw (not ASCII) portable pixmap   (.ppm), 24 bpp, color RGB,  magic header = P6
//      raw (not ASCII) portable floatmap (.fpm), 32 bpp, monochrome, magic header = Pf
//      raw (not ASCII) portable floatmap (.fmf), 32 bpp, monochrome, magic header = P9
//    If 
//
//  History:
//    2015/06/22-szeliski
//      Created, based on BetaVisSDK VisPPMFileIO.cpp code
//
//------------------------------------------------------------------------

#include "vtcommon.h"
#include "vtfileio.h"

namespace vt {

    // Does the specified extension match the extension of a file type supported by this file handler?
    bool PPMMatchExtension(const wchar_t *filetype);

    // Read the PPM/PGM/PFM file
    HRESULT PPMLoadFile(const wchar_t * filename, CImg & img);

    // Write the PPM/PGM/PFM file
    HRESULT PPMSaveFile(const wchar_t * filename, const CImg & img);
}
