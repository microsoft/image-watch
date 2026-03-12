#pragma once
#include <random>

#define FAST_TEST_ATTRIBUTE(x)			 \
BEGIN_TEST_METHOD_ATTRIBUTE(x)			 \
TEST_METHOD_ATTRIBUTE(L"Category", L"Fast") \
TEST_PRIORITY(1)						\
END_TEST_METHOD_ATTRIBUTE()

#define SLOW_TEST_ATTRIBUTE(x)			 \
BEGIN_TEST_METHOD_ATTRIBUTE(x)			 \
TEST_METHOD_ATTRIBUTE(L"Category", L"Slow") \
TEST_PRIORITY(2)						\
END_TEST_METHOD_ATTRIBUTE()

#define BEGIN_TEST_FAST(x)		\
FAST_TEST_ATTRIBUTE(x)			\
BEGIN_TEST(x)

#define BEGIN_TEST_SLOW(x)		\
FAST_TEST_ATTRIBUTE(x)			\
BEGIN_TEST(x)

#define BEGIN_TEST_PERF(x)				\
BEGIN_TEST_METHOD_ATTRIBUTE(x)		\
TEST_METHOD_ATTRIBUTE(L"Category", L"Perf") \
END_TEST_METHOD_ATTRIBUTE()			\
BEGIN_TEST(x)

#if defined(_DEBUG) && (WINAPI_FAMILY==WINAPI_FAMILY_DESKTOP_APP)
class MemoryChecker
{
public:
	MemoryChecker()
	{
		_CrtMemCheckpoint(&start_);
	}

	~MemoryChecker()
	{
		_CrtMemState stop;
		_CrtMemCheckpoint(&stop);

		_CrtMemState diff;
		if (_CrtMemDifference(&diff, &start_,  &stop))
		{
			_CrtDumpMemoryLeaks();			
			WIN_ASSERT_FAIL(L"Memory difference detected.");
		}
	}

private:
	MemoryChecker(const MemoryChecker&);
	MemoryChecker& operator=(const MemoryChecker&);

private:
	_CrtMemState start_;
};

#define BEGIN_TEST_EX(x)			\
BEGIN_TEST(x)						\
{									\
MemoryChecker mc;

#else

#define BEGIN_TEST_EX(x)			\
BEGIN_TEST(x)						\
{   

#endif

#define BEGIN_TEST_EX_FAST(x)		\
FAST_TEST_ATTRIBUTE(x)					\
BEGIN_TEST_EX(x)

#define BEGIN_TEST_EX_SLOW(x)		\
SLOW_TEST_ATTRIBUTE(x)					\
BEGIN_TEST_EX(x)



#define END_TEST_EX					\
}									\
END_TEST

#define BEGIN_TEST_VTR(x)			\
BEGIN_TEST_EX(x)                    \
{                                   \
	CVTResourceManagerInit ri;

#define END_TEST_VTR                \
}		                            \
END_TEST_EX

#define BEGIN_TEST_VTR_FAST(x)		\
FAST_TEST_ATTRIBUTE(x)				\
BEGIN_TEST_VTR(x)

#define BEGIN_TEST_VTR_SLOW(x)		\
SLOW_TEST_ATTRIBUTE(x)				\
BEGIN_TEST_VTR(x)

int ElFormatFromString(const std::string& s);
std::string ElFormatToString(int elf);
int PixFormatFromString(const std::string& s);
std::string PixFormatToString(int pxf);

std::wstring GetEnvironmentVariable(const std::wstring& name);
std::wstring GetImageDir();
std::wstring GetFileName(const std::wstring& path);
std::wstring GetFileExtension(const std::wstring& path);

double GetAverageAbsoluteDifference(const vt::CImg& a, const vt::CImg& b);

void LoadImage(const std::wstring& path, vt::CImg& img);
void SaveImage(const std::wstring& path, const vt::CImg& img);

void SaveTwoImages(const vt::CImg& img1, const vt::CImg& img2);

void ShowMessage(const WCHAR* fmt, ...);

//+-----------------------------------------------------------------------------
//
// helpers to generate random test parameters
//
//------------------------------------------------------------------------------
namespace vt {
class CRand
{
public:
    CRand(int seed = 1) : engine(seed) {}
    double DRand()
    {
        return std::uniform_real_distribution<double>(0.0, 1.0)(engine);
    }
    double URand(double min, double max)
    {
        return std::uniform_real_distribution<double>(min, max)(engine);
    }
    int IRand(int max)
    {
        return std::uniform_int_distribution<int>(0, max)(engine);
    }
private:
    std::mt19937 engine;
};
} // namespace vt

inline vt::CRect
RandRect(int w, int h, vt::CRand& rand)
{
    vt::CRect r;
    r.right  = rand.IRand(w+1)%(w+1);
    r.bottom = rand.IRand(h+1)%(h+1);
    r.right  = vt::VtMax(r.right, 1L);
    r.bottom = vt::VtMax(r.bottom, 1L);
    r.left   = (r.right<=1)?  0: (rand.IRand(r.right)  % (r.right-1));
    r.top    = (r.bottom<=1)? 0: (rand.IRand(r.bottom) % (r.bottom-1));
    return r;
}

inline void
RandPixelValue(vt::Byte* pixval, const vt::CImgInfo& info, vt::CRand& rand)
{
	using namespace vt;

    for( int i = 0; i < info.Bands(); i++ )
    {
        switch( EL_FORMAT(info.type) )
        {
        case EL_FORMAT_BYTE:
            pixval[i] = (Byte)(rand.IRand(256)%255);
            break;
        case EL_FORMAT_SHORT:
            ((UInt16*)pixval)[i] = (UInt16)(rand.IRand(0xffff)%0xffff);
            break;
        case EL_FORMAT_HALF_FLOAT:
			VtConv(((HALF_FLOAT*)pixval)+i, (float)(rand.DRand()));
			break;
        case EL_FORMAT_FLOAT:
            ((float*)pixval)[i] = (float)(rand.DRand());
            break;
        }
    }

    if( EL_FORMAT(info.type) == EL_FORMAT_HALF_FLOAT )
    {

    }
}

////////////////////////////////////////////////////////////////////////////////

template<typename TestFunc>
void TestFormats(TestFunc test, bool binary = false)
{
	CImg img;
	LoadImage(GetImageDir() + L"newman_small.png", img);

	CImg img2;
	if (binary)
	{
		CImg tmp;
		tmp.Create(img.Width(), img.Height(), img.GetType());
		LoadImage(GetImageDir() + L"burns.jpg", tmp);
		VtResizeImage(img2, img.Rect(), tmp, eSamplerKernelBilinear);
	}

	const int elformats[] = { EL_FORMAT_BYTE, EL_FORMAT_SHORT, 
		EL_FORMAT_FLOAT, EL_FORMAT_HALF_FLOAT };
	const int pixformats[] = { PIX_FORMAT_LUMA, 
		PIX_FORMAT_RGB  | (2<<VT_IMG_BANDS_SHIFT), 
		PIX_FORMAT_RGBA | (3<<VT_IMG_BANDS_SHIFT) };

	std::for_each(pixformats, pixformats + _countof(pixformats), 
		[&](int spf)
	{
		std::for_each(elformats, elformats + _countof(elformats), 
			[&](int sef)
		{
			CImg in;
			in.Create(img.Width(), img.Height(), spf | sef);
			VtConvertImage(in, img);

			CImg in2;
			if (binary)
			{
				in2.Create(img.Width(), img.Height(), spf | sef);
				VtConvertImage(in2, img2);
			}

			std::for_each(pixformats, pixformats + _countof(pixformats), 
				[&](int dpf)
			{
				std::for_each(elformats, elformats + _countof(elformats), 
					[&](int def)
				{
					CImg out;
					out.Create(img.Width(), img.Height(), dpf | def);

					test(in, in2, out);
				});
			});
		});
	});
}

////////////////////////////////////////////////////////////////////////////////
void GenerateRamp(vt::CImg& img);
