//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Numerical matrix routines
//
//  History:
//      2003/11/12-swinder
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_matrix2x2.h"

namespace vt {

	//+-----------------------------------------------------------------------------
	// CVec
	//------------------------------------------------------------------------------
	//
	// *** constructors
	// CVec() - uninitialized
	// CVec(v) - initialise to value v
	// CVec(p) - initialise to values pointed to by p
	// CVec(a, b, ...) - initialise vector to values a, b, etc
	//
	// *** information
	// Size() - returns number of elements
	// MagnitudeSq() - returns squared geometric length
	// Magnitude() - returns geometric length
	// Unit() - returns normalised (unit) vector
	//
	// *** operators
	// vec[i] - references i-th element
	// vec(i) - references i-th element
	// +vec - unary plus
	// -vec - unary minus
	// vec + vec - returns sum of vectors
	// vec - vec - returns difference of vectors
	// vec ^ vec - returns CMtx as outer product (a b)(c d) = (ac ad; bc bd)
	// vec * vec - returns scalar product
	// vec * mtx - returns CVec as right sided vector multiply
	// vec * s - returns vector scaled by s
	// vec / s - returns vector divided by s
	// vec == vec - vector equality test
	// vec != vec - vector inequality test
	// vec = p - sets vector to elements pointed to by p
	// vec = s - sets vector elements to scalar value s
	// vec += vec - vector addition
	// vec -= vec - vector subtraction
	// vec *= s - vector scaling
	// vec /= s - vector scaling by division
	// vec *= mtx - right sided vector multiply
	//
	// *** methods
	// El(i) - return reference to element i
	// Dump() - print in matlab format
	//
	// --------------------------------------------------------------------------------------------------------
	// CMtx
	// --------------------------------------------------------------------------------------------------------
	//
	// *** constructors
	// CMtx() - uninitialized
	// CMtx(v) - initialise to value v
	// CMtx(p) - initialise to values pointed to by p
	//
	// *** information
	// Rows() - number of rows
	// Cols() - number of columns
	// Size() - total number of elements
	//
	// *** operators
	// mtx[i] - returns pointer to row i
	// mtx(r,c) - return reference to element in row r, column c
	// +mtx - unary plus
	// -mtx - unary minus
	// mtx + mtx - returns sum of matrices
	// mtx - mtx - returns difference of matrices
	// mtx * mtx - returns matrix multiply
	// mtx * vec - returns CVec as left sided vector multiply
	// mtx * s - returns matrix scaled by s
	// mtx / s - returns matrix divided by s
	// mtx == mtx - matrix equality test
	// mtx != mtx - matrix inequality test
	// mtx = p - sets matrix to elements pointed to by p
	// mtx = s - sets matrix elements to scalar value s
	// mtx += mtx - matrix addition
	// mtx -= mtx - matrix subtraction
	// mtx *= mtx - matrix multiply
	// mtx *= s - matrix scaling
	// mtx /= s - matrix divisive scaling
	//
	// *** methods
	// GetRow(i) - extract row i as a CVec
	// GetCol(i) - extract column i as a CVec
	// SetRow(i,v) - set row i
	// SetCol(i,v) - set column i
	// GetDiag() - returns a vector comprised of the diagonal elements
	// MakeDiag(v) - initialises to a diagonal matrix with the given diagonal
	// El(r,c) - return reference to element in row r, column c
	// MakeI() - set to identity matrix
	// MakeRotation(t) - make rotation matrix (homogeneous 2d rotation)
	// T() - return the transpose
	// Inv() - return the inverse
	// Tr() - return the trace
	// Det() - return the determinant
	// Dump() - print in matlab format

	template <class ET> 
	class CMtx3x3;

	template <class ET> 
	class CVec4;

	template <class ET> 
	class CVec;

	template <class ET> 
	class CMtx;

	template <class ET>
	class CVec3
	{
	public:
		typedef ET ELT;
		static int Bands() { return 3; }

		CVec3() {}
		explicit CVec3(const CVec2<ET> &v, ET zz=(ET)1) { x=v.x; y=v.y; z=zz; }
		explicit CVec3(const CVec<ET> &v) { operator=(v); }
		explicit CVec3(const ET &f) { operator=(f); }
		CVec3(const ET &m0, const ET &m1, const ET &m2) { El(0) = m0; El(1) = m1; El(2) = m2; }

		template<typename RHS>
		CVec3(const CVec3<RHS>& v);

		int Size() const { return 3; }

		const ET *Ptr() const { return &x; }
		ET *Ptr() { return &x; }

		const ET &El(int i) const { return (&x)[i]; }
		ET &El(int i) { return (&x)[i]; }
		const ET &operator[] (int i) const { return (&x)[i]; }
		ET &operator[] (int i) { return (&x)[i]; }
		const ET &operator() (int i) const { return (&x)[i]; }
		ET &operator() (int i) { return (&x)[i]; }

		CVec3<ET> operator+ () const { return *this; }
		CVec3<ET> operator- () const { return CVec3<ET>(-El(0), -El(1), -El(2)); }
		CVec3<ET> operator+ (const CVec3<ET> &v) const { return CVec3<ET>(El(0) + v[0], El(1) + v[1], El(2) + v[2]); }
		CVec3<ET> operator- (const CVec3<ET> &v) const { return CVec3<ET>(El(0) - v[0], El(1) - v[1], El(2) - v[2]); }
		ET operator* (const CVec3<ET> &v) const { return v[0] * El(0) + v[1] * El(1)+ v[2] * El(2); }
		CVec3<ET> operator* (const ET &f) const { return CVec3<ET>(El(0) * f, El(1) * f, El(2) * f); }
		CVec3<ET> operator/ (const ET &f) const { return CVec3<ET>(El(0) / f, El(1) / f, El(2) / f); }
		bool operator== (const CVec3<ET> &v) const { return El(0)==v[0] && El(1)==v[1] && El(2)==v[2]; }
		bool operator!= (const CVec3<ET> &v) const { return !(operator==(v)); }

		ET MagnitudeSq() const { return VtMagnitudeSq(El(0)) + VtMagnitudeSq(El(1)) + VtMagnitudeSq(El(2)); }
		ET Magnitude() const { return (ET)sqrt(VtReScalar(MagnitudeSq())); }
		CVec3<ET> Unit() const { return *this/Magnitude(); }
		CVec2<ET> Dehom() const { ET sc = (ET)1/z; return CVec2<ET>(sc*x, sc*y); }
		CVec4<ET> Hom() const { return CVec4<ET>(*this); }
		CVec3<ET> Cross(const CVec3<ET>& v) const
		{ return CVec3<ET>(y * v.z - v.y * z,
		z * v.x - v.z * x,
		x * v.y - v.x * y); }
		CVec3<ET> Conj() const { return CVec3<ET>(VtConj(El(0)), VtConj(El(1)), VtConj(El(2))); }

		CVec3<ET> & operator= (const ET &f) { El(0) = f; El(1) = f; El(2) = f; return *this; }
		CVec3<ET> & operator+= (const CVec3<ET> &v) { El(0) += v[0]; El(1) += v[1]; El(2) += v[2]; return *this; }
		CVec3<ET> & operator-= (const CVec3<ET> &v) { El(0) -= v[0]; El(1) -= v[1]; El(2) -= v[2]; return *this; }
		CVec3<ET> & operator*= (const ET &f) { El(0) *= f; El(1) *= f; El(2) *= f; return *this; }
		CVec3<ET> & operator/= (const ET &f) { El(0) /= f; El(1) /= f; El(2) /= f; return *this; }

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec3<ET>::operator=
		//
		//  Synopsis:  assigment operator from arbitrary vector types
		//
		//  Returns:   the new CVec3
		//
		//------------------------------------------------------------------------

		CVec3<ET>& operator= (const CVec<ET> &v)
		{
			if(v.Size()>0)
				El(0) = v[0];
			if(v.Size()>1)
				El(1) = v[1];
			if(v.Size()>2)
				El(2) = v[2];
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec3<ET>::Init
		//
		//  Synopsis:  initialize from an array pointer
		//
		//  Returns:   the current CVec3
		//
		//------------------------------------------------------------------------

		CVec3<ET>& Init(const ET *p, int iSize)
		{
			if(p && iSize>0)
			{
				iSize = VtMin(Size(), iSize);
				memcpy(Ptr(), p, sizeof(ET) * iSize); // memcpy handles size = 0 case
			}
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec3<ET>::operator^
		//
		//  Synopsis:  outer product operator
		//
		//  Returns:   the outer product matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET> operator^ (const CVec3<ET> &v) const
		{
			CMtx3x3<ET> m;
			m(0,0) = El(0) * v[0];
			m(0,1) = El(0) * v[1];
			m(0,2) = El(0) * v[2];
			m(1,0) = El(1) * v[0];
			m(1,1) = El(1) * v[1];
			m(1,2) = El(1) * v[2];
			m(2,0) = El(2) * v[0];
			m(2,1) = El(2) * v[1];
			m(2,2) = El(2) * v[2];
			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec3<ET>::operator*
		//
		//  Synopsis:  right hand size 3x3 matrix multiply
		//
		//  Returns:   the new CVec3
		//
		//------------------------------------------------------------------------

		CVec3<ET> operator* (const CMtx3x3<ET> &m) const
		{
			return CVec3<ET>(El(0) * m(0,0) + El(1) * m(1,0) + El(2) * m(2,0),
				El(0) * m(0,1) + El(1) * m(1,1) + El(2) * m(2,1),
				El(0) * m(0,2) + El(1) * m(1,2) + El(2) * m(2,2));
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec3<ET>::operator*=
		//
		//  Synopsis:  right hand size 3x3 matrix multiply
		//
		//  Returns:   the current CVec3
		//
		//------------------------------------------------------------------------

		CVec3<ET>& operator*= (const CMtx3x3<ET> &m)
		{
			*this = *this * m;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec3<ET>::SortAbs
		//
		//  Synopsis:  sort the vector according to the magnitude of its elements
		//
		//  Returns:   fills the maptable with the indices of the ordered elements
		//
		//------------------------------------------------------------------------

		void SortAbs(int *piMapTable) const
		{
			int i, j;

			if(piMapTable==NULL)
				return;

			for(j=0; j<Size(); j++)
				piMapTable[j] = j;
			for(j=0; j<Size()-1; j++)
			{
				double dMax = -1.0;
				int iMax = 0;
				for(i=j; i<Size(); i++)
				{
					double dV = (double)VtReScalar(VtMagnitude(El(piMapTable[i])));
					if(dV>dMax)
					{
						dMax = dV;
						iMax = i;
					}
				}
				if(iMax!=j)
				{
					int iTmp = piMapTable[j];
					piMapTable[j] = piMapTable[iMax];
					piMapTable[iMax] = iTmp;
				}
			}
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec3<ET>::MaxAbs
		//
		//  Synopsis:  returns the index of the element with largest magnitude
		//
		//  Returns:   the index
		//
		//------------------------------------------------------------------------

		int MaxAbs() const
		{
			double fMax = -1.0;
			int iLoc = 0;
			int i;
			for(i=0; i<Size(); i++)
			{
				double v = VtReScalar(VtMagnitude(El(i)));
				if(v>fMax)
				{
					fMax = v;
					iLoc = i;
				}
			}
			return iLoc;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec3<ET>::Dump
		//
		//  Synopsis:  print out the vector in matlab format to debug stream
		//
		//  Returns:   none
		//
		//------------------------------------------------------------------------

		void Dump(FILE *fp = stdout) const
		{ 
			FormatVecStdio(*this, fp);
		}


	public:
		ET x, y, z;
	};

	//
	//  invert matrix (Gauss Jordan with partial pivoting)
	//

	template <class ET>
	inline static void
		ExchangeRows3(int iRow1, int iRow2, CMtx3x3<ET> &mat)
	{
		ET f;

		f = mat[iRow1][0];  mat[iRow1][0] = mat[iRow2][0];  mat[iRow2][0] = f;
		f = mat[iRow1][1];  mat[iRow1][1] = mat[iRow2][1];  mat[iRow2][1] = f;
		f = mat[iRow1][2];  mat[iRow1][2] = mat[iRow2][2];  mat[iRow2][2] = f;
	}

	template <class ET>
	inline static void
		ScaleRow3(int iRow, ET f, CMtx3x3<ET> &mat)
	{
		mat[iRow][0] *= f;
		mat[iRow][1] *= f;
		mat[iRow][2] *= f;
	}

	template <class ET>
	inline static void
		AddScaled3(int iRow1, ET f, int iRow2, CMtx3x3<ET> &mat)
	{
		mat[iRow2][0] += f*mat[iRow1][0];
		mat[iRow2][1] += f*mat[iRow1][1];
		mat[iRow2][2] += f*mat[iRow1][2];
	}


	template <class ET>
	class CMtx3x3
	{
	public:
		CMtx3x3() {}
		CMtx3x3(const CMtx3x3<float> &m) 
		{ 
			m_m[0] = (ET)m(0,0); m_m[1] = (ET)m(0,1); m_m[2] = (ET)m(0,2);
			m_m[3] = (ET)m(1,0); m_m[4] = (ET)m(1,1); m_m[5] = (ET)m(1,2);
			m_m[6] = (ET)m(2,0); m_m[7] = (ET)m(2,1); m_m[8] = (ET)m(2,2);
		}
		CMtx3x3(const CMtx3x3<double> &m) 
		{ 
			m_m[0] = (ET)m(0,0); m_m[1] = (ET)m(0,1); m_m[2] = (ET)m(0,2);
			m_m[3] = (ET)m(1,0); m_m[4] = (ET)m(1,1); m_m[5] = (ET)m(1,2);
			m_m[6] = (ET)m(2,0); m_m[7] = (ET)m(2,1); m_m[8] = (ET)m(2,2);
		}
		explicit CMtx3x3(const CMtx<ET> &m) { operator=(m); }
		explicit CMtx3x3(const ET &f) { operator=(f); }
		CMtx3x3(const ET *p, int iSize) { memcpy(Ptr(), p, sizeof(ET) * VtMin(Size(), iSize)); }
		CMtx3x3(ET _00, ET _01, ET _02,
			ET _10, ET _11, ET _12,
			ET _20, ET _21, ET _22 ) 
		{ m_m[0] = _00; m_m[1] = _01; m_m[2] = _02;
		m_m[3] = _10; m_m[4] = _11; m_m[5] = _12;
		m_m[6] = _20; m_m[7] = _21; m_m[8] = _22; }

		int Rows() const { return 3; }
		int Cols() const { return 3; }
		int Size() const { return Rows() * Cols(); }

		const ET *Ptr() const { return m_m; }
		ET *Ptr() { return m_m; }

		CVec3<ET> GetRow(int i) const { return CVec3<ET>(m_m[i*Cols()+0], m_m[i*Cols()+1], m_m[i*Cols()+2]); }
		CVec3<ET> GetCol(int i) const { return CVec3<ET>(m_m[i], m_m[i+Cols()], m_m[i+2*Cols()]); }

		const ET &El(int iRow, int iCol) const { return m_m[iRow * Cols() + iCol]; }
		ET &El(int iRow, int iCol) { return m_m[iRow * Cols() + iCol]; }
		const ET *operator[] (int i) const { return m_m + i * Cols(); }
		ET *operator[] (int i) { return m_m + i * Cols(); }
		const ET &operator() (int iRow, int iCol) const { return El(iRow, iCol); }
		ET &operator() (int iRow, int iCol) { return El(iRow, iCol); }

		CMtx3x3<ET> & MakeDiag(const CVec3<ET> &v) { memset(m_m, 0, sizeof(ET) * Size());
		El(0,0) = v[0]; El(1,1) = v[1]; El(2,2) = v[2]; return *this; }
		CMtx3x3<ET> & MakeScale(ET sx, ET sy, ET sz=ET(1)) 
		{ return MakeDiag(CVec3<ET>(sx, sy, sz)); }
		CMtx3x3<ET> & MakeTranslate(ET tx, ET ty) 
		{ *this = CMtx3x3(ET(1), 0, tx, 0, ET(1), ty, 0, 0, ET(1)); return *this; }
		CMtx3x3<ET> & MakeSymmetric()
		{ m_m[3] = m_m[1]; m_m[6] = m_m[2]; m_m[7] = m_m[5]; return *this; } // copies below diagonal entries from above diagonal
		ET Tr() const { return m_m[0] + m_m[4] + m_m[8]; }
		ET Det() const { return m_m[0] * (m_m[4]*m_m[8]-m_m[5]*m_m[7]) 
			- m_m[1] * (m_m[3]*m_m[8]-m_m[6]*m_m[5]) 
			+ m_m[2] * (m_m[3]*m_m[7]-m_m[4]*m_m[6]); }

		CMtx3x3<ET> & SetRow(int i, const CVec3<ET> &v) { El(i,0) = v[0]; El(i,1) = v[1]; El(i,2) = v[2]; return *this; }
		CMtx3x3<ET> & SetCol(int i, const CVec3<ET> &v) { El(0,i) = v[0]; El(1,i) = v[1]; El(2,i) = v[2]; return *this; }
		CVec3<ET> GetDiag() const { return CVec3<ET>(El(0,0), El(1,1), El(2,2)); }

		CMtx3x3<ET> operator+ () const { return *this; }
		bool operator!= (const CMtx3x3<ET> &m) const { return !(operator==(m)); }

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator=
		//
		//  Synopsis:  general assigment from another matrix of arbitrary size
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET>& operator= (const CMtx<ET> &m)
		{
			int iMinR = VtMin(m.Rows(), Rows());
			int iMinC = VtMin(m.Cols(), Cols());
			int i, j;
			for(i=0; i<iMinR; i++)
				for(j=0; j<iMinC; j++)
					El(i,j) = m.El(i,j);
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator/
		//
		//  Synopsis:  division by a constant
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET> operator/ (const ET &f)  const
		{
			CMtx3x3 mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = m_m[i] / f;
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator-
		//
		//  Synopsis:  unary minus
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET> operator- () const
		{
			CMtx3x3 mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = -m_m[i];
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator+
		//
		//  Synopsis:  addition
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET> operator+ (const CMtx3x3<ET> &m) const
		{
			CMtx3x3 mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = m_m[i] + m.m_m[i];
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator-
		//
		//  Synopsis:  subtraction
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET> operator- (const CMtx3x3<ET> &m) const
		{
			CMtx3x3 mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = m_m[i] - m.m_m[i];
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator*
		//
		//  Synopsis:  scaling by a constant
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET> operator*(const ET &f) const
		{
			CMtx3x3<ET> mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = m_m[i] * f;
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator=
		//
		//  Synopsis:  set all elements to a constant
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET>& operator=(const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] = f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::Init
		//
		//  Synopsis:  initialize from an array pointer
		//
		//  Returns:   the current CMtx3x3
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET>& Init(const ET *p, int iSize)
		{
			if(p && iSize>0)
			{
				iSize = VtMin(Rows() * Cols(), iSize);
				memcpy(Ptr(), p, sizeof(ET) * iSize); // memcpy handles size = 0 case
			}
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator+=
		//
		//  Synopsis:  add a matrix to the current matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET>& operator+= (const CMtx3x3<ET> &m)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] += m.m_m[i];
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator-=
		//
		//  Synopsis:  subtract a matrix from the current matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET>& operator-= (const CMtx3x3<ET> &m)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] -= m.m_m[i];
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator*=
		//
		//  Synopsis:  multiply two matrices RHS multiply by RHS matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET>& operator*= (const CMtx3x3<ET> &m)
		{
			*this = *this * m;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator*=
		//
		//  Synopsis:  scale the current matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET>& operator*= (const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] *= f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator/=
		//
		//  Synopsis:  divide the current matrix by a scale
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET>& operator/= (const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] /= f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator*
		//
		//  Synopsis:  matrix multiply
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET> operator*(const CMtx3x3<ET> &m) const
		{
			CMtx3x3<ET> mr;
			mr(0,0) = El(0,0) * m(0,0) + El(0,1) * m(1,0) + El(0,2) * m(2,0);
			mr(0,1) = El(0,0) * m(0,1) + El(0,1) * m(1,1) + El(0,2) * m(2,1);
			mr(0,2) = El(0,0) * m(0,2) + El(0,1) * m(1,2) + El(0,2) * m(2,2);

			mr(1,0) = El(1,0) * m(0,0) + El(1,1) * m(1,0) + El(1,2) * m(2,0);
			mr(1,1) = El(1,0) * m(0,1) + El(1,1) * m(1,1) + El(1,2) * m(2,1);
			mr(1,2) = El(1,0) * m(0,2) + El(1,1) * m(1,2) + El(1,2) * m(2,2);

			mr(2,0) = El(2,0) * m(0,0) + El(2,1) * m(1,0) + El(2,2) * m(2,0);
			mr(2,1) = El(2,0) * m(0,1) + El(2,1) * m(1,1) + El(2,2) * m(2,1);
			mr(2,2) = El(2,0) * m(0,2) + El(2,1) * m(1,2) + El(2,2) * m(2,2);
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator*
		//
		//  Synopsis:  multiply matrix by a 3x1 vector
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CVec3<ET> operator*(const CVec3<ET> &v) const
		{
			CVec3<ET> vr;
			vr(0) = El(0,0) * v(0) + El(0,1) * v(1) + El(0,2) * v(2);
			vr(1) = El(1,0) * v(0) + El(1,1) * v(1) + El(1,2) * v(2);
			vr(2) = El(2,0) * v(0) + El(2,1) * v(1) + El(2,2) * v(2);
			return vr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::MakeI
		//
		//  Synopsis:  form the identity
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET>& MakeI()
		{
			m_m[0] = m_m[4] = m_m[8] = (ET)1.0;
			m_m[1] = m_m[2] = m_m[3] = m_m[5] = m_m[6] = m_m[7] = 0;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::operator==
		//
		//  Synopsis:  see if two matrices are equal
		//
		//  Returns:   true or false
		//
		//------------------------------------------------------------------------

		bool operator== (const CMtx3x3<ET> &m) const
		{
			int i;
			for(i=0; i<9; i++)
				if(m_m[i]!=m.m_m[i])
					break;
			return i==9;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::T
		//
		//  Synopsis:  transposes the 3x3 matrix
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET> T() const
		{
			CMtx3x3<ET> mr;
			mr(0,0) = El(0,0);
			mr(0,1) = El(1,0);
			mr(0,2) = El(2,0);
			mr(1,0) = El(0,1);
			mr(1,1) = El(1,1);
			mr(1,2) = El(2,1);
			mr(2,0) = El(0,2);
			mr(2,1) = El(1,2);
			mr(2,2) = El(2,2);
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::H
		//
		//  Synopsis:  conjugate transpose the 3x3 matrix
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET> H() const
		{
			CMtx3x3<ET> mr;
			mr(0,0) = VtConj(El(0,0));
			mr(0,1) = VtConj(El(1,0));
			mr(0,2) = VtConj(El(2,0));
			mr(1,0) = VtConj(El(0,1));
			mr(1,1) = VtConj(El(1,1));
			mr(1,2) = VtConj(El(2,1));
			mr(2,0) = VtConj(El(0,2));
			mr(2,1) = VtConj(El(1,2));
			mr(2,2) = VtConj(El(2,2));
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::Conj
		//
		//  Synopsis:  conjugate the 3x3 matrix
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET> Conj() const
		{
			CMtx3x3<ET> mr;
			mr(0,0) = VtConj(El(0,0));
			mr(0,1) = VtConj(El(0,1));
			mr(0,2) = VtConj(El(0,2));
			mr(1,0) = VtConj(El(1,0));
			mr(1,1) = VtConj(El(1,1));
			mr(1,2) = VtConj(El(1,2));
			mr(2,0) = VtConj(El(2,0));
			mr(2,1) = VtConj(El(2,1));
			mr(2,2) = VtConj(El(2,2));
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::DeleteRowCol
		//
		//  Synopsis:  delete a specific row and column to make a 2x2 matrix
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET> DeleteRowCol(int iRow, int iCol) const
		{
			CMtx2x2<ET> mr;
			if(iRow<0 || iRow>=Rows() || iCol<0 || iCol>=Cols())
				return mr=(ET)0;

			int i, j;
			int r, c;
			for(r=i=0; i<Rows(); i++)
				if(i!=iRow)
				{
					for(c=j=0; j<Cols(); j++)
						if(j!=iCol)
							mr(r,c++) = El(i,j);
					r++;
				}
				return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::MakeRotation
		//
		//  Synopsis:  make a 3x3 rotation matrix from the angle given
		//             this is just a single 2x2 rotation using homogeneous coords.
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET>& MakeRotation(ET fTheta)
		{
			ET fSin, fCos;
			VtSinCos( fTheta, &fSin, &fCos );
			El(0,0) = fCos; El(0,1) = -fSin; El(0,2) = (ET)0;
			El(1,0) = fSin; El(1,1) = fCos; El(1,2) = (ET)0;
			El(2,0) = (ET)0; El(2,1) = (ET)0; El(2,2) = (ET)1;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::MakeSimilarity
		//
		//  Synopsis:  make a 3x3 similarity matrix from the parameters given.
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET>& MakeSimilarity(ET fScale, ET fRotation, ET fDX, ET fDY)
		{
			ET fSin, fCos;
			VtSinCos( fRotation, &fSin, &fCos );
			fSin *= fScale;
			fCos *= fScale;
			El(0,0) = fCos; El(0,1) = -fSin; El(0,2) = fDX;
			El(1,0) = fSin; El(1,1) = fCos; El(1,2) = fDY;
			El(2,0) = (ET)0; El(2,1) = (ET)0; El(2,2) = (ET)1;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::Inv
		//
		//  Synopsis:  invert the 3x3 matrix
		//
		//  Returns:   the new matrix. returns {0} if exactly singular
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET> Inv() const
		{
			CMtx3x3<ET> mr;
			mr.MakeI();
			CMtx3x3<ET> m = *this;

			int iRow, iCol, iPivotRow = 0;
			ET fPivot, f;

			for (iCol = 0; iCol < Cols(); iCol++)
			{
				double fMax = 0;
				for (iRow = iCol; iRow < Rows(); iRow++)
				{
					// find maximum row element to pivot on
					double fm = VtReScalar(VtMagnitude(m(iRow,iCol)));
					if (fm > fMax)
					{
						iPivotRow = iRow;
						fMax = fm;
					}
				}
				if (fMax == 0)
				{
					// singular
					mr = (ET)0;
					return mr;
				}

				if (iPivotRow != iCol)
				{
					ExchangeRows3(iCol, iPivotRow, m);
					ExchangeRows3(iCol, iPivotRow, mr);
				}
				fPivot = m(iCol,iCol);
				ScaleRow3(iCol, ((ET)1)/fPivot, m);
				ScaleRow3(iCol, ((ET)1)/fPivot, mr);
				for (iRow = 0; iRow < Rows(); iRow++)
					if (iRow != iCol) 
					{
						f = -m(iRow,iCol);
						AddScaled3(iCol, f, iRow, m);
						AddScaled3(iCol, f, iRow, mr);
					}
			}

			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::MaxAbs
		//
		//  Synopsis:  locate maxuimum element
		//
		//  Returns:   row col
		//
		//------------------------------------------------------------------------

		void MaxAbs(int &row, int &col) const // get maximum element
		{
			row = 0;
			col = 0;
			int i,j;
			double fMax = 0;
			for(i=0; i<Rows(); i++)
				for(j=0; j<Cols(); j++)
				{
					double f = (double)VtReScalar(VtMagnitude(El(i,j)));
					if(f>fMax)
					{
						fMax = f;
						row = i;
						col = j;
					}
				}
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx3x3<ET>::Dump
		//
		//  Synopsis:  dump out the matrix in matlab format
		//
		//  Returns:   none
		//
		//------------------------------------------------------------------------

		void Dump(FILE *fp = stdout) const
		{ 
			FormatMtxStdio(*this, fp);
		}

		CMtx<ET> operator*(const CMtx<ET> &m) const
		{
			// Make sure that the dimensions match
			CMtx<ET> mr;
			mr.SetError(m.GetError());
			if(mr.IsError())
				return mr;

			int nrows = Rows();
			int ncols = m.Cols();
			int nk    = m.Rows();

			if(nk != Cols())
			{
				mr.SetError(E_INVALIDARG);
				return mr;
			}

			if(FAILED(mr.Create(nrows, ncols)))
				return mr;

			mr = (ET)0;
			int i,j,k;

			for (i=0; i<nrows; i++)
				for (j=0; j<ncols; j++)
					for (k=0; k<nk; k++)
						mr(i,j) += El(i,k) * m(k,j);

			return mr;
		}
	private:
		ET m_m[9];
	};

	typedef CVec3<float> CVec3f;
	typedef CVec3<double> CVec3d;
	typedef CMtx3x3<float> CMtx3x3f;
	typedef CMtx3x3<double> CMtx3x3d;
	typedef CVec3<Complex<float> > CVec3cf;
	typedef CVec3<Complex<double> > CVec3cd;
	typedef CMtx3x3<Complex<float> > CMtx3x3cf;
	typedef CMtx3x3<Complex<double> > CMtx3x3cd;

	template <>	template <typename RHS>
	CVec3<double>::CVec3(const CVec3<RHS>& v)
	{ 
		x = v.x;
		y = v.y;
		z = v.z;
	} 

	template <typename ET> template <typename RHS>
	CVec3<ET>::CVec3(const CVec3<RHS>& v)
	{ 
		static_assert(sizeof(CVec3<RHS>::ELT) != sizeof(RHS), 
			"CVec3 copy constructor only defined for CVec3<double> to avoid loss of precision");
	} 

	template <class T>
	CMtx3x3< Complex<T> > VtComplexify(const CMtx3x3<T> &mtx)
	{
		CMtx3x3< Complex<T> > out;
		int i,j;
		for(i=0; i<mtx.Rows(); i++)
			for(j=0; j<mtx.Cols(); j++)
			{
				out(i,j).Re = mtx(i,j);
				out(i,j).Im = 0;
			}
			return out;
	}

	template <class T>
	CVec3< Complex<T> > VtComplexify(const CVec3<T> &v)
	{
		CVec3< Complex<T> > out;
		out(0).Re = v(0);
		out(0).Im = 0;
		out(1).Re = v(1);
		out(1).Im = 0;
		out(2).Re = v(2);
		out(2).Im = 0;
		return out;
	}

	template <class T>
	CMtx3x3<T> VtRe(const CMtx3x3< Complex<T> > &mtx)
	{
		CMtx3x3<T> out;
		int i,j;
		for(i=0; i<mtx.Rows(); i++)
			for(j=0; j<mtx.Cols(); j++)
				out(i,j) = mtx(i,j).Re;
		return out;
	}

	template <class T>
	CVec3<T> VtRe(const CVec3< Complex<T> > &v)
	{
		CVec3<T> out;
		out(0) = v(0).Re;
		out(1) = v(1).Re;
		out(2) = v(2).Re;
		return out;
	}

	template <class T>
	CMtx3x3<T> VtIm(const CMtx3x3< Complex<T> > &mtx)
	{
		CMtx3x3<T> out;
		int i,j;
		for(i=0; i<mtx.Rows(); i++)
			for(j=0; j<mtx.Cols(); j++)
				out(i,j) = mtx(i,j).Im;
		return out;
	}

	template <class T>
	CVec3<T> VtIm(const CVec3< Complex<T> > &v)
	{
		CVec3<T> out;
		out(0) = v(0).Im;
		out(1) = v(1).Im;
		out(2) = v(2).Im;
		return out;
	}

	//
	// define conversions between specific types
	//
	template <typename TS, typename TD>
	inline CMtx3x3<TD>& VtConvertMtx(const CMtx3x3<TS>& s, CMtx3x3<TD>& d)
	{
#define cnvrow(r) d(r,0) = TD(s(r,0)); d(r,1) = TD(s(r,1));\
	d(r,2) = TD(s(r,2)); 
		cnvrow(0) cnvrow(1) cnvrow(2)
#undef cnvrow
			return d;
	}

	template <class T> 
	inline CMtx3x3<T> operator*(const T &f, const CMtx3x3<T> &m) { return m*f; }

	template <class T> 
	inline CVec3<T> operator*(const T &f, const CVec3<T> &v) { return v*f; }

};
