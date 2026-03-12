#pragma unmanaged
#include <vtcore.h>

//// use this transform instead of CRotateTransform to force output to floats
/*class FlipTransform : public vt::CRotateTransform 
{	
	virtual void GetSrcPixFormat(int* pfrmtSrcs, 
		UINT, int)
	{ 
		// keep original input format 
		pfrmtSrcs[0] = OBJ_UNDEFINED;
	}

	virtual void GetDstPixFormat(int& frmtDst,
		const int* pfrmtSrcs, UINT)
	{ 
		// imagewatch operations must have float output
		frmtDst = VT_IMG_FIXED(VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, 
			VT_IMG_BANDS(pfrmtSrcs[0])));
	}

	virtual HRESULT Transform(vt::CImg* pimgDst, const vt::CRect& rctDst,
		const vt::CImg& imgSrc, const vt::CPoint& ptSrc)
	{
		VT_HR_BEGIN();

		VT_HR_EXIT(vt::CRotateTransform::Transform(pimgDst, rctDst, imgSrc, 
			ptSrc));		
		// assumption: rotate transform converts to output format
		VT_ASSERT(EL_FORMAT(pimgDst->GetType()) == EL_FORMAT_FLOAT);

		// if format was converted, undo default scaling
		int sf = 1;
		switch (EL_FORMAT(imgSrc.GetType()))
		{
		case EL_FORMAT_BYTE:
		case EL_FORMAT_SBYTE:
			sf = 255;
			break;
		case EL_FORMAT_SHORT:
		case EL_FORMAT_SSHORT:
			sf = 65536;
			break;
		}
		
		if (sf != 1)
			VT_HR_EXIT(vt::VtScaleImage(*pimgDst, *pimgDst, (float)sf));

		VT_HR_END();
	}

	virtual HRESULT Clone(ITaskState **ppState)
    {
		return vt::CloneTaskState<FlipTransform>(
			ppState, [this](FlipTransform* pN) -> HRESULT
			{ pN->InitializeFlip(m_rect, m_eFlipMode, m_dstType);
			return S_OK; });
    }
};*/
#pragma managed


#include "WatchedImageFlipOpBase.h"
#include "WatchedImageHelpers.h"

///////////////////////////////////////////////////////////////////////////////

using namespace Microsoft::ImageWatch;

Microsoft::ImageWatch::WatchedImageFlipOpBase::WatchedImageFlipOpBase()
	: transform_(nullptr)
{
	transform_ = new vt::CRotateTransform();	

	SetTransform(transform_);
}

Microsoft::ImageWatch::WatchedImageFlipOpBase::~WatchedImageFlipOpBase()
{
	this->!WatchedImageFlipOpBase();
}

Microsoft::ImageWatch::WatchedImageFlipOpBase::!WatchedImageFlipOpBase()
{
	delete transform_;
}

array<Type^>^ Microsoft::ImageWatch::WatchedImageFlipOpBase::
	GetNonImageArgumentTypes()
{
	auto res = gcnew array<Type^>(0);	
	return res;
}

bool Microsoft::ImageWatch::WatchedImageFlipOpBase::AreNonImageArgsValid()
{	
	return true;
}

bool Microsoft::ImageWatch::WatchedImageFlipOpBase::InitializeTransform()
{	
	if (InputImage == nullptr || !InputImage->HasValidInfo())
		return false;

	vt::eFlipMode fm = (vt::eFlipMode)Mode;

	if (fm != vt::eFlipModeNone &&
		fm != vt::eFlipModeHorizontal &&
		fm != vt::eFlipModeRotate180 &&
		fm != vt::eFlipModeVertical &&
		fm != vt::eFlipModeTranspose &&
		fm != vt::eFlipModeRotate90 &&
		fm != vt::eFlipModeTransverse &&
		fm != vt::eFlipModeRotate270)
		return false;

	transform_->InitializeFlip(InputImage->GetInfo()->Width, 
		InputImage->GetInfo()->Height, fm);

	return true;
}

bool Microsoft::ImageWatch::WatchedImageFlipOpBase::GetOpViewFormat(
	int% width, int%height, int% vttype)
{
	if (InputImage == nullptr || !InputImage->HasValidInfo())
		return false;

	int inputWidth = 0;
	int inputHeight = 0;
	int inputType = 0;
	if (!InputImage->GetViewFormat(inputWidth, inputHeight, inputType))
		return false;

	vt::eFlipMode fm = (vt::eFlipMode)Mode;
	if (fm == vt::eFlipMode::eFlipModeRotate90 ||
		fm == vt::eFlipMode::eFlipModeRotate270 ||
		fm == vt::eFlipMode::eFlipModeTranspose ||
		fm == vt::eFlipMode::eFlipModeTransverse)
	{
		width = inputHeight;
		height = inputWidth;	
	}
	else
	{
		width = inputWidth;
		height = inputHeight;
	}	


	// use this line for forcing float output:
	//vttype = VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, VT_IMG_BANDS(inputType));
	vttype = inputType;

	return true;
}
