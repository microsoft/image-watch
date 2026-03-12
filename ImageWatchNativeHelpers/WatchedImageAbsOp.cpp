#pragma unmanaged
#include "vtcore.h"

class AbsTransform : 
	public vt::CImageTransformUnaryPoint<AbsTransform, false>
{
public:
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
#define TYPED_ABS_SPAN_CASE(elformat, type) \
	case elformat: \
	VT_ASSERT(EL_FORMAT(pimgDst->GetType()) == EL_FORMAT_FLOAT); \
	AbsSpan(reinterpret_cast<float*>(pimgDst->BytePtr(y)), \
	reinterpret_cast<const type*>(src.BytePtr(y)), \
			src.Width() * src.Bands()); \
	break;
			switch (EL_FORMAT(src.GetType()))
			{
			default:
				return E_NOTIMPL;

				TYPED_ABS_SPAN_CASE(EL_FORMAT_BYTE, vt::Byte);
				TYPED_ABS_SPAN_CASE(EL_FORMAT_SBYTE, int8_t);
				TYPED_ABS_SPAN_CASE(EL_FORMAT_SHORT, vt::UInt16);
				TYPED_ABS_SPAN_CASE(EL_FORMAT_SSHORT, int16_t);
				TYPED_ABS_SPAN_CASE(EL_FORMAT_HALF_FLOAT, vt::HALF_FLOAT);
				TYPED_ABS_SPAN_CASE(EL_FORMAT_FLOAT, float);
				TYPED_ABS_SPAN_CASE(EL_FORMAT_INT, int);
				TYPED_ABS_SPAN_CASE(EL_FORMAT_DOUBLE, double);
			}			
#undef TYPED_Abs_SPAN_CASE
		}

		return S_OK;
	}

	virtual HRESULT Clone(vt::ITaskState **ppState)
	{
		return vt::CloneTaskState<AbsTransform>(ppState, 
			new AbsTransform(*this));	
	}

private:
	template <typename T>
	void AbsSpan(float* dst, const T* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
			dst[i] = static_cast<float>(fabs(static_cast<float>(src[i])));
	}	

	template <>
	void AbsSpan(float* dst, const vt::HALF_FLOAT* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
			dst[i] = static_cast<float>(fabs(static_cast<float>(HF2F(src[i]))));
	}	

	template <>
	void AbsSpan(float* dst, const double* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
			vt::VtClip(&dst[i], abs(src[i]));
	}	
};
#pragma managed

#include "WatchedImageAbsOp.h"
#include "WatchedImageHelpers.h"

///////////////////////////////////////////////////////////////////////////////

using namespace Microsoft::ImageWatch;

Microsoft::ImageWatch::WatchedImageAbsOp::WatchedImageAbsOp()
	: transform_(nullptr)
{
	transform_ = new AbsTransform();	

	SetTransform(transform_);
}

Microsoft::ImageWatch::WatchedImageAbsOp::~WatchedImageAbsOp()
{
	this->!WatchedImageAbsOp();
}

Microsoft::ImageWatch::WatchedImageAbsOp::!WatchedImageAbsOp()
{
	delete transform_;
}

array<Type^>^ Microsoft::ImageWatch::WatchedImageAbsOp::
	GetNonImageArgumentTypes()
{
	return nullptr;
}

bool Microsoft::ImageWatch::WatchedImageAbsOp::AreNonImageArgsValid()
{	
	return true;
}

bool Microsoft::ImageWatch::WatchedImageAbsOp::InitializeTransform()
{	
	return true;
}

bool Microsoft::ImageWatch::WatchedImageAbsOp::GetOpViewFormat(
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
