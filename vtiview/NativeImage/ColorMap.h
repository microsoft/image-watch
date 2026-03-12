#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <memory>

#include <vtcore.h>

#include "NativeImageBase.h"
#include "NativeImageHelpers.h"
#include "Namespace.h"
#include "BandSelectTransform.h"

using namespace System;
using namespace System::Windows;

namespace vt{

template<typename SrcElT, typename DstElT>
struct AlphaBlendRGBAOp
{
public:
	typedef float ImplSrcElType;
	typedef float ImplDstElType;
	typedef int ParamType;
	struct TmpType 
	{ 
	};

	static const int NumSrcBands = 4;
	static const int NumDstBands = 4;
	static const int NumSrcElPerGroup = 4;
	static const int NumDstElPerGroup = 4;
	static const int NumGroupsPerOpGeneric = 1;
	//static const int NumGroupsPerOpSSE2 = 16;

public:
	void EvalGeneric(const ImplSrcElType* pSrc1, const ImplSrcElType* pSrc2, 
		ImplDstElType* pDst, ParamType*, TmpType& /* tmp */) 
	{
		const float a = pSrc1[3];
		const float oneMinusA = 1.f - a;

		pDst[0] = pSrc1[0] * a + pSrc2[0] * oneMinusA;
		pDst[1] = pSrc1[1] * a + pSrc2[1] * oneMinusA;
		pDst[2] = pSrc1[2] * a + pSrc2[2] * oneMinusA;
		pDst[3] = 1.f;
	}
};

template<typename TI, typename TO>
void VtAlphaBlendRGBASpan(TO* dst, int numDstBands, 
	const TI* foregroundWithAlpha, const TI* background, int numPix)
{
	BinarySpanOp(foregroundWithAlpha, background, 4, dst, numDstBands, numPix, 
		AlphaBlendRGBAOp<TI,TO>()); 
}

}

BEGIN_NI_NAMESPACE;

class ColorMap
{
public:
	virtual std::wstring GetName() const = 0;
	virtual bool IsCompatibleWith(const vt::CImgInfo& info) const = 0;
	virtual double GetDomainStart(int elementFormat) const = 0;
	virtual double GetDomainEnd(int elementFormat) const = 0;
	virtual HRESULT Map(const vt::CImg& src, vt::CRGBAByteImg& dst) const = 0;
	virtual bool UsesAlpha(const vt::CImgInfo& srcInfo) const = 0;
};

inline double GetDefaultDomainStart(int elementFormat)
{
	switch (EL_FORMAT(elementFormat))
	{
	default: 
		return 0.0;
	case EL_FORMAT_SBYTE:
		return -(vt::ElTraits<int8_t>::MaxVal() + 1);
	case EL_FORMAT_SSHORT:
		return -(vt::ElTraits<int16_t>::MaxVal() + 1);
	}
}

inline double GetDefaultDomainEnd(int elementFormat)
{
	switch (EL_FORMAT(elementFormat))
	{
	case EL_FORMAT_SBYTE:
		return vt::ElTraits<int8_t>::MaxVal();
	case EL_FORMAT_BYTE:
		return vt::ElTraits<vt::Byte>::MaxVal();
	case EL_FORMAT_SSHORT:
		return vt::ElTraits<int16_t>::MaxVal();
	case EL_FORMAT_SHORT:
		return vt::ElTraits<vt::UInt16>::MaxVal();
	case EL_FORMAT_INT:
		// for the purpose of color mapping, ints behave like floats or 
		// doubles since VtConv(float, int) doesnt scale (currently)
		return vt::ElTraits<float>::MaxVal();
	case EL_FORMAT_HALF_FLOAT:
		return vt::ElTraits<vt::HALF_FLOAT>::MaxVal();
	case EL_FORMAT_FLOAT:
		return vt::ElTraits<float>::MaxVal();
	case EL_FORMAT_DOUBLE:
		return vt::ElTraits<double>::MaxVal();
	default:
		return 0.0;
	}
}

class ScaleOffsetTransform : 
	public vt::CImageTransformUnaryPoint<ScaleOffsetTransform, true>
{
public:
	ScaleOffsetTransform(float scale, float offset, int refPixelType, 
		int numInputBands)
		: scale_(scale), offset_(offset), refPixelType_(refPixelType),
		numInputBands_(numInputBands)
	{		
	}

public:
	virtual void GetSrcPixFormat(int* pfrmtSrcs, 
		UINT, int /* frmtDst */)
	{ 
		pfrmtSrcs[0] = VT_IMG_FIXED(VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, 
			numInputBands_));
	}

	virtual void GetDstPixFormat(int& frmtDst, const int* pfrmtSrcs, UINT)
	{ 
		frmtDst = VT_IMG_FIXED(VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, 
			VT_IMG_BANDS(*pfrmtSrcs)));
	}

	virtual HRESULT Transform(vt::CImg* pimgDst, const vt::CRect& /* rctDst */, 
		const vt::CImg& src)
	{
		VT_HR_BEGIN();

		// we assume the transform framework has converted the input image 
		// to float, but originally the pixels were of type refPixelType_
		VT_ASSERT(EL_FORMAT(src.GetType()) == EL_FORMAT_FLOAT);

		float factor = 1.0f;
		switch (EL_FORMAT(refPixelType_))
		{
		case EL_FORMAT_SBYTE:
		case EL_FORMAT_BYTE:
			factor = 1.f/vt::ElTraits<vt::Byte>::MaxVal();
			break;
		case EL_FORMAT_SHORT:
		case EL_FORMAT_SSHORT:
			factor = 1.f/vt::ElTraits<vt::UInt16>::MaxVal();
			break;
		}

		float signedToUnsignedColormapShift = 0.f;
		switch (EL_FORMAT(refPixelType_))
		{
		case EL_FORMAT_SBYTE:			
		case EL_FORMAT_SSHORT:
			signedToUnsignedColormapShift = 0.5f;
			break;
		}

		VT_HR_EXIT(vt::VtScaleOffsetImage(*pimgDst, src, scale_, 
			offset_ * factor + signedToUnsignedColormapShift));

		VT_HR_END(;);
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
		return vt::CloneTaskState<ScaleOffsetTransform>(ppState, 
			new ScaleOffsetTransform(*this));
	}

private:
	float scale_;
	float offset_;
	int refPixelType_;
	int numInputBands_;
};

class ColorMapTransform : 
	public vt::CImageTransformUnaryPoint<ColorMapTransform, false>
{
public:
	ColorMapTransform(const ColorMap* map, int numSrcBands)
		: m_map(map), m_numSrcBands(numSrcBands)
	{
		VT_ASSERT(map != nullptr);
	}

public:
	virtual void GetSrcPixFormat(int* pfrmtSrcs, UINT, int /* frmtDst */)
	{ 
		pfrmtSrcs[0] = VT_IMG_FIXED(VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, m_numSrcBands));
	}

	virtual void GetDstPixFormat(int& frmtDst, const int*, UINT)
	{ 
		frmtDst = VT_IMG_FIXED(OBJ_RGBAIMG);
	}

	virtual HRESULT Transform(vt::CImg* pimgDst, const vt::CRect& /* rctDst */, 
		const vt::CImg& src)
	{
		VT_HR_BEGIN();

		if (src.Width() != pimgDst->Width() &&
			src.Height() != pimgDst->Height())
			return E_FAIL;

		vt::CRGBAByteImg dst;
		VT_HR_EXIT(pimgDst->Share(dst));

		VT_HR_EXIT(m_map->Map(src, dst));

		VT_HR_END(;);
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
		return vt::CloneTaskState<ColorMapTransform>(ppState, 
			new ColorMapTransform(*this));
	}

private:
	const ColorMap* m_map;
	int m_numSrcBands;
}; 

class RGBAFloatCheckerBoardTemplate
{
public:
	static const int Width = 1024;
	static const int GridSize = 8;

public:
	RGBAFloatCheckerBoardTemplate()
	{
		const float fg[4] = { .95f, .95f, .95f, 1.f };
		const float bg[4] = { .78f, .78f, .78f, 1.f };
		
		for (int x = 0; x < Width + GridSize; ++x)
		{
			bool p = (x / GridSize) % 2 != 0;
			memcpy(line0 + x * 4, p ? fg : bg, sizeof(fg));
			memcpy(line1 + x * 4, p ? bg : fg, sizeof(fg));
		}
	}

	const float* GetLine(int xofs, int yofs) const
	{
		if (yofs / GridSize % 2 != 0)
			return line0 + 4 * (xofs % GridSize);
		else
			return line1 + 4 * (xofs % GridSize);
	}

private:
	__declspec(align(16)) float line0[4*(Width + GridSize)];
	__declspec(align(16)) float line1[4*(Width + GridSize)];	
};

extern RGBAFloatCheckerBoardTemplate g_cbTemplate;

class AlphaBlendTransform :
	public vt::CImageTransformUnaryPoint<AlphaBlendTransform, false>
{
public:
	virtual HRESULT Transform(vt::CImg* pimgDst, const vt::CRect& rctDst, 
		const vt::CImg& src)
	{
		VT_HR_BEGIN();

		VT_HR_EXIT(g_cbTemplate.Width >= src.Width() ? S_OK : E_FAIL);

		VT_HR_EXIT(src.GetType() == OBJ_RGBAFLOATIMG ? S_OK : E_FAIL);
		VT_HR_EXIT(pimgDst->GetType() == OBJ_RGBAFLOATIMG ? S_OK : E_FAIL);
		
		for (int y = 0; y < src.Height(); ++y)
		{
			/*vt::VtConvertSpan(pimgDst->BytePtr(y), pimgDst->GetType(), 
				pimgDst->Bands(), 
				g_cbTemplate.GetLine(rctDst.left, y + rctDst.top), 
				EL_FORMAT_FLOAT, 4, 4 * src.Width());
			*/

			vt::VtAlphaBlendRGBASpan((float*)pimgDst->BytePtr(y), 
				pimgDst->Bands(), (float*)src.BytePtr(y), 
				g_cbTemplate.GetLine(rctDst.left, y + rctDst.top),
				src.Width());
		}

		VT_HR_END(;);
	}

	virtual HRESULT Clone(vt::ITaskState **ppState)
	{
		return vt::CloneTaskState<AlphaBlendTransform>(ppState, 
			new AlphaBlendTransform(*this));
	}
};

class DefaultColorMap : public ColorMap
{
public:
	virtual std::wstring GetName() const
	{
		return L"VisionTools Default";
	}

	virtual bool IsCompatibleWith(const vt::CImgInfo& info) const	
	{
		// can process all types of input if they have 1,3 or 4 bands, 
		// even though VtConvertImage only knows standard types 
		// (byte/float/halffloat/ushort). this is because 
		// ColorMapTransform fixes the input type to float
		bool elFormatOK = (EL_FORMAT(info.type) == EL_FORMAT_SBYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_BYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_SSHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_SHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_HALF_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_INT ||
			EL_FORMAT(info.type) == EL_FORMAT_DOUBLE);

		bool bandsOk = info.Bands() == 1 || info.Bands() == 3 || 
			info.Bands() == 4;

		return elFormatOK && bandsOk;
	}

	virtual double GetDomainStart(int elementFormat) const
	{
		return GetDefaultDomainStart(elementFormat);
	}

	virtual double GetDomainEnd(int elementFormat) const
	{
		return GetDefaultDomainEnd(elementFormat);
	}

	virtual HRESULT Map(const vt::CImg& src, vt::CRGBAByteImg& dst) const
	{
		return vt::VtConvertImage(dst, src);
	}

	virtual bool UsesAlpha(const vt::CImgInfo& srcInfo) const
	{
		return srcInfo.Bands() == 4;
	}
};

inline void YUVToRGBA(vt::Byte y, vt::Byte u, vt::Byte v, vt::RGBAPix& dst)
{
	const int c = y - 16;
	const int d = u - 128;
	const int e = v - 128;

	const int r = (298 * c           + 409 * e + 128) >> 8;
	const int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
	const int b = (298 * c + 516 * d           + 128) >> 8;

	dst.b = static_cast<vt::Byte>(vt::VtClamp(b, 0, 255));
	dst.g = static_cast<vt::Byte>(vt::VtClamp(g, 0, 255));
	dst.r = static_cast<vt::Byte>(vt::VtClamp(r, 0, 255));
	dst.a = 255;
}

class UVColorMap : public ColorMap
{
public:
	virtual std::wstring GetName() const
	{
		return L"UV";
	}

	virtual bool IsCompatibleWith(const vt::CImgInfo& info) const	
	{
		return info.Bands() == 2 &&
			(EL_FORMAT(info.type) == EL_FORMAT_SBYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_BYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_SSHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_SHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_HALF_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_INT ||
			EL_FORMAT(info.type) == EL_FORMAT_DOUBLE);
	}

	virtual double GetDomainStart(int elementFormat) const
	{
		return GetDefaultDomainStart(elementFormat);
	}

	virtual double GetDomainEnd(int elementFormat) const
	{
		return GetDefaultDomainEnd(elementFormat);
	}

	virtual HRESULT Map(const vt::CImg& src, vt::CRGBAByteImg& dst) const
	{
		if (EL_FORMAT(src.GetType()) != EL_FORMAT_FLOAT || src.Bands() != 2)
			return E_FAIL;
		
		const vt::CUVFloatImg& typedSrc = 
			static_cast<const vt::CUVFloatImg&>(src);

		for (int row = 0; row < src.Height(); ++row)
		{
			const auto* srcPtr = typedSrc.Ptr(row);
			auto* dstPtr = dst.Ptr(row);
			for (int x = 0; x < src.Width(); ++x)
			{
				const vt::Byte y = 172;
				vt::Byte u, v;
				vt::VtConv(&u, srcPtr[x].u); 
				vt::VtConv(&v, srcPtr[x].v); 

				YUVToRGBA(y, u, v, dstPtr[x]);
			}
		}
				 
		return S_OK;
	}

	virtual bool UsesAlpha(const vt::CImgInfo& /* srcInfo */) const
	{
		return false;
	}
};

class YUVColorMap : public ColorMap
{
public:
	virtual std::wstring GetName() const
	{
		return L"YUV";
	}

	virtual bool IsCompatibleWith(const vt::CImgInfo& info) const	
	{
		return info.Bands() == 3 &&
			(EL_FORMAT(info.type) == EL_FORMAT_SBYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_BYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_SSHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_SHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_HALF_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_INT ||
			EL_FORMAT(info.type) == EL_FORMAT_DOUBLE);
	}

	virtual double GetDomainStart(int elementFormat) const
	{
		return GetDefaultDomainStart(elementFormat);
	}

	virtual double GetDomainEnd(int elementFormat) const
	{
		return GetDefaultDomainEnd(elementFormat);
	}

	virtual HRESULT Map(const vt::CImg& src, vt::CRGBAByteImg& dst) const
	{
		if (EL_FORMAT(src.GetType()) != EL_FORMAT_FLOAT || src.Bands() != 3)
			return E_FAIL;
		
		// interpret rgb as yuv, for struct pixel access
		const vt::CRGBFloatImg& typedSrc = 
			static_cast<const vt::CRGBFloatImg&>(src);

		for (int row = 0; row < src.Height(); ++row)
		{
			const auto* srcPtr = typedSrc.Ptr(row);
			auto* dstPtr = dst.Ptr(row);
			for (int x = 0; x < src.Width(); ++x)
			{
				vt::Byte y, u, v;
				vt::VtConv(&y, srcPtr[x].b); 
				vt::VtConv(&u, srcPtr[x].g); 
				vt::VtConv(&v, srcPtr[x].r); 

				YUVToRGBA(y, u, v, dstPtr[x]);
			}
		}
				 
		return S_OK;
	}

	virtual bool UsesAlpha(const vt::CImgInfo& /* srcInfo */) const
	{
		return false;
	}
};

class YVUColorMap : public ColorMap
{
public:
	virtual std::wstring GetName() const
	{
		return L"YVU";
	}

	virtual bool IsCompatibleWith(const vt::CImgInfo& info) const	
	{
		return info.Bands() == 3 &&
			(EL_FORMAT(info.type) == EL_FORMAT_SBYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_BYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_SSHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_SHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_HALF_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_INT ||
			EL_FORMAT(info.type) == EL_FORMAT_DOUBLE);
	}

	virtual double GetDomainStart(int elementFormat) const
	{
		return GetDefaultDomainStart(elementFormat);
	}

	virtual double GetDomainEnd(int elementFormat) const
	{
		return GetDefaultDomainEnd(elementFormat);
	}

	virtual HRESULT Map(const vt::CImg& src, vt::CRGBAByteImg& dst) const
	{
		if (EL_FORMAT(src.GetType()) != EL_FORMAT_FLOAT || src.Bands() != 3)
			return E_FAIL;
		
		// interpret rgb as yvu, for struct pixel access
		const vt::CRGBFloatImg& typedSrc = 
			static_cast<const vt::CRGBFloatImg&>(src);

		for (int row = 0; row < src.Height(); ++row)
		{
			const auto* srcPtr = typedSrc.Ptr(row);
			auto* dstPtr = dst.Ptr(row);
			for (int x = 0; x < src.Width(); ++x)
			{
				vt::Byte y, v, u;
				vt::VtConv(&y, srcPtr[x].b); 
				vt::VtConv(&v, srcPtr[x].g); 
				vt::VtConv(&u, srcPtr[x].r); 

				YUVToRGBA(y, u, v, dstPtr[x]);
			}
		}
				 
		return S_OK;
	}

    virtual bool UsesAlpha(const vt::CImgInfo& /* srcInfo */) const
	{
		return false;
	}
};


class FlowColorMap : public ColorMap
{
public:
	FlowColorMap()
	{
		// relative lengths of color transitions:
		// these are chosen based on perceptual similarity
		// (e.g. one can distinguish more shades between red and yellow 
		//  than between yellow and green)
		int RY = 15;
		int YG = 6;
		int GC = 4;
		int CB = 11;
		int BM = 13;
		int MR = 6;
		ncols_ = RY + YG + GC + CB + BM + MR;
		VT_ASSERT(ncols_ <= MAXCOLS);
		int i;
		int k = 0;
		for (i = 0; i < RY; i++) setcols(255,	   255*i/RY,	 0,	k++);
		for (i = 0; i < YG; i++) setcols(255-255*i/YG, 255,		 0,	k++);
		for (i = 0; i < GC; i++) setcols(0,		   255,		 255*i/GC, k++);
		for (i = 0; i < CB; i++) setcols(0,		   255-255*i/CB, 255, k++);
		for (i = 0; i < BM; i++) setcols(255*i/BM,	   0,		 255, k++);
		for (i = 0; i < MR; i++) setcols(255,	   0,	255-255*i/MR, k++);
	}

	virtual std::wstring GetName() const
	{
		return L"Middlebury Flow";
	}

	virtual bool IsCompatibleWith(const vt::CImgInfo& info) const	
	{
		return EL_FORMAT(info.type) == EL_FORMAT_FLOAT && info.Bands() == 2;
	}

	virtual double GetDomainStart(int elementFormat) const
	{
		return GetDefaultDomainStart(elementFormat);
	}

	virtual double GetDomainEnd(int elementFormat) const
	{
		return GetDefaultDomainEnd(elementFormat);
	}

	virtual HRESULT Map(const vt::CImg& src, vt::CRGBAByteImg& dst) const
	{
		VT_ASSERT(EL_FORMAT(src.GetType()) == EL_FORMAT_FLOAT);
		VT_ASSERT(src.Bands() == 2);
		
		for (int y = 0; y < dst.Height(); ++y)
		{
			const vt::CVec2f* in = 
				reinterpret_cast<const vt::CVec2f*>(src.BytePtr(0, y));
			
			vt::Byte* out = dst.BytePtr(0, y);

			for (int x = 0; x < dst.Width(); ++x, ++in, out += 4)
			{
				float rad = static_cast<float>(sqrt(in->x * in->x + in->y * in->y));
				float a = (float)(atan2(-in->y, -in->x) / VT_PI);
				float fk = (a + 1.0f) / 2.0f * (ncols_-1);
				int k0 = (int)fk;
				int k1 = (k0 + 1) % ncols_;
				float f = fk - k0;
				//f = 0; // uncomment to see original color wheel
				for (int b = 0; b < 3; b++) 
				{
					float col0 = colorwheel_[k0][b] / 255.0f;
					float col1 = colorwheel_[k1][b] / 255.0f;
					float col = (1 - f) * col0 + f * col1;
					if (rad > 1.0f)
						rad = 1.0f;	
					if (rad < 0.0f)
						rad = 0.0f;	
					col = 1 - rad * (1 - col); // increase saturation with radius
					out[2 - b] = (vt::Byte)(255.0 * col);
				}
			}
		}

		return S_OK;
	}

    virtual bool UsesAlpha(const vt::CImgInfo& /* srcInfo */) const
	{
		return false;
	}

private:	
	int ncols_;
	static const int MAXCOLS = 60;
	int colorwheel_[MAXCOLS][3];

	void setcols(int r, int g, int b, int k)
	{
	    colorwheel_[k][0] = r;
		colorwheel_[k][1] = g;
		colorwheel_[k][2] = b;
	}	
};

class FirstThreeBGRColorMap : public ColorMap
{
public:
	virtual std::wstring GetName() const
	{
		return L"Band 0-2 BGR";
	}

	virtual bool IsCompatibleWith(const vt::CImgInfo& info) const	
	{
		return (EL_FORMAT(info.type) == EL_FORMAT_SBYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_BYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_SSHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_SHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_HALF_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_INT ||
			EL_FORMAT(info.type) == EL_FORMAT_DOUBLE);
	}

	virtual double GetDomainStart(int elementFormat) const
	{
		return GetDefaultDomainStart(elementFormat);
	}

	virtual double GetDomainEnd(int elementFormat) const
	{
		return GetDefaultDomainEnd(elementFormat);
	}

	virtual HRESULT Map(const vt::CImg& src, vt::CRGBAByteImg& dst) const
	{
		vt::Byte fill[4] = { 0, 0, 0, 255 };
		vt::BandIndexType bands[4] = { vt::BandIndex0, vt::BandIndexFill, 
			vt::BandIndexFill, vt::BandIndexFill };

		if (src.Bands() > 1)
			bands[1] = vt::BandIndex1;	
		
		if (src.Bands() > 2)
			bands[2] = vt::BandIndex2;
		
		return vt::VtConvertBands(dst, src, 4, bands, fill);
	}

	virtual bool UsesAlpha(const vt::CImgInfo&) const
	{
		return false;
	}
};

class RedGreenColorMap : public ColorMap
{
public:
	virtual std::wstring GetName() const
	{
		return L"Red/Green";
	}

	virtual bool IsCompatibleWith(const vt::CImgInfo& info) const	
	{
		return info.Bands() == 2 &&
			(EL_FORMAT(info.type) == EL_FORMAT_SBYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_BYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_SSHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_SHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_HALF_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_INT ||
			EL_FORMAT(info.type) == EL_FORMAT_DOUBLE);
	}

	virtual double GetDomainStart(int elementFormat) const
	{
		return GetDefaultDomainStart(elementFormat);
	}

	virtual double GetDomainEnd(int elementFormat) const
	{
		return GetDefaultDomainEnd(elementFormat);
	}

	virtual HRESULT Map(const vt::CImg& src, vt::CRGBAByteImg& dst) const
	{
		vt::Byte fill[4] = { 0, 0, 0, 255 };
		vt::BandIndexType bands[4] = { vt::BandIndexFill, 
			vt::BandIndex1, vt::BandIndex0, vt::BandIndexFill };

		return vt::VtConvertBands(dst, src, 4, bands, fill);
	}

	virtual bool UsesAlpha(const vt::CImgInfo&) const
	{
		return false;
	}
};

class RGBFlippedColorMap : public ColorMap
{
public:
	virtual std::wstring GetName() const
	{
		return L"RGB Flipped";
	}

	virtual bool IsCompatibleWith(const vt::CImgInfo& info) const	
	{
		return info.Bands() == 3 &&
			(EL_FORMAT(info.type) == EL_FORMAT_SBYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_BYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_SSHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_SHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_HALF_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_INT ||
			EL_FORMAT(info.type) == EL_FORMAT_DOUBLE);
	}

	virtual double GetDomainStart(int elementFormat) const
	{
		return GetDefaultDomainStart(elementFormat);
	}

	virtual double GetDomainEnd(int elementFormat) const
	{
		return GetDefaultDomainEnd(elementFormat);
	}

	virtual HRESULT Map(const vt::CImg& src, vt::CRGBAByteImg& dst) const
	{
		vt::Byte fill[4] = { 0, 0, 0, 255 };
		vt::BandIndexType bands[4] = { vt::BandIndex2, vt::BandIndex1, 
			vt::BandIndex0, vt::BandIndexFill };

		return vt::VtConvertBands(dst, src, 4, bands, fill);
	}

	virtual bool UsesAlpha(const vt::CImgInfo&) const
	{
		return false;
	}
};

class RGBAFlippedColorMap : public ColorMap
{
public:
	virtual std::wstring GetName() const
	{
		return L"RGBA Flipped";
	}

	virtual bool IsCompatibleWith(const vt::CImgInfo& info) const	
	{
		return info.Bands() == 4 &&
			(EL_FORMAT(info.type) == EL_FORMAT_SBYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_BYTE ||
			EL_FORMAT(info.type) == EL_FORMAT_SSHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_SHORT ||
			EL_FORMAT(info.type) == EL_FORMAT_HALF_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_FLOAT ||
			EL_FORMAT(info.type) == EL_FORMAT_INT ||
			EL_FORMAT(info.type) == EL_FORMAT_DOUBLE);
	}

	virtual double GetDomainStart(int elementFormat) const
	{
		return GetDefaultDomainStart(elementFormat);
	}

	virtual double GetDomainEnd(int elementFormat) const
	{
		return GetDefaultDomainEnd(elementFormat);
	}

	virtual HRESULT Map(const vt::CImg& src, vt::CRGBAByteImg& dst) const
	{
		vt::Byte fill[4] = { 0, 0, 0, 0 };
		vt::BandIndexType bands[4] = { vt::BandIndex2, vt::BandIndex1, 
			vt::BandIndex0, vt::BandIndex3 };

		return vt::VtConvertBands(dst, src, 4, bands, fill);
	}

	virtual bool UsesAlpha(const vt::CImgInfo&) const
	{
		return true;
	}
};

class NullColorMap : public ColorMap
{
public:
	virtual std::wstring GetName() const
	{
		return L"Null";
	}

	virtual bool IsCompatibleWith(const vt::CImgInfo&) const	
	{
		return true;
	}

	virtual double GetDomainStart(int elementFormat) const
	{
		return GetDefaultDomainStart(elementFormat);
	}

	virtual double GetDomainEnd(int elementFormat) const
	{
		return GetDefaultDomainEnd(elementFormat);
	}

	virtual HRESULT Map(const vt::CImg&, vt::CRGBAByteImg& dst) const
	{
		return dst.Clear();
	}

	virtual bool UsesAlpha(const vt::CImgInfo&) const
	{
		return false;
	}
};

class ColorMapStore
{
private:
	typedef std::vector<const ColorMap*> MapType;
	MapType maps_;

	DefaultColorMap defaultColorMap_;
	NullColorMap nullColorMap_;
	FirstThreeBGRColorMap firstThreeBGRColorMap_;
	RedGreenColorMap redGreenColorMap_;
	UVColorMap uvColorMap_;
	FlowColorMap flowColorMap_;
	YUVColorMap yuvColorMap_;
	YVUColorMap yvuColorMap_;
	RGBFlippedColorMap rgbFlippedColorMap_;
	RGBAFlippedColorMap rgbaFlippedColorMap_;

public:
	ColorMapStore()
	{
		// NOTE: the order here determines which map gets returned by 
		// GetDefaultMapName 
		maps_.push_back(&defaultColorMap_);
		maps_.push_back(&redGreenColorMap_);
		maps_.push_back(&firstThreeBGRColorMap_);
		maps_.push_back(&flowColorMap_);
		maps_.push_back(&uvColorMap_);
		maps_.push_back(&yuvColorMap_);
		maps_.push_back(&yvuColorMap_);
		maps_.push_back(&rgbFlippedColorMap_);
		maps_.push_back(&rgbaFlippedColorMap_);
	}

	const ColorMap* GetNullMap() const
	{
		return &nullColorMap_;
	}

	const ColorMap* GetMapByName(const std::wstring& name) const
	{
		auto it = std::find_if(maps_.begin(), maps_.end(), 
			[name](const MapType::value_type m)
		{ 
			return m->GetName() == name;
		});

		if (it != maps_.end())
			return *it;

		return nullptr;
	}

	std::wstring GetDefaultMapName(const vt::CImgInfo& info) const
	{
		auto it = std::find_if(maps_.begin(), maps_.end(), 
			[info](const MapType::value_type m)
		{
			return m->IsCompatibleWith(info);
		});

		if (it != maps_.end())
			return (*it)->GetName();

		// still return something, even if it's not compatible
		return defaultColorMap_.GetName();
	}

	std::wstring GetDefaultMapName() const
	{
		return defaultColorMap_.GetName();
	}

	std::vector<std::wstring> 
		GetCompatibleMapNames(const vt::CImgInfo& info) const
	{
		std::vector<std::wstring> res;

		std::for_each(maps_.begin(), maps_.end(), 
			[&res, info](const MapType::value_type m)
		{
			if (m->IsCompatibleWith(info))
				res.push_back(m->GetName());
		});

		return res;
	}
};

extern ColorMapStore g_colorMapStore;

public ref class ColorMapManager abstract sealed 
{
public:
	static String^ GetDefaultMapName()
	{
		return gcnew String(g_colorMapStore.GetDefaultMapName().c_str());
	}

	static String^ GetDefaultMapName(NativeImageBase^ ni)
	{
		if (ni == nullptr)
			throw gcnew ArgumentNullException();

		auto name = g_colorMapStore.GetDefaultMapName(
			reinterpret_cast<vt::IImageReader*>(
			ni->IImageReader.ToPointer())->GetImgInfo());

		return gcnew String(name.c_str());
	}

	static cli::array<String^>^ GetCompatibleMapNames(NativeImageBase^ ni)
	{
		if (ni == nullptr)
			throw gcnew ArgumentNullException();

		auto names = g_colorMapStore.GetCompatibleMapNames(
			reinterpret_cast<vt::IImageReader*>(
			ni->IImageReader.ToPointer())->GetImgInfo());

		cli::array<String^>^ res = 
			gcnew cli::array<String^>((int)names.size());

		for (size_t i=0; i<names.size(); ++i)
			res[(int)i] = gcnew String(names[i].c_str());

		return res;
	}

	static cli::array<String^>^ GetCompatibleSingleBandMapNames(
		NativeImageBase^ ni)
	{
		if (ni == nullptr)
			throw gcnew ArgumentNullException();

		auto info = reinterpret_cast<vt::IImageReader*>(
			ni->IImageReader.ToPointer())->GetImgInfo();
		info.type = VT_IMG_MAKE_TYPE(EL_FORMAT(info.type), 1);
		auto names = g_colorMapStore.GetCompatibleMapNames(info);

		cli::array<String^>^ res = 
			gcnew cli::array<String^>((int)names.size());

		for (size_t i=0; i<names.size(); ++i)
			res[(int)i] = gcnew String(names[i].c_str());

		return res;
	}

	static double GetMapDomainStart(String^ name, NativeImageBase^ ni)
	{
		if (String::IsNullOrEmpty(name))
			throw gcnew ArgumentException();

		if (ni == nullptr)
			throw gcnew ArgumentNullException();

		const ColorMap* map = g_colorMapStore.GetMapByName(
			Details::ManagedToNativeString(name));

		if (map == nullptr)
			throw gcnew ApplicationException();

		int elFormat = EL_FORMAT(reinterpret_cast<vt::IImageReader*>(
			ni->IImageReader.ToPointer())->GetImgInfo().type);

		return map->GetDomainStart(elFormat);
	}

	static double GetMapDomainEnd(String^ name, NativeImageBase^ ni)
	{
		if (String::IsNullOrEmpty(name))
			throw gcnew ArgumentException();

		if (ni == nullptr)
			throw gcnew ArgumentNullException();

		const ColorMap* map = g_colorMapStore.GetMapByName(
			Details::ManagedToNativeString(name));

		if (map == nullptr)
			throw gcnew ApplicationException();

		int elFormat = EL_FORMAT(reinterpret_cast<vt::IImageReader*>(
			ni->IImageReader.ToPointer())->GetImgInfo().type);

		return map->GetDomainEnd(elFormat);
	}
};

END_NI_NAMESPACE