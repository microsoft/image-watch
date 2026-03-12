////////////////////////////////////////////////////////////////////////////////
// statstest.cpp : Unit tests for VisionTools functions from vt_stats.h
//

#include "stdafx.h"

#include "cthelpers.h"

using namespace vt;

BEGIN_TEST_FILE(StatsTest)

BEGIN_TEST_EX_FAST(MinMaxTest)
{
	TestFormats([](const CImg& in, const CImg&, CImg&)
	{
		if (IsColorImage(in))
			return;

		CImg tmp;
		in.CopyTo(tmp);
		
		float mxval = .67f;
		float ofs = 10.0f;
		VtScaleOffsetImage(tmp, in, mxval, ofs);

		float mn, mx;
		VtMinMaxImage(tmp, mn, mx);

		float typed_mxval = mxval;
		switch (EL_FORMAT(tmp.GetType()))
		{
		case EL_FORMAT_BYTE:
			typed_mxval *= 255; break;
		case EL_FORMAT_SHORT:
			typed_mxval *= 65535;
		}

		WIN_ASSERT_TRUE(fabs(ofs - mn)/(typed_mxval) < 1e-3);
		WIN_ASSERT_TRUE(fabs(typed_mxval + ofs - mx)/(typed_mxval + ofs) < 1e-3);
	});
}
END_TEST_EX;

BEGIN_TEST_EX_FAST(MinMaxColorTest)
{
	TestFormats([](const CImg& in, const CImg&, CImg&)
	{
		if (!IsColorImage(in))
			return;

		CImg tmp;
		in.CopyTo(tmp);
		
		float mxval = .67f;
		float ofs = 10.0f;
		VtScaleOffsetColorImage(tmp, in, 
			RGBAFloatPix(mxval, mxval, mxval, 1.0f), 
			RGBAFloatPix(ofs, ofs, ofs, 0.0f));

		RGBAFloatPix mnpix, mxpix;
		VtMinMaxColorImage(tmp, mnpix, mxpix);

		float typed_mxval = mxval;
		switch (EL_FORMAT(tmp.GetType()))
		{
		case EL_FORMAT_BYTE:
			typed_mxval *= 255; break;
		case EL_FORMAT_SHORT:
			typed_mxval *= 65535;
		}

		WIN_ASSERT_TRUE(fabs(ofs - mnpix.r)/(typed_mxval) < 1e-3);
		WIN_ASSERT_TRUE(fabs(typed_mxval + ofs - mxpix.r)/(typed_mxval + ofs) < 1e-3);
		WIN_ASSERT_TRUE(fabs(ofs - mnpix.g)/(typed_mxval) < 1e-3);
		WIN_ASSERT_TRUE(fabs(typed_mxval + ofs - mxpix.g)/(typed_mxval + ofs) < 1e-3);
		WIN_ASSERT_TRUE(fabs(ofs - mnpix.b)/(typed_mxval) < 1e-3);
		WIN_ASSERT_TRUE(fabs(typed_mxval + ofs - mxpix.b)/(typed_mxval + ofs) < 1e-3);
	});
}
END_TEST_EX;

END_TEST_FILE