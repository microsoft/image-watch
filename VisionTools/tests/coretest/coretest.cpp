////////////////////////////////////////////////////////////////////////////////
// coretest.cpp : Unit tests for VisionTools core
//

#include "stdafx.h"

#include "cthelpers.h"

using namespace vt;

vt::CRect DivRect(int i, const vt::CRect& rct)
{
	int h3 = rct.Height() / 3;
	int w2 = rct.Width() / 2;

	vt::CRect r;
	switch (i)
	{
	case 0:
		r.left = 0, r.right = rct.Width();
		r.top = 0, r.bottom = h3;
		break;
	case 1:
		r.left = 0, r.right = rct.Width() - w2;
		r.top = h3, r.bottom = rct.Height() - h3;
		break;
	case 2:
		r.left = rct.Width() - w2, r.right = rct.Width();
		r.top = h3, r.bottom = rct.Height() - h3;
		break;
	case 3:
		r.left = 0, r.right = rct.Width();
		r.top = rct.Height() - h3, r.bottom = rct.Height();
		break;
	}
	return r;
}

//+-----------------------------------------------------------------------------
//
//
HRESULT test_callback(void*, LONG, CTaskProgress*)
{
	return S_OK;
}


class CThreadFillTransform : public CImageTransformPoint<CThreadFillTransform, true>
{
	// IImageTransform implementation
public:
	virtual HRESULT Transform(OUT CImg* pimgDstRegion,
		IN  const CRect&,
		IN  CImg *const *,
		IN  UINT /*uSrcCnt*/)
	{
		((CByteImg*)pimgDstRegion)->Fill(m_fill);
		return S_OK;
	}

	virtual HRESULT Clone(ITaskState **ppClone)
	{
		*ppClone = new CThreadFillTransform();
		return (*ppClone) ? S_OK : E_OUTOFMEMORY;
	}

public:
	CThreadFillTransform()
	{
		m_fill = (Byte)InterlockedIncrement(&m_fillstatic);
	}

	static LONG m_fillstatic;

protected:
	Byte m_fill;
};

bool VerifySequencerResult(const CLumaByteImg& dst, int bh)
{
	for (int yy = 0; yy < dst.Height(); yy += bh)
	{
		Byte t = dst(0, yy);
		if (t < 1)
		{
			return false;
		}
		for (int y = yy; y < yy + bh && y < dst.Height(); y++)
		{
			for (int x = 0; x < dst.Width(); x++)
			{
				if (dst(x, y) != t)
				{
					return false;
				}
			}
		}
	}
	return true;
}

LONG CThreadFillTransform::m_fillstatic = 0;

LONG g_callbackcount = 0;
const LONG g_testrecurse = 180;

HRESULT
TaskTest2Callback(void*, LONG, CTaskProgress*)
{
	if (InterlockedIncrement(&g_callbackcount) < g_testrecurse)
	{
		WIN_ASSERT_EQUAL(S_OK, PushTaskAndWait(TaskTest2Callback, NULL, NULL));
	}
	return S_OK;
}

DWORD __stdcall
StartTasks(LPVOID lpThreadParameter)
{
	CRGBImg orig;
	LoadImage(GetImageDir() + L"newman.png", orig);

	CRand rnd;

	for (int i = 0; i < 400; i++)
	{
		int iSrcW = 64 + rnd.IRand(1600);
		int iSrcH = 64 + rnd.IRand(1600);

		CRGBImg src, dst;
		WIN_ASSERT_EQUAL(S_OK, VtResizeImage(src, vt::CRect(0, 0, iSrcW, iSrcH),
			orig));
		dst.Create(iSrcW, iSrcH);

		CImgReaderWriter<CImg> srcrw, dstrw;
		src.Share(srcrw);
		dst.Share(dstrw);

		CRotateTransform x[4];
		x[0].InitializeRotate(iSrcW, iSrcH, 1);
		x[1].InitializeRotate(iSrcH, iSrcW, 1);
		x[2].InitializeRotate(iSrcW, iSrcH, 1);
		x[3].InitializeRotate(iSrcH, iSrcW, 1);

		CTransformGraphUnaryNode g[4];
		g[0].SetTransform(&x[0]);
		g[0].BindSourceToTransform(&g[1]);
		g[1].SetTransform(&x[1]);
		g[1].BindSourceToTransform(&g[2]);
		g[2].SetTransform(&x[2]);
		g[2].BindSourceToTransform(&g[3]);
		g[3].SetTransform(&x[3]);
		g[3].BindSourceToReader(&srcrw);

		g[0].SetDest(&dstrw);

		WIN_ASSERT_EQUAL(S_OK, PushTransformTaskAndWait(g));

		WIN_ASSERT_TRUE(VtCompareImages(srcrw, dst));
	}

	SetEvent(*(HANDLE*)lpThreadParameter);

	return 0;
}

struct AutoImageDebugger
{
	AutoImageDebugger()
	{
#ifndef VT_WINRT
		VtImageDebuggerStart();
#endif
	}
};

AutoImageDebugger g_imageDebugger;

////////////////////////////////////////////////////////////////////////////////
// Tests the various image Create calls
//
template<class T>
void TestCreate(T& img, bool pass, int type)
{
	Byte* p = (Byte*)0x100;
	if (pass)
	{
		WIN_ASSERT_TRUE(((CImg&)img).Create(320, 256, type) == S_OK);
		WIN_ASSERT_TRUE(((CImg&)img).Create(p, 320, 256, 320 * 16, type) == S_OK);
	}
	else
	{
		WIN_ASSERT_TRUE(((CImg&)img).Create(320, 256, type) != S_OK);
		WIN_ASSERT_TRUE(((CImg&)img).Create(p, 320, 256, 320 * 16, type) != S_OK);
	}
}


BEGIN_TEST_FILE(CoreTest)

BEGIN_TEST_EX_FAST(ImgCreateTest)
{
	// test that create works as expected for the various image types
	CLumaByteImg  imglb;
	TestCreate(imglb, true,  OBJ_LUMAIMG);
	TestCreate(imglb, false, OBJ_LUMASHORTIMG);
	TestCreate(imglb, false, OBJ_LUMAFLOATIMG);
	TestCreate(imglb, false, OBJ_RGBIMG);
	TestCreate(imglb, true,  OBJ_BYTEIMG);

	CLumaShortImg imgls;
	TestCreate(imgls, false, OBJ_BYTEIMG);
	TestCreate(imgls, true,  OBJ_LUMASHORTIMG);
	TestCreate(imgls, false, OBJ_LUMAFLOATIMG);
	TestCreate(imgls, false, OBJ_BYTEIMG);
	TestCreate(imgls, true,  OBJ_SHORTIMG);

	CLumaFloatImg imglf;
	TestCreate(imglf, false, OBJ_BYTEIMG);
	TestCreate(imglf, false, OBJ_LUMASHORTIMG);
	TestCreate(imglf, true,  OBJ_LUMAFLOATIMG);
	TestCreate(imglf, false, OBJ_BYTEIMG);
	TestCreate(imglf, true,  OBJ_FLOATIMG);

	CRGBByteImg  imgrgbb;
	TestCreate(imgrgbb, true,  OBJ_RGBIMG);
	TestCreate(imgrgbb, false, OBJ_RGBSHORTIMG);
	TestCreate(imgrgbb, false, OBJ_RGBFLOATIMG);
	TestCreate(imgrgbb, false, OBJ_LUMAIMG);
	TestCreate(imgrgbb, true,  VT_IMG_MAKE_TYPE(EL_FORMAT_BYTE, 3));

	CRGBShortImg imgrgbs;
	TestCreate(imgrgbs, false, OBJ_RGBIMG);
	TestCreate(imgrgbs, true,  OBJ_RGBSHORTIMG);
	TestCreate(imgrgbs, false, OBJ_RGBFLOATIMG);
	TestCreate(imgrgbs, false, OBJ_BYTEIMG);
	TestCreate(imgrgbs, true,  VT_IMG_MAKE_TYPE(EL_FORMAT_SHORT, 3));

	CRGBFloatImg imgrgbf;
	TestCreate(imgrgbf, false, OBJ_RGBIMG);
	TestCreate(imgrgbf, false, OBJ_RGBSHORTIMG);
	TestCreate(imgrgbf, true,  OBJ_RGBFLOATIMG);
	TestCreate(imgrgbf, false, OBJ_BYTEIMG);
	TestCreate(imgrgbf, true,  VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, 3));

	CImg img;
	TestCreate(img, true, OBJ_LUMAIMG);
	TestCreate(img, true, OBJ_LUMASHORTIMG);
	TestCreate(img, true, OBJ_LUMAFLOATIMG);
	TestCreate(img, true, OBJ_RGBIMG);
	TestCreate(img, true, OBJ_RGBSHORTIMG);
	TestCreate(img, true, OBJ_RGBFLOATIMG);
	TestCreate(img, true,  OBJ_BYTEIMG);
	TestCreate(img, true,  OBJ_SHORTIMG);
	TestCreate(img, true,  OBJ_FLOATIMG);

	CByteImg imgb;
	TestCreate(imgb, true, OBJ_LUMAIMG);
	TestCreate(imgb, false, OBJ_LUMASHORTIMG);
	TestCreate(imgb, false, OBJ_LUMAFLOATIMG);
	TestCreate(imgb, true, OBJ_RGBIMG);
	TestCreate(imgb, false, OBJ_RGBSHORTIMG);
	TestCreate(imgb, false, OBJ_RGBFLOATIMG);
	TestCreate(imgb, true,  OBJ_BYTEIMG);
	TestCreate(imgb, false,  OBJ_SHORTIMG);
	TestCreate(imgb, false,  OBJ_FLOATIMG);

	CShortImg imgs;
	TestCreate(imgs, false, OBJ_LUMAIMG);
	TestCreate(imgs, true, OBJ_LUMASHORTIMG);
	TestCreate(imgs, false, OBJ_LUMAFLOATIMG);
	TestCreate(imgs, false, OBJ_RGBIMG);
	TestCreate(imgs, true, OBJ_RGBSHORTIMG);
	TestCreate(imgs, false, OBJ_RGBFLOATIMG);
	TestCreate(imgs, false,  OBJ_BYTEIMG);
	TestCreate(imgs, true,  OBJ_SHORTIMG);
	TestCreate(imgs, false,  OBJ_FLOATIMG);

	CFloatImg imgf;
	TestCreate(imgf, false, OBJ_LUMAIMG);
	TestCreate(imgf, false, OBJ_LUMASHORTIMG);
	TestCreate(imgf, true, OBJ_LUMAFLOATIMG);
	TestCreate(imgf, false, OBJ_RGBIMG);
	TestCreate(imgf, false, OBJ_RGBSHORTIMG);
	TestCreate(imgf, true, OBJ_RGBFLOATIMG);
	TestCreate(imgf, false, OBJ_BYTEIMG);
	TestCreate(imgf, false, OBJ_SHORTIMG);
	TestCreate(imgf, true,  OBJ_FLOATIMG);
}
END_TEST_EX

////////////////////////////////////////////////////////////////////////////////
// Tests if image is invariant to saving/reloading
BEGIN_TEST_EX_FAST(LoadSaveFormats)
{
    const double MAX_LOSSY_DIFF = 15.0;

    const wchar_t* images[] = { L"burns.jpg", 
        L"lena512color.tiff",
        L"newman.png" };

    struct FileFormats
    {
        std::wstring extension;
        bool lossy;
    };

    const FileFormats formats[] = { { L"bmp", false }, { L"png", false }, 
    { L"tif", false }, { L"jpg", true }, { L"wdp", true }, { L"vti", false } };

    for (size_t i = 0; i < _countof(images); ++i)
    {
        CImg img;
        LoadImage(GetImageDir() + images[i], img);

        for (size_t j = 0; j < _countof(formats); ++j)
        {
            // write file to working directory
            std::wstring filename = images[i];
            const std::wstring tmp_path = filename.substr(0, 
                filename.rfind('.') + 1) + formats[j].extension;

            CImg img2;
            if (formats[j].extension == L"vti")
            {
                WIN_ASSERT_EQUAL(S_OK, img.Save(tmp_path.c_str()));
                WIN_ASSERT_EQUAL(S_OK, img2.Load(tmp_path.c_str()));
            }
            else
            {
                SaveImage(tmp_path, img);
                LoadImage(tmp_path, img2);
            }

            const double dist = GetAverageAbsoluteDifference(img, img2);

            WIN_ASSERT_TRUE(formats[j].lossy ? (dist < MAX_LOSSY_DIFF) : 
                (dist == 0.0));

            if (PathFileExists(tmp_path.c_str()))
                DeleteFile(tmp_path.c_str());
        }
    }
}
END_TEST_EX

////////////////////////////////////////////////////////////////////////////////
// Tests load/save interfaces
BEGIN_TEST_EX_FAST(LoadSaveQuality)
{
    const wchar_t* image_file = L"newman.png";

    const wchar_t* quality_tag = L"ImageQuality";
    struct Quality
    {
        std::wstring extension;
        float quality_level;
        float min_dist;
        float max_dist;
    };

    Quality qlist[] = { { L"jpg", 0.5f, 5.0f, 10.0f }, 
    { L"jpg", 0.95f, 0.0f, 1.5f }, { L"wdp", 0.95f, 1.0f, 3.0f },
    { L"wdp", 1.0f, 0.0f, 0.0f } };

    for (size_t i = 0; i < _countof(qlist); ++i)
    {
        CImg img;
        WIN_ASSERT_EQUAL(S_OK, VtLoadImage((GetImageDir() + image_file).c_str(), 
            img));

        // write file to working directory
        std::wstring filename = image_file;
        const std::wstring tmp_path = filename.substr(0, 
            filename.rfind('.') + 1) + qlist[i].extension;

        CParams params;
        CParamValue v;	
        v.Set(qlist[i].quality_level);
        params.SetByName(quality_tag, 0, v);
        WIN_ASSERT_EQUAL(S_OK, VtSaveImage(tmp_path.c_str(), img, true, &params));

        CImg img2;
        WIN_ASSERT_EQUAL(S_OK, VtLoadImage(tmp_path.c_str(), img2));

        const double dist = GetAverageAbsoluteDifference(img, img2);
        WIN_ASSERT_TRUE(dist <= qlist[i].max_dist && dist >= qlist[i].min_dist);

        if (PathFileExists(tmp_path.c_str()))
            DeleteFile(tmp_path.c_str());		
    }
}
END_TEST_EX


////////////////////////////////////////////////////////////////////////////////
// Tests convert to gray
BEGIN_TEST_EX_FAST(ConvertToGray)
{
    const wchar_t* images[] = { L"newman.png" };

    for (size_t i = 0; i < _countof(images); ++i)
    {
        CImg input;
        LoadImage(GetImageDir() + images[i], input);

        CImg output;
        WIN_ASSERT_EQUAL(S_OK, output.Create(input.Width(), input.Height(), 
			                                 OBJ_LUMAIMG));
        WIN_ASSERT_EQUAL(S_OK, VtConvertImage(output, input));

        CLumaImg expected;
        LoadImage(GetImageDir() + GetFileName(images[i]) + L"_gray" + 
            GetFileExtension(images[i]), expected);

        const double dist = GetAverageAbsoluteDifference(expected, output);

        WIN_ASSERT_TRUE(dist < 1e-3);
    }
}
END_TEST_EX

////////////////////////////////////////////////////////////////////////////////
// Tests resizing
BEGIN_TEST_EX_FAST(Resize)
{
    struct ResizeSettings
    {
        std::wstring name;
        double factor;
        eSamplerKernel ftype;
    };

    ResizeSettings settings[] = { 
        { L"small", 0.1, eSamplerKernelLanczos3Precise },
        { L"large", 3.0, eSamplerKernelLanczos3Precise },
    };

    const wchar_t* images[] = { L"newman.png" };

    for (size_t i = 0; i < _countof(images); ++i)
    {
        CImg input;
        LoadImage(GetImageDir() + images[i], input);

        // todo: remove this once VtResizeImage() is template-free
        _ASSERT(EL_FORMAT(input.GetType()) == EL_FORMAT_BYTE);

        for (size_t s = 0; s < _countof(settings); ++s)
        {
            CImg output;
            const int outw = static_cast<int>(input.Width() * 
                settings[s].factor + 0.5);
            const int outh = static_cast<int>(input.Height() * 
                settings[s].factor + 0.5);

            WIN_ASSERT_EQUAL(S_OK, output.Create(outw, outh, input.GetType()));

            WIN_ASSERT_EQUAL(S_OK, VtResizeImage(output, output.Rect(), input, 
                settings[s].ftype, IMAGE_EXTEND(Extend)));

            CImg expected;
            LoadImage(GetImageDir() + GetFileName(images[i]) + L"_" + 
                settings[s].name + GetFileExtension(images[i]), expected);

            const double dist = GetAverageAbsoluteDifference(expected, output);

            WIN_ASSERT_TRUE(dist < 1.);
        }
    }
}
END_TEST_EX

////////////////////////////////////////////////////////////////////////////////
// Tests return of alpha channel for pngs with and without transparent palette values
BEGIN_TEST_EX_FAST(LoadIndexedAlpha)
{
    CImg img0,img1;
    LoadImage(GetImageDir() + L"8bit_transparent.png", img0);
    WIN_ASSERT_TRUE( img0.GetType() == OBJ_RGBAIMG );
    LoadImage(GetImageDir() + L"8bit_nontransparent.png", img1);
    WIN_ASSERT_TRUE( img1.GetType() == OBJ_RGBIMG );
}
END_TEST_EX

////////////////////////////////////////////////////////////////////////////////
// Tests loading/saving metadata
BEGIN_TEST_EX_FAST(LoadSaveMetadata)
{
    const std::wstring filename = L"burns.jpg";	
    std::wstring formats[] = { L"jpg", L"wdp" };

    for (size_t i = 0; i < _countof(formats); ++i)
    {		
        CImg inimg;
        LoadImage(GetImageDir() + filename, inimg);

        CParamValue invalue;
        char* intext = "___MAGIC_VALUE___";
        invalue.Set(intext);
        CParams p;
        p.SetById(ImagePropertyDocumentName, 0, invalue);
        inimg.SetMetaData(&p);

        // write file to working directory
        const std::wstring tmpname = filename.substr(0, 
            filename.rfind('.') + 1) + formats[i];

        SaveImage(tmpname, inimg);

        CImg outimg;
        LoadImage(tmpname, outimg);

        CParamValue* outvalue;
        outimg.GetMetaData()->GetById(&outvalue, ImagePropertyDocumentName);

        WIN_ASSERT_TRUE(outvalue != NULL);

        char* outtext = NULL;
        outvalue->Get(&outtext);

        WIN_ASSERT_TRUE(std::string(intext) == std::string(outtext));

        if (PathFileExists(tmpname.c_str()))
            DeleteFile(tmpname.c_str());
    }
}
END_TEST_EX

////////////////////////////////////////////////////////////////////////////////
// 
// Tests pad operation
//
class CSimpleCopyTransform: public CImageTransformUnaryPoint<CSimpleCopyTransform, false>
{
public:
    void GetSrcPixFormat(IN OUT int* pfrmtSrcs, 
                         IN UINT,
                         IN int /*frmtDst*/)
    { pfrmtSrcs[0] = VT_IMG_FIXED(OBJ_RGBAIMG); }
    void GetDstPixFormat(OUT int& frmtDst,
                         IN  const int*, 
                         IN  UINT)
    { frmtDst = VT_IMG_FIXED(OBJ_RGBAIMG); }
    HRESULT Transform(CImg* pDst, IN const CRect&, const CImg& src)
    {  return pDst? src.CopyTo(*pDst): S_OK; }
    virtual HRESULT Clone(ITaskState **ppState)
    { return CloneTaskState(ppState, VT_NOTHROWNEW CSimpleCopyTransform()); }
};

BEGIN_TEST_VTR_FAST(PadTest)
{
    CImgReaderWriter<CByteImg> src;
    WIN_ASSERT_EQUAL( S_OK, src.Create(3, 3, 3, 3, 3, vt::CPoint(0,0)) );
    for( int y = 0; y < 3; y++ )
    {
        for( int x = 0; x < 3; x++ )
        {
            for( int b = 0; b < 3; b++ )
            {
                src.Pix(x,y,b) = Byte(b*9+y*3+x+1);
            }
        }
    }

    CImgReaderWriter<CByteImg> dst;
    WIN_ASSERT_EQUAL( S_OK, dst.Create(7,7,3,3,3,vt::CPoint(-2,-2)) );

    CConvertTransform t(VT_IMG_FIXED(OBJ_RGBIMG));
    CTransformGraphUnaryNode n(&t);
    n.SetDest(&dst);

    // test zero pad mode
    IMAGE_EXTEND exZero(Zero);
    n.BindSourceToReader(&src, &exZero);
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&n) );
    for( int y = 0; y < 7; y++ )
    {
        for( int x = 0; x < 7; x++ )
        {
            Byte expected = Byte((y>=2 && y<5 && x>=2 && x<5)?
                ((y-2)*3+(x-2)+1): 0);
            WIN_ASSERT_EQUAL( expected, dst.Pix(x,y) );   
        }
    }

    // test Extend pad mode
    IMAGE_EXTEND exExtend(ExtendZeroAlpha);
    n.BindSourceToReader(&src, &exExtend);
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&n) );
    for( int y = 0; y < 7; y++ )
    {
        for( int x = 0; x < 7; x++ )
        {
            int testy = VtMin(2, VtMax(0, y-2));
            int testx = VtMin(2, VtMax(0, x-2));
            Byte expected = Byte(testy*3+testx+1);
            WIN_ASSERT_EQUAL( expected, dst.Pix(x,y) );   
        }
    }

    // test const pad mode
    IMAGE_EXTEND exConst;
    // force convert to byte and fill to 3 bands
    UInt16 v127 = 0x7f00;
    exConst.Initialize(Constant, Constant, v127, v127);
    n.BindSourceToReader(&src, &exConst);
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&n) );
    for( int y = 0; y < 7; y++ )
    {
        for( int x = 0; x < 7; x++ )
        {
            for( int b = 0; b < 3; b++ )
            {
                Byte expected = Byte((y>=2 && y<5 && x>=2 && x<5)?
                    (b*9+(y-2)*3+(x-2)+1): 127);
                WIN_ASSERT_EQUAL( expected, dst.Pix(x,y,b) );   
            }
        }
    }

    // test typemax pad mode
    IMAGE_EXTEND exConst1;
    exConst1.Initialize(TypeMax, TypeMax);
    n.BindSourceToReader(&src, &exConst1);
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&n) );
    for( int y = 0; y < 7; y++ )
    {
        for( int x = 0; x < 7; x++ )
        {
            Byte expected = Byte((y>=2 && y<5 && x>=2 && x<5)?
                ((y-2)*3+(x-2)+1): 255);
            WIN_ASSERT_EQUAL( expected, dst.Pix(x,y) );   
        }
    }

    // test typemin pad mode
    IMAGE_EXTEND exConst2;
    exConst2.Initialize(TypeMin, TypeMin);
    n.BindSourceToReader(&src, &exConst2);
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&n) );
    for( int y = 0; y < 7; y++ )
    {
        for( int x = 0; x < 7; x++ )
        {
            Byte expected = Byte((y>=2 && y<5 && x>=2 && x<5)?
                ((y-2)*3+(x-2)+1): 0);
            WIN_ASSERT_EQUAL( expected, dst.Pix(x,y) );   
        }
    }

    // test wrapX + Zero
    IMAGE_EXTEND exWxZ(Wrap, Zero);
    n.BindSourceToReader(&src, &exWxZ);
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&n) );
    for( int y = 0; y < 7; y++ )
    {
        for( int x = 0; x < 7; x++ )
        {
            int testx = x-2;
            testx = (testx<0)? testx+3: (testx>2)? testx-3: testx;
            Byte expected = Byte((y<2 || y>4)? 0: ((y-2)*3+testx+1));
            WIN_ASSERT_EQUAL( expected, dst.Pix(x,y) );   
        }
    }

    // test wrapY + Extend
    IMAGE_EXTEND exExWy(ExtendZeroAlpha, Wrap);
    n.BindSourceToReader(&src, &exExWy);
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&n) );
    for( int y = 0; y < 7; y++ )
    {
        for( int x = 0; x < 7; x++ )
        {
            int testy = y-2;
            testy = (testy<0)? testy+3: (testy>2)? testy-3: testy;   
            int testx = VtMin(2, VtMax(0, x-2));
            Byte expected = Byte(testy*3+testx+1);
            WIN_ASSERT_EQUAL( expected, dst.Pix(x,y) );   
        }
    }

    // test wrapX + WrapY
    // and try a transform block size of 2x2
    IMAGE_EXTEND exWxWy(Wrap, Wrap);
    n.BindSourceToReader(&src, &exWxWy);
	CRasterBlockMap bm(BLOCKITER_INIT(dst.GetImgInfo().LayerRect(), 2, 2), 4, 4);
	n.SetDest(NODE_DEST_PARAMS(&dst, &bm));

    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&n, (CTaskStatus*)NULL) );
    for( int y = 0; y < 7; y++ )
    {
        for( int x = 0; x < 7; x++ )
        {
            int testx = x-2;
            testx = (testx<0)? testx+3: (testx>2)? testx-3: testx;
            int testy = y-2;
            testy = (testy<0)? testy+3: (testy>2)? testy-3: testy;   
            Byte expected = Byte(testy*3+testx+1);
            WIN_ASSERT_EQUAL( expected, dst.Pix(x,y) );   
        }
    }

    // test extend with alpha + without alpha
    CImgReaderWriter<CRGBAImg> dstColor;
    WIN_ASSERT_EQUAL( S_OK, dstColor.Create(7,7,3,3,vt::CPoint(-2,-2)) );

    CSimpleCopyTransform xcolor;
    CTransformGraphUnaryNode ncolor(&xcolor);
    ncolor.SetDest(&dstColor);
    for( int k = 0; k < 2; k++ )
    {
        IMAGE_EXTEND exEx((k==0)? Extend: ExtendZeroAlpha);
        ncolor.BindSourceToReader(&src, &exEx);
        WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&ncolor) );
        for( int y = 0; y < 7; y++ )
        {
            for( int x = 0; x < 7; x++ )
            {
                int testy = VtMin(2, VtMax(0, y-2));
                int testx = VtMin(2, VtMax(0, x-2));
                Byte expectedR = Byte(9+9+testy*3+testx+1);
                Byte expectedG = Byte(9+testy*3+testx+1);
                Byte expectedB = Byte(testy*3+testx+1);
                Byte expectedA = (k==0)? 255: ( x>=2 && x<=4 && y>=2 && y<=4)? 255: 0;
                const RGBAPix* pOut = dstColor.Ptr(x, y);
                WIN_ASSERT_EQUAL( true, pOut->r==expectedR && 
                    pOut->g==expectedG && 
                    pOut->b==expectedB &&
                    pOut->a==expectedA );
            }
        }
    }
}
END_TEST_VTR

////////////////////////////////////////////////////////////////////////////////
//
// Some simple tests of the transform framework
//
class CBWTransform: public IImageTransform
{
public:
	CBWTransform(int i): m_canover(i)
	{}

public:
	virtual bool RequiresCloneForConcurrency()
	{ return false;	}

	// by default we use the pixel format of the dest
	virtual void    GetSrcPixFormat(IN OUT int* pfrmtSrcs, 
									IN UINT  /*uSrcCnt*/,
									IN int frmtDst)
	{ pfrmtSrcs[0] = frmtDst; }

	virtual void    GetDstPixFormat(OUT int& frmtDst,
									IN  const int* pfrmtSrcs, 
									IN  UINT  /*uSrcCnt*/)
	{ frmtDst = pfrmtSrcs[0]; }

	virtual HRESULT GetRequiredSrcRect(OUT TRANSFORM_SOURCE_DESC* pSrcReq,
									   OUT UINT& uSrcReqCount,
									   IN  UINT,
									   IN  const CRect& rctDst
									  )
	{
		pSrcReq[0].bCanOverWrite = m_canover? true: false;
		pSrcReq[0].uSrcIndex     = 0;
		pSrcReq[0].rctSrc        = rctDst;
		uSrcReqCount             = 1;

		int test = (rctDst.top/256) & 1;
		if( ((rctDst.left/256) & 1)==test )
		{
			if( test==0 )
			{
				uSrcReqCount = 0;
			}
			else
			{
				pSrcReq[0].rctSrc = CRect(0,0,0,0);
			}
		}

		return S_OK;
	}

	virtual HRESULT GetAffectedDstRect(OUT CRect& rctDst,
									  IN  const CRect& rctSrc,
									  IN  UINT /*uSrcIndex*/,
									  IN  UINT /*uSrcCnt*/)
	{
		rctDst = rctSrc;
		return S_OK;
	}

	virtual HRESULT GetResultingDstRect(OUT CRect& rctDst,
	                                    IN  const CRect& rctSrc,
	                                    IN  UINT /*uSrcIndex*/,
										IN  UINT /*uSrcCnt*/)
	{		
		rctDst = rctSrc;
		return S_OK;
	}

	virtual HRESULT Transform(OUT CImg* pimgDst, 
							  IN  const CRect& /*rctDst*/,
							  IN  CImg *const *ppimgSrcRegions,
							  IN  const TRANSFORM_SOURCE_DESC* pSrcDesc,
							  IN  UINT  uSrcCnt
							  )
	{ 
		HRESULT hr = S_OK;
		if( uSrcCnt == 0 || pSrcDesc[0].rctSrc.IsRectEmpty() )
		{
			pimgDst->Clear();
		}
		else
		{
			hr = VtConvertImage(*pimgDst, *ppimgSrcRegions[0]);
		}
		return hr;
	}		

	virtual HRESULT Clone(ITaskState **ppState)
	{ return CloneTaskState(ppState, VT_NOTHROWNEW CBWTransform(m_canover)); }

protected:
	int m_canover;
};

BEGIN_TEST_VTR_FAST(TransformTest)
{
	RGBAPix v(255,255,255);
	CRGBAImgInCache source, dest;
	WIN_ASSERT_EQUAL(S_OK, source.Create(1024, 1200));
	WIN_ASSERT_EQUAL(S_OK, dest.Create(720, 512));
	WIN_ASSERT_EQUAL(S_OK, source.Fill(v));

	for( int i = 0; i < 2; i++ )
	{
		CBWTransform t(i);
		CTransformGraphUnaryNode g(&t, &source, &dest);
	
		VT_TRANSFORM_TASK_OPTIONS opts;
		opts.maxthreads = 1;
		WIN_ASSERT_EQUAL(S_OK, PushTransformTaskAndWait(&g, (CTaskStatus*)NULL, &opts));
	
		CRGBAImg verify;
		WIN_ASSERT_EQUAL(S_OK, dest.ReadImg(verify));
		for( int y = 0; y < verify.Height(); y++ )
		{		
			int test = (y/256) & 1;
	
			for( int x = 0; x < verify.Width(); x++ )
			{
				RGBAPix pix = verify(x,y);
				if( ((x/256) & 1)==test )
				{
					WIN_ASSERT_TRUE( pix.r==0 && pix.g==0 && pix.b==0 && pix.a==0 );
				}
				else
				{
					WIN_ASSERT_TRUE( pix.r==255 && pix.g==255 && pix.b==255 && pix.a==255);
				}
			}  
		} 
	}
}
END_TEST_VTR

class CIncTransform: public CImageTransformUnaryPoint<CIncTransform, true>
{
public:
	virtual HRESULT Transform(OUT CImg *pimgDst, const CRect &rctDst, CImg& imgSrc)
	{
		if( pimgDst->GetImgInfo().type != OBJ_LUMAIMG ||
			imgSrc.GetImgInfo().type   != OBJ_LUMAIMG ||
			CRect(pimgDst->Rect())     != vt::CRect(0,0,rctDst.Width(),rctDst.Height()) ||
			CRect(pimgDst->Rect())     != CRect(imgSrc.Rect()) )
		{
			return E_UNEXPECTED;
		}

		CByteImg& dst = (CByteImg&)*pimgDst;
		CByteImg& src = (CByteImg&)imgSrc;
		for( int y = 0; y < dst.Height(); y++ )
		{
			for( int x = 0; x < dst.Width(); x++ )
			{
				dst.Pix(x,y) = src.Pix(x,y)+1;
			}
		}
		return S_OK;
	}

	virtual HRESULT Clone(ITaskState** ppClone)
	{
		*ppClone = new CIncTransform();
		return (*ppClone)? S_OK: E_OUTOFMEMORY; 
	}
};

class CAddLRTransform: public CImageTransformPoint<CAddLRTransform, false>
{
public:
	virtual HRESULT Transform(OUT CImg* pimgDst, 
							  IN  const CRect& rctDst,
							  IN  CImg *const *ppimgSrcRegions,
							  IN  UINT  uSrcCnt)
	{
		if( uSrcCnt != 2 || 
			pimgDst->GetImgInfo().type            != OBJ_LUMAIMG ||
		    ppimgSrcRegions[0]->GetImgInfo().type != OBJ_LUMAIMG ||
		    ppimgSrcRegions[1]->GetImgInfo().type != OBJ_LUMAIMG ||
			CRect(pimgDst->Rect()) != vt::CRect(0,0,rctDst.Width(),rctDst.Height()) )
		{
			return E_UNEXPECTED;
		}

		CByteImg& dst = (CByteImg&)*pimgDst;
		CByteImg& src0 = (CByteImg&)(*ppimgSrcRegions[0]);
		CByteImg& src1 = (CByteImg&)(*ppimgSrcRegions[1]);
		for( int y = 0; y < dst.Height(); y++ )
		{
			for( int x = 0; x < dst.Width(); x++ )
			{
				dst.Pix(x,y) = src0.Pix(x,y) + src1.Pix(x,y);
			}
		}
		return S_OK;
	}

	virtual HRESULT Clone(ITaskState** ppClone)
	{
		*ppClone = new CAddLRTransform();
		return (*ppClone)? S_OK: E_OUTOFMEMORY; 
	}
};

bool VerifyTransform2Result(const CLumaByteImg& dst, const CLumaByteImg& src, 
							int m, int a, int cntr=7)
{
	int off = (7-cntr)/2;

	for( CBlockIterator bi(dst.Rect(),7); !bi.Done(); bi.Advance()  )
	{
		vt::CRect r = bi.GetRect();
		for( int y = r.top; y < r.bottom; y++ )
		{
			for( int x = r.left; x < r.right; x++ )
			{
				if( y < r.top+off  || y >= r.top+7-off ||
					x < r.left+off || x >= r.left+7-off )
				{
					if( dst.Pix(x,y) != 0 )
						return false;
				}
				else
				{
					if( dst.Pix(x,y) != src.Pix(x,y)*m+a )
						return false;
				}
			}
		}
	}
	return true;
}

class CTest2BlockMap: public ITransformBlockMap
{
public:
	virtual int GetTotalBlocks()
	{
		int x = (m_r.Width() + 6)  / 7;
		int y = (m_r.Height() + 6) / 7;
		return x*y;
	}

	virtual HRESULT GetBlockItems(vt::CRect& rct, int& iPrefetchCount, 
								  int iBlockIndex)
	{
		int x = (m_r.Width() + 6)  / 7;

		int y = iBlockIndex / x;
		x = iBlockIndex - y*x;

		rct = (m_cntr + vt::CPoint(x*7,y*7)) & m_rc;
		iPrefetchCount  = 0;
		return S_OK;
	}

	virtual CRect   GetPrefetchRect(int , int )
	{ return vt::CRect(0,0,0,0); }

public:
	void Initialize(const vt::CRect& r, int cntr = 7)
	{ 
		m_r = r;
		m_rc = r;
		m_cntr.left = m_cntr.top = (7-cntr)/2;
	    m_cntr.right  = m_cntr.left + cntr; 
	    m_cntr.bottom = m_cntr.top  + cntr; 
	}

	void Initialize(const vt::CRect& r, const vt::CRect& rc, int cntr = 7)
	{ 
		m_r = r;
		m_rc = rc;
		m_cntr.left = m_cntr.top = (7-cntr)/2;
	    m_cntr.right  = m_cntr.left + cntr; 
	    m_cntr.bottom = m_cntr.top  + cntr; 
	}

protected:
	vt::CRect m_r, m_rc, m_cntr;
};

class CTransform2Reader: public CImgReaderWriter<CLumaByteImg>
{
public:
	virtual HRESULT Prefetch(const vt::CRect & r, UINT)
	{
		CLumaByteImg ips;
		m_imgPre.Share(ips, &r);
		CopyTo(ips, &r);
		return S_OK;
	}

public:
	void InitializePre(int w, int h)
	{ 
		m_imgPre.Create(w, h);
		m_imgPre.Clear();
	}

	const CLumaByteImg& ImgPre()
	{ return m_imgPre; }

protected:
	CLumaByteImg m_imgPre;
};

BEGIN_TEST_VTR_FAST(TransformTest2)
{
	// build the following graph
	//     
	//      g0->i1   (s+2+s+3+s+4)
	//     / \
	//    g1 g2->i2  (s+2+s+3) | (s+4)
	//    |\/ 
    //    |g3  (s+3)
	//    \|
	//     g4->i3  (s+2)
	//     |
    //     g5  (s+1)
	//     |
	//     s
 
    CTransform2Reader imgSrc;
	CImgReaderWriter<CLumaByteImg> imgDst;
	LoadImage(GetImageDir() + L"newman.png", imgSrc);
	WIN_ASSERT_EQUAL(S_OK, imgDst.Create(imgSrc.Width(), imgSrc.Height(), 
		                                 imgSrc.Width(), imgSrc.Height(),
										 vt::CPoint(0,0)));
	
	// divide intensity by 4 to accomodate math done in transforms
	for( int y = 0; y < imgSrc.Height(); y++ )
	{
		for( int x = 0; x < imgSrc.Width(); x++ )
		{
			imgSrc.Pix(x,y) >>= 2;
		}
	}

	// build the graph and run
	CIncTransform   xi;
	CAddLRTransform xlr;
	CTransformGraphNaryNode g0(&xlr), g1(&xlr);
	CTransformGraphUnaryNode g2(&xi), g3(&xi), g4(&xi), g5(&xi);
 
	g0.SetDest(&imgDst);
	g0.SetSourceCount(2);
	g0.BindSourceToTransform(0, &g1);
	g0.BindSourceToTransform(1, &g2);

	g1.SetSourceCount(2);
	g1.BindSourceToTransform(0, &g4);
	g1.BindSourceToTransform(1, &g3);

	g2.BindSourceToTransform(&g3);

	g3.BindSourceToTransform(&g4);

	g4.BindSourceToTransform(&g5);

	g5.BindSourceToReader(&imgSrc); 

	// execute with the default block iteration
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&g0) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst, imgSrc, 3, 9) );

	// run with a specific block iterator
	CRasterBlockMap bmdef(imgDst.Rect(), 24, 16);
	g0.SetDest(NODE_DEST_PARAMS(&imgDst, &bmdef));

	imgSrc.InitializePre(imgSrc.Width(), imgSrc.Height());
	imgSrc.Prefetch(vt::CRect(0,0,4*24,4*16), 0);

    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&g0) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst, imgSrc, 3, 9) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgSrc.ImgPre(), imgSrc, 1, 0) );

    // run again with intermediate writers
	CImgReaderWriter<CLumaByteImg> imgDst2, imgDst3;
	WIN_ASSERT_EQUAL(S_OK, imgDst2.Create(imgSrc.Width(), imgSrc.Height(), 
										 imgSrc.Width(), imgSrc.Height(),
										 vt::CPoint(0,0)));
	WIN_ASSERT_EQUAL(S_OK, imgDst3.Create(imgSrc.Width(), imgSrc.Height(), 
										 imgSrc.Width(), imgSrc.Height(),
										 vt::CPoint(0,0)));

	g0.SetDest(&imgDst);
	g2.SetDest(&imgDst2);
	g4.SetDest(&imgDst3);

	imgDst.Clear();
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&g0) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst, imgSrc, 3, 9) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst2, imgSrc, 1, 4) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst3, imgSrc, 1, 2) );

	// use custom block maps
	CTest2BlockMap bm1, bm2, bm3;
	bm1.Initialize(imgDst.Rect());
	bm2.Initialize(imgDst.Rect(),5);
	bm3.Initialize(imgDst.Rect(),3);

	g0.SetDest(NODE_DEST_PARAMS(&imgDst, &bm1));
	g2.SetDest(NODE_DEST_PARAMS(&imgDst2, &bm2));
	g4.SetDest(NODE_DEST_PARAMS(&imgDst3, &bm3));

	imgDst.Clear();
	imgDst2.Clear();
	imgDst3.Clear();
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&g0) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst, imgSrc, 3, 9) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst2, imgSrc, 1, 4, 5) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst3, imgSrc, 1, 2, 3) );

	// try head node not requesting as much as remainder of nodes
	// which means that the traversal will start at internal nodes
	// when head node is requesting 0 sized rects
	vt::CRect rctC(0,0,imgDst.Width()>>1,imgDst.Height()>>1);
	bm1.Initialize(imgDst.Rect(), rctC, 7);

	CImgReaderWriter<CLumaByteImg> imgDst4;
	WIN_ASSERT_EQUAL(S_OK, imgDst4.Create(rctC.Width(), rctC.Height(),
										  rctC.Width(), rctC.Height(),
										  vt::CPoint(0,0)));

	g0.SetDest(NODE_DEST_PARAMS(&imgDst4, &bm1));

	CLumaByteImg imgShare;
	imgSrc.Share(imgShare,&rctC);

	imgDst2.Clear();
	imgDst3.Clear();
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&g0) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst4, imgShare, 3, 9) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst2, imgSrc, 1, 4, 5) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst3, imgSrc, 1, 2, 3) );

    // same test as before but swap the writers
    g2.SetDest(NODE_DEST_PARAMS(&imgDst3, &bm3));
    g4.SetDest(NODE_DEST_PARAMS(&imgDst2, &bm2));

	imgDst4.Clear();
	imgDst2.Clear();
	imgDst3.Clear();
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&g0) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst4, imgShare, 3, 9) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst2, imgSrc, 1, 2, 5) );
	WIN_ASSERT_TRUE( VerifyTransform2Result(imgDst3, imgSrc, 1, 4, 3) );
}
END_TEST_VTR


class CCopy4Transform: public IImageTransform
{
public:
	virtual bool RequiresCloneForConcurrency()
	{ return false;	}

	virtual void    GetSrcPixFormat(IN OUT int* ptypeSrcs, 
									IN UINT  uSrcCnt,
									IN int /*typeDst*/)
	{ 
		for( UINT i = 0; i < uSrcCnt; i++ )
		{
			ptypeSrcs[i] = VT_IMG_FIXED(OBJ_LUMAFLOATIMG);
		}
    }

	// by default this transform uses the type of the first source as the 
	// output type
	virtual void    GetDstPixFormat(OUT int& typeDst,
									IN  const int* ptypeSrcs, 
									IN  UINT  /*uSrcCnt*/)
	{ typeDst = ptypeSrcs[0]; }

	virtual HRESULT GetRequiredSrcRect(OUT TRANSFORM_SOURCE_DESC* pSrcReq,
                                       OUT UINT& uSrcReqCount,
                                       IN  UINT  uSrcCnt,
                                       IN  const CRect& rctLayerDst
                                       )
	{
		for( UINT i = 0; i < uSrcCnt; i++, pSrcReq++ )
		{
			pSrcReq->bCanOverWrite = true;
			pSrcReq->rctSrc        = DivRect(i,rctLayerDst)+rctLayerDst.TopLeft();
			pSrcReq->uSrcIndex     = i;
		}
		uSrcReqCount = uSrcCnt;;
		return S_OK;
	}

	virtual HRESULT GetAffectedDstRect(OUT CRect& rctDst,
									  IN  const CRect& rctSrc,
									  IN  UINT /*uSrcIndex*/,
									  IN  UINT /*uSrcCnt*/)
	{
		rctDst = rctSrc;
		return S_OK;
	}

	virtual HRESULT GetResultingDstRect(OUT CRect& rctDst,
	                                    IN  const CRect& rctSrc,
	                                    IN  UINT /*uSrcIndex*/,
										IN  UINT /*uSrcCnt*/)
	{
		rctDst = rctSrc;
		return S_OK;
	}

    virtual HRESULT Transform(OUT CImg* pimgDst, 
                              IN  const CRect& rctDst,
                              IN  CImg *const *ppimgSrcRegions,
                              IN  const TRANSFORM_SOURCE_DESC* pSrcDesc,
                              IN  UINT  uSrcCnt
                              )
    {
		if( uSrcCnt != 4 || pimgDst->GetImgInfo().type != OBJ_LUMAFLOATIMG ||
            CRect(pimgDst->Rect()) != vt::CRect(0,0,rctDst.Width(),rctDst.Height()) )
        {
			return E_UNEXPECTED;
        }

        for( int i = 0; i < 4; i++ )
        {
            if( ppimgSrcRegions[i]->GetImgInfo().type != OBJ_LUMAFLOATIMG ||
                !rctDst.RectInRect(pSrcDesc[i].rctSrc) )
            {
                return E_UNEXPECTED;
            }
        }

		CFloatImg& dst = (CFloatImg&)*pimgDst;
        for( int i = 0; i < 4; i++ )
        {
            CFloatImg& src1 = (CFloatImg&)(*ppimgSrcRegions[i]);
            int y1 = pSrcDesc[i].rctSrc.top - rctDst.top;
            int x1 = pSrcDesc[i].rctSrc.left - rctDst.left;

            for( int y = 0; y < src1.Height(); y++ )
            {
                for( int x = 0; x < src1.Width(); x++ )
                {
                    dst.Pix(x1+x,y1+y) = src1.Pix(x,y);
                }
            }
        }
		return S_OK;
	}

	virtual HRESULT Clone(ITaskState** ppClone)
	{
		*ppClone = new CCopy4Transform();
		return (*ppClone)? S_OK: E_OUTOFMEMORY; 
	}
};

class CInvertTransform: public CImageTransformUnaryPoint<CInvertTransform, false>
{
public:
	virtual void    GetSrcPixFormat(IN OUT int* ptypeSrcs, 
									IN UINT  /*uSrcCnt*/,
									IN int /*typeDst*/)
	{ ptypeSrcs[0] = VT_IMG_FIXED(OBJ_LUMAFLOATIMG); }

    virtual HRESULT Transform(OUT CImg *pimgDst, const CRect &rctDst, const CImg& imgSrc)
	{
		if( pimgDst->GetImgInfo().type != OBJ_LUMAFLOATIMG ||
			imgSrc.GetImgInfo().type   != OBJ_LUMAFLOATIMG ||
			CRect(pimgDst->Rect())     != vt::CRect(0,0,rctDst.Width(),rctDst.Height()) ||
			CRect(pimgDst->Rect())     != CRect(imgSrc.Rect()) )
		{
			return E_UNEXPECTED;
		}
        
		CFloatImg& dst  = (CFloatImg&)*pimgDst;
		const CFloatImg& src0 = (CFloatImg&)imgSrc;
		for( int y = 0; y < dst.Height(); y++ )
		{
			for( int x = 0; x < dst.Width(); x++ )
			{
				dst.Pix(x,y) = -src0.Pix(x,y);
			}
		}
		return S_OK;
	}

	virtual HRESULT Clone(ITaskState** ppClone)
	{
		*ppClone = new CInvertTransform();
		return (*ppClone)? S_OK: E_OUTOFMEMORY; 
	}
};

class CAdd2Transform: public CImageTransformPoint<CAdd2Transform, false>
{
public:
	virtual void    GetSrcPixFormat(IN OUT int* ptypeSrcs, 
									IN UINT  uSrcCnt,
									IN int /*typeDst*/)
	{ 
		for( UINT i = 0; i < uSrcCnt; i++ )
		{
			ptypeSrcs[i] = VT_IMG_FIXED(OBJ_LUMAFLOATIMG);
		}
    }

    virtual HRESULT Transform(OUT CImg* pimgDst, 
                              IN  const CRect& rctDst,
                              IN  CImg *const *ppimgSrcRegions,
                              IN  UINT  uSrcCnt
                              )
    {
		if( uSrcCnt != 2 || pimgDst->GetImgInfo().type != OBJ_LUMAFLOATIMG ||
            CRect(pimgDst->Rect()) != vt::CRect(0,0,rctDst.Width(),rctDst.Height()) )
        {
			return E_UNEXPECTED;
        }

        for( int i = 0; i < 2; i++ )
        {
            if( ppimgSrcRegions[i]->GetImgInfo().type != OBJ_LUMAFLOATIMG ||
				rctDst.Width() != ppimgSrcRegions[i]->Width() ||
				rctDst.Height() != ppimgSrcRegions[i]->Height() )
            {
                return E_UNEXPECTED;
            }
        }

        VtAddImages(*pimgDst, *ppimgSrcRegions[0], *ppimgSrcRegions[1]);
		return S_OK;
	}

	virtual HRESULT Clone(ITaskState** ppClone)
	{
		*ppClone = new CAdd2Transform();
		return (*ppClone)? S_OK: E_OUTOFMEMORY; 
	}
};

bool VerifyTransform3Result(const CLumaByteImg& dst)
{
    for( int y = 0; y < dst.Height(); y++ )
    {
        for( int x = 0; x < dst.Width(); x++ )
        {
            if( dst.Pix(x,y) != 0 )
            {
                return false;
            }
        }
    }
	return true;
}

BEGIN_TEST_VTR_FAST(TransformTest3)
{
	// build the following graph
	//     
	//      g0
	//     / \
	//    g1 | 
	//    |  |
    //    g2 |
	//    \ /
	//     s
    //
    // where g2 reads the 4 's' and colesces them, g1 inverts 's' and 
    // sets 'can overwrite', g0 reads 1 large 's' and addes to the first input

    CImgReaderWriter<CLumaByteImg> imgSrc;
	CImgReaderWriter<CLumaByteImg> imgDst;
	LoadImage(GetImageDir() + L"newman.png", imgSrc);
	WIN_ASSERT_EQUAL(S_OK, imgDst.Create(imgSrc.Width(), imgSrc.Height(), 
		                                 imgSrc.Width(), imgSrc.Height(),
										 vt::CPoint(0,0)));

	// build the graph
	CCopy4Transform  x2;
	CInvertTransform x1;
    CAdd2Transform   x0;
	CTransformGraphNaryNode  g0(&x0), g2(&x2);
	CTransformGraphUnaryNode g1(&x1);
 
	g0.SetDest(&imgDst);
	g0.SetSourceCount(2);
	g0.BindSourceToTransform(0, &g1);
    g0.BindSourceToReader(1, &imgSrc);

	g1.BindSourceToTransform(&g2);
	g2.SetSourceCount(4);
    for( int i = 0; i < 4; i++ )
    {
        g2.BindSourceToReader(i, &imgSrc); 
    }

	// execute with the default block iteration
    WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&g0) );
	WIN_ASSERT_TRUE( VerifyTransform3Result(imgDst) );
}
END_TEST_VTR



BEGIN_TEST_VTR_SLOW(TaskTest)
{
	// make sure pushing 0 items works
	CTaskStatusEvent status;

	WIN_ASSERT_EQUAL(S_OK, status.Begin() );
	PushTaskAndWait(test_callback, NULL, &status, 0L);

	WIN_ASSERT_TRUE( status.GetDone() );

	// test the thread affinity feature
	CImgReaderWriter<CLumaByteImg> dst;
	WIN_ASSERT_EQUAL(S_OK, dst.Create(1313, 777, 1313, 777, vt::CPoint(0,0)));

	CTransform2Reader src;
	WIN_ASSERT_EQUAL(S_OK, src.Create(1313, 777, 1313, 777, vt::CPoint(0,0)));
	src.InitializePre(1313, 777);

	CImg newman;
	LoadImage(GetImageDir() + L"newman.png", newman);
	VtResizeImage(src, src.Rect(), newman);

    for( int i = 0; i < 400; i++ )
    {
        CRasterBlockMap bm(dst.Rect(), 256, 16, 4, 1);
    
        CThreadAffinityWorkIdSequencer seq;
        seq.Initialize(bm.GetBlockCols(), bm.GetBlockRows(), 4);
    
        CThreadFillTransform xtf;
        CTransformGraphUnaryNode g0(&xtf, &src);
        g0.SetDest(NODE_DEST_PARAMS(&dst, &bm));
    
        CThreadFillTransform::m_fillstatic = 0;
    
        VT_TRANSFORM_TASK_OPTIONS opts;
        opts.pSeq = &seq;
        WIN_ASSERT_EQUAL( S_OK, PushTransformTaskAndWait(&g0, (CTaskStatus*)NULL, &opts) );
     
        WIN_ASSERT_TRUE( VerifySequencerResult(dst, 16) ); 
    }
}
END_TEST_VTR

BEGIN_TEST_VTR_SLOW(TaskTest2)
{
	// make sure that tasks can go recursively deep without deadlocking
	CTaskStatusEvent stat;
	WIN_ASSERT_EQUAL(S_OK, stat.Begin() );

	WIN_ASSERT_EQUAL(S_OK, PushTask(TaskTest2Callback, NULL, &stat) );

	WIN_ASSERT_EQUAL(S_OK, stat.WaitForDone());

	WIN_ASSERT_EQUAL(S_OK, stat.GetTaskError());

	// create a set of threads that concurrently issue transform tasks to
	// the transform framework
	const int num_test_threads = 16;
	HANDLE hevents[num_test_threads];
	for( int i = 0; i < num_test_threads; i++ )
	{
		hevents[i] = CreateEvent(NULL, true, false, NULL);
	}
	for( int i = 0; i < num_test_threads; i++ )
	{
		QueueUserWorkItem(StartTasks, &hevents[i], 0);
	}
	WaitForMultipleObjects(num_test_threads, hevents, true, INFINITE);
}
END_TEST_VTR

END_TEST_FILE