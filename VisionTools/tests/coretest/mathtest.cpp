////////////////////////////////////////////////////////////////////////////////
// mathtest.cpp : Unit tests for VisionTools functions from vt_imgmath.h
//

#include "stdafx.h"

#include "cthelpers.h"

using namespace vt;

// "conditial expression constant"
#pragma warning ( disable : 4127 )

template<typename T, typename U>
bool IsColorMatch(RGBAType<T> a, RGBAType<U> b, RGBAFloatPix scale, 
	RGBAFloatPix offset, float tol)
{
	float v1, v2;

	VtConv(&v1, T(a.r*scale.r+offset.r));
	VtConv(&v2, b.r);
	if (fabs((float)(v1 - v2)) > tol)
		return false;
	VtConv(&v1, T(a.g*scale.g+offset.g));
	VtConv(&v2, b.g);
	if (fabs((float)(v1 - v2)) > tol)
		return false;
	VtConv(&v1, T(a.b*scale.b+offset.b));
	VtConv(&v2, b.b);
	if (fabs((float)(v1 - v2)) > tol)
		return false;
	VtConv(&v1, T(a.a*scale.a+offset.a));
	VtConv(&v2, b.a);
	if (fabs((float)(v1 - v2)) > tol)
    {
		return false;
    }

	return true;
}

template<typename TI, typename TO>
void LogTestFunc(const CTypedImg<TI>& in, const CTypedImg<TO>& out, float fMin)
{
	for (int y = 0; y < in.Height(); y += 7)
	{
		for (int x = 0; x < in.Width(); x += 7)
		{
			float vin = in(x, y);
			float vout = (vin > 0.f ? logf(vin) : fMin) *
				float(ElTraits<TO>::MaxVal()) / float(ElTraits<TI>::MaxVal());

			if (!(ElTraits<TO>::ElFormat() == EL_FORMAT_FLOAT ||
				ElTraits<TO>::ElFormat() == EL_FORMAT_HALF_FLOAT))
			{
				TO vint;
				VtClip(&vint, vout);
				WIN_ASSERT_TRUE(fabs(float(vint - out(x, y))) < 1.0);
			}
			else
				WIN_ASSERT_TRUE(fabs(vout - out(x, y)) < 1e-3);
		}
	}
}

template<typename TI, typename TO>
void ExpTestFunc(const CTypedImg<TI>& in, const CTypedImg<TO>& out)
{
	for (int y = 0; y < in.Height(); y += 7)
	{
		for (int x = 0; x < in.Width(); x += 7)
		{
			float vin = in(x, y);
			float vout = expf(vin) * float(ElTraits<TO>::MaxVal()) /
				float(ElTraits<TI>::MaxVal());

			if (!(ElTraits<TO>::ElFormat() == EL_FORMAT_FLOAT ||
				ElTraits<TO>::ElFormat() == EL_FORMAT_HALF_FLOAT))
			{
				TO vint;
				VtClip(&vint, vout);
				WIN_ASSERT_TRUE(fabs(float(vint - out(x, y))) < 1.0);
			}
			else
			{
				//printf_s("%f %f\n", vout, out(x,y));
				WIN_ASSERT_TRUE((IsInf(vout) && IsInf(out(x, y))) ||
					fabs(vout - out(x, y)) / vout < 1e-3);
			}
		}
	}
}

template<typename TI, typename TO>
void GammaTestFunc(const CTypedImg<TI>& in, const CTypedImg<TO>& out, float power)
{
	for (int y = 0; y < in.Height(); y += 7)
	{
		for (int x = 0; x < in.Width(); x += 7)
		{
			float vin = in(x, y) / float(ElTraits<TI>::MaxVal());
			float vout = powf(vin, power) * float(ElTraits<TO>::MaxVal());

			if (!(ElTraits<TO>::ElFormat() == EL_FORMAT_FLOAT ||
				ElTraits<TO>::ElFormat() == EL_FORMAT_HALF_FLOAT))
			{
				TO vint;
				VtClip(&vint, vout);
				WIN_ASSERT_TRUE(fabs(float(vint - out(x, y))) < 1.0);
			}
			else
			{
				//printf_s("%f %f\n", vout, out(x,y));
				WIN_ASSERT_TRUE((IsInf(vout) && IsInf(out(x, y))) ||
					fabs(vout - out(x, y)) / (vout + 1e-6) < 1e-3);
			}
		}
	}
}

template<typename TI, typename TO>
void GammaColorTestFunc(const CTypedImg<TI>& in, const CTypedImg<TO>& out, float power)
{
	for (int y = 0; y < in.Height(); y += 7)
	{
		for (int x = 0; x < in.Width(); x += 7)
		{
			for (int c = 0; c < 3; ++c)
			{
				float vin = in(x, y, c) / float(ElTraits<TI>::MaxVal());
				float vout = powf(vin, power) * float(ElTraits<TO>::MaxVal());

				if (!(ElTraits<TO>::ElFormat() == EL_FORMAT_FLOAT ||
					ElTraits<TO>::ElFormat() == EL_FORMAT_HALF_FLOAT))
				{
					TO vint;
					VtClip(&vint, vout);
					WIN_ASSERT_TRUE(fabs(float(vint - out(x, y, c))) < 1.0);
				}
				else
				{
					//printf_s("%f %f\n", vout, out(x,y));
					WIN_ASSERT_TRUE((IsInf(vout) && IsInf(out(x, y, c))) ||
						fabs(vout - out(x, y, c)) / (vout + 1e-6) < 1e-3);
				}
			}

			if (in.Bands() == 4)
				WIN_ASSERT_TRUE(fabs(in(x, y, 3) - float(ElTraits<TI>::MaxVal())) /
				float(ElTraits<TI>::MaxVal()) < 1e-3);

			if (out.Bands() == 4)
				WIN_ASSERT_TRUE(fabs(out(x, y, 3) - float(ElTraits<TO>::MaxVal())) /
				float(ElTraits<TO>::MaxVal()) < 1e-3);
		}
	}
}

template<typename TI, typename TO>
void BlendTestFunc(const CTypedImg<TI>& in1, const CTypedImg<TI>& in2,
	const CTypedImg<TO>& out, float w1)
{
	for (int y = 0; y < in1.Height(); y++)
	{
		for (int x = 0; x < in1.Width(); x++)
		{
			float vin = (w1*(float)in1(x, y) + (1.f - w1)*(float)in2(x, y)) /
				float(ElTraits<TI>::MaxVal());
			float vout = vin * float(ElTraits<TO>::MaxVal());

			if (!(ElTraits<TO>::ElFormat() == EL_FORMAT_FLOAT ||
				ElTraits<TO>::ElFormat() == EL_FORMAT_HALF_FLOAT))
			{
				TO voutint;
				VtClip(&voutint, vout);
				//printf_s("(%d,%d) %d %d\n", x,y, voutint, out(x,y));
				WIN_ASSERT_TRUE(fabs(float(voutint - out(x, y))) <= 1.0);
			}
			else
			{
				//printf_s("(%d,%d) %f %f\n", x,y, vout, out(x,y));
				WIN_ASSERT_TRUE((IsInf(vout) && IsInf(out(x, y))) ||
					fabs(vout - out(x, y)) / (vout + 1e-6) < 1e-3);
			}
		}
	}
}

BEGIN_TEST_FILE(MathTest)

BEGIN_TEST_EX_FAST(AddTest)
{
	CRGBFloatImg in;
	LoadImage(GetImageDir() + L"newman_small.png", in);	
	
	// test case 1: RGB float
	{
		CRGBHalfFloatImg tmp1, tmp2;
		WIN_ASSERT_EQUAL(VtConvertImage(tmp1, in), S_OK);
		WIN_ASSERT_EQUAL(VtConvertImage(tmp2, in), S_OK);

		CRGBFloatImg res;
		WIN_ASSERT_EQUAL(VtAddImages(res, tmp1, tmp2), S_OK);

		CRGBFloatImg tst(res.Width(), res.Height());
		WIN_ASSERT_EQUAL(VtScaleImage(tst, res, 0.5f), S_OK);

		WIN_ASSERT_TRUE(GetAverageAbsoluteDifference(in, tst) < 1e-3);
	}

	// test case 2: gray byte images, pure CImg, ..
	{
		CLumaImg ing;
		WIN_ASSERT_EQUAL(VtConvertImage(ing, in), S_OK);

		CLumaImg in1;
		WIN_ASSERT_EQUAL(VtScaleImage(in1, ing, 0.5f), S_OK);
		CImg in2;
		WIN_ASSERT_EQUAL(in1.CopyTo(in2), S_OK);

		CImg res;
		WIN_ASSERT_EQUAL(VtAddImages(res, in1, in2), S_OK);

		WIN_ASSERT_TRUE(GetAverageAbsoluteDifference(ing, res) < 2.0f);
	}
}
END_TEST_EX;

BEGIN_TEST_EX_FAST(SubTest)
{
	CRGBFloatImg in;
	LoadImage(GetImageDir() + L"newman_small.png", in);	

	// test case 1: RGB float
	{
		CRGBFloatImg tmp;
		WIN_ASSERT_EQUAL(VtScaleImage(tmp, in, 2.0), S_OK);

		CRGBFloatImg res;
		WIN_ASSERT_EQUAL(VtSubImages(res, tmp, in), S_OK);

		WIN_ASSERT_TRUE(GetAverageAbsoluteDifference(in, res) < 1e-6);
	}

	// test case 2: luma byte images, pure CImg, ..
	{
		CLumaImg ing;
		WIN_ASSERT_EQUAL(VtConvertImage(ing, in), S_OK);

		CLumaImg in1;
		WIN_ASSERT_EQUAL(VtScaleImage(in1, ing, 0.5f), S_OK);

		CImg res;
		WIN_ASSERT_EQUAL(VtSubImages(res, ing, in1), S_OK);

		WIN_ASSERT_TRUE(GetAverageAbsoluteDifference(in1, res) < 1.0f);
	}
}
END_TEST_EX;

BEGIN_TEST_EX_FAST(MulTest)
{
	CRGBFloatImg in;
	LoadImage(GetImageDir() + L"newman_small.png", in);	

	// test case 1: RGB float
	{
		CRGBFloatImg f(in.Width(), in.Height());
		f.Fill(RGBFloatPix(0.33f, 0.33f, 0.33f));

		CRGBFloatImg tmp;
		WIN_ASSERT_EQUAL(VtMulImages(tmp, in, f), S_OK);

		CRGBFloatImg res;
		WIN_ASSERT_EQUAL(VtAddImages(res, tmp, tmp), S_OK);
		WIN_ASSERT_EQUAL(VtAddImages(res, tmp, res), S_OK);

		WIN_ASSERT_TRUE(GetAverageAbsoluteDifference(in, res) < 1e-2);
	}
}
END_TEST_EX;

BEGIN_TEST_EX_FAST(ScaleTest)
{
	struct ScaleTest
	{
		void operator()(const CImg& in, const CImg&, CImg& out)
		{
			VtScaleImage(out, in, 0.33f);

			CLumaFloatImg tst1, tst2;
			VtConvertImage(tst1, in);
			VtConvertImage(tst2, out);

			for (int y = 0; y < tst2.Height(); y += 7)
			{			
				for (int x = 0; x < tst2.Width(); x += 7)
				{
					const float v1 = 0.33f*tst1(x,y);
					const float v2 = tst2(x,y);
					WIN_ASSERT_TRUE(fabs((float)(v1 - v2)) < 1.0/255);
				}
			}
		}
	};

	TestFormats(ScaleTest());
}
END_TEST_EX;

BEGIN_TEST_EX_FAST(ScaleOffsetTest)
{
	struct ScaleOffsetTest
	{
		void operator()(const CImg& in, const CImg&, CImg& out)
		{
			float typed_offset = 0.5f;
			switch (EL_FORMAT(in.GetType()))
			{
			case EL_FORMAT_BYTE:
				typed_offset *= 0xff;
				break;
			case EL_FORMAT_SHORT:
				typed_offset *= 0xffff;
				break;
			}

			VtScaleOffsetImage(out, in, 0.33f, typed_offset);

			CLumaFloatImg tst1, tst2;
			VtConvertImage(tst1, in);
			VtConvertImage(tst2, out);

			for (int y = 0; y < tst2.Height(); y += 7)
			{			
				for (int x = 0; x < tst2.Width(); x += 7)
				{
					const float v1 = 0.33f*tst1(x,y) + 0.5f;
					const float v2 = tst2(x,y);
					WIN_ASSERT_TRUE(fabs((float)(v1 - v2)) < 1.0/255);
				}
			}
		}
	};

	TestFormats(ScaleOffsetTest());
}
END_TEST_EX;


BEGIN_TEST_EX_FAST(ScaleColorTest)
{
	TestFormats([](const CImg& in, const CImg&, CImg& out)
	{
		if (!IsColorImage(in) || !IsColorImage(out))
			return;

		RGBAFloatPix s(0.5f, 0.4f, 0.3f, 1.f);

		VtScaleColorImage(out, in, s);

		CRGBAFloatImg atst;
		CRGBAFloatImg btst;
		VtConvertImage(atst, in);		
		VtConvertImage(btst, out);		

		for (int y = 0; y < in.Height(); y += 7)
		{			
			for (int x = 0; x < in.Width(); x += 7)
			{
				WIN_ASSERT_TRUE(IsColorMatch(atst(x,y), btst(x,y), s, 
					RGBAFloatPix(0.0, 0.0, 0.0, 0.0), 1.f/255));
			}
		}
	});
}
END_TEST_EX;

BEGIN_TEST_EX_FAST(ScaleOffsetColorTest)
{
	TestFormats([](const CImg& in, const CImg&, CImg& out)
	{
		if (!IsColorImage(in) || !IsColorImage(out))
			return;

		float typed_offset = 0.5f;
		switch (EL_FORMAT(in.GetType()))
		{
		case EL_FORMAT_BYTE:
			typed_offset *= 0xff;
			break;
		case EL_FORMAT_SHORT:
			typed_offset *= 0xffff;
			break;
		}

		RGBAFloatPix s(0.5f, 0.4f, 0.3f, 1.f);
		RGBAFloatPix o(0, typed_offset, 0.0, 0.0);

		VtScaleOffsetColorImage(out, in, s, o);

		CRGBAFloatImg atst;
		CRGBAFloatImg btst;
		VtConvertImage(atst, in);		
		VtConvertImage(btst, out);		

		for (int y = 0; y < in.Height(); y += 7)
		{			
			for (int x = 0; x < in.Width(); x += 7)
			{
				WIN_ASSERT_TRUE(IsColorMatch(atst(x,y), btst(x,y), s, 
					RGBAFloatPix(0, 0.5f, 0.0, 0.0), 1.f/255));
			}
		}
	});
}
END_TEST_EX;

BEGIN_TEST_EX_FAST(MultiplyAlphaTest)
{
	TestFormats([](const CImg& in, const CImg&, CImg& out)
	{
		if (in.Bands() != 4  || out.Bands() != 4)
			return;

		float scale = 0.5f;
		switch (EL_FORMAT(in.GetType()))
		{
		case EL_FORMAT_BYTE:
			scale = float(0x80)/float(0xff);
			break;
		case EL_FORMAT_SHORT:
			scale = float(0x8000)/float(0xffff);
			break;
		}

		CImg a;
		VtScaleColorImage(a, in, RGBAFloatPix(1, 1, 1, scale));

		WIN_ASSERT_EQUAL(VtMultiplyAlpha(out, a), S_OK);

		CRGBAFloatImg atst;
		CRGBAFloatImg btst;
		VtConvertImage(atst, a);		
		VtConvertImage(btst, out);		

		for (int y = 0; y < in.Height(); y++)
		{			
			for (int x = 0; x < in.Width(); x++)
			{
				WIN_ASSERT_TRUE(IsColorMatch(atst(x,y), btst(x,y), 
					RGBAFloatPix(scale, scale, scale, 1.0), 
					RGBAFloatPix(0, 0.0, 0.0, 0.0), 1.f/255));
			}
		}
	});
}
END_TEST_EX;

BEGIN_TEST_EX_FAST(LogTest)
{
	TestFormats([](const CImg& in, const CImg&, CImg& out)
	{
		if (in.Bands() > 1 || out.Bands() > 1)
			return;

		/*if (EL_FORMAT(in.GetType()) == EL_FORMAT_BYTE &&
		EL_FORMAT(out.GetType()) == EL_FORMAT_BYTE)
		int stophere = 1;*/

		float fMin = 1e-3f;
		WIN_ASSERT_EQUAL(VtLogImage(out, in, fMin), S_OK);

		switch (EL_FORMAT(in.GetType()))
		{
		case EL_FORMAT_FLOAT:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE:
				LogTestFunc((CTypedImg<float>&) in, (CTypedImg<Byte>&) out, 
					fMin);
				break;
			case EL_FORMAT_FLOAT: 
				LogTestFunc((CTypedImg<float>&) in, (CTypedImg<float>&) out, 
					fMin);
				break;
			}
			break;
		case EL_FORMAT_BYTE:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE:
				LogTestFunc((CTypedImg<Byte>&) in, (CTypedImg<Byte>&) out, 
					fMin);
				break;
			case EL_FORMAT_FLOAT: 
				LogTestFunc((CTypedImg<Byte>&) in, (CTypedImg<float>&) out, 
					fMin);
				break;
			}
			break;
		default:
			return;
		}
	});
}
END_TEST_EX;

BEGIN_TEST_EX_FAST(ExpTest)
{
	TestFormats([](const CImg& in, const CImg&, CImg& out)
	{
		if (in.Bands() > 1 || out.Bands() > 1)
			return;

		WIN_ASSERT_EQUAL(VtExpImage(out, in), S_OK);

		switch (EL_FORMAT(in.GetType()))
		{
		case EL_FORMAT_FLOAT:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE:
				ExpTestFunc((CTypedImg<float>&) in, (CTypedImg<Byte>&) out);
				break;
			case EL_FORMAT_FLOAT: 
				ExpTestFunc((CTypedImg<float>&) in, (CTypedImg<float>&) out);
				break;
			}
			break;
		case EL_FORMAT_BYTE:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE:
				ExpTestFunc((CTypedImg<Byte>&) in, (CTypedImg<Byte>&) out);
				break;
			case EL_FORMAT_FLOAT: 
				ExpTestFunc((CTypedImg<Byte>&) in, (CTypedImg<float>&) out);
				break;
			}
			break;
		default:
			return;
		}
#undef DO_TEST

	});
}
END_TEST_EX;

BEGIN_TEST_EX_FAST(GammaTest)
{
	TestFormats([](const CImg& in, const CImg&, CImg& out)
	{
		if (in.Bands() > 1 || out.Bands() > 1)
			return;

		float power = 1.2f;		

#define DO_TEST(srct, dstt) \
	GammaTestFunc((CTypedImg<srct>&) in, (CTypedImg<dstt>&) out, power); \
	break;

		CACHED_GAMMA_MAP map;
		WIN_ASSERT_EQUAL(map.Initialize(EL_FORMAT(in.GetType()), 
			EL_FORMAT(out.GetType()), power), S_OK);
		WIN_ASSERT_EQUAL(VtMap(out, in, map), S_OK);

		switch (EL_FORMAT(in.GetType()))
		{
		case EL_FORMAT_FLOAT:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE: DO_TEST(float, Byte);
			case EL_FORMAT_SHORT: DO_TEST(float, UInt16);
			case EL_FORMAT_FLOAT: DO_TEST(float, float);
			}
			break;
		case EL_FORMAT_SHORT:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE: DO_TEST(UInt16, Byte);
			case EL_FORMAT_SHORT: DO_TEST(UInt16, UInt16);
			case EL_FORMAT_FLOAT: DO_TEST(UInt16, float);
			}
			break;
		case EL_FORMAT_BYTE:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE: DO_TEST(Byte, Byte);
			case EL_FORMAT_SHORT: DO_TEST(Byte, UInt16);
			case EL_FORMAT_FLOAT: DO_TEST(Byte, float);
			}
			break;
		default:
			return;
		}
#undef DO_TEST

	});
}
END_TEST_EX;

BEGIN_TEST_EX_FAST(GammaColorTest)
{
	TestFormats([](const CImg& in, const CImg&, CImg& out)
	{
		if (in.Bands() == 1 || out.Bands() == 1)
			return;
				
		float power = 1.6f;

#define DO_TEST(srct, dstt) \
	GammaColorTestFunc((CTypedImg<srct>&) in, (CTypedImg<dstt>&) out, power); \
	break;

		CACHED_GAMMA_MAP map;
		WIN_ASSERT_EQUAL(map.Initialize(EL_FORMAT(in.GetType()), 
			EL_FORMAT(out.GetType()), power), S_OK);
		WIN_ASSERT_EQUAL(VtColorMap(out, in, map), S_OK);

		switch (EL_FORMAT(in.GetType()))
		{
		case EL_FORMAT_FLOAT:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE: DO_TEST(float, Byte);
			case EL_FORMAT_SHORT: DO_TEST(float, UInt16);
			case EL_FORMAT_FLOAT: DO_TEST(float, float);
			}
			break;
		case EL_FORMAT_SHORT:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE: DO_TEST(UInt16, Byte);
			case EL_FORMAT_SHORT: DO_TEST(UInt16, UInt16);
			case EL_FORMAT_FLOAT: DO_TEST(UInt16, float);
			}
			break;
		case EL_FORMAT_BYTE:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE: DO_TEST(Byte, Byte);
			case EL_FORMAT_SHORT: DO_TEST(Byte, UInt16);
			case EL_FORMAT_FLOAT: DO_TEST(Byte, float);
			}
			break;
		default:
			return;
		}
#undef DO_TEST

	});
}
END_TEST_EX;

BEGIN_TEST_EX_FAST(BlendTest)
{
	auto test = [](const CImg& in1, const CImg& in2, CImg& out)
	{
		if (in1.Bands() > 1 || out.Bands() > 1)
			return;

		float wA = 0.67f;

#define DO_TEST(srct, dstt) \
	WIN_ASSERT_EQUAL(VtBlendImages((CTypedImg<dstt>&) out, \
	(CTypedImg<srct>&) in1, (CTypedImg<srct>&) in2,  wA, 1-wA), S_OK); \
	BlendTestFunc((CTypedImg<srct>&) in1, (CTypedImg<srct>&) in2, \
	(CTypedImg<dstt>&) out, wA); \
	break;

		switch (EL_FORMAT(in1.GetType()))
		{
		case EL_FORMAT_FLOAT:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE: DO_TEST(float, Byte);
			case EL_FORMAT_SHORT: DO_TEST(float, UInt16);
			case EL_FORMAT_FLOAT: DO_TEST(float, float);
			}
			break;
		case EL_FORMAT_SHORT:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE: DO_TEST(UInt16, Byte);
			case EL_FORMAT_SHORT: DO_TEST(UInt16, UInt16);
			case EL_FORMAT_FLOAT: DO_TEST(UInt16, float);
			}
			break;
		case EL_FORMAT_BYTE:
			switch (EL_FORMAT(out.GetType()))
			{
			case EL_FORMAT_BYTE: DO_TEST(Byte, Byte);
			case EL_FORMAT_SHORT: DO_TEST(Byte, UInt16);
			case EL_FORMAT_FLOAT: DO_TEST(Byte, float);
			}
			break;
		default:
			return;
		}
#undef DO_TEST
	};
	
	TestFormats(test, true);
}
END_TEST_EX;

END_TEST_FILE