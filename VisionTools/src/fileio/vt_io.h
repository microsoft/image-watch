//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      General image format reader/writer
//
//  History:
//      2006/1/1-erudolph
//            Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vtcore.h"

namespace vt {

//+---------------------------------------------------------------------------
// 
// File loading/saving functions
//
//----------------------------------------------------------------------------

/// \ingroup error
/// <summary> Return i/o HRESULT as string </summary>
/// <param name="hr"> HRESULT from a VisionTools i/o operation </param>
/// <param name="buf"> Buffer to write the error text to </param>
/// <param name="numBufElem"> Buffer size, in characters </param>
/// <returns> Pointer to the beginning of the text buffer </returns>
const wchar_t* VtIOErrorToString(
    HRESULT hr, 
    __out_ecount(numBufElem) wchar_t* buf, 
    int numBufElem);

/// \ingroup loadsave
/// <summary> Load image from file </summary>
/// <param name="pszFile"> File name. </param>
/// <param name="imgDst"> Destination image. </param>
/// <param name="bLoadMetadata"> If true, metadata will be loaded. </param>
/// <returns> 
///		- S_OK on success
///		- Use VtIOErrorToString() to get extended error information.
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///	The pixel format of `imgDst` is determined using \ref stdconvrules.
HRESULT VtLoadImage(const wchar_t * pszFile, CImg &imgDst, 
                    bool bLoadMetadata = true);

#if defined(MAKE_DOXYGEN_WORK) || defined(VT_WINRT)
/// \ingroup loadsave
/// <summary> Load image from file </summary>
/// <param name="storageFile"> Storage file. </param>
/// <param name="imgDst"> Destination image. </param>
/// <param name="bLoadMetadata"> If true, metadata will be loaded. </param>
/// <returns> 
///		- S_OK on success
///		- Use VtIOErrorToString() to get extended error information.
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///	- Available only in WinRT configurations.
///	- The pixel format of `imgDst` is determined using \ref stdconvrules.
HRESULT VtLoadImage(Windows::Storage::IStorageFile^ storageFile, CImg &imgDst, 
                    bool bLoadMetadata = true);

/// \ingroup loadsave
/// <summary> Load image from stream </summary>
/// <param name="stream"> Stream. </param>
/// <param name="imgDst"> Destination image. </param>
/// <param name="bLoadMetadata"> If true, metadata will be loaded. </param>
/// <returns> 
///		- S_OK on success
///		- Use VtIOErrorToString() to get extended error information.
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///	- Available only in WinRT configurations.
///	- The pixel format of `imgDst` is determined using \ref stdconvrules.
HRESULT VtLoadImage(Windows::Storage::Streams::IRandomAccessStream^ stream, CImg &imgDst, 
                    bool bLoadMetadata = true);
#endif

HRESULT VtLoadImage(const wchar_t * pszFile, IImageWriter* pWriter, 
					const CPoint* ptDst = NULL, const CRect* pRectSrc = NULL,
                    bool bLoadMetadata = true);

/// \ingroup loadsave
/// <summary> Save image to file </summary>
/// <param name="pszFile"> File name. </param>
/// <param name="imgSrc"> Source image. </param>
/// <param name="bSaveMetadata"> If true, metadata will be saved. </param>
/// <param name="pParams"> (internal) </param>
/// <param name="pProgress"> (internal) </param>
/// <returns> 
///		- S_OK on success
///		- Use VtIOErrorToString() to get extended error information.
/// </returns>
HRESULT VtSaveImage(const wchar_t * pszFile, const CImg &imgSrc, 
                    bool bSaveMetadata = true,
                    const CParams* pParams = NULL,
                    CTaskProgress* pProgress = NULL);

#if defined(MAKE_DOXYGEN_WORK) || defined(VT_WINRT)
/// \ingroup loadsave
/// <summary> Save image to file </summary>
/// <param name="storageFile"> Storage file. </param>
/// <param name="imgSrc"> Source image. </param>
/// <param name="bSaveMetadata"> If true, metadata will be saved. </param>
/// <param name="pParams"> (internal) </param>
/// <param name="pProgress"> (internal) </param>
/// <returns> 
///		- S_OK on success
///		- Use VtIOErrorToString() to get extended error information.
/// </returns>
/// <DL><DT> Remarks: </DT></DL>
///	Available only in WinRT configurations.

HRESULT VtSaveImage(Windows::Storage::IStorageFile^ storageFile, const CImg& imgSrc,
                    bool bSaveMetadata = true,
                    const CParams* pParams = NULL,
                    CTaskProgress* pProgress = NULL);
#endif

/// \ingroup loadsave
/// <summary> Save image to file, with quality setting </summary>
/// <param name="pszFile"> File name. </param>
/// <param name="imgSrc"> Source image. </param>
/// <param name="bSaveMetadata"> If true, metadata will be saved. </param>
/// <param name="quality"> Image quality, between 0 and 1. 
/// Ignored if output format doesn't support compression. </param>
/// <returns> 
///		- S_OK on success
///		- E_INVALIDARG if quality is out of range
///		- Use VtIOErrorToString() to get extended error information.
/// </returns>
HRESULT VtSaveImage(const wchar_t * pszFile, const CImg &imgSrc, 
                    bool bSaveMetadata, float quality);

HRESULT VtSaveImage(const wchar_t * pszFile, IImageReader* pReader, 
					const CRect* pRect = NULL, 
					bool bSaveMetadata = true,
                    const CParams* pParams = NULL,
                    CTaskProgress* pProgress = NULL );

#if defined(VT_OSX)
    // char* versions since wchar is not well supported in OSX
    HRESULT VtLoadImage(const char * pszFile, CImg &imgDst);
    HRESULT VtSaveImage(const char * pszFile, const CImg &imgSrc);
    HRESULT VtSaveImageToPhotosAlbum(const char * pszFile, const CImg &imgSrc);
    HRESULT VtSaveImageToDocs(const char * pszFile, const CImg &imgSrc);
#endif
};
