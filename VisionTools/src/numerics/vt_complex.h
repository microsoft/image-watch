//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Complex number class
//
//  History:
//      2003/11/17-swinder
//			Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include <math.h>

namespace vt {

template <class T>
class Complex
{
public:
    typedef T ELT;
    static int Bands() { return 2; }

    Complex() {}
    Complex(T re, T im) { Re = re; Im = im; }

    // these constructors prevent warnings on double - float conversion in matrix constructors
    Complex(const Complex<float> &c) { Re = (T)c.Re; Im = (T)c.Im; }
    Complex(const Complex<double> &c) { Re = (T)c.Re; Im = (T)c.Im; }
    Complex(int v) { Re = (T)v; Im = (T)0; }
    Complex(float v) { Re = (T)v; Im = (T)0; }
    Complex(double v) { Re = (T)v; Im = (T)0; }

    //operator Complex<float> () { return Complex<float>((float)Re, (float)Im); }
    //operator Complex<double> () { return Complex<double>((double)Re, (double)Im); }

    //Complex<T> & operator()(const Complex<double> &f) { Re = (T)f.Re; Im = (T)f.Im; return *this; }
    //Complex<T> & operator()(const Complex<float> &f) { Re = (T)f.Re; Im = (T)f.Im; return *this; }
    Complex<T> & operator+() const { return *this; }
    Complex<T> operator-() const { return Complex<T>(-Re, -Im); }
	Complex<T> operator+(const Complex<T> &f) const { return Complex<T>(Re + f.Re, Im + f.Im); }
    Complex<T> operator+(const T &d) const { return Complex<T>(Re + d, Im); }
	Complex<T> operator-(const Complex<T> &f) const { return Complex<T>(Re - f.Re, Im - f.Im); }
    Complex<T> operator-(const T &d) const { return Complex<T>(Re - d, Im); }
	Complex<T> operator*(const Complex<T> &f) const
		{ return Complex<T>(Re * f.Re - Im * f.Im, Re * f.Im + Im * f.Re); }
    Complex<T> operator/(const Complex<T> &f) const
        {
            if(fabs(f.Re) >= fabs(f.Im))
            {
                T dc = f.Im/f.Re;
                T sc = 1/(f.Re + f.Im * dc);
                return Complex<T>((Re + Im * dc) * sc, (Im - Re * dc) * sc);
            }
            else
            {
                T cd = f.Re/f.Im;
                T sc = 1/(f.Re * cd + f.Im);
                return Complex<T>((Re * cd + Im) * sc, (Im * cd - Re) * sc);
            }
        }
    Complex<T> operator*(const T &d) const { return Complex<T>(Re * d, Im * d); }
    Complex<T> operator/(const T &d) const { return Complex<T>(Re / d, Im / d); }
	Complex<T> & operator+=(const Complex<T> &f) { Re += f.Re; Im += f.Im; return *this; }
	Complex<T> & operator-=(const Complex<T> &f) { Re -= f.Re; Im -= f.Im; return *this; }
    Complex<T> & operator+=(const T &d) { Re += d; return *this; }
    Complex<T> & operator-=(const T &d) { Re -= d; return *this; }
	Complex<T> & operator*=(const T &d) { Re *= d; Im *= d; return *this; }
    Complex<T> & operator/=(const T &d) { Re /= d; Im /= d; return *this; }
	Complex<T> & operator*=(const Complex<T> &f)
		{ T tmp = Re * f.Re - Im * f.Im; Im = Re * f.Im + Im * f.Re; Re = tmp; return *this; }
    Complex<T> & operator/=(const Complex<T> &f)
        {   
            if(fabs(f.Re) >= fabs(f.Im))
            {
                T dc = f.Im/f.Re;
                T sc = 1/(f.Re + f.Im * dc);
                T tmp = (Re + Im * dc) * sc;
                Im = (Im - Re * dc) * sc;
                Re = tmp;
            }
            else
            {
                T cd = f.Re/f.Im;
                T sc = 1/(f.Re * cd + f.Im);
                T tmp = (Re * cd + Im) * sc;
                Im = (Im * cd - Re) * sc;
                Re = tmp;
            }
            return *this; 
        }
    Complex<T> operator~() const { return Conj(); }
    bool operator==(const T &d) const { return Re==d && Im==(T)0; }
    bool operator!=(const T &d) const { return !(operator==(d)); }
    bool operator==(const Complex<T> &f) const { return Re==f.Re && Im==f.Im; }
    bool operator!=(const Complex<T> &f) const { return !(operator==(f)); }

    T Arg() const { return (T)atan2(Im, Re); }
    T Magnitude() const { return VtHypot(Im, Re); }
    T MagnitudeSq() const { return Re * Re + Im * Im; }
    Complex<T> Conj() const { return Complex(Re, -Im); }

#pragma warning(push)
#pragma warning(disable : 4201)
    union {
		T m_v[2];
		struct { T x, y; };
        struct { T Re, Im; };
	};
#pragma warning(pop)
};

template <class T> inline Complex<T> operator*(const T &f, const Complex<T> &c) { return c * f; }
template <class T> inline Complex<T> operator+(const T &f, const Complex<T> &c) { return c + f; }
template <class T> inline Complex<T> operator-(const T &f, const Complex<T> &c) { return Complex<T>(f-c.Re, -c.Im); }
template <class T> inline Complex<T> operator/(const T &f, const Complex<T> &c) { return Complex<T>(f,(T)0)/c; }

template <class T> inline T VtConj(const T &v) { return v; }
template <class T> inline Complex<T> VtConj(const Complex<T> &v) { return v.Conj(); }

template <class T> inline double VtRe(const T &v) { return v; }
template <class T> inline Complex<T> VtRe(const Complex<T> &v) { return Complex<T>(v.Re, (T)0); }

template <class T> inline T VtIm(const T &v) { return (T)0; }
template <class T> inline Complex<T> VtIm(const Complex<T> &v) { return Complex<T>((T)0, v.Im); }

inline float VtMagnitude(const float &v) { return fabsf(v); }
template <class T> inline T VtMagnitude(const T &v) { return fabs(v); }
template <class T> inline Complex<T> VtMagnitude(const Complex<T> &v) { return Complex<T>(v.Magnitude(), (T)0); }

template <class T> inline T VtMagnitudeSq(const T &v) { return v*v; }
template <class T> inline Complex<T> VtMagnitudeSq(const Complex<T> &v) { return Complex<T>(v.MagnitudeSq(), (T)0); }

template <class T> inline T VtReScalar(const T &v) { return v; }
template <class T> inline T VtReScalar(const Complex<T> &v) { return v.Re; }

typedef Complex<float> Complexf;
typedef Complex<double> Complexd;

};
