//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Mixed utility functions
//
//  History:
//      2003/11/12-swinder/mattu
//          Created
//
//------------------------------------------------------------------------
#pragma once

#include "vtcommon.h"

#include "vt_complex.h"

#include <math.h>

#define VT_E        2.7182818284590452353602874713526624977572
#define VT_LOG2E    1.4426950408889634073599246810018921374458
#define VT_LOG10E   0.4342944819032518276511289189166050822944
#define VT_LN2      0.6931471805599453094172321214581765680663
#define VT_LN10     2.3025850929940456840179914546843642075803
#define VT_PI       3.1415926535897932384626433832795028841972
#define VT_PI_2     1.5707963267948966192313216916397514421008
#define VT_PI_4     0.7853981633974483096156608458198757210504
#define VT_PI_180   0.0174532925199432957692369076848861271344
#define VT_1_PI     0.3183098861837906715377675267450287240657
#define VT_2_PI     0.6366197723675813430755350534900574481369
#define VT_180_PI   57.295779513082320876798154814105170332418
#define VT_2_SQRTPI 1.1283791670955125738961589031215451716780
#define VT_SQRT2    1.4142135623730950488016887242096980785983
#define VT_SQRT1_2  0.7071067811865475244008443621048490392684
#define VT_SQRT3    1.7320508075688772935274463415058723669689
#define VT_SQRT3_2  0.8660254037844386467637231707529361834845
#define VT_SQRT2PI  2.5066282746310005024157652848110452529864

#define VT_MAXFLOAT (3.40282346638528860e+38)
#define VT_MAXDOUBLE (1.79769313486231570e+308)

namespace vt {

	template <class ET> class CVec2;
	template <class ET> class CVec3;
	template <class ET> class CMtx2x2;
	template <class ET> class CMtx3x3;
	template <class ET> class CVec;
	template <class ET> class CMtx;

	typedef CVec<float> CVecf;
	typedef CVec<double> CVecd;
	typedef CMtx<float> CMtxf;
	typedef CMtx<double> CMtxd;
	typedef CVec<Complex<float> > CVeccf;
	typedef CVec<Complex<double> > CVeccd;
	typedef CMtx<Complex<float> > CMtxcf;
	typedef CMtx<Complex<double> > CMtxcd;

	//------------------------------------------------------------------------------
	//
	// Math utils
	//
	//------------------------------------------------------------------------------

    inline float Fabs(float f) { return fabsf(f); }
    inline double Fabs(double d) { return fabs(d); }

    inline float Sqrt(float f) { return sqrtf(f); }
    inline double Sqrt(double d) { return sqrt(d); }

    inline void VtSinCos(float rad, float* pfSin, float* pfCos)
	{
#if defined(_M_IX86) && defined(_MSC_VER)
		_asm
		{
			mov eax, pfSin
				mov edx, pfCos
				fld rad
				fsincos
				//_emit 0xd9
				//_emit 0xfb
				fstp dword ptr [edx]
			fstp dword ptr [eax]
		}
#else
		*pfSin = (float)sin( rad );
		*pfCos = (float)cos( rad );
#endif
	}

	inline void VtSinCos(double rad, double* pfSin, double* pfCos)
	{
		*pfSin = sin( rad );
		*pfCos = cos( rad );
	}

	template <class T>
	void VtSinCos(const Complex<T> &rad, Complex<T> *pfSin, Complex<T> *pfCos)
	{
		if(rad.Im==0)
		{
			VtSinCos(rad.Re, &(pfSin->Re), &(pfCos->Re));
			pfSin->Im = pfCos->Im = 0;
		}
		else
		{
			T fS, fC;
			VtSinCos(rad.Re, &fS, &fC);
			T fExpIm = exp(rad.Im);
			T fExpNegIm = exp(-rad.Im);
			T fMul1 = ((T)0.5) * (fExpIm + fExpNegIm);
			T fMul2 = ((T)0.5) * (fExpIm - fExpNegIm);
			pfSin->Re = fMul1 * fS;
			pfSin->Im = fMul2 * fC;
			pfCos->Re = fMul1 * fC;
			pfCos->Im = -fMul2 * fS;
		}
	}

	inline bool IsNan(float f)
	{
		int fi = *(int*)&f; 
		return ((fi & 0x7f800000) == (0x7f800000)) && ((fi & 0x007fffff) != 0);
	}

	inline bool IsInf(float f)
	{
		int fi = *(int*)&f; 
		return ((fi & 0x7f800000) == (0x7f800000)) && ((fi & 0x007fffff) == 0);
	}

	// calculate hypotenuse in sensible way
	template <class T>
	T VtHypot(T x, T y)
	{
		if (x < T(0)) x = -x;
		if (y < T(0)) y = -y;

		if (x > y)
		{
			T t = y / x;
			return x * Sqrt(T(1) + t*t);
		}
		else
		{
			if (y > T(0))
			{
				T t = x / y;
				return y * Sqrt(T(1) + t*t);
			}
			else
				return x;
		}
	}

	template <class T>
	Complex<T> VtHypot(const Complex<T> &x, const Complex<T> &y)
	{
		return Complex<T>(sqrt(x.MagnitudeSq() + y.MagnitudeSq()), (T)0);
	}

	double VtWrapAngle(double dAngle); // brings angle into range -pi to +pi
	float VtWrapAngle(float fAngle); // brings angle into range -pi to +pi

	template <class T> 
	Complex<T> VtSqrt(const Complex<T> &v)
	{
		/*if(v.Im==0)
		{
		if(v.Re>=0)
		return Complex<T>(sqrt(v.Re), (T)0);
		else
		return Complex<T>((T)0, sqrt(-v.Re));
		}
		T mag = v.Magnitude();
		T sign = v.Im>=0 ? (T)1 : (T)-1;
		return Complex<T>((T)(VT_SQRT2/2.0) * sqrt(mag + v.Re), sign * (T)(VT_SQRT2/2.0) * sqrt(mag - v.Re));
		*/
		if(v.Re>=0 && v.Im==0)
			return Complex<T>(sqrt(v.Re), (T)0);

		T ModRe = fabs(v.Re);
		T ModIm = fabs(v.Im);
		T w;
		if(ModRe>=ModIm)
		{
			T dc = ModIm/ModRe;
			w = sqrt(ModRe) * sqrt((T)0.5*(1+sqrt(1+dc*dc)));
		}
		else
		{
			T cd = ModRe/ModIm;
			w = sqrt(ModIm) * sqrt((T)0.5*(cd+sqrt(1+cd*cd)));
		}
		if(v.Re>=0)
			return Complex<T>(w, v.Im / (2*w));

		T q = v.Im>=0 ? w : -w;
		return Complex<T>(v.Im/(2*q), q);
	}

	template <class T>
	T VtGauss1D(T x, T sigma)
	{
		// 2.5066... = sqrt(2*pi)
		return exp(-x*x/(2*sigma*sigma))/(sigma*(T)VT_SQRT2PI);
	}

	template <class T>
	T VtGauss2D(T x, T y, T sigma)
	{
		T sq = sigma * sigma;
		return exp(-(x*x+y*y)/(2*sq))/(sq*(2*(T)VT_PI));
	}

	template <class T>
	T VtGauss2DCov(T x, T y, const CMtx2x2<T> &m)
	{
		T det = m.Det();
		T norm = 2*(T)VT_PI*sqrt(det);
		return exp(-(m(1,1)*x*x - 2*m(0,1)*x*y + m(0,0)*y*y)/(2*det))/norm;
	}

	template <class T>
	T VtGaussNDCov(const CVec<T> &v, const CMtx<T> &m)
	{
		T norm = sqrt(pow(2*(T)VT_PI, v.Size())*m.Det());
		return exp(-(v*(m.Inv()*v))/2)/norm;
	}

	template <class T>
	T VtGaussNDInvCov(const CVec<T> &v, const CMtx<T> &m)
	{
		T norm = sqrt(pow(2*(T)VT_PI, v.Size())*(1/m.Det()));
		return exp(-(v*(m*v))/2)/norm;
	}

	// returns an approximation to the symmetric (Gaussian integral) error function
	// this is a polynomial approximation with |error| < 1e-7 for all x.
	double VtErf(double x);

	// Huber error function
	// this function is equal to x for |x|<scale and is sublinear (sqrt(|x|)) for larger
	// values of |x|.
	template <class T>
	T VtHuberFunction(T x, T scale)
	{
		T z = fabs(x);
		if(z<scale)
			return x;
		T u = sqrt(scale*(2*z - scale));
		return x<0 ? -u : u;
	}

	// quadratic fit
	// fits a quadratic to the three evenly spaced y values given and returns the x location
	// of the maximum or minimum in the range -1:1. if there is no maximum/minimum or
	// x is out of range, then the return value is clamped to +1, 0 or -1, depending on the gradient.
	// the max/min y value is also returned.
	template <class T>
	T VtQuadraticFit1D(T ym1, T y0, T yp1, T &yrtn)
	{
		T u = ym1 - yp1;
		T v = 2*(yp1 + ym1 - 2*y0);
		if(fabs(u) >= fabs(v))
		{
			T r;
			if(v>=0)
				r = (T)(u==0 ? 0 : (u>0 ? 1 : -1));
			else
				r = (T)(u==0 ? 0 : (u>0 ? -1 : 1));
			if(r==0)
				yrtn = 0;
			else if(r>0)
				yrtn = yp1;
			else
				yrtn = ym1;
			return r;
		}

		T uv = u/v;
		yrtn = y0 - u*uv/4;
		return uv;
	}

	// quadratic fit in 2d around a 3x3 region
	// this function fits the quadratic surface by least squares
	// F = 0.5 * x' * H * x + D' * x + dcterm
	// where x=[x y]', H is the hessian matrix and D is the matrix of first derivatives.
	// this fucntion can be used to evaluate the first and second derivatives of F at the origin.
	// pRow0 points to the center value for F and pRowm1 is the center for row y-1 and pRowp1 is the
	// center for row y+1.
	template <class T>
	void VtQuadraticFit2D(const T *pFm1, const T *pF0, const T *pFp1,
		CMtx2x2<T> &mHessRtn, CVec2<T> &vDerivRtn, T &fDCRtn)
	{
		T fRm1 = pFm1[-1] + pFm1[0] + pFm1[1];
		T fRp1 = pFp1[-1] + pFp1[0] + pFp1[1];
		T fCm1 = pFm1[-1] + pF0[-1] + pFp1[-1];
		T fCp1 = pFm1[1] + pF0[1] + pFp1[1];
		mHessRtn(0,0) = (fCm1 - 2*(pFm1[0] + pF0[0] + pFp1[0]) + fCp1)/(T)3.0;
		mHessRtn(1,1) = (fRm1 - 2*(pF0[-1] + pF0[0] + pF0[1]) + fRp1)/(T)3.0;
		mHessRtn(0,1) = (T)0.25 * (pFm1[-1] - pFm1[1] - pFp1[-1] + pFp1[1]);
		mHessRtn(1,0) = mHessRtn(0,1);
		vDerivRtn(0) = (-fCm1 + fCp1)/(T)6.0;
		vDerivRtn(1) = (-fRm1 + fRp1)/(T)6.0;
		fDCRtn = (-pFm1[-1] + 2*pFm1[0] - pFm1[1] 
		+ 2*pF0[-1] + 5*pF0[0] + 2*pF0[1] 
		- pFp1[-1] + 2*pFp1[0] - pFp1[1])/(T)9.0;
	}

	// fit a 3d quadratic surface by least squares to the data around a 3x3x3 region
	// data must contain 27 values and be organized as:
	// y(-1,-1,-1) y(0,-1,-1), y(1,-1,-1) y(-1,0,-1) y(0,0,-1), y(1,0,-1) y(-1,1,-1) y(0,1,-1), y(1,1,-1)
	// y(-1,-1,0) y(0,-1,0), y(1,-1,0) y(-1,0,0) y(0,0,0), y(1,0,0) y(-1,1,0) y(0,1,0), y(1,1,0)
	// y(-1,-1,1) y(0,-1,1), y(1,-1,1) y(-1,0,1) y(0,0,1), y(1,0,1) y(-1,1,1) y(0,1,1), y(1,1,1)
	//
	// The fit equation is:
	// y = 0.5*(x' A x) + b' x + c
	template <class T>
	void VtQuadraticFit3D(const T *p, CMtx3x3<T> &mA, CVec3<T> &vb, T &fc)
	{
		fc = (1/(T)27) * (p[1]+p[3]+p[5]+p[7]+p[9]+p[11]+p[15]+p[17]+p[19]+p[21]+p[23]+p[25]
		-2*(p[0]+p[2]+p[6]+p[8]+p[18]+p[20]+p[24]+p[26])
			+4*(p[4]+p[10]+p[12]+p[14]+p[16]+p[22])
			+7*p[13]);
		vb(0) = (6/(T)108) * (p[2]+p[5]+p[8]+p[11]+p[14]+p[17]+p[20]+p[23]+p[26]
		-(p[0]+p[3]+p[6]+p[9]+p[12]+p[15]+p[18]+p[21]+p[24]));
		vb(1) = (6/(T)108) * (p[6]+p[7]+p[8]+p[15]+p[16]+p[17]+p[24]+p[25]+p[26]
		-(p[0]+p[1]+p[2]+p[9]+p[10]+p[11]+p[18]+p[19]+p[20]));
		vb(2) = (6/(T)108) * (p[18]+p[19]+p[20]+p[21]+p[22]+p[23]+p[24]+p[25]+p[26]
		-(p[0]+p[1]+p[2]+p[3]+p[4]+p[5]+p[6]+p[7]+p[8]));
		T xx = (6/(T)108) * (p[0]+p[3]+p[6]+p[2]+p[5]+p[8]+p[9]+p[12]+p[15]+p[11]+p[14]+p[17]
		+p[18]+p[21]+p[24]+p[20]+p[23]+p[26]
		-2*(p[1]+p[4]+p[7]+p[10]+p[13]+p[16]+p[19]+p[22]+p[25]));
		T yy = (6/(T)108) * (p[0]+p[1]+p[2]+p[6]+p[7]+p[8]+p[9]+p[10]+p[11]+p[15]+p[16]+p[17]
		+p[18]+p[19]+p[20]+p[24]+p[25]+p[26]
		-2*(p[3]+p[4]+p[5]+p[12]+p[13]+p[14]+p[21]+p[22]+p[23]));
		T zz = (6/(T)108) * (p[0]+p[1]+p[2]+p[3]+p[4]+p[5]+p[6]+p[7]+p[8]+p[18]+p[19]+p[20]
		+p[21]+p[22]+p[23]+p[24]+p[25]+p[26]
		-2*(p[9]+p[10]+p[11]+p[12]+p[13]+p[14]+p[15]+p[16]+p[17]));
		T xy = (9/(T)108) * (p[0]+p[8]+p[9]+p[17]+p[18]+p[26]
		-(p[2]+p[6]+p[11]+p[15]+p[20]+p[24]));
		T xz = (9/(T)108) * (p[0]+p[3]+p[6]+p[20]+p[23]+p[26]
		-(p[2]+p[5]+p[8]+p[18]+p[21]+p[24]));
		T yz = (9/(T)108) * (p[0]+p[1]+p[2]+p[24]+p[25]+p[26]
		-(p[6]+p[7]+p[8]+p[18]+p[19]+p[20]));
		mA(0,0) = 2*xx;
		mA(1,1) = 2*yy;
		mA(2,2) = 2*zz;
		mA(0,1) = mA(1,0) = xy;
		mA(0,2) = mA(2,0) = xz;
		mA(1,2) = mA(2,1) = yz;
	}

	// computes the modified bessel function I_n(x) = I_{-n}(x) = (-i)^n J_n(ix) using polynomial approximation
	double VtModBessel(double x, int n);

	// returns the highest common factor of two integers >0 using Euclid's algorithm
	UInt32 VtFactor(UInt32 a, UInt32 b);

	// return maximum value in array
	float VtArrayMax(float *pArray, int iNumEls, int *piIndexRtn = NULL);
	double VtArrayMax(double *pArray, int iNumEls, int *piIndexRtn = NULL);

	// return minimum value in array
	float VtArrayMin(float *pArray, int iNumEls, int *piIndexRtn = NULL);
	double VtArrayMin(double *pArray, int iNumEls, int *piIndexRtn = NULL);

	// return maximum value of fabs(array[i])
	float VtArrayMaxAbs(float *pArray, int iNumEls, int *piIndexRtn = NULL);
	double VtArrayMaxAbs(double *pArray, int iNumEls, int *piIndexRtn = NULL);

	// return minimum value of fabs(array[i])
	float VtArrayMinAbs(float *pArray, int iNumEls, int *piIndexRtn = NULL);
	double VtArrayMinAbs(double *pArray, int iNumEls, int *piIndexRtn = NULL);

	// NOTE: the MeanAndCovariance templates depend on CMtx. To avoid cyclic 
	// dependency they are defined and explicitly instantiated in mathutils.cpp

	// determines the mean and covariance of a data set
	// where the covariance is given by cov_rc = (1/N) * sum_i (x_ir - mean_r) * (x_ic - mean_c)
	// dim is the dimensionality of the data, count is the number of data vectors (size of data set)
	// sample is the number of vectors to randomly sample. if this is = 0 or = count, then all the data is used
	// this is to allow faster evaluation of approximate statistics over very large data sets
	template <class T>
	HRESULT VtMeanAndCovariance(T **pdata, int iDim, int iCount, int iSample, CVec<T> &vMeanRtn, CMtx<T> &mCovRtn);

	// alternate version of the same function. you must provide a callback function to sample the data into the vector
	// that is provided. the index number indicates to you which element of the data vector the function wants.
	// tif the sample function returns false then this data vector is ignored.
	template <class T>
	HRESULT VtMeanAndCovariance(bool (*pCallback)(int, CVec<T> &, void *), void *pUser, 
		int iDim, int iCount, int iSample, CVec<T> &vMeanRtn, CMtx<T> &mCovRtn);
};

