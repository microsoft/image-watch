//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      File IO Interfaces used by various fileio classes 
//
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"

namespace vt {

//+---------------------------------------------------------------------------
// 
// Generic interface supported by various IO objects 
//
//----------------------------------------------------------------------------
class IVTImageWriter
{
public:
    virtual ~IVTImageWriter() {};

    // Clone the writer
    virtual HRESULT Clone( IVTImageWriter **ppWriter ) = 0;

    // OpenFile opens the file or stream for writing
    virtual HRESULT OpenFile( const WCHAR * pszFile ) = 0;
	virtual HRESULT OpenFile( IStream* pStream, const WCHAR * pszType ) = 0;

    // Save the image
    virtual HRESULT SetImage( IStream* pStream, CRect& rect,
							  const CParams* pParams = NULL ) = 0;
    virtual HRESULT SetImage( const CImg & cImg, bool bSaveMetadata = true,
							  const CParams* pParams = NULL,
                              CTaskProgress* pProgress = NULL ) = 0;
    virtual HRESULT SetImage( IImageReader* pReader, const CRect* pRect = NULL,
                              bool bSaveMetadata = true,
							  const CParams* pParams = NULL,
                              CTaskProgress* pProgress = NULL ) = 0;

    // Close the file
    virtual HRESULT CloseFile( ) = 0;
};

};
