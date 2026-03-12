#pragma once

#include "vtcore.h"

class DrawPixelGridTransform :
	public vt::CImageTransformUnaryPoint<DrawPixelGridTransform, false>
{
public:
	DrawPixelGridTransform(const vt::CMtx3x3f& warpMatrix, 
		const vt::RGBPix gridColor, vt::Byte opacity, float startScale)
		: warpMatrix_(warpMatrix), gridColor_(gridColor), opacity_(opacity),
		startScale_(startScale)
	{
	}

public:
	virtual HRESULT Transform(vt::CImg* pimgDst, const vt::CRect& rctDst, 
		const vt::CImg& src)
	{
		if (warpMatrix_(0,0) <= 1.f/startScale_)
		{
			if (EL_FORMAT(pimgDst->GetType()) != EL_FORMAT_FLOAT || 
				pimgDst->Bands() != 4)
				return E_FAIL;
			if (EL_FORMAT(src.GetType()) != EL_FORMAT_FLOAT || 
				src.Bands() != 4)
				return E_FAIL;

			vt::RGBFloatPix c;
			vt::VtConv(&c.r, gridColor_.r);
			vt::VtConv(&c.g, gridColor_.g);
			vt::VtConv(&c.b, gridColor_.b);
			
			float o;
			vt::VtConv(&o, opacity_);

			DrawNNGrid(*static_cast<vt::CRGBAFloatImg*>(pimgDst), 
				static_cast<const vt::CRGBAFloatImg&>(src), 
				warpMatrix_, rctDst.left, rctDst.top, c, o);

			return S_OK;
		}
		else
		{			
			return vt::VtConvertImage(*pimgDst, src);
		}
	}

	virtual HRESULT Clone(vt::ITaskState **ppState)
	{
		return vt::CloneTaskState<DrawPixelGridTransform>(ppState, 
			new DrawPixelGridTransform(*this));
	}

private:	
	static void BlendRGBPixel(vt::RGBAFloatPix& dst, 
		const vt::RGBAFloatPix& src, const vt::RGBFloatPix& overlay, 
		float opacity)
	{
		dst.r = (src.r * opacity + (1.f - opacity)*overlay.r);
		dst.g = (src.g * opacity + (1.f - opacity)*overlay.g);
		dst.b = (src.b * opacity + (1.f - opacity)*overlay.b);
		
		dst.a = 1.0f;
	}

	static void DrawNNGrid(vt::CRGBAFloatImg& dst, 
		const vt::CRGBAFloatImg& src, const vt::CMtx3x3f& xfrm, 
		int xofs, int yofs,
		const vt::RGBFloatPix& gridColor, float opacity)
	{
		for (int x=0; x<dst.Width(); ++x)
		{
			int prev = (int)(xfrm * 
				vt::CVec3f((float)(xofs + x - 1), 0, 1.f)).Dehom().x;
			int curr = (int)(xfrm * 
				vt::CVec3f((float)(xofs + x), 0, 1.f)).Dehom().x;
			int next = (int)(xfrm * 
				vt::CVec3f((float)(xofs + x + 1), 0, 1.f)).Dehom().x;

			if (prev != curr || next != curr)
			{
				auto* srcPtr = src.Ptr();
				auto* dstPtr = dst.Ptr();
				for (int y=0; y<dst.Height(); ++y)
				{
					BlendRGBPixel(dstPtr[x], srcPtr[x], gridColor, opacity);
					srcPtr = src.NextRow(srcPtr);
					dstPtr = dst.NextRow(dstPtr);
				}
			}		
		}

		for (int y=0; y<dst.Height(); ++y)
		{
			int prev = (int)(xfrm * 
				vt::CVec3f(0, (float)(yofs + y - 1), 1.f)).Dehom().y;
			int curr = (int)(xfrm * 
				vt::CVec3f(0, (float)(yofs + y), 1.f)).Dehom().y;
			int next = (int)(xfrm * 
				vt::CVec3f(0, (float)(yofs + y + 1), 1.f)).Dehom().y;

			if (prev != curr || next != curr)
			{
				auto* srcPtr = src.Ptr(y);
				auto* dstPtr = dst.Ptr(y);
				for (int x=0; x<dst.Width(); ++x)
					BlendRGBPixel(dstPtr[x], srcPtr[x], gridColor, opacity);
			}		
		}
	}

private:
	const vt::CMtx3x3f warpMatrix_;
	const vt::RGBPix gridColor_;
	vt::Byte opacity_;
	float startScale_;
};