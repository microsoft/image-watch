//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      SSE routines for image warping
//
//  History:
//      2010/09/27-mattu
//          Created
//
//------------------------------------------------------------------------
#pragma once

//+-----------------------------------------------------------------------
// bilinear warp for float source and destination images
//------------------------------------------------------------------------

struct BILIN_PIX_ADDR
{
	short u,  v;
	short ur, vr;
};

struct BILIN_COEF
{
	float uc, vc;
};

#if defined(VT_SSE_SUPPORTED)
inline void BilinProcessAddrSSE(OUT BILIN_PIX_ADDR *pP,
								OUT BILIN_COEF     *pC,
								int leftB, int topB, int rightB, int bottomB,
								const CVec2f* pAddr, int iCnt)
{
    __m128i x6i = _mm_set_epi32(topB,leftB,topB,leftB);
    __m128i x5i = _mm_set_epi32(bottomB,rightB,bottomB,rightB);
    for ( int x = 0; x < iCnt; x+=2, pAddr+=2, pP+=2, pC+=2 )
    {
        __m128  x0f = _mm_loadu_ps(&pAddr->x);      // x0,y0,x1,y1
        __m128i x1i = _mm_cvttps_epi32(x0f);  
        __m128  x2f = _mm_cvtepi32_ps(x1i);
        x2f = _mm_cmplt_ps(x0f, x2f);
        x1i = _mm_add_epi32(x1i, _mm_castps_si128(x2f));  // int   floor(x0,y0,x1,y1)
        x2f = _mm_cvtepi32_ps(x1i);                 // float floor(x0,y0,x1,y1)
    
        x0f = _mm_sub_ps(x0f, x2f);                 // generate coefs
        _mm_storeu_ps((float*)pC, x0f);             //x0c, y0c, x1c, y1c
        
        x1i = _mm_sub_epi32(x1i, x6i);              // x0,y0,x1,y1	
        __m128i x0i = _mm_sub_epi32(x5i, x1i);      // x0r,y0r,x1r,y1r
        __m128i x2i =_mm_unpackhi_epi64(x1i,x0i);   // x1,y1,x1r,y1r    
        x1i =_mm_unpacklo_epi64(x1i,x0i);           // x0,y0,x0r,y0r    
        x1i = _mm_packs_epi32(x1i, x2i);            // x0,y0,x0r,y0r,x1,y1,x1r,y1r   
        _mm_storeu_si128( (__m128i*)pP, x1i);
    }
}

inline int BilinProcessPixels1BandSSE(OUT float* pDstF,
                                      const IN CFloatImg& imgSrcBlk,
                                      const IN BILIN_PIX_ADDR *pP,
                                      const IN BILIN_COEF     *pC,
                                      int iCnt)
{
	size_t stride = imgSrcBlk.StrideBytes();

    float pkd00[4], pkd01[4], pkd10[4], pkd11[4], coefx[4], coefy[4];

    int x = 0;
    for ( ;x < iCnt-3; x+=4, pDstF+=4, pP+=4, pC+=4 )
    {
        // pack the next 4 pixels and their coefs
        for( int i = 0; i < 4; i++ )
        {        
            if ( (pP[i].u|pP[i].v|pP[i].ur|pP[i].vr) & 0x8000 )
            {
                // if any of the sign bits for the next 4 pixels are not 0 then 
                // this set of output pixel requires source pixels that weren't 
                // provided 
                goto Exit;
            }

            const float* pSrc = imgSrcBlk.Ptr(pP[i].u, pP[i].v);
            pkd00[i]  = pSrc[0];
            pkd01[i]  = pSrc[1];
            pSrc = (float*)((Byte*)pSrc + stride);
            pkd10[i]  = pSrc[0];
            pkd11[i]  = pSrc[1];
            coefx[i] = pC[i].uc;
            coefy[i] = pC[i].vc;
        }

        // load the horizontal coefs
        __m128 x1 = _mm_loadu_ps(coefx);

        // filter 1st row x 4 bands  -> result in x2
        __m128 x2 = _mm_loadu_ps(pkd00);
        __m128 x3 = _mm_loadu_ps(pkd01);
        x3 = _mm_sub_ps(x3, x2);    // v1-v0
        x3 = _mm_mul_ps(x3, x1);    // c*(v1-v0)
        x2 = _mm_add_ps(x2, x3);    // v0+c*(v1-v0)  

        // filter 2nd row x 4 bands -> result in x3
        x3 = _mm_loadu_ps(pkd10);
        __m128 x4 = _mm_loadu_ps(pkd11);
        x4 = _mm_sub_ps(x4, x3);
        x4 = _mm_mul_ps(x4, x1);
        x3 = _mm_add_ps(x3, x4);

        // load the vertical coefs
        x1 = _mm_loadu_ps(coefy);

        // do the vertical filter
        x3 = _mm_sub_ps(x3, x2);
        x3 = _mm_mul_ps(x3, x1);
        x2 = _mm_add_ps(x2, x3);

        // store the result
        _mm_storeu_ps(pDstF,  x2);
    }
Exit:
	return x;
}

template<__m128 (*pfnLoad)(const float*), void (*pfnStore)(float*, const __m128&)>
int BilinearWarpSpan4BandSSE(float* pDstF, const CFloatImg& imgSrcBlk, 
                             const CVec2f* pAddr, int iSpan,
                             int leftB, int rightB, int topB, int bottomB)
{
    int x = 0;
    int stride = imgSrcBlk.StrideBytes();

    __m128i x_minus1   = _mm_set1_epi32(-1);
    __m128i x_tl_bound = _mm_set_epi32(topB,leftB,topB,leftB);
    __m128i x_br_bound = _mm_set_epi32(bottomB,rightB,bottomB,rightB);
    for ( ; x < iSpan-1; x+=2, pAddr+=2, pDstF+=8 )
    {
        __m128  x_addr   = _mm_loadu_ps(&pAddr->x);      // x0,y0,x1,y1
        __m128i x_addr_i = _mm_cvttps_epi32(x_addr);  
        __m128  x_addrclmp_f = _mm_cvtepi32_ps(x_addr_i);
        __m128  x_addr_mask = _mm_cmplt_ps(x_addr, x_addrclmp_f);
        x_addr_i = _mm_add_epi32(x_addr_i, _mm_castps_si128(x_addr_mask)); // floor(x0,y0,x1,y1)
        __m128  x_addrflr = _mm_cvtepi32_ps(x_addr_i);                     // floor(x0,y0,x1,y1)
        __m128  x_coef    = _mm_sub_ps(x_addr, x_addrflr);                 // generate coefs

        // compute the addresses relative to the boundaries of the supplied tile
        x_addr_i = _mm_sub_epi32(x_addr_i, x_tl_bound);          // x0,y0,x1,y1	
        __m128i x_addr_br = _mm_sub_epi32(x_br_bound, x_addr_i); // x0r,y0r,x1r,y1r

        // compare relative addresses against 0 
        __m128i x_mask_tl = _mm_cmplt_epi32(x_minus1, x_addr_i);
        __m128i x_mask_br = _mm_cmplt_epi32(x_minus1, x_addr_br);

        // clip the addresses against the generated masks
        x_addr_i = _mm_and_si128 (x_addr_i, x_mask_tl);
        x_addr_i = _mm_and_si128 (x_addr_i, x_mask_br);

        // --- generate pixel 1
        // fetch from the clipped addresses
        const float* pSrc = imgSrcBlk.Ptr(
#if defined(VT_IS_MSVC_COMPILER)
            x_addr_i.m128i_i32[0], x_addr_i.m128i_i32[1]
#else
			SSE2_mm_extract_epi32(x_addr_i, 0), SSE2_mm_extract_epi32(x_addr_i, 1)
#endif
		);

        // load the horizontal coefs
        __m128 x1 = _mm_shuffle_ps(x_coef, x_coef, _MM_SHUFFLE(0,0,0,0));

        // filter 1st row x 4 bands  -> result in x2
        __m128 x2 = pfnLoad(pSrc);
        __m128 x3 = pfnLoad(pSrc+4);
        x3 = _mm_sub_ps(x3, x2);    // v1-v0
        x3 = _mm_mul_ps(x3, x1);    // c*(v1-v0)
        x2 = _mm_add_ps(x2, x3);    // v0+c*(v1-v0)  

        // filter 2nd row x 4 bands -> result in x3
        pSrc = (float*)((Byte*)pSrc + stride);
        x3 = pfnLoad(pSrc);
        __m128 x4 = pfnLoad(pSrc+4);
        x4 = _mm_sub_ps(x4, x3);
        x4 = _mm_mul_ps(x4, x1);
        x3 = _mm_add_ps(x3, x4);

        // load the vertical coefs
        x1 = _mm_shuffle_ps(x_coef, x_coef, _MM_SHUFFLE(1,1,1,1));

        // do the vertical filter
        x3 = _mm_sub_ps(x3, x2);
        x3 = _mm_mul_ps(x3, x1);
        x2 = _mm_add_ps(x2, x3);

        // generate the result mask based on out-of-bounds pixels
        __m128i x_mask_pix1 = _mm_set1_epi32(
#if defined(VT_IS_MSVC_COMPILER)
            x_mask_tl.m128i_i32[0] & x_mask_tl.m128i_i32[1] &
            x_mask_br.m128i_i32[0] & x_mask_br.m128i_i32[1]
#else
			SSE2_mm_extract_epi32(x_mask_tl, 0) & SSE2_mm_extract_epi32(x_mask_tl, 1) &
			SSE2_mm_extract_epi32(x_mask_br, 0) & SSE2_mm_extract_epi32(x_mask_br, 1)
#endif
		);
        x2 = _mm_castsi128_ps(_mm_and_si128 (_mm_castps_si128(x2), x_mask_pix1));

        // store the result
        pfnStore(pDstF,  x2);

        // --- repeat above for pixel 2
        // fetch from the clipped addresses
        pSrc = imgSrcBlk.Ptr(
#if defined(VT_IS_MSVC_COMPILER)
            x_addr_i.m128i_i32[2], x_addr_i.m128i_i32[3]
#else
			SSE2_mm_extract_epi32(x_addr_i, 2), SSE2_mm_extract_epi32(x_addr_i, 3)
#endif
		);

        // load the horizontal coefs
        x1 = _mm_shuffle_ps(x_coef, x_coef, _MM_SHUFFLE(2,2,2,2));

        // filter 1st row x 4 bands  -> result in x2
        x2 = pfnLoad(pSrc);
        x3 = pfnLoad(pSrc+4);
        x3 = _mm_sub_ps(x3, x2);    // v1-v0
        x3 = _mm_mul_ps(x3, x1);    // c*(v1-v0)
        x2 = _mm_add_ps(x2, x3);    // v0+c*(v1-v0)  

        // filter 2nd row x 4 bands -> result in x3
        pSrc = (float*)((Byte*)pSrc + stride);
        x3 = pfnLoad(pSrc);
        x4 = pfnLoad(pSrc+4);
        x4 = _mm_sub_ps(x4, x3);
        x4 = _mm_mul_ps(x4, x1);
        x3 = _mm_add_ps(x3, x4);

        // load the vertical coefs
        x1 = _mm_shuffle_ps(x_coef, x_coef, _MM_SHUFFLE(3,3,3,3));

        // do the vertical filter
        x3 = _mm_sub_ps(x3, x2);
        x3 = _mm_mul_ps(x3, x1);
        x2 = _mm_add_ps(x2, x3);

        // generate the result mask based on out-of-bounds pixels
        __m128i x_mask_pix2 = _mm_set1_epi32(
#if defined(VT_IS_MSVC_COMPILER)
			x_mask_tl.m128i_i32[2] & x_mask_tl.m128i_i32[3] &
            x_mask_br.m128i_i32[2] & x_mask_br.m128i_i32[3]
#else
			SSE2_mm_extract_epi32(x_mask_tl, 2) & SSE2_mm_extract_epi32(x_mask_tl, 3) &
			SSE2_mm_extract_epi32(x_mask_br, 2) & SSE2_mm_extract_epi32(x_mask_br, 3)
#endif
		);
        x2 = _mm_castsi128_ps(_mm_and_si128 (_mm_castps_si128(x2), x_mask_pix2));

        // store the result
        pfnStore(pDstF+4,  x2);
    }

    return x;
}

int BilinearWarpSpan4BandSSE(float* pDstF, const CFloatImg& imgSrcBlk, 
                             const CVec2f* pAddr, int iSpan,
                             int leftB, int rightB, int topB, int bottomB)
{
    bool bDstAligned = IsAligned16(pDstF);
    bool bSrcAligned = IsAligned16(imgSrcBlk.Ptr(0)) && IsAligned16(imgSrcBlk.Ptr(1));
    if( bDstAligned && bSrcAligned )
    {
        return BilinearWarpSpan4BandSSE<Load4AlignedSSE, Store4AlignedSSE>(
            pDstF, imgSrcBlk, pAddr, iSpan, leftB, rightB, topB, bottomB);
    }
    else
    {
        return BilinearWarpSpan4BandSSE<Load4UnAlignedSSE, Store4UnAlignedSSE>(
            pDstF, imgSrcBlk, pAddr, iSpan, leftB, rightB, topB, bottomB);
    }
}

void
BilinearWarpSpanSSE(float* pDstF, const CFloatImg& imgSrcBlk, 
                    const vt::CRect& rctSrcBlk, const CVec2f* pAddr, int iSpan)
{
    int nb = imgSrcBlk.Bands();

    int leftB   = rctSrcBlk.left;
    int rightB  = imgSrcBlk.Width()-2;
    int topB    = rctSrcBlk.top;
    int bottomB = imgSrcBlk.Height()-2;

        const int c_maxintaddrspan = 64;
        BILIN_PIX_ADDR  pixaddrbuf[c_maxintaddrspan];
        BILIN_COEF      coefbuf[c_maxintaddrspan];

        for ( int k = 0; k < iSpan; k+=c_maxintaddrspan)
        {
            int iCurSpan = VtMin(c_maxintaddrspan, iSpan-k);

            // convert the float addresses to pixel coordinates and filter coefs
            BilinProcessAddrSSE(pixaddrbuf, coefbuf, leftB, topB, rightB, bottomB,
                                pAddr, iCurSpan);
            pAddr+=iCurSpan;

            // perform the filter
            BILIN_PIX_ADDR *pP = pixaddrbuf;
            BILIN_COEF     *pC = coefbuf;
            for ( int x = 0; x < iCurSpan; )
            {
                if ( nb==1 )
                {
                    int c = BilinProcessPixels1BandSSE(pDstF, imgSrcBlk, pP, pC,
                                                       iCurSpan-x);
                    pDstF += c;
                    x  += c;
                    pP += c;
                    pC += c;
                }

                size_t stride = imgSrcBlk.StrideBytes();
                for ( ;x < iCurSpan; x++, pP++, pC++ )
                {
                    if ( (pP->u|pP->v|pP->ur|pP->vr) & 0x8000 )
                    {
                        // if any of the sign bits are not 0 then this output
                        // pixel requires source pixels that weren't provided 
                        break;
                    }

                    // get the top-left source pixel
                    const float* pSrc = imgSrcBlk.Ptr(pP->u, pP->v);

                    // compute the bilinear coef
                    float xc0 = 1.f-pC->uc;
                    float xc1 = pC->uc;
                    float yc0 = 1.f-pC->vc;
                    float yc1 = pC->vc;

                    for ( int b = 0; b < nb; b++, pDstF++, pSrc++ )
                    {
                        const float *pr = pSrc;
                        float y0 = BilinearFilterPixel(pr[0], pr[nb], xc0, xc1);

                        pr = (float*)((Byte*)pr + stride);
                        float y1 = BilinearFilterPixel(pr[0], pr[nb], xc0, xc1);

                        *pDstF = BilinearFilterPixel(y0, y1, yc0, yc1);
                    }
                }

                int iClearSpan = 0;
                for ( ;x < iCurSpan; x++, iClearSpan++, pP++, pC++ )
                {
                    if ( ((pP->u|pP->v|pP->ur|pP->vr) & 0x8000) == 0 )
                    {
                        // opposite termination condition from above while(), 
                        // thus keep accumulating OOB addresses
                        break;
                    }
                }
                if ( iClearSpan )
                {
                    // clear the OOB span
                    memset(pDstF, 0, iClearSpan*nb*sizeof(float));
                    pDstF += iClearSpan*nb;
                }
            } 
        }
}

//+----------------------------------------------------------------------------
// bicubic warp support
//-----------------------------------------------------------------------------

// load coefs into the XMM registers and unpacks into what filters below need
inline void
LoadCubicCoefsSSE(__m128& x0, __m128& x1, __m128& x2, __m128& x3,
                  const float* pCoefs)
{
    x0 = _mm_loadu_ps(pCoefs);
    x2 = x0;

    x0 = _mm_unpacklo_ps(x0, x0);  // x0 = c1c1c0c0
    x1 = _mm_movehl_ps(x0, x0);    // x1 = c1c1c1c1  - 2 instruc
    x0 = _mm_movelh_ps(x0, x0);    // x0 = c0c0c0c0
    x2 = _mm_unpackhi_ps(x2, x2);  // x2 = c3c3c2c2
    x3 = _mm_movehl_ps(x2, x2);    // x3 = c3c3c3c3  - 2 instruc
    x2 = _mm_movelh_ps(x2, x2);    // x2 = c2c2c2c2  
}
#endif

struct BICUBIC_PIX_ADDR
{
	short u,  v;
	short ur, vr;
};

struct BICUBIC_COEF_ADDR
{
	short uc, vc;
};

#if defined(VT_SSE_SUPPORTED)
void
BicubicWarpSpan4SSE(float* pDstF, 
					const CFloatImg& imgSrcBlk, const vt::CRect& rctSrcBlk,
					const CVec2f* pAddr, const float* pFilterCoef, int iSpan)
{
	const int iFilterSupport = 1;
	const int iFilterWidth   = 4;
	int leftB   = iFilterSupport+rctSrcBlk.left;
	int rightB  = imgSrcBlk.Width()-iFilterWidth;
	int topB    = iFilterSupport+rctSrcBlk.top;
	int bottomB = imgSrcBlk.Height()-iFilterWidth;

	size_t stride = imgSrcBlk.StrideBytes();

	const int c_maxintaddrspan = 64;
	BICUBIC_PIX_ADDR  pixaddrbuf[c_maxintaddrspan];
	BICUBIC_COEF_ADDR coefaddrbuf[c_maxintaddrspan];

	for( int k = 0; k < iSpan; k+=c_maxintaddrspan)
	{
		int iCurSpan = VtMin(c_maxintaddrspan, iSpan-k);

		// convert the float addresses to pixel coordinates and filter coef
		// look-up table coordinates
		BICUBIC_PIX_ADDR* pP  = pixaddrbuf;
		BICUBIC_COEF_ADDR* pC = coefaddrbuf;
		__m128  x7f = _mm_set1_ps(float(WARP_LOOKUP_COUNT));
		__m128i x6i = _mm_set_epi32(topB,leftB,topB,leftB);
		__m128i x5i = _mm_set_epi32(bottomB,rightB,bottomB,rightB);
		for( int x = 0; x < iCurSpan; x+=2, pAddr+=2, pP+=2, pC+=2 )
		{
			__m128  x0f = _mm_loadu_ps(&pAddr->x);      // x0,y0,x1,y1
			__m128i x1i = _mm_cvttps_epi32(x0f);  
			__m128  x2f = _mm_cvtepi32_ps(x1i);
			x2f = _mm_cmplt_ps(x0f, x2f);
			x1i = _mm_add_epi32(x1i, _mm_castps_si128(x2f));  // int   floor(x0,y0,x1,y1)
			x2f = _mm_cvtepi32_ps(x1i);                 // float floor(x0,y0,x1,y1)
	
			x0f = _mm_sub_ps(x0f, x2f);                 // generate coef addresses
			x0f = _mm_mul_ps(x0f, x7f);
			__m128i x0i = _mm_cvtps_epi32(x0f);
			x0i = _mm_slli_epi32 (x0i, 2);
			x0i = _mm_packs_epi32(x0i, x1i);
			_mm_storel_epi64((__m128i*)pC, x0i);

			x1i = _mm_sub_epi32(x1i, x6i);              // x0,y0,x1,y1	
			x0i = _mm_sub_epi32(x5i, x1i);              // x0r,y0r,x1r,y1r
			__m128i x2i =_mm_unpackhi_epi64(x1i,x0i);   // x1,y1,x1r,y1r    
			x1i =_mm_unpacklo_epi64(x1i,x0i);           // x0,y0,x0r,y0r    
			x1i = _mm_packs_epi32(x1i, x2i);            // x0,y0,x0r,y0r,x1,y1,x1r,y1r   
			_mm_storeu_si128( (__m128i*)pP, x1i);
		}

		// perform the warp
		pP  = pixaddrbuf;
		pC  = coefaddrbuf;
		for( int x = 0; x < iCurSpan; )
		{
			for( ;x < iCurSpan; x++, pDstF+=4, pP++, pC++ )
			{
				if( (pP->u|pP->v|pP->ur|pP->vr) & 0x8000 )
				{
					// if any of the sign bits are not 0 then this output pixel
					// requires source pixels that weren't provided 
					break;
				} 
				const float* pSrc = imgSrcBlk.Ptr(pP->u, pP->v);
				const float* pxcoef = pFilterCoef+pC->uc;
				const float* pycoef = pFilterCoef+pC->vc;
	
				_mm_prefetch((char*)pycoef, _MM_HINT_T0);
	
				// load the horizontal coefs
				__m128 x0, x1, x2, x3;
				LoadCubicCoefsSSE(x0, x1, x2, x3, pxcoef);
	
				// filter 1st row x 4 bands  -> result in x4
				__m128 x4 = _mm_loadu_ps(pSrc);
				 x4 = _mm_mul_ps(x4, x0);
	
				__m128 x5 = _mm_loadu_ps(pSrc+4);
				x5 = _mm_mul_ps(x5, x1);
				x4 = _mm_add_ps(x4, x5);
	
				x5 = _mm_loadu_ps(pSrc+8);
				x5 = _mm_mul_ps(x5, x2);
				x4 = _mm_add_ps(x4, x5);
	
				x5 = _mm_loadu_ps(pSrc+12);
				x5 = _mm_mul_ps(x5, x3);
				x4 = _mm_add_ps(x4, x5);
	
				// filter 2nd row x 4 bands -> result in x5
				pSrc = (float*)((Byte*)pSrc + stride);
				x5 = _mm_loadu_ps(pSrc);
				x5 = _mm_mul_ps(x5, x0);
	
				__m128 x6 = _mm_loadu_ps(pSrc+4);
				x6 = _mm_mul_ps(x6, x1);
				x5 = _mm_add_ps(x5, x6);
	
				x6 = _mm_loadu_ps(pSrc+8);
				x6 = _mm_mul_ps(x6, x2);
				x5 = _mm_add_ps(x5, x6);
	
				x6 = _mm_loadu_ps(pSrc+12);
				x6 = _mm_mul_ps(x6, x3);
				x5 = _mm_add_ps(x5, x6);
	
				// filter 3nd row x 4 bands -> result in x6
				pSrc = (float*)((Byte*)pSrc + stride);
				x6 = _mm_loadu_ps(pSrc);
	
				x6 = _mm_mul_ps(x6, x0);
	
				__m128 x7 = _mm_loadu_ps(pSrc+4);
				x7 = _mm_mul_ps(x7, x1);
				x6 = _mm_add_ps(x6, x7);
	
				x7 = _mm_loadu_ps(pSrc+8);
				x7 = _mm_mul_ps(x7, x2);
				x6 = _mm_add_ps(x6, x7);
	
				x7 = _mm_loadu_ps(pSrc+12);
				x7 = _mm_mul_ps(x7, x3);
				x6 = _mm_add_ps(x6, x7);
	
				// filter 4th row x 4 bands -> result in x7
				pSrc = (float*)((Byte*)pSrc + stride);
				x7 = _mm_loadu_ps(pSrc);
				x7 = _mm_mul_ps(x7, x0);
	
				x0 = _mm_loadu_ps(pSrc+4);
				x0 = _mm_mul_ps(x0, x1);
				x7 = _mm_add_ps(x7, x0);
	
				x0 = _mm_loadu_ps(pSrc+8);
				x0 = _mm_mul_ps(x0, x2);
				x7 = _mm_add_ps(x7, x0);
	
				x0 = _mm_loadu_ps(pSrc+12);
				x0 = _mm_mul_ps(x0, x3);
				x7 = _mm_add_ps(x7, x0);
	
				// now do vertical filter
				LoadCubicCoefsSSE(x0, x1, x2, x3, (const float*)pycoef);
	
				x4 = _mm_mul_ps(x4, x0);
	
				x5 = _mm_mul_ps(x5, x1);
				x4 = _mm_add_ps(x4, x5);
	
				x6 = _mm_mul_ps(x6, x2);
				x4 = _mm_add_ps(x4, x6);
	
				x7 = _mm_mul_ps(x7, x3);
				x4 = _mm_add_ps(x4, x7);
	
				// store the result
				_mm_storeu_ps(pDstF,  x4);
			}
	
			int iClearSpan = 0;
			for( ;x < iCurSpan; x++, iClearSpan++, pP++, pC++ )
			{
				if( ((pP->u|pP->v|pP->ur|pP->vr) & 0x8000) == 0 )
				{
					// opposite termination condition from above while(), 
					// thus keep accumulating OOB addresses
					break;
				}
			}
			if( iClearSpan )
			{
				// clear the OOB span
				memset(pDstF, 0, iClearSpan*4*sizeof(float));
				pDstF += iClearSpan*4;
			}
		}
	}
}
#endif

