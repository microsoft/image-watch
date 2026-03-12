//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Interfaces to access portions of an image or image list
//
//  History:
//      2007/06/1-mattu
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_image.h"
#include "vt_layer.h"
#include "vt_params.h"

namespace vt {

// forward decls
HRESULT
CreateImageForTransform(CImg& dst, int iWidth, int iHeight, int iDefaultType);

HRESULT
VtConvertImage(CImg &cDst, const CImg &cSrc, bool bBypassCache);

//+-----------------------------------------------------------------------------
//
// function: TotalPixelCount
//
// Synopsis: function that returns the total number of pixels in an 
//           indexed reader/writer
//
//------------------------------------------------------------------------------
template <class T>
UINT64 TotalPixelCount(T* pIndexed)
{ 
	UINT nFrames = pIndexed->GetFrameCount(); 
	UINT64 uPixelCount = 0;
	for( UINT i = 0; i < nFrames; i++ )
	{
		CImgInfo infoDst = pIndexed->GetImgInfo(i);
		uPixelCount += UINT64(infoDst.width)*UINT64(infoDst.height);
	}
	return uPixelCount;
}

template <class T>
UINT64 TotalByteCount(T* pIndexed)
{ 
	UINT nFrames = pIndexed->GetFrameCount(); 
	UINT64 uByteCount = 0;
	for( UINT i = 0; i < nFrames; i++ )
	{
		CImgInfo info = pIndexed->GetImgInfo(i);
		uByteCount += UINT64(info.width)*UINT64(info.height)*
			UINT64(info.PixSize());
	}
	return uByteCount;
}

template <class T>
UINT64 TotalPixelCount(const vt::vector<T*>* pv)
{
	UINT64 uPixelCount = 0;
	for( int j = 0; j < (int)pv->size(); j++  )
	{
		uPixelCount += TotalPixelCount((*pv)[j]);
	}
	return uPixelCount;
}

template <class T>
UINT64 TotalByteCount(const vt::vector<T*>* pv)
{
	UINT64 uByteCount = 0;
	for( int j = 0; j < (int)pv->size(); j++  )
	{
		uByteCount += TotalByteCount((*pv)[j]);
	}
	return uByteCount;
}


//+-----------------------------------------------------------------------------
//
// Class: IImageReader
//
// Synopsis: interface for getting image information and portions of images
//
//------------------------------------------------------------------------------
class IImageReader
{
public:
	virtual ~IImageReader() {} 

	virtual CLayerImgInfo GetImgInfo(UINT uLevel = 0) = 0;

	virtual HRESULT GetMetaData(OUT CParams& params) = 0;

	virtual HRESULT ReadRegion(IN const CRect& region, OUT CImg& dst, 
							   UINT uLevel = 0) = 0;

	virtual HRESULT ReadImg(OUT CImg& dst, UINT uLevel=0)
	{ return ReadRegion(GetImgInfo(uLevel).Rect(), dst, uLevel); }

	virtual HRESULT Prefetch(IN const CRect& /*region*/, UINT /*uLevel*/ = 0)
	{ return S_OK;  }
};

//+-----------------------------------------------------------------------------
//
// Class: IImageWriter
// 
// Synopsis: interface for writing an image or portion of an image
//
//------------------------------------------------------------------------------
class IImageWriter
{
public:
	virtual ~IImageWriter()	{}
 
	virtual CLayerImgInfo GetImgInfo(UINT uLevel = 0) = 0;
	virtual HRESULT  WriteImg(IN const CImg& src, UINT uLevel=0) = 0;  
	virtual HRESULT  WriteRegion(IN const CRect& region, IN const CImg& src,
								 UINT uLevel=0) = 0;
	virtual HRESULT  Fill(const void* pbValue, const RECT *prct = NULL,
						  UINT uLevel = 0) = 0;
	virtual HRESULT  SetMetaData(const CParams *pParams) = 0;
};

//+-----------------------------------------------------------------------------
//
// Class: IImageReaderWriter
// 
// Synopsis: combined reader/writer interface
//
//------------------------------------------------------------------------------
class IImageReaderWriter : public IImageWriter, public IImageReader
{
public:
	virtual ~IImageReaderWriter() {}

	virtual CLayerImgInfo GetImgInfo(UINT uLevel = 0) = 0;
};

//+-----------------------------------------------------------------------------
//
// Class: CImgReaderWriter
//
// Synopsis: an implementation of IImageReaderWriter that simply wraps a 
//           contained image and adds members of CLayerImgInfo
// 
//------------------------------------------------------------------------------
template <class Img> 
class CImgReaderWriter: public Img, public IImageReaderWriter
{
public:
    CImgReaderWriter() : m_bLayerValid(false) {}
	
private:
	CImgReaderWriter(const CImgReaderWriter&);
	CImgReaderWriter& operator=(const CImgReaderWriter&);

public:
	// if base Img is a CImg
	HRESULT Create(const CLayerImgInfo& info)
	{
		SetLayerInfo(info.compositeWidth, info.compositeHeight,
				     info.origin, info.compositeWrapX, info.compositeWrapY);
		return Img::Create(info);
	}
 
	// if base Img is a CTypedImg
	HRESULT Create(int iW, int iH, int iBands, int iCompW, int iCompH, 
				   const CPoint& origin, bool bCompWrapX=false, bool bCompWrapY=false)
	{
		SetLayerInfo(iCompW, iCompH, origin, bCompWrapX, bCompWrapY);
		return Img::Create(iW, iH, iBands);
	}
	 
	// if base Img is a CCompositeImg 
	HRESULT Create(int iW, int iH, int iCompW, int iCompH, const CPoint& origin, 
				   bool bCompWrapX=false, bool bCompWrapY=false)
	{
		SetLayerInfo(iCompW, iCompH, origin, bCompWrapX, bCompWrapY);
		return Img::Create(iW, iH);
	}

public:
	// IImageReader implementation
	virtual CLayerImgInfo GetImgInfo(UINT uLevel=0)
	{ 
		return ( uLevel != 0 )?     CLayerImgInfo(): 
			   ( !m_bLayerValid )?  CLayerImgInfo(Img::GetImgInfo()):
			                        CLayerImgInfo(Img::GetImgInfo(), m_iCompW, m_iCompH, 
								                  m_locComp, m_bCompWrapX, m_bCompWrapY);
    }

	virtual HRESULT GetMetaData(OUT CParams& params)
    {
		const CParams* pSrc = Img::GetMetaData();
		if( pSrc )
		{
			params = *pSrc;
		}
		return S_OK; 
	}

	virtual HRESULT ReadImg(OUT CImg& dst, UINT uLevel=0)
    { return ReadRegion(Img::Rect(), dst, uLevel); }

	virtual HRESULT ReadRegion(IN const CRect& region, OUT CImg& dst,
							   UINT uLevel=0)
	{	
		HRESULT hr = E_INVALIDARG;
		if( uLevel == 0 )
		{
			CImgInfo info = GetImgInfo();
			hr = CreateImageForTransform(dst, region.Width(), region.Height(),
										 info.type);
			if( hr == S_OK )
			{
				Img imgRgn;
                Img::Share(imgRgn, region);
				hr = VtConvertImage(dst, imgRgn, false);
			}
		}
		return hr;
	}

	// IImageWriter implementation
	virtual HRESULT WriteImg(IN const CImg& src, UINT uLevel=0)
    { return ( uLevel==0 )? WriteRegion(Img::Rect(), src): E_INVALIDARG; }

	virtual HRESULT WriteRegion(IN const CRect& region, IN const CImg& src,
								UINT uLevel=0)
	{
		HRESULT hr = E_INVALIDARG;
		if( uLevel == 0 ) 
		{
			CImg imgDstRgn;
            Img::Share(imgDstRgn, region);
			hr = VtConvertImage(imgDstRgn, src, true);
		}
		return hr;
	}

	virtual HRESULT  Fill(const void* pbValue, const RECT *prct = NULL,
						  UINT uLevel = 0)
    { return ( uLevel==0 )? CImg::Fill((const Byte*)pbValue, prct): E_INVALIDARG; }

	virtual HRESULT  SetMetaData(const CParams *pParams)
	{ return Img::SetMetaData(pParams); }

public:
	void SetLayerInfo(int iCompW, int iCompH, const CPoint& origin, 
					  bool bCompWrapX=false, bool bCompWrapY=false)
	{
		m_bLayerValid = true;
		m_iCompW  = iCompW;
		m_iCompH  = iCompH;
		m_locComp = origin;
		m_bCompWrapX = bCompWrapX;
		m_bCompWrapY = bCompWrapY;
	}

protected:
	bool         m_bLayerValid;
	int          m_iCompW;    // the composite width and height
	int          m_iCompH;
	vt::CPoint   m_locComp;   // location of this image in the composite   
	bool         m_bCompWrapX: 1; // indicates that the composite wraps
	bool         m_bCompWrapY: 1; 
};

//+-----------------------------------------------------------------------------
//
// Class: CPyramidReader
//
// Synopsis: an implementation of IImageReader that wraps a pyramid
// 
//------------------------------------------------------------------------------
template <class Pyr> 
class CPyramidReader: public Pyr, public IImageReader
{
public:
	// IImageReader implementation
	virtual CLayerImgInfo GetImgInfo(UINT uLevel=0)
	{ return CLayerImgInfo(Pyr::GetImgInfo(uLevel)); }

	virtual HRESULT GetMetaData(OUT CParams& params)
	{ params = *Pyr::GetMetaData(); return S_OK; }

	virtual HRESULT ReadImg(OUT CImg& dst, UINT uLevel=0)
	{ return ReadRegion(GetImgInfo(uLevel).Rect(), dst, uLevel); }

	virtual HRESULT ReadRegion(IN const CRect& region, OUT CImg& dst,
							   UINT uLevel=0)
	{ 
		CImgInfo info = GetImgInfo();
		HRESULT hr = CreateImageForTransform(dst, region.Width(), region.Height(),
											 info.type);
		if( hr == S_OK )
		{
			CImg imgRgn;
			Pyr::GetLevel(uLevel).Share(imgRgn, region);
			hr = VtConvertImage(dst, imgRgn, false);
		}
		return hr;
    }
};

//+-----------------------------------------------------------------------------
//
// Class: IIndexedImageReader
// 
// Synopsis: a reader that can index into multiple frames
//
//------------------------------------------------------------------------------
class IIndexedImageReader
{
public:
	virtual ~IIndexedImageReader()
	{} 

	virtual CLayerImgInfo GetImgInfo(UINT uIndex, UINT uLevel = 0) = 0;

	virtual HRESULT GetMetaData(UINT uIndex, CParams& params) = 0;

	virtual HRESULT ReadRegion(UINT uIndex, IN const CRect& region, 
							   OUT CImg& dst, UINT uLevel = 0) = 0;

	virtual HRESULT ReadImg(UINT uIndex, OUT CImg& dst, UINT uLevel=0)
	{ return ReadRegion(uIndex, GetImgInfo(uIndex, uLevel).Rect(), dst, uLevel); }

	virtual HRESULT Prefetch(UINT /*uIndex*/, IN const CRect& /*region*/, 
							 UINT uLevel = 0)
	{ UNREFERENCED_PARAMETER(uLevel); return S_OK;  }

	virtual UINT    GetFrameCount()      = 0;
};

//+-----------------------------------------------------------------------------
//
// Class: IIndexedImageWriter
// 
// Synopsis: a writer that can index into multiple frames
//
//------------------------------------------------------------------------------
class IIndexedImageWriter
{
public:
	virtual ~IIndexedImageWriter()
	{}
	 
	virtual CLayerImgInfo GetImgInfo(UINT uIndex, UINT uLevel = 0) = 0;

	virtual HRESULT  WriteImg(UINT uIndex, IN const CImg& src,
							  UINT uLevel=0)
	{ return WriteRegion(uIndex, GetImgInfo(uIndex, uLevel).Rect(), src, uLevel); }

	virtual HRESULT  WriteRegion(UINT uIndex, IN const CRect& region,
								 IN const CImg& src, UINT uLevel=0) = 0;

	virtual HRESULT  Fill(UINT uIndex, const void* pbValue, 
						  const RECT *prct = NULL, UINT uLevel = 0) = 0;

	virtual HRESULT  SetMetaData(UINT uIndex, IN const CParams* pParams) = 0;

	virtual UINT     GetFrameCount() = 0;
};

//+-----------------------------------------------------------------------------
//
// Class: IIndexedImageReaderWriter
// 
// Synopsis: combined reader/writer interface
//
//------------------------------------------------------------------------------
class IIndexedImageReaderWriter : public IIndexedImageWriter, 
	                              public IIndexedImageReader
{
public:
	virtual ~IIndexedImageReaderWriter() {};
	virtual CLayerImgInfo GetImgInfo(UINT uIndex, UINT uLevel = 0) = 0;
	virtual UINT GetFrameCount() = 0;
};

//+-----------------------------------------------------------------------------
//
// function: CheckIndexImpl
// 
// Synopsis: helper function for implementors of IndexedImage iterators that
//           checks the index against the available frame count
//
//------------------------------------------------------------------------------
template<class T>
HRESULT CheckIndexImpl(T* p, UINT srcindex )
{
	return ( srcindex < p->GetFrameCount() )? S_OK: E_INVALIDARG; 
}

//+-----------------------------------------------------------------------------
//
// Class: CIndexedImageReaderIterator
// 
// Synopsis: an iterator class to step through indexed readers
//
//------------------------------------------------------------------------------
class CIndexedImageReaderIterator: public IImageReader
{
public:
	// IImageReader implementation
	virtual CLayerImgInfo GetImgInfo(UINT uLevel=0)
	{ return m_pSrc? m_pSrc->GetImgInfo(m_uCurIndex, LevelOffset(uLevel)): CLayerImgInfo(); }

	virtual HRESULT GetMetaData(OUT CParams& params)
	{ return m_pSrc? m_pSrc->GetMetaData(m_uCurIndex, params): E_NOINIT; }

	virtual HRESULT ReadImg(OUT CImg& dst, UINT uLevel=0)
	{ return m_pSrc? m_pSrc->ReadImg(m_uCurIndex, dst, LevelOffset(uLevel)): E_NOINIT; }

	virtual HRESULT ReadRegion(IN const CRect& region, OUT CImg& dst,
							   UINT uLevel=0)
	{ 
		return m_pSrc? 
			m_pSrc->ReadRegion(m_uCurIndex, region, dst, LevelOffset(uLevel)): E_NOINIT;
    }
	
	virtual HRESULT Prefetch(IN const CRect& region, UINT uLevel=0)
	{ return m_pSrc? m_pSrc->Prefetch(m_uCurIndex, region, LevelOffset(uLevel)): E_NOINIT; }

public:
	CIndexedImageReaderIterator() : m_pSrc(NULL), m_uCurIndex(0), m_uLevel(0)
	{}

	CIndexedImageReaderIterator(IIndexedImageReader* pSrc, UINT uIndex=0, 
								UINT uLevel=0): 
		m_pSrc(pSrc), m_uCurIndex(uIndex), m_uLevel(uLevel)
	{}

	void SetSource(IIndexedImageReader* pSrc, UINT uIndex=0, UINT uLevel=0)
	{ 
		m_pSrc      = pSrc;
		m_uCurIndex = uIndex;
		m_uLevel    = uLevel;
	}

	IIndexedImageReader* GetSource()
    { return m_pSrc; }

	virtual HRESULT SetIndex(UINT uIndex)
	{ 
		HRESULT hr = CheckIndexImpl(this, uIndex);
		if( hr == S_OK )
		{
			m_uCurIndex = uIndex;
		}
		return hr;
	}

	virtual UINT GetIndex()
	{ return m_uCurIndex; }

	virtual UINT GetFrameCount()
	{ return m_pSrc? m_pSrc->GetFrameCount(): 0; }

protected:
	UINT LevelOffset(UINT uLevel)
	{ return m_uLevel+uLevel; }

protected:
	IIndexedImageReader* m_pSrc;
	UINT                 m_uCurIndex;
	UINT                 m_uLevel;
};

//+-----------------------------------------------------------------------------
//
// Class: CImageReaderList
// 
// Synopsis: an IIndexedImageReader that wraps a collection of images
//
//------------------------------------------------------------------------------
template <class T>
class CImageReaderList: public IIndexedImageReader
{
public:
	// IIndexedImageReader implementation
	virtual UINT GetFrameCount()
	{ return (UINT)m_vecStore.size(); }

	virtual CLayerImgInfo GetImgInfo(UINT uIndex, UINT uLevel=0)
	{ return GetImg(uIndex).GetImgInfo(uLevel); }

	virtual HRESULT GetMetaData(UINT uIndex, OUT CParams& params)
	{ return GetImg(uIndex).GetMetaData(params);	}

	virtual HRESULT ReadImg(UINT uIndex, OUT CImg& dst, UINT uLevel=0)
	{ return GetImg(uIndex).ReadImg(dst, uLevel);	}

	virtual HRESULT ReadRegion(UINT uIndex, IN const CRect& region, OUT CImg& dst,
							   UINT uLevel=0)
	{ return GetImg(uIndex).ReadRegion(region, dst, uLevel);	}

	virtual HRESULT Prefetch(UINT uIndex, IN const CRect& region, 
							 UINT uLevel = 0)
	{ return GetImg(uIndex).Prefetch(region, uLevel);  }

public:
	HRESULT Create(UINT uSize)
	{ return m_vecStore.resize(uSize); }

	T& GetImg(UINT uIndex)
	{ 
		VT_ASSERT( uIndex<m_vecStore.size() );
		return m_vecStore[uIndex];
	}

	void Clear()
	{ m_vecStore.clear(); }

protected:
	vt::vector<T> m_vecStore;
};

//+-----------------------------------------------------------------------------
//
// Class: CIndexedImageWriterIterator
// 
// Synopsis: an iterator class to step through indexed writers
//
//------------------------------------------------------------------------------
class CIndexedImageWriterIterator: public IImageWriter
{
public:
	CIndexedImageWriterIterator() : m_pDst(NULL), m_uCurIndex(0)
	{}
 
	CIndexedImageWriterIterator(IIndexedImageWriter* pSrc, UINT uIndex=0): 
		m_pDst(pSrc), m_uCurIndex(uIndex)
	{}

	// IImageWriter implementation
	virtual CLayerImgInfo GetImgInfo(UINT uLevel = 0)
	{ return m_pDst? m_pDst->GetImgInfo(m_uCurIndex, uLevel): CLayerImgInfo(); }

	virtual HRESULT  WriteImg(IN const CImg& src, UINT uLevel=0)
	{ return m_pDst? m_pDst->WriteImg(m_uCurIndex, src, uLevel): E_NOINIT; }

	virtual HRESULT  WriteRegion(IN const CRect& r, IN const CImg& src,
								 UINT uLevel=0)
	{ return m_pDst? m_pDst->WriteRegion(m_uCurIndex, r, src, uLevel): E_NOINIT; }

	virtual HRESULT  Fill(const void* pbValue, const RECT *prct = NULL,
						  UINT uLevel = 0)
	{  return m_pDst? m_pDst->Fill(m_uCurIndex, pbValue, prct, uLevel): E_NOINIT; }

	virtual HRESULT  SetMetaData(const CParams* pParams)
	{ return m_pDst? m_pDst->SetMetaData(m_uCurIndex, pParams): E_NOINIT; }

public:
	void SetDest(IIndexedImageWriter* pDst, UINT uIndex=0)
	{ 
		m_pDst      = pDst;
		m_uCurIndex = uIndex;
	}

	IIndexedImageWriter* GetDest()
    { return m_pDst; }

	virtual HRESULT SetIndex(UINT uIndex)
	{ 
		HRESULT hr = CheckIndexImpl(this, uIndex);
		if( hr == S_OK )
		{
			m_uCurIndex = uIndex;
		}
		return hr;
	}

	virtual UINT GetIndex()
	{ return m_uCurIndex; }

	virtual UINT GetFrameCount()
	{ return m_pDst? m_pDst->GetFrameCount(): 0; }

protected:
	IIndexedImageWriter* m_pDst;
	UINT                 m_uCurIndex;
};

//+-----------------------------------------------------------------------------
//
// Class: CIndexedImageReaderWriterIterator
// 
// Synopsis: an iterator class to step through indexed reader/writer
//
//------------------------------------------------------------------------------
class CIndexedImageReaderWriterIterator: public IImageReaderWriter
{
public:
	CIndexedImageReaderWriterIterator() : m_pIRW(NULL), m_uCurIndex(0)
	{}
 
	CIndexedImageReaderWriterIterator(IIndexedImageReaderWriter* pIRW, 
                                      UINT uIndex=0): 
        m_pIRW(pIRW), m_uCurIndex(uIndex)
	{}

	// IImageReader implementation
	virtual CLayerImgInfo GetImgInfo(UINT uLevel=0)
	{ return m_pIRW? m_pIRW->GetImgInfo(m_uCurIndex, uLevel): CLayerImgInfo(); }

	virtual HRESULT GetMetaData(OUT CParams& params)
	{ return m_pIRW? m_pIRW->GetMetaData(m_uCurIndex, params): E_NOINIT; }

	virtual HRESULT ReadImg(OUT CImg& dst, UINT uLevel=0)
	{ return m_pIRW? m_pIRW->ReadImg(m_uCurIndex, dst, uLevel): E_NOINIT; }

	virtual HRESULT ReadRegion(IN const CRect& region, OUT CImg& dst,
							   UINT uLevel=0)
	{ 
		return m_pIRW? 
			m_pIRW->ReadRegion(m_uCurIndex, region, dst, uLevel): E_NOINIT;
    }
	
	virtual HRESULT Prefetch(IN const CRect& region, UINT uLevel=0)
	{ return m_pIRW? m_pIRW->Prefetch(m_uCurIndex, region, uLevel): E_NOINIT; }

	// IImageWriter implementation
	virtual HRESULT  WriteImg(IN const CImg& src, UINT uLevel=0)
	{ return m_pIRW? m_pIRW->WriteImg(m_uCurIndex, src, uLevel): E_NOINIT; }

	virtual HRESULT  WriteRegion(IN const CRect& r, IN const CImg& src,
								 UINT uLevel=0)
	{ return m_pIRW? m_pIRW->WriteRegion(m_uCurIndex, r, src, uLevel): E_NOINIT; }

	virtual HRESULT  Fill(const void* pbValue, const RECT *prct = NULL,
						  UINT uLevel = 0)
	{  return m_pIRW? m_pIRW->Fill(m_uCurIndex, pbValue, prct, uLevel): E_NOINIT; }

	virtual HRESULT  SetMetaData(const CParams* pParams)
	{ return m_pIRW? m_pIRW->SetMetaData(m_uCurIndex, pParams): E_NOINIT; }

public:
	void SetSource(IIndexedImageReaderWriter* pIRW, UINT uIndex=0)
	{ 
		m_pIRW  = pIRW;
		m_uCurIndex = uIndex;
	}

	IIndexedImageReaderWriter* GetSource()
    { return m_pIRW; }

	virtual HRESULT SetIndex(UINT uIndex)
	{ 
		HRESULT hr = CheckIndexImpl(this, uIndex);
		if( hr == S_OK )
		{
			m_uCurIndex = uIndex;
		}
		return hr;
	}

	virtual UINT GetIndex()
	{ return m_uCurIndex; }

	virtual UINT GetFrameCount()
	{ return m_pIRW? m_pIRW->GetFrameCount(): 0; }

protected:
	IIndexedImageReaderWriter* m_pIRW;
	UINT                       m_uCurIndex;
};

//+-----------------------------------------------------------------------------
//
// Class: CLevelChangeReader
// 
// Synopsis: an image reader presents the sub-level of a contained image as 
//           level 0
//
//------------------------------------------------------------------------------
class CLevelChangeReader: public IImageReader
{
	// IImageReader
public:
	virtual CLayerImgInfo GetImgInfo(UINT uLevel=0)
	{ return m_pSrc->GetImgInfo(LevelOffset(uLevel)); }

	virtual HRESULT GetMetaData(OUT CParams &params)
	{ return m_pSrc->GetMetaData(params); }
 
	virtual HRESULT ReadImg(OUT CImg& dst, UINT uLevel=0)
	{ return m_pSrc->ReadImg(dst, LevelOffset(uLevel)); }

	virtual HRESULT ReadRegion(const CRect &region, OUT CImg &dst, UINT uLevel=0)
	{ return m_pSrc->ReadRegion(region, dst, LevelOffset(uLevel)); }

	virtual HRESULT Prefetch(const CRect &region, UINT uLevel=0)
    { return m_pSrc->Prefetch(region, LevelOffset(uLevel)); }

public:
	CLevelChangeReader() : m_pSrc(NULL), m_uLevel(0)
	{}

	CLevelChangeReader(IImageReader* pSrc, UINT uLevel) : 
		m_pSrc(pSrc), m_uLevel(uLevel)
	{}

	void SetSource(IImageReader* pSrc, UINT uLevel)
	{ 
		m_pSrc   = pSrc;
		m_uLevel = uLevel;
	}

protected:
	UINT LevelOffset(UINT uLevel)
	{ return m_uLevel+uLevel; }

protected:
	IImageReader* m_pSrc;
	UINT          m_uLevel;
};

//+-----------------------------------------------------------------------------
//
// Class: CIndexedLevelChangeReader
// 
// Synopsis: an indexed image reader presents the sub-level of a contained 
//           image as level 0
//
//------------------------------------------------------------------------------
class CIndexedLevelChangeReader: public IIndexedImageReader
{
	// IIndexedImageReader
public:
	virtual UINT GetFrameCount() 
	{ return m_pSrc->GetFrameCount(); }

	virtual CLayerImgInfo GetImgInfo(UINT uIndex, UINT uLevel=0)
	{ return m_pSrc->GetImgInfo(uIndex, LevelOffset(uLevel)); }

	virtual HRESULT GetMetaData(UINT uIndex, OUT CParams &params)
	{ return m_pSrc->GetMetaData(uIndex, params); }
 
	virtual HRESULT ReadImg(UINT uIndex, OUT CImg& dst, UINT uLevel=0)
	{ return m_pSrc->ReadImg(uIndex, dst, LevelOffset(uLevel)); }

	virtual HRESULT ReadRegion(UINT uIndex, const CRect &region, OUT CImg &dst, 
							   UINT uLevel=0)
	{ return m_pSrc->ReadRegion(uIndex, region, dst, LevelOffset(uLevel)); }

	virtual HRESULT Prefetch(UINT uIndex, const CRect &region, UINT uLevel=0)
    { return m_pSrc->Prefetch(uIndex, region, LevelOffset(uLevel)); }

public:
	CIndexedLevelChangeReader() :
		m_pSrc(NULL), m_uLevel(0)
	{}

	CIndexedLevelChangeReader(IIndexedImageReader* pSrc, UINT uLevel) :
		m_pSrc(pSrc), m_uLevel(uLevel)
	{}

	void Initialize(IIndexedImageReader* pSrc, UINT uLevel)
	{
		m_pSrc   = pSrc;
		m_uLevel = uLevel;
	}

protected:
	UINT LevelOffset(UINT uLevel)
	{ return m_uLevel+uLevel; }

protected:
	IIndexedImageReader* m_pSrc;
	UINT          m_uLevel;
};

//+-----------------------------------------------------------------------------
//
// Class: CLevelChangeWriter
// 
// Synopsis: an image writer that presents the sub-level of a contained image as 
//           level 0
//
//------------------------------------------------------------------------------
class CLevelChangeWriter: public IImageWriter
{
	// IImageWriter
public:
	virtual CLayerImgInfo GetImgInfo(UINT uLevel=0)
	{ return m_pSrc->GetImgInfo(LevelOffset(uLevel)); }

	virtual HRESULT  WriteImg(IN const CImg& src, UINT uLevel=0)
	{ return m_pSrc? m_pSrc->WriteImg(src, LevelOffset(uLevel)): E_NOINIT; }

	virtual HRESULT  WriteRegion(IN const CRect& r, IN const CImg& src,
								 UINT uLevel=0)
	{ return m_pSrc? m_pSrc->WriteRegion(r, src, LevelOffset(uLevel)): E_NOINIT; }

	virtual HRESULT  Fill(const void* pbValue, const RECT *prct = NULL,
						  UINT uLevel = 0)
	{  return m_pSrc? m_pSrc->Fill(pbValue, prct, LevelOffset(uLevel)): E_NOINIT; }

	virtual HRESULT  SetMetaData(const CParams* pParams)
	{ return m_pSrc? m_pSrc->SetMetaData(pParams): E_NOINIT; }

public:
	CLevelChangeWriter(IImageWriter* pSrc, UINT uLevel) : 
		m_pSrc(pSrc), m_uLevel(uLevel)
	{}

protected:
	UINT LevelOffset(UINT uLevel)
	{ return m_uLevel+uLevel; }

protected:
	IImageWriter* m_pSrc;
	UINT          m_uLevel;
};

//+-----------------------------------------------------------------------------
//
// Class: CCropIndexedImageReader
// 
// Synopsis: an indexed image reader that crops a provided indexed image reader
//
//------------------------------------------------------------------------------
class CCropIndexedImageReader: public IIndexedImageReader
{
public:
	// IIndexedImageReader
	virtual UINT GetFrameCount() 
	{ return (UINT)m_vecInfo.size(); }

	virtual CLayerImgInfo GetImgInfo(UINT uIndex, UINT uLevel=0)
	{ return ValidArgs(uIndex, uLevel)? m_vecInfo[uIndex].li: CLayerImgInfo(); }

	virtual HRESULT GetMetaData(UINT uIndex, OUT CParams &params)
	{
		return ValidArgs(uIndex)? 
			m_pSrc->GetMetaData(m_vecInfo[uIndex].uSrcIndex, params): 
			E_INVALIDARG;
	}
 
	virtual HRESULT ReadImg(UINT uIndex, OUT CImg& dst, UINT uLevel=0)
	{ 
		HRESULT hr = E_INVALIDARG;
		if( ValidArgs(uIndex, uLevel) )
		{
			LINFO li = m_vecInfo[uIndex];
			hr = m_pSrc->ReadRegion(
				li.uSrcIndex, vt::CRect(li.li.Rect())+li.ptSrc, dst, uLevel); 
		}
		return hr;
	}

	virtual HRESULT ReadRegion(UINT uIndex, const CRect &region, OUT CImg &dst, 
							   UINT uLevel=0)
	{ 
		HRESULT hr = E_INVALIDARG;
		if( ValidArgs(uIndex, uLevel) )
		{
			LINFO li = m_vecInfo[uIndex];
			hr = m_pSrc->ReadRegion(li.uSrcIndex, region+li.ptSrc, dst, uLevel); 
		}
        return hr;
	}

	virtual HRESULT Prefetch(UINT uIndex, const CRect &region, UINT uLevel=0)
    {
		HRESULT hr = E_INVALIDARG;
		if( ValidArgs(uIndex, uLevel) )
		{
			LINFO li = m_vecInfo[uIndex];
			hr = m_pSrc->Prefetch(li.uSrcIndex, region+li.ptSrc, uLevel); 
		}
        return hr;
	}

	// implementation specific stuff
public:
	CCropIndexedImageReader() : m_pSrc(NULL)
	{}

	HRESULT Initialize(IIndexedImageReader* pSrc, const vt::CRect& rctCrop)
	{
		m_pSrc       = pSrc;

		UINT uSrcCnt = pSrc->GetFrameCount();
		HRESULT hr   = m_vecInfo.reserve(uSrcCnt);
		m_vecInfo.resize(0);
		for( UINT i = 0; i < uSrcCnt && hr==S_OK; i++ )
		{
			vt::CRect rcts[4];
			CLayerImgInfo lisrc = pSrc->GetImgInfo(i);
			int iRctCnt = SplitLayerRect(rcts, lisrc);

			for( int j = 0; j < iRctCnt && hr==S_OK; j++ )
			{
				vt::CRect r = rcts[j];
				if( r.IntersectRect(&r, &rctCrop) )
				{
					LINFO ln;
					ln.uSrcIndex = i;
					ln.ptSrc     = CompositeRectToImgRect(r, lisrc).TopLeft();
					ln.li        = CLayerImgInfo(
						CImgInfo(r.Width(), r.Height(), lisrc.type),
						lisrc.compositeWidth, lisrc.compositeHeight, r.TopLeft(), 
						lisrc.compositeWrapX, lisrc.compositeWrapY); 
					hr = m_vecInfo.push_back(ln);
				}
			}
		}
        return hr;
	}

protected:
	struct LINFO
	{
		CLayerImgInfo li;
		UINT          uSrcIndex;
		vt::CPoint    ptSrc;
	};

protected:
	bool ValidArgs(UINT uIndex, UINT uLevel=0)
	{ return uIndex < GetFrameCount() && uLevel == 0; }

protected:
	IIndexedImageReader* m_pSrc;
	vt::vector<LINFO>    m_vecInfo;
};

//+-----------------------------------------------------------------------------
//
// Class: CCropImageReaderWriter
// 
// Synopsis: an image reader/writer that crops a provided image reader/writer
//
//------------------------------------------------------------------------------
class CCropImageReaderWriter: public IImageReaderWriter
{
public:
	// IImageReader
	virtual CLayerImgInfo GetImgInfo(UINT uLevel=0)
	{
        if (m_pSrc == NULL)
            return CLayerImgInfo();

        CLayerImgInfo li = m_pSrc->GetImgInfo(uLevel);
        vt::CRect r = LayerRectAtLevel(m_rctCrop, uLevel) & li.LayerRect();

        li.width = r.Width();
        li.height = r.Height();
        li.origin = r.TopLeft();
        return li;
    }

	virtual HRESULT GetMetaData(OUT CParams &params)
	{
		return m_pSrc == NULL ? E_NOINIT : m_pSrc->GetMetaData(params);
	}
 
	virtual HRESULT ReadImg(OUT CImg& dst, UINT uLevel=0)
	{ 
        if (m_pSrc == NULL)
            return E_NOINIT;

        CLayerImgInfo li = m_pSrc->GetImgInfo(uLevel);
        vt::CRect r = LayerRectAtLevel(m_rctCrop, uLevel) & li.LayerRect();

        return m_pSrc->ReadRegion(r - li.origin, dst, uLevel); 
	}

	virtual HRESULT ReadRegion(const CRect &region, OUT CImg &dst, 
							   UINT uLevel=0)
	{ 
        if (m_pSrc == NULL)
            return E_NOINIT;

        CLayerImgInfo li = m_pSrc->GetImgInfo(uLevel);
        vt::CRect r = LayerRectAtLevel(m_rctCrop, uLevel) & li.LayerRect();

        return m_pSrc->ReadRegion(((region + r.TopLeft()) & r) - li.origin, dst, uLevel); 
	}

	virtual HRESULT Prefetch(const CRect &region, UINT uLevel=0)
    {
        if (m_pSrc == NULL)
            return E_NOINIT;

        CLayerImgInfo li = m_pSrc->GetImgInfo(uLevel);
        vt::CRect r = LayerRectAtLevel(m_rctCrop, uLevel) & li.LayerRect();

        return m_pSrc->Prefetch(((region + r.TopLeft()) & r) - li.origin, uLevel); 
	}

	// IImageWtriter
	virtual HRESULT  WriteImg(IN const CImg& src, UINT uLevel=0)
    {
        if (m_pSrc == NULL)
            return E_NOINIT;

        CLayerImgInfo li = m_pSrc->GetImgInfo(uLevel);
        vt::CRect r = LayerRectAtLevel(m_rctCrop, uLevel) & li.LayerRect();

        return m_pSrc->WriteRegion(r - li.origin, src, uLevel); 
    }

    virtual HRESULT  WriteRegion(IN const CRect& region, IN const CImg& src,
								 UINT uLevel=0)
    {
        if (m_pSrc == NULL)
            return E_NOINIT;

        CLayerImgInfo li = m_pSrc->GetImgInfo(uLevel);
        vt::CRect r = LayerRectAtLevel(m_rctCrop, uLevel) & li.LayerRect();

        return m_pSrc->WriteRegion(((region + r.TopLeft()) & r) - li.origin, src, uLevel); 
    }

    virtual HRESULT  Fill(const void* pbValue, const RECT *prct = NULL,
						  UINT uLevel = 0)
    {
        if (m_pSrc == NULL)
            return E_NOINIT;

        CLayerImgInfo li = m_pSrc->GetImgInfo(uLevel);
        vt::CRect r = LayerRectAtLevel(m_rctCrop, uLevel) & li.LayerRect();

        if (prct != NULL)
            r = ((CRect(*prct) + r.TopLeft()) & r) - li.origin;
        else
            r -= li.origin;
        return m_pSrc->Fill(pbValue, &r, uLevel); 
    }

    virtual HRESULT  SetMetaData(const CParams *pParams)
	{
		return m_pSrc == NULL ? E_NOINIT : m_pSrc->SetMetaData(pParams);
	}

    // implementation specific stuff
public:
	CCropImageReaderWriter() : m_pSrc(NULL)
	{}

	HRESULT Initialize(IImageReaderWriter* pSrc, const vt::CRect& rctCrop)
	{
        if (pSrc == NULL)
            return E_INVALIDARG;
        m_pSrc = pSrc;

        if (!vt::CRect(m_pSrc->GetImgInfo().LayerRect()).RectInRect(&rctCrop))
            return E_INVALIDARG;
        m_rctCrop = rctCrop;

        return S_OK;
	}

protected:
	IImageReaderWriter* m_pSrc;
	CRect               m_rctCrop;
};

//+-----------------------------------------------------------------------------
//
// Class: CLevelMapReaderWriter
//
// Synopsis: class that remaps a set of sources into levels of the presented
//           IImageReaderWriter
// 
//------------------------------------------------------------------------------
class CLevelMapReaderWriter: public IImageReaderWriter
{
public: 
	// IImageReaderWriter
	virtual CLayerImgInfo GetImgInfo(UINT uLevel = 0)
	{ return ValidArgs(uLevel)? m_vecMap[uLevel].info: CLayerImgInfo(); }

	virtual HRESULT GetMetaData(CParams& params)
	{ return ValidArgs(0)? RW(0)->GetMetaData(params): E_NOINIT; }

	virtual HRESULT ReadImg(OUT CImg& dst, UINT uLevel=0)
	{ 
		return ValidArgs(uLevel)? 
			RW(uLevel)->ReadImg(dst, LS(uLevel)): E_INVALIDARG; 
    }

	virtual HRESULT ReadRegion(IN const CRect& region, OUT CImg& dst, 
                               UINT uLevel = 0)
	{ 
		return ValidArgs(uLevel)? 
			RW(uLevel)->ReadRegion(region, dst, LS(uLevel)): E_INVALIDARG;
    }

	virtual HRESULT Prefetch(IN const CRect& region, UINT uLevel = 0)
	{ 
		return ValidArgs(uLevel)? 
			RW(uLevel)->Prefetch(region, LS(uLevel)): E_INVALIDARG; 
    }

public:
	// IImageWriter implementation
	virtual HRESULT  WriteImg(IN const CImg& src, UINT uLevel=0)
	{ 
		return ValidArgs(uLevel)? 
			RW(uLevel)->WriteImg(src, LS(uLevel)): E_INVALIDARG; 
	}

	virtual HRESULT  WriteRegion(IN const CRect& r, IN const CImg& src,
								 UINT uLevel=0)
	{ 
		return ValidArgs(uLevel)? 
			RW(uLevel)->WriteRegion(r, src, LS(uLevel)): E_INVALIDARG;
	}

	virtual HRESULT  Fill(const void* pbValue, const RECT *prct = NULL,
						  UINT uLevel = 0)
	{  
		return ValidArgs(uLevel)? 
			RW(uLevel)->Fill(pbValue, prct, LS(uLevel)): E_INVALIDARG;
	}

	virtual HRESULT  SetMetaData(const CParams* pParams)
	{ return ValidArgs(0)? RW(0)->SetMetaData(pParams): E_NOINIT; }

public:
    CLevelMapReaderWriter()
    {}

	HRESULT SetLevel(IImageReaderWriter* pSrc, UINT uSrcLevel, UINT uDstLevel)
	{
		CLayerImgInfo i = pSrc->GetImgInfo(uSrcLevel);
		return SetLevel(pSrc, uSrcLevel, uDstLevel, 
						i.compositeWidth, i.compositeHeight, i.origin, 
						i.compositeWrapX, i.compositeWrapY);
	}

	HRESULT SetLevel(IImageReaderWriter* pSrc, UINT uSrcLevel, UINT uDstLevel,
					 int cw, int ch, const CPoint& loc, 
					 bool bCompWrapX=false, bool bCompWrapY=false)
	{
		HRESULT hr = S_OK;
		if( uDstLevel >= m_vecMap.size() )
		{
			hr = m_vecMap.resize(uDstLevel+1);
		}
		if( hr == S_OK )
		{
			LEVEL_INFO& li = m_vecMap[uDstLevel];
			li.pSrc      = pSrc;
			li.uSrcLevel = uSrcLevel;
			li.info      = CLayerImgInfo(pSrc->GetImgInfo(uSrcLevel), cw, ch, 
										 loc, bCompWrapX, bCompWrapY);
		}
		return hr;
	}

protected:
	bool ValidArgs(UINT uLevel)
	{ return (uLevel < m_vecMap.size()) && (m_vecMap[uLevel].pSrc != NULL); }

	IImageReaderWriter* RW(UINT uLevel)
	{ return m_vecMap[uLevel].pSrc; }

    UINT LS(UINT uLevel)
	{ return m_vecMap[uLevel].uSrcLevel; }

protected:
	struct LEVEL_INFO
	{
		IImageReaderWriter* pSrc;
		UINT                uSrcLevel;
		CLayerImgInfo       info;
		LEVEL_INFO() : pSrc(NULL)
		{}
	};

protected:
    vt::vector<LEVEL_INFO> m_vecMap;
};

//+-----------------------------------------------------------------------------
//
// Class: CIndexedMapReader
//
// Synopsis: class that remaps a set of source readers into a new IIndexedImageReader
// 
//------------------------------------------------------------------------------
class CIndexedMapReader: public IIndexedImageReader
{
public: 
	// IIndexedImageReader
	virtual CLayerImgInfo GetImgInfo(UINT uIndex, UINT uLevel = 0)
	{ return ValidArgs(uIndex)? Rdr(uIndex)->GetImgInfo(uLevel): CLayerImgInfo(); }

	virtual HRESULT GetMetaData(UINT uIndex, CParams& params)
	{ return ValidArgs(uIndex)? Rdr(uIndex)->GetMetaData(params): E_INVALIDARG; }

	virtual HRESULT ReadImg(UINT uIndex, OUT CImg& dst, UINT uLevel=0)
	{ return ValidArgs(uIndex)? Rdr(uIndex)->ReadImg(dst, uLevel): E_INVALIDARG; }

	virtual HRESULT ReadRegion(UINT uIndex, IN const CRect& region, OUT CImg& dst, 
                               UINT uLevel = 0)
	{ 
		return ValidArgs(uIndex)? Rdr(uIndex)->ReadRegion(region, dst, uLevel): 
			                      E_INVALIDARG;
    }

	virtual HRESULT Prefetch(UINT uIndex, IN const CRect& region, UINT uLevel = 0)
	{ return ValidArgs(uIndex)? Rdr(uIndex)->Prefetch(region, uLevel): E_INVALIDARG; }

	virtual UINT    GetFrameCount()
	{ return (UINT)m_vecMap.size(); }

public:
    CIndexedMapReader()
    {}

	HRESULT PushBack(IImageReader* pSrc, UINT uLevel = 0)
	{
		HRESULT hr = m_vecMap.resize(m_vecMap.size()+1);
		if( hr == S_OK )
		{
			m_vecMap.back().lc.SetSource(pSrc, uLevel);
		}
		return hr;
	}

    HRESULT PushBack(IIndexedImageReader* pSrc, UINT uStartIndex, UINT uCnt=1,
					 UINT uLevel=0)
    { 
		UINT uBase = (UINT)m_vecMap.size();
		HRESULT hr = m_vecMap.resize(uBase+uCnt);
		for( UINT i = 0; i < uCnt && hr==S_OK; i++, uStartIndex++ )
		{
            VT_ASSERT( uStartIndex < pSrc->GetFrameCount() );
			m_vecMap[uBase+i].indexed.SetSource(pSrc, uStartIndex, uLevel);   
		}
		return hr;
	}

public:
    void Reset()
    { m_vecMap.resize(0); }

protected:
	struct SRC_INFO
	{
		CIndexedImageReaderIterator indexed; 
		CLevelChangeReader          lc;  
	};

protected:
	bool ValidArgs(UINT uIndex)
	{ return uIndex < GetFrameCount(); }

	IImageReader* Rdr(UINT i)
	{ 
		return m_vecMap[i].indexed.GetSource()? 
			(IImageReader*)&m_vecMap[i].indexed: (IImageReader*)&m_vecMap[i].lc; 
    }

protected:
    vt::vector<SRC_INFO> m_vecMap;
};

//+-----------------------------------------------------------------------------
//
// Class: CIndexedPrefetchHelper
//
// Synopsis: helper class that handles the commonly occuring pattern of
//           pre-fetching frames ahead within a processing loop
//
//------------------------------------------------------------------------------
class CIndexedPrefetchHelper
{
public:
	CIndexedPrefetchHelper(UINT uDepth) : m_uPrefetchIndex(0), m_uDepth(uDepth)
	{}

	HRESULT Prefetch(IN IIndexedImageReader* pReader, UINT uReadIndex)
	{
		HRESULT hr = S_OK;
		UINT nFrames = pReader->GetFrameCount();
		for( ; hr == S_OK && m_uPrefetchIndex < (uReadIndex+m_uDepth) && 
			   m_uPrefetchIndex < nFrames; m_uPrefetchIndex++ )
		{
            CImgInfo info = pReader->GetImgInfo(m_uPrefetchIndex);
            pReader->Prefetch(m_uPrefetchIndex, info.Rect());
		}
		return hr;
	}
	
protected:
	UINT m_uPrefetchIndex;
	UINT m_uDepth;
};

};
