//+----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-----------------------------------------------------------------------------
#pragma once

#include "vt_mathutils.h"

namespace vt {

// determines the mean and covariance of a data set
// where the covariance is given by cov_rc = (1/N) * sum_i (x_ir - mean_r) * (x_ic - mean_c)
// dim is the dimensionality of the data, count is the number of data vectors (size of data set)
// sample is the number of vectors to randomly sample. if this is = 0 or = count, then all the data is used
// this is to allow faster evaluation of approximate statistics over very large data sets
template <class T>
HRESULT VtMeanAndCovariance(T **pdata, int iDim, int iCount, int iSample, CVec<T> &vMeanRtn, CMtx<T> &mCovRtn)
{
	HRESULT hr = NOERROR;

	CMtxd mVarSum;
	CRand rnd;

	if(iDim<1 || iCount<1)
		VT_HR_EXIT( E_INVALIDARG );
	if(pdata==NULL)
		VT_HR_EXIT( E_POINTER );

	VT_HR_EXIT( vMeanRtn.Create(iDim) );
	VT_HR_EXIT( mVarSum.Create(iDim, iDim) );
	VT_HR_EXIT( mCovRtn.Create(iDim, iDim) );
	vMeanRtn = (T)0;
	mVarSum = 0.0;

	if(iSample<1 || iSample>iCount)
		iSample = iCount;

	rnd.Seed(93824); // always the same

	int i, j, k;
	for(i=0; i<iSample; i++)
	{
		int ind;
		if(iSample!=iCount)
			ind = rnd.IRand(iCount); // pick a random subset
		else
			ind = i;
		for(j=0; j<iDim; j++)
		{
			T f = pdata[ind][j];
			vMeanRtn[j] += f;

			for(k=j; k<iDim; k++)
				mVarSum(j,k) += (double)f * (double)pdata[ind][k];
		}
	}
	vMeanRtn *= 1/(float)iSample;
	for(j=0; j<iDim; j++)
		for(k=j; k<iDim; k++)
			mCovRtn(j,k) = (T)(mVarSum(j,k)/iSample - (double)vMeanRtn[j] * (double)vMeanRtn[k]);
	mCovRtn.MakeSymmetric();

Exit:
	return hr;
}

// alternate version of the same function. you must provide a callback function to sample the data into the vector
// that is provided. the index number indicates to you which element of the data vector the function wants.
// tif the sample function returns false then this data vector is ignored.

template <class T>
HRESULT VtMeanAndCovariance(bool (*pCallback)(int, CVec<T> &, void *), void *pUser, 
	int iDim, int iCount, int iSample, CVec<T> &vMeanRtn, CMtx<T> &mCovRtn)
{
	VT_HR_BEGIN();

	CMtxd mVarSum;
	CVec<T> vData;
	CRand rnd;

	if(iDim<1 || iCount<1)
		VT_HR_EXIT( E_INVALIDARG );

	VT_HR_EXIT( vMeanRtn.Create(iDim) );
	VT_HR_EXIT( mVarSum.Create(iDim, iDim) );
	VT_HR_EXIT( mCovRtn.Create(iDim, iDim) );
	VT_HR_EXIT( vData.Create(iDim) );
	vMeanRtn = (T)0;
	mVarSum = 0.0;

	if(iSample<1 || iSample>iCount)
		iSample = iCount;

	rnd.Seed(93824); // always the same

	int i, j, k;
	int iRealSample = 0;
	for(i=0; i<iSample; i++)
	{
		int ind;
		if(iSample!=iCount)
			ind = rnd.IRand(iCount); // pick a random subset
		else
			ind = i;
		if(!(*pCallback)(ind, vData, pUser))
			continue;
		iRealSample++;
		for(j=0; j<iDim; j++)
		{
			T f = vData[j];
			vMeanRtn[j] += f;

			for(k=j; k<iDim; k++)
				mVarSum(j,k) += (double)f * (double)vData[k];
		}
	}
	vMeanRtn *= 1/(float)iRealSample;
	for(j=0; j<iDim; j++)
		for(k=j; k<iDim; k++)
			mCovRtn(j,k) = (T)(mVarSum(j,k)/iRealSample - (double)vMeanRtn[j] * (double)vMeanRtn[k]);
	mCovRtn.MakeSymmetric();

	VT_HR_END();
}

};