#include "stdafx.h"
#include "cthelpers.h"

using namespace vt;

struct IntStringMapping
{
	int i;
	const char* s;
};

const IntStringMapping elf_map[] = 
{ 
	{ EL_FORMAT_BYTE, "Byte" },
	{ EL_FORMAT_SHORT, "Short" },
	{ EL_FORMAT_FLOAT, "Float" },
	{ EL_FORMAT_HALF_FLOAT, "HalfFloat" }
};

const IntStringMapping* const elf_map_end = elf_map + _countof(elf_map);

const IntStringMapping pxf_map[] = 
{ 
	{ PIX_FORMAT_LUMA, "Luma" },
	{ PIX_FORMAT_RGB  | (2<<VT_IMG_BANDS_SHIFT), "RGB" },
	{ PIX_FORMAT_RGBA | (3<<VT_IMG_BANDS_SHIFT), "RGBA" },
};

const IntStringMapping* const pxf_map_end = pxf_map + _countof(pxf_map);

int ElFormatFromString(const std::string& s)
{
	auto it = std::find_if(elf_map, elf_map_end, 
	[&s](const IntStringMapping& m) { return m.s == s; });

	return it != elf_map_end ? (*it).i : -1;
}

std::string ElFormatToString(int elf)
{
	auto it = std::find_if(elf_map, elf_map_end, 
	[&elf](const IntStringMapping& m) { return m.i == elf; });

	return it != elf_map_end ? (*it).s : "";
}

int PixFormatFromString(const std::string& s)
{
	auto it = std::find_if(pxf_map, pxf_map_end, 
	[&s](const IntStringMapping& m) { return m.s == s; });

	return it != pxf_map_end ? (*it).i : -1;
}

std::string PixFormatToString(int pxf)
{
	auto it = std::find_if(pxf_map, pxf_map_end, 
	[&pxf](const IntStringMapping& m) { return m.i == pxf; });

	return it != pxf_map_end ? (*it).s : "";
}

#if WINAPI_FAMILY==WINAPI_FAMILY_DESKTOP_APP

std::wstring GetEnvironmentVariable(const std::wstring& name)
{
	std::vector<wchar_t> buf(256);
	GetEnvironmentVariable(name.c_str(), &buf[0], (DWORD)buf.size());
	buf.back() = L'\0';

	return &buf[0];
}

std::wstring GetImageDir()
{
	std::wstring dir = GetEnvironmentVariable(L"VisionToolsRoot");
    if (dir[0] != L'\0')
    {
        dir += L"\\tests\\coretest\\images\\";
    }
    else 
    {
        dir = GetEnvironmentVariable(L"IMAGE_DIR");
    }

    if (dir[0] == L'\0')
    {
		dir = L"..\\..\\..\\..\\images\\";
    }
	
	if (dir.back() != L'\\')
		dir.push_back(L'\\');

	return dir;
}

std::wstring GetFileExtension(const std::wstring& path)
{
	return PathFindExtension(path.c_str());
}

std::wstring GetFileName(const std::wstring& path)
{
	const std::wstring ext = GetFileExtension(path);
	
	std::wstring name = PathFindFileName(path.c_str());
	if (ext.size() > 0)
		name.erase(name.rfind(ext), ext.size());

	return name;
}

void LoadImage(const std::wstring& path, CImg& img)
{
	WIN_ASSERT_EQUAL(S_OK, VtLoadImage(path.c_str(), img), L"VtLoadImage failed");
}

void SaveImage(const std::wstring& path, const CImg& img)
{
	WIN_ASSERT_EQUAL(S_OK, VtSaveImage(path.c_str(), img), L"VtSaveImage failed");
}
void SaveTwoImages(const vt::CImg& img1, const vt::CImg& img2)
{
	WIN_ASSERT_EQUAL(S_OK, VtSaveImage(L"tmpimg0.png", img1), L"VtSaveImage failed");
	WIN_ASSERT_EQUAL(S_OK, VtSaveImage(L"tmpimg1.png", img2), L"VtSaveImage failed");
}

void ShowMessage(const WCHAR* pszFormat, ...)
{
    WCHAR tmp[4096];
    va_list marker;
    va_start(marker, pszFormat);
    vswprintf_s(tmp, sizeof(tmp)/sizeof(WCHAR), pszFormat, marker);
    OutputDebugString(tmp);
    OutputDebugString(L"\n");
}

#else

// instantiated elsewhere for non-desktop app use

#endif

double GetAverageAbsoluteDifference(const vt::CImg& a, const vt::CImg& b)
{
	double dist = VT_MAXDOUBLE;
	
	WIN_ASSERT_EQUAL(S_OK, VtSADImages(a, b, dist));
	
	return dist / (a.Width() * a.Height());
}

//+-----------------------------------------------------------------------------
//
// helper function to generate a ramp image
//
//------------------------------------------------------------------------------
template <class T>
void GenerateRamp(CTypedImg<T>& dst)
{
	float fScale = 1.f/float(dst.Width()*dst.Height());
	float fBandInc = 1.f/float(dst.Bands());
    for( int y = 0; y < dst.Height(); y++ )
    {
        T* pDst = dst.Ptr(y);
        for( int x = 0; x < dst.Width(); x++ )
        {
			float fRampVal = x*y*fScale;
			for( int b = 0; b < dst.Bands(); b++, pDst++ )
			{
				VtConv(pDst, fRampVal);

				fRampVal+=fBandInc;
				if( fRampVal > 1.f )
				{
					fRampVal -= 1.f;
				}
			}
        }
    }
}

void GenerateRamp(CImg& img)
{
    switch( EL_FORMAT(img.GetType()) )
    {
    case EL_FORMAT_BYTE:
        GenerateRamp((CByteImg&)img);
        break;
	case EL_FORMAT_HALF_FLOAT:
		{
			CFloatImg imgF;
			imgF.Create(img.Width(), img.Height(), img.Bands());
			GenerateRamp(imgF);
			VtConvertImage(img, imgF);
		}
        break;
    case EL_FORMAT_SHORT:
        GenerateRamp((CShortImg&)img);
        break;
    case EL_FORMAT_FLOAT:
        GenerateRamp((CFloatImg&)img);
        break;
    }
}
