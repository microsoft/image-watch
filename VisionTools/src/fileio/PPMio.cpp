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

#include "stdafx.h"
#include "PPMio.h"

using namespace vt;

// Does the specified extension match the extension of a file type supported by this file handler?
bool vt::PPMMatchExtension(const wchar_t *filetype)
{
    return
        _wcsicmp(filetype, L".pbm") == 0 ||
        _wcsicmp(filetype, L".pgm") == 0 ||
        _wcsicmp(filetype, L".ppm") == 0 ||
        _wcsicmp(filetype, L".pmf") == 0 || // Scharstein-Szeliski non-standard extension
        _wcsicmp(filetype, L".pfm") == 0;   // 1 - band PFM image, see http://netpbm.sourceforge.net/doc/pfm.html
}

// Read the PPM/PGM/PFM file
HRESULT vt::PPMLoadFile(const wchar_t * filename, CImg & img)
{
    // Open the file
    FILE *stream = NULL;
    VT_HR_BEGIN();
    _wfopen_s(&stream, filename, L"rb");
    if (stream == NULL)
        return E_INVALIDARG;

    char magic[2], c;
    magic[0] = (char)fgetc(stream);
    magic[1] = (char)fgetc(stream);

    // Check the magic number
    bool PFM = magic[1] == 'f'; // only single-band PFM is supported
    bool isFloat = magic[1] == '9' ||   // P9 for PMF, added by Rick, 01-Nov-01
        PFM;
    if (magic[0] != 'P' && magic[0] != 'p' ||
        magic[1] < '1' || magic[1] > '6' && ! isFloat) {
        VT_HR_EXIT(E_INVALIDARG);
    }
    int bits_per_pixel = ((magic[1] == '1' || magic[1] == '4') ? 1 :
        (magic[1] == '2' || magic[1] == '5') ? 8 : (isFloat) ? 32 : 24);
    bool ascii = (magic[1] < '4');

    // Bitmap mode (P1, P4) not currently supported
    if (bits_per_pixel != 8 && bits_per_pixel != 24 && bits_per_pixel != 32)
        VT_HR_EXIT(E_INVALIDARG);

    // Ascii (not raw) mode not currently supported
    if (ascii)
        VT_HR_EXIT(E_INVALIDARG);

    // Skip rest of line and also comment lines
    while ((c = (char) fgetc(stream)) != '\n');
    while ((c = (char) fgetc(stream)) == '#')  // comment line
        while ((c = (char) fgetc(stream)) != '\n');    // skip rest of line
    ungetc(c, stream);

    // Read in the dimensions
    int width, height;
    float scalef;
    if (fscanf_s(stream, (bits_per_pixel == 1) ? "%d %d" : "%d %d %f",
        &width, &height, &scalef) != 2 + (bits_per_pixel > 1) ||
        PFM && scalef > 0) { // only support littleendian, i.e., negative scalef
        VT_HR_EXIT(E_INVALIDARG);
    }

    // Skip rest of line
    if ((c = (char) fgetc(stream)) != '\n')
        ungetc(c, stream);

    // Create the appropriate image
    CByteImg byteImg;
    CRGBImg rgbImg;
    CFloatImg floatImg;
    if (bits_per_pixel == 8)
    {
        VT_HR_EXIT(byteImg.Create(width, height));
    }
    else if (bits_per_pixel == 24)
    {
        VT_HR_EXIT(rgbImg.Create(width, height));
    }
    else
    {
        VT_HR_EXIT(floatImg.Create(width, height));
    }
    CImg & tmpImg =
        (bits_per_pixel == 8) ? *(CImg *)&byteImg :
        (bits_per_pixel == 24) ? *(CImg *)&rgbImg : *(CImg *)&floatImg;

    // Read in the data, one row at a time
    for (int row = 0; row < height; row++) {
        Byte *imgdata = tmpImg.BytePtr(PFM ? height - 1 - row : row);
        int n = width * bits_per_pixel / 8;
        if (fread(imgdata, sizeof(Byte), n, stream) != (size_t) n)
            return E_FAIL;
        // Vision tools color image is stored as BGR, so swap B and R
        for (int col = 0; col < width && bits_per_pixel == 24; col++)
        {
            Byte *rgb = &imgdata[3 * col];
            Byte red = rgb[0];
            rgb[0] = rgb[2];
            rgb[2] = red;
        }
    }

    VT_HR_EXIT(tmpImg.CopyTo(img)); // do a copy and/or conversion
    VT_HR_EXIT_LABEL();
    fclose(stream);
    return hr;
}

// Write the PPM/PGM/PFM file
//
//  Note:  do we trust the file extension (e.g., PGM vs. PPM) to determine the number of bands
//   or do we look at the image type?
//  For now, since we don't expect to use this much, just use the file extension

HRESULT vt::PPMSaveFile(const wchar_t * filename, const CImg & img)
{
    // Open the file
    FILE *stream = NULL;
    VT_HR_BEGIN();
    _wfopen_s(&stream, filename, L"wb");
    if (stream == NULL)
        return E_INVALIDARG;

    // Determine the file type
    wchar_t penultimate = filename[wcslen(filename) - 2];
    bool PFM = penultimate == L'f';
    bool isFloat = PFM || penultimate == L'm';
    int bits_per_pixel = ((isFloat) ? 32 :
        (penultimate == L'b') ? 1 :
        (penultimate == L'g') ? 8 :
        (penultimate == L'p') ? 24 : 0);
    if (bits_per_pixel != 8 && bits_per_pixel != 24 && bits_per_pixel != 32)
        VT_HR_EXIT(E_INVALIDARG);     // Bitmap mode (P1, P4) not currently supported
    char magic2 = (PFM ? 'f' :
        (bits_per_pixel == 8) ? '5' :
        (bits_per_pixel == 24) ? '6' : '9');

    // Write out the header
    int width = img.Width();
    int height = img.Height();
    fprintf(stream, "P%c\n%d %d\n%d\n", magic2, img.Width(), img.Height(),
        PFM ? -1 : 255); // -1 indicates little-endian for PFM

    // Create the appropriate image
    CByteImg byteImg;
    CRGBImg rgbImg;
    CFloatImg floatImg;
    if (bits_per_pixel == 8)
    {
        VT_HR_EXIT(byteImg.Create(width, height));
    }
    else if (bits_per_pixel == 24)
    {
        VT_HR_EXIT(rgbImg.Create(width, height));
    }
    else
    {
        VT_HR_EXIT(floatImg.Create(width, height));
    }
    CImg & tmpImg =
        (bits_per_pixel == 8) ? *(CImg *)&byteImg :
        (bits_per_pixel == 24) ? *(CImg *)&rgbImg : *(CImg *)&floatImg;
    VT_HR_EXIT(img.CopyTo(tmpImg));

    // Write out the data, one row at a time
    int n = width * bits_per_pixel / 8;
    vector<Byte> line; line.resize(n);
    Byte *linePtr = &line[0];
    for (int row = 0; row < height; row++) {
        Byte *imgdata = tmpImg.BytePtr(PFM ? height - 1 - row : row);
        memcpy(linePtr, imgdata, n);

        // Vision tools color image is stored as BGR, so swap B and R
        for (int col = 0; col < width && bits_per_pixel == 24; col++)
        {
            Byte *rgb = &linePtr[3 * col];
            Byte red = rgb[0];
            rgb[0] = rgb[2];
            rgb[2] = red;
        }
        if (fwrite(linePtr, sizeof(Byte), n, stream) != (size_t)n)
            return E_FAIL;
    }
    VT_HR_EXIT_LABEL();
    fclose(stream);
    return hr;
}
