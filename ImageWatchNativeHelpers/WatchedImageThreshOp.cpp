#pragma unmanaged
#include "vtcore.h"

class ThreshTransform : 
	public vt::CImageTransformUnaryPoint<ThreshTransform, false>
{
public:
	bool Initialize(float thresh)
	{	
		m_thresh = thresh;
		return true;
	}

	virtual void GetSrcPixFormat(int* pfrmtSrcs, 
		UINT, int)
	{ 
		pfrmtSrcs[0] = OBJ_UNDEFINED;
	}

	virtual void GetDstPixFormat(int& frmtDst,
		const int* pfrmtSrcs, UINT)
	{ 
		frmtDst = VT_IMG_FIXED(VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, 
			VT_IMG_BANDS(pfrmtSrcs[0])));
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
		for (int y = 0; y < src.Height(); ++y)
		{
#define TYPED_THRESH_SPAN_CASE(elformat, type) \
	case elformat: \
	VT_ASSERT(EL_FORMAT(pimgDst->GetType()) == EL_FORMAT_FLOAT); \
	ThreshSpan(reinterpret_cast<float*>(pimgDst->BytePtr(y)), \
	reinterpret_cast<const type*>(src.BytePtr(y)), \
			src.Width() * src.Bands()); \
	break;
			switch (EL_FORMAT(src.GetType()))
			{
			default:
				return E_NOTIMPL;

				TYPED_THRESH_SPAN_CASE(EL_FORMAT_BYTE, vt::Byte);
				TYPED_THRESH_SPAN_CASE(EL_FORMAT_SBYTE, int8_t);
				TYPED_THRESH_SPAN_CASE(EL_FORMAT_SHORT, vt::UInt16);
				TYPED_THRESH_SPAN_CASE(EL_FORMAT_SSHORT, int16_t);
				TYPED_THRESH_SPAN_CASE(EL_FORMAT_HALF_FLOAT, vt::HALF_FLOAT);
				TYPED_THRESH_SPAN_CASE(EL_FORMAT_FLOAT, float);
				TYPED_THRESH_SPAN_CASE(EL_FORMAT_INT, int);
				TYPED_THRESH_SPAN_CASE(EL_FORMAT_DOUBLE, double);
			}			
#undef TYPED_THRESH_SPAN_CASE
		}

		return S_OK;
	}

	virtual HRESULT Clone(vt::ITaskState **ppState)
	{
		return vt::CloneTaskState<ThreshTransform>(ppState, 
			new ThreshTransform(*this));	
	}

private:
	template <typename T>
	void ThreshSpan(float* dst, const T* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
		{
			const float v = static_cast<float>(src[i]);
			dst[i] = v >= m_thresh ? 1.f : 0.f;
		}
	}	

	template <>
	void ThreshSpan(float* dst, const vt::HALF_FLOAT* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
		{
			const float v = HF2F(src[i]);
			dst[i] = v >= m_thresh ? 1.f : 0.f;
		}
	}	

	template <>
	void ThreshSpan(float* dst, const double* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
		{
			float vf = 0;
			vt::VtClip(&vf, src[i]);
			dst[i] = vf >= m_thresh ? 1.f : 0.f;
		}
	}	

private:
	float m_thresh;
};
#pragma managed

#include "WatchedImageThreshOp.h"
#include "WatchedImageHelpers.h"

///////////////////////////////////////////////////////////////////////////////

using namespace Microsoft::ImageWatch;

Microsoft::ImageWatch::WatchedImageThreshOp::WatchedImageThreshOp()
	: transform_(nullptr)
{
	transform_ = new ThreshTransform();	

	SetTransform(transform_);
}

Microsoft::ImageWatch::WatchedImageThreshOp::~WatchedImageThreshOp()
{
	this->!WatchedImageThreshOp();
}

Microsoft::ImageWatch::WatchedImageThreshOp::!WatchedImageThreshOp()
{
	delete transform_;
}

float Microsoft::ImageWatch::WatchedImageThreshOp::Threshold::get()
{
	if (!CheckArguments())
		return 0.f;

	return (float)Args[1];
}

array<Type^>^ Microsoft::ImageWatch::WatchedImageThreshOp::
	GetNonImageArgumentTypes()
{
	auto res = gcnew array<Type^>(1);
	res[0] = float::typeid;
	return res;
}

bool Microsoft::ImageWatch::WatchedImageThreshOp::AreNonImageArgsValid()
{	
	return true;
}

bool Microsoft::ImageWatch::WatchedImageThreshOp::InitializeTransform()
{	
	return transform_->Initialize(Threshold);
}

bool Microsoft::ImageWatch::WatchedImageThreshOp::GetOpViewFormat(
	int% width, int%height, int% vttype)
{
	if (InputImage == nullptr || !InputImage->HasValidInfo())
		return false;

	int inputWidth = 0;
	int inputHeight = 0;
	int inputType = 0;
	if (!InputImage->GetViewFormat(inputWidth, inputHeight, inputType))
		return false;
	
	width = inputWidth;
	height = inputHeight;
	vttype = VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, VT_IMG_BANDS(inputType));

	return true;
}
