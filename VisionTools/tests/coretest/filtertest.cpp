#include "stdafx.h"
#include "cthelpers.h"

//-----------------------------------------------------------------------------
//  macros to use unit test code for perf tuning
//-----------------------------------------------------------------------------
#if 1
#if _DEBUG
#define START_TIMING(__reps)
#define END_TIMING()
#else
#define START_TIMING(__reps) \
    int __repsVar = __reps; \
    ShowMessage(L"Running...");\
    for (int kkk = 0; kkk < 10; kkk++)\
    {\
        CTimer timer;\
        for ( int jjj = 0; jjj < __repsVar; jjj++)\
        {
#define END_TIMING() \
        } \
        ShowMessage(L"... %f ms",timer.GetTimeMilliSec()/(float)__repsVar); \
    } \
    ShowMessage(L"Done");
#endif
#endif

using namespace vt;

BEGIN_TEST_FILE(FilterTest)
////////////////////////////////////////////////////////////////////////////////
// Tests VtSeparableFilter121 for typical use cases, comparing result to that
// produced by VtSeparableFilter with same parameters
BEGIN_TEST_EX_FAST(Filter121_Byte)
{
    CVTResourceManagerInit ri;

	std::wstring warpimage = GetImageDir() + L"\\lena512color.bmp";
    CRGBAImg imgSrcRGBA;
    LoadImage(warpimage, imgSrcRGBA);

    IMAGE_EXTEND ex(Extend);

    C121Kernel k121;
    C1dKernelSet ks;
    ks.Create(1,2);
    ks.Set(0, -1, k121.AsKernel());

    vt::vector<vt::wstring> messages;

    // no-decimate no-transform no-blocking 
    for( int b = 1; b <= 4; b++ )
    {
        if( b==2 ) continue;

        CByteImg imgSrc(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);  
        WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgSrc, imgSrcRGBA) );

        CRect rctDst(0,0,imgSrc.Width(),imgSrc.Height());
        CByteImg imgDstA(imgSrc.Width(), imgSrc.Height(), b);
        WIN_ASSERT_EQUAL(S_OK, VtSeparableFilter121(imgDstA,rctDst,imgSrc,CPoint(0,0)) );
        CByteImg imgDstB(imgSrc.Width(), imgSrc.Height(), b);
        WIN_ASSERT_EQUAL(S_OK,  VtSeparableFilter(imgDstB,imgSrc,k121.AsKernel()) );

        WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		    1, &messages, 2.001f/255.f)) );
    }

    // decimate no-transform no-blocking 
    for( int b = 1; b <= 4; b++ )
    {
        if( b==2 ) continue;

        CByteImg imgSrc(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);  
        WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgSrc, imgSrcRGBA ));

        CRect rctDst(0,0,imgSrc.Width()/2,imgSrc.Height()/2);
        CByteImg imgDstA(imgSrc.Width()/2, imgSrc.Height()/2, b);
        WIN_ASSERT_EQUAL(S_OK, VtSeparableFilter121Decimate2to1(imgDstA,rctDst,imgSrc,CPoint(0,0)) );
        CByteImg imgDstB(imgSrc.Width()/2, imgSrc.Height()/2, b);
        WIN_ASSERT_EQUAL(S_OK, VtSeparableFilter(imgDstB,rctDst,imgSrc,CPoint(0,0),ks) );

        WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		    1, &messages, 2.001f/255.f)) );
    }

    // no-decimate blocking
    for( int b = 1; b <= 4; b++ )
    {
        if( b==2 ) continue;

        CByteImg imgSrc(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);  
        WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgSrc, imgSrcRGBA) );
        CByteImg imgDstA(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);
        CByteImg imgDstB(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);

        for( CBlockIterator bi(imgDstA.Rect(), 256); !bi.Done(); bi.Advance() )
        {
            vt::CRect rctDst = bi.GetRect();
            CByteImg imgDstBlock;
            WIN_ASSERT_EQUAL(S_OK, imgDstA.Share(imgDstBlock, &rctDst) );
            WIN_ASSERT_EQUAL(S_OK, VtSeparableFilter121(imgDstBlock, rctDst, imgSrc, vt::CPoint(0,0)) );
        }
        for( CBlockIterator bi(imgDstB.Rect(), 256); !bi.Done(); bi.Advance() )
        {
            vt::CRect rctDst = bi.GetRect();
            CByteImg imgDstBlock;
            WIN_ASSERT_EQUAL(S_OK, imgDstB.Share(imgDstBlock, &rctDst) );
            WIN_ASSERT_EQUAL(S_OK, VtSeparableFilter(imgDstBlock, rctDst, imgSrc, vt::CPoint(0,0), k121.AsKernel()) );
        }
        WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		    1, &messages, 2.001f/255.f)) );
    }

    // decimate blocking
    for( int b = 1; b <= 4; b++ )
    {
        if( b==2 ) continue;

        CByteImg imgSrc(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);  
        WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgSrc, imgSrcRGBA) );
        CByteImg imgDstA(imgSrcRGBA.Width()/2, imgSrcRGBA.Height()/2, b);
        CByteImg imgDstB(imgSrcRGBA.Width()/2, imgSrcRGBA.Height()/2, b);

        for( CBlockIterator bi(imgDstA.Rect(), 128); !bi.Done(); bi.Advance() )
        {
            vt::CRect rctDst = bi.GetRect();
            CByteImg imgDstBlock;
            WIN_ASSERT_EQUAL(S_OK, imgDstA.Share(imgDstBlock, &rctDst) );
            WIN_ASSERT_EQUAL(S_OK, VtSeparableFilter121Decimate2to1(imgDstBlock, rctDst, imgSrc, vt::CPoint(0,0)) );
        }
        for( CBlockIterator bi(imgDstB.Rect(), 128); !bi.Done(); bi.Advance() )
        {
            vt::CRect rctDst = bi.GetRect();
            CByteImg imgDstBlock;
            WIN_ASSERT_EQUAL(S_OK, imgDstB.Share(imgDstBlock, &rctDst) );
            WIN_ASSERT_EQUAL(S_OK, VtSeparableFilter(imgDstBlock, rctDst, imgSrc, vt::CPoint(0,0), ks) );
        }
        WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		    1, &messages, 2.001f/255.f)) );
    }

#if 0
    // still mysteriously failing - 121 result slightly blurrier than it should
    // be for what reason I have no idea...

    // no-decimate transform
    for( int b = 1; b <= 4; b++ )
    {
        if( b==2 ) continue;

        CImgReaderWriter<CByteImg> imgSrc;
        WIN_ASSERT_EQUAL(S_OK, imgSrc.Create(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b, 
                      imgSrcRGBA.Width(), imgSrcRGBA.Height(), vt::CPoint(0,0)) );  
        WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgSrc, imgSrcRGBA) );
        CImgReaderWriter<CByteImg> imgDstA;
        WIN_ASSERT_EQUAL(S_OK, imgDstA.Create(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b,
                       imgSrcRGBA.Width(), imgSrcRGBA.Height(), vt::CPoint(0,0)) );
        CImgReaderWriter<CByteImg> imgDstB;
        WIN_ASSERT_EQUAL(S_OK, imgDstB.Create(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b,
                       imgSrcRGBA.Width(), imgSrcRGBA.Height(), vt::CPoint(0,0)) );
        {
            CSeparableFilter121Transform x;
            WIN_ASSERT_EQUAL(S_OK, x.Initialize() );
            CTransformGraphUnaryNode g(&x);
            g.BindSourceToReader(&imgSrc,&ex);
            
			CRasterBlockMap bm(BLOCKITER_INIT(imgDstA.Rect(), 640, 64), 4, 4);
			g.SetDest(NODE_DEST_PARAMS(&imgDstA, &bm));
            WIN_ASSERT_EQUAL(S_OK, PushTransformTaskAndWait(&g, (CTaskStatus*)NULL) ); 
        }
        {
            CSeparableFilterTransform x;
			WIN_ASSERT_EQUAL(S_OK, x.Initialize(OBJ_UNDEFINED, k121.AsKernel()) );
            CTransformGraphUnaryNode g(&x);
            g.BindSourceToReader(&imgSrc,&ex);

			CRasterBlockMap bm(BLOCKITER_INIT(imgDstB.Rect(), 640, 64), 4, 4);
			g.SetDest(NODE_DEST_PARAMS(&imgDstB, &bm));
            WIN_ASSERT_EQUAL(S_OK, PushTransformTaskAndWait(&g, (CTaskStatus*)NULL) ); 
        }

        WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		    1, &messages,2.001f/255.f)) );
    }
#endif

    // decimate transform
    for( int b = 1; b <= 4; b++ )
    {
        if( b==2 ) continue;

        CImgReaderWriter<CByteImg> imgSrc;
        WIN_ASSERT_EQUAL(S_OK, imgSrc.Create(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b, 
                      imgSrcRGBA.Width(), imgSrcRGBA.Height(), vt::CPoint(0,0)) );  
        WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgSrc, imgSrcRGBA) );
        CImgReaderWriter<CByteImg> imgDstA;
        WIN_ASSERT_EQUAL(S_OK, imgDstA.Create(imgSrcRGBA.Width()/2, imgSrcRGBA.Height()/2, b,
                       imgSrcRGBA.Width()/2, imgSrcRGBA.Height()/2, vt::CPoint(0,0)) );
        CImgReaderWriter<CByteImg> imgDstB;
        WIN_ASSERT_EQUAL(S_OK, imgDstB.Create(imgSrcRGBA.Width()/2, imgSrcRGBA.Height()/2, b,
                       imgSrcRGBA.Width()/2, imgSrcRGBA.Height()/2, vt::CPoint(0,0)) );
        {
            CSeparableFilter121Transform x;
            WIN_ASSERT_EQUAL(S_OK, x.InitializeDecimate2to1() );
            CTransformGraphUnaryNode g(&x);
            g.BindSourceToReader(&imgSrc,&ex);
			CRasterBlockMap bm(BLOCKITER_INIT(imgDstA.GetImgInfo().LayerRect(), 320, 64), 4, 4);
			g.SetDest(NODE_DEST_PARAMS(&imgDstA, &bm));
            WIN_ASSERT_EQUAL(S_OK, PushTransformTaskAndWait(&g, (CTaskStatus*)NULL) ); 
        }
        {
            CSeparableFilterTransform x;
            WIN_ASSERT_EQUAL(S_OK, x.InitializeForPyramidGen(OBJ_UNDEFINED, k121.AsKernel()) );
            CTransformGraphUnaryNode g(&x);
            g.BindSourceToReader(&imgSrc,&ex);
			CRasterBlockMap bm(BLOCKITER_INIT(imgDstB.GetImgInfo().LayerRect(), 320, 64), 4, 4);
			g.SetDest(NODE_DEST_PARAMS(&imgDstB, &bm));
            WIN_ASSERT_EQUAL(S_OK, PushTransformTaskAndWait(&g, (CTaskStatus*)NULL) ); 
        }

        WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		    1, &messages, 2.001f/255.f)) );
    }

    // uneven size tests
    {
        int tcase[][3] = { // test case data: width, height, band count
            { 513, 634, 1 },
            { 510, 631, 1 },
            { 513, 634, 4 },
            { 510, 631, 4 },
            { 100, 99, 1 },
            { 500, 643, 3 },};
        for( int i=0; i<sizeof(tcase)/(3*sizeof(int)); i++ )
        {
            int w = tcase[i][0];
            int h = tcase[i][1];
            int b = tcase[i][2];

            CByteImg imgSrcC(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);  
            WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgSrcC, imgSrcRGBA) );
            CByteImg imgSrc(w, h, b);  
            WIN_ASSERT_EQUAL(S_OK, VtResizeImage(imgSrc, imgSrc.Rect(), imgSrcC) );

            CRect rctDst(0,0,w,h);
            CByteImg imgDstA(w, h, b);
            WIN_ASSERT_EQUAL(S_OK, VtSeparableFilter121(imgDstA,rctDst,imgSrc,CPoint(0,0)) );
            CByteImg imgDstB(w, h, b);
            WIN_ASSERT_EQUAL(S_OK, VtSeparableFilter(imgDstB,imgSrc,k121.AsKernel()) );

            WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		        1, &messages, 2.001f/255.f)) );
        }
    }
}
END_TEST_EX

////////////////////////////////////////////////////////////////////////////////
// Tests VtSeparableFilter121 for typical use cases, comparing result to that
// produced by VtSeparableFilter with same parameters
BEGIN_TEST_EX_FAST(Filter121_Float)
{
    CVTResourceManagerInit ri;

	std::wstring warpimage = GetImageDir() + L"\\lena512color.bmp";
    CRGBAFloatImg imgSrcRGBA;
    LoadImage(warpimage, imgSrcRGBA);

    C121Kernel k121;
    C1dKernelSet ks;
    ks.Create(1,2);
    ks.Set(0, -1, k121.AsKernel());

    vt::vector<vt::wstring> messages;

    // no-decimate no-transform no-blocking 
    for( int b = 1; b <= 4; b++ )
    {
        if( b==2 ) continue;

        CFloatImg imgSrc(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);  
        VtConvertImage(imgSrc, imgSrcRGBA);

        CRect rctDst(0,0,imgSrc.Width(),imgSrc.Height());
        CFloatImg imgDstA(imgSrc.Width(), imgSrc.Height(), b);
        VtSeparableFilter121(imgDstA,rctDst,imgSrc,CPoint(0,0));
        CFloatImg imgDstB(imgSrc.Width(), imgSrc.Height(), b);
        VtSeparableFilter(imgDstB,imgSrc,k121.AsKernel());

        WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		    1, &messages, 1.001f/255.f)) );
    }

    // decimate no-transform no-blocking 
    for( int b = 1; b <= 4; b++ )
    {
        if( b==2 ) continue;

        CFloatImg imgSrc(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);  
        VtConvertImage(imgSrc, imgSrcRGBA);

        CRect rctDst(0,0,imgSrc.Width()/2,imgSrc.Height()/2);
        CFloatImg imgDstA(imgSrc.Width()/2, imgSrc.Height()/2, b);
        VtSeparableFilter121Decimate2to1(imgDstA,rctDst,imgSrc,CPoint(0,0));
        CFloatImg imgDstB(imgSrc.Width()/2, imgSrc.Height()/2, b);
        VtSeparableFilter(imgDstB,rctDst,imgSrc,CPoint(0,0),ks);

        WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		    1, &messages, 1.001f/255.f)) );
    }

    // no-decimate blocking
    for( int b = 1; b <= 4; b++ )
    {
        if( b==2 ) continue;

        CFloatImg imgSrc(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);  
        VtConvertImage(imgSrc, imgSrcRGBA);
        CFloatImg imgDstA(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);
        CFloatImg imgDstB(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);

        for( CBlockIterator bi(imgDstA.Rect(), 256); !bi.Done(); bi.Advance() )
        {
            vt::CRect rctDst = bi.GetRect();
            CFloatImg imgDstBlock;
            imgDstA.Share(imgDstBlock, &rctDst);
            VtSeparableFilter121(imgDstBlock, rctDst, imgSrc, vt::CPoint(0,0));
        }
        for( CBlockIterator bi(imgDstB.Rect(), 256); !bi.Done(); bi.Advance() )
        {
            vt::CRect rctDst = bi.GetRect();
            CFloatImg imgDstBlock;
            imgDstB.Share(imgDstBlock, &rctDst);
            VtSeparableFilter(imgDstBlock, rctDst, imgSrc, vt::CPoint(0,0), k121.AsKernel());
        }
        WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		    1, &messages, 1.001f/255.f)) );
    }

    // decimate blocking
    for( int b = 1; b <= 4; b++ )
    {
        if( b==2 ) continue;

        CFloatImg imgSrc(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);  
        VtConvertImage(imgSrc, imgSrcRGBA);
        CFloatImg imgDstA(imgSrcRGBA.Width()/2, imgSrcRGBA.Height()/2, b);
        CFloatImg imgDstB(imgSrcRGBA.Width()/2, imgSrcRGBA.Height()/2, b);

        for( CBlockIterator bi(imgDstA.Rect(), 128); !bi.Done(); bi.Advance() )
        {
            vt::CRect rctDst = bi.GetRect();
            CFloatImg imgDstBlock;
            imgDstA.Share(imgDstBlock, &rctDst);
            VtSeparableFilter121Decimate2to1(imgDstBlock, rctDst, imgSrc, vt::CPoint(0,0));
        }
        for( CBlockIterator bi(imgDstB.Rect(), 128); !bi.Done(); bi.Advance() )
        {
            vt::CRect rctDst = bi.GetRect();
            CFloatImg imgDstBlock;
            imgDstB.Share(imgDstBlock, &rctDst);
            VtSeparableFilter(imgDstBlock, rctDst, imgSrc, vt::CPoint(0,0), ks);
        }
        WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		    1, &messages, 1.001f/255.f)) );
    }
}
END_TEST_EX

////////////////////////////////////////////////////////////////////////////////
// Tests VtSeparableFilterBox, comparing result to that produced by VtSeparableFilter
BEGIN_TEST_EX_FAST(FilterBox)
{
    CVTResourceManagerInit ri;

	std::wstring warpimage = GetImageDir() + L"\\lena512color.bmp";
    CRGBAFloatImg imgSrcRGBA;
    LoadImage(warpimage, imgSrcRGBA);

    vt::vector<vt::wstring> messages;

    // decimate 2:1
    {
        int b = 1;

        C1dKernelSet ksbox2;
        {
            C1dKernel k;
            float kw[] = { .5f, .5f };
            k.Create(2, 0, kw);
            ksbox2.Create(1,2);
            ksbox2.Set(0, 0, k);
        }

        // float
        {
            CFloatImg imgSrc(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);  
            VtConvertImage(imgSrc, imgSrcRGBA);

            CRect rctDst(0,0,imgSrc.Width()/2,imgSrc.Height()/2);
            CFloatImg imgDstA(imgSrc.Width()/2, imgSrc.Height()/2, b);
            VtSeparableFilterBoxDecimate2to1(imgDstA,rctDst,imgSrc,CPoint(0,0));
            CFloatImg imgDstB(imgSrc.Width()/2, imgSrc.Height()/2, b);
            VtSeparableFilter(imgDstB,rctDst,imgSrc,CPoint(0,0),ksbox2);

            WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		        1, &messages, 1.001f/255.f)) );
        }

        // byte
        {
            CByteImg imgSrc(imgSrcRGBA.Width(), imgSrcRGBA.Height(), b);  
            VtConvertImage(imgSrc, imgSrcRGBA);

            CRect rctDst(0,0,imgSrc.Width()/2,imgSrc.Height()/2);
            CByteImg imgDstA(imgSrc.Width()/2, imgSrc.Height()/2, b);
            VtSeparableFilterBoxDecimate2to1(imgDstA,rctDst,imgSrc,CPoint(0,0));
            CByteImg imgDstB(imgSrc.Width()/2, imgSrc.Height()/2, b);
            VtSeparableFilter(imgDstB,rctDst,imgSrc,CPoint(0,0),ksbox2);

            WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		        1, &messages, 1.001f/255.f)) );
        }
    }

    // decimate 4:1
    {
        C1dKernelSet ksbox4;
        {
            C1dKernel k;
            float kw[] = { .25f, .25f, .25f, .25f };
            k.Create(4, 1, kw);
            ksbox4.Create(1,4);
            ksbox4.Set(0, 0, k);
        }

        // float
        {
            CLumaFloatImg imgSrc(imgSrcRGBA.Width(), imgSrcRGBA.Height());  
            VtConvertImage(imgSrc, imgSrcRGBA);

            CRect rctDst(0,0,imgSrc.Width()/4,imgSrc.Height()/4);
            CLumaFloatImg imgDstA(imgSrc.Width()/4, imgSrc.Height()/4);
            VtSeparableFilterBoxDecimate4to1(imgDstA,rctDst,imgSrc,CPoint(0,0));
            CLumaFloatImg imgDstB(imgSrc.Width()/4, imgSrc.Height()/4);
            VtSeparableFilter(imgDstB,rctDst,imgSrc,CPoint(0,0),ksbox4);

            WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		        1, &messages, 1.001f/255.f)) );
        }

        // byte
        {
            CLumaByteImg imgSrc(imgSrcRGBA.Width(), imgSrcRGBA.Height());  
            VtConvertImage(imgSrc, imgSrcRGBA);

            CRect rctDst(0,0,imgSrc.Width()/4,imgSrc.Height()/4);
            CLumaByteImg imgDstA(imgSrc.Width()/4, imgSrc.Height()/4);
            VtSeparableFilterBoxDecimate4to1(imgDstA,rctDst,imgSrc,CPoint(0,0));
            CLumaByteImg imgDstB(imgSrc.Width()/4, imgSrc.Height()/4);
            VtSeparableFilter(imgDstB,rctDst,imgSrc,CPoint(0,0),ksbox4);

            WIN_ASSERT_EQUAL( true, (VtCompareImages(imgDstA, imgDstB,
		        1, &messages, 1.001f/255.f)) );
        }
    }
}
END_TEST_EX

////////////////////////////////////////////////////////////////////////////////
// 
// very basic tests for the CPyramid class
//
BEGIN_TEST_EX_FAST(Pyramid)
{
	// construct an full-octave and sub-octave primal lanczos pyramid and compare
	// the full-octave levels
	{
	CFloatImg lfi1;
	WIN_ASSERT_EQUAL(S_OK, lfi1.Create(753, 895, 3));
	GenerateRamp(lfi1);

	CFloatPyramid lfp1;
	PYRAMID_PROPERTIES pp1;
	pp1.eAutoFilter = ePyramidFilterLanczos3Primal;
	pp1.bTruncateOddLevels = false;
	WIN_ASSERT_EQUAL(S_OK, lfp1.Create(lfi1, &pp1));

	CRGBFloatPyramid lfp2;
	PYRAMID_PROPERTIES pp2;
	pp2.eAutoFilter = ePyramidFilterNone;
	pp2.bTruncateOddLevels = false;
	pp2.iSubOctavesPerLevel = 3;
	pp2.iSubOctaveStepSize  = 3;
	WIN_ASSERT_EQUAL(S_OK, lfp2.Create(lfi1.Width(), lfi1.Height(), &pp2));
	GenerateRamp(lfp2.GetLevel(0));
	WIN_ASSERT_EQUAL(S_OK, VtConstructLanczosPyramid(lfp2));

	for( int l = 0; l < lfp1.GetNumLevels(); l++ )
	{
		CImg& i1 = lfp1.GetLevel(l);
		CImg& i2 = lfp2.GetLevel(l);
		WIN_ASSERT_EQUAL( true, VtCompareImages(i1, i2) );
	}
	}


	// TODO: repeat for dual

	// repeat above for gaussian
	{
	CFloatImg lfi1;
	WIN_ASSERT_EQUAL(S_OK, lfi1.Create(647, 1122, 4));
	GenerateRamp(lfi1);

	CFloatPyramid lfp1;
	PYRAMID_PROPERTIES pp1;
	pp1.eAutoFilter = ePyramidFilterGaussianPrimal;
	pp1.bTruncateOddLevels = false;
	WIN_ASSERT_EQUAL(S_OK, lfp1.Create(lfi1, &pp1));

	CRGBAFloatPyramid lfp2;
	PYRAMID_PROPERTIES pp2;
	pp2.eAutoFilter = ePyramidFilterNone;
	pp2.bTruncateOddLevels = false;
	pp2.iSubOctavesPerLevel = 3;
	pp2.iSubOctaveStepSize  = 3;
	WIN_ASSERT_EQUAL(S_OK, lfp2.Create(lfi1.Width(), lfi1.Height(), &pp2));
	GenerateRamp(lfp2.GetLevel(0));
	WIN_ASSERT_EQUAL(S_OK, VtConstructGaussianPyramid(lfp2));

	for( int l = 0; l < lfp1.GetNumLevels(); l++ )
	{
		CImg& i1 = lfp1.GetLevel(l);
		CImg& i2 = lfp2.GetLevel(l);
		WIN_ASSERT_EQUAL( true, VtCompareImages(i1, i2) );
	}
	}
}
END_TEST_EX
////////////////////////////////////////////////////////////////////////////////

END_TEST_FILE