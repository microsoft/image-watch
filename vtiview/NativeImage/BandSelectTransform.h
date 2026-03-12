#pragma once

#include "vtcore.h"

class BandSelectTransform :
	public vt::CImageTransformUnaryPoint<BandSelectTransform, false>
{
public:
	void Initialize(int band, bool convertToFloat)
	{		
		m_band = band;
		m_convertToFloat = convertToFloat;
	}

	virtual void GetSrcPixFormat(int* pfrmtSrcs, 
		UINT, int)
	{ 
		pfrmtSrcs[0] = OBJ_UNDEFINED;
	}

	virtual void GetDstPixFormat(int& frmtDst,
		const int* pfrmtSrcs, UINT)
	{ 
		frmtDst = VT_IMG_FIXED(VT_IMG_MAKE_TYPE(
			m_convertToFloat ? EL_FORMAT_FLOAT : EL_FORMAT(pfrmtSrcs[0]), 1));
	}

	virtual HRESULT GetRequiredSrcRect(OUT vt::TRANSFORM_SOURCE_DESC* pSrcReq,
		OUT UINT& uSrcReqCount, IN UINT /*uSrcCnt*/, 
		IN const vt::CRect& rctLayerDst)
	{
		pSrcReq->bCanOverWrite = false;
		pSrcReq->rctSrc        = rctLayerDst;
		pSrcReq->uSrcIndex     = 0;
		uSrcReqCount           = 1;
		return S_OK;
	}
	
	virtual HRESULT Transform(vt::CImg* pimgDst, const vt::CRect&, 
		const vt::CImg& src)
	{
		VT_ASSERT(m_band >= 0);
		
		// using UNDOCUMENTED feature here: assuming BandIndex0 = 0
		vt::BandIndexType bi = static_cast<vt::BandIndexType>(m_band);

		return vt::VtConvertBands(*pimgDst, src, 1, &bi, nullptr);
	}

	virtual HRESULT Clone(vt::ITaskState **ppState)
	{
		return vt::CloneTaskState<BandSelectTransform>(ppState, 
			new BandSelectTransform(*this));
	}

private:
	int m_band;
	bool m_convertToFloat;
};