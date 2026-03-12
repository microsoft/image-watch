//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Mixed utility functions
//
//  History:
//      2003/11/12-swinder/mattu
//			Created
//
//------------------------------------------------------------------------

#include "stdafx.h"

#include "vt_mathutils.h"
#include "vt_matrix.h"

#if !defined(_MSC_VER) && !defined(abs)
template <typename T> T abs(const T& v) { return v >= 0 ? v : -v; }
#endif

using namespace vt;

double vt::VtWrapAngle(double dA)
{
	if(dA>-VT_PI && dA<=VT_PI)
		return dA;
	if(dA<0.0)
		return dA - 2.0*VT_PI * (double)( ((int)(dA / VT_PI) - 1)/2 );
	else
		return dA - 2.0*VT_PI * (double)( ((int)(dA / VT_PI) + 1)/2 );
}

float vt::VtWrapAngle(float fA)
{
	if(fA>-(float)VT_PI && fA<=(float)VT_PI)
		return fA;
	if(fA<0.0)
		return fA - 2.0f*(float)VT_PI * (float)( ((int)(fA / (float)VT_PI) - 1)/2 );
	else
		return fA - 2.0f*(float)VT_PI * (float)( ((int)(fA / (float)VT_PI) + 1)/2 );
}

// close polynomial approximation to the error function (|error| < 1e-7)
double vt::VtErf(double x)
{
    double t, z, ans;
    z = fabs(x);
    if(z>6)
        return x>0 ? 1 : -1;
    t=1.0/(1+0.5*z);
    ans = t*exp(-z*z-1.26551223+t*(1.00002368+t*(0.37409196+t*(0.09678418+
        t*(-0.18628806+t*(0.27886807+t*(-1.13520398+t*(1.48851587+
        t*(-0.82215223+t*0.17087277)))))))));
    return x>=0 ? 1-ans : -1+ans;
}

// computes the modified bessel function I_n(x) = (-i)^n J_n(ix) using polynomial approximation
double vt::VtModBessel(double x, int n)
{
    double q = fabs(x);

    n = abs(n);

    if(n==0)
    {
		double y;

        // compute I_0(x)
        if(q<3.75)
        {
            y = x/3.75;
            y = y*y;
            return 1 + y * (3.5156229 + y * (3.0899424 + y * (1.2067492 
                + y * (0.2659732 + y * (0.360768e-1 + y * 0.45813e-2)))));
        }
        else
        {
            y = 3.75/q;
            return (exp(q) / sqrt(q)) * (0.39894228 + y * (0.1328592e-1 + y * (0.225319e-2 
                + y * (-0.157565e-2 + y * (0.916281e-2 + y * (-0.2057706e-1 + y * (0.2635537e-1 
                + y * (-0.1647633e-1 + y * 0.392377e-2))))))));
        }
    }
    else if(n==1)
    {
		double y, val;

		// compute I_1(x)
        if(q<3.75)
        {
            y = x/3.75;
            y = y*y;
            val =  q * (0.5 + y * (0.87890594 + y * (0.51498869 + y * (0.15084934 + 
                y * (0.2658733e-1 + y * (0.301532e-2 + y * 0.32411e-3))))));
        }
        else
        {
            y = 3.75/q;
            val = 0.2282967e-1 + y * (-0.2895312e-1 + y * (0.1787654e-1 - y * 0.420059e-2));
            val = 0.39894228 + y * (-0.3988024e-1 + y * (-0.362018e-2 + y * (0.163801e-2 
                + y * (-0.1031555e-1 + y * val))));
            val *= (exp(q) / sqrt(q));
        }
        return x<0 ? -val : val;
    }
    else
    {
        // compute I_n(x) using downward recursion
        if(x==0)
            return 0;
        else
        {
            double u = 2/q;
            double val = 0;
            double bi = 1;
            double biprev = 0;
            int i;

            for(i = 2*(n + (int)sqrt(40.0*n)); i>0; i--)
            {
                double tmp = biprev + i*u*bi;
                biprev = bi;
                bi = tmp;
                if(fabs(bi)>1.0e10)
                {
                    // renormalize
                    val *= 1.0e-10;
                    bi *= 1.0e-10;
                    biprev *= 1.0e-10;
                }
                if(i==n)
                    val = biprev;
            }
            val *= VtModBessel(x, 0) / bi;
            return (x<0 && (n & 1)) ? -val : val;
        }
    }
}

// returns the highest common factor of two integers >0 using Euclid's algorithm
UInt32 vt::VtFactor(UInt32 a, UInt32 b)
{
    if(a<1 || b<1)
        return 1;
    while(b>0)
    {
        UInt32 x = b;
        b = a%b;
        a = x;
    }
    return a;
}

// return maximum value in array
float vt::VtArrayMax(float *pArray, int iNumEls, int *piIndexRtn)
{
#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)
    if( piIndexRtn==NULL && g_SupportSSE1() && iNumEls >= 4 )
    {
        int x = 0;
        float fMax = -VT_MAXFLOAT;
        __m128 xf1 = _mm_set1_ps(fMax);

        if(IsAligned16(pArray))
        {
            for(; x<(iNumEls-3); x+=4, pArray+=4)
            {
                __m128 xf2 = _mm_load_ps(pArray);
                xf1 = _mm_max_ps(xf1, xf2);
            }
        }
        else
        {
            for(; x<(iNumEls-3); x+=4, pArray+=4)
            {
                __m128 xf2 = _mm_loadu_ps(pArray);
                xf1 = _mm_max_ps(xf1, xf2);
            }
        }

        if( xf1.m128_f32[0]>fMax ) fMax = xf1.m128_f32[0];
        if( xf1.m128_f32[1]>fMax ) fMax = xf1.m128_f32[1];
        if( xf1.m128_f32[2]>fMax ) fMax = xf1.m128_f32[2];
        if( xf1.m128_f32[3]>fMax ) fMax = xf1.m128_f32[3];

        for(; x<iNumEls; x++, pArray++ )
        {
            if(*pArray>fMax)
                fMax = *pArray;
        }

        return fMax;
    }
#endif

    int i;
    int iInd = 0;
    float fMax = -VT_MAXFLOAT;
    for(i=0; i<iNumEls; i++)
        if(pArray[i] > fMax)
        {
            fMax = pArray[i];
            iInd = i;
        }
    if(piIndexRtn)
        *piIndexRtn = iInd;
    return fMax;
}

double vt::VtArrayMax(double *pArray, int iNumEls, int *piIndexRtn)
{
    int i;
    int iInd = 0;
    double fMax = -VT_MAXDOUBLE;
    for(i=0; i<iNumEls; i++)
        if(pArray[i] > fMax)
        {
            fMax = pArray[i];
            iInd = i;
        }
    if(piIndexRtn)
        *piIndexRtn = iInd;
    return fMax;
}

// return minimum value in array
float vt::VtArrayMin(float *pArray, int iNumEls, int *piIndexRtn)
{
#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(_MSC_VER)
    if( piIndexRtn==NULL && g_SupportSSE1() && iNumEls >= 4 )
    {
        int x = 0;
        float fMin = VT_MAXFLOAT;
        __m128 xf1 = _mm_set1_ps(fMin);

        if(IsAligned16(pArray))
        {
            for(; x<(iNumEls-3); x+=4, pArray+=4)
            {
                __m128 xf2 = _mm_load_ps(pArray);
                xf1 = _mm_min_ps(xf1, xf2);
            }
        }
        else
        {
            for(; x<(iNumEls-3); x+=4, pArray+=4)
            {
                __m128 xf2 = _mm_loadu_ps(pArray);
                xf1 = _mm_min_ps(xf1, xf2);
            }
        }

        if( xf1.m128_f32[0]<fMin ) fMin = xf1.m128_f32[0];
        if( xf1.m128_f32[1]<fMin ) fMin = xf1.m128_f32[1];
        if( xf1.m128_f32[2]<fMin ) fMin = xf1.m128_f32[2];
        if( xf1.m128_f32[3]<fMin ) fMin = xf1.m128_f32[3];

        for(; x<iNumEls; x++, pArray++ )
        {
            if(*pArray<fMin)
                fMin = *pArray;
        }

        return fMin;
    }
#endif

    int i;
    int iInd = 0;
    float fMin = VT_MAXFLOAT;
    for(i=0; i<iNumEls; i++)
        if(pArray[i] < fMin)
        {
            fMin = pArray[i];
            iInd = i;
        }
    if(piIndexRtn)
        *piIndexRtn = iInd;
    return fMin;
}

double vt::VtArrayMin(double *pArray, int iNumEls, int *piIndexRtn)
{
    int i;
    int iInd = 0;
    double fMin = VT_MAXDOUBLE;
    for(i=0; i<iNumEls; i++)
        if(pArray[i] < fMin)
        {
            fMin = pArray[i];
            iInd = i;
        }
    if(piIndexRtn)
        *piIndexRtn = iInd;
    return fMin;
}

// return maximum value of fabs(array[i])
float vt::VtArrayMaxAbs(float *pArray, int iNumEls, int *piIndexRtn)
{
    int i;
    int iInd = 0;
    float fMax = 0;
    for(i=0; i<iNumEls; i++)
    {
        float f = fabsf(pArray[i]);
        if(f > fMax)
        {
            fMax = f;
            iInd = i;
        }
    }
    if(piIndexRtn)
        *piIndexRtn = iInd;
    return fMax;
}

double vt::VtArrayMaxAbs(double *pArray, int iNumEls, int *piIndexRtn)
{
    int i;
    int iInd = 0;
    double fMax = 0;
    for(i=0; i<iNumEls; i++)
    {
        double f = fabs(pArray[i]);
        if(f > fMax)
        {
            fMax = f;
            iInd = i;
        }
    }
    if(piIndexRtn)
        *piIndexRtn = iInd;
    return fMax;
}

// return minimum value of fabs(array[i])
float vt::VtArrayMinAbs(float *pArray, int iNumEls, int *piIndexRtn)
{
    int i;
    int iInd = 0;
    float fMin = VT_MAXFLOAT;
    for(i=0; i<iNumEls; i++)
    {
        float f = fabsf(pArray[i]);
        if(f < fMin)
        {
            fMin = f;
            iInd = i;
        }
    }
    if(piIndexRtn)
        *piIndexRtn = iInd;
    return fMin;
}

double vt::VtArrayMinAbs(double *pArray, int iNumEls, int *piIndexRtn)
{
    int i;
    int iInd = 0;
    double fMin = VT_MAXDOUBLE;
    for(i=0; i<iNumEls; i++)
    {
        double f = fabs(pArray[i]);
        if(f < fMin)
        {
            fMin = f;
            iInd = i;
        }
    }
    if(piIndexRtn)
        *piIndexRtn = iInd;
    return fMin;
}

