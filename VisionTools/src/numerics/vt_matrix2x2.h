//+-----------------------------------------------------------------------------
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
//------------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"

#include "vt_mathutils.h"

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
	// MakeRotation(t) - make rotation matrix
	// ET() - return the transpose
	// Inv() - return the inverse
	// Tr() - return the trace
	// Det() - return the determinant
	// Dump() - print in matlab format

	template <class ET>
	class CVec2
	{
	public:
		typedef ET ELT;
		static int Bands() { return 2; }

		CVec2() {}
		explicit CVec2(const CVec<ET> &v) { operator=(v); }
		explicit CVec2(const ET &f) { operator=(f); }
		CVec2(const ET &m0, const ET &m1) { x = m0; y = m1; }

		template <typename RHS>
		CVec2(const CVec2<RHS>& v);

		int Size() const { return 2; }

		const ET *Ptr() const { return &x; }
		ET *Ptr() { return &x; }

		const ET &El(int i) const { return (&x)[i]; }
		ET &El(int i) { return (&x)[i]; }
		const ET &operator[] (int i) const { return (&x)[i]; }
		ET &operator[] (int i) { return (&x)[i]; }
		const ET &operator() (int i) const { return (&x)[i]; }
		ET &operator() (int i) { return (&x)[i]; }

		CVec2<ET> operator+ () const { return *this; }
		CVec2<ET> operator- () const { return CVec2<ET>(-El(0), -El(1)); }
		CVec2<ET> operator+ (const CVec2<ET> &v) const { return CVec2<ET>(El(0) + v[0], El(1) + v[1]); }
		CVec2<ET> operator- (const CVec2<ET> &v) const { return CVec2<ET>(El(0) - v[0], El(1) - v[1]); }
		ET operator* (const CVec2<ET> &v) const { return v[0] * El(0) + v[1] * El(1); }
		CVec2<ET> operator* (const ET &f) const { return CVec2<ET>(El(0) * f, El(1) * f); }
		CVec2<ET> operator/ (const ET &f) const { return CVec2<ET>(El(0) / f, El(1) / f); }
		bool operator== (const CVec2<ET> &v) const { return El(0)==v[0] && El(1)==v[1]; }
		bool operator!= (const CVec2<ET> &v) const { return !(operator==(v)); }

		ET MagnitudeSq() const { return VtMagnitudeSq(El(0)) + VtMagnitudeSq(El(1)); }
		ET Magnitude() const { return VtHypot(El(0), El(1)); }
		CVec2<ET> Unit() const { return *this/Magnitude(); }
		CVec3<ET> Hom() const { return CVec3<ET>(*this); }
		ET Arg() const { return ET(atan2(VtReScalar(El(1)), VtReScalar(El(0)))); }
		CVec2<ET> Conj() const { return CVec2<ET>(VtConj(El(0)), VtConj(El(1))); }
        CVec2<ET> Perp() const { return CVec2<ET>(-El(1), El(0)); }

		CVec2<ET> & operator= (const ET &f) { El(0) = f; El(1) = f; return *this; }
		CVec2<ET> & operator+= (const CVec2<ET> &v) { El(0) += v[0]; El(1) += v[1]; return *this; }
		CVec2<ET> & operator-= (const CVec2<ET> &v) { El(0) -= v[0]; El(1) -= v[1]; return *this; }
		CVec2<ET> & operator*= (const ET &f) { El(0) *= f; El(1) *= f; return *this; }
		CVec2<ET> & operator/= (const ET &f) { El(0) /= f; El(1) /= f; return *this; }

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec2<ET>::operator=
		//
		//  Synopsis:  assigment operator from arbitrary vector types
		//
		//  Returns:   the new CVec2
		//
		//------------------------------------------------------------------------

		CVec2<ET> & operator= (const CVec<ET> &v)
		{
			if(v.Size()>0)
				El(0) = v[0];
			if(v.Size()>1)
				El(1) = v[1];
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec2<ET>::Init
		//
		//  Synopsis:  initialize from an array pointer
		//
		//  Returns:   the current CVec2
		//
		//------------------------------------------------------------------------

		CVec2<ET>& Init(const ET *p, int iSize)
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
		//  Member:    CVec2<ET>::operator^
		//
		//  Synopsis:  outer product operator
		//
		//  Returns:   the outer product matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET> operator^ (const CVec2<ET> &v) const
		{
			CMtx2x2<ET> m;
			m(0,0) = El(0) * v[0];
			m(0,1) = El(0) * v[1];
			m(1,0) = El(1) * v[0];
			m(1,1) = El(1) * v[1];
			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec2<ET>::operator*
		//
		//  Synopsis:  right hand size 2x2 matrix multiply
		//
		//  Returns:   the new CVec2
		//
		//------------------------------------------------------------------------

		CVec2<ET> operator* (const CMtx2x2<ET> &m) const
		{
			return CVec2<ET>(El(0) * m(0,0) + El(1) * m(1,0), 
				El(0) * m(0,1) + El(1) * m(1,1));
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec2<ET>::operator*=
		//
		//  Synopsis:  right hand size 2x2 matrix multiply
		//
		//  Returns:   the current CVec2
		//
		//------------------------------------------------------------------------

		CVec2<ET>& operator*= (const CMtx2x2<ET> &m)
		{
			*this = *this * m;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CVec2<ET>::SortAbs
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
		//  Member:    CVec2<ET>::MaxAbs
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
		//  Member:    CVec2<ET>::Dump
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
		ET x, y;
	};


	template <class ET>
	class CMtx2x2
	{
	public:
		CMtx2x2() {}
		CMtx2x2(const CMtx2x2<float> &m) 
		{ m_m[0] = (ET)m(0,0); m_m[1] = (ET)m(0,1); m_m[2] = (ET)m(1,0); m_m[3] = (ET)m(1,1); }
		CMtx2x2(const CMtx2x2<double> &m) 
		{ m_m[0] = (ET)m(0,0); m_m[1] = (ET)m(0,1); m_m[2] = (ET)m(1,0); m_m[3] = (ET)m(1,1); }
		explicit CMtx2x2(const CMtx<ET> &m) { operator=(m); }
		explicit CMtx2x2(const ET &f) { operator=(f); }
		CMtx2x2(const ET *p, int iSize) { memcpy(Ptr(), p, sizeof(ET) * VtMin(Size(), iSize)); }
		CMtx2x2(ET _00, ET _01,
			ET _10, ET _11 ) 
		{ m_m[0] = _00; m_m[1] = _01;
		m_m[2] = _10; m_m[3] = _11; }

		int Rows() const { return 2; }
		int Cols() const { return 2; }
		int Size() const { return Rows() * Cols(); }

		const ET *Ptr() const { return m_m; }
		ET *Ptr() { return m_m; }

		CVec2<ET> GetRow(int i) const { return CVec2<ET>(m_m[i*Cols()+0], m_m[i*Cols()+1]); }
		CVec2<ET> GetCol(int i) const { return CVec2<ET>(m_m[i], m_m[i+Cols()]); }
		const ET &El(int iRow, int iCol) const { return m_m[iRow * Cols() + iCol]; }
		ET &El(int iRow, int iCol) { return m_m[iRow * Cols() + iCol]; }
		const ET *operator[] (int i) const { return m_m + i * Cols(); }
		const ET &operator() (int iRow, int iCol) const { return m_m[iRow * Cols() + iCol]; }
		ET *operator[] (int i) { return m_m + i * Cols(); }
		ET &operator() (int iRow, int iCol) { return El(iRow, iCol); }

		CMtx2x2<ET> & MakeDiag(const CVec2<ET> &v) { El(0,0) = v[0]; El(0,1) = 0; El(1,0) = 0; El(1,1) = v[1]; return *this; }
		CMtx2x2<ET> & MakeScale(ET sx, ET sy) {return MakeDiag(CVec2<ET>(sx, sy)); }
		CMtx2x2<ET> & MakeSymmetric() { m_m[2] = m_m[1]; return *this; } // copies below diagonal entries from above diagonal
		ET Tr() const { return m_m[0] + m_m[3]; }
		ET Det() const { return m_m[0] * m_m[3] - m_m[1] * m_m[2]; }

		CMtx2x2<ET> & SetRow(int i, const CVec2<ET> &v) { El(i,0) = v[0]; El(i,1) = v[1]; return *this; }
		CMtx2x2<ET> & SetCol(int i, const CVec2<ET> &v) { El(0,i) = v[0]; El(1,i) = v[1]; return *this; }
		CVec2<ET> GetDiag() const { return CVec2<ET>(El(0,0), El(1,1)); }

		CMtx2x2<ET> operator+ () const { return *this; }
		bool operator!= (const CMtx2x2<ET> &m) const { return !(operator==(m)); }

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator=
		//
		//  Synopsis:  general assigment from another matrix of arbitrary size
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET>& operator= (const CMtx<ET> &m)
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
		//  Member:    CMtx2x2<ET>::operator/
		//
		//  Synopsis:  division by a constant
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET> operator/ (const ET &f)  const
		{
			CMtx2x2 mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = m_m[i] / f;
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator-
		//
		//  Synopsis:  unary minus
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET> operator- () const
		{
			CMtx2x2 mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = -m_m[i];
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator+
		//
		//  Synopsis:  addition
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET> operator+ (const CMtx2x2<ET> &m) const
		{
			CMtx2x2 mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = m_m[i] + m.m_m[i];
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator-
		//
		//  Synopsis:  subtraction
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET> operator- (const CMtx2x2<ET> &m) const
		{
			CMtx2x2 mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = m_m[i] - m.m_m[i];
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator*
		//
		//  Synopsis:  scaling by a constant
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET> operator*(const ET &f) const
		{
			CMtx2x2<ET> mr;
			int i;
			for(i=0; i<Size(); i++)
				mr.m_m[i] = m_m[i] * f;
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator=
		//
		//  Synopsis:  set all elements to a constant
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET>& operator=(const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] = f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::Init
		//
		//  Synopsis:  initialize from an array pointer
		//
		//  Returns:   the current CMtx2x2
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET>& Init(const ET *p, int iSize)
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
		//  Member:    CMtx2x2<ET>::operator+=
		//
		//  Synopsis:  add a matrix to the current matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET>& operator+= (const CMtx2x2<ET> &m)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] += m.m_m[i];
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator-=
		//
		//  Synopsis:  subtract a matrix to the current matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET>& operator-= (const CMtx2x2<ET> &m)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] -= m.m_m[i];
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator+=
		//
		//  Synopsis:  multiply two matrices RHS multiply by RHS matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET>& operator*= (const CMtx2x2<ET> &m)
		{
			*this = *this * m;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator*=
		//
		//  Synopsis:  scale the current matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET>& operator*= (const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] *= f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator/=
		//
		//  Synopsis:  divide the current matrix by a scale
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET>& operator/= (const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				m_m[i] /= f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator*
		//
		//  Synopsis:  matrix multiply
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET> operator*(const CMtx2x2<ET> &m) const
		{
			CMtx2x2<ET> mr;
			mr(0,0) = El(0,0) * m(0,0) + El(0,1) * m(1,0);
			mr(0,1) = El(0,0) * m(0,1) + El(0,1) * m(1,1);
			mr(1,0) = El(1,0) * m(0,0) + El(1,1) * m(1,0);
			mr(1,1) = El(1,0) * m(0,1) + El(1,1) * m(1,1);
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator*
		//
		//  Synopsis:  multiply matrix by a 2x1 vector
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CVec2<ET> operator*(const CVec2<ET> &v) const
		{
			CVec2<ET> vr;
			vr(0) = El(0,0) * v(0) + El(0,1) * v(1);
			vr(1) = El(1,0) * v(0) + El(1,1) * v(1);
			return vr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::MakeI
		//
		//  Synopsis:  form the identity
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET>& MakeI()
		{
			m_m[0] = m_m[3] = ET(1.0);
			m_m[1] = m_m[2] = 0;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::MakeRotation
		//
		//  Synopsis:  make a 2x2 rotation matrix from the angle given
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET>& MakeRotation(ET fTheta)
		{
			ET fSin, fCos;
			VtSinCos( fTheta, &fSin, &fCos );
			El(0,0) = fCos; El(0,1) = -fSin;
			El(1,0) = fSin; El(1,1) = fCos;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::operator==
		//
		//  Synopsis:  see if two matrices are equal
		//
		//  Returns:   true or false
		//
		//------------------------------------------------------------------------

		bool operator== (const CMtx2x2<ET> &m) const
		{
			return m(0,0)==El(0,0) && m(0,1)==El(0,1) && m(1,0)==El(1,0) && m(1,1)==El(1,1);
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::MaxAbs
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
		//  Member:    CMtx2x2<ET>::Inv
		//
		//  Synopsis:  invert the 2x2 matrix
		//
		//  Returns:   the new matrix. returns {0 0; 0 0} if exactly singular
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET> Inv() const
		{
			CMtx2x2<ET> mr;
			ET fDet = Det();
			if(fDet==0)
				mr = (ET)0;
			else
			{
				mr(0,0) = El(1,1) / fDet;
				mr(0,1) = -El(0,1) / fDet;
				mr(1,0) = -El(1,0) / fDet;
				mr(1,1) = El(0,0) / fDet;
			}
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::T
		//
		//  Synopsis:  transpose the 2x2 matrix
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET> T() const
		{
			CMtx2x2<ET> mr;
			mr(0,0) = El(0,0);
			mr(0,1) = El(1,0);
			mr(1,0) = El(0,1);
			mr(1,1) = El(1,1);
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::H
		//
		//  Synopsis:  conjugate transpose the 2x2 matrix
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET> H() const
		{
			CMtx2x2<ET> mr;
			mr(0,0) = VtConj(El(0,0));
			mr(0,1) = VtConj(El(1,0));
			mr(1,0) = VtConj(El(0,1));
			mr(1,1) = VtConj(El(1,1));
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::Conj
		//
		//  Synopsis:  conjugate the 2x2 matrix
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx2x2<ET> Conj() const
		{
			CMtx2x2<ET> mr;
			mr(0,0) = VtConj(El(0,0));
			mr(0,1) = VtConj(El(0,1));
			mr(1,0) = VtConj(El(1,0));
			mr(1,1) = VtConj(El(1,1));
			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    CMtx2x2<ET>::Dump
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
		//  Matrix arithmetic operations.
		//
		//  Synopsis:  Multiplying fixed size matrices by general CMtx<ET>
		//
		//  Returns:   CMtx<ET>
		//
		//------------------------------------------------------------------------

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
		ET m_m[4];
	};

	typedef CVec2<float> CVec2f;
	typedef CVec2<double> CVec2d;
	typedef CMtx2x2<float> CMtx2x2f;
	typedef CMtx2x2<double> CMtx2x2d;
	typedef CVec2<Complex<float> > CVec2cf;
	typedef CVec2<Complex<double> > CVec2cd;
	typedef CMtx2x2<Complex<float> > CMtx2x2cf;
	typedef CMtx2x2<Complex<double> > CMtx2x2cd;

	template <>	template <typename RHS>
	CVec2<double>::CVec2(const CVec2<RHS>& v)
	{ 
		x = v.x;
		y = v.y;
	}  

	template <typename ET> template <typename RHS>
	CVec2<ET>::CVec2(const CVec2<RHS>& v)
	{ 
		static_assert(sizeof(CVec2<RHS>::ELT) != sizeof(RHS), 
			"CVec2 copy constructor only defined for CVec2<double> to avoid loss of precision");
	} 

	template <class T>
	CMtx2x2< Complex<T> > VtComplexify(const CMtx2x2<T> &mtx)
	{
		CMtx2x2< Complex<T> > out;
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
	CVec2< Complex<T> > VtComplexify(const CVec2<T> &v)
	{
		CVec2< Complex<T> > out;
		out(0).Re = v(0);
		out(0).Im = 0;
		out(1).Re = v(1);
		out(1).Im = 0;
		return out;
	}

	template <class T>
	CMtx2x2<T> VtRe(const CMtx2x2< Complex<T> > &mtx)
	{
		CMtx2x2<T> out;
		int i,j;
		for(i=0; i<mtx.Rows(); i++)
			for(j=0; j<mtx.Cols(); j++)
				out(i,j) = mtx(i,j).Re;
		return out;
	}

	template <class T>
	CVec2<T> VtRe(const CVec2< Complex<T> > &v)
	{
		CVec2<T> out;
		out(0) = v(0).Re;
		out(1) = v(1).Re;
		return out;
	}

	template <class T>
	CMtx2x2<T> VtIm(const CMtx2x2< Complex<T> > &mtx)
	{
		CMtx2x2<T> out;
		int i,j;
		for(i=0; i<mtx.Rows(); i++)
			for(j=0; j<mtx.Cols(); j++)
				out(i,j) = mtx(i,j).Im;
		return out;
	}

	template <class T>
	CVec2<T> VtIm(const CVec2< Complex<T> > &v)
	{
		CVec2<T> out;
		out(0) = v(0).Im;
		out(1) = v(1).Im;
		return out;
	}

	//
	// define conversions between specific types
	//
	template <typename TS, typename TD>
	inline CMtx2x2<TD>& VtConvertMtx(const CMtx2x2<TS>& s, CMtx2x2<TD>& d)
	{
#define cnvrow(r) d(r,0) = TD(s(r,0)); d(r,1) = TD(s(r,1));
		cnvrow(0) cnvrow(1)
#undef cnvrow
			return d;
	}

	template <class T> 
	inline CMtx2x2<T> operator*(const T &f, const CMtx2x2<T> &m) { return m*f; }

	template <class T> 
	inline CVec2<T> operator*(const T &f, const CVec2<T> &v) { return v*f; }

};
