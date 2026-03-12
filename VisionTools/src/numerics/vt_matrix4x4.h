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

namespace vt {

	// --------------------------------------------------------------------------------------------------------
	// CVec
	// --------------------------------------------------------------------------------------------------------
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
	// MakeRotationX(t) - make rotation matrix about X
	// MakeRotationY(t) - make rotation matrix about Y
	// MakeRotationZ(t) - make rotation matrix about Z
	// T() - return the transpose
	// Inv() - return the inverse
	// Tr() - return the trace
	// Det() - return the determinant
	// Dump() - print in matlab format

	template <class ET> 
	class CMtx4x4;

	template <class ET> 
	class CVec;

	template <class ET> 
	class CMtx;

	template <class ET>
	class CVec4
	{
	public:
		CVec4() {}

		explicit CVec4(const CVec3<ET> &v, ET ww=(ET)1) { x=v.x; y=v.y; z=v.z; w=ww; }
		explicit CVec4(const CVec<ET> &v) { operator=(v); }
		explicit CVec4(const ET &f) { operator=(f); }
		CVec4(const ET &m0, const ET &m1, const ET &m2, const ET &m3)
		{ El(0) = m0; El(1) = m1; El(2) = m2; El(3) = m3; }

		template <typename RHS>
		CVec4(const CVec4<RHS>& v);

		int Size() const { return 4; }

		const ET *Ptr() const { return &x; }
		ET *Ptr() { return &x; }

		const ET &El(int i) const { return (&x)[i]; }
		ET &El(int i) { return (&x)[i]; }
		const ET &operator[] (int i) const { return (&x)[i]; }
		ET &operator[] (int i) { return (&x)[i]; }
		const ET &operator() (int i) const { return (&x)[i]; }
		ET &operator() (int i) { return (&x)[i]; }

		CVec4<ET> operator+ () const { return *this; }
		CVec4<ET> operator- () const { return CVec4<ET>(-El(0), -El(1), -El(2), -El(3)); }
		CVec4<ET> operator+ (const CVec4<ET> &v) const 
		{ return CVec4<ET>(El(0) + v[0], El(1) + v[1], El(2) + v[2], El(3) + v[3]); }
		CVec4<ET> operator- (const CVec4<ET> &v) const 
		{ return CVec4<ET>(El(0) - v[0], El(1) - v[1], El(2) - v[2], El(3) - v[3]); }
		ET operator* (const CVec4<ET> &v) const { return v[0] * El(0) + v[1] * El(1) + v[2] * El(2) + v[3] * El(3); }
		CVec4<ET> operator* (const ET &f) const { return CVec4<ET>(El(0) * f, El(1) * f, El(2) * f, El(3) * f); }
		CVec4<ET> operator/ (const ET &f) const { return CVec4<ET>(El(0) / f, El(1) / f, El(2) / f, El(3) / f); }
		bool operator== (const CVec4<ET> &v) const { return El(0)==v[0] && El(1)==v[1] && El(2)==v[2] && El(3)==v[3]; }
		bool operator!= (const CVec4<ET> &v) const { return !(operator==(v)); }

		ET MagnitudeSq() const { return VtMagnitudeSq(El(0)) + VtMagnitudeSq(El(1)) 
			+ VtMagnitudeSq(El(2)) + VtMagnitudeSq(El(3)); }
		ET Magnitude() const { return (ET)sqrt(VtReScalar(MagnitudeSq())); }
		CVec4<ET> Unit() const { return *this/Magnitude(); }
		CVec3<ET> Dehom() const { ET sc = (ET)1/w; return CVec3<ET>(sc*x, sc*y, sc*z); }
		CVec4<ET> Conj() const { return CVec4<ET>(VtConj(El(0)), VtConj(El(1)), VtConj(El(2)), VtConj(El(3))); }

		CVec4<ET> & operator= (const ET &f) { El(0) = f; El(1) = f; El(2) = f; El(3) = f; return *this; }
		CVec4<ET> & operator+= (const CVec4<ET> &v) { El(0) += v[0]; El(1) += v[1]; El(2) += v[2]; El(3) += v[3]; return *this; }
		CVec4<ET> & operator-= (const CVec4<ET> &v) { El(0) -= v[0]; El(1) -= v[1]; El(2) -= v[2]; El(3) -= v[3]; return *this; }
		CVec4<ET> & operator*= (const ET &f) { El(0) *= f; El(1) *= f; El(2) *= f; El(3) *= f; return *this; }
		CVec4<ET> & operator/= (const ET &f) { El(0) /= f; El(1) /= f; El(2) /= f; El(3) /= f; return *this; }

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator=
		//
		//  Synopsis:  assigment operator from arbitrary vector types
		//
		//  Returns:   the current CVec4
		//
		//------------------------------------------------------------------------
		CVec4<ET> & operator= (const CVec<ET> &v)
		{
			if(v.Size()>0)
				El(0) = v[0];
			if(v.Size()>1)
				El(1) = v[1];
			if(v.Size()>2)
				El(2) = v[2];
			if(v.Size()>3)
				El(3) = v[3];
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Init
		//
		//  Synopsis:  initialize from an array pointer
		//
		//  Returns:   the current CVec4
		//
		//------------------------------------------------------------------------

		CVec4<ET> & Init(const ET *p, int iSize)
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
		//  Member:    operator^
		//
		//  Synopsis:  outer product operator
		//
		//  Returns:   the outer product matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> operator^ (const CVec4<ET> &v) const
		{
			CMtx4x4<ET> m;
			m(0,0) = El(0) * v[0];
			m(0,1) = El(0) * v[1];
			m(0,2) = El(0) * v[2];
			m(0,3) = El(0) * v[3];
			m(1,0) = El(1) * v[0];
			m(1,1) = El(1) * v[1];
			m(1,2) = El(1) * v[2];
			m(1,3) = El(1) * v[3];
			m(2,0) = El(2) * v[0];
			m(2,1) = El(2) * v[1];
			m(2,2) = El(2) * v[2];
			m(2,3) = El(2) * v[3];
			m(3,0) = El(3) * v[0];
			m(3,1) = El(3) * v[1];
			m(3,2) = El(3) * v[2];
			m(3,3) = El(3) * v[3];
			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*
		//
		//  Synopsis:  right hand size 4x4 matrix multiply
		//
		//  Returns:   the new CVec4
		//
		//------------------------------------------------------------------------

		CVec4<ET> operator* (const CMtx4x4<ET> &m) const
		{
			return CVec4<ET>(El(0) * m(0,0) + El(1) * m(1,0) + El(2) * m(2,0) + El(3) * m(3,0),
				El(0) * m(0,1) + El(1) * m(1,1) + El(2) * m(2,1) + El(3) * m(3,1),
				El(0) * m(0,2) + El(1) * m(1,2) + El(2) * m(2,2) + El(3) * m(3,2),
				El(0) * m(0,3) + El(1) * m(1,3) + El(2) * m(2,3) + El(3) * m(3,3) );
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*=
		//
		//  Synopsis:  right hand size 4x4 matrix multiply
		//
		//  Returns:   the current CVec4
		//
		//------------------------------------------------------------------------

		CVec4<ET> & operator*= (const CMtx4x4<ET> &m)
		{
			*this = *this * m;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    SortAbs
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
		//  Member:    MaxAbs
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
		//  Member:    Dump
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
		ET x, y, z, w;
	};

	// invert

	template <class ET>
	inline static void
		ExchangeRows4(int iRow1, int iRow2, CMtx4x4<ET> &mat)
	{
		ET f;

		f = mat[iRow1][0];  mat[iRow1][0] = mat[iRow2][0];  mat[iRow2][0] = f;
		f = mat[iRow1][1];  mat[iRow1][1] = mat[iRow2][1];  mat[iRow2][1] = f;
		f = mat[iRow1][2];  mat[iRow1][2] = mat[iRow2][2];  mat[iRow2][2] = f;
		f = mat[iRow1][3];  mat[iRow1][3] = mat[iRow2][3];  mat[iRow2][3] = f;
	}

	template <class ET>
	inline static void
		ScaleRow4(int iRow, ET f, CMtx4x4<ET> &mat)
	{
		mat[iRow][0] *= f;
		mat[iRow][1] *= f;
		mat[iRow][2] *= f;
		mat[iRow][3] *= f;
	}

	template <class ET>
	inline static void
		AddScaled4(int iRow1, ET f, int iRow2, CMtx4x4<ET> &mat)
	{
		mat[iRow2][0] += f*mat[iRow1][0];
		mat[iRow2][1] += f*mat[iRow1][1];
		mat[iRow2][2] += f*mat[iRow1][2];
		mat[iRow2][3] += f*mat[iRow1][3];
	}

	template <class ET>
	class CMtx4x4
	{
	public:
		CMtx4x4() {}
		CMtx4x4(const CMtx4x4<float> &m) 
		{ 
			m_m[0] = (ET)m(0,0); m_m[1] = (ET)m(0,1); m_m[2] = (ET)m(0,2); m_m[3] = (ET)m(0,3);
			m_m[4] = (ET)m(1,0); m_m[5] = (ET)m(1,1); m_m[6] = (ET)m(1,2); m_m[7] = (ET)m(1,3);
			m_m[8] = (ET)m(2,0); m_m[9] = (ET)m(2,1); m_m[10] = (ET)m(2,2); m_m[11] = (ET)m(2,3);
			m_m[12] = (ET)m(3,0); m_m[13] = (ET)m(3,1); m_m[14] = (ET)m(3,2); m_m[15] = (ET)m(3,3);
		}
		CMtx4x4(const CMtx4x4<double> &m) 
		{ 
			m_m[0] = (ET)m(0,0); m_m[1] = (ET)m(0,1); m_m[2] = (ET)m(0,2); m_m[3] = (ET)m(0,3);
			m_m[4] = (ET)m(1,0); m_m[5] = (ET)m(1,1); m_m[6] = (ET)m(1,2); m_m[7] = (ET)m(1,3);
			m_m[8] = (ET)m(2,0); m_m[9] = (ET)m(2,1); m_m[10] = (ET)m(2,2); m_m[11] = (ET)m(2,3);
			m_m[12] = (ET)m(3,0); m_m[13] = (ET)m(3,1); m_m[14] = (ET)m(3,2); m_m[15] = (ET)m(3,3);
		}
		explicit CMtx4x4(const CMtx<ET> &m) { operator=(m); }
		explicit CMtx4x4(const ET &f) { operator=(f); }
		CMtx4x4(const ET *p, int iSize) { memcpy(Ptr(), p, sizeof(ET) * VtMin(Size(), iSize)); }
		CMtx4x4(ET _00, ET _01, ET _02, ET _03,
			ET _10, ET _11, ET _12, ET _13,
			ET _20, ET _21, ET _22, ET _23,
			ET _30, ET _31, ET _32, ET _33 ) 
		{ m_m[0]  = _00; m_m[1]  = _01; m_m[2]  = _02; m_m[3]  = _03;
		m_m[4]  = _10; m_m[5]  = _11; m_m[6]  = _12; m_m[7]  = _13;
		m_m[8]  = _20; m_m[9]  = _21; m_m[10] = _22; m_m[11] = _23;
		m_m[12] = _30; m_m[13] = _31; m_m[14] = _32; m_m[15] = _33; }

		int Rows() const { return 4; }
		int Cols() const { return 4; }
		int Size() const { return Rows() * Cols(); }

		const ET *Ptr() const { return m_m; }
		ET *Ptr() { return m_m; }

		CVec4<ET> GetRow(int i) const { return CVec4<ET>(m_m[i*Cols()+0], m_m[i*Cols()+1], m_m[i*Cols()+2], m_m[i*Cols()+3]); }
		CVec4<ET> GetCol(int i) const { return CVec4<ET>(m_m[i], m_m[i+Cols()], m_m[i+2*Cols()], m_m[i+3*Cols()]); }

		const ET &El(int iRow, int iCol) const { return m_m[iRow * Cols() + iCol]; }
		ET &El(int iRow, int iCol) { return m_m[iRow * Cols() + iCol]; }
		const ET *operator[] (int i) const { return m_m + i * Cols(); }
		ET *operator[] (int i) { return m_m + i * Cols(); }
		const ET &operator() (int iRow, int iCol) const { return El(iRow, iCol); }
		ET &operator() (int iRow, int iCol) { return El(iRow, iCol); }

		CMtx4x4<ET> & MakeDiag(const CVec4<ET> &v) { memset(m_m, 0, Size() * sizeof(ET));
		El(0,0) = v[0]; El(1,1) = v[1]; El(2,2) = v[2]; El(3,3) = v[3]; return *this; }
		CMtx4x4<ET> & MakeScale(ET sx, ET sy, ET sz, ET sw = ET(1)) 
		{ return MakeDiag(CVec4<ET>(sx, sy, sz, sw)); }
		CMtx4x4<ET> & MakeTranslate(ET tx, ET ty, ET tz)
		{ MakeI(); El(0, 3) = tx; El(1, 3) = ty; El(2, 3) = tz; return *this; }
		CMtx4x4<ET> & MakeSymmetric() // copies below diagonal entries from above diagonal
		{ m_m[4] = m_m[1]; m_m[8] = m_m[2]; m_m[9] = m_m[6];
		m_m[12] = m_m[3]; m_m[13] = m_m[7]; m_m[14] = m_m[11]; return *this; } 
		ET Tr() const { return m_m[0] + m_m[5] + m_m[10] + m_m[15]; }

		CMtx4x4<ET> & SetRow(int i, const CVec4<ET> &v) { El(i,0) = v[0]; El(i,1) = v[1]; El(i,2) = v[2]; El(i,3) = v[3]; return *this; }
		CMtx4x4<ET> & SetCol(int i, const CVec4<ET> &v) { El(0,i) = v[0]; El(1,i) = v[1]; El(2,i) = v[2]; El(3,i) = v[3]; return *this; }
		CVec4<ET> GetDiag() const { return CVec4<ET>(El(0,0), El(1,1), El(2,2), El(3,3)); }

		CMtx4x4<ET> operator+ () const { return *this; }
		bool operator!= (const CMtx4x4<ET> &m) const { return !(operator==(m)); }


		//+-----------------------------------------------------------------------
		//
		//  Member:    operator=
		//
		//  Synopsis:  general assigment from another matrix of arbitrary size
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET>& operator= (const CMtx<ET> &m)
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
		//  Member:    operator/
		//
		//  Synopsis:  division by a constant
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> operator/ (const ET &f)  const
		{
			CMtx4x4 mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = m_m[i] / f;
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator-
		//
		//  Synopsis:  unary minus
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> operator- () const
		{
			CMtx4x4 mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = -m_m[i];
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator+
		//
		//  Synopsis:  addition
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> operator+ (const CMtx4x4<ET> &m) const
		{
			CMtx4x4 mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = m_m[i] + m.m_m[i];
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator-
		//
		//  Synopsis:  subtraction
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> operator- (const CMtx4x4<ET> &m) const
		{
			CMtx4x4 mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = m_m[i] - m.m_m[i];
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*
		//
		//  Synopsis:  scaling by a constant
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> operator*(const ET &f) const
		{
			CMtx4x4<ET> mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = m_m[i] * f;
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator=
		//
		//  Synopsis:  set all elements to a constant
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET>& operator=(const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] = f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Init
		//
		//  Synopsis:  initialize from an array pointer
		//
		//  Returns:   the current CMtx4x4
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET>& Init(const ET *p, int iSize)
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
		//  Member:    operator+=
		//
		//  Synopsis:  add a matrix to the current matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET>& operator+= (const CMtx4x4<ET> &m)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] += m.m_m[i];
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator-=
		//
		//  Synopsis:  subtract a matrix from the current matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET>& operator-= (const CMtx4x4<ET> &m)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] -= m.m_m[i];
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*=
		//
		//  Synopsis:  multiply two matrices RHS multiply by RHS matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET>& operator*= (const CMtx4x4<ET> &m)
		{
			*this = *this * m;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*=
		//
		//  Synopsis:  scale the current matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET>& operator*= (const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] *= f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator/=
		//
		//  Synopsis:  divide the current matrix by a scale
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET>& operator/= (const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] /= f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*
		//
		//  Synopsis:  matrix multiply
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> operator*(const CMtx4x4<ET> &m) const
		{
			CMtx4x4<ET> mr;
			mr(0,0) = El(0,0) * m(0,0) + El(0,1) * m(1,0) + El(0,2) * m(2,0) + El(0,3) * m(3,0);
			mr(0,1) = El(0,0) * m(0,1) + El(0,1) * m(1,1) + El(0,2) * m(2,1) + El(0,3) * m(3,1);
			mr(0,2) = El(0,0) * m(0,2) + El(0,1) * m(1,2) + El(0,2) * m(2,2) + El(0,3) * m(3,2);
			mr(0,3) = El(0,0) * m(0,3) + El(0,1) * m(1,3) + El(0,2) * m(2,3) + El(0,3) * m(3,3);

			mr(1,0) = El(1,0) * m(0,0) + El(1,1) * m(1,0) + El(1,2) * m(2,0) + El(1,3) * m(3,0);
			mr(1,1) = El(1,0) * m(0,1) + El(1,1) * m(1,1) + El(1,2) * m(2,1) + El(1,3) * m(3,1);
			mr(1,2) = El(1,0) * m(0,2) + El(1,1) * m(1,2) + El(1,2) * m(2,2) + El(1,3) * m(3,2);
			mr(1,3) = El(1,0) * m(0,3) + El(1,1) * m(1,3) + El(1,2) * m(2,3) + El(1,3) * m(3,3);

			mr(2,0) = El(2,0) * m(0,0) + El(2,1) * m(1,0) + El(2,2) * m(2,0) + El(2,3) * m(3,0);
			mr(2,1) = El(2,0) * m(0,1) + El(2,1) * m(1,1) + El(2,2) * m(2,1) + El(2,3) * m(3,1);
			mr(2,2) = El(2,0) * m(0,2) + El(2,1) * m(1,2) + El(2,2) * m(2,2) + El(2,3) * m(3,2);
			mr(2,3) = El(2,0) * m(0,3) + El(2,1) * m(1,3) + El(2,2) * m(2,3) + El(2,3) * m(3,3);

			mr(3,0) = El(3,0) * m(0,0) + El(3,1) * m(1,0) + El(3,2) * m(2,0) + El(3,3) * m(3,0);
			mr(3,1) = El(3,0) * m(0,1) + El(3,1) * m(1,1) + El(3,2) * m(2,1) + El(3,3) * m(3,1);
			mr(3,2) = El(3,0) * m(0,2) + El(3,1) * m(1,2) + El(3,2) * m(2,2) + El(3,3) * m(3,2);
			mr(3,3) = El(3,0) * m(0,3) + El(3,1) * m(1,3) + El(3,2) * m(2,3) + El(3,3) * m(3,3);
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*
		//
		//  Synopsis:  multiply matrix by a 3x1 vector
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CVec4<ET> operator*(const CVec4<ET> &v) const
		{
			CVec4<ET> vr;
			vr(0) = El(0,0) * v(0) + El(0,1) * v(1) + El(0,2) * v(2) + El(0,3) * v(3);
			vr(1) = El(1,0) * v(0) + El(1,1) * v(1) + El(1,2) * v(2) + El(1,3) * v(3);
			vr(2) = El(2,0) * v(0) + El(2,1) * v(1) + El(2,2) * v(2) + El(2,3) * v(3);
			vr(3) = El(3,0) * v(0) + El(3,1) * v(1) + El(3,2) * v(2) + El(3,3) * v(3);
			return vr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    MakeI
		//
		//  Synopsis:  form the identity
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET>& MakeI()
		{
			memset(m_m, 0, Size() * sizeof(ET));
			m_m[0] = m_m[5] = m_m[10] = m_m[15] = (ET)1.0;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator==
		//
		//  Synopsis:  see if two matrices are equal
		//
		//  Returns:   true or false
		//
		//------------------------------------------------------------------------

		bool operator== (const CMtx4x4<ET> &m) const
		{
			int i;
			for(i=0; i<9; i++)
				if(m_m[i]!=m.m_m[i])
					break;
			return i==9;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Det
		//
		//  Synopsis:  finds the determinant of a 4x4 matrix
		//
		//  Returns:   the determinant
		//
		//------------------------------------------------------------------------

		ET Det() const
		{ 
			return m_m[0] * DeleteRowCol(0,0).Det()
				- m_m[1] * DeleteRowCol(0,1).Det()
				+ m_m[2] * DeleteRowCol(0,2).Det()
				- m_m[3] * DeleteRowCol(0,3).Det(); 
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    T
		//
		//  Synopsis:  transposes the 4x4 matrix
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> T() const
		{
			CMtx4x4<ET> mr;
			int i, j;
			for(i=0; i<Rows(); i++)
				for(j=0; j<Cols(); j++)
					mr(i,j) = El(j,i);
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    H
		//
		//  Synopsis:  conjugate transpose the 4x4 matrix
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> H() const
		{
			CMtx4x4<ET> mr;
			int i, j;
			for(i=0; i<Rows(); i++)
				for(j=0; j<Cols(); j++)
					mr(i,j) = VtConj(El(j,i));
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Conj
		//
		//  Synopsis:  conjugate the 4x4 matrix
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> Conj() const
		{
			CMtx4x4<ET> mr;
			int i, j;
			for(i=0; i<Rows(); i++)
				for(j=0; j<Cols(); j++)
					mr(i,j) = VtConj(El(i,j));
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    MaxAbs
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
		//  Member:    Dump
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

		//+-----------------------------------------------------------------------
		//
		//  Member:    DeleteRowCol
		//
		//  Synopsis:  delete a specific row and column to make a 3x3 matrix
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx3x3<ET> DeleteRowCol(int iRow, int iCol) const
		{
			CMtx3x3<ET> mr;
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
		//  Member:    MakeRotationX
		//
		//  Synopsis:  make a 4x4 rotation matrix from the angle given
		//             rotation is about the X axis
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> & MakeRotationX(ET fTheta)
		{
			ET fSin, fCos;
			VtSinCos( fTheta, &fSin, &fCos );

			El(0,0) = (ET)1; El(0,1) = (ET)0; El(0,2) = (ET)0;  El(0,3) = (ET)0; 
			El(1,0) = (ET)0; El(1,1) = fCos; El(1,2) = -fSin; El(1,3) = (ET)0; 
			El(2,0) = (ET)0; El(2,1) = fSin; El(2,2) = fCos;  El(2,3) = (ET)0; 
			El(3,0) = (ET)0; El(3,1) = (ET)0; El(3,2) = (ET)0;  El(3,3) = (ET)1;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    MakeRotationY
		//
		//  Synopsis:  make a 4x4 rotation matrix from the angle given
		//             rotation is about the Y axis
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> & MakeRotationY(ET fTheta)
		{
			ET fSin, fCos;
			VtSinCos( fTheta, &fSin, &fCos );

			El(0,0) = fCos;  El(0,1) = (ET)0; El(0,2) = fSin;  El(0,3) = (ET)0; 
			El(1,0) = (ET)0;  El(1,1) = (ET)1; El(1,2) = (ET)0;  El(1,3) = (ET)0; 
			El(2,0) = -fSin; El(2,1) = (ET)0; El(2,2) = fCos;  El(2,3) = (ET)0; 
			El(3,0) = (ET)0;  El(3,1) = (ET)0; El(3,2) = (ET)0;  El(3,3) = (ET)1;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    MakeRotationZ
		//
		//  Synopsis:  make a 4x4 rotation matrix from the angle given
		//             rotation is about the Z axis
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> & MakeRotationZ(ET fTheta)
		{
			ET fSin, fCos;
			VtSinCos( fTheta, &fSin, &fCos );

			El(0,0) = fCos; El(0,1) = -fSin; El(0,2) = (ET)0; El(0,3) = (ET)0; 
			El(1,0) = fSin; El(1,1) = fCos;  El(1,2) = (ET)0; El(1,3) = (ET)0; 
			El(2,0) = (ET)0; El(2,1) = (ET)0;  El(2,2) = (ET)1; El(2,3) = (ET)0; 
			El(3,0) = (ET)0; El(3,1) = (ET)0;  El(3,2) = (ET)0; El(3,3) = (ET)1;
			return *this;
		}


		//+-----------------------------------------------------------------------
		//
		//  Member:    Inv
		//
		//  Synopsis:  invert the 4x4 matrix
		//
		//  Returns:   the new matrix. returns {0} if exactly singular
		//
		//------------------------------------------------------------------------

		CMtx4x4<ET> Inv() const
		{
			CMtx4x4<ET> mr;
			mr.MakeI();
			CMtx4x4<ET> m = *this;

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
					ExchangeRows4(iCol, iPivotRow, m);
					ExchangeRows4(iCol, iPivotRow, mr);
				}
				fPivot = m(iCol,iCol);
				ScaleRow4(iCol, ((ET)1)/fPivot, m);
				ScaleRow4(iCol, ((ET)1)/fPivot, mr);
				for (iRow = 0; iRow < Rows(); iRow++)
					if (iRow != iCol) 
					{
						f = -m(iRow,iCol);
						AddScaled4(iCol, f, iRow, m);
						AddScaled4(iCol, f, iRow, mr);
					}
			}

			return mr;
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

	protected:
		ET m_m[16];
	};


	typedef CVec4<float>  CVec4f;
	typedef CVec4<double> CVec4d;
	typedef CMtx4x4<float>  CMtx4x4f;
	typedef CMtx4x4<double> CMtx4x4d;
	typedef CVec4<Complex<float> > CVec4cf;
	typedef CVec4<Complex<double> > CVec4cd;
	typedef CMtx4x4<Complex<float> > CMtx4x4cf;
	typedef CMtx4x4<Complex<double> > CMtx4x4cd;

	template <>	template <typename RHS>
	CVec4<double>::CVec4(const CVec4<RHS>& v)
	{ 
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
	}  

	template <typename ET> template <typename RHS>
	CVec4<ET>::CVec4(const CVec4<RHS>& v)
	{ 
		static_assert(sizeof(CVec4<RHS>::ELT) != sizeof(RHS), 
			"CVec4 copy constructor only defined for CVec4<double> to avoid loss of precision");
	} 

	template <class T>
	CMtx4x4< Complex<T> > VtComplexify(const CMtx4x4<T> &mtx)
	{
		CMtx4x4< Complex<T> > out;
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
	CVec4< Complex<T> > VtComplexify(const CVec4<T> &v)
	{
		CVec4< Complex<T> > out;
		out(0).Re = v(0);
		out(0).Im = 0;
		out(1).Re = v(1);
		out(1).Im = 0;
		out(2).Re = v(2);
		out(2).Im = 0;
		out(3).Re = v(3);
		out(3).Im = 0;
		return out;
	}

	template <class T>
	CMtx4x4<T> VtRe(const CMtx4x4< Complex<T> > &mtx)
	{
		CMtx4x4<T> out;
		int i,j;
		for(i=0; i<mtx.Rows(); i++)
			for(j=0; j<mtx.Cols(); j++)
				out(i,j) = mtx(i,j).Re;
		return out;
	}

	template <class T>
	CVec4<T> VtRe(const CVec4< Complex<T> > &v)
	{
		CVec4<T> out;
		out(0) = v(0).Re;
		out(1) = v(1).Re;
		out(2) = v(2).Re;
		out(3) = v(3).Re;
		return out;
	}

	template <class T>
	CMtx4x4<T> VtIm(const CMtx4x4< Complex<T> > &mtx)
	{
		CMtx4x4<T> out;
		int i,j;
		for(i=0; i<mtx.Rows(); i++)
			for(j=0; j<mtx.Cols(); j++)
				out(i,j) = mtx(i,j).Im;
		return out;
	}

	template <class T>
	CVec4<T> VtIm(const CVec4< Complex<T> > &v)
	{
		CVec4<T> out;
		out(0) = v(0).Im;
		out(1) = v(1).Im;
		out(2) = v(2).Im;
		out(3) = v(3).Im;
		return out;
	}

	//
	// define conversions between specific types
	//
	template <typename TS, typename TD>
	inline CMtx4x4<TD>& VtConvertMtx(const CMtx4x4<TS>& s, CMtx4x4<TD>& d)
	{
#define cnvrow(r) d(r,0) = TD(s(r,0)); d(r,1) = TD(s(r,1));\
	d(r,2) = TD(s(r,2)); d(r,3) = TD(s(r,3));
		cnvrow(0) cnvrow(1) cnvrow(2) cnvrow(3)
#undef cnvrow
			return d;
	}

	template <class T> 
	inline CMtx4x4<T> operator*(const T &f, const CMtx4x4<T> &m) { return m*f; }

	template <class T> 
	inline CVec4<T> operator*(const T &f, const CVec4<T> &v) { return v*f; }

};
