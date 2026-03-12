//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for kernels
//
//  History:
//      2008/2/27-swinder
//          Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_image.h"
#include "vt_kernel.h"

using namespace vt;

////////////////////////////////////////////////////////////////////////////////
// C1dKernel
////////////////////////////////////////////////////////////////////////////////
HRESULT C1dKernel::Create(int iTaps, int iCenter, const float* pfK)
{
    if(iTaps<1 || iCenter<0 || iCenter>=iTaps)
        return E_INVALIDARG;

    if( m_cMem.Alloc(iTaps * sizeof(float), align16Byte, true) == NULL )
    {
        m_iCenter = 0;
        m_iTaps = 0;
        return E_OUTOFMEMORY;
    }

    m_iTaps = iTaps;
    m_iCenter = iCenter;

	memset(Ptr(), 0, sizeof(float)*m_iTaps);
    Ptr()[m_iCenter] = 1; // default to impulse

    if( pfK )
    {
        Set(pfK);
    }

    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////
// Create1dGaussKernel
////////////////////////////////////////////////////////////////////////////////
HRESULT 
vt::Create1dGaussKernel(C1dKernel& dst, float fSigma, int iDeriv, float fKW)
{
    if(fSigma<0 || iDeriv<0 || iDeriv>4 || fKW<1)
        return E_INVALIDARG;
    if(fSigma==0 && iDeriv>0)
        return E_INVALIDARG;

    int iSize_2 = (int)(fSigma*fKW);
    if(iSize_2<1)
        iSize_2 = 1;
    int iSize = 1 + 2*iSize_2;

    HRESULT hr = dst.Create(iSize, iSize_2);
    if(FAILED(hr))
        return hr;

    if(fSigma==0 && iDeriv==0)
        return NOERROR; // filter is an impulse

    int i;
    if(iDeriv==0)
    {
        float fSum = 0.0;
        double fSigx = fSigma * VT_SQRT2;
        //double fSigs = fSigma * fSigma;
        //double fConst = exp(fSigs);
        for (i=-iSize_2;i<=iSize_2; i++)
        {
            fSum += dst[iSize_2+i] = 
                (float)(0.5*(VtErf((i+0.5)/fSigx) - VtErf((i-0.5)/fSigx)));
        }
        for (i=-iSize_2;i<=iSize_2; i++)
        {
            dst[iSize_2+i] /= fSum;
        }
    }
    else if(iDeriv==1)
    {
        for (i=-iSize_2;i<=iSize_2; i++)
        {
            dst[iSize_2+i] = (float)(VtGauss1D(i+0.5, (double)fSigma) - 
                                     VtGauss1D(i-0.5, (double)fSigma));
        }
    }
    else if(iDeriv==2)
    {
        double fRcps = 1.0/(fSigma*fSigma);
        for (i=-iSize_2;i<=iSize_2; i++)
        {
            double fXm = i-0.5;
            double fXp = i+0.5;
            dst[iSize_2+i] = (float)(fRcps*((-fXp)*VtGauss1D(fXp, (double)fSigma) - 
                                            (-fXm)*VtGauss1D(fXm, (double)fSigma)));
        }
    }
    else if(iDeriv==3)
    {
        double fRcps = 1.0/(fSigma*fSigma);
        for (i=-iSize_2;i<=iSize_2; i++)
        {
            double fXm = i-0.5;
            double fXp = i+0.5;
            dst[iSize_2+i] = (float)(fRcps*((fXp*fXp*fRcps - 1) * VtGauss1D(fXp, (double)fSigma) -
                                            (fXm*fXm*fRcps - 1) * VtGauss1D(fXm, (double)fSigma) ));
        }
    }
    else
    {
        double fRcps = 1.0/(fSigma*fSigma);
        for (i=-iSize_2;i<=iSize_2; i++)
        {
            double fXm = i-0.5;
            double fXp = i+0.5;
            dst[iSize_2+i] = 
                (float)(fRcps*fRcps*((-fXp*fXp*fXp*fRcps + 3*fXp) * VtGauss1D(fXp, (double)fSigma) 
                                     - (-fXm*fXm*fXm*fRcps + 3*fXm) * VtGauss1D(fXm, (double)fSigma) ));
        }
    }

    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////
// C1dGaussBesselKernel
////////////////////////////////////////////////////////////////////////////////
HRESULT 
vt::Create1dGaussBesselKernel(C1dKernel& dst, float fSigma, float fKW)
{
    if(fSigma<0 || fKW<1)
        return E_INVALIDARG;

    int iSize_2 = (int)(fSigma*fKW);
    if(iSize_2<1)
        iSize_2 = 1;
    int iSize = 1 + 2*iSize_2;

    HRESULT hr = dst.Create(iSize, iSize_2);
    if(FAILED(hr))
        return hr;

    if(fSigma==0)
        return NOERROR; // filter is an impulse

    int i;
    float fSum = 0;
    if(fSigma<5)
    {
        // use modified bessel function
        double fSigs = fSigma * fSigma;
        for (i=-iSize_2;i<=iSize_2; i++)
        {
            fSum += dst[iSize_2+i] = (float)(VtModBessel(fSigs, i));
        }
    }
    else
    {
        // use Gaussian because mod Bessel overflows for large sigma
        // and Gaussian is good approximation at this size
        double fSigx = fSigma * VT_SQRT2;
        for (i=-iSize_2;i<=iSize_2; i++)
        {
            fSum += dst[iSize_2+i] = (float)(0.5*(VtErf((i+0.5)/fSigx) - 
                                                  VtErf((i-0.5)/fSigx)));
        }
    }
    for (i=-iSize_2;i<=iSize_2; i++)
    {
        dst[iSize_2+i] /= fSum;
    }

    return NOERROR;
}



///////////////////////////////////////////////////////////////////////////////
//  Class C1dKernelSet
///////////////////////////////////////////////////////////////////////////////
HRESULT
C1dKernelSet::Create(int iCycle, int iCoordShiftPerCycle)
{
    HRESULT hr;

    VT_HR_EXIT( m_vecKernelSet.resize(iCycle) );
    VT_HR_EXIT( m_vecCoordSet.resize(iCycle) );

    m_iCoordShiftPerCycle = iCoordShiftPerCycle;

Exit:
    return hr;
}

HRESULT
C1dKernelSet::Create(const C1dKernelSet& ks)
{
    HRESULT hr;

    VT_HR_EXIT( Create( ks.GetCycle(), ks.GetCoordShiftPerCycle() ) );

    for( UInt32 i = 0; i < ks.GetCycle(); i++ )
    {
        VT_HR_EXIT( Set(i, ks.GetCoord(i), ks.GetKernel(i)) );
    }

Exit:
    return hr;
}

HRESULT
C1dKernelSet::Create(const C1dKernel& k)
{
    HRESULT hr;
    VT_HR_EXIT( Create( 1, 1 ) );

    VT_HR_EXIT( Set(0, -k.Center(), k) );

Exit:
    return hr;
}

HRESULT
C1dKernelSet::Set(UInt32 uIndex, int iCoord, const C1dKernel& krnl)
{
    if( uIndex >= m_vecKernelSet.size() )
    {
        VT_ASSERT(0);
        return E_INVALIDARG;
    }

	VT_HR_BEGIN()

	VT_HR_EXIT(krnl.Ptr() != NULL ? S_OK : E_INVALIDARG);

	VT_HR_EXIT( m_vecKernelSet[uIndex].Create(krnl) );
    m_vecCoordSet[uIndex]  = iCoord;

    VT_HR_END()
}

HRESULT
C1dKernelSet::Set(UInt32 uIndex, int iCoord, int iTaps, const float *pfK)
{
    HRESULT hr;

    if( uIndex >= m_vecKernelSet.size() )
    {
        VT_ASSERT(0);
        return E_INVALIDARG;
    }

    C1dKernel& krnl = m_vecKernelSet[uIndex];
    VT_HR_EXIT( krnl.Create(iTaps, 0, pfK) );
    m_vecCoordSet[uIndex]  = iCoord;

Exit:
    return hr;
}

C1dKernel&
C1dKernelSet::GetKernel(UInt32 uIndex)
{
    VT_ASSERT( uIndex < m_vecKernelSet.size() );
    return m_vecKernelSet[uIndex];
}

const C1dKernel&
C1dKernelSet::GetKernel(UInt32 uIndex) const
{
    VT_ASSERT( uIndex < m_vecKernelSet.size() );
    return m_vecKernelSet[uIndex];
}

int
C1dKernelSet::GetCoord(UInt32 uIndex) const
{
    VT_ASSERT( uIndex < m_vecKernelSet.size() );
    return m_vecCoordSet[uIndex];
}

void GetKernelSetPosition(OUT UInt32& uIndexInCycle, OUT int& iSrcStart, 
						  int iDstStart, const C1dKernelSet& ks)
{
	// this function divides the supplied dst coord by the cycle count and
	// rounds to neg-infinity to find the first coordinate in the cycle
	int iCycleLen   = (int)ks.GetCycle();
	int iCycle      = iDstStart / iCycleLen;
	int iCycleStart = iCycle * iCycleLen;
	int iCycleRem   = iDstStart - iCycleStart;
	if( iCycleRem < 0 )
	{
		uIndexInCycle = (UInt32)(iCycleRem + iCycleLen);
		iSrcStart     = (iCycle-1) * ks.GetCoordShiftPerCycle();
	}
	else
	{
		uIndexInCycle = (UInt32)iCycleRem;
		iSrcStart     = iCycle * ks.GetCoordShiftPerCycle();
	}
}

void C1dKernelSet::GetSourceRegionNoClipping(int dx, int iDstTileWidth, int &sx, 
											 int &iSrcTileWidth) const
{
	// from the kernel and dx and dsttilewidth we can calculate what left 
    // coordinate and width of the source region of pixels are needed. We 
    // dont clip to the source image size
	sx = 0;
	iSrcTileWidth = 0;

	UInt32 i;      // index number within the cycle for given dx
    int iOrigin; // source x coordinate at the start of this cycle
	GetKernelSetPosition(i, iOrigin, dx, *this);

	// extents of the first kernel
	int iStart = iOrigin + GetCoord(i); // may be negative
	int iEnd = iStart + GetKernel(i).Width();

	sx = iStart;
	int ex = iEnd;

	int x;
	for(x=1; x<iDstTileWidth; x++)
	{
		i++;
		if(i==GetCycle())
		{
			i = 0;
			iOrigin += GetCoordShiftPerCycle();
		}

		iStart = iOrigin + GetCoord(i);
		iEnd = iStart + GetKernel(i).Width();

		if(iStart<sx)
			sx = iStart;
		if(iEnd>ex)
			ex = iEnd;
	}

	iSrcTileWidth = ex - sx;
}

void C1dKernelSet::GetSourceRegion(int iTotalSrcWidth, int dx, int iDstTileWidth, 
                                   int &sx, int &iSrcTileWidth) const
{
	// from the kernel and dx and dsttilewidth we can calculate where the min 
    // and max extended region will come from in the source image. then we
    // just clip this range against the total source width. this all assumes 
    // the kernel origin is at the left of the total source width

	sx = 0;
	iSrcTileWidth = 0;
	if(dx<0)
		return; // probably should be an error

	// source origin and cycle offset of kernel when generating the first
	// pixel of the output image
	UInt32 uFirstIndex = dx % GetCycle();
    int iSrcOrgX = GetCoordShiftPerCycle() * (dx / GetCycle()); 

	UInt32 i = uFirstIndex;

	// start with the extents of the first kernel
	int iStart = GetCoord(i);
	int iEnd = GetCoord(i) + GetKernel(i).Width();

	// search up to min(width image, kernel count)
	// for minimum left edge value
	int x;
	int iMinWidth = VtMin(iDstTileWidth, (int)GetCycle());
	for(x=1; x<iMinWidth; x++)
	{
		i = (i+1) % GetCycle();
		int iCoord = GetCoord(i);
		if(iCoord<iStart)
			iStart = iCoord;
	}

	// skip to near end of span, leaving at maximum kernel count remaining
	x = iDstTileWidth - iMinWidth;

	// determine index and origin for this starting location
	int iCycles = x / GetCycle();
	i = uFirstIndex + x % GetCycle();
	if(i >= GetCycle())
	{
		i -= GetCycle();
		iCycles++;
	}
	int iOrigin = iCycles * GetCoordShiftPerCycle();

	// search trailing output pixels to determine maximum right edge coordinate
	for(; x<iDstTileWidth; x++)
	{
		int iR = iOrigin + GetCoord(i) + GetKernel(i).Width();
		if(iR>iEnd)
			iEnd = iR;

		i++;
		if(i==GetCycle())
		{
			i = 0;
			iOrigin += GetCoordShiftPerCycle();
		}
	}

	iStart += iSrcOrgX;
	iEnd += iSrcOrgX;

	// clip to total source image bounds
	if(iStart < 0)
		iStart = 0;
	if(iEnd > iTotalSrcWidth)
		iEnd = iTotalSrcWidth;

	sx = iStart;
	iSrcTileWidth = iEnd - iStart;
}

void C1dKernelSet::GetDestinationRegion(int sx, int iSrcTileWidth, 
                                        int &dx, int &iDstTileWidth) const
{
	// Given the source sx and source tile width where a change has been made 
    // in the source image, this function calculates which destination pixels 
    // (dx and destination tile width) will change if the filter is applied.

	dx = 0;
	iDstTileWidth = 0;
	if(sx<0 || iSrcTileWidth < 1)
		return;

	UInt32 i = 0;
	int x = 0;
	int iCycles = 0;
	int iOrigin = 0;

	// brute force search for the first cycle where there is overlap
	int iStart, iEnd;
	for (;;)
	{
		iStart = iOrigin + GetCoord(i);
		iEnd = iStart + GetKernel(i).Width();
		if(iEnd > sx)
			break;

		i++;
		x++;
		if(i==GetCycle())
		{
			i=0;
			iCycles++;
			iOrigin += GetCoordShiftPerCycle();
		}
	}

	dx = x;

	int iCount = 0;

	do {
		i++;
		x++;
		if(i==GetCycle())
		{
			i=0;
			iCycles++;
			iOrigin += GetCoordShiftPerCycle();
		}

		iStart = iOrigin + GetCoord(i);
		if(iStart >= sx + iSrcTileWidth)
			iCount++;
		else
			iCount = 0;
	} while(iCount < (int)GetCycle()); // ensure the whole cycle is outside the span

	iDstTileWidth = x - dx - iCount + 1;
}

//+-----------------------------------------------------------------------------
//
// function: CreateKernelSet
//
//------------------------------------------------------------------------------
HRESULT CreateKernelSet(C1dKernelSet& dst, int iInputSamples, int iOutputSamples, 
                        int iSize_2, bool bNormalize, float fPhase,
                        float (*func)(float, void *), void *userdata)
{
	HRESULT hr = NOERROR;

	if(iInputSamples <= 0 || iOutputSamples <= 0)
		VT_HR_RET( E_INVALIDARG );

	int iFactor = (int)VtFactor(iInputSamples, iOutputSamples);
	iInputSamples /= iFactor;
	iOutputSamples /= iFactor;

	VT_HR_RET( dst.Create(iOutputSamples, iInputSamples) );

    iSize_2 = VtMax(iSize_2, 1);
    int iSize = 1 + 2*iSize_2;

	vt::vector<float> vK;
	VT_HR_RET( vK.resize(iSize+1) );

	for(int i=0; i<iOutputSamples; i++)
	{
		float fOffset = fPhase + (i * iInputSamples)/(float)iOutputSamples;
		int iCoord = (int)floor(fOffset) - iSize_2;
		int iTaps = iSize + ((fOffset==0) ? 0 : 1);
		float fSum = 0;
		float fMax = 0;
		for(int x=0; x<iTaps; x++)
		{
			float f = (*func)(iCoord - fOffset + x, userdata);
			vK[x] = f;
			fSum += f;
            fMax = VtMax(fMax, (float)fabs(f));
		}
		if(fSum > 0 && bNormalize)
        {
			for(int x=0; x<iTaps; x++)
            {
                vK[x] /= fSum;
            }
        }

#define ZEROTRIM_EPSILON	1e-8

		// trim any leading or trailing zeros
		float *pfKTrim = vK.begin();
		while(iTaps>1 && fabs(pfKTrim[0]) < fMax * ZEROTRIM_EPSILON)
		{
			pfKTrim++;
			iCoord++;
			iTaps--;
		}
	
		while(iTaps>1 && fabs(pfKTrim[iTaps-1]) < fMax * ZEROTRIM_EPSILON)
        {
			iTaps--;
        }

#undef ZEROTRIM_EPSILON

		VT_HR_RET( dst.Set(i, iCoord, iTaps, pfKTrim) );
	}

    return hr;
}

//+-----------------------------------------------------------------------------
//
// function: Create1dGaussianKernelSet
//
//------------------------------------------------------------------------------
float FilterFunction_Triangle(float x, void *pUser)
{
	if(pUser==NULL)
		return 0;
	float fWidth = *(float *)pUser;
	return VtMax(float(1.f - fabs(x)/fWidth), 0.f);
}

float FilterFunction_Gaussian(float x, void *pUser)
{
	if(pUser==NULL)
		return 0;
	float fSigma = *(float *)pUser;
    double fSigx = fSigma * VT_SQRT2;
	return (float)(0.5*(VtErf((x+0.5)/fSigx) - VtErf((x-0.5)/fSigx)));
}

float FilterFunction_Gaussian_Deriv1(float x, void *pUser)
{
	if(pUser==NULL)
		return 0;
	float fSigma = *(float *)pUser;
	return (float)(VtGauss1D(x+0.5, (double)fSigma) - 
                   VtGauss1D(x-0.5, (double)fSigma));
}

float FilterFunction_Gaussian_Deriv2(float x, void *pUser)
{
	if(pUser==NULL)
		return 0;
	float fSigma = *(float *)pUser;
	double fRcps = 1.0/(fSigma*fSigma);
	double fXm = x-0.5;
	double fXp = x+0.5;
    return (float)(fRcps*( (-fXp)*VtGauss1D(fXp, (double)fSigma) - 
                           (-fXm)*VtGauss1D(fXm, (double)fSigma) ));
}

float FilterFunction_Gaussian_Deriv3(float x, void *pUser)
{
	if(pUser==NULL)
		return 0;
	float fSigma = *(float *)pUser;
	double fRcps = 1.0/(fSigma*fSigma);
	double fXm = x-0.5;
	double fXp = x+0.5;
    return (float)(fRcps*( (fXp*fXp*fRcps - 1) * VtGauss1D(fXp, (double)fSigma)
                - (fXm*fXm*fRcps - 1) * VtGauss1D(fXm, (double)fSigma) ));
}

float FilterFunction_Gaussian_Deriv4(float x, void *pUser)
{
	if(pUser==NULL)
		return 0;
	float fSigma = *(float *)pUser;
	double fRcps = 1.0/(fSigma*fSigma);
	double fXm = x-0.5;
	double fXp = x+0.5;
    return (float)(fRcps*fRcps*( (-fXp*fXp*fXp*fRcps + 3*fXp) * 
                                 VtGauss1D(fXp, (double)fSigma) 
                                 - (-fXm*fXm*fXm*fRcps + 3*fXm) *
                                 VtGauss1D(fXm, (double)fSigma) ));
}

HRESULT
vt::Create1dGaussianKernelSet(C1dKernelSet& dst, 
                              int iInputSamples, int iOutputSamples, 
                              float fSigma, int iDeriv, float fKW,
                              float fPhase)
{
    if(fSigma<0 || iDeriv<0 || iDeriv>4 || fKW<1)
        return E_INVALIDARG;
    if(fSigma==0 && iDeriv>0)
        return E_INVALIDARG;

	float (*pFunc)(float, void *);
	switch(iDeriv)
	{
	case 0:
		if(fSigma==0)
		{
			fSigma = 1;
			pFunc = FilterFunction_Triangle;
		}
		else
			pFunc = FilterFunction_Gaussian;
		break;
	case 1:
		pFunc = FilterFunction_Gaussian_Deriv1;
		break;
	case 2:
		pFunc = FilterFunction_Gaussian_Deriv2;
		break;
	case 3:
		pFunc = FilterFunction_Gaussian_Deriv3;
		break;
	default:
		pFunc = FilterFunction_Gaussian_Deriv4;
		break;
	}

    return CreateKernelSet(dst, iInputSamples, iOutputSamples, (int)(fSigma*fKW), 
                           iDeriv==0, fPhase, pFunc, &fSigma);
}

//+-----------------------------------------------------------------------------
//
// function: Create1dLanczosKernelSet
//
//------------------------------------------------------------------------------
struct LanczosParams 
{
	float fWidth;
	int iA;
};

float FilterFunction_Lanczos(float x, void *pUser)
{
	if(pUser==NULL)
		return 0;
	LanczosParams *p = (LanczosParams *)pUser;
	if(x==0)
		return 1;
	x = fabsf(x) / p->fWidth;
	if(x > p->iA)
		return 0;
	double pix = VT_PI * x;
	return (float)(p->iA * sin(pix) * sin(pix/p->iA) / (pix*pix));
}

HRESULT
vt::Create1dLanczosKernelSet(C1dKernelSet& dst, int iInputSamples, 
                             int iOutputSamples, int a, float fPhase)
{
	LanczosParams lanc;
	lanc.fWidth = VtMax(float(iInputSamples) / float(iOutputSamples), 1.0f);

    a = VtMax(a, 2);
	lanc.iA = a;

    // kernel will get auto-trimmed to remove any zeros
	int size_2 = (int)ceil(a * lanc.fWidth);

	return CreateKernelSet(dst, iInputSamples, iOutputSamples, size_2, true,
                           fPhase, FilterFunction_Lanczos, &lanc);
}

//+-----------------------------------------------------------------------------
//
// function: Create1dBilinearKernelSet
//
//------------------------------------------------------------------------------
HRESULT 
vt::Create1dBilinearKernelSet(C1dKernelSet& dst, int iInputSamples,
                              int iOutputSamples, float fPhase)
{
	float fWidth = VtMax(float(iInputSamples) / float(iOutputSamples), 1.0f);
	int size_2   = (int)ceil(fWidth);

	return CreateKernelSet(dst, iInputSamples, iOutputSamples, size_2, true,
                           fPhase, FilterFunction_Triangle, &fWidth);
}

//+-----------------------------------------------------------------------------
//
// function: Create1dBicubicKernelSet
//
//------------------------------------------------------------------------------
float FilterFunction_Cubic(float x, void *pUser)
{
	if(pUser==NULL)
		return 0;
	float fWidth = *(float *)pUser;
	if(x==0)
		return 1;
    x = fabsf(x)/fWidth;
	if(x>=2)
		return 0;
	float xx = x*x;
	float a = -0.5f;
	if(x<1)
		return (a+2)*x*xx - (a+3)*xx + 1;
	else
		return a*(x*xx - 5*xx + 8*x - 4);
}

HRESULT 
vt::Create1dBicubicKernelSet(C1dKernelSet& dst, int iInputSamples,
                             int iOutputSamples, float fPhase)
{
	float fWidth = VtMax( float(iInputSamples) / float(iOutputSamples), 1.0f);
	int size_2 = (int)ceil(2 * fWidth);

	return CreateKernelSet(dst, iInputSamples, iOutputSamples, size_2, true,
                           fPhase, FilterFunction_Cubic, &fWidth);
}

//+-----------------------------------------------------------------------------
//
// function: Create1dBicubicBSplineKernelSet
//
//------------------------------------------------------------------------------
float FilterFunction_BicubicBSpline(float x, void *pUser)
{
	if(pUser==NULL)
		return 0;
	float fWidth = *(float *)pUser;
	if(x==0)
		return 4.0f / 6.0f;
    x = fabsf(x)/fWidth;
	if(x>=2.0f)
		return 0;

    if(x >= 1.0f)
    {
        x = 2.0f - x;
        return x * x * x / 6.0f;
    }

    x = 1.0f - x;
    float x2 = x * x;

    return (1.0f + 3.0f * (x + x2 - x * x2)) / 6.0f;
}

HRESULT 
vt::Create1dBicubicBSplineKernelSet(C1dKernelSet& dst, int iInputSamples,
                             int iOutputSamples, float fPhase)
{
	float fWidth = VtMax( float(iInputSamples) / float(iOutputSamples), 1.0f);
	int size_2 = (int)ceil(2 * fWidth);

	return CreateKernelSet(dst, iInputSamples, iOutputSamples, size_2, true,
                           fPhase, FilterFunction_BicubicBSpline, &fWidth);
}


//+-----------------------------------------------------------------------------
//
// function: Create1dTriggsKernelSet
//
//------------------------------------------------------------------------------
struct TriggsParams 
{
	float *pfWeights;
	int iCycles;
	float fWidth;
};

float FilterFunction_Triggs(float x, void *pUser)
{
	if(pUser==NULL)
		return 0;
	TriggsParams *p = (TriggsParams *)pUser;
	if(x==0)
		return 1;
    x = fabsf(x)/p->fWidth;
	int c = (int)x;
	if(c >= p->iCycles)
		return 0;
	float pix = (float)VT_PI * x;
	return p->pfWeights[c] * sinf(pix)/pix;
}

HRESULT 
vt::Create1dTriggsKernelSet(C1dKernelSet& dst, int iInputSamples,
                            int iOutputSamples, int iCycles, float fPhase)
{
	TriggsParams tp;

	static float rgfCycle4[] = {1, 0.95f, 0.87f, 0.46f};
    static float rgfCycle5[] = {1, 0.97f, 0.91f, 0.82f, 0.44f};
    static float rgfCycle6[] = {1, 0.99f, 0.95f, 0.9f, 0.82f, 0.44f};

	tp.iCycles = iCycles;
	switch(iCycles)
	{
	case 4:
		tp.pfWeights = rgfCycle4;
		break;
	case 5:
		tp.pfWeights = rgfCycle5;
		break;
	case 6:
		tp.pfWeights = rgfCycle6;
		break;
	default:
		return E_INVALIDARG;
		break;
	}
	
	tp.fWidth  = VtMax( float(iInputSamples) / float(iOutputSamples), 1.0f);
	int size_2 = (int)ceil(iCycles * tp.fWidth);

	return CreateKernelSet(dst, iInputSamples, iOutputSamples, size_2, true, 
                           fPhase, FilterFunction_Triggs, &tp);
}
