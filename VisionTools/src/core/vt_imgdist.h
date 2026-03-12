//+-----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Routines for computing distance between images
//
//  History:
//      2004/11/08-mattu
//			Created
//
//------------------------------------------------------------------------------

#pragma once

#include "vtcommon.h"
#include "vt_readerwriter.h"

namespace vt {

//+-----------------------------------------------------------------------------
//
// Compute the sum of absolute differences (SAD) between two images or spans
//
//     Currently the following types are implemented:
//
//     Byte, UInt16, float
//
//------------------------------------------------------------------------------

HRESULT VtSADImages(const CImg& imgA, const CImg& imgB, double& dist);

template <class T, class TResult>
TResult VtSADSpan(const T* pA, const T* pB, int iSpan);

template <class T, class TResult>
TResult VtSADSpan(const RGBAType<T>* pA, const RGBAType<T>* pB, int iSpan);

//+-----------------------------------------------------------------------------
//
// Compute the sum of an image or span
//
//     Currently the following types are implemented:
//
//     Byte, UInt16, float
//
//------------------------------------------------------------------------------

HRESULT VtSumImage(const CImg& img, double& sum);

template <class T, class TResult>
TResult VtSumSpan(const T* pP, int iSpan);

template <class T, class TResult>
TResult VtSumSpan(const RGBAType<T>* pP, int iSpan);

//+-----------------------------------------------------------------------------
//
// Function: VtCompareImages
//
// Synopsis: Compare two images pixel by pixel and return information about
//           if they match or not.  
// 
//------------------------------------------------------------------------------
bool
VtCompareImages(IImageReader* pImg1, IImageReader* pImg2,
				int iMaxMessageCount = 1,
				vt::vector<vt::wstring>* pvecMessages = NULL,
				float fMaxAllowedDiff = 0.f,
				const vt::CRect* pRect = NULL);


bool
inline VtCompareImages(CImg& img1, IImageReader* pImg2,
		   		       int iMaxMessageCount = 1,
				       vt::vector<vt::wstring>* pvecMessages = NULL,
				       float fMaxAllowedDiff = 0.f,
				       const vt::CRect* pRect = NULL)
{
	CImgReaderWriter<CImg> rw1;
	img1.Share(rw1);
	return VtCompareImages(&rw1, pImg2, iMaxMessageCount, pvecMessages,
						   fMaxAllowedDiff, pRect);
}

bool
inline VtCompareImages(IImageReader* pImg1, CImg& img2,
		   		       int iMaxMessageCount = 1,
				       vt::vector<vt::wstring>* pvecMessages = NULL,
				       float fMaxAllowedDiff = 0.f,
				       const vt::CRect* pRect = NULL)
{
	CImgReaderWriter<CImg> rw2;
	img2.Share(rw2);
	return VtCompareImages(pImg1, &rw2, iMaxMessageCount, pvecMessages,
						   fMaxAllowedDiff, pRect);
}

bool
inline VtCompareImages(CImg& img1, CImg& img2,
		   		       int iMaxMessageCount = 1,
				       vt::vector<vt::wstring>* pvecMessages = NULL,
				       float fMaxAllowedDiff = 0.f,
				       const vt::CRect* pRect = NULL)
{
	CImgReaderWriter<CImg> rw1;
	img1.Share(rw1);

    CImgReaderWriter<CImg> rw2;
	img2.Share(rw2);

	return VtCompareImages(&rw1, &rw2, iMaxMessageCount, pvecMessages,
						   fMaxAllowedDiff, pRect);
}

};
