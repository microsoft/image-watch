//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Numerical matrix routines
//
//  History:
//      2004/5/13-swinder
//          Created
//
//------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_complex.h"
#include "vt_matrix2x2.h"
#include "vt_matrix3x3.h"
#include "vt_matrix4x4.h"

namespace vt {

	// --------------------------------------------------------------------------------------------------------
	// CVec - general length n column vector
	// --------------------------------------------------------------------------------------------------------
	//
	// *** constructors
	// CVec(size) - uninitialized
	// CVec(v) - initialise from another vector, can be CVec
	// HRESULT Create(size) - construct with error check return
	// bool IsError() - returns error status (out of memory or bad arguments)
	// HRESULT GetError() - return hresult error
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
	// vec.Init p - sets vector to elements pointed to by p
	// vec = s - sets vector elements to scalar value s
	// vec += vec - vector addition
	// vec -= vec - vector subtraction
	// vec *= s - vector scaling
	// vec /= s - vector scaling by division
	// vec *= mtx - right sided vector multiply
	//
	// *** methods
	// El(i) - return reference to element i
	// SortAbs(int *piMapTable) - makes a table of pointers to elements sorted by magnitude
	// int MaxAbs() - finds index of element with max magnitude
	// Dump() - print in matlab format

	// --------------------------------------------------------------------------------------------------------
	// CMtx - general rows x cols matrix
	// --------------------------------------------------------------------------------------------------------
	//
	// *** constructors
	// CMtx(rows, cols) - uninitialized
	// CMtx(m) - initialise from another matrix, can be CMtx, CMtx2x2, CMtx3x3 or CMtx4x4.
	// HRESULT Create(rows, cols) - construct with error check return
	// bool IsError() - returns error status (out of memory or bad arguments)
	// HRESULT GetError() - return hresult error
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
	// mtx Init p - sets matrix to elements pointed to by p
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
	// SetRow(v,i) - set row i
	// SetCol(v,i) - set column i
	// GetDiag() - returns a vector comprised of the diagonal elements
	// MakeDiag(v) - initialises to a diagonal matrix with the given diagonal
	// El(r,c) - return reference to element in row r, column c
	// MakeI() - set to identity matrix
	// T() - return the transpose
	// Tr() - return the trace
	// Dump() - print in matlab format

	template <class ET>
	class CVec : public CErrorBase
	{
	public:
		CVec() : m_iSize(0), m_p(NULL), m_bWrap(false) {}
		CVec(const CVec<ET> &v) : m_iSize(0), m_p(NULL) { operator=(v); }
		CVec(int iSize) : m_iSize(0), m_p(NULL) { Create(iSize); }
		CVec(const ET *p, int iSize) : m_iSize(0), m_p(NULL) { Create(iSize); Init(p, iSize); }

		//+-----------------------------------------------------------------------
		//
		//  Member:    ~CVec()
		//
		//  Synopsis:  destructor
		//
		//  Returns:   none
		//
		//------------------------------------------------------------------------

		virtual ~CVec()
		{
			if(m_p && !m_bWrap)
				delete m_p;
		}

		int Size() const { return m_iSize; }

		const ET *Ptr() const { return m_p; }
		ET *Ptr() { return m_p; }

		const ET &El(int i) const { return m_p[i]; }
		ET &El(int i) { return m_p[i]; }
		const ET &operator[] (int i) const { return m_p[i]; }
		ET &operator[] (int i) { return m_p[i]; }
		const ET &operator() (int i) const { return m_p[i]; }
		ET &operator() (int i) { return m_p[i]; }

		CVec<ET> operator+ () const { return *this; }
		bool operator!= (const CVec<ET> &v) const { return !(operator==(v)); }

		ET Magnitude() const { return (ET)sqrt(VtReScalar(MagnitudeSq())); }
		CVec<ET> Unit() const { return *this/Magnitude(); }

		CVec<ET> & operator*= (const CMtx<ET> &m) { *this = *this * m; return *this; }

		//+-----------------------------------------------------------------------
		//
		//  Member:    Wrap()
		//
		//  Synopsis:  wrap around some memory
		//
		//  Returns:   none
		//
		//------------------------------------------------------------------------

		void Wrap(ET *p, int iSize)
		{
			ClearError();
			Free();
			m_p = p;
			m_iSize = iSize;
			m_bWrap = true;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Create()
		//
		//  Synopsis:  create a vector of the specified size
		//
		//  Returns:   S_OK if no problems
		//
		//------------------------------------------------------------------------

		HRESULT Create(int iSize)
		{
			ClearError();

			if(m_bWrap)
				m_p = NULL;

			m_bWrap = false;

			if(iSize<=0)
			{
				Free();
				return NOERROR;
			}

			if(m_p && iSize==m_iSize)
				return NOERROR;

			if(m_p)
				delete m_p;

			m_p = VT_NOTHROWNEW ET [iSize];
			if(m_p)
			{
				m_iSize = iSize;
				return NOERROR;
			}

			m_iSize = 0;

			return SetError(E_OUTOFMEMORY);
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Free
		//
		//  Synopsis:  deallocates the memory and sets size to 0x0
		//
		//  Returns:   none
		//
		//------------------------------------------------------------------------

		void Free()
		{
			m_iSize = 0;
			if(m_p && !m_bWrap)
				delete m_p;
			m_p = NULL;
			m_bWrap = false;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator=
		//
		//  Synopsis:  assigment operator from arbitrary vector types
		//
		//  Returns:   the current CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> & operator= (const CVec<ET> &v)
		{
			// if v has out of memory error then its size will be zero
			if((m_bWrap && Size()==v.Size()) || SUCCEEDED(Create(v.Size())))
			{
				SetError(v.GetError()); // pass on the error status

				if(!IsError() && v.m_p && m_p)
					memcpy(m_p, v.m_p, v.Size() * sizeof(ET));
			}

			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator-
		//
		//  Synopsis:  unary minus operator
		//
		//  Returns:   the new CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> operator- () const
		{
			CVec<ET> v(Size());
			v.SetError(GetError());
			if(!v.IsError())
			{
				int i;
				for(i=0; i<Size(); i++)
					v(i) = -El(i);
			}
			return v;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator+
		//
		//  Synopsis:  vector addition
		//
		//  Returns:   the new CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> operator+ (const CVec<ET> &v) const
		{
			CVec<ET> q(Size());
			q.SetError(GetError());
			q.SetError(v.GetError());
			if(!q.IsError())
			{
				if(v.Size()!=Size())
				{
					q = (ET)0;
					q.SetError(E_INVALIDARG);
					return q;
				}
				int i;
				for(i=0; i<Size(); i++)
					q(i) = El(i) + v[i];
			}
			return q;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator-
		//
		//  Synopsis:  vector substraction
		//
		//  Returns:   the new CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> operator- (const CVec<ET> &v) const
		{
			CVec<ET> q(Size());
			q.SetError(GetError());
			q.SetError(v.GetError());
			if(!q.IsError())
			{
				if(v.Size()!=Size())
				{
					q = (ET)0;
					q.SetError(E_INVALIDARG);
					return q;
				}
				int i;
				for(i=0; i<Size(); i++)
					q(i) = El(i) - v[i];
			}
			return q;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator^
		//
		//  Synopsis:  matrix outer product between vectors
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> operator^ (const CVec<ET> &v) const
		{
			CMtx<ET> m(Size(), v.Size());
			m.SetError(GetError());
			m.SetError(v.GetError());
			if(m.IsError())
				return m;
			int i, j;
			for(i=0; i<Size(); i++)
				for(j=0; j<v.Size(); j++)
					m(i,j) = El(i) * v(j);
			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*
		//
		//  Synopsis:  dot product
		//
		//  Returns:   the new CVec
		//
		//------------------------------------------------------------------------

		ET operator* (const CVec<ET> &v) const
		{
			if(v.Size()!=Size())
			{
				//SetError(E_INVALIDARG);
				return 0;
			}
			ET f = 0;
			int i;
			for(i=0; i<Size(); i++)
				f += El(i) *  v[i];

			return f;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*
		//
		//  Synopsis:  rescale the vector
		//
		//  Returns:   the new CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> operator* (const ET &f) const
		{
			CVec<ET> v(Size());
			v.SetError(GetError());
			if(!v.IsError())
			{
				int i;
				for(i=0; i<Size(); i++)
					v(i) = El(i)*f;
			}
			return v;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*
		//
		//  Synopsis:  right sided matrix multiply
		//
		//  Returns:   the new CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> operator* (const CMtx<ET> &m) const
		{
			CVec<ET> v(m.Cols());
			v.SetError(m.GetError());
			v.SetError(GetError());

			if(!v.IsError())
			{
				if(m.Rows()!=Size())
				{
					v.SetError(E_INVALIDARG);
					v = (ET)0;
					return v;
				}

				int i, j;
				for(j=0; j<m.Cols(); j++)
				{
					ET f = 0;
					for(i=0; i<m.Rows(); i++)
						f += El(i) * m(i, j);
					v[j] = f;
				}
			}
			return v;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator/
		//
		//  Synopsis:  divide by a scalar
		//
		//  Returns:   the new CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> operator/ (const ET &f) const
		{
			CVec<ET> v(Size());
			v.SetError(GetError());

			if(!v.IsError())
			{
				int i;
				for(i=0; i<Size(); i++)
					v(i) = El(i)/f;
			}
			return v;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Conj
		//
		//  Synopsis:  compute the conjugate vector
		//
		//  Returns:   the new CVec
		//
		//------------------------------------------------------------------------
		CVec<ET> Conj() const
		{
			CVec<ET> v(Size());
			v.SetError(GetError());

			if(!v.IsError())
			{
				int i;
				for(i=0; i<Size(); i++)
					v(i) = VtConj(El(i));
			}
			return v;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator==
		//
		//  Synopsis:  vector compare
		//
		//  Returns:   true or false
		//
		//------------------------------------------------------------------------

		bool operator== (const CVec<ET> &v) const
		{
			if(IsError() || v.IsError())
				return false;

			if(v.Size()==Size())
			{
				int i;
				for(i=0; i<Size(); i++)
					if(v[i]!=El(i))
						return false;
				return true;
			}

			return false;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    MagnitudeSq
		//
		//  Synopsis:  calculate the squared magnitude of the vector
		//
		//  Returns:   magnitude
		//
		//------------------------------------------------------------------------

		ET MagnitudeSq() const
		{
			ET fMagSq = 0;
			int i;
			for(i=0; i<Size(); i++)
			{
				fMagSq += VtMagnitudeSq(El(i));
			}

			return fMagSq;
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

			if(piMapTable==NULL || Size()==0 || IsError())
				return; // silent return

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
		//  Member:    Init
		//
		//  Synopsis:  initialize from an array pointer
		//
		//  Returns:   the current CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> & Init(const ET *p, int iSize)
		{
			if(p && Ptr() && iSize>0)
			{
				iSize = VtMin(Size(), iSize);
				memcpy(Ptr(), p, sizeof(ET) * iSize); // memcpy handles size = 0 case
			}
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator=
		//
		//  Synopsis:  assignment from a scalar
		//
		//  Returns:   the current CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> & operator= (const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				El(i) = f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator+=
		//
		//  Synopsis:  add two vectors
		//
		//  Returns:   the current CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> & operator+= (const CVec<ET> &v)
		{ 
			SetError(v.GetError());

			if(!IsError())
			{
				int i;
				int iSize = VtMin(Size(), v.Size());
				for(i=0; i<iSize; i++)
					El(i) += v[i];
			}
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator-=
		//
		//  Synopsis:  subtract a vector
		//
		//  Returns:   the current CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> & operator-= (const CVec<ET> &v)
		{ 
			SetError(v.GetError());

			if(!IsError())
			{
				int i;
				int iSize = VtMin(Size(), v.Size());
				for(i=0; i<iSize; i++)
					El(i) -= v[i];
			}

			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*=
		//
		//  Synopsis:  multiply by a scalar
		//
		//  Returns:   the current CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> & operator*= (const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				El(i) *= f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator/=
		//
		//  Synopsis:  divide by a scalar
		//
		//  Returns:   the current CVec
		//
		//------------------------------------------------------------------------

		CVec<ET> & operator/= (const ET &f)
		{ 
			int i;
			for(i=0; i<Size(); i++)
				El(i) /= f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Extract
		//
		//  Synopsis:  extract a sub vector from the current vector
		//
		//  Returns:   none
		//
		//------------------------------------------------------------------------

		CVec<ET> Extract(int iSize, int iStart) const
		{
			CVec<ET> m(iSize);

			m.SetError(GetError());

			if(!m.IsError())
			{
				int i;
				for(i=0; i<iSize; i++)
				{
					int iLoc = i + iStart;
					if(iLoc>=0 && iLoc<Size())
						m[i] = El(iLoc);
					else
						m[i] = 0;
				}
			}

			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Update
		//
		//  Synopsis:  overwrite a sub vector in the current vector
		//
		//  Returns:   none
		//
		//------------------------------------------------------------------------

		CVec<ET> & Update(const CVec<ET> &vec, int iStart)
		{
			SetError(vec.GetError());

			if(!IsError())
			{
				int i;
				for(i=0; i<vec.Size(); i++)
				{
					int iLoc = i + iStart;
					if(iLoc>=0 && iLoc<Size())
						El(iLoc) = vec[i];
				}
			}

			return *this;
		}
		// convert to non-homogenous system
		CVec<ET> Dehom() const
		{
			CVec<ET> v;
			if(Size()<2)
			{
				v.SetError(E_INVALIDARG);
				return v;
			}
			if(IsError())
				v.SetError(GetError());
			v.Create(Size()-1);
			if(v.IsError())
				return v;
			ET fac = ((ET)1.0)/El(Size()-1);
			int i;
			for (i=0; i<Size()-1; i++)
				v[i] = El(i) * fac;
			return v;
		}

		// convert to homogenous system
		CVec<ET> Hom() const
		{
			CVec<ET> v;
			if(IsError())
				v.SetError(GetError());
			v.Create(Size()+1);
			if(v.IsError())
				return v;
			int i;
			for(i=0; i<Size(); i++)
				v[i] = El(i);
			v[Size()] = (ET)1.0;
			return v;
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
			if(IsError())
				fprintf(fp, "[ Error ]\n");
			else
				FormatVecStdio(*this, fp);
		}

	private:
		int m_iSize;
		ET *m_p;
		bool m_bWrap;
	};

	// functions to support inverse

	template <class ET>
	inline static void
		ExchangeRows(int iRow1, int iRow2, CMtx<ET> &mat)
	{
		ET f;
		int i;
		for(i=0; i<mat.Cols(); i++)
		{
			f = mat[iRow1][i]; 
			mat[iRow1][i] = mat[iRow2][i];
			mat[iRow2][i] = f;
		}
	}

	template <class ET>
	inline static void
		ScaleRow(int iRow, ET f, CMtx<ET> &mat)
	{
		int i;
		for(i=0; i<mat.Cols(); i++)
			mat[iRow][i] *= f;
	}

	template <class ET>
	inline static void
		AddScaled(int iRow1, ET f, int iRow2, CMtx<ET> &mat)
	{
		int i;
		for(i=0; i<mat.Cols(); i++)
			mat[iRow2][i] += f*mat[iRow1][i];
	}
	
	template <class ET>
	class CMtx : public CErrorBase
	{
	public:
		CMtx() : m_iRows(0), m_iCols(0), m_p(NULL), m_bWrap(false) {}
		CMtx(const CMtx<ET> &m) : m_iRows(0), m_iCols(0), m_p(NULL) { operator=(m); }
		CMtx(int iRows, int iCols) : m_iRows(0), m_iCols(0), m_p(NULL) { Create(iRows, iCols); }
		CMtx(const ET *p, int iRows, int iCols) : m_iRows(0), m_iCols(0), m_p(NULL) 
		{ Create(iRows, iCols); Init(p, iRows*iCols); }
		CMtx(const CMtx2x2<ET> &m) : m_iRows(0), m_iCols(0), m_p(NULL) { operator=(m); }
		CMtx(const CMtx3x3<ET> &m) : m_iRows(0), m_iCols(0), m_p(NULL) { operator=(m); }
		CMtx(const CMtx4x4<ET> &m) : m_iRows(0), m_iCols(0), m_p(NULL) { operator=(m); }

		//+-----------------------------------------------------------------------
		//
		//  Member:    ~CMtx()
		//
		//  Synopsis:  destructor
		//
		//  Returns:   none
		//
		//------------------------------------------------------------------------

		virtual ~CMtx()
		{
			if(m_p && !m_bWrap)
				delete m_p;
		}

		int Rows() const { return m_iRows; }
		int Cols() const { return m_iCols; }
		int Size() const { return Rows() * Cols(); }

		const ET *Ptr() const { return m_p; }
		ET *Ptr() { return m_p; }

		void GetCol(int iCol, CVec<ET>& v) const { GetColSlice(iCol, 0, Rows(), v); }

		const ET &El(int iRow, int iCol) const { return m_p[iRow * Cols() + iCol]; }
		ET &El(int iRow, int iCol) { return m_p[iRow * Cols() + iCol]; }
		const ET *operator[] (int i) const { return m_p + i * Cols(); }
		const ET &operator() (int iRow, int iCol) const { return m_p[iRow * Cols() + iCol]; }
		ET *operator[] (int i) { return m_p + i * Cols(); }
		ET &operator() (int iRow, int iCol) { return El(iRow, iCol); }

		CMtx<ET> operator+ () const { return *this; }
		bool operator!= (const CMtx<ET> &m) const { return !(operator==(m)); }

		CMtx<ET> & operator*= (const CMtx<ET> &m) { *this = *this * m; return *this; }

		//+-----------------------------------------------------------------------
		//
		//  Member:    Wrap
		//
		//  Synopsis:  wraps existing memory
		//
		//  Returns:   none
		//
		//------------------------------------------------------------------------

		void Wrap(ET *p, int iRows, int iCols)
		{
			ClearError();
			Free();
			m_p = p;
			m_iRows = iRows;
			m_iCols = iCols;
			m_bWrap = true;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Create
		//
		//  Synopsis:  create a matrix of the specified size
		//
		//  Returns:   S_OK if nothing goes wrong
		//
		//------------------------------------------------------------------------

		HRESULT Create(int iRows, int iCols)
		{
			ClearError();

			if(m_bWrap)
				m_p = NULL;
			m_bWrap = false;

			if(iRows<=0 || iCols<=0)
			{
				Free();
				return NOERROR;
			}

			// Unsigned copies to appease Windows OACR
			unsigned int uRows = (unsigned int)iRows;
			unsigned int uCols = (unsigned int)iCols;

			if(m_p && uRows * uCols==(unsigned int)(Size()))
			{
				m_iRows = iRows;
				m_iCols = iCols;
				return NOERROR;
			}

			if(m_p)
				delete m_p;

			m_p = VT_NOTHROWNEW ET [uRows * uCols];
			if(m_p)
			{
				m_iRows = iRows;
				m_iCols = iCols;
				return NOERROR;
			}

			m_iRows = m_iCols = 0;

			return SetError(E_OUTOFMEMORY);
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Free
		//
		//  Synopsis:  deallocates the memory and sets size to 0x0
		//
		//  Returns:   none
		//
		//------------------------------------------------------------------------

		void Free()
		{
			m_iRows = m_iCols = 0;
			if(m_p && !m_bWrap)
				delete m_p;
			m_bWrap = false;
			m_p = NULL;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator=
		//
		//  Synopsis:  assignment of matrix, creates new one with correct size
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & operator= (const CMtx<ET> &m)
		{
			// if m has out of memory then its size will be zero
			if((m_bWrap && Rows()==m.Rows() && Cols()==m.Cols()) || SUCCEEDED(Create(m.Rows(), m.Cols())))
			{
				SetError(m.GetError()); // pass on the error status

				if(!IsError() && m.m_p && m_p)
					memcpy(m_p, m.m_p, m.Size() * sizeof(ET));
			}

			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator=
		//
		//  Synopsis:  assignment of matrix, creates new one with correct size
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & operator= (const CMtx2x2<ET> &m)
		{
			if((m_bWrap && Rows()==2 && Cols()==2) || SUCCEEDED(Create(2,2)))
				memcpy(m_p, m.Ptr(), 4 * sizeof(ET));

			return *this;   
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator=
		//
		//  Synopsis:  assignment of matrix, creates new one with correct size
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & operator= (const CMtx3x3<ET> &m)
		{
			if((m_bWrap && Rows()==3 && Cols()==3) || SUCCEEDED(Create(3,3)))
				memcpy(m_p, m.Ptr(), 9 * sizeof(ET));

			return *this;   
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator=
		//
		//  Synopsis:  assignment of matrix, creates new one with correct size
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & operator= (const CMtx4x4<ET> &m)
		{
			if((m_bWrap && Rows()==4 && Cols()==4) || SUCCEEDED(Create(4,4)))
				memcpy(m_p, m.Ptr(), 16 * sizeof(ET));

			return *this;   
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Init
		//
		//  Synopsis:  initialize from an array pointer
		//
		//  Returns:   the current CMtx
		//
		//------------------------------------------------------------------------

		CMtx<ET> & Init(const ET *p, int iSize)
		{
			if(p && Ptr() && iSize>0)
			{
				iSize = VtMin(Rows() * Cols(), iSize);
				memcpy(Ptr(), p, sizeof(ET) * iSize); // memcpy handles size = 0 case
			}
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Fill
		//
		//  Synopsis:  sets all entries to a value
		//
		//  Returns:   none
		//
		//------------------------------------------------------------------------

        void Fill(const ET & v)
        {
            for (int i = 0, iEnd = Rows()*Cols(); i < iEnd; i++)
            {
                m_p[i] = v;
            }
        }

		//+-----------------------------------------------------------------------
		//
		//  Member:    GetRow
		//
		//  Synopsis:  makes a vector out of the specified row
		//
		//  Returns:   the new vector
		//
		//------------------------------------------------------------------------

		CVec<ET> GetRow(int i) const
		{
			CVec<ET> v(Cols());
			v.SetError(GetError());

			if(!v.IsError())
			{
				if(i<0 || i>=Rows())
					v = (ET)0;
				else
					memcpy(v.Ptr(), Ptr() + i * Cols(), Cols() * sizeof(ET));
			}

			return v;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    GetCol
		//
		//  Synopsis:  makes a vector out of the specified column
		//
		//  Returns:   the new vector
		//
		//------------------------------------------------------------------------

		CVec<ET> GetCol(int j) const
		{
			CVec<ET> v(Rows());
			v.SetError(GetError());

			if(!v.IsError())
			{
				if(j<0 || j>=Cols())
					v = (ET)0;
				else
				{
					int i;
					for(i=0; i<Rows(); i++)
						v[i] = El(i, j);
				}
			}

			return v;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    GetColSlice
		//
		//  Synopsis:  extracts a slice of the specified column into a vector
		//
		//  Returns:   nothing
		//
		//------------------------------------------------------------------------

		void GetColSlice(int iCol, int iStartRow, int cRows, CVec<ET>& v) const
		{
			if(iCol<0 || iCol>=Cols())
				v = (ET)0;
			else
			{
				cRows = VtMin(cRows, Rows()-iStartRow);
				cRows = VtMin(cRows, v.Size());

				int cCols = Cols();
				const ET* mp = Ptr() + iStartRow*cCols + iCol;
				ET* p = v.Ptr();
				for(int i=0; i<cRows; i++, mp+=cCols)
					p[i] = *mp;
			}
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    GetCols
		//
		//  Synopsis:  get a specified set of columns
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> GetCols(const vector<int> &vCols) const
		{
			CMtx<ET> m(Rows(), (int)vCols.size());
			m.SetError(GetError());
			if(!m.IsError())
			{
				int i,j;
				for(j=0; j<(int)vCols.size(); j++)
					for(i=0; i<Rows(); i++)
						m(i,j) = El(i, vCols[j]);
			}

			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    GetRows
		//
		//  Synopsis:  get a specified set of rows
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> GetRows(const vector<int> &vRows) const
		{
			CMtx<ET> m((int)vRows.size(), Cols());
			m.SetError(GetError());
			if(!m.IsError())
			{
				int i,j;
				for(j=0; j<Cols(); j++)
					for(i=0; i<(int)vRows.size(); i++)
						m(i,j) = El(vRows[i], j);
			}

			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    SetCols
		//
		//  Synopsis:  set a specified set of columns
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & SetCols(const vector<int> &vCols, const CMtx<ET> &m)
		{
			SetError(m.GetError());
			if(!m.IsError())
			{
				int i,j;
				int iCols = VtMin((int)vCols.size(), m.Cols());
				int iRows = VtMin(Rows(), m.Rows());
				for(j=0; j<iCols; j++)
				{
					int k = vCols[j];
					if(k>=0 && k<Cols())
						for(i=0; i<iRows; i++)
							El(i, k) = m(i, j);
				}
			}

			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    SetRows
		//
		//  Synopsis:  set a specified set of rows
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & SetRows(const vector<int> &vRows, const CMtx<ET> &m)
		{
			SetError(m.GetError());
			if(!m.IsError())
			{
				int i,j;
				int iRows = VtMin((int)vRows.size(), m.Rows());
				int iCols = VtMin(Cols(), m.Cols());
				for(i=0; i<iRows; i++)
				{
					int k = vRows[i];
					if(k>=0 && k<Rows())
						for(j=0; j<iCols; j++)
							El(k, j) = m(i, j);
				}
			}

			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Update
		//
		//  Synopsis:  update a sub-matrix starting at row, col
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & Update(int iRow, int iCol, const CMtx<ET> &m)
		{
			SetError(m.GetError());
			if(!IsError())
			{
				int i,j;
				int iRowEnd = iRow + m.Rows();
				int iColEnd = iCol + m.Cols();
				for(i=iRow; i<iRowEnd; i++)
				{
					if(i>=0 && i<Rows())
						for(j=iCol; j<iColEnd; j++)
							if(j>=0 && j<Cols())
								El(i,j) = m(i-iRow,j-iCol);
				}
			}

			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Extract
		//
		//  Synopsis:  extract a sub-matrix from the given location
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> Extract(int iRow, int iCol, int iRowCount, int iColCount) const
		{
			CMtx<ET> m;
			m.SetError(GetError());
			if(m.IsError())
				return m;

			if(iRowCount<0)
				iRowCount = Rows() - iRow;
			if(iColCount<0)
				iColCount = Cols() - iCol;

			if(iRowCount<0 || iColCount<0)
			{
				m.SetError(E_INVALIDARG);
				return m;
			}

			m.Create(iRowCount, iColCount);
			if(!m.IsError())
			{
				int i,j;
				for(i=0; i<iRowCount; i++)
				{
					int u = i + iRow;
					if(u>=0 && u<Rows())
					{
						int v = iCol;
						for(j=0; j<iColCount; j++, v++)
							m(i,j) = (v>=0 && v<Cols()) ? El(u,v) : (ET)0;
					}
					else
					{
						for(j=0; j<iColCount; j++)
							m(i,j) = (ET)0;
					}
				}
			}

			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator=
		//
		//  Synopsis:  assignment from a scalar
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & operator= (const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				Ptr()[i] = f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    MakeI
		//
		//  Synopsis:  construct identity matrix
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & MakeI()
		{
			int i, j;
			for(i=0; i<Rows(); i++)
				for(j=0; j<Cols(); j++)
					El(i,j) = i==j;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    MakeDiag
		//
		//  Synopsis:  construct a diagonal matrix from the given vector
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & MakeDiag(const CVec<ET> &v)
		{
			int iMin = VtMin(Rows(), Cols());
			int i;
			*this = (ET)0;
			for(i=0; i<iMin; i++)
				if(i<v.Size())
					El(i,i) = v[i];
				else
					El(i,i) = 1;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    MakeSymmetric
		//
		//  Synopsis:  make a symmetric matrix by copying the elements from above
		//             the diagonal
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & MakeSymmetric()
		{
			if(Rows()==Cols())
			{
				int i,j;
				for(i=1;i<Rows();i++)
					for(j=0;j<i; j++)
						El(i,j) = El(j,i);
			}
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    T
		//
		//  Synopsis:  construct the matrix transpose
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> T() const
		{
			CMtx<ET> m(Cols(), Rows());
			m.SetError(GetError());

			if(!m.IsError())
			{
				int i, j;
				for(i=0; i<Rows(); i++)
					for(j=0; j<Cols(); j++)
						m.El(j,i) = El(i,j);

			}
			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    H
		//
		//  Synopsis:  construct the matrix conjugate transpose
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> H() const
		{
			CMtx<ET> m(Cols(), Rows());
			m.SetError(GetError());

			if(!m.IsError())
			{
				int i, j;
				for(i=0; i<Rows(); i++)
					for(j=0; j<Cols(); j++)
						m.El(j,i) = VtConj(El(i,j));

			}
			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Conj
		//
		//  Synopsis:  construct the matrix conjugate transpose
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> Conj() const
		{
			CMtx<ET> m(Rows(), Cols());
			m.SetError(GetError());

			if(!m.IsError())
			{
				int i, j;
				for(i=0; i<Rows(); i++)
					for(j=0; j<Cols(); j++)
						m.El(i,j) = VtConj(El(i,j));

			}
			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Tr
		//
		//  Synopsis:  calculate the trace of a matrix
		//
		//  Returns:   the trace
		//
		//------------------------------------------------------------------------

		ET Tr() const
		{
			int i;
			int iMin = VtMin(Rows(), Cols());
			ET fTr = (ET)0;
			for(i=0; i<iMin; i++)
				fTr += El(i,i);
			return fTr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    SetRow
		//
		//  Synopsis:  set a specific row of the matrix to a given vector
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & SetRow(int i, const CVec<ET> &v)
		{
			SetError(v.GetError());
			if(!IsError() && i>=0 && i<Rows())
			{
				int iMin = VtMin(Cols(), v.Size());
				int j;
				for(j=0; j<iMin; j++)
					El(i,j) = v[j];
			}
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    SetCol
		//
		//  Synopsis:  set a specific column of the matrix to a given vector
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & SetCol(int i, const CVec<ET> &v) 
		{
			SetError(v.GetError());

			if(!IsError() && i>=0 && i<Cols())
			{
				int iMin = VtMin(Rows(), v.Size());
				int j;
				for(j=0; j<iMin; j++)
					El(j,i) = v[j];
			}
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    SetColSlice
		//
		//  Synopsis:  set slice of a specific column of the matrix to a given vector
		//
		//  Returns:   nothing
		//
		//------------------------------------------------------------------------

		void SetColSlice(int iCol, int iStartRow, int cRows, const CVec<ET>& v)
		{
			SetError(v.GetError());

			if(!IsError() && iCol>=0 && iCol<Cols())
			{
				cRows = VtMin(cRows, Rows()-iStartRow);
				cRows = VtMin(cRows, v.Size());

				int cCols = Cols();
				ET* mp = Ptr() + iStartRow*cCols + iCol;
				const ET* p = v.Ptr();
				for(int i=0; i<cRows; i++, mp+=cCols)
					*mp = p[i];
			}
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    GetDiag
		//
		//  Synopsis:  extract the diagonal elements of a matrix
		//
		//  Returns:   the new vector
		//
		//------------------------------------------------------------------------

		CVec<ET> GetDiag() const 
		{
			int iMin = VtMin(Rows(), Cols());
			CVec<ET> v(iMin);
			v.SetError(GetError());

			if(!v.IsError())
			{
				int i;
				for(i=0; i<iMin; i++)
					v[i] = El(i,i);
			}
			return v;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator-
		//
		//  Synopsis:  unary minus operator
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> operator- () const
		{
			CMtx<ET> m(Rows(),Cols());
			m.SetError(GetError());

			if(!m.IsError())
			{
				int i;
				for(i=0; i<Size(); i++)
					m.Ptr()[i] = -Ptr()[i];
			}
			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator+
		//
		//  Synopsis:  addition of two matrices
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> operator+ (const CMtx<ET> &m) const
		{
			CMtx<ET> mnew(Rows(),Cols());
			mnew.SetError(GetError());
			mnew.SetError(m.GetError());

			if(!mnew.IsError())
			{
				int i;
				for(i=0; i<Size(); i++)
					mnew.Ptr()[i] = Ptr()[i];
				int iMinR = VtMin(m.Rows(), Rows());
				int iMinC = VtMin(m.Cols(), Cols());
				int j;
				for(i=0; i<iMinR; i++)
					for(j=0; j<iMinC; j++)
						mnew.El(i,j) += m.El(i,j);
			}
			return mnew;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator-
		//
		//  Synopsis:  matrix subtraction
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> operator- (const CMtx<ET> &m) const
		{
			CMtx<ET> mnew(Rows(),Cols());
			mnew.SetError(GetError());
			mnew.SetError(m.GetError());

			if(!mnew.IsError())
			{
				int i;
				for(i=0; i<Size(); i++)
					mnew.Ptr()[i] = Ptr()[i];
				int iMinR = VtMin(m.Rows(), Rows());
				int iMinC = VtMin(m.Cols(), Cols());
				int j;
				for(i=0; i<iMinR; i++)
					for(j=0; j<iMinC; j++)
						mnew.El(i,j) -= m.El(i,j);
			}
			return mnew;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*
		//
		//  Synopsis:  matrix multiplication
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> operator* (const CMtx<ET> &m) const
		{
			CMtx<ET> mnew(Rows(),m.Cols());
			mnew.SetError(GetError());
			mnew.SetError(m.GetError());

			if(!mnew.IsError())
			{
				if(Cols()!=m.Rows())
				{
					mnew = (ET)0;
					mnew.SetError(E_INVALIDARG);
					return mnew;
				}

				int i,j,k;
				for(i=0; i<mnew.Rows(); i++)
					for(j=0; j<mnew.Cols(); j++)
					{
						ET f = (ET)0;
						for(k=0; k<Cols(); k++)
							f += El(i,k) * m.El(k,j);
						mnew.El(i,j) = f;
					}
			}
			return mnew;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*
		//
		//  Synopsis:  mutliply by a scalar
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> operator* (const ET &f) const
		{
			CMtx<ET> m(Rows(),Cols());
			m.SetError(GetError());

			if(!m.IsError())
			{
				int i;
				for(i=0; i<Size(); i++)
					m.Ptr()[i] = Ptr()[i] * f;
			}
			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*
		//
		//  Synopsis:  multiply by a vector (v2 = M * v1)
		//
		//  Returns:   the new vector
		//
		//------------------------------------------------------------------------

		CVec<ET> operator* (const CVec<ET> &v) const
		{
			CVec<ET> vnew(Rows());
			vnew.SetError(GetError());
			vnew.SetError(v.GetError());

			if(!vnew.IsError())
			{
				if(v.Size()!=Cols())
				{
					vnew = (ET)0;
					vnew.SetError(E_INVALIDARG);
					return vnew;
				}
				int i, j;
				for(i=0; i<Rows(); i++)
				{
					ET f = (ET)0;
					for(j=0; j<Cols(); j++)
						f += El(i,j) * v[j];
					vnew[i] = f;
				}
			}
			return vnew;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator/
		//
		//  Synopsis:  divide by a scalar
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> operator/ (const ET &f) const
		{
			CMtx<ET> m(Rows(),Cols());
			m.SetError(GetError());

			if(!m.IsError())
			{
				int i;
				for(i=0; i<Size(); i++)
					m.Ptr()[i] = Ptr()[i] / f;
			}
			return m;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator==
		//
		//  Synopsis:  matrix comparison
		//
		//  Returns:   true or false
		//
		//------------------------------------------------------------------------

		bool operator== (const CMtx<ET> &m) const
		{
			if(IsError() || m.IsError() || m.Rows()!=Rows() || m.Cols()!=Cols())
				return false;
			int i;
			for(i=0; i<Size(); i++)
				if(m.Ptr()[i] != Ptr()[i])
					return false;
			return true;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator+=
		//
		//  Synopsis:  matrix addition
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & operator+= (const CMtx<ET> &m)
		{
			SetError(m.GetError());

			if(!IsError())
			{
				int iMinR = VtMin(m.Rows(), Rows());
				int iMinC = VtMin(m.Cols(), Cols());
				int i,j;
				for(i=0; i<iMinR; i++)
					for(j=0; j<iMinC; j++)
						El(i,j) += m.El(i,j);
			}
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator-=
		//
		//  Synopsis:  matrix subtraction
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & operator-= (const CMtx<ET> &m)
		{
			SetError(m.GetError());

			if(!IsError())
			{  
				int iMinR = VtMin(m.Rows(), Rows());
				int iMinC = VtMin(m.Cols(), Cols());
				int i,j;
				for(i=0; i<iMinR; i++)
					for(j=0; j<iMinC; j++)
						El(i,j) -= m.El(i,j);
			}

			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator*=
		//
		//  Synopsis:  multiplication by a scalar
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & operator*= (const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				Ptr()[i] *= f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    operator/=
		//
		//  Synopsis:  divide by a scalar
		//
		//  Returns:   the current matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> & operator/= (const ET &f)
		{
			int i;
			for(i=0; i<Size(); i++)
				Ptr()[i] /= f;
			return *this;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    DeleteRowCol
		//
		//  Synopsis:  remove one row and one column from the matrix
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> DeleteRowCol(int iRow, int iCol) const
		{
			CMtx<ET> mr;
			mr.SetError(GetError());
			if(mr.IsError())
				return mr;

			if(Rows()<2 || Cols()<2 || iRow<0 || iRow>=Rows() || iCol<0 || iCol>=Cols())
			{
				mr.SetError(E_INVALIDARG);
				return mr;
			}

			HRESULT hr = mr.Create(Rows()-1, Cols()-1);
			if(SUCCEEDED(hr))
			{
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
			}

			return mr;
		}

		//+-----------------------------------------------------------------------
		//
		//  Member:    Inv
		//
		//  Synopsis:  calculate the matrix inverse
		//
		//  Returns:   the new matrix
		//
		//------------------------------------------------------------------------

		CMtx<ET> Inv() const
		{
			CMtx<ET> mr;
			mr.SetError(GetError());

			if(mr.IsError() || Rows()==0 || Cols()==0)
				return mr;

			HRESULT hr = mr.Create(Rows(), Cols());
			if(FAILED(hr))
				return mr;

			if(Rows()!=Cols())
				return mr = (ET)0;

			mr.MakeI();
			CMtx<ET> m = *this;

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
					ExchangeRows(iCol, iPivotRow, m);
					ExchangeRows(iCol, iPivotRow, mr);
				}
				fPivot = m(iCol,iCol);
				ScaleRow(iCol, ((ET)1)/fPivot, m);
				ScaleRow(iCol, ((ET)1)/fPivot, mr);
				for (iRow = 0; iRow < Rows(); iRow++)
					if (iRow != iCol) 
					{
						f = -m(iRow,iCol);
						AddScaled(iCol, f, iRow, m);
						AddScaled(iCol, f, iRow, mr);
					}
			}

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
			if(IsError())
				return;

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
		//  Synopsis:  print out the matrix in matlab format to debug stream
		//
		//  Returns:   none
		//
		//------------------------------------------------------------------------

		void Dump(FILE *fp = stdout) const
		{ 
			if(IsError())
				fprintf(fp, "[ Error ]\n");
			else
				FormatMtxStdio(*this, fp);
		}

		HRESULT LoadMatlab(FILE *fp)
		{
			HRESULT hr = NOERROR;
			// search for leading '['
			int iCols = 0;
			int iRows = 0;
			int c;
			vector<ET> vLoad;
			if(fp==NULL)
				VT_HR_EXIT( E_POINTER );
			while((c = getc(fp))!='[' && c!=EOF);
			if(c==EOF)
				VT_HR_EXIT( E_FAIL );
			double f;

			for (;;)
			{
				int iCount = 0;
#if _MSC_VER
				while((c=fscanf_s(fp, "%lf", &f))==1)
#else
				while((c=fscanf(fp, "%lf", &f))==1)
#endif
				{
					ET fpush = (ET)f;
					VT_HR_EXIT( vLoad.push_back(fpush) );
					iCount++;
				}
				if(c==EOF)
					VT_HR_EXIT( E_FAIL );
				c = getc(fp);
				if(c==EOF)
					VT_HR_EXIT( E_FAIL );
				if(iRows==0)
				{
					if(iCount==0)
						VT_HR_EXIT( E_FAIL );
					iCols = iCount;
				}
				else if(iCols!=iCount)
					VT_HR_EXIT( E_FAIL );
				iRows++;
				if(c==']')
					break;
				if(c!=';')
					VT_HR_EXIT( E_FAIL );
			}

			VT_HR_EXIT( Create(iRows, iCols) );

			int i;
			for(i=0; i<Size(); i++)
				Ptr()[i] = vLoad[i];

Exit:
			return SetError(hr);
		}



	private:
		int m_iRows, m_iCols;
		ET *m_p;
		bool m_bWrap;
	};

	template <class T>
	CMtx< Complex<T> > VtComplexify(const CMtx<T> &mtx)
	{
		CMtx< Complex<T> > out(mtx.Rows(), mtx.Cols());
		out.SetError(mtx.GetError());
		if(!out.IsError())
		{
			int i,j;
			for(i=0; i<mtx.Rows(); i++)
				for(j=0; j<mtx.Cols(); j++)
				{
					out(i,j).Re = mtx(i,j);
					out(i,j).Im = 0;
				}
		}
		return out;
	}

	template <class T>
	CVec< Complex<T> > VtComplexify(const CVec<T> &v)
	{
		CVec< Complex<T> > out(v.Size());
		out.SetError(v.GetError());
		if(!out.IsError())
		{
			int i;
			for(i=0; i<v.Size(); i++)
			{
				out(i).Re = v(i);
				out(i).Im = 0;
			}
		}
		return out;
	}

	template <class T>
	CMtx<T> VtRe(const CMtx< Complex<T> > &mtx)
	{
		CMtx<T> out(mtx.Rows(), mtx.Cols());
		out.SetError(mtx.GetError());
		if(!out.IsError())
		{
			int i,j;
			for(i=0; i<mtx.Rows(); i++)
				for(j=0; j<mtx.Cols(); j++)
				{
					out(i,j) = mtx(i,j).Re;
				}
		}
		return out;
	}

	template <class T>
	CVec<T> VtRe(const CVec< Complex<T> > &v)
	{
		CVec<T> out(v.Size());
		int i;
		for(i=0; i<v.Size(); i++)
			out(i) = v(i).Re;
		return out;
	}

	template <class T>
	CMtx<T> VtIm(const CMtx< Complex<T> > &mtx)
	{
		CMtx<T> out(mtx.Rows(), mtx.Cols());
		out.SetError(mtx.GetError());
		if(!out.IsError())
		{
			int i,j;
			for(i=0; i<mtx.Rows(); i++)
				for(j=0; j<mtx.Cols(); j++)
				{
					out(i,j) = mtx(i,j).Im;
				}
		}
		return out;
	}

	template <class T>
	CVec<T> VtIm(const CVec< Complex<T> > &v)
	{
		CVec<T> out(v.Size());
		int i;
		for(i=0; i<v.Size(); i++)
			out(i) = v(i).Im;
		return out;
	}

	template <class TS, class TD>
	CMtx<TD>& VtConvertMtx(const CMtx<TS>& s, CMtx<TD>& d)
	{
		if(d.Rows()!=s.Rows() || d.Cols()!=s.Cols())
			d.SetError(E_INVALIDARG);
		d.SetError(s.GetError());
		if(!d.IsError())
		{
			int i,j;
			for(i=0; i<s.Rows(); i++)
				for(j=0; j<s.Cols(); j++)
				{
					d(i,j) = (TD)(s(i,j));
				}
		}
		return d;
	}

	template <class TS, class TD>
	CMtx<Complex<TD> >& VtConvertMtx(const CMtx<Complex<TS> >& s, CMtx<Complex<TD> >& d)
	{
		if(d.Rows()!=s.Rows() || d.Cols()!=s.Cols())
			d.SetError(E_INVALIDARG);
		d.SetError(s.GetError());
		if(!d.IsError())
		{
			int i,j;
			for(i=0; i<s.Rows(); i++)
				for(j=0; j<s.Cols(); j++)
				{
					d(i,j).Re = (TD)(s(i,j)).Re;
					d(i,j).Im = (TD)(s(i,j)).Im;
				}
		}
		return d;
	}

	template <class TS, class TD>
	void VtConvertVec(const CVec<TS>& s, CVec<TD>& d)
	{
		if(d.Size()!=s.Size())
			d.SetError(E_INVALIDARG);
		d.SetError(s.GetError());
		if(!d.IsError())
		{
			int i;
			for(i=0; i<s.Size(); i++)
			{
				d(i) = (TD)(s(i));
			}
		}
		return d;
	}

	template <class TS, class TD>
	void VtConvertVec(const CVec<Complex<TS> >& s, CVec<Complex<TD> >& d)
	{
		if(d.Size()!=s.Size())
			d.SetError(E_INVALIDARG);
		d.SetError(s.GetError());
		if(!d.IsError())
		{
			int i;
			for(i=0; i<s.Size(); i++)
			{
				d(i).Re = (TD)(s(i)).Re;
				d(i).Im = (TD)(s(i)).Im;
			}
		}
		return d;
	}

	template <class T> 
	inline CMtx<T> operator*(const T &f, const CMtx<T> &m) { return m*f; }

	template <class T> 
	inline CVec<T> operator*(const T &f, const CVec<T> &v) { return v*f; }

	//+----------------------------------------------------------------------------
	//
	//  Functions:  FormatVec, FormatMtx
	//
	//  Synopsis:  formatting rountines to output vector and matrix
	//
	//-----------------------------------------------------------------------------

	template <class ET>
	void PrintElementStdio(const ET &d, FILE *fp)
	{
		fprintf(fp, "%10.6f ", d);
	}

	template <class ET>
	void PrintElementStdio(const Complex<ET> &d, FILE *fp)
	{
		fprintf(fp, "(%g,%g) ", d.Re, d.Im);
	}

	template <class V>
	void FormatVecStdio(const V& vec, FILE *fp)
	{
		fprintf(fp, "[ ");
		for( int i = 0; i < vec.Size(); i++ )
		{
			PrintElementStdio(vec.El(i), fp);
		}
		fprintf(fp, "]\n");
	}

	template <class V>
	void FormatMtxStdio(const V& mtx, FILE *fp)
	{
		int i, j;
		for(i=0; i<mtx.Rows(); i++)
		{
			if(i==0)
				fprintf(fp, "[ ");
			else
				fprintf(fp, "  ");

			for(j=0; j<mtx.Cols(); j++)
				PrintElementStdio(mtx.El(i,j), fp);

			if(i==mtx.Rows()-1)
				fprintf(fp, " ]\n");
			else
				fprintf(fp, ";\n");
		}
	}

	// multiplies a matrix with itself to form a symmetric matrix
	// mdst = msrc.T() * msrc
	HRESULT VtFastMatrixMul(CMtx<float> &msrc, CMtx<float> &mdst);

};



