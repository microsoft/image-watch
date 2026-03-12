////////////////////////////////////////////////////////////////////////////////
//
//  hdpio.hpp
//
// hd photo interface functions 
// 
#pragma once

namespace vt {

//+----------------------------------------------------------------------------
//
// Class CHDPWriter
// 
// Synopsis:
//     HP Photo writing routines
// 
//-----------------------------------------------------------------------------
class CHDPWriter: public IVTImageWriter
{
public:
    CHDPWriter();
    ~CHDPWriter();

    // Clone the writer
    HRESULT Clone( IVTImageWriter **ppWriter );

    // OpenFile opens the file or stream for writing
    HRESULT OpenFile( const WCHAR * pszFile )
        { return OpenFile( NULL, pszFile ); }
    HRESULT OpenFile( IStream* pStream, const WCHAR * pszFile );

    // Save an image
    HRESULT SetImage( IStream* pStream, CRect& rect, 
					  const CParams* pParams = NULL );
    HRESULT SetImage( const CImg & cImg, bool bSaveMetadata = true,
					  const CParams* pParams = NULL,
                      CTaskProgress* pProgress = NULL );
    HRESULT SetImage( IImageReader* pReader, const CRect* pRect = NULL,
                      bool bSaveMetadata = true,
					  const CParams* pParams = NULL,
                      CTaskProgress* pProgress = NULL );

	// finish writing and close
    HRESULT CloseFile( );

private:
	void* m_pvEncodeStream;
	void* m_pvStreamConv;
};
//+----------------------------------------------------------------------------
//
// Load functions
// 
//-----------------------------------------------------------------------------
HRESULT VtLoadHDPhoto(IStream* pStream, CImg &imgDst, const CRect* pRect = NULL,
                      bool bLoadMetadata=true);
HRESULT VtLoadHDPhoto(const WCHAR * pszFile, CImg &cImg, const CRect* pRect = NULL,
					  bool bLoadMetadata = true, int iIndex = -1);

//+----------------------------------------------------------------------------
//
// Save functions
// 
//-----------------------------------------------------------------------------
HRESULT VtSaveHDPhoto(IStream* pStream, const CImg &imgSrc, 
					  bool bSaveMetadata, int iIndex, 
					  CParams* pSaveParams);
HRESULT VtSaveHDPhoto(const WCHAR * pszFile, const CImg &cImg, 
					  bool bSaveMetadata = true, int iIndex = -1, 
					  CParams* pSaveParams = NULL);
HRESULT VtSaveHDPhoto(const WCHAR * pszFile, 
                      IImageReader* pReader, const CRect* pRect = NULL,
					  bool bSaveMetadata = true, int iIndex = -1, 
					  CParams* pSaveParams = NULL);

};