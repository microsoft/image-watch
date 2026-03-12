#include <ppl.h>
#include <concurrent_vector.h>

#include "vtcore.h"
#include "BandSelectTransform.h"

#include "Namespace.h"

BEGIN_NI_NAMESPACE;

class MinMaxTransform : 
	public vt::CImageTransformUnaryPoint<MinMaxTransform, true>
{
public:
	MinMaxTransform(Concurrency::concurrent_vector<float>& mn, 
		Concurrency::concurrent_vector<float>& mx)
		: minVector_(mn), maxVector_(mx)
	{
	}

private:
	//MinMaxTransform(const MinMaxTransform&);
	MinMaxTransform& operator=(const MinMaxTransform&);

public:
	virtual HRESULT Transform(vt::CImg*, const vt::CRect&, const vt::CImg& src)
	{
		VT_HR_BEGIN();

		const vt::CImg* mmSrc = &src;

		const auto srcElFormat = EL_FORMAT(mmSrc->GetType());
		vt::CFloatImg tmp;
		if (srcElFormat == EL_FORMAT_INT || srcElFormat == EL_FORMAT_DOUBLE ||
			srcElFormat == EL_FORMAT_SBYTE || srcElFormat == EL_FORMAT_SSHORT)
		{
			VT_HR_EXIT(vt::VtConvertImage(tmp, src));
			mmSrc = &tmp;
		}

		bool ignoreNaN = srcElFormat == EL_FORMAT_FLOAT || 
			srcElFormat == EL_FORMAT_HALF_FLOAT ||
			srcElFormat == EL_FORMAT_DOUBLE;
		float mn, mx;
		VT_HR_EXIT(VtMinMaxImage(*mmSrc, mn, mx, ignoreNaN));

		float factor = 1.f;
		if (srcElFormat == EL_FORMAT_SBYTE)
			factor = 255.f;
		else if (srcElFormat == EL_FORMAT_SSHORT)
			factor = 65535.f;

		minVector_.push_back(mn * factor);
		maxVector_.push_back(mx * factor);

		VT_HR_END();
	}

	virtual HRESULT GetRequiredSrcRect(vt::TRANSFORM_SOURCE_DESC* pSrcReq, 
		UINT& uSrcReqCount, UINT, const vt::CRect& rctLayerDst)
	{
		pSrcReq->bCanOverWrite = false;
		pSrcReq->rctSrc = rctLayerDst;
		pSrcReq->uSrcIndex  = 0;
		uSrcReqCount = 1;

		return S_OK;
	}

	virtual HRESULT Clone(vt::ITaskState **ppState)
	{
		return vt::CloneTaskState<MinMaxTransform>(ppState, 
			new MinMaxTransform(*this));
	}

private:
	Concurrency::concurrent_vector<float>& minVector_;
	Concurrency::concurrent_vector<float>& maxVector_;
}; 

HRESULT ComputeMinMaxElement(vt::IImageReader& reader, int selectedBand,
	float& minVal, float& maxVal)
{
	VT_HR_BEGIN();

	vt::CTransformGraphUnaryNode graph[2];

	BandSelectTransform bsxform;	
	if (selectedBand != -1)
	{
		VT_HR_EXIT(graph[0].BindSourceToReader(&reader));

		bsxform.Initialize(selectedBand, false);
		graph[0].SetTransform(&bsxform);
		VT_HR_EXIT(graph[1].BindSourceToTransform(&graph[0]));
	}
	else
	{
		VT_HR_EXIT(graph[1].BindSourceToReader(&reader));
	}
	
	Concurrency::concurrent_vector<float> mn, mx;
	MinMaxTransform mmxform(mn, mx);
	graph[1].SetTransform(&mmxform);

	vt::CRasterBlockMap bm(reader.GetImgInfo(0).LayerRect(), 256, 256, 4, 4);
	graph[1].SetDest(vt::NODE_DEST_PARAMS(NULL, &bm));
	VT_HR_EXIT(vt::PushTransformTaskAndWait(&graph[1], (vt::CTaskProgress*)NULL));

	minVal = *std::min_element(mn.begin(), mn.end());
	maxVal = *std::max_element(mx.begin(), mx.end());

	// all NaNs will lead to minval < maxval
	if (minVal > maxVal)
		minVal = maxVal = 0;

	VT_HR_END();
}

END_NI_NAMESPACE;
