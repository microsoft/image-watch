#pragma unmanaged
#include "vtcore.h"

class ScaleTransform : 
	public vt::CImageTransformUnaryPoint<ScaleTransform, false>
{
public:
	void Initialize(float scale)
	{	
		m_scale = scale;
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
#define TYPED_SCALE_SPAN_CASE(elformat, type) \
	case elformat: \
	VT_ASSERT(EL_FORMAT(pimgDst->GetType()) == EL_FORMAT_FLOAT); \
	ScaleSpan(reinterpret_cast<float*>(pimgDst->BytePtr(y)), \
	reinterpret_cast<const type*>(src.BytePtr(y)), \
			src.Width() * src.Bands()); \
	break;
			switch (EL_FORMAT(src.GetType()))
			{
			default:
				return E_NOTIMPL;

				TYPED_SCALE_SPAN_CASE(EL_FORMAT_BYTE, vt::Byte);
				TYPED_SCALE_SPAN_CASE(EL_FORMAT_SBYTE, int8_t);
				TYPED_SCALE_SPAN_CASE(EL_FORMAT_SHORT, vt::UInt16);
				TYPED_SCALE_SPAN_CASE(EL_FORMAT_SSHORT, int16_t);
				TYPED_SCALE_SPAN_CASE(EL_FORMAT_HALF_FLOAT, vt::HALF_FLOAT);
				TYPED_SCALE_SPAN_CASE(EL_FORMAT_FLOAT, float);
				TYPED_SCALE_SPAN_CASE(EL_FORMAT_INT, int);
				TYPED_SCALE_SPAN_CASE(EL_FORMAT_DOUBLE, double);
			}			
#undef TYPED_CLAMP_SPAN_CASE
		}

		return S_OK;
	}

	virtual HRESULT Clone(vt::ITaskState **ppState)
	{
		return vt::CloneTaskState<ScaleTransform>(ppState, 
			new ScaleTransform(*this));	
	}

private:
	template <typename T>
	void ScaleSpan(float* dst, const T* src, int numel)
	{	
		vt::VtConvertSpan(dst, src, numel);

		float vtconv_correction = 1.f;
		if (vt::ElTraits<T>::ElFormat() == EL_FORMAT_BYTE ||
			vt::ElTraits<T>::ElFormat() == EL_FORMAT_SBYTE)
		{
			vtconv_correction = 255.f;
		}
		else if (vt::ElTraits<T>::ElFormat() == EL_FORMAT_SHORT ||
			vt::ElTraits<T>::ElFormat() == EL_FORMAT_SSHORT)
		{
			vtconv_correction = 65535.f;
		}

		// assuming this works in place
		vt::VtScaleSpan(dst, dst, m_scale * vtconv_correction, numel);
	}	
	
	template <>
	void ScaleSpan(float* dst, const double* src, int numel)
	{	
		for (int i=0; i<numel; ++i)
			vt::VtConv(&dst[i], src[i] * m_scale);
	}	

private:
	float m_scale;
};
#pragma managed

#include "WatchedImageScaleOpBase.h"
#include "WatchedImageHelpers.h"

///////////////////////////////////////////////////////////////////////////////

using namespace Microsoft::ImageWatch;

Microsoft::ImageWatch::WatchedImageScaleOpBase::WatchedImageScaleOpBase()
	: transform_(nullptr)
{
	transform_ = new ScaleTransform();	

	SetTransform(transform_);
}

Microsoft::ImageWatch::WatchedImageScaleOpBase::~WatchedImageScaleOpBase()
{
	this->!WatchedImageScaleOpBase();
}

Microsoft::ImageWatch::WatchedImageScaleOpBase::!WatchedImageScaleOpBase()
{
	delete transform_;
}

bool Microsoft::ImageWatch::WatchedImageScaleOpBase::InitializeTransform()
{	
	transform_->Initialize(Scale);
	return true;
}

bool Microsoft::ImageWatch::WatchedImageScaleOpBase::GetOpViewFormat(
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
