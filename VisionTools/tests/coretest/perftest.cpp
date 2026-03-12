#include "stdafx.h"

#include "cthelpers.h"
#include "perftest.h"

BEGIN_TEST_FILE(PerfTests)

class PerfMonitor
{
public:
	PerfMonitor()
	{
		PerfTestManager::Initialize();
	}

	~PerfMonitor()
	{
		PerfTestManager::PrintSummary();
	}
};

PerfMonitor g_pm;

////////////////////////////////////////////////////////////////////////////////

template<typename TestImpl>
class GenericPerfTestNoOutput : public PerfTest<GenericPerfTestNoOutput<TestImpl>>
{	
public:
	GenericPerfTestNoOutput(const std::string& name)
		: name_(name)
	{
		LoadImage(GetImageDir() + L"newman.png", img1_);
	}

public:
	bool RunSingleTestCase()
	{
		return static_cast<TestImpl*>(this)->RunSingleTestCase();
	}

protected:
	virtual std::string GetName() const
	{
		return name_;
	}

	virtual std::string GetParamNames() const
	{
		return "Width, Height, ElFormat, PixFormat";
	}

	virtual void Initialize()
	{
		int w = GetParamValue<int>("Width");
		int h = GetParamValue<int>("Height");

		in1_.Create(w, h, 
			ElFormatFromString(GetParamValue<std::string>("ElFormat")) |
			PixFormatFromString(GetParamValue<std::string>("PixFormat")));

        vt::VtResizeImage(in1_, in1_.Rect(), img1_, vt::eSamplerKernelLanczos3);
	}

protected:
	std::string name_;
	vt::CImg img1_, in1_;
};

template<typename TestImpl>
class GenericPerfTestUnary : public PerfTest<GenericPerfTestUnary<TestImpl>>
{	
public:
	GenericPerfTestUnary(const std::string& name)
		: name_(name)
	{
		LoadImage(GetImageDir() + L"newman.png", img1_);
	}

public:
	bool RunSingleTestCase()
	{
		return static_cast<TestImpl*>(this)->RunSingleTestCase();
	}

protected:
	virtual std::string GetName() const
	{
		return name_;
	}

	virtual std::string GetParamNames() const
	{
		return "Width, Height, InElFormat, InPixFormat,"
			"OutElFormat, OutPixFormat";
	}

	virtual void Initialize()
	{
		int w = GetParamValue<int>("Width");
		int h = GetParamValue<int>("Height");

		in1_.Create(w, h, 
			ElFormatFromString(GetParamValue<std::string>("InElFormat")) |
			PixFormatFromString(GetParamValue<std::string>("InPixFormat")));
        
		vt::VtResizeImage(in1_, in1_.Rect(), img1_, vt::eSamplerKernelLanczos3);

		out_.Create(w, h, 
			ElFormatFromString(GetParamValue<std::string>("OutElFormat")) |
			PixFormatFromString(GetParamValue<std::string>("OutPixFormat")));

		out_.Clear();
	}

protected:
	std::string name_;
	vt::CImg img1_, in1_, out_;
};

////////////////////////////////////////////////////////////////////////////////

template<typename TestImpl>
class GenericPerfTestBinary : public GenericPerfTestUnary<TestImpl>
{
public:
	GenericPerfTestBinary(const std::string& name)
		: GenericPerfTestUnary(name)
	{
		LoadImage(GetImageDir() + L"burns.jpg", img2_);
	}

public:
	bool RunSingleTestCase()
	{
		return static_cast<TestImpl*>(this)->RunSingleTestCase();
	}

protected:
	virtual void Initialize()
	{
		GenericPerfTestUnary<TestImpl>::Initialize();

		int w = GetParamValue<int>("Width");
		int h = GetParamValue<int>("Height");
		
		in2_.Create(w, h, 
			ElFormatFromString(GetParamValue<std::string>("InElFormat")) |
			PixFormatFromString(GetParamValue<std::string>("InPixFormat")));

        vt::VtResizeImage(in2_, in2_.Rect(), img2_, vt::eSamplerKernelLanczos3);
	}

protected:
	vt::CImg img2_, in2_;
};

template<typename TestType>
void RunGenericPerfTest(TestType& test, int w, int num_trials, 
	bool loop_count_auto_mode, int loop_count_preference)
{
	size_t loop_count = 0;

	if (loop_count_auto_mode)
	{
		// loop_count_preference = milliseconds per Run()
		float elapsed = test.RunOnce();
		loop_count = std::max<int>(1, 
			(int)(loop_count_preference/elapsed/num_trials));
	}
	else
	{
		// loop_count_preference = num iter at width 100
		loop_count = std::max<int>(1, (int) 
			(loop_count_preference * (100*100*1.0/(w*w))));
	}

	DiffResultType rt = Error;
	test.Run(num_trials, loop_count, &rt);

	switch (rt)
	{
	case Same:
	case New:
		std::cout << "."; break;
	case Better:
		std::cout << "+"; break;
	case ShortTime:
	case HighVariance:
		std::cout << "?"; break;
	case Error:
		std::cout << "x"; break;
	default:
		std::cout << "!"; break;
	}
}

////////////////////////////////////////////////////////////////////////////////

template<typename TestType, int NumElFormats, int NumPixFormats,
	int NumWidths>
void PerfTestRange(TestType& test, int num_trials, bool loop_count_auto_mode, 
	int loop_count_preference, const int (&elformats)[NumElFormats], 
	const int (&pixformats)[NumPixFormats], const int (&widths)[NumWidths])
{
	std::cout << "Running " << NumElFormats * NumElFormats * NumPixFormats * 
		NumPixFormats * NumWidths << " tests: ";

	std::for_each(widths, widths + _countof(widths), [&](int w)
	{
		test.SetParamValue("Width", w);
		test.SetParamValue("Height", (int) (0.5 + w*0.75));

		std::for_each(pixformats, pixformats + _countof(pixformats), 
			[&](int spf)
		{
			test.SetParamValue("InPixFormat", PixFormatToString(spf));

			std::for_each(elformats, elformats + _countof(elformats), 
				[&](int sef)
			{
				test.SetParamValue("InElFormat", ElFormatToString(sef));

				std::for_each(pixformats, pixformats + _countof(pixformats), 
					[&](int dpf)
				{
					test.SetParamValue("OutPixFormat", PixFormatToString(dpf));

					std::for_each(elformats, elformats + _countof(elformats), 
						[&](int def)
					{
						test.SetParamValue("OutElFormat", ElFormatToString(def));

						RunGenericPerfTest(test, w, num_trials, loop_count_auto_mode,
							loop_count_preference);
					});
				});
			});
		});
	});

	std::cout << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

template<typename TestType>
void PerfTestRange(TestType& test, int num_trials, bool loop_count_auto_mode, 
	int loop_count_preference)
{
	const int elformats[] = { EL_FORMAT_BYTE , EL_FORMAT_SHORT, 
		EL_FORMAT_FLOAT, EL_FORMAT_HALF_FLOAT };
	const int pixformats[] = { PIX_FORMAT_LUMA,
        PIX_FORMAT_RGB | (2<<VT_IMG_BANDS_SHIFT), 
		PIX_FORMAT_RGBA | (3<<VT_IMG_BANDS_SHIFT) };
	const int widths[] = { 100, 300, 1000, 3000 };

	PerfTestRange(test, num_trials, loop_count_auto_mode, loop_count_preference,
		elformats, pixformats, widths);
}

////////////////////////////////////////////////////////////////////////////////

template<typename TestType, int NumElFormats, int NumPixFormats,
	int NumWidths>
void PerfTestRangeNoOutput(TestType& test, int num_trials, bool loop_count_auto_mode, 
	int loop_count_preference, const int (&elformats)[NumElFormats], 
	const int (&pixformats)[NumPixFormats], const int (&widths)[NumWidths])
{
	std::cout << "Running " << NumElFormats * NumPixFormats * NumWidths << 
		" tests: ";

	std::for_each(widths, widths + _countof(widths), [&](int w)
	{
		test.SetParamValue("Width", w);
		test.SetParamValue("Height", (int) (0.5 + w*0.75));

		std::for_each(pixformats, pixformats + _countof(pixformats), 
			[&](int spf)
		{
			test.SetParamValue("PixFormat", PixFormatToString(spf));

			std::for_each(elformats, elformats + _countof(elformats), 
				[&](int sef)
			{
				test.SetParamValue("ElFormat", ElFormatToString(sef));

				RunGenericPerfTest(test, w, num_trials, loop_count_auto_mode,
					loop_count_preference);
			});
		});
	});

	std::cout << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

template<typename TestType>
void PerfTestRangeNoOutput(TestType& test, int num_trials, bool loop_count_auto_mode, 
	int loop_count_preference)
{
	const int elformats[] = { EL_FORMAT_BYTE , EL_FORMAT_SHORT, 
		EL_FORMAT_FLOAT, EL_FORMAT_HALF_FLOAT };
	const int pixformats[] = { PIX_FORMAT_LUMA,
        PIX_FORMAT_RGB | (2<<VT_IMG_BANDS_SHIFT), 
		PIX_FORMAT_RGBA | (3<<VT_IMG_BANDS_SHIFT) };
	const int widths[] = { 100, 300, 1000, 3000 };

	PerfTestRange(test, num_trials, loop_count_auto_mode, loop_count_preference,
		elformats, pixformats, widths);
}

////////////////////////////////////////////////////////////////////////////////



BEGIN_TEST_PERF(Convert)
{
	class ConvertPerfTest : public GenericPerfTestUnary<ConvertPerfTest>
	{
	public:
		ConvertPerfTest()
			: GenericPerfTestUnary("Convert")
		{
		}

		bool RunSingleTestCase()
		{
			return SUCCEEDED(vt::VtConvertImage(out_, in1_));
		}
	};

	ConvertPerfTest t;

	PerfTestRange(t, 25, true, 500);
}
END_TEST;

BEGIN_TEST_PERF(Add)
{
	class AddPerfTest : public GenericPerfTestBinary<AddPerfTest>
	{
	public:
		AddPerfTest()
			: GenericPerfTestBinary<AddPerfTest>("Add")
		{
		}

		bool RunSingleTestCase()
		{
			return SUCCEEDED(vt::VtAddImages(out_, in1_, in2_));
		}
	};

	const int elformats[] = { EL_FORMAT_FLOAT, EL_FORMAT_BYTE, EL_FORMAT_SHORT };
	const int pixformats[] = { PIX_FORMAT_LUMA };
	const int widths[] = { 100, 300, 1000, 3000 };

	AddPerfTest t;
	PerfTestRange(t, 25, true, 500, elformats, pixformats, widths);
}
END_TEST;

BEGIN_TEST_PERF(Mul)
{
	class MulPerfTest : public GenericPerfTestBinary<MulPerfTest>
	{
	public:
		MulPerfTest()
			: GenericPerfTestBinary<MulPerfTest>("Mul")
		{
		}

		bool RunSingleTestCase()
		{
			return SUCCEEDED(vt::VtMulImages(out_, in1_, in2_));
		}
	};

	const int elformats[] = { EL_FORMAT_FLOAT, EL_FORMAT_BYTE, EL_FORMAT_SHORT };
	const int pixformats[] = { PIX_FORMAT_LUMA };
	const int widths[] = { 100, 300, 1000, 3000 };

	MulPerfTest t;
	PerfTestRange(t, 25, true, 500, elformats, pixformats, widths);
}
END_TEST;

BEGIN_TEST_PERF(Scale)
{
	class ScalePerfTest : public GenericPerfTestUnary<ScalePerfTest>
	{
	public:
		ScalePerfTest()
			: GenericPerfTestUnary<ScalePerfTest>("Scale")
		{
		}

		bool RunSingleTestCase()
		{
			return SUCCEEDED(vt::VtScaleImage(out_, in1_, 0.33f));
		}
	};

	const int elformats[] = { EL_FORMAT_FLOAT, EL_FORMAT_BYTE, 
		EL_FORMAT_SHORT, EL_FORMAT_HALF_FLOAT };
	const int pixformats[] = { PIX_FORMAT_LUMA };
	const int widths[] = { 300, 1000, 3000 };

	ScalePerfTest t;
	PerfTestRange(t, 25, true, 500, elformats, pixformats, widths);
}
END_TEST;

BEGIN_TEST_PERF(ScaleOffset)
{
	class ScaleOffsetPerfTest : public GenericPerfTestUnary<ScaleOffsetPerfTest>
	{
	public:
		ScaleOffsetPerfTest()
			: GenericPerfTestUnary<ScaleOffsetPerfTest>("ScaleOffset")
		{
		}

		bool RunSingleTestCase()
		{
			return SUCCEEDED(vt::VtScaleOffsetImage(out_, in1_, 0.33f, 0.5f));
		}
	};

	const int elformats[] = { EL_FORMAT_FLOAT, EL_FORMAT_BYTE, 
		EL_FORMAT_SHORT, EL_FORMAT_HALF_FLOAT };
	const int pixformats[] = { PIX_FORMAT_LUMA };
	const int widths[] = { 300, 1000, 3000 };

	ScaleOffsetPerfTest t;
	PerfTestRange(t, 25, true, 500, elformats, pixformats, widths);
}
END_TEST;

BEGIN_TEST_PERF(ScaleColor)
{
	class ScaleColorPerfTest : public GenericPerfTestUnary<ScaleColorPerfTest>
	{
	public:
		ScaleColorPerfTest()
			: GenericPerfTestUnary<ScaleColorPerfTest>("ScaleColor")
		{
		}

		bool RunSingleTestCase()
		{
			return SUCCEEDED(vt::VtScaleColorImage(out_, in1_, 
				vt::RGBAFloatPix(0.33f, 1, 1, 1)));
		}
	};

	const int elformats[] = { EL_FORMAT_FLOAT, EL_FORMAT_BYTE, 
		EL_FORMAT_SHORT, EL_FORMAT_HALF_FLOAT };
	const int pixformats[] = { 
        PIX_FORMAT_RGB | (2<<VT_IMG_BANDS_SHIFT), 
		PIX_FORMAT_RGBA | (3<<VT_IMG_BANDS_SHIFT) };
	const int widths[] = { 300, 1000 };

	ScaleColorPerfTest t;
	PerfTestRange(t, 25, true, 500, elformats, pixformats, widths);
}
END_TEST;

BEGIN_TEST_PERF(ScaleOffsetColor)
{
	class ScaleOffsetColorPerfTest : 
		public GenericPerfTestUnary<ScaleOffsetColorPerfTest>
	{
	public:
		ScaleOffsetColorPerfTest()
			: GenericPerfTestUnary<ScaleOffsetColorPerfTest>("ScaleOffsetColor")
		{
		}

		bool RunSingleTestCase()
		{
			return SUCCEEDED(vt::VtScaleOffsetColorImage(out_, in1_, 
				vt::RGBAFloatPix(0.33f, 1, 1, 1), vt::RGBAFloatPix(0.5f, 1, 1, 1)));
		}
	};

	const int elformats[] = { EL_FORMAT_FLOAT, EL_FORMAT_BYTE, 
		EL_FORMAT_SHORT, EL_FORMAT_HALF_FLOAT };
	const int pixformats[] = { 
        PIX_FORMAT_RGB | (2<<VT_IMG_BANDS_SHIFT), 
		PIX_FORMAT_RGBA | (3<<VT_IMG_BANDS_SHIFT) };
	const int widths[] = { 300, 1000 };

	ScaleOffsetColorPerfTest t;
	PerfTestRange(t, 25, true, 500, elformats, pixformats, widths);
}
END_TEST;

BEGIN_TEST_PERF(MultiplyAlpha)
{
	class MultiplyAlphaTest : 
		public GenericPerfTestUnary<MultiplyAlphaTest>
	{
	public:
		MultiplyAlphaTest()
			: GenericPerfTestUnary("MultiplyAlpha")
		{
		}

		bool RunSingleTestCase()
		{
			return SUCCEEDED(vt::VtMultiplyAlpha(out_, in1_));
		}
	};

	const int elformats[] = { EL_FORMAT_FLOAT, EL_FORMAT_BYTE, 
		EL_FORMAT_SHORT, EL_FORMAT_HALF_FLOAT };
	const int pixformats[] = { 
        PIX_FORMAT_RGB | (2<<VT_IMG_BANDS_SHIFT), 
		PIX_FORMAT_RGBA | (3<<VT_IMG_BANDS_SHIFT) };
	const int widths[] = { 300, 1000 };

	MultiplyAlphaTest t;
	PerfTestRange(t, 25, true, 500, elformats, pixformats, widths);
}
END_TEST;

BEGIN_TEST_PERF(MinMax)
{
	class MinMaxTest : 
		public GenericPerfTestNoOutput<MinMaxTest>
	{
	public:
		MinMaxTest()
			: GenericPerfTestNoOutput("MinMax")
		{
		}

		bool RunSingleTestCase()
		{
			float mn, mx;
			return SUCCEEDED(vt::VtMinMaxImage((vt::CFloatImg&)in1_, mn, mx));
		}
	};

	const int elformats[] = { EL_FORMAT_FLOAT, EL_FORMAT_BYTE, EL_FORMAT_SHORT,
		EL_FORMAT_HALF_FLOAT };
	const int pixformats[] = { PIX_FORMAT_LUMA };
	const int widths[] = { 300, 1000, 3000 };

	MinMaxTest t;
	PerfTestRangeNoOutput(t, 25, true, 500, elformats, pixformats, widths);
}
END_TEST;

BEGIN_TEST_PERF(MinMaxColor)
{
	class MinMaxColorTest : 
		public GenericPerfTestNoOutput<MinMaxColorTest>
	{
	public:
		MinMaxColorTest()
			: GenericPerfTestNoOutput("MinMaxColor")
		{
		}

		bool RunSingleTestCase()
		{
			vt::RGBAFloatPix mn, mx;
			return SUCCEEDED(vt::VtMinMaxColorImage(in1_, mn, mx));
		}
	};

	const int elformats[] = { EL_FORMAT_FLOAT, EL_FORMAT_BYTE, EL_FORMAT_SHORT,
		EL_FORMAT_HALF_FLOAT };
	const int pixformats[] = { 
        PIX_FORMAT_RGB | (2<<VT_IMG_BANDS_SHIFT), 
		PIX_FORMAT_RGBA | (3<<VT_IMG_BANDS_SHIFT) };
	const int widths[] = { 300, 1000, 3000 };

	MinMaxColorTest t;
	PerfTestRangeNoOutput(t, 25, true, 500, elformats, pixformats, widths);
}
END_TEST;

BEGIN_TEST_PERF(BlendTest)
{
	class BlendTest : 
		public GenericPerfTestBinary<BlendTest>
	{
	public:
		BlendTest()
			: GenericPerfTestBinary("Blend")
		{
		}

		bool RunSingleTestCase()
		{
			return SUCCEEDED(vt::VtBlendImages(out_, in1_, in2_, .67f, .33f));
		}
	};

	const int elformats[] = { EL_FORMAT_FLOAT, EL_FORMAT_BYTE, EL_FORMAT_SHORT,
		EL_FORMAT_HALF_FLOAT };
	const int pixformats[] = { PIX_FORMAT_LUMA };
	const int widths[] = { 300, 1000, 3000 };

	BlendTest t;
	PerfTestRange(t, 25, true, 500, elformats, pixformats, widths);
}
END_TEST;

#if 0
extern HRESULT
VtSeparableFilter2(vt::CImg &imgDst, const vt::CRect& rctDst, 
                   const vt::CImg &imgSrc, vt::CPoint ptSrcOrigin,
                   const vt::C1dKernel& k,
                   const vt::IMAGE_EXTEND& ex = vt::IMAGE_EXTEND(vt::Zero));

BEGIN_TEST_PERF(SeparableFilter2Test)
{
	class SeparableFilter2Test : 
		public GenericPerfTestUnary<SeparableFilter2Test>
	{
	public:
		SeparableFilter2Test()
			: GenericPerfTestUnary("SeparableFilter2")
		{
		}

		bool RunSingleTestCase()
		{
			vt::C1dKernel k;
			Create1dGaussKernel(k, 1.f);
			//return SUCCEEDED(vt::VtSeparableFilter(in1_, out_, k, k));
            return SUCCEEDED(VtSeparableFilter2(out_, out_.Rect(), in1_, 
                vt::CPoint(0,0), k, vt::IMAGE_EXTEND(vt::ExtendWithAlpha)));
		}
	};

	const int elformats[] = { EL_FORMAT_BYTE, EL_FORMAT_SHORT, 
		                      EL_FORMAT_HALF_FLOAT, EL_FORMAT_FLOAT };
	const int pixformats[] = { PIX_FORMAT_LUMA, 
        PIX_FORMAT_RGB | (2<<VT_IMG_BANDS_SHIFT), 
		PIX_FORMAT_RGBA | (3<<VT_IMG_BANDS_SHIFT) };
	const int widths[] = { 300, 1000, 3000 };

	SeparableFilter2Test t;
	PerfTestRange(t, 1, true, 4, elformats, pixformats, widths);
}
END_TEST;
#endif

END_TEST_FILE