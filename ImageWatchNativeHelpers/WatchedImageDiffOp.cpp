#pragma unmanaged
#include "vtcore.h"

class DiffTransform : 
	public vt::CImageTransformPoint<DiffTransform, false>
{
public:
	virtual void  GetSrcPixFormat(int* ptypeSrcs, UINT uSrcCnt, 
		int /*typeDst*/)
	{ 
		for( UINT i = 0; i < uSrcCnt; i++ )		
			ptypeSrcs[i] = OBJ_UNDEFINED; 
    }

	// by default this transform uses the type of the first source as the 
	// output type
	virtual void GetDstPixFormat(int& typeDst, const int* ptypeSrcs, 
		UINT /*uSrcCnt*/)
	{
		typeDst = VT_IMG_FIXED(VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, 
			VT_IMG_BANDS(ptypeSrcs[0])));
	}

	virtual HRESULT GetRequiredSrcRect(vt::TRANSFORM_SOURCE_DESC* pSrcReq,
		UINT& uSrcReqCount, UINT uSrcCnt, const vt::CRect& rctLayerDst)
	{
		for( UINT i = 0; i < uSrcCnt; i++, pSrcReq++ )
		{
			pSrcReq->bCanOverWrite = false;
			pSrcReq->rctSrc        = rctLayerDst;
			pSrcReq->uSrcIndex     = i;
		}
		uSrcReqCount = uSrcCnt;;
		return S_OK;
	}
	
	virtual HRESULT Transform(vt::CImg* pimgDstRegion, 
		const vt::CRect& /*rctLayerDst*/, vt::CImg *const *ppimgSrcRegions,
		UINT /*uSrcCnt*/)
	{
		for (int y = 0; y < pimgDstRegion->Height(); ++y)
		{
#define TYPED_FIRST_TO_FLOAT_SPAN_CASE(elformat, type) \
	case elformat: \
	VT_ASSERT(EL_FORMAT(pimgDstRegion->GetType()) == EL_FORMAT_FLOAT); \
	VT_ASSERT(pimgDstRegion->Bands() == ppimgSrcRegions[0]->Bands()); \
	ConvertSpanToFloat(reinterpret_cast<float*>(pimgDstRegion->BytePtr(y)), \
	reinterpret_cast<const type*>(ppimgSrcRegions[0]->BytePtr(y)), \
			pimgDstRegion->Width() * pimgDstRegion->Bands()); \
	break;
			switch (EL_FORMAT(ppimgSrcRegions[0]->GetType()))
			{
			default:
				return E_NOTIMPL;

				TYPED_FIRST_TO_FLOAT_SPAN_CASE(EL_FORMAT_BYTE, vt::Byte);
				TYPED_FIRST_TO_FLOAT_SPAN_CASE(EL_FORMAT_SBYTE, int8_t);
				TYPED_FIRST_TO_FLOAT_SPAN_CASE(EL_FORMAT_SHORT, vt::UInt16);
				TYPED_FIRST_TO_FLOAT_SPAN_CASE(EL_FORMAT_SSHORT, int16_t);
				TYPED_FIRST_TO_FLOAT_SPAN_CASE(EL_FORMAT_HALF_FLOAT, 
					vt::HALF_FLOAT);
				TYPED_FIRST_TO_FLOAT_SPAN_CASE(EL_FORMAT_FLOAT, float);
				TYPED_FIRST_TO_FLOAT_SPAN_CASE(EL_FORMAT_INT, int);
				TYPED_FIRST_TO_FLOAT_SPAN_CASE(EL_FORMAT_DOUBLE, double);
			}			
#undef TYPED_FIRST_TO_FLOAT_SPAN_CASE

#define TYPED_SECOND_DIFF_SPAN_CASE(elformat, type) \
	case elformat: \
	DiffFloatSpan(reinterpret_cast<float*>(pimgDstRegion->BytePtr(y)), \
	reinterpret_cast<const type*>(ppimgSrcRegions[1]->BytePtr(y)), \
			pimgDstRegion->Width() * pimgDstRegion->Bands()); \
	break;
			switch (EL_FORMAT(ppimgSrcRegions[1]->GetType()))
			{
			default:
				return E_NOTIMPL;

				TYPED_SECOND_DIFF_SPAN_CASE(EL_FORMAT_BYTE, vt::Byte);
				TYPED_SECOND_DIFF_SPAN_CASE(EL_FORMAT_SBYTE, int8_t);
				TYPED_SECOND_DIFF_SPAN_CASE(EL_FORMAT_SHORT, vt::UInt16);
				TYPED_SECOND_DIFF_SPAN_CASE(EL_FORMAT_SSHORT, int16_t);
				TYPED_SECOND_DIFF_SPAN_CASE(EL_FORMAT_HALF_FLOAT, 
					vt::HALF_FLOAT);
				TYPED_SECOND_DIFF_SPAN_CASE(EL_FORMAT_FLOAT, float);
				TYPED_SECOND_DIFF_SPAN_CASE(EL_FORMAT_INT, int);
				TYPED_SECOND_DIFF_SPAN_CASE(EL_FORMAT_DOUBLE, double);
			}			
#undef TYPED_SECOND_DIFF_SPAN_CASE
		}

		return S_OK;
	}		
	
	virtual HRESULT Clone(vt::ITaskState **ppState)
	{
		return vt::CloneTaskState<DiffTransform>(ppState, 
			new DiffTransform(*this));	
	}

private:
	template <typename T>
	void ConvertSpanToFloat(float* dst, const T* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
			dst[i] = static_cast<float>(src[i]);
	}	

	template <>
	void ConvertSpanToFloat(float* dst, const vt::HALF_FLOAT* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)			
			dst[i] = HF2F(src[i]);
	}	

	template <>
	void ConvertSpanToFloat(float* dst, const double* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
			vt::VtClip(&dst[i], src[i]);		
	}	

	template <typename T>
	void DiffFloatSpan(float* dst, const T* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
			dst[i] -= static_cast<float>(src[i]);
	}	

	template <>
	void DiffFloatSpan(float* dst, const vt::HALF_FLOAT* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)			
			dst[i] -= HF2F(src[i]);
	}	

	template <>
	void DiffFloatSpan(float* dst, const double* src, int numel)
	{		
		for (int i = 0; i < numel; ++i)
		{
			float vf = 0;
			vt::VtClip(&vf, src[i]);
			dst[i] -= vf;
		}
	}	
};
#pragma managed

#include "WatchedImageDiffOp.h"
#include "WatchedImageHelpers.h"

///////////////////////////////////////////////////////////////////////////////

using namespace Microsoft::ImageWatch;

Microsoft::ImageWatch::WatchedImageDiffOp::WatchedImageDiffOp()
	: transform_(nullptr)
{
	transform_ = new DiffTransform();	

	SetTransform(transform_);
}

Microsoft::ImageWatch::WatchedImageDiffOp::~WatchedImageDiffOp()
{
	this->!WatchedImageDiffOp();
}

Microsoft::ImageWatch::WatchedImageDiffOp::!WatchedImageDiffOp()
{
	delete transform_;
}

array<Type^>^ Microsoft::ImageWatch::WatchedImageDiffOp::
	GetNonImageArgumentTypes()
{
	return nullptr;
}

bool Microsoft::ImageWatch::WatchedImageDiffOp::AreImageArgsValid()
{
	if (FirstInputImage == nullptr || !FirstInputImage->HasValidInfo())
		return false;
	if (SecondInputImage == nullptr || !SecondInputImage->HasValidInfo())
		return false;

	auto i1 = FirstInputImage->GetReaderWriter();
	auto i2 = SecondInputImage->GetReaderWriter();
	if (i1 == nullptr || i2 == nullptr)
		return false;

	return i1->GetImgInfo().width == i2->GetImgInfo().width &&
		i1->GetImgInfo().height == i2->GetImgInfo().height &&
		i1->GetImgInfo().Bands() == i2->GetImgInfo().Bands();
}

bool Microsoft::ImageWatch::WatchedImageDiffOp::AreNonImageArgsValid()
{	
	return true;
}

bool Microsoft::ImageWatch::WatchedImageDiffOp::InitializeTransform()
{	
	return true;
}

bool Microsoft::ImageWatch::WatchedImageDiffOp::GetOpViewFormat(
	int% width, int%height, int% vttype)
{
	if (!AreImageArgsValid())
		return false;

	int inputWidth = 0;
	int inputHeight = 0;
	int inputType = 0;
	if (!FirstInputImage->GetViewFormat(inputWidth, inputHeight, inputType))
		return false;

	width = inputWidth;
	height = inputHeight;
	vttype = VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, VT_IMG_BANDS(inputType));

	return true;
}
