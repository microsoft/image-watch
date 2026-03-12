////////////////////////////////////////////////////////////////////////////////
// converttest.cpp : Unit tests for VisionTools core
//
#include "stdafx.h"
#include "cthelpers.h"

using namespace vt;

BEGIN_TEST_FILE(ConvertTests)

////////////////////////////////////////////////////////////////////////////////
//
// basic tests of pixel format conversions
//
template <class T>
bool CheckAlphaOpaque(const CCompositeImg<T>& src)
{
	for (int y = 0; y < src.Height(); y++)
	{
		const T* p = src.Ptr(y);
		for (int x = 0; x < src.Width(); x++, p++)
		{
			if (!p->IsOpaque()) return false;
		}
	}
	return true;
}

bool CheckAlphaOpaque(const CImg& src)
{
	switch (EL_FORMAT(src.GetType()))
	{
	case EL_FORMAT_BYTE:
		return CheckAlphaOpaque((CRGBAImg&)src);
		break;
	case EL_FORMAT_HALF_FLOAT:
		return CheckAlphaOpaque((CRGBAHalfFloatImg&)src);
		break;
	case EL_FORMAT_SHORT:
		return CheckAlphaOpaque((CRGBAShortImg&)src);
		break;
	case EL_FORMAT_FLOAT:
		return CheckAlphaOpaque((CRGBAFloatImg&)src);
		break;
	}
	return false;
}

BEGIN_TEST_VTR_FAST(ConvertTest)
{
	// load the source
	CRGBImg src8;
	LoadImage(GetImageDir() + L"newman.png", src8);

	// test conversion between formats
	int elformat[4] = { EL_FORMAT_BYTE, EL_FORMAT_HALF_FLOAT,
		EL_FORMAT_SHORT, EL_FORMAT_FLOAT };
	int tol[4] = { 0xff, 0x3ff, 0xffff, 0xffff };

	for (UINT i = 0; i < 4; i++)
	{
        CImg imgRef, imgTest, imgJ;
        int typei = VT_IMG_MAKE_TYPE(elformat[i], src8.Bands());
		WIN_ASSERT_EQUAL(S_OK, imgRef.Create(
			src8.Width(), src8.Height(), typei));
		WIN_ASSERT_EQUAL(S_OK, imgTest.Create(imgRef.GetImgInfo()));

		// convert source to current bitdepth
		WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgRef, src8));

		// allow 1 pixel value difference in current format depth
		float fTol = 1.0001f / float(tol[i]);

		// convert to higher bit-depth format and back
		for (UINT j = i; j < 4; j++)
		{
			int typej = VT_IMG_MAKE_TYPE(elformat[j], src8.Bands());
			WIN_ASSERT_EQUAL(S_OK, imgJ.Create(
				src8.Width(), src8.Height(), typej));

			WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgJ, imgRef));
			WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgTest, imgJ));

			WIN_ASSERT_TRUE(VtCompareImages(imgRef, imgTest, 1, NULL, (j == i) ? 0 : fTol));
		}
	}

	// TODO: determine maxval stuff for HALF_FLOAT

	// test 3band to 4band and 4band to 3band
	for (UINT i = 0; i < 4; i++)
	{
		CImg imgRamp, imgTest, imgJ4;
		int type3 = VT_IMG_MAKE_TYPE(elformat[i], 3);
		WIN_ASSERT_EQUAL(S_OK, imgRamp.Create(256, 256, type3));
		WIN_ASSERT_EQUAL(S_OK, imgTest.Create(imgRamp.GetImgInfo()));

		// generate a ramp in the current bitdepth
		GenerateRamp(imgRamp);

		// allow 1 pixel value difference in current format depth
		float fTol = 1.0001f / float(tol[i]);

		for (UINT j = i; j < 4; j++)
		{
			// convert to 4 banded higher bitdepth
			int type4 = VT_IMG_MAKE_TYPE(elformat[j], 4);
			WIN_ASSERT_EQUAL(S_OK, imgJ4.Create(imgRamp.Width(), imgRamp.Height(), type4));

			WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgJ4, imgRamp));
			WIN_ASSERT_TRUE(CheckAlphaOpaque(imgJ4));
			WIN_ASSERT_EQUAL(S_OK, VtConvertImage(imgTest, imgJ4));

			WIN_ASSERT_TRUE(VtCompareImages(imgRamp, imgTest, 1, NULL,
				(j == i) ? 0 : fTol));
		}
	}

	// test color-swap
	for (UINT k = 0; k < 2; k++)
	{
		int bands = k ? 3 : 4;
		int pixfrmt = k ? PIX_FORMAT_RGB : PIX_FORMAT_RGBA;

		for (UINT i = 0; i < 4; i++)
		{
			int typei = VT_IMG_MAKE_COMP_TYPE(pixfrmt, elformat[i], bands);

			CImg imgRamp, imgTest, imgJ;
			WIN_ASSERT_EQUAL(S_OK, imgRamp.Create(256, 256, typei));
			WIN_ASSERT_EQUAL(S_OK, imgTest.Create(imgRamp.GetImgInfo()));

			// generate a ramp in the current bitdepth
			GenerateRamp(imgRamp);

			// allow 1 pixel value difference in current format depth
			float fTol = 1.0001f / float(tol[i]);

			for (UINT j = i; j < 4; j++)
			{
				int typej = VT_IMG_MAKE_COMP_TYPE(pixfrmt, elformat[j], bands);

				// convert to swapped colors at higher depth
				WIN_ASSERT_EQUAL(S_OK, imgJ.Create(
					imgRamp.Width(), imgRamp.Height(), typej));
				VtRGBColorSwapImage(imgJ, imgRamp);
				VtRGBColorSwapImage(imgTest, imgJ);

				WIN_ASSERT_TRUE(VtCompareImages(imgRamp, imgTest, 1, NULL,
					(j == i) ? 0 : fTol));

				// for matching pixel types also check in-place convert
				if (i == j)
				{
					VtRGBColorSwapImage(imgTest, imgRamp);
					VtRGBColorSwapImage(imgTest);

					WIN_ASSERT_TRUE(VtCompareImages(imgRamp, imgTest, 1, NULL,
						(j == i) ? 0 : fTol));
				}
			}
		}
	}

	// TODO: test negative values
}
END_TEST_VTR

BEGIN_TEST_EX_FAST(ConvertToHSBTest)
{
	CRGBAImg in;
	LoadImage(GetImageDir() + L"newman_small.png", in);

	// convert from byte RGBA
	CFloatImg hsb;
	WIN_ASSERT_TRUE(VtConvertImageRGBToHSB(hsb, in) == S_OK);
	CRGBAImg test(in.Width(), in.Height());
	WIN_ASSERT_TRUE(VtConvertImageHSBToRGB(test, hsb) == S_OK);

	WIN_ASSERT_TRUE(GetAverageAbsoluteDifference(in, test) < 3e-2f);

	// convert from short RGB
	CRGBShortImg inRGBShort;
	WIN_ASSERT_TRUE(VtConvertImage(inRGBShort, in) == S_OK);
	CFloatImg hsbShort;
	WIN_ASSERT_TRUE(VtConvertImageRGBToHSB(hsbShort, inRGBShort) == S_OK);

	CRGBShortImg testShort;
	WIN_ASSERT_TRUE(VtConvertImageHSBToRGB(testShort, hsbShort) == S_OK);

	CFloatImg a, b;
	// VtSADImages doesnt support short
	WIN_ASSERT_TRUE(VtConvertImage(a, inRGBShort) == S_OK);
	WIN_ASSERT_TRUE(VtConvertImage(b, testShort) == S_OK);
	WIN_ASSERT_TRUE(GetAverageAbsoluteDifference(a, b) < 4e-5f);
}
END_TEST_EX

BEGIN_TEST_EX_FAST(ModifyHSBTest)
{
	CRGBAImg in;
	LoadImage(GetImageDir() + L"lena512color.bmp", in);

	CRGBAFloatImg out;
	WIN_ASSERT_TRUE(VtRGBModifyHSB(out, in, 50.0f, 0.9f, 1.1f) == S_OK);
	CRGBAImg test;
	WIN_ASSERT_TRUE(VtRGBModifyHSB(test, out, -50.0, 1.0f / 0.9f, 1.0f / 1.1f) == S_OK);

	WIN_ASSERT_TRUE(GetAverageAbsoluteDifference(in, test) < 1.0f);
}
END_TEST_EX

BEGIN_TEST_EX_FAST(ConvertBandsTest)
{
	// test case 1
	{
		CRGBAImg in;
		LoadImage(GetImageDir() + L"newman_small.png", in);

		CRGBImg tmp;
		BandIndexType bi[] = { BandIndex1, BandIndex2, BandIndex0 };
		WIN_ASSERT_TRUE(VtConvertBands(tmp, in, sizeof(bi) / sizeof(bi[0]),
			bi) == S_OK);

		BandIndexType biinv[] = { BandIndex2, BandIndex0, BandIndex1,
			BandIndexFill };
		Byte fv[4] = { 0, 0, 0, 255 };
		CRGBAImg test;
		WIN_ASSERT_TRUE(VtConvertBands(test, tmp,
			sizeof(biinv) / sizeof(biinv[0]), biinv, fv) == S_OK);

		WIN_ASSERT_TRUE(GetAverageAbsoluteDifference(in, test) == 0.0f);
	}

	// test case 2
	{
		CRGBFloatImg in;

		LoadImage(GetImageDir() + L"burns.jpg", in);

		CShortImg inShort(in.Width(), in.Height(), 4);
		WIN_ASSERT_TRUE(VtConvertImage(inShort, in) == S_OK);
		WIN_ASSERT_TRUE(inShort.IsValid());

		CHalfFloatImg tmp;
		BandIndexType bi[] = { BandIndex0, BandIndex0, BandIndexIgnore,
			BandIndex2, BandIndex1 };
		WIN_ASSERT_TRUE(VtConvertBands(tmp, inShort, sizeof(bi) / sizeof(bi[0]),
			bi) == S_OK);

		BandIndexType biinv[] = { BandIndex1, BandIndex4, BandIndex3 };
		CRGBFloatImg test;
		WIN_ASSERT_TRUE(VtConvertBands(test, tmp,
			sizeof(biinv) / sizeof(biinv[0]), biinv) == S_OK);

		WIN_ASSERT_TRUE(GetAverageAbsoluteDifference(in, test) < 1e-3f);
	}
}
END_TEST_EX

BEGIN_TEST_EX_FAST(ConvertToXXX)
{
	CImg img;
	CLumaImg lum;
	CRGBAImg in;
	LoadImage(GetImageDir() + L"newman_small.png", in);

	// convert from byte RGBA to byte luminance
	WIN_ASSERT_TRUE(VtConvertImageToLuma(img, in) == S_OK);
	WIN_ASSERT_TRUE(img.GetType() == OBJ_LUMAIMG);

	// convert from byte luminance to byte RGB
	WIN_ASSERT_TRUE(VtConvertImageToRGB(img, in) == S_OK);
	WIN_ASSERT_TRUE(img.GetType() == OBJ_RGBIMG);

	// convert from byte RGB to byte RGBA
	WIN_ASSERT_TRUE(VtConvertImageToRGBA(img, in) == S_OK);
	WIN_ASSERT_TRUE(img.GetType() == OBJ_RGBAIMG);

	// convert from byte RGBA to byte luminance
	WIN_ASSERT_TRUE(VtConvertImageToLuma(img, in) == S_OK);
	WIN_ASSERT_TRUE(img.GetType() == OBJ_LUMAIMG);

	// compare against luminance image
	WIN_ASSERT_TRUE(VtConvertImage(lum, in) == S_OK);
	WIN_ASSERT_TRUE(GetAverageAbsoluteDifference(lum, img) == 0);
}
END_TEST_EX

END_TEST_FILE