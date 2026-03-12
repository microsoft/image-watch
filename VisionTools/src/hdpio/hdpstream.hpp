////////////////////////////////////////////////////////////////////////////////
//
//  hdpstream.hpp
//
//  interface between the HDPhoto SDK stream and a COM IStream
// 
#pragma once

class CWMPStreamConv
{
public:
	CWMPStreamConv(IStream* pStream)
	{
		m_pStream = pStream;
		m_wmpStream.state.pvObj  = this;
		m_wmpStream.Close  = CloseWS_IStream;
		m_wmpStream.EOS    = EOSWS_IStream;
		m_wmpStream.Read   = ReadWS_IStream;
		m_wmpStream.Write  = WriteWS_IStream;
		m_wmpStream.SetPos = SetPosWS_IStream;
		m_wmpStream.GetPos = GetPosWS_IStream;

	}

	~CWMPStreamConv()
	{
	}

    WMPStream* GetWMPStream() { return &m_wmpStream; } 

	static ERR  CloseWS_IStream(struct WMPStream** pme);
	static Bool EOSWS_IStream(struct WMPStream* me);
	static ERR  ReadWS_IStream(struct WMPStream* me, void* pv, size_t cb);
	static ERR  WriteWS_IStream(struct WMPStream* me, const void* pv, size_t cb);
	static ERR  SetPosWS_IStream(struct WMPStream* me, size_t offPos);
	static ERR  GetPosWS_IStream(struct WMPStream* me, size_t* poffPos);
	 
private:
	IStream*   m_pStream;
	WMPStream  m_wmpStream;
};

ERR CWMPStreamConv::CloseWS_IStream(struct WMPStream** pme)
{
    return WMP_errSuccess;
}

Bool CWMPStreamConv::EOSWS_IStream(struct WMPStream* me)
{
	assert(0);  // shouldn't get called
	return FALSE;
}

ERR  CWMPStreamConv::ReadWS_IStream(struct WMPStream* me, void* pv, size_t cb)
{
	ERR dpkerr = WMP_errFileIO;
	CWMPStreamConv* pThis = (CWMPStreamConv*)(me->state.pvObj);
	if( pThis->m_pStream )
	{
		HRESULT hr = pThis->m_pStream->Read(pv, (ULONG)cb, NULL);
		if( hr == S_OK ) dpkerr = WMP_errSuccess;
	}
	return dpkerr;
}

ERR  CWMPStreamConv::WriteWS_IStream(struct WMPStream* me, const void* pv, 
									 size_t cb)
{
	ERR dpkerr = WMP_errFileIO;
	CWMPStreamConv* pThis = (CWMPStreamConv*)(me->state.pvObj);
	if( pThis->m_pStream )
	{
		HRESULT hr = pThis->m_pStream->Write(pv, (ULONG)cb, NULL);
		if( hr == S_OK ) dpkerr = WMP_errSuccess;
	}
	return dpkerr;
}

ERR  CWMPStreamConv::SetPosWS_IStream(struct WMPStream* me, size_t offPos)
{
	ERR dpkerr = WMP_errFileIO;
	CWMPStreamConv* pThis = (CWMPStreamConv*)(me->state.pvObj);
	if( pThis->m_pStream )
	{
        LARGE_INTEGER liPos;
        liPos.QuadPart = offPos;

		HRESULT hr = pThis->m_pStream->Seek(liPos, STREAM_SEEK_SET, NULL);
		if( hr == S_OK ) dpkerr = WMP_errSuccess;
	}
	return dpkerr;
}

ERR  CWMPStreamConv::GetPosWS_IStream(struct WMPStream* me, size_t* poffPos)
{
	ERR dpkerr = WMP_errFileIO;
	CWMPStreamConv* pThis = (CWMPStreamConv*)(me->state.pvObj);
	if( pThis->m_pStream )
	{
		LARGE_INTEGER  liPos;
        ULARGE_INTEGER uliQueryPos;
        liPos.QuadPart = 0;

		HRESULT hr = pThis->m_pStream->Seek(liPos, STREAM_SEEK_CUR, &uliQueryPos);
		if( hr == S_OK )
		{
			*poffPos = (size_t)uliQueryPos.QuadPart;
			dpkerr   = WMP_errSuccess;;
	    }
		else
		{
			*poffPos = 0;
		}
	}
	return dpkerr;
}
