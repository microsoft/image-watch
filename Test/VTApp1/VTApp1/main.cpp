#include "stdafx.h"
#include <vector>
#include <fstream>
#include <vtcore.h>

using namespace vt;

// Initialize the VisionTools multicore framework.
CVTResourceManagerInit g_vtResourceManagerInit;

struct MyPlanarYUVImage
{
	unsigned int ncols;
	unsigned int nrows;
	unsigned char* ydata;
	unsigned char* udata;
	unsigned char* vdata;
};

struct YV12Img
{
	CLumaByteImg y;
	CLumaByteImg v;
	CLumaByteImg u;
};
struct IYUVImg
{
	CLumaByteImg y;
	CLumaByteImg v;
	CLumaByteImg u;
};
struct YUY2Img
{
	CByteImg data;
};

int foo(int a)
{
	std::ofstream f("images\\foo.txt");
	f << a;
	f.close();
	return 123;
}

void Test(CImg& a, CImg& b)
{
	CImg* aptr = &a;
	CImg** aptrptr = &aptr;
}

void dbltest()
{
	CTypedImg<double> a(10,10);
	a.Fill(1e123);
	a(1,1) = -1e100;
}

void inttest()
{
	CIntImg a(10,10,1);
	CTypedImg<int16_t> ss(10,10);
	CTypedImg<uint16_t> us(10,10);
	CTypedImg<int8_t> sb(10,10);
	CTypedImg<uint8_t> ub(10,10);

	sb.Fill(0);
	ub.Fill(127);

	ss(0,0) = 0;
	sb(0,0) = 0;
	us(0,0) = 0;
	ub(0,0) = 0;
	a(0,0) = 0;

	for (int y=0; y<10; ++y)
		for (int x=0; x<10; ++x)
		{
			a(x,y) = (x*y)*500000;	
			sb(x,y) = x*y- 10000;
		}
}


namespace vt
{template<> struct ElTraits<long long > 
{
	typedef uint64_t BLOB_T;

	static int    ElFormat()   { return 123; }
	static float  MaxVal()     { return 1.f; }
	//static BLOB_T MaxValBlob() // TODO: implement the following
	//static float  TypeMax() { return 3.402823466e+38f; } 
	//static float  TypeMin() { return -3.402823466e+38f; }
};}



void ptrtest(const CImg& a)
{
	int x = -1;
	CImg* aptr = (CImg* )&a;
	CImg const*const*const aptrptr = &aptr;
	CImg*& aptrref = aptr;
	const CImg& arefc = a;
	const CImg& arefcv = a;

	std::vector<int> asdf;

	CTypedImg<long long> bad;
	bad.Create(10, 10);
}

namespace vt
{
	class asdf {};
}

struct My8BitRGBImage
{
	My8BitRGBImage()
		: ncols(10), nrows(3)
	{
	}
	unsigned int ncols;
	unsigned int nrows;
	unsigned char* data;
};

template <typename T>
struct MyGenericImage
{
	MyGenericImage()
		: ncols(10), nrows(3), nchannels(4)
	{
	}
	unsigned int ncols;
	unsigned int nrows;
	unsigned int nchannels;
	T* data;
};

template <typename T>
class mytemplate
{
};

template <typename T>
struct mytraits;

template <>
struct mytraits<unsigned char>
{
	static const int value = 1;
};

class TComPicYuv
	{
	public:
		int   m_iPicWidth;
		int   m_iPicHeight;

		short*  m_piPicOrgY;    // Pel is short actualy
		short*  m_piPicOrgU;
		short*  m_piPicOrgV;

	};

void aaa()
{
	

	TComPicYuv p;

	p.m_iPicWidth = 300;
	p.m_iPicHeight = 200;

	p.m_piPicOrgY = new short[300*200];
	p.m_piPicOrgU = new short[300*200];
	p.m_piPicOrgV = new short[300*200];

}

int wmain(int argc, const wchar_t* argv[])
{
	aaa();

	vt::CRGBByteImg errimg;
	errimg.Create(1920, 1080);
	errimg.Fill(RGBPix(255, 0, 255));

	My8BitRGBImage rgb;
	MyGenericImage<float> rgbf;

	std::vector<int> v;

	dbltest();
	ptrtest(CByteImg(320,240));
	inttest();

	VT_HR_BEGIN();

	vt::asdf asdf;
	(void)asdf;

	// TODO: Change the following code for your own purposes.

	vt::vector<vt::CRGBByteImg*> vimg;
	vimg.resize(10);
	vimg[4] = new CRGBByteImg();
	vimg[4]->Create(100, 200);

	CTypedImg<Byte> a;

	a.Create(0,0,1);

	CImg& aref = a;
	CByteImg b;
	b.Create(2,2, 14);
	CImg c;
	CImg* cptr = &c;
	CImg* aptr = &a;
	CImg** aptrptr = &aptr;
	CImg*& aptrref = aptr;

	a.Create(1000,1000, 3);
	
	CIntImg bint2;
	bint2.Create(400,300,2);
	bint2.Fill(-10000);
	CUVShortImg bshort2;
	bshort2.Create(400,300);
	bshort2.Fill(UVShortPix(20000, 20000));
	bshort2(0,0,0) = 0;
	bshort2(0,0,1) = 0;
	bshort2(0,1,0) = 0;
	bshort2(0,1,1) = 30000;
	bshort2(1,1,0) = 30000;
	bshort2(1,1,1) = 30000;
	CTypedImg<int16_t> bshort1;
	bshort1.Create(400,300,1);
	bshort1.Fill(12345);

	a.Clear();
	a.Fill(200);

	CTypedImg<float> matrix(5,5);
	for (int i=0; i<5; ++i)
		for (int j = 0;j<5;++j)
			matrix(i,j) = rand()*0.001f;

	wchar_t* fn = L"fsad";

	Test(b, a);

	//for (;;);	

	CRGBByteImg originalImage;
	CFloatImg fimg;
	CImg blurredImage;
	VT_HR_EXIT(VtLoadImage(L"images\\000166.bmp", originalImage));
	VtConvertImage(fimg, originalImage);

	CShortImg si;
	VtScaleImage(si, originalImage, 1.f/255);

	CImg fruit;
	auto path = L"images\\fruits.jpg";
	VtLoadImage(path, fruit);

	My8BitRGBImage f2;
	f2.ncols = fruit.Width();
	f2.nrows = fruit.Height();
	f2.data = fruit.BytePtr();
	rgbf.data = (float*)f2.data;

	CImg couch;
	VtLoadImage(L"images\\0000.tif", couch);

	CImg mask;
	VtLoadImage(L"images\\cb.jpg", mask);

	CTypedImg<int8_t> sa(10, 10, 1);
	sa.Fill(-120);	
	sa(0,0) = -80;
	sa(1,1) = 0;
	sa(2,2) = 100;
	sa(3,3) = 127;
	CFloatImg f;
	VtConvertImage(f, sa);
	CTypedImg<int16_t> sb(10, 10, 3);
	sb.Fill(1000);
	sb(2,3) = -2000;


	CByteImg tst2;
	VtScaleImage(tst2, mask, 0.2f);

	CImg img;
	img.Create(10, 23, VT_IMG_MAKE_TYPE(EL_FORMAT_DOUBLE, 3));

	CImgReaderWriter<CImg> imgRW;
	img.Share(imgRW);

	CShortImg vector(1,23);

	char* fna = "images\\asdf.vti";
	const wchar_t* fnb = L"images\\asdf.vti";

	VT_HR_EXIT_LABEL();

	return SUCCEEDED(hr) ? 0 : 1;
}