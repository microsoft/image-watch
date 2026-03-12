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

#include "vtcommon.h"
#include "vt_io.h"
#include "vt_iointerfaces.h"
#include <PropIdl.h>
#include <propsys.h>

#if defined(VT_WINRT)
#include <robuffer.h>
#include "TempFileHelper.h"
#endif

namespace vt {

//+----------------------------------------------------------------------------
//
// Function CheckWIC
// 
// Synopsis:
//     returns true if WIC is installed, false otherwise
// 
//-----------------------------------------------------------------------------
bool CheckWIC(vt::wstring& errmsg);

//+----------------------------------------------------------------------------
//
// Function GetWICErrorString
// 
// Synopsis:
//     Gets an error message for the specified HRESULT.
//     Returns true if successful, false otherwise.
//
//-----------------------------------------------------------------------------
bool GetWICErrorString(HRESULT hr, vt::wstring& errmsg);

//+----------------------------------------------------------------------------
//
// Function GetWICDecoderFileExtensions
//
// Synopsis:
//     Fills in a vector of file extensions for which decoders are available.
//
//-----------------------------------------------------------------------------
HRESULT GetWICDecoderFileExtensions(vt::vector<vt::wstring>& fileExtensions);

//+----------------------------------------------------------------------------
//
// Function GetWICEncoderFileExtensions
//
// Synopsis:
//     Fills in a vector of file extensions for which encoders are available.
//
//-----------------------------------------------------------------------------
HRESULT GetWICEncoderFileExtensions(vt::vector<vt::wstring>& fileExtensions);

//+----------------------------------------------------------------------------
//
// Class CWicReader
// 
// Synopsis:
//     WIC reading routines
// 
//-----------------------------------------------------------------------------
class CWicReader
{
public:
    CWicReader();
    ~CWicReader();

    // OpenFile opens the file and reads meta-data
    HRESULT OpenFile( const WCHAR * pszFile );

#if defined(MAKE_DOXYGEN_WORK) || defined(VT_WINRT)
	HRESULT OpenFile( Windows::Storage::IStorageFile^ storageFile );
    HRESULT OpenFile( Windows::Storage::Streams::IRandomAccessStream^ stream );
#endif

    // OpenStream opens the stream and reads meta-data
    HRESULT OpenStream( IStream *pStream );

    HRESULT CloseFile( );

    // return meta-data
    int  Width( )  const { return m_info.width;  }
    int  Height( ) const { return m_info.height; }
    CImgInfo GetImgInfo() const { return m_info; }
    const CParams *GetMetaData() const { return &m_params; }

    // Load the image
    HRESULT GetImage( CImg & cImg, const CRect* pRect = NULL,
                      bool bLoadMetadata = true );
    HRESULT GetImage( IImageWriter* pWriter, const CPoint* ptDst = NULL,
                      const CRect* pRect = NULL,
                      bool bLoadMetadata = true );
    template <class Writer>
    HRESULT GetImage( Writer& writer, const CPoint* ptDst = NULL,
                      const CRect* pRect = NULL,
                      bool bLoadMetadata = true )
    {
        HRESULT hr = Writer.Create(m_info);
        if (hr == S_OK)
            hr = GetImage((IImageWriter*) &Writer, ptDst, pRect, bLoadMetadata);
        return hr;
    }

private:
	HRESULT InitializeFactory();
    HRESULT LoadFrame();
    HRESULT LoadPixels(IImageWriter* pWriter,
                       const CPoint* ptDst,
                       const CRect* pRect);

	CImgInfo m_info;
    CParams  m_params;

    IWICImagingFactory*    m_pFactory;
    IWICBitmapDecoder*     m_pDecoder;
    IWICBitmapFrameDecode* m_pBitmapFrame;
#if defined(VT_WINRT)
	Windows::Storage::Streams::IBufferByteAccess* m_pBufferByteAccess;
#endif
    bool                   m_bComStarted;
};

//+----------------------------------------------------------------------------
//
// Class CWicWriter
// 
// Synopsis:
//     WIC writing routines
// 
//-----------------------------------------------------------------------------
class CWicWriter: public IVTImageWriter
{
public:
    CWicWriter();
    ~CWicWriter();

	// Clone the writer
    HRESULT Clone( IVTImageWriter **ppWriter );

    // OpenFile opens the file or stream for writing
    HRESULT OpenFile( const WCHAR * pszFile )
    { return OpenFile( NULL, pszFile ); }

#if defined(MAKE_DOXYGEN_WORK) || defined(VT_WINRT)
	HRESULT OpenFile( Windows::Storage::IStorageFile^ storageFile );
#endif

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
	HRESULT InitializeFactory();
	HRESULT OpenFileInternal( IStream* pStream, const WCHAR * pszFile );

    IWICImagingFactory* m_pFactory;
    IWICStream*         m_pStream;
    IWICBitmapEncoder*  m_pEncoder;
    bool                m_bComStarted;
    vt::wstring         m_strFileName;
#if defined(VT_WINRT)
	CTempFileHelper     m_tempFileHelper;
#endif
};

//+----------------------------------------------------------------------------
//
// Class CWicMetadataReader
// 
// Synopsis:
//     Helper class to read WIC metadata from stream (e.g. from Photoshop)
// 
//-----------------------------------------------------------------------------
class CWicMetadataReader
{
public:
    CWicMetadataReader();
    ~CWicMetadataReader();

    // OpenStream opens a stream and reads the metadata.
    HRESULT OpenStream(IStream* pStream);

    VOID CloseStream();

    HRESULT GetMetadata(const CParams** ppParams);

protected:
    HRESULT LoadMetadata();

protected:
    IWICComponentFactory* m_pFactory;
    IWICStream*           m_pStream;
    CParams               m_params; // meta-data converted
    bool                  m_bComStarted;
};

//+----------------------------------------------------------------------------
//
// Class CWicMetadataWriter
// 
// Synopsis:
//     Helper class to write WIC metadata to a stream (e.g. to PSD file)
// 
//-----------------------------------------------------------------------------
class CWicMetadataWriter
{
public:
    CWicMetadataWriter();
    ~CWicMetadataWriter();

    // OpenStream opens a stream for writing metadata.
    HRESULT OpenStream(IStream* pStream);

    VOID CloseStream();

    // SetMetadata writes the metadata.
    HRESULT SetMetadata(const CParams* pParams);

protected:
    HRESULT SaveMetadata();

protected:
    IWICComponentFactory* m_pFactory;
    IStream*              m_pStream;
    CParams               m_params; // meta-data converted
    bool                  m_bComStarted;
};

};
