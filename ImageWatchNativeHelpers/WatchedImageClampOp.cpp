#pragma unmanaged
#include "vtcore.h"

class ClampTransform : 
	public vt::CImageTransformUnaryPoint<ClampTransform, false>
{
public:
	bool Initialize(float min, float max)
	{	
		if (min > max)
			return false;

		m_min = min;
		m_max = max;

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
		if (m_min > m_max)		
			return E_INVALIDARG;

		for (int y = 0; y < src.Height(); ++y)
		{
#define TYPED_CLAMP_SPAN_CASE(elformat, type) \
	case elformat: \
	VT_ASSERT(EL_FORMAT(pimgDst->GetType()) == EL_FORMAT_FLOAT); \
	ClampSpan(reinterpret_cast<float*>(pimgDst->BytePtr(y)), \
	reinterpret_cast<const type*>(src.BytePtr(y)), \
			src.Width() * src.Bands()); \
	break;
			switch (EL_FORMAT(src.GetType()))
			{
			default:
				return E_NOTIMPL;

				TYPED_CLAMP_SPAN_CASE(EL_FORMAT_BYTE, vt::Byte);
				TYPED_CLAMP_SPAN_CASE(EL_FORMAT_SBYTE, int8_t);
				TYPED_CLAMP_SPAN_CASE(EL_FORMAT_SHORT, vt::UInt16);
				TYPED_CLAMP_SPAN_CASE(EL_FORMAT_SSHORT, int16_t);
				TYPED_CLAMP_SPAN_CASE(EL_FORMAT_HALF_FLOAT, vt::HALF_FLOAT);
				TYPED_CLAMP_SPAN_CASE(EL_FORMAT_FLOAT, float);
				TYPED_CLAMP_SPAN_CASE(EL_FORMAT_INT, int);
				TYPED_CLAMP_SPAN_CASE(EL_FORMAT_DOUBLE, double);
			}			
#undef TYPED_CLAMP_SPAN_CASE
		}

		return S_OK;
	}

	virtual HRESULT Clone(vt::ITaskState **ppState)
	{
		return vt::CloneTaskState<ClampTransform>(ppState, 
			new ClampTransform(*this));	
	}

private:
	template <typename T>
	void ClampSpan(float* dst, const T* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
		{
			const float v = static_cast<float>(src[i]);
			dst[i] = vt::VtClamp(v, m_min, m_max);
		}
	}	

	template <>
	void ClampSpan(float* dst, const vt::HALF_FLOAT* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
		{
			const float v = HF2F(src[i]);
			dst[i] = vt::VtClamp(v, m_min, m_max);
		}
	}	

	template <>
	void ClampSpan(float* dst, const double* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
		{
			float vf = 0;
			vt::VtClip(&vf, src[i]);
			dst[i] = vt::VtClamp(vf, m_min, m_max);
		}
	}	

private:
	float m_min, m_max;
};
#pragma managed

#include "WatchedImageClampOp.h"
#include "WatchedImageHelpers.h"

///////////////////////////////////////////////////////////////////////////////

using namespace Microsoft::ImageWatch;

Microsoft::ImageWatch::WatchedImageClampOp::WatchedImageClampOp()
	: transform_(nullptr)
{
	transform_ = new ClampTransform();	

	SetTransform(transform_);
}

Microsoft::ImageWatch::WatchedImageClampOp::~WatchedImageClampOp()
{
	this->!WatchedImageClampOp();
}

Microsoft::ImageWatch::WatchedImageClampOp::!WatchedImageClampOp()
{
	delete transform_;
}

float Microsoft::ImageWatch::WatchedImageClampOp::Min::get()
{
	if (!CheckArguments())
		return 0.f;

	return (float)Args[1];
}

float Microsoft::ImageWatch::WatchedImageClampOp::Max::get()
{
	if (!CheckArguments())
		return 0.f;

	return (float)Args[2];
}

array<Type^>^ Microsoft::ImageWatch::WatchedImageClampOp::
	GetNonImageArgumentTypes()
{
	auto res = gcnew array<Type^>(2);
	res[0] = float::typeid;
	res[1] = float::typeid;
	return res;
}

bool Microsoft::ImageWatch::WatchedImageClampOp::AreNonImageArgsValid()
{	
	return Min < Max;
}

bool Microsoft::ImageWatch::WatchedImageClampOp::InitializeTransform()
{	
	return transform_->Initialize(Min, Max);
}

bool Microsoft::ImageWatch::WatchedImageClampOp::GetOpViewFormat(
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
