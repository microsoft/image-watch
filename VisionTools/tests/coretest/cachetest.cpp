////////////////////////////////////////////////////////////////////////////////
// coretest.cpp : Unit tests for VisionTools core
//
#include "stdafx.h"

#include "..\src\core\vtiio.h"

#include "cthelpers.h"

using namespace vt;



////////////////////////////////////////////////////////////////////////////////
//
// basic tests of CImageImport and the image cache
//
const UINT g_nTestImages = 6;



CPyramid*    g_pReferenceResults[g_nTestImages];
wchar_t      g_reffilename1[MAX_PATH];
wchar_t      g_reffilename2[MAX_PATH];

class CICTestTask
{
public:
	CICTestTask(IIndexedImageReaderWriter *pSrc): m_pSrc(pSrc)
	{m_cs.Startup();}

	~CICTestTask() 
	{}

	static HRESULT TaskCallback(void* pV, LONG i, vt::CTaskProgress*)
	{ return((CICTestTask*)pV)->TaskCallback(i);}

protected: 
	HRESULT TaskCallback(int i);

protected:
    CCritSection m_cs;
	IIndexedImageReaderWriter *m_pSrc;
};

HRESULT CICTestTask::TaskCallback(int i)
{
	CRand rand(i+1);

    int types[4] = {EL_FORMAT_BYTE, EL_FORMAT_SHORT, EL_FORMAT_FLOAT, EL_FORMAT_HALF_FLOAT};

	UINT uFrameCnt = m_pSrc->GetFrameCount();

	// random image
	UINT uIndex = rand.IRand(uFrameCnt)%uFrameCnt;
	CImgInfo info = m_pSrc->GetImgInfo(uIndex);

	// random level
	UINT uLevels = ComputePyramidLevelCount(info.width, info.height);
	UINT uLevel  = rand.IRand(uLevels)%uLevels;

	// random rect in image
	info = m_pSrc->GetImgInfo(uIndex, uLevel);
	WIN_ASSERT_TRUE( info.width>=1 && info.height>=1 );
	vt::CRect r = RandRect(info.width, info.height, rand);

	// ensure that the info matches
	CImgInfo inforef = g_pReferenceResults[uIndex]->GetImgInfo(uLevel);
	WIN_ASSERT_TRUE( !memcmp(&info, &inforef, sizeof(info)) );

	// random operation
	double op = rand.DRand();
#define TEST_CREATE
#ifdef TEST_CREATE
	if( op > 0.95 &&
        wcslen(g_reffilename1) > 0 && wcslen(g_reffilename2) > 0)
	{
		// test that the cache is robust to failure cases
		CImageCache* pIC = VtGetImageCacheInstance();

		UINT uSrcId;
		HRESULT hr = pIC->AddSource(L"*<|.vti", uSrcId, true);
		WIN_ASSERT_EQUAL(HRESULT(0x8007007b), hr);

		// I think that this should fail since the cache already has
		// one of these open with different properties?
		// Or perhaps the same process should never have two copies
		// open?
		CImgInfo info3(777,666,VT_IMG_MAKE_TYPE(EL_FORMAT_SHORT,3));
		IMG_CACHE_SOURCE_PROPERTIES props3;
		props3.pBackingBlob = g_reffilename1;
		hr = pIC->AddSource(info3, 1, uSrcId, &props3);
		WIN_ASSERT_EQUAL(HRESULT(0x80070050), hr);

		CImgInfo info2(120,1200,VT_IMG_MAKE_TYPE(EL_FORMAT_BYTE,3));
		IMG_CACHE_SOURCE_PROPERTIES props2;
		props2.pBackingBlob = g_reffilename2;
		hr = pIC->AddSource(info2, 1, uSrcId, &props2);
		WIN_ASSERT_EQUAL(HRESULT(0x80070050), hr);

		CImgInfo info1(10,10,VT_IMG_MAKE_TYPE(EL_FORMAT_BYTE,10));
		IMG_CACHE_SOURCE_PROPERTIES props1;
		props1.pBackingBlob = L"|||.vti";
		hr = pIC->AddSource(info1, 1, uSrcId, &props1);
		WIN_ASSERT_EQUAL(HRESULT(0x8007007b), hr);
	}
	else
#endif   
	if ( op > 0.4 )
	{
		// pick a random pixel format 1,3,4 bands + byte, short, half_float, float
		int iB = rand.IRand(3)%3;
		iB = (iB==0)? 1: iB+2;
		int type = VT_IMG_MAKE_TYPE(types[rand.IRand(4)%4], iB);

		CImg img;
		WIN_ASSERT_EQUAL(S_OK, img.Create(r.Width(), r.Height(), type));
		WIN_ASSERT_EQUAL(S_OK, m_pSrc->ReadRegion(uIndex, r, img, uLevel));

		float fTol = (uLevel==0)? 0: 2.01f/255.f;
		CImg imgRef;
		WIN_ASSERT_EQUAL(S_OK, imgRef.Create(r.Width(), r.Height(), type) );
		CImg imgRefOrig;
		m_cs.Enter();
	    HRESULT hrt = g_pReferenceResults[uIndex]->GetLevel(uLevel).Share(imgRefOrig, &r);
		m_cs.Leave();
		WIN_ASSERT_EQUAL(S_OK, hrt);
		WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgRef, imgRefOrig));
		WIN_ASSERT_TRUE( VtCompareImages(img, imgRef, 1, NULL, fTol) );
	} 
	else if ( op > 0.1 )
	{
		CImg imgRef;
		m_cs.Enter();
		HRESULT hrt = g_pReferenceResults[uIndex]->GetLevel(uLevel).Share(imgRef, &r);  
		m_cs.Leave();
		WIN_ASSERT_EQUAL(S_OK, hrt);
		WIN_ASSERT_EQUAL(S_OK, m_pSrc->WriteRegion(uIndex, r, imgRef, uLevel));
	} 
	else
	{
		WIN_ASSERT_EQUAL(S_OK, m_pSrc->Prefetch(uIndex, r, uLevel));
	}

	return S_OK;
}


//+-----------------------------------------------------------------------------
//
// Function: MergeSource
// 
// Synopsis: the core merging routine
// 
//------------------------------------------------------------------------------
void MergeSource(CFloatImg& dst, CFloatImg& prev, CShortImg& src)
{
	for (int y = 0; y < dst.Height(); y++)
	{
		float *ptrDst = dst.Ptr(y);
		float *ptrPrev = prev.Ptr(y);
		unsigned __int16 *ptrSrc = src.Ptr(y);
		for (int x = 0; x < dst.Width(); x++)
		{
			float value = float(*ptrSrc) / 65534.f;
			*ptrDst = *ptrPrev + value;

			ptrPrev++;
			ptrDst++;
			ptrSrc++;
		}
	}
}

BEGIN_TEST_FILE(CacheTests)

#if WINAPI_FAMILY==WINAPI_FAMILY_DESKTOP_APP

BEGIN_TEST_VTR_SLOW(CacheTest)
{
	CRand rand;

    int types[4] = {EL_FORMAT_BYTE, EL_FORMAT_SHORT, EL_FORMAT_FLOAT, EL_FORMAT_HALF_FLOAT};

	// max size pixel in test below
	Byte fillval[16];

	// set the cache size small enough to cause disk backing
	WIN_ASSERT_EQUAL(S_OK, VtGetImageCacheInstance()->SetMaxMemoryUsage(4*1024*1024) );

	// load the source
	CRGBAImg src;
	LoadImage(GetImageDir() + L"newman_large.png", src);

    GetTempPath(MAX_PATH, g_reffilename1);
	wcscat_s(g_reffilename1, MAX_PATH, L"rwtest.vti");

    GetTempPath(MAX_PATH, g_reffilename2);
	wcscat_s(g_reffilename2, MAX_PATH, L"readtest.vti");

	WIN_ASSERT_EQUAL(S_OK, VtSaveImage(g_reffilename2, src));

	// generate the test examples
	int iW = src.Width();
	int iH = src.Height();
	for ( UINT k = 0; k < 4; k++ )
	{
        IMG_CACHE_SOURCE_STATISTICS Stats1[g_nTestImages];
		CPyramid refs[g_nTestImages];
		CImgListInCache ic;
		for ( UINT i = 0; i < g_nTestImages; i++ )
		{
			// randomly resize the source
			int iTW = (int)(rand.URand(0.5, 1.2)*(double(iW)));
			int iTH = (int)(rand.URand(0.5, 1.2)*(double(iH)));

			// pick a random pixel format 1,3,4 bands + byte, short, half_float, float
			int iB = rand.IRand(3)%3;
			iB = (iB==0)? 1: iB+2;
			int type = VT_IMG_MAKE_TYPE(types[rand.IRand(4)%4], iB);

			// create test image
			PYRAMID_PROPERTIES pyrprops;
			pyrprops.bTruncateOddLevels = false;
			pyrprops.bFloatPixelUseSupressRingingFilter = true;
			WIN_ASSERT_EQUAL(S_OK, refs[i].Create(iTW, iTH, type, &pyrprops) );
			WIN_ASSERT_EQUAL(S_OK, VtResizeImage(refs[i].GetLevel(0), vt::CRect(0,0,iTW,iTH), src, 
				                                 eSamplerKernelLanczos3));

			// set the ref pointer
			g_pReferenceResults[i] = refs+i;

			// create an imgincache
			if( i == 0 )
			{
				IMG_CACHE_SOURCE_PROPERTIES icsprops;
				icsprops.pBackingBlob = g_reffilename1;
				WIN_ASSERT_EQUAL(S_OK, ic.PushBack(refs[i].GetLevel(0), &icsprops));
			}
			else
			{
				WIN_ASSERT_EQUAL(S_OK, ic.PushBack(refs[i].GetLevel(0)));
			}
		}

		// create a set of tasks that randomly read, prefetch
		CICTestTask testtasks(&ic);

		// use up to 8 threads to bang on cache
		VT_TASK_OPTIONS opts(8);

		CImgInCache testread;
		WIN_ASSERT_EQUAL(S_OK, testread.Create(g_reffilename2, true) );

		// run the task on opts.maxthreads separate threads
		WIN_ASSERT_EQUAL(S_OK, vt::PushTaskAndWait(CICTestTask::TaskCallback, 
												   &testtasks, NULL, 256, &opts) );

		// randomly update sources
		for ( UINT i = 0; i < g_nTestImages; i++ )
		{
            // print stats
            WIN_ASSERT_EQUAL( S_OK, VtGetImageCacheInstance()->
                GetSourceStatistics(ic.GetCacheSourceId(i), Stats1[i]) );
            WIN_ASSERT_TRUE( Stats1[i].reads.clear_miss_bytes      == 0 && Stats1[i].reads.reader_miss_bytes      == 0 );
            WIN_ASSERT_TRUE( Stats1[i].writes.clear_miss_bytes     == 0 && Stats1[i].writes.reader_miss_bytes     == 0 );
            WIN_ASSERT_TRUE( Stats1[i].prefetches.clear_miss_bytes == 0 && Stats1[i].prefetches.reader_miss_bytes == 0 );
            WIN_ASSERT_TRUE( Stats1[i].clears.clear_miss_bytes     == 0 && Stats1[i].clears.reader_miss_bytes     == 0 );
            /*
			printf("pass 1 image %d read     request 0x%08I64x blob 0x%08I64x autogen 0x%08I64x\n", i,
                Stats1[i].reads.request_bytes, Stats1[i].reads.blob_miss_bytes, Stats1[i].reads.autogen_miss_bytes);
            printf("pass 1 image %d write    request 0x%08I64x blob 0x%08I64x autogen 0x%08I64x\n", i,
                Stats1[i].writes.request_bytes, Stats1[i].writes.blob_miss_bytes, Stats1[i].writes.autogen_miss_bytes);
            printf("pass 1 image %d prefetch request 0x%08I64x blob 0x%08I64x autogen 0x%08I64x\n", i,
                Stats1[i].prefetches.request_bytes, Stats1[i].prefetches.blob_miss_bytes, Stats1[i].prefetches.autogen_miss_bytes);
            printf("pass 1 image %d clear    request 0x%08I64x blob 0x%08I64x autogen 0x%08I64x\n", i,
                Stats1[i].clears.request_bytes, Stats1[i].clears.blob_miss_bytes, Stats1[i].clears.autogen_miss_bytes);
            printf("pass 1 image %d flushed 0x%08I64x reflush 0x%08I64x\n", i,
                Stats1[i].flushed_bytes, Stats1[i].reflush_bytes);
			*/

			if ( rand.DRand() > 0.6 )
			{
				// reset then rewrite some
				WIN_ASSERT_EQUAL( S_OK, VtGetImageCacheInstance()->
								  ClearSource(ic.GetCacheSourceId(i)) );

				WIN_ASSERT_EQUAL(S_OK, ic.WriteImg(i, refs[i].GetLevel(0)));
			} 
			else
			{
				// for others "fill" random rects in level 0 of both cache 
				// and reference pyramid 
				CImgInfo info = ic.GetImgInfo(i);
				for ( UINT j = 0; j < 8; j++ )
				{
					vt::CRect r = RandRect(info.width/4, info.height/4, rand);
					RandPixelValue(fillval, info, rand);
					WIN_ASSERT_EQUAL(S_OK, ic.Fill(i, fillval, &r));
					WIN_ASSERT_EQUAL(S_OK, refs[i].GetLevel(0).Fill(fillval, &r) );
				}
				refs[i].Invalidate();
			}
		}

		// run the task again
		WIN_ASSERT_EQUAL(S_OK, vt::PushTaskAndWait(CICTestTask::TaskCallback, 
												   &testtasks, NULL, 256, &opts) );

		for ( UINT i = 0; i < g_nTestImages; i++ )
		{
            // print stats
            IMG_CACHE_SOURCE_STATISTICS Stats2;
            WIN_ASSERT_EQUAL( S_OK, VtGetImageCacheInstance()->
                GetSourceStatistics(ic.GetCacheSourceId(i), Stats2) );
            Stats2 -= Stats1[i];
            /*
			printf("pass 2 image %d flushed 0x%08I64x reflush 0x%08I64x\n", i,
                Stats2.flushed_bytes, Stats2.reflush_bytes);
			*/
            WIN_ASSERT_TRUE( Stats2.reads.clear_miss_bytes      == 0 && Stats2.reads.reader_miss_bytes      == 0 );
            WIN_ASSERT_TRUE( Stats2.writes.clear_miss_bytes     == 0 && Stats2.writes.reader_miss_bytes     == 0 );
            WIN_ASSERT_TRUE( Stats2.prefetches.clear_miss_bytes == 0 && Stats2.prefetches.reader_miss_bytes == 0 );
            WIN_ASSERT_TRUE( Stats2.clears.clear_miss_bytes     == 0 && Stats2.clears.reader_miss_bytes     == 0 );
            /*
			printf("pass 2 image %d read     request 0x%08I64x blob 0x%08I64x autogen 0x%08I64x\n", i,
                Stats2.reads.request_bytes, Stats2.reads.blob_miss_bytes, Stats2.reads.autogen_miss_bytes);
            printf("pass 2 image %d write    request 0x%08I64x blob 0x%08I64x autogen 0x%08I64x\n", i,
                Stats2.writes.request_bytes, Stats2.writes.blob_miss_bytes, Stats2.writes.autogen_miss_bytes);
            printf("pass 2 image %d prefetch request 0x%08I64x blob 0x%08I64x autogen 0x%08I64x\n", i,
                Stats2.prefetches.request_bytes, Stats2.prefetches.blob_miss_bytes, Stats2.prefetches.autogen_miss_bytes);
            printf("pass 2 image %d clear    request 0x%08I64x blob 0x%08I64x autogen 0x%08I64x\n", i,
                Stats2.clears.request_bytes, Stats2.clears.blob_miss_bytes, Stats2.clears.autogen_miss_bytes);
            printf("pass 2 image %d flushed 0x%08I64x reflush 0x%08I64x\n", i,
                Stats2.flushed_bytes, Stats2.reflush_bytes);
			*/
        }

		ic.Deallocate();

		WIN_ASSERT_EQUAL( true, DeleteFile(g_reffilename1)!=0 );
	}

	WIN_ASSERT_EQUAL( true, DeleteFile(g_reffilename2)!=0 );
}
END_TEST_VTR

BEGIN_TEST_VTR_SLOW(VtiTest)
{
	CRand rand;

    int types[4] = {EL_FORMAT_BYTE, EL_FORMAT_SHORT, EL_FORMAT_FLOAT, EL_FORMAT_HALF_FLOAT};

	// max size pixel in test below
	Byte fillval[16];

	// load the source
	CRGBAImg src;
	LoadImage(GetImageDir() + L"newman_large.png", src);

    g_reffilename1[0] = g_reffilename2[0] = L'\0';

	// generate the test examples
	int iW = src.Width();
	int iH = src.Height();
	for ( UINT k = 0; k < 4; k++ )
	{
		CPyramid refs[g_nTestImages];
        vector<CLayerImgInfo> infos;
		CVtiReaderWriter vw;
		for ( UINT i = 0; i < g_nTestImages; i++ )
		{
			// randomly resize the source
			int iTW = (int)(rand.URand(0.5, 1.2)*(double(iW)));
			int iTH = (int)(rand.URand(0.5, 1.2)*(double(iH)));

			// pick a random pixel format 1,3,4 bands + byte, short, half_float, float
			int iB = rand.IRand(3)%3;
			iB = (iB==0)? 1: iB+2;

			int type = VT_IMG_MAKE_TYPE(types[rand.IRand(4)%4], iB);

			// create test image
			PYRAMID_PROPERTIES props;
			props.bTruncateOddLevels = false;
			props.bFloatPixelUseSupressRingingFilter = true;
			WIN_ASSERT_EQUAL(S_OK, refs[i].Create(iTW, iTH, type, &props) );
			WIN_ASSERT_EQUAL(S_OK, VtResizeImage(refs[i].GetLevel(0), vt::CRect(0,0,iTW,iTH), src, 
				                                 eSamplerKernelLanczos3));

			// set the ref pointer
			g_pReferenceResults[i] = refs+i;

            WIN_ASSERT_EQUAL(S_OK, infos.push_back(CLayerImgInfo(refs[i].GetImgInfo())) );
		}

        // create a VTI file
        WIN_ASSERT_EQUAL(S_OK, vw.CreateBlob(infos) );
		for ( int i = 0; i < g_nTestImages; i++ )
            for ( int l = 0; l < refs[i].GetNumLevels(); l++ )
                WIN_ASSERT_EQUAL(S_OK, vw.WriteImg(i, refs[i].GetLevel(l), l));

		// create a set of tasks that randomly read, prefetch
		CICTestTask testtasks(&vw);

		// only 1 thread to bang on VTI file
		VT_TASK_OPTIONS opts(1);

		// run the task on opts.maxthreads separate threads
		WIN_ASSERT_EQUAL(S_OK, vt::PushTaskAndWait(CICTestTask::TaskCallback, 
												   &testtasks, NULL, 256, &opts) );

		// randomly update sources
		for ( UINT i = 0; i < g_nTestImages; i++ )
		{
			if ( rand.DRand() > 0.6 )
			{
				// reset then rewrite some
				WIN_ASSERT_EQUAL( S_OK, vw.Fill(i, fillval) );

				WIN_ASSERT_EQUAL(S_OK, vw.WriteImg(i, refs[i].GetLevel(0)));
			} 
			else
			{
				// for others "fill" random rects in random levels of both VTI file 
				// and reference pyramid 
                UINT uLevels = refs[i].GetNumLevels();
				for ( UINT j = 0; j < 8; j++ )
				{
                    UINT uLevel = rand.IRand(uLevels)%uLevels;
    				CImgInfo info = vw.GetImgInfo(i, uLevel);
					vt::CRect r = RandRect(info.width/4, info.height/4, rand);
					RandPixelValue(fillval, info, rand);
					WIN_ASSERT_EQUAL(S_OK, vw.Fill(i, fillval, &r, uLevel));
					WIN_ASSERT_EQUAL(S_OK, refs[i].GetLevel(uLevel).Fill(fillval, &r) );
				}
			}
		}

		// run the task again
		WIN_ASSERT_EQUAL(S_OK, vt::PushTaskAndWait(CICTestTask::TaskCallback, 
												   &testtasks, NULL, 256, &opts) );

		vw.CloseBlob();
	}
}
END_TEST_VTR

class CICInit
{
public:
	CICInit()
	{
		m_bStarted = false; 
		if( ImageCacheStartup() == S_OK )
		{
			m_bStarted = true;
		}
	}
 
	~CICInit()
	{
        if( m_bStarted )
		{
			ImageCacheShutdown();
		}
	}

private:
	bool m_bStarted;
};


BEGIN_TEST_EX_SLOW(CacheLifetimeTest)
{
	// get the image cache backing thread started and test that the image
	// cache can shutdown cleanly - even with pending IOs

	CRand rand;

	for ( int i = 0; i < 100; i++ )
	{
        CICInit init;
				
		CImageCache* pIC = VtGetImageCacheInstance();

		WIN_ASSERT_NOT_NULL(pIC);

		// set the cache size small enough to cause disk backing
		WIN_ASSERT_EQUAL(S_OK, pIC->SetMaxMemoryUsage(4*1024*1024) );

		UINT sourceids[10];
		for ( UINT j = 0; j < 10; j++ )
		{
			vt::CRect r = RandRect(2048, 3333, rand);
            if( j==9 )
            {
                // give one of the backing blobs an invalid filename
                IMG_CACHE_SOURCE_PROPERTIES props;
                props.pBackingBlob = L"*>.bib";
                WIN_ASSERT_EQUAL(HRESULT(0x8007007b), pIC->AddSource(
                    CImgInfo(r.Width(), r.Height(), OBJ_BYTEIMG), 1,
                    sourceids[j], &props) );
            }
            else
            {
                WIN_ASSERT_EQUAL(S_OK, pIC->AddSource(
                    CImgInfo(r.Width(), r.Height(), OBJ_BYTEIMG), 1, sourceids[j]) );
            }
		}

		for ( UINT j = 0; j < 64; j++ )
		{
            int index = rand.IRand(10)%10;
			int source = sourceids[index];
			vt::CRect rfull = pIC->GetImgInfo(source).Rect();
			vt::CRect rsub = RandRect(rfull.Width(), rfull.Height(), rand);
			double v = rand.DRand();
			if ( v > 0.6 )
			{
				CByteImg junk;
				WIN_ASSERT_EQUAL(S_OK, junk.Create(rsub.Width(), rsub.Height()));
                if( index != 9 )
                {
                    WIN_ASSERT_EQUAL(S_OK, pIC->WriteRegion(source, 0, rsub, junk) );
                }
			} 
			else if ( v > 0.5 )
			{
				CByteImg junk;
				WIN_ASSERT_EQUAL(S_OK, junk.Create(rsub.Width(), rsub.Height()));
                if( index!=9)
                {
                    WIN_ASSERT_EQUAL(S_OK, pIC->ReadRegion(source, 0, rsub, junk) );
                }
			}
			else
			{
                if( index!=9)
                {
                    WIN_ASSERT_EQUAL(S_OK, pIC->Prefetch(source, 0, rsub));
                }
			}
		}
	}
}
END_TEST_EX

//+-----------------------------------------------------------------------------
//
// a simple example transform that works with the multi-core framework
//
//------------------------------------------------------------------------------
class CCreateRampTransform: 
public CImageTransformUnaryPoint<CCreateRampTransform, false>
{
public:
	CCreateRampTransform(float fScale) : m_fScale(fScale)
	{}

	HRESULT Transform(OUT CImg* pimgDstRegion, 
					  IN  const CRect& rctDst,
					  IN  CImg& imgBkgndRegion)
	{
		for ( int y = 0; y < rctDst.Height(); y++ )
		{
			for ( int x = 0; x < rctDst.Width(); x++)
			{
				// add background value with x*y/scale
				((CFloatImg&)*pimgDstRegion).Pix(x,y) = 
				((CFloatImg&)imgBkgndRegion).Pix(x,y) + 
				float((rctDst.left+x)*(rctDst.top+y)) * m_fScale;
			}
		}
		return S_OK;
	}

	HRESULT Clone(ITaskState** ppClone)
	{
		*ppClone = new CCreateRampTransform(m_fScale);
		return(*ppClone)? S_OK: E_OUTOFMEMORY;
	}

private:
	float m_fScale;
};

//+-----------------------------------------------------------------------------
//
// test VTI files
//
//------------------------------------------------------------------------------
BEGIN_TEST_VTR_FAST(CacheVTITest)
{
	const wchar_t filename[] = L"ramp.vti";

	// create a background value image
	CLumaFloatImgInCache icbkgnd;
	WIN_ASSERT_EQUAL(S_OK, icbkgnd.Create(2048, 4096) );
	WIN_ASSERT_EQUAL(S_OK, icbkgnd.Fill(0.125f) );

	int width  = icbkgnd.GetImgInfo().width;
	int height = icbkgnd.GetImgInfo().height;

	// instantiate our multi-core transform test 
	CCreateRampTransform x(1.f/float(width*height));

	// write
	{
		// create an image in cache specifying a filename, so that it is
		// persisted
		IMG_CACHE_SOURCE_PROPERTIES props;
		props.pBackingBlob = filename;

		CLumaFloatImgInCache icwr;
		WIN_ASSERT_EQUAL(S_OK, icwr.Create(width, height, &props) );

		// generate a ramp via multi-core transform
		CTransformGraphUnaryNode g(&x, &icbkgnd, &icwr);
		WIN_ASSERT_EQUAL(S_OK, PushTransformTaskAndWait(&g) );
	}

	// read 
	{
		// recreate the ramp
		CImgInCache ictest;
		WIN_ASSERT_EQUAL(S_OK, ictest.Create(icbkgnd.GetImgInfo()) );

		CTransformGraphUnaryNode g(&x, &icbkgnd, &ictest);
		WIN_ASSERT_EQUAL(S_OK, PushTransformTaskAndWait(&g) );

		// load the file into a CImg, build a pyramid, and compare
		CImg imgrd;
		WIN_ASSERT_EQUAL(S_OK, imgrd.Load(filename) );

		CPyramid pyr;
		PYRAMID_PROPERTIES props;
		props.bTruncateOddLevels = false;
		props.bFloatPixelUseSupressRingingFilter = true;
		WIN_ASSERT_EQUAL(S_OK, pyr.Create(imgrd, &props) );
	    for( int l = 0; l < (int)pyr.GetNumLevels(); l++ )
		{
			CLevelChangeReader rdl(&ictest, l);
			WIN_ASSERT_TRUE( VtCompareImages(&rdl, pyr.GetLevel(l)) );
		}

		// load the file into a CImgInCache and compare
		CImgInCache icrd;
		WIN_ASSERT_EQUAL(S_OK, icrd.Create(filename) );
	    for( int l = 0; l < (int)pyr.GetNumLevels(); l++ )
		{
			CLevelChangeReader rdlt(&ictest, l);
			CLevelChangeReader rdl(&icrd, l);
			WIN_ASSERT_TRUE( VtCompareImages(&rdlt, &rdl) );
		}
	}

	DeleteFile(filename);
}
END_TEST_VTR

#endif

//+-----------------------------------------------------------------------------
//
// a cache test that exercises the IImageReader source
//
//------------------------------------------------------------------------------


class CMergeTransform: public CImageTransformPoint<CMergeTransform, false>
{
public:
    CMergeTransform()
	{}

public:
	// hard-code this transform to only operate on single-band short images
	// as a source and single-band float images as dest
	virtual void    GetSrcPixFormat(IN OUT int* pfrmtSrcs, 
									IN UINT  /*uSrcCnt*/,
									IN int /*frmtDst*/)
	{ 
		pfrmtSrcs[0] = VT_IMG_FIXED(VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, 1));
		pfrmtSrcs[1] = VT_IMG_FIXED(VT_IMG_MAKE_TYPE(EL_FORMAT_SHORT, 1));
    }

	virtual void GetDstPixFormat(OUT int& frmtDst,
								 IN  const int* /*pfrmtSrcs*/, 
								 IN  UINT  /*uSrcCnt*/)
	{ 
		frmtDst = VT_IMG_FIXED(VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, 1)); 
	}

	virtual HRESULT Transform(OUT CImg* pDst, 
							  IN  const CRect& /*rctLayerDst*/,
							  IN  CImg *const *ppSrc,
							  IN  UINT  /*uSrcCnt*/
							  )
	{ 
		MergeSource((CFloatImg&)*pDst, (CFloatImg&)*ppSrc[0], (CShortImg&)*ppSrc[1]);
		return S_OK;  
	}

	virtual HRESULT Clone(ITaskState **ppState)
	{ return CloneTaskState(ppState, VT_NOTHROWNEW CMergeTransform()); }
};

HRESULT ProcessViaTransform(CFloatImgInCache& merged, IIndexedImageReader* pSources)
{
	VT_HR_BEGIN()

	CImgInfo info0 = pSources->GetImgInfo(0);

	VT_HR_EXIT( merged.Create(info0.width, info0.height, 1) );

	float fill = 0.f;
	merged.Fill(&fill);

	CMergeTransform x;
	CTransformGraphNaryNode g(&x, &merged);
	VT_HR_EXIT( g.SetSourceCount(2) );
	g.BindSourceToReader(0, &merged);
	
	for( int i = 0; i < (int)pSources->GetFrameCount(); i++ )
	{  
		CImgInfo info = pSources->GetImgInfo(i);
		VT_ASSERT( info0.width == info.width && info0.height == info.height );

		CIndexedImageReaderIterator rd(pSources, i);  
		g.BindSourceToReader(1, &rd);

		VT_TRANSFORM_TASK_OPTIONS opts;
		opts.maxthreads = 1;
		VT_HR_EXIT( PushTransformTaskAndWait(&g, (CTaskStatus*)NULL, &opts) );
	}

	VT_HR_END()
}

class CMergeImgReader: public IImageReader
{
public:
	CMergeImgReader(IIndexedImageReader* pSources) : m_pSrc(pSources), m_bWarn(false)
	{}

public:
	// IImageReader
	virtual CLayerImgInfo GetImgInfo(UINT uLevel = 0)
	{ 
		if( !m_pSrc )
		{
			return CLayerImgInfo();
		}
		CLayerImgInfo info = m_pSrc->GetImgInfo(0, uLevel);
		info.type = VT_IMG_MAKE_TYPE(EL_FORMAT_FLOAT, 1);
		return info; 
	}

	virtual HRESULT GetMetaData(CParams &/*params*/)
    { return S_OK; }

	virtual HRESULT ReadImg(OUT CImg& dst, UINT uLevel=0)
	{ return m_pSrc? ReadRegion(m_pSrc->GetImgInfo(0, uLevel).Rect(), dst, uLevel): E_NOINIT; }

	virtual HRESULT  ReadRegion(IN const CRect& r, OUT CImg& dst, 
								UINT /*uLevel = 0*/)
	{ 
		if( !m_pSrc )
		{
			return E_NOINIT;
		}
		
		VT_HR_BEGIN() 

		if( m_bWarn )
		{
			VT_HR_EXIT( E_UNEXPECTED );
		}
		
		VT_HR_EXIT( CreateImageForTransform(dst, r.Width(), r.Height(), 
											EL_FORMAT_FLOAT) );

		CFloatImg imgf;
		if( EL_FORMAT(dst.GetType()) == EL_FORMAT_FLOAT && dst.Bands() == 1 )
		{
			dst.Share(imgf);
		}
		else
		{
			VT_HR_EXIT( imgf.Create(r.Width(), r.Height()) );
		}

		imgf.Fill(0.0f);//std::numeric_limits<float>::quiet_NaN());
		CShortImg src;
		for( int i = 0; i < (int)m_pSrc->GetFrameCount(); i++ )
		{
			VT_HR_EXIT( m_pSrc->ReadRegion(i, r, src) );
			MergeSource(imgf, imgf, src);
		}

		if( EL_FORMAT(dst.GetType()) != EL_FORMAT_FLOAT || dst.Bands() != 1 )
		{
			VT_HR_EXIT( VtConvertImage(dst, imgf) );
		}

		VT_HR_END()  	
	}

	virtual HRESULT Prefetch(IN const CRect& region, UINT uLevel = 0)
	{ 
		if( !m_pSrc )
		{
			return E_NOINIT;
		}
		for( int i = 0; i < (int)m_pSrc->GetFrameCount(); i++ )
		{
			m_pSrc->Prefetch(i, region, uLevel);
		}
		return S_OK;
	}

public:
	void SetWarn(bool v)
	{ m_bWarn = v; }

protected:
	IIndexedImageReader* m_pSrc;
	bool m_bWarn;
};

HRESULT ProcessViaReader(UINT& uSrcId, CMergeImgReader& mrd)
{
	VT_HR_BEGIN() 

	CImgInfo info = mrd.GetImgInfo();

    CImageCache* pIC = VtGetImageCacheInstance();
	VT_HR_EXIT( pIC->AddSource(&mrd, uSrcId, true) );

	// read through the cache - this should generate a merged image
	for( CBlockIterator bi(info.Rect(), 256); !bi.Done(); bi.Advance() )
	{
		CFloatImg imgBlk;
		VT_HR_EXIT( pIC->ReadRegion(uSrcId, 0, bi.GetRect(), imgBlk) );
	}

	VT_HR_END()
}

void GenMergeTest(CShortImg& imgGen, int i)
{
	CShortImg imgGenSrc;
	imgGenSrc.Create(imgGen.Width(), imgGen.Height());
	for( int y = 0; y < imgGenSrc.Height(); y++ )
	{
		for( int x = 0; x < imgGenSrc.Width(); x++ )
		{
			imgGenSrc.Pix(x, y) = UInt16(float(0xffff)*sin(float(2.*VT_PI)*float(x)/float(imgGen.Width())));
		}
	}

	CMtx3x3f rot = 
		CMtx3x3f().MakeTranslate(float(imgGen.Width()/2), float(imgGen.Height()/2)) *
		CMtx3x3f().MakeRotation(float(VT_PI)*float(i)/6.f) *
		CMtx3x3f().MakeTranslate(-float(imgGen.Width()/2), -float(imgGen.Height()/2));
	
	VtWarpImage(imgGen, imgGen.Rect(), imgGenSrc, rot, eSamplerKernelBilinear, 
				IMAGE_EXTEND(Wrap));
}


BEGIN_TEST_VTR_FAST(CacheReaderSource)
{
	CImgListInCache test_sources;
	for( int i = 0; i < 3; i++ )
	{
		CShortImg imgGen;
		imgGen.Create(1024, 768);
		GenMergeTest(imgGen, i);
		WIN_ASSERT_EQUAL(S_OK, test_sources.PushBack(imgGen));
	}

	CFloatImgInCache merged;
	WIN_ASSERT_EQUAL(S_OK, ProcessViaTransform(merged, &test_sources) );

	CMergeImgReader mrd(&test_sources);
	UINT uSrcId;
	WIN_ASSERT_EQUAL(S_OK, ProcessViaReader(uSrcId, mrd) );

	// read through again and set the flag to print on reads
	CImageCache* pIC = VtGetImageCacheInstance();
	mrd.SetWarn(true);

	// copy reader result to another imgincache
	CImgInCache rdres;
	WIN_ASSERT_EQUAL(S_OK, rdres.Create(mrd.GetImgInfo()) );

	CFloatImg imgres;
	WIN_ASSERT_EQUAL(S_OK, pIC->ReadImg(uSrcId, 0, imgres, 0));
	WIN_ASSERT_EQUAL(S_OK, rdres.WriteImg(imgres));

	// read every level and assert that they match
	UINT nLev = ComputePyramidLevelCount(1024, 768);
	for( UINT l = 0; l < nLev; l++ )
	{
		CFloatImg img1, img2;
		WIN_ASSERT_EQUAL(S_OK, merged.ReadImg(img1, l));

		WIN_ASSERT_EQUAL(S_OK, rdres.ReadImg(img2, l));

		WIN_ASSERT_TRUE(VtCompareImages(img1, img2));
	}

	pIC->DeleteSource(uSrcId);
}
END_TEST_VTR

END_TEST_FILE