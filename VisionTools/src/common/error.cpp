
#include "vt_error.h"
#include "vt_string.h"

using namespace vt;

const wchar_t* vt::VtErrorToString(    
    HRESULT hr, 
    __in_ecount(numBufElem) wchar_t* buf, 
    int numBufElem)
{
#define ERROR_CASE(code, text) \
	case code: VtStringPrintf(buf, numBufElem, text); return buf;

	switch (hr)
	{
		ERROR_CASE(E_INVALIDSRC, L"Invalid source image")
		ERROR_CASE(E_INVALIDDST, L"Invalid destination image")
		ERROR_CASE(E_NOINIT, L"Not initialized")
		ERROR_CASE(E_READFAILED, L"Read failed")
		ERROR_CASE(E_WRITEFAILED, L"Write failed")
		ERROR_CASE(E_BADFORMAT, L"Bad format")
		ERROR_CASE(E_TYPEMISMATCH, L"Type mismatch")
		ERROR_CASE(E_NOTFOUND, L"Not found")
		ERROR_CASE(E_NOCOMPRESSOR, L"No compressor")
		ERROR_CASE(E_TOOCOMPLEX, L"Too complex")
	}

#undef ERROR_CASE

#ifdef _MSC_VER
	if (0 != FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, numBufElem,
		NULL) )
    {
	    // remove annoying newline
	    for (size_t i = 0; i < wcslen(buf); ++i)
	    {
		    if (buf[i] == L'\n' || buf[i] == L'\r')
		    {
			    buf[i] = L'\0';
			    return buf;
		    }
	    }
    }
#else
	VtStringPrintf(buf, numBufElem, L"Unknown error: %d", hr);
#endif

	return buf;
}
