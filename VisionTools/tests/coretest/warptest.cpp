////////////////////////////////////////////////////////////////////////////////
// coretest.cpp : Unit tests for VisionTools core
//
#include "stdafx.h"

#include "cthelpers.h"

using namespace vt;


template<class T>
T Flip(T v, int d)
{
	return T(d - 1) - v;
}

////////////////////////////////////////////////////////////////////////////////
//
// basic tests of the image warp function
//
BEGIN_TEST_FILE(WarpTests)

class CRot180AG: public IAddressGenerator
{
public:
	virtual vt::CRect MapDstRectToSrc(const CRect &rctDst)
	{
		return vt::CRect(Flip(rctDst.right-1,  m_iW),
						 Flip(rctDst.bottom-1, m_iH),
						 Flip(rctDst.left,   m_iW)+1,
						 Flip(rctDst.top,    m_iH)+1);
	}

	virtual vt::CRect MapSrcRectToDst(const CRect &rctSrc)
	{ return MapDstRectToSrc(rctSrc); }

	virtual HRESULT MapDstSpanToSrc(OUT CVec2f *pSpan, const vt::CPoint &ptDst, int iSpan)
	{
		float y = float(Flip(ptDst.y, m_iH));  
		for( int i = 0; i < iSpan; i++, pSpan++ )
		{
			pSpan->x = float(Flip(ptDst.x+i, m_iW));
			pSpan->y = y;
		}
		return S_OK;
	}

	virtual HRESULT MapDstAddrToSrc(OUT CVec2f *pSpan, int iSpan)
	{
		for( int i = 0; i < iSpan; i++, pSpan++ )
		{
			pSpan->x = Flip(pSpan->x, m_iW);
			pSpan->y = Flip(pSpan->y, m_iH);
		}
		return S_OK;
	}

	virtual HRESULT Clone(IAddressGenerator **ppClone)
	{ 
        HRESULT hr = CloneAddressGenerator<CRot180AG>(ppClone);
        if( hr == S_OK )
        {
            ((CRot180AG*)*ppClone)->m_iW = m_iW;
            ((CRot180AG*)*ppClone)->m_iH = m_iH;
        }
        return hr;
    }

public:
    CRot180AG() : m_iW(0), m_iH(0)
	{}

	CRot180AG(int w, int h) : m_iW(w), m_iH(h)
	{}

protected:
	int m_iW, m_iH;
};

BEGIN_TEST_VTR_FAST(WarpTest)
{
	std::wstring warpimage = GetImageDir() + L"\\newman.png";
 
	CRGBAImg input;
	LoadImage(warpimage, input);

	for( int i = 0; i < 2; i++ )
	{
		bool bOffset = (i==0);

		CFlowImg imgFlow;
		WIN_ASSERT_EQUAL( S_OK, imgFlow.Create(320, 240) );
		for( int y = 0; y < imgFlow.Height(); y++ )
		{
			CVec2f* pV = imgFlow.Ptr(y);
			for( int x = 0; x < imgFlow.Width(); x++, pV++ )
			{
				if( bOffset )
				{
					*pV = CVec2f(0.5f, 0.5f);
				}
				else
				{
					*pV = CVec2f(float(x)+0.5f, float(y)+0.5f);
				}
			}
		}
	
		CRGBImg out1;
		WIN_ASSERT_EQUAL( S_OK, VtWarpImage(out1, imgFlow.Rect(), input, 
											imgFlow, eSamplerKernelBilinear, 
											IMAGE_EXTEND(Zero), bOffset) );
	
		CMtx3x3f matT = CMtx3x3f().MakeTranslate(0.5f, 0.5f);
		CRGBImg out2;
		WIN_ASSERT_EQUAL( S_OK, VtWarpImage(
			out2, vt::CRect(0,0,imgFlow.Width()-1, imgFlow.Height()-1), 
			input, matT) );
	
		CRGBImg out1Cmp;
		RECT r = out2.Rect();
		out1.Share(out1Cmp, &r);
		WIN_ASSERT_EQUAL( true, VtCompareImages(out1Cmp, out2, 1, NULL, 1.0001f/255.f) );
	
		CRGBImgInCache out3;
		WIN_ASSERT_EQUAL( S_OK, out3.Create(imgFlow.Width(), imgFlow.Height()) );
	
		CRot180AG r1(imgFlow.Width(), imgFlow.Height());
		CRot180AG r2(imgFlow.Width(), imgFlow.Height());
		CFlowFieldAddressGen r3;
		WIN_ASSERT_EQUAL( S_OK, r3.Initialize(imgFlow, bOffset) );
		IAddressGenerator* pAG[3] = {&r1, &r2, &r3};
	
		CWarpTransform x;
		WIN_ASSERT_EQUAL( S_OK, x.Initialize(pAG,3,VT_IMG_FIXED(OBJ_RGBAIMG)) );
		
		CImgReaderWriter<CRGBAImg> input2;
		WIN_ASSERT_EQUAL( S_OK, input.Share(input2) );
		CTransformGraphUnaryNode g(&x, &input2, &out3);
		WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&g) );
	
		WIN_ASSERT_EQUAL( true, VtCompareImages(out1, &out3) );
	}
}
END_TEST_VTR

eFlipMode TranslateFlipMode(int r)
{
	VT_ASSERT( r >= 4 && r < 7 );
	if( r == 4 )      return vt::eFlipModeTranspose;
	else if( r == 5 ) return vt::eFlipModeHorizontal;
	else              return vt::eFlipModeVertical;
}

BEGIN_TEST_VTR_FAST(RotateTest)
{
	std::wstring warpimage = GetImageDir() + L"\\newman_small.png";
     
    int types[4] = {EL_FORMAT_BYTE, EL_FORMAT_SHORT, EL_FORMAT_FLOAT, EL_FORMAT_HALF_FLOAT};
	
    CRGBAImg input;
	LoadImage(warpimage, input);

	for ( int i = 0; i < 4; i++ )
	{
		for ( int srcb = 1; srcb <= 4; srcb++ )
		{
			if (srcb==2) continue;

			int srctype = VT_IMG_MAKE_TYPE(types[i], srcb);

			// create an image of source type
			CImgReaderWriter<CImg> src;   
			WIN_ASSERT_EQUAL( S_OK, ((CImg&)src).Create(
				input.Width(), input.Height(), srctype) );
			WIN_ASSERT_EQUAL( S_OK, VtConvertImage(src, input) );

			for ( int j = 0; j < 4; j++ )
			{
				for ( int dstb = 1; dstb <= 4; dstb++ )
				{
					if (dstb==2) continue;
									
					int dsttype = VT_IMG_FIXED(VT_IMG_MAKE_TYPE(types[j], dstb));

					// rotate by 0 through 270 degrees in steps of 90
					for( int r = 0; r < 7; r++ )
					{
						int iDstW, iDstH;
						if( r < 4 )
						{
							iDstW = (r&1)? src.Height(): src.Width();
							iDstH = (r&1)? src.Width(): src.Height();
						}
						else
						{
							iDstW = (r==4)? src.Height(): src.Width();
							iDstH = (r==4)? src.Width(): src.Height();
						}

						// create an image of destination type
						CImg dstrot;   
						WIN_ASSERT_EQUAL( S_OK, dstrot.Create(iDstW, iDstH, dsttype) );

						// run rotate
						if( r < 4 )
						{
							WIN_ASSERT_EQUAL( S_OK, VtRotateImage(dstrot, src, r) );
						}
						else
						{
							WIN_ASSERT_EQUAL( S_OK, VtFlipImage(
								dstrot, src, TranslateFlipMode(r)) );
						}
                                                
						// run warp version of rotate and compare
						CMtx3x3f m;
						switch(r)
						{
						case 0:
						    m = CMtx3x3f(1,0,0,0,1,0,0,0,1);
							break;
						case 1:
							m = CMtx3x3f(0,1,0,-1,0,float(iDstW-1),0,0,1);
							break;
						case 2:
							m = CMtx3x3f(-1,0,float(iDstW-1),0,-1,float(iDstH-1),0,0,1);
							break;
						case 3:
							m = CMtx3x3f(0,-1,float(iDstH-1),1,0,0,0,0,1);
							break;
						case 4:
							m = CMtx3x3f(0,1,0,1,0,0,0,0,1);
							break;
						case 5:
							m = CMtx3x3f(-1,0,float(iDstW-1),0,1,0,0,0,1);
							break;
						case 6:
							m = CMtx3x3f(1,0,0,0,-1,float(iDstH-1),0,0,1);
							break;
						}

						CImg dstwarp;   
						WIN_ASSERT_EQUAL( S_OK, dstwarp.Create(iDstW, iDstH, dsttype) );

						WIN_ASSERT_EQUAL( 
							S_OK, VtWarpImage(dstwarp, dstwarp.Rect(), src, m));

						// special case short to byte because Warp() goes
						// through a float conversion that rotate doesn't do;
                        // also special case all byte source and non-byte dest
                        // cases since Warp calls Resize and Resize does byte
                        // source processing for all dest types
                        WIN_ASSERT_EQUAL( true, VtCompareImages(
							dstrot, dstwarp, 1, NULL,
                            ((i==1 && j==0)||((i==0)&&(j>0)))? 1.001f/255.f: 0.f) );

						// test the transform version
						CImgReaderWriter<CImg> dstxfrm;
						WIN_ASSERT_EQUAL( S_OK, ((CImg&)dstxfrm).Create(
							iDstW, iDstH, dsttype) );

						CRotateTransform x;
						if( r < 4 )
						{
							x.InitializeRotate(src.Width(), src.Height(), r, 
								               VT_IMG_MAKE_TYPE(dsttype, dstb));
						}
						else
						{
							x.InitializeFlip(src.Width(), src.Height(), TranslateFlipMode(r), 
											 VT_IMG_MAKE_TYPE(dsttype, dstb));
						}

						CTransformGraphUnaryNode g(&x, &src, &dstxfrm);
						WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&g) );
	
						// special case short to byte because Warp() goes
						// through a float conversion that rotate doesn't do   
						WIN_ASSERT_EQUAL( true, VtCompareImages(
							dstrot, dstxfrm, 1, NULL,
							(i==1 && j==0)? 1.001f/255.f: 0.f) );

                        /*if( !VtCompareImages(dstrot, dstwarp) )
                        {
                            vt::wstring_b<512> fname;
						    fname.format(L"c:\\temp\\rot_%d_%d_%d_%d_%dw.png",
									     i,srcb,j,dstb,r);
						    VtSaveImage(fname, dstwarp);
                                                    
						    fname.format(L"c:\\temp\\rot_%d_%d_%d_%d_%d.png",
						  			     i,srcb,j,dstb,r);
						    VtSaveImage(fname, dstrot);
                        }*/
					}
				}
			}
		}
	}
}
END_TEST_VTR

BEGIN_TEST_VTR_FAST(MultibandWarp)
{
    std::wstring warpimage = GetImageDir() + L"\\newman_large.png";

    CRGBAImg input;
    LoadImage(warpimage, input);

	for( int cb = 1; cb <= 3; cb++ )
	{
		CByteImg imgMB;
		WIN_ASSERT_EQUAL( S_OK, imgMB.Create(input.Width(), input.Height(), cb) );

		// create a multiband image out of newman
		for( int y = 0; y < imgMB.Height(); y++ )
		{
			Byte* pDst = imgMB.Ptr(y);
			Byte* pSrc = input.BytePtr(y);
			for( int x = 0; x < imgMB.Width(); x++, pSrc+=input.Bands() )
			{
				for( int b = 0; b < cb; b++, pDst++ )
				{
					*pDst = pSrc[b%input.Bands()];
				}
			}
		}

		// warp newman and warp multiband image
		vt::CRect rctDst(0,0,input.Width(), input.Height());
		CMtx3x3f rot;
		rot.MakeRotation(float(3.14159/180.f));
		CRGBImg  input_w;
		CByteImg imgMB_w;
		WIN_ASSERT_EQUAL( S_OK, VtWarpImage(input_w, rctDst, input, rot) );
		WIN_ASSERT_EQUAL( S_OK, VtWarpImage(imgMB_w, rctDst, imgMB, rot) );

		// pull out one band at a time and compare
		CLumaByteImg input_w_b, imgMB_w_b;
		for( int b = 0; b < cb; b++ )
		{
			BandIndexType inidx = BandIndexType(b%input.Bands());
			BandIndexType mbidx = BandIndexType(b);
			VtConvertBands(input_w_b, input_w, 1, &inidx);
			VtConvertBands(imgMB_w_b, imgMB_w, 1, &mbidx);
			WIN_ASSERT_EQUAL( true, VtCompareImages(input_w_b, imgMB_w_b, 1, NULL, 2.001f/255.f) );
		}
	}
}
END_TEST_VTR

BEGIN_TEST_VTR_SLOW(MultibandResize)
{
    std::wstring warpimage = GetImageDir() + L"\\newman_large.png";

    CRGBImg input;
    LoadImage(warpimage, input);

	for( int cb = 5; cb <= 8; cb++ )
	{
		CByteImg imgMB;
		WIN_ASSERT_EQUAL( S_OK, imgMB.Create(input.Width(), input.Height(), cb) );

		// create a multiband image out of newman
		for( int y = 0; y < imgMB.Height(); y++ )
		{
			Byte* pDst = imgMB.Ptr(y);
			Byte* pSrc = input.BytePtr(y);
			for( int x = 0; x < imgMB.Width(); x++, pSrc+=input.Bands() )
			{
				for( int b = 0; b < cb; b++, pDst++ )
				{
					*pDst = pSrc[b%input.Bands()];
				}
			}
		}

		// resize the newman and resize the multiband image
		vt::CRect rctDst(0,0,input.Width()/2, input.Height()/2);
		CRGBImg  input_2;
		CByteImg imgMB_2;
		WIN_ASSERT_EQUAL( S_OK, VtResizeImage(input_2, rctDst, input) );
		WIN_ASSERT_EQUAL( S_OK, VtResizeImage(imgMB_2, rctDst, imgMB) );

		// pull out one band at a time and compare
		CLumaByteImg input_2_b, imgMB_2_b;
		for( int b = 0; b < cb; b++ )
		{
			BandIndexType inidx = BandIndexType(b%input.Bands());
			BandIndexType mbidx = BandIndexType(b);
			VtConvertBands(input_2_b, input_2, 1, &inidx);
			VtConvertBands(imgMB_2_b, imgMB_2, 1, &mbidx);
			WIN_ASSERT_EQUAL( true, VtCompareImages(input_2_b, imgMB_2_b) );
		}
	}
}
END_TEST_VTR

int printMessage(const char *pszFormat, ...)
{
	char tmp[4096];
	va_list marker;
	va_start(marker, pszFormat);
	vsprintf_s(tmp, sizeof(tmp) / sizeof(char), pszFormat, marker);
	OutputDebugStringA(tmp);
	printf("%s", tmp); fflush(stdout);
	return 0;
}

BEGIN_TEST_VTR_SLOW(ResizeBandFormatMix)
{
    std::wstring testimage = GetImageDir() + L"\\newman.png";

    const int nimg = 16;
    int dstw = 300;
    int dsth = 200;
    CImg *psrc[nimg], *pdst[nimg];
#define INST_IMG_(_NUM_,_BAND_,_FORMAT_) \
    C##_BAND_##_FORMAT_##Img s##_NUM_, d##_NUM_; psrc[_NUM_] = &s##_NUM_; pdst[_NUM_] = &d##_NUM_; \
    WIN_ASSERT_EQUAL(S_OK, d##_NUM_.Create(dstw,dsth));

    INST_IMG_( 0,Luma,Byte)
    INST_IMG_( 1,Luma,Short)
    INST_IMG_( 2,Luma,HalfFloat)
    INST_IMG_( 3,Luma,Float)
    INST_IMG_( 4,UV,Byte)
    INST_IMG_( 5,UV,Short)
    INST_IMG_( 6,UV,HalfFloat)
    INST_IMG_( 7,UV,Float)
    INST_IMG_( 8,RGB,Byte)
    INST_IMG_( 9,RGB,Short)
    INST_IMG_(10,RGB,HalfFloat)
    INST_IMG_(11,RGB,Float)
    INST_IMG_(12,RGBA,Byte)
    INST_IMG_(13,RGBA,Short)
    INST_IMG_(14,RGBA,HalfFloat)
    INST_IMG_(15,RGBA,Float)

    CLumaByteImg imgRef;
    WIN_ASSERT_EQUAL(S_OK, imgRef.Create(dstw,dsth) );
    {
        CLumaByteImg imgb1;
        LoadImage(testimage, imgb1);
        WIN_ASSERT_EQUAL(S_OK, VtResizeImage(imgRef,imgRef.Rect(),imgb1) );
        CRGBByteImg imgb3;
        WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgb3,imgb1) );
        CUVByteImg imgb2;
        BandIndexType bits[] = { BandIndex0, BandIndex1 };
        WIN_ASSERT_EQUAL(S_OK, VtConvertBands(imgb2,imgb3,2,bits) );
        CRGBAByteImg imgb4;
        WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgb4,imgb1) );
        for (int y=0; y<imgb4.Height(); y++)
        {
            for (int x=0; x<imgb4.Width(); x++)
            {
                RGBAPix pix = *imgb4.Ptr(x,y);
                pix.a = pix.r;
                *imgb4.Ptr(x,y) = pix;
            }
        }
        for (int i=0; i<nimg; i++)
        {
            switch(psrc[i]->Bands())
            {
            default:
            case 1: WIN_ASSERT_EQUAL(S_OK, VtConvertImage(*psrc[i], imgb1) ); break;
            case 2: WIN_ASSERT_EQUAL(S_OK, VtConvertImage(*psrc[i], imgb2) ); break;
            case 3: WIN_ASSERT_EQUAL(S_OK, VtConvertImage(*psrc[i], imgb3) ); break;
            case 4: WIN_ASSERT_EQUAL(S_OK, VtConvertImage(*psrc[i], imgb4) ); break;
            }
        }
    }
    for (int si=0; si<nimg; si++)
    {
        for (int di=0; di<nimg; di++)
        {
            if (VtIsValidConvertImagePair(*pdst[di],*psrc[si]))
            {
                WIN_ASSERT_EQUAL(S_OK, VtResizeImage(*pdst[di],pdst[di]->Rect(),*psrc[si]) );
                for (int b=0; b<pdst[di]->Bands(); b++)
                {
                    if ((b==3) && (psrc[si]->Bands()<4)) { continue; }
                    CLumaByteImg tbimg;
           			BandIndexType mbidx = BandIndexType(b);
        			WIN_ASSERT_EQUAL(S_OK, VtConvertBands(tbimg, *pdst[di], 1, &mbidx) );
                    //printMessage(" %d %d %d\n",si,di,b);
        			WIN_ASSERT_EQUAL( true, VtCompareImages(imgRef, tbimg, 1, NULL, 2.001f/255.f) );
                }
            }
        }
    }

}
END_TEST_VTR

//
// Verifies that per-channel results are identical regardless of the number of
// channels; this is guaranteed by Vision Tools for 1..4 bands only (due to 
// byte specialization that is only done for 1..4 bands); this tests byte i/o
// and bilinear only
//
void _FAST_Abc()
{
};

BEGIN_TEST_VTR_FAST(_1to4BandByteConsistency)
{
    std::wstring warpimage = GetImageDir() + L"\\newman.png";

    CLumaByteImg input;
    LoadImage(warpimage, input);

    // resize the source image
    vt::CRect rctDst(0,0,input.Width()-23, input.Height()+58);
	CLumaByteImg input_2;
    WIN_ASSERT_EQUAL( S_OK, VtResizeImage(input_2, rctDst, input, eSamplerKernelBilinear) );

	for( int cb = 2; cb <= 4; cb++ )
	{
		CByteImg imgMB;
		WIN_ASSERT_EQUAL( S_OK, imgMB.Create(input.Width(), input.Height(), cb) );

		// create a multiband image out of newman
		for( int y = 0; y < imgMB.Height(); y++ )
		{
			Byte* pDst = imgMB.BytePtr(y);
			Byte* pSrc = input.BytePtr(y);
			for( int x = 0; x < imgMB.Width(); x++, pSrc++ )
			{
				for( int b = 0; b < cb; b++, pDst++ )
				{
					*pDst = *pSrc;
				}
			}
		}

		// resize the multiband image
		CByteImg imgMB_2;
		WIN_ASSERT_EQUAL( S_OK, VtResizeImage(imgMB_2, rctDst, imgMB, eSamplerKernelBilinear) );

		// pull out one band at a time and compare
		CLumaByteImg imgMB_2_b;
		for( int b = 0; b < cb; b++ )
		{
			BandIndexType mbidx = BandIndexType(b);
			VtConvertBands(imgMB_2_b, imgMB_2, 1, &mbidx);
            // no tolerance - should match exactly
			WIN_ASSERT_EQUAL( true, VtCompareImages(input_2, imgMB_2_b) );
		}
	}
}
END_TEST_VTR


float LUBW(float sx, float sy, int w, int h)
{ return ( sx < 0 || sx >= w || sy < 0 || sy >= h)? 0.f: 255.f; }

bool VerifyTranslate(const CLumaImg& d, int sw, int sh, float tx, float ty)
{
	for( int y = 0; y < d.Height(); y++ )
	{
		for( int x = 0; x < d.Width(); x++ )
		{
			float sx = float(x) + tx;
			float sy = float(y) + ty;   
			float cx = sx - floor(sx);
			float cy = sy - floor(sy);

			float v0 = LUBW(sx,   sy, sw, sh);
			float v1 = LUBW(sx+1, sy, sw, sh);
			float v2 = LUBW(sx,   sy+1, sw, sh);
			float v3 = LUBW(sx+1, sy+1, sw, sh);

			float v = (cx*v1+(1.f-cx)*v0)*(1-cy) + (cx*v3+(1.f-cx)*v2)*cy;

			if( d.Pix(x,y) != F2I(v) )
			{
				return false;
			}
		}
	}
	return true;
}

BEGIN_TEST_VTR_FAST(TranslateTest)
{
	CLumaImg w(32, 24);
	w.Fill(255);

	for( int i = 0; i < 4; i++ )
	{
		CLumaImg d;
		float tx = (i==0)? 3.5f: (i==1)? -3.5f: 0;
		float ty = (i==2)? 3.5f: (i==3)? -3.5f: 0;

		WIN_ASSERT_EQUAL( S_OK, VtResizeImage(d, vt::CRect(0,0,40,32), w, 
											  1.0, tx, 1.0, ty,
											  vt::eSamplerKernelBilinear,
											  IMAGE_EXTEND(Zero)) );

		WIN_ASSERT_TRUE( VerifyTranslate(d, w.Width(), w.Height(), tx, ty) );
	}
}
END_TEST_VTR


#if 0
extern HRESULT
VtSeparableFilter2(CImg &imgDst, const CRect& rctDst, 
                   const CImg &imgSrc, CPoint ptSrcOrigin,
                   const C1dKernelSet& ksh, const C1dKernelSet& ksv,
                   const IMAGE_EXTEND& ex=IMAGE_EXTEND(Extend));

BEGIN_TEST_VTR_FAST(FilterTest)
{ 
    std::wstring warpimage = GetImageDir() + L"\\newman_large.png";

    CRGBAImg input;
    LoadImage(warpimage, input);

    int types[3] = {EL_FORMAT_BYTE, EL_FORMAT_SHORT, EL_FORMAT_FLOAT};

    vt::wstring_b<256> fname;

    //C1d8TapMipMapKernel k8;
    //const C1dKernel& k = k8.AsKernel();
    C1dKernel k;
    WIN_ASSERT_EQUAL( S_OK, Create1dGaussKernel(k, 1.f) );

	C1dKernelSet ks;
	WIN_ASSERT_EQUAL( S_OK, ks.Create(k) );

    /*C1dKernelSet krnlsetUp;
	float arfKrnlUp0[] = {1.f/8.f, 6.f/8.f, 1.f/8.f};
	float arfKrnlUp1[] = {4.f/8.f, 4.f/8.f};
	krnlsetUp.Create(2, 1);
	krnlsetUp.Set(0, -1, 3, arfKrnlUp0);
	krnlsetUp.Set(1, 0, 2, arfKrnlUp1);*/

    for ( int i = 0; i < 3; i++ )
    {
        int srctype = types[i];
        for ( int srcb = 1; srcb <= 4; srcb++ )
        {
            if (srcb==2) continue;

            for ( int j = 0; j < 3; j++ )
            {
                int dsttype = types[j];

                for ( int dstb = 1; dstb <= 4; dstb++ )
                {
                    if (dstb==2) continue;
                    printf("srctype(%d), srcbands(%d) => dsttype(%d), dstbands(%d), kernel length(%d)\n", 
						   srctype, srcb, dsttype, dstb, k.Width());

                    // separable perf test
                    CImg src;   
                    WIN_ASSERT_EQUAL( S_OK, src.Create(
						input.Width(), input.Height(), srcb, 
						VtElSizeFromType(srctype), srctype) );
                    WIN_ASSERT_EQUAL( S_OK, VtConvertImage(src, input) );

                    // new perf test
                    CImg dstnew;   
                    WIN_ASSERT_EQUAL( S_OK, dstnew.Create(
						input.Width(), input.Height(), dstb, 
						VtElSizeFromType(dsttype), dsttype) );

                    CTimer tnewsep;
                    WIN_ASSERT_EQUAL( S_OK, VtSeparableFilter(
						dstnew, dstnew.Rect(), src, vt::CPoint(0,0), k, 
						IMAGE_EXTEND(ExtendWithAlpha)) );
					printf("    new separable filter took %f ms\n", tnewsep.GetTimeMilliSec());

                    CTimer tnewpoly;
					VtSeparableFilter2(dstnew, dstnew.Rect(), src, vt::CPoint(0,0), 
									   ks, ks, IMAGE_EXTEND(ExtendWithAlpha));
                    printf("    new polyphase filter took %f ms\n", tnewpoly.GetTimeMilliSec());
                    fname.format(L"c:\\temp\\newpoly_%d_%d_%d_%d_N.png",i,srcb,j,dstb);
                    VtSaveImage(fname, dstnew);
#if 0
					if( srctype == dsttype && srcb == dstb )
					{
						// polyphase perf test
						CImg dstpoly;   
						WIN_ASSERT_EQUAL( S_OK, dstpoly.Create(dstnew.GetImgInfo()) );
	
						CTimer tpoly;
						WIN_ASSERT_EQUAL( S_OK, VtSeparableFilter(src, dstpoly, ks, ks) ); 
						printf("    polyphase filter took %f ms\n", tpoly.GetTimeMilliSec());
	
						fname.format(L"c:\\temp\\seppoly_%d_%d_N.png",i,srcb);
						VtSaveImage(fname, dstpoly);
	
	#if _DEBUG
						vt::CRect r = dstpoly.Rect();
						r.left += k.Width()/2; 
						r.top  += k.Width()/2;
						r.right  -= k.Width()/2;
						r.bottom -= k.Width()/2;

						WIN_ASSERT_EQUAL( true, VtCompareImages(dstpoly, dstnew, 1, NULL, 1.1f/255.f, &r) );   
	#endif
					}
#endif
				}
            }
        }
    }
}
END_TEST_VTR
#endif

#if 0
BEGIN_TEST_VTR_FAST(ResizeTest)
{ 
	std::wstring warpimage = GetImageDir() + L"\\newman.png";

	CRGBAImg input;
	LoadImage(warpimage, input);

	int types[3] = {EL_FORMAT_BYTE, EL_FORMAT_SHORT, EL_FORMAT_FLOAT};

	vt::wstring_b<256> fname;

	//for( int i = 0; i < 1; i++ )
	for( int i = 0; i < 3; i++ )
	{
		int type = types[i];
		//for( int b = 1; b <= 1; b++ )
		for( int b = 1; b <= 4; b++ )
		{
			if (b==2) continue;

			printf("type(%d), bands(%d)\n", type, b);

			CImg src;   
			WIN_ASSERT_EQUAL( S_OK, src.Create(input.Width(), input.Height(), b, 
											   VtElSizeFromType(type), type) );
			WIN_ASSERT_EQUAL( S_OK, VtConvertImage(src, input) );
			//fname.format(L"c:\\temp\\input_%d_%d.png",i,b);
			//VtSaveImage(fname, src);

			CImg dst;   
			WIN_ASSERT_EQUAL( S_OK, dst.Create(
				(26*input.Width())/10, (26*input.Height())/10, b, 
				VtElSizeFromType(type), type) );
			dst.Clear();

			CTimer tr;
			WIN_ASSERT_EQUAL( S_OK, VtResizeImage(dst, dst.Rect(), src, eSamplerKernelBilinear) );
			printf("    resize took %f ms\n", tr.GetTimeMilliSec());

			//fname.format(L"c:\\temp\\resize_%d_%d_N.png",i,b);
			//VtSaveImage(fname, dst);
			dst.Clear();

			CMtx3x3f m;
			m.MakeScale(1.f/2.6f, 1.f/2.6f);
			
			CTimer tw;
			WIN_ASSERT_EQUAL( S_OK, VtWarpImg(dst, dst.Rect(), src, m, eSamplerKernelBilinear) );
			printf("    warp took %f ms\n", tw.GetTimeMilliSec());

			fname.format(L"c:\\temp\\warp_%d_%d_N.png",i,b);
			VtSaveImage(fname, dst);
			if( i==0 )
			{
				//CImg ref;
				//fname.format(L"c:\\temp\\warp_%d_%d.png",i,b);
				//VtLoadImage(fname, ref);
				//WIN_ASSERT_EQUAL( true, VtCompareImages(dst, ref) );   
			}
		}
	}
}
END_TEST_VTR
#endif

END_TEST_FILE