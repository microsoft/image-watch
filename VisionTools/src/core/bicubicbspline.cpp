//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for image resampling
//
//  History:
//      2010/10/05-mafinch
//          Created
//
//------------------------------------------------------------------------
#include "stdafx.h"

#include "vt_bicubicbspline.h"

#include "vt_pad.h"
#include "vt_pixeliterators.h"
#include "vt_convert.h"

#if (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64))
#define MAF_ALLOW_SSE
#endif

using namespace vt;

// 6 * lower off-diagonal of L.
static const float s_arInverseUDiag[] = {
    1.2f, 1.5789474f, 1.6056338f, 1.6075472f, 1.6076845f, 1.6076944f,
    1.6076951f, 
	1.6076952f, // limit value repeats until penultimate element.
    // potentially pad with repeats so length is a multiple of 8 
    // but last element is different from the limit
    1.2679492f
};
static const int iUpperDiag = sizeof(s_arInverseUDiag)/
	sizeof(s_arInverseUDiag[0]);

// upper off-diagonal of U, constant along the diagonal
static const float s_UUpper = 0.16666667f; 

// lower off-diagonal of L 
static const float s_arLLower[] = {
    0.f, 0.2f, 0.26315789f, 0.26760563f, 0.26792453f, 0.26794742f,
    0.26794907f, 0.26794918f, 0.26794919f, /*, ... */
    /* potentially pad with repeats so length is a multiple of 8 */
    0.26794919f, 0.26794919f, 0.26794919f, 0.26794919f, 
    0.26794919f, 0.26794919f, 0.26794919f
};
static const int s_iLowerDiag = sizeof(s_arLLower)/sizeof(s_arLLower[0]);

static const int CUBIC_BSPLINE_PROLOGUE = 11;
static const int CUBIC_BSPLINE_EPILOGUE = CUBIC_BSPLINE_PROLOGUE;

// For each row, solve left to right then back right to left.
void PreprocessRows(
                    OUT CFloatImg& dstImg,
                    IN const float* pSrc,
                    IN const int iSrcStride)
{
    const int iBands = dstImg.Bands();
    const int iHeight = dstImg.Height();
    const int iWidth = dstImg.Width();

    float* const pDst = dstImg.Ptr();
    const int iDstStride = dstImg.StrideBytes() / sizeof(float);

    VT_ASSERT(iWidth * iBands <= iSrcStride);
    VT_ASSERT(iWidth > s_iLowerDiag);

    for(int row = 0; row < iHeight; ++row)
    {
        const int iSrcRowBase = row * iSrcStride;
        const int iDstRowBase = row * iDstStride;

	    // First element is copied from input.
        for(int k = 0; k < iBands; ++k)
        {
	        pDst[iDstRowBase + k] = pSrc[iSrcRowBase + k];
        }

	    // Iterate through all elements of packed version of lower diagonal 
	    // off-diagonal elements.
        for (int i = 1; i < s_iLowerDiag; i++)
	    {
		    // Note we use pSrc for this, but (modified) pDst for previous value.
		    // y[i] = b[i] - L[i,i-1] * y[i-1]
            for(int k = 0; k < iBands; ++k)
            {
                const int iDst = iDstRowBase + i * iBands + k;
                const int iSrc = iSrcRowBase + i * iBands + k;
                const int iDstBack = iDst - iBands;
                pDst[iDst] = pSrc[iSrc] - s_arLLower[i] * pDst[iDstBack];
            }
	    }

	    // The rest of lower diagonal off-diagonals are same as last in array.
        const float fLastLower = s_arLLower[s_iLowerDiag - 1];

	    // Continue through the rest of the row using constant L[i] value
        for (int i = s_iLowerDiag; i < iWidth; i++)
	    {
		    // y[i] = b[i] - L[i,i-1] * y[i-1]
            for(int k = 0; k < iBands; ++k)
            {
                const int iDst = iDstRowBase + i * iBands + k;
                const int iSrc = iSrcRowBase + i * iBands + k;
                const int iDstBack = iDst - iBands;
                pDst[iDst] = pSrc[iSrc] - fLastLower * pDst[iDstBack];
            }
	    }
    

	    // Finished with pSrc now, all of its effect is in dst.

        // second pass: solve U x = y (y solution of first pass)

        const int iRowLast = iDstRowBase + (iWidth - 1) * iBands;
        // x[n-1] = y[n-1] / U[n-1, n-1] 
        for(int k = 0; k < iBands; ++k)
        {
            pDst[iRowLast + k] *= s_arInverseUDiag[iUpperDiag - 1];
        }

        const float fInverseLimitDiag = s_arInverseUDiag[iUpperDiag - 2];

        VT_ASSERT(iWidth >= iUpperDiag - 2);

        // Finish off end row, assuming these are all in the constant limit part of the matrix.
        for (int i = iWidth - 2; i >= iUpperDiag - 2; i--)
	    {
		    // In this range, U[i, i-1] is constant
		    // x[i] = 1/U[i,i] * (y[i] - x[i-1] * U[i, i-1])
            for(int k = 0; k < iBands; ++k)
            {
                const int iDst = iDstRowBase + i * iBands + k;
                const int iDstPlus = iDst + iBands;
                pDst[iDst] 
                    = fInverseLimitDiag 
                        * (pDst[iDst] 
                            - pDst[iDstPlus] * s_UUpper);
            }
	    }

        for (int i = iUpperDiag - 3; i >= 0; i--)
	    {
		    // x[i] = 1/U[i,i] * (y[i] - x[i-1] * U[i, i-1])
            for(int k = 0; k < iBands; ++k)
            {
                const int iDst = iDstRowBase + i * iBands + k;
                const int iDstPlus = iDst + iBands;
                pDst[iDst] = s_arInverseUDiag[i] * (pDst[iDst]- pDst[iDstPlus] * s_UUpper);
            }
	    }
    }
	
}
// For each column, solve top to bottom then back up bottom to top.
// Notice this could be made more cache/parallelization friendly in the general case by 
// processing in smaller chunks of columns. But we are assuming here that the
// framework has already chunked up dstImg.
// Note also that all of srcImg's influence has already gone into dstImg in PreprocessRows above.
void PreprocessColumns(IN OUT CFloatImg& dstImg)
{
    const int iBands = dstImg.Bands();
    const int iHeight = dstImg.Height();
    const int iWidth = dstImg.Width();

    float* const pDst = dstImg.Ptr();
    const int iDstStride = dstImg.StrideBytes() / sizeof(float);

    for(int column = 0; column < iWidth; ++column)
    {
        const int iDstColBase = column * iBands;

        // pSrc == pDst, no need to copy here.

	    // Iterate through all elements of packed version of lower diagonal 
	    // off-diagonal elements.
        for (int j = 1; j < s_iLowerDiag; j++)
	    {
		    // Note we use pSrc for this, but (modified) pDst for previous value.
		    // y[i] = b[i] - L[i,i-1] * y[i-1]
            const int iDst = iDstColBase + j * iDstStride;
            const int iDstBack = iDst - iDstStride;
            for(int k = 0; k < iBands; ++k)
            {
                pDst[iDst + k] -= s_arLLower[j] * pDst[iDstBack + k];
            }
	    }

	    // The rest of lower diagonal off-diagonals are same as last in array.
        const float fLastLower = s_arLLower[s_iLowerDiag - 1];

	    // Continue through the rest of the row using constant L[i] value
        for (int j = s_iLowerDiag; j < iHeight; j++)
	    {
		    // y[i] = b[i] - L[i,i-1] * y[i-1]
            const int iDst = iDstColBase + j * iDstStride;
            const int iDstBack = iDst - iDstStride;
            for(int k = 0; k < iBands; ++k)
            {
                pDst[iDst + k] -= fLastLower * pDst[iDstBack + k];
            }
	    }
    

        // second pass: solve U x = y (y solution of first pass)

        const int iColLast = iDstColBase + (iHeight - 1) * iDstStride;
        // x[n-1] = y[n-1] / U[n-1, n-1] 
        for(int k = 0; k < iBands; ++k)
        {
            pDst[iColLast + k] *= s_arInverseUDiag[iUpperDiag - 1];
        }

        const float fInverseLimitDiag = s_arInverseUDiag[iUpperDiag - 2];

        VT_ASSERT(iHeight >= iUpperDiag - 2);

        // Finish off end row, assuming these are all in the constant limit part of the matrix.
        for (int j = iHeight - 2; j >= iUpperDiag - 2; j--)
	    {
		    // In this range, U[i, i-1] is constant
		    // x[i] = 1/U[i,i] * (y[i] - x[i-1] * U[i, i+1])
            const int iDst = iDstColBase + j * iDstStride;
            const int iDstPlus = iDst + iDstStride;
            for(int k = 0; k < iBands; ++k)
            {
                pDst[iDst + k] = fInverseLimitDiag * (pDst[iDst + k] - pDst[iDstPlus + k] * s_UUpper);
            }
	    }

        for (int j = iUpperDiag - 3; j >= 0; j--)
	    {
		    // x[i] = 1/U[i,i] * (y[i] - x[i-1] * U[i, i+1])
            const int iDst = iDstColBase + j * iDstStride;
            const int iDstPlus = iDst + iDstStride;
            for(int k = 0; k < iBands; ++k)
            {
                pDst[iDst + k] = s_arInverseUDiag[j] * (pDst[iDst + k] - pDst[iDstPlus + k] * s_UUpper);
            }
	    }
    }
	
}

////////////////////////////////////////////////////////////////////////////////////////
// End Scalar version
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// Begin FVec (SSE) version
////////////////////////////////////////////////////////////////////////////////////////

#ifdef MAF_ALLOW_SSE

// For each row, solve left to right then back right to left.
void PreprocessRowsFVec4(
                    OUT CFloatImg& dstImg,
                    IN const float* pSrc,
                    IN const int iSrcStride)
{
    const int iHeight = dstImg.Height();
    const int iWidth = dstImg.Width();

    float* const pDst = dstImg.Ptr();
    const int iDstStride = dstImg.StrideBytes() / sizeof(float);

    VT_ASSERT(dstImg.Bands() == 4);
    VT_ASSERT(iWidth * 4 <= iSrcStride);
    VT_ASSERT(iWidth > s_iLowerDiag);

    for(int row = 0; row < iHeight; ++row)
    {
        const int iSrcRowBase = row * iSrcStride;
        const int iDstRowBase = row * iDstStride;

	    // First 4-element is copied from input.
        pDst[iDstRowBase + 0] = pSrc[iSrcRowBase + 0];
        pDst[iDstRowBase + 1] = pSrc[iSrcRowBase + 1];
        pDst[iDstRowBase + 2] = pSrc[iSrcRowBase + 2];
        pDst[iDstRowBase + 3] = pSrc[iSrcRowBase + 3];

	    // Iterate through all elements of packed version of lower diagonal 
	    // off-diagonal elements.
        for (int i = 1; i < s_iLowerDiag; i++)
	    {
		    // Note we use pSrc for this, but (modified) pDst for previous value.
		    // y[i] = b[i] - L[i,i-1] * y[i-1]
            const int iSrc = iSrcRowBase + i * 4;
            const simdReg src(pSrc + iSrc);
            const int iDst = iDstRowBase + i * 4;
            const int iDstBack = iDst - 4;
            const simdReg dstBack(pDst + iDstBack);
            simdReg::Unpack(pDst + iDst, src - simdReg(s_arLLower[i]) * dstBack);
	    }

	    // The rest of lower diagonal off-diagonals are same as last in array.
        const simdReg fvLastLower(s_arLLower[s_iLowerDiag - 1]);

	    // Continue through the rest of the row using constant L[i] value
        for (int i = s_iLowerDiag; i < iWidth; i++)
	    {
		    // y[i] = b[i] - L[i,i-1] * y[i-1]
            const int iSrc = iSrcRowBase + i * 4;
            const simdReg src(pSrc + iSrc);
            const int iDst = iDstRowBase + i * 4;
            const int iDstBack = iDst - 4;
            const simdReg dstBack(pDst + iDstBack);
            simdReg::Unpack(pDst + iDst, src - fvLastLower * dstBack);
	    }
    

	    // Finished with pSrc now, all of its effect is in pDst.

        // second pass: solve U x = y (y solution of first pass)

        const int iRowLast = iDstRowBase + (iWidth - 1) * 4; 
        // x[n-1] = y[n-1] / U[n-1, n-1] 
        simdReg::Unpack(pDst + iRowLast, simdReg(pDst + iRowLast) * simdReg(s_arInverseUDiag[iUpperDiag - 1]));

        const simdReg fvInverseLimitDiag(s_arInverseUDiag[iUpperDiag - 2]);
        const simdReg fv_UUpper(s_UUpper);

        VT_ASSERT(iWidth >= iUpperDiag - 2);

        // Finish off end row, assuming these are all in the constant limit part of the matrix.
        for (int i = iWidth - 2; i >= iUpperDiag - 2; i--)
	    {
		    // In this range, U[i, i-1] is constant
		    // x[i] = 1/U[i,i] * (x[i] - x[i-1] * U[i, i-1])
            const int iDst = iDstRowBase + i * 4;
            const int iDstPlus = iDst + 4;
            simdReg::Unpack(pDst + iDst,
                fvInverseLimitDiag 
                    * (simdReg(pDst + iDst) - simdReg(pDst + iDstPlus) * fv_UUpper));
	    }

        for (int i = iUpperDiag - 3; i >= 0; i--)
	    {
            const simdReg fv_InverseUDiag(s_arInverseUDiag[i]);

		    // x[i] = 1/U[i,i] * (x[i] - x[i-1] * U[i, i-1])
            const int iDst = iDstRowBase + i * 4;
            const int iDstPlus = iDst + 4;
            simdReg::Unpack(pDst + iDst,
                fv_InverseUDiag
                    * (simdReg(pDst + iDst) - simdReg(pDst + iDstPlus) * fv_UUpper));
	    }
    }
	
}

// For each column, solve top to bottom then back up bottom to top.
// Notice this could be made more cache/parallelization friendly in the general case by 
// processing in smaller chunks of columns. But we are assuming here that the
// framework has already chunked up dstImg.
// Note also that all of srcImg's influence has already gone into dstImg in PreprocessRowsFVec4
// above.
void PreprocessColumnsFVec4( IN OUT CFloatImg& dstImg )
{
    const int iWidth = dstImg.Width();
    const int iHeight = dstImg.Height();
    
    float* const pDst = dstImg.Ptr();
    const int iDstStride = dstImg.StrideBytes() / sizeof(float);

    VT_ASSERT(dstImg.Bands() == 4);
    VT_ASSERT(iHeight > s_iLowerDiag);

    for(int column = 0; column < iWidth; ++column)
    {
        const int iDstColBase = column * 4;

        // pSrc == pDst, so no need to copy forward to initialize first element.

	    // Iterate through all elements of packed version of lower diagonal 
	    // off-diagonal elements.
        for (int j = 1; j < s_iLowerDiag; j++)
	    {
		    // Note we use pSrc for this, but (modified) pDst for previous value.
		    // y[i] = b[i] - L[i,i-1] * y[i-1]
            const simdReg fv_LLower(s_arLLower[j]);
            const int iDst = iDstColBase + j * iDstStride;
            const int iDstBack = iDst - iDstStride;
            simdReg::Unpack(pDst + iDst, simdReg(pDst + iDst) - fv_LLower * simdReg(pDst + iDstBack));
	    }

	    // The rest of lower diagonal off-diagonals are same as last in array.
        const simdReg fvLastLower(s_arLLower[s_iLowerDiag - 1]);

	    // Continue through the rest of the row using constant L[i] value
        for (int j = s_iLowerDiag; j < iHeight; j++)
	    {
		    // y[i] = b[i] - L[i,i-1] * y[i-1]
            const int iDst = iDstColBase + j * iDstStride;
            const int iDstBack = iDst - iDstStride;
            simdReg::Unpack(pDst + iDst, simdReg(pDst + iDst) - fvLastLower * simdReg(pDst + iDstBack));
	    }
    
        // second pass: solve U x = y (y solution of first pass)

        const simdReg fv_LastInverseUDiag(s_arInverseUDiag[iUpperDiag - 1]);

        const int iColLast = iDstColBase + (iHeight - 1) * iDstStride;

        // x[n-1] = y[n-1] / U[n-1, n-1] 
        simdReg::Unpack(pDst + iColLast, simdReg(pDst + iColLast) * fv_LastInverseUDiag);

        const simdReg fvInverseLimitDiag(s_arInverseUDiag[iUpperDiag - 2]);

        VT_ASSERT(iHeight >= iUpperDiag - 2);

        const simdReg fv_UUpper(s_UUpper);

        // Finish off end row, assuming these are all in the constant limit part of the matrix.
        for (int j = iHeight - 2; j >= iUpperDiag - 2; j--)
	    {
		    // In this range, U[i, i-1] is constant
		    // x[i] = 1/U[i,i] * (y[i] - x[i-1] * U[i, i+1])
            const int iDst = iDstColBase + j * iDstStride;
            const int iDstPlus = iDst + iDstStride;
            simdReg::Unpack(pDst + iDst, 
                fvInverseLimitDiag 
                    * (simdReg(pDst + iDst) - simdReg(pDst + iDstPlus) * fv_UUpper));
	    }

        for (int j = iUpperDiag - 3; j >= 0; j--)
	    {
		    // x[i] = 1/U[i,i] * (y[i] - x[i-1] * U[i, i+1])
            const simdReg fv_InverseUDiag(s_arInverseUDiag[j]);
            const int iDst = iDstColBase + j * iDstStride;
            const int iDstPlus = iDst + iDstStride;
            simdReg::Unpack(pDst + iDst, 
                fv_InverseUDiag 
                    * (simdReg(pDst + iDst) - simdReg(pDst + iDstPlus) * fv_UUpper));
	    }
    }
	
}


#endif // MAF_ALLOW_SSE

static void PreprocessScalar(
                OUT CFloatImg& dstImg,
                IN const float* pSrc,
                IN const int iSrcStride)
{
    // Forward and back substition right and left across all rows independently.
    PreprocessRows(dstImg, pSrc, iSrcStride);

    // Forward and back substitution up and down across all columns independently.
    PreprocessColumns(dstImg);
}

#ifdef MAF_ALLOW_SSE
static void PreprocessFVec(
                OUT CFloatImg& dstImg,
                IN const float* pSrc,
                IN const int iSrcStride)
{
    // Forward and back substition right and left across all rows independently.
    PreprocessRowsFVec4(dstImg, pSrc, iSrcStride);

    // Forward and back substitution up and down across all columns independently.
    PreprocessColumnsFVec4(dstImg);
}

#endif // MAF_ALLOW_SSE

/// Note that iSrcStride is stride in floats, not bytes.
static void PreprocessSelect(
                OUT CFloatImg& dstImg,
                IN const float* pSrc,
                IN const int iSrcStride)
{
    // If we have a 4 channel system, pass it through the FVec (SSE) route,
    // otherwise handle as scalar.
    // Note that we assume dstImg is 16 byte aligned, because we just
    // allocated it ourselves.
    VT_ASSERT(!(INT_PTR(dstImg.BytePtr()) & 0xf));

#ifdef MAF_ALLOW_SSE
    const bool fourBands = dstImg.Bands() == 4;
    const bool aligned16Byte = !(intptr_t(pSrc) & 0xf) && !(iSrcStride & 0x3);
    const bool canSSE = fourBands && aligned16Byte && g_SupportSSE2();
    if(canSSE)
    {
        PreprocessFVec(
            dstImg,
            pSrc,
            iSrcStride);
    }
    else
#endif // MAF_ALLOW_SSE
    {
        PreprocessScalar(
            dstImg,
            pSrc,
            iSrcStride);
    }
}

static HRESULT MakePaddedScratch(CFloatImg& padded, const CFloatImg& original, int padding)
{
    return padded.Create(
            original.Width() + 2 * padding, 
            original.Height() + 2 * padding, 
            original.Bands());
}

static HRESULT UnPadScratch(CFloatImg& dst, const CFloatImg& padded, int padding)
{
        const CRect srcRect(padding, padding, padding + dst.Width(), padding + dst.Height());
        return dst.Paste(0, 0, padded, &srcRect);
}
          
static HRESULT PreprocessBicubicBSpline(CFloatImg& imgDst, const CRect& rctDst, 
										const CFloatImg& imgSrc, const CPoint& srcTopLeft)
{
    VT_HR_BEGIN();

    // All of these issues should have been handled upstream. In this internal function,
    // we will assume (and assert) that the respective images are okay.
    VT_ASSERT(EL_FORMAT(imgDst.GetType()) == EL_FORMAT_FLOAT); 
    VT_ASSERT(EL_FORMAT(imgSrc.GetType()) == EL_FORMAT_FLOAT);
    VT_ASSERT(imgSrc.Bands() == imgDst.Bands()); 

    // Want a pointer into imgSrc corresponding to the new padded destination image tile's origin.
    const float* pSrcImage = imgSrc.Ptr(
                    rctDst.left - srcTopLeft.x - CUBIC_BSPLINE_PROLOGUE, 
                    rctDst.top - srcTopLeft.y - CUBIC_BSPLINE_PROLOGUE);

    VT_PTR_EXIT(pSrcImage);

    CFloatImg paddedDst;
    VT_HR_EXIT( MakePaddedScratch(paddedDst, imgDst, CUBIC_BSPLINE_PROLOGUE) );

    const int iSrcFloatStride = imgSrc.StrideBytes() >> 2;

    // This doesn't return an error code to check, because nothing can possibly go worng.
    PreprocessSelect(paddedDst, pSrcImage, iSrcFloatStride);

    VT_HR_EXIT(UnPadScratch(imgDst, paddedDst, CUBIC_BSPLINE_PROLOGUE));

    VT_HR_END();
}

HRESULT VtPreprocessBicubicBSpline(CFloatImg& imgDst, const CRect& rctDstIn, 
                                const CFloatImg& imgSrc, const IMAGE_EXTEND& ex)
{
    VT_HR_BEGIN();

    const UInt32 c_blocksize = 200;

    for( CBlockIterator bi(rctDstIn, c_blocksize); !bi.Done(); bi.Advance() )
    {
        // get blk rct in dst image coords
        const CRect rctDst = bi.GetRect();

        const CRect srcPaddedRect(rctDst.left - CUBIC_BSPLINE_PROLOGUE,
                            rctDst.top - CUBIC_BSPLINE_PROLOGUE,
                            rctDst.right + CUBIC_BSPLINE_EPILOGUE,
                            rctDst.bottom + CUBIC_BSPLINE_EPILOGUE);

        const int iBands = imgSrc.Bands();
        const int iType = imgSrc.GetType();
        const bool needPadding = (srcPaddedRect.left < 0)
                                ||(srcPaddedRect.top < 0)
                                ||(srcPaddedRect.right > imgSrc.Width())
                                ||(srcPaddedRect.bottom > imgSrc.Height());

        const bool needCopy = (imgDst.Bands() != iBands)
                            || (imgDst.GetType() != iType)
                            || needPadding;
    
        CFloatImg dstImgTile;
        VT_HR_EXIT( imgDst.Share(dstImgTile, &rctDst) );

        CPoint srcTopLeft;
        CFloatImg srcImgTile;

        // Check that we have sufficient padding.
        if(needCopy)
        {
            // hanging over the edge, need to create an extended copy.
            VT_HR_EXIT( srcImgTile.Create(srcPaddedRect.Width(), srcPaddedRect.Height(),
                                        iBands, 
                                        align64ByteRows) );
            
            VT_HR_EXIT( VtCropPadImage(srcImgTile, srcPaddedRect, imgSrc, ex) );

            srcTopLeft = CPoint(srcPaddedRect.left, srcPaddedRect.top);
        }
        else
        {
            // Can just share.
            VT_HR_EXIT( imgSrc.Share(srcImgTile, &srcPaddedRect) );

            srcTopLeft = CPoint(srcPaddedRect.left, srcPaddedRect.top);
        }

        VT_HR_EXIT( PreprocessBicubicBSpline(dstImgTile, rctDst, srcImgTile, srcTopLeft) );
    }

    VT_HR_END();
}

HRESULT vt::VtPreprocessBicubicBSpline(CFloatImg& imgDst, const CFloatImg& imgSrc, const IMAGE_EXTEND& ex)
{
    const CRect dstRect(0, 0, imgDst.Width(), imgDst.Height());

    return VtPreprocessBicubicBSpline(imgDst, dstRect, imgSrc, ex);
}

#ifndef VT_NO_XFORMS

HRESULT CPreprocessBicubicBSpline::Transform(OUT CImg* pDstImg, 
											 IN  const CRect& rctDst,
											 IN const CImg& srcImg,
											 IN const CPoint& srcTopLeft)
{
    VT_HR_BEGIN();

    VT_ASSERT(pDstImg != NULL);
    ANALYZE_ASSUME(pDstImg); // to placate compiler warning.

    // Not sure why we are getting an empty rect here, but if we get one we'll assume
    // there's no work to do.
    if(!rctDst.IsRectEmpty())
    {
        VT_ASSERT(pDstImg->GetType() == EL_FORMAT_FLOAT);
        VT_ASSERT(srcImg.GetType() == EL_FORMAT_FLOAT);

        CFloatImg& dstFloatImg = reinterpret_cast<CFloatImg&>(*pDstImg);

        const CFloatImg& srcFloatImg = reinterpret_cast<const CFloatImg&>(srcImg);

        VT_HR_EXIT( PreprocessBicubicBSpline(dstFloatImg, rctDst, srcFloatImg, srcTopLeft) );
    }

    VT_HR_END();
}

CRect CPreprocessBicubicBSpline::GetRequiredSrcRect(IN  const CRect& rctDst)
{
    return vt::CRect(
        rctDst.left - CUBIC_BSPLINE_PROLOGUE,
        rctDst.top - CUBIC_BSPLINE_PROLOGUE,
        rctDst.right + CUBIC_BSPLINE_EPILOGUE,
        rctDst.bottom + CUBIC_BSPLINE_EPILOGUE);
}

CRect CPreprocessBicubicBSpline::GetAffectedDstRect(IN  const CRect& rctSrc)
{
	return vt::CRect(
        rctSrc.left - CUBIC_BSPLINE_EPILOGUE,
        rctSrc.top - CUBIC_BSPLINE_EPILOGUE,
        rctSrc.right + CUBIC_BSPLINE_PROLOGUE,
        rctSrc.bottom + CUBIC_BSPLINE_PROLOGUE);
}

void CPreprocessBicubicBSpline::GetDstPixFormat(OUT int& frmtDst,
												IN  const int* pfrmtSrcs, 
												IN  UINT  /*uSrcCnt*/)
{ 
    frmtDst =  ((*pfrmtSrcs) & ~VT_IMG_ELFRMT_MASK) | EL_FORMAT_FLOAT;
}

#endif // VT_NO_XFORMS
