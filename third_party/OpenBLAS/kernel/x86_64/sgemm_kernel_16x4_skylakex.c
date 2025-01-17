/*********************************************************************************
Copyright (c) 2013, The OpenBLAS Project
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
3. Neither the name of the OpenBLAS project nor the names of
its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE OPENBLAS PROJECT OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************************/


/* comment below left for history, data does not represent the implementation in this file */

/*********************************************************************
* 2014/07/28 Saar
*        BLASTEST               : OK
*        CTEST                  : OK
*        TEST                   : OK
*
* 2013/10/28 Saar
* Parameter:
*	SGEMM_DEFAULT_UNROLL_N	4
*	SGEMM_DEFAULT_UNROLL_M	16
*	SGEMM_DEFAULT_P		768
*	SGEMM_DEFAULT_Q		384
*	A_PR1			512
*	B_PR1			512
*	
* 
* 2014/07/28 Saar
* Performance at 9216x9216x9216:
*       1 thread:      102 GFLOPS       (SANDYBRIDGE:  59)      (MKL:   83)
*       2 threads:     195 GFLOPS       (SANDYBRIDGE: 116)      (MKL:  155)
*       3 threads:     281 GFLOPS       (SANDYBRIDGE: 165)      (MKL:  230)
*       4 threads:     366 GFLOPS       (SANDYBRIDGE: 223)      (MKL:  267)
*
*********************************************************************/

#include "common.h"
#include <immintrin.h>



/*******************************************************************************************
* 8 lines of N
*******************************************************************************************/
 





/*******************************************************************************************
* 4 lines of N
*******************************************************************************************/

#define INIT64x4()	\
	row0 = _mm512_setzero_ps();					\
	row1 = _mm512_setzero_ps();					\
	row2 = _mm512_setzero_ps();					\
	row3 = _mm512_setzero_ps();					\
	row0b = _mm512_setzero_ps();					\
	row1b = _mm512_setzero_ps();					\
	row2b = _mm512_setzero_ps();					\
	row3b = _mm512_setzero_ps();					\
	row0c = _mm512_setzero_ps();					\
	row1c = _mm512_setzero_ps();					\
	row2c = _mm512_setzero_ps();					\
	row3c = _mm512_setzero_ps();					\
	row0d = _mm512_setzero_ps();					\
	row1d = _mm512_setzero_ps();					\
	row2d = _mm512_setzero_ps();					\
	row3d = _mm512_setzero_ps();					\

#define KERNEL64x4_SUB() 						\
	zmm0   = _mm512_loadu_ps(AO);					\
	zmm1   = _mm512_loadu_ps(A1);					\
	zmm5   = _mm512_loadu_ps(A2);					\
	zmm7   = _mm512_loadu_ps(A3);					\
	zmm2   =  _mm512_broadcastss_ps(_mm_load_ss(BO));		\
	zmm3   =  _mm512_broadcastss_ps(_mm_load_ss(BO+1));		\
	row0  += zmm0 * zmm2;						\
	row1  += zmm0 * zmm3;						\
	row0b += zmm1 * zmm2;						\
	row1b += zmm1 * zmm3;						\
	row0c += zmm5 * zmm2;						\
	row1c += zmm5 * zmm3;						\
	row0d += zmm7 * zmm2;						\
	row1d += zmm7 * zmm3;						\
	zmm2   =  _mm512_broadcastss_ps(_mm_load_ss(BO+2));		\
	zmm3   =  _mm512_broadcastss_ps(_mm_load_ss(BO+3));		\
	row2  += zmm0 * zmm2;						\
	row3 += zmm0 * zmm3;						\
	row2b += zmm1 * zmm2;						\
	row3b += zmm1 * zmm3;						\
	row2c += zmm5 * zmm2;						\
	row3c += zmm5 * zmm3;						\
	row2d += zmm7 * zmm2;						\
	row3d += zmm7 * zmm3;						\
	BO += 4;							\
	AO += 16;							\
	A1 += 16;							\
	A2 += 16;							\
	A3 += 16;							\


#define SAVE64x4(ALPHA)							\
	zmm0   = _mm512_set1_ps(ALPHA);					\
	row0  *= zmm0;							\
	row1  *= zmm0;							\
	row2  *= zmm0;							\
	row3 *= zmm0;							\
	row0b *= zmm0;							\
	row1b *= zmm0;							\
	row2b *= zmm0;							\
	row3b *= zmm0;							\
	row0c *= zmm0;							\
	row1c *= zmm0;							\
	row2c *= zmm0;							\
	row3c *= zmm0;							\
	row0d *= zmm0;							\
	row1d *= zmm0;							\
	row2d *= zmm0;							\
	row3d *= zmm0;							\
	row0  += _mm512_loadu_ps(CO1 + 0*ldc);				\
	row1  += _mm512_loadu_ps(CO1 + 1*ldc);				\
	row2  += _mm512_loadu_ps(CO1 + 2*ldc);				\
	row3 += _mm512_loadu_ps(CO1 + 3*ldc);				\
	_mm512_storeu_ps(CO1 + 0*ldc, row0);				\
	_mm512_storeu_ps(CO1 + 1*ldc, row1);				\
	_mm512_storeu_ps(CO1 + 2*ldc, row2);				\
	_mm512_storeu_ps(CO1 + 3*ldc, row3);				\
	row0b  += _mm512_loadu_ps(CO1 + 0*ldc + 16);			\
	row1b  += _mm512_loadu_ps(CO1 + 1*ldc + 16);			\
	row2b  += _mm512_loadu_ps(CO1 + 2*ldc + 16);			\
	row3b += _mm512_loadu_ps(CO1 + 3*ldc + 16);			\
	_mm512_storeu_ps(CO1 + 0*ldc + 16, row0b);			\
	_mm512_storeu_ps(CO1 + 1*ldc + 16, row1b);			\
	_mm512_storeu_ps(CO1 + 2*ldc + 16, row2b);			\
	_mm512_storeu_ps(CO1 + 3*ldc + 16, row3b);			\
	row0c  += _mm512_loadu_ps(CO1 + 0*ldc + 32);			\
	row1c  += _mm512_loadu_ps(CO1 + 1*ldc + 32);			\
	row2c  += _mm512_loadu_ps(CO1 + 2*ldc + 32);			\
	row3c  += _mm512_loadu_ps(CO1 + 3*ldc + 32);			\
	_mm512_storeu_ps(CO1 + 0*ldc + 32, row0c);			\
	_mm512_storeu_ps(CO1 + 1*ldc + 32, row1c);			\
	_mm512_storeu_ps(CO1 + 2*ldc + 32, row2c);			\
	_mm512_storeu_ps(CO1 + 3*ldc + 32, row3c);			\
	row0d  += _mm512_loadu_ps(CO1 + 0*ldc + 48);			\
	row1d  += _mm512_loadu_ps(CO1 + 1*ldc + 48);			\
	row2d  += _mm512_loadu_ps(CO1 + 2*ldc + 48);			\
	row3d  += _mm512_loadu_ps(CO1 + 3*ldc + 48);			\
	_mm512_storeu_ps(CO1 + 0*ldc + 48, row0d);			\
	_mm512_storeu_ps(CO1 + 1*ldc + 48, row1d);			\
	_mm512_storeu_ps(CO1 + 2*ldc + 48, row2d);			\
	_mm512_storeu_ps(CO1 + 3*ldc + 48, row3d);		


#define INIT48x4()	\
	row0 = _mm512_setzero_ps();					\
	row1 = _mm512_setzero_ps();					\
	row2 = _mm512_setzero_ps();					\
	row3 = _mm512_setzero_ps();					\
	row0b = _mm512_setzero_ps();					\
	row1b = _mm512_setzero_ps();					\
	row2b = _mm512_setzero_ps();					\
	row3b = _mm512_setzero_ps();					\
	row0c = _mm512_setzero_ps();					\
	row1c = _mm512_setzero_ps();					\
	row2c = _mm512_setzero_ps();					\
	row3c = _mm512_setzero_ps();					\

#define KERNEL48x4_SUB() 						\
	zmm0   = _mm512_loadu_ps(AO);					\
	zmm1   = _mm512_loadu_ps(A1);					\
	zmm5   = _mm512_loadu_ps(A2);					\
	zmm2   =  _mm512_broadcastss_ps(_mm_load_ss(BO));		\
	zmm3   =  _mm512_broadcastss_ps(_mm_load_ss(BO+1));		\
	row0  += zmm0 * zmm2;						\
	row1  += zmm0 * zmm3;						\
	row0b += zmm1 * zmm2;						\
	row1b += zmm1 * zmm3;						\
	row0c += zmm5 * zmm2;						\
	row1c += zmm5 * zmm3;						\
	zmm2   =  _mm512_broadcastss_ps(_mm_load_ss(BO+2));		\
	zmm3   =  _mm512_broadcastss_ps(_mm_load_ss(BO+3));		\
	row2  += zmm0 * zmm2;						\
	row3 += zmm0 * zmm3;						\
	row2b += zmm1 * zmm2;						\
	row3b += zmm1 * zmm3;						\
	row2c += zmm5 * zmm2;						\
	row3c += zmm5 * zmm3;						\
	BO += 4;							\
	AO += 16;							\
	A1 += 16;							\
	A2 += 16;


#define SAVE48x4(ALPHA)							\
	zmm0   = _mm512_set1_ps(ALPHA);					\
	row0  *= zmm0;							\
	row1  *= zmm0;							\
	row2  *= zmm0;							\
	row3 *= zmm0;							\
	row0b *= zmm0;							\
	row1b *= zmm0;							\
	row2b *= zmm0;							\
	row3b *= zmm0;							\
	row0c *= zmm0;							\
	row1c *= zmm0;							\
	row2c *= zmm0;							\
	row3c *= zmm0;							\
	row0  += _mm512_loadu_ps(CO1 + 0*ldc);				\
	row1  += _mm512_loadu_ps(CO1 + 1*ldc);				\
	row2  += _mm512_loadu_ps(CO1 + 2*ldc);				\
	row3 += _mm512_loadu_ps(CO1 + 3*ldc);				\
	_mm512_storeu_ps(CO1 + 0*ldc, row0);				\
	_mm512_storeu_ps(CO1 + 1*ldc, row1);				\
	_mm512_storeu_ps(CO1 + 2*ldc, row2);				\
	_mm512_storeu_ps(CO1 + 3*ldc, row3);				\
	row0b  += _mm512_loadu_ps(CO1 + 0*ldc + 16);			\
	row1b  += _mm512_loadu_ps(CO1 + 1*ldc + 16);			\
	row2b  += _mm512_loadu_ps(CO1 + 2*ldc + 16);			\
	row3b += _mm512_loadu_ps(CO1 + 3*ldc + 16);			\
	_mm512_storeu_ps(CO1 + 0*ldc + 16, row0b);			\
	_mm512_storeu_ps(CO1 + 1*ldc + 16, row1b);			\
	_mm512_storeu_ps(CO1 + 2*ldc + 16, row2b);			\
	_mm512_storeu_ps(CO1 + 3*ldc + 16, row3b);			\
	row0c  += _mm512_loadu_ps(CO1 + 0*ldc + 32);			\
	row1c  += _mm512_loadu_ps(CO1 + 1*ldc + 32);			\
	row2c  += _mm512_loadu_ps(CO1 + 2*ldc + 32);			\
	row3c  += _mm512_loadu_ps(CO1 + 3*ldc + 32);			\
	_mm512_storeu_ps(CO1 + 0*ldc + 32, row0c);			\
	_mm512_storeu_ps(CO1 + 1*ldc + 32, row1c);			\
	_mm512_storeu_ps(CO1 + 2*ldc + 32, row2c);			\
	_mm512_storeu_ps(CO1 + 3*ldc + 32, row3c);		


#define INIT32x4()	\
	row0 = _mm512_setzero_ps();					\
	row1 = _mm512_setzero_ps();					\
	row2 = _mm512_setzero_ps();					\
	row3 = _mm512_setzero_ps();					\
	row0b = _mm512_setzero_ps();					\
	row1b = _mm512_setzero_ps();					\
	row2b = _mm512_setzero_ps();					\
	row3b = _mm512_setzero_ps();					\

#define KERNEL32x4_SUB() 						\
	zmm0   = _mm512_loadu_ps(AO);					\
	zmm1   = _mm512_loadu_ps(A1);					\
	zmm2   =  _mm512_broadcastss_ps(_mm_load_ss(BO));		\
	zmm3   =  _mm512_broadcastss_ps(_mm_load_ss(BO+1));		\
	row0  += zmm0 * zmm2;						\
	row1  += zmm0 * zmm3;						\
	row0b += zmm1 * zmm2;						\
	row1b += zmm1 * zmm3;						\
	zmm2   =  _mm512_broadcastss_ps(_mm_load_ss(BO+2));		\
	zmm3   =  _mm512_broadcastss_ps(_mm_load_ss(BO+3));		\
	row2  += zmm0 * zmm2;						\
	row3  += zmm0 * zmm3;						\
	row2b += zmm1 * zmm2;						\
	row3b += zmm1 * zmm3;						\
	BO += 4;							\
	AO += 16;							\
	A1 += 16;


#define SAVE32x4(ALPHA)							\
	zmm0   = _mm512_set1_ps(ALPHA);					\
	row0  *= zmm0;							\
	row1  *= zmm0;							\
	row2  *= zmm0;							\
	row3 *= zmm0;							\
	row0b *= zmm0;							\
	row1b *= zmm0;							\
	row2b *= zmm0;							\
	row3b *= zmm0;							\
	row0  += _mm512_loadu_ps(CO1 + 0*ldc);				\
	row1  += _mm512_loadu_ps(CO1 + 1*ldc);				\
	row2  += _mm512_loadu_ps(CO1 + 2*ldc);				\
	row3 += _mm512_loadu_ps(CO1 + 3*ldc);				\
	_mm512_storeu_ps(CO1 + 0*ldc, row0);				\
	_mm512_storeu_ps(CO1 + 1*ldc, row1);				\
	_mm512_storeu_ps(CO1 + 2*ldc, row2);				\
	_mm512_storeu_ps(CO1 + 3*ldc, row3);				\
	row0b  += _mm512_loadu_ps(CO1 + 0*ldc + 16);			\
	row1b  += _mm512_loadu_ps(CO1 + 1*ldc + 16);			\
	row2b  += _mm512_loadu_ps(CO1 + 2*ldc + 16);			\
	row3b += _mm512_loadu_ps(CO1 + 3*ldc + 16);			\
	_mm512_storeu_ps(CO1 + 0*ldc + 16, row0b);			\
	_mm512_storeu_ps(CO1 + 1*ldc + 16, row1b);			\
	_mm512_storeu_ps(CO1 + 2*ldc + 16, row2b);			\
	_mm512_storeu_ps(CO1 + 3*ldc + 16, row3b);		



#define INIT16x4()	\
	row0 = _mm512_setzero_ps();					\
	row1 = _mm512_setzero_ps();					\
	row2 = _mm512_setzero_ps();					\
	row3 = _mm512_setzero_ps();					\

#define KERNEL16x4_SUB() 						\
	zmm0   = _mm512_loadu_ps(AO);					\
	zmm2   =  _mm512_broadcastss_ps(_mm_load_ss(BO));		\
	zmm3   =  _mm512_broadcastss_ps(_mm_load_ss(BO+1));		\
	row0  += zmm0 * zmm2;						\
	row1  += zmm0 * zmm3;						\
	zmm2   =  _mm512_broadcastss_ps(_mm_load_ss(BO+2));		\
	zmm3   =  _mm512_broadcastss_ps(_mm_load_ss(BO+3));		\
	row2  += zmm0 * zmm2;						\
	row3 += zmm0 * zmm3;						\
	BO += 4;							\
	AO += 16;


#define SAVE16x4(ALPHA)							\
	zmm0   = _mm512_set1_ps(ALPHA);					\
	row0  *= zmm0;							\
	row1  *= zmm0;							\
	row2  *= zmm0;							\
	row3  *= zmm0;							\
	row0  += _mm512_loadu_ps(CO1 + 0 * ldc);			\
	row1  += _mm512_loadu_ps(CO1 + 1 * ldc);			\
	row2  += _mm512_loadu_ps(CO1 + 2 * ldc);			\
	row3  += _mm512_loadu_ps(CO1 + 3 * ldc);			\
	_mm512_storeu_ps(CO1 + 0 * ldc, row0);				\
	_mm512_storeu_ps(CO1 + 1 * ldc, row1);				\
	_mm512_storeu_ps(CO1 + 2 * ldc, row2);				\
	_mm512_storeu_ps(CO1 + 3 * ldc, row3);			



/*******************************************************************************************/

#define INIT8x4()							\
	ymm4 = _mm256_setzero_ps();					\
	ymm6 = _mm256_setzero_ps();					\
	ymm8 = _mm256_setzero_ps();					\
	ymm10 = _mm256_setzero_ps();					\

#define KERNEL8x4_SUB() 						\
	ymm0   = _mm256_loadu_ps(AO);					\
	ymm2   =  _mm256_broadcastss_ps(_mm_load_ss(BO + 0));		\
	ymm3   =  _mm256_broadcastss_ps(_mm_load_ss(BO + 1));		\
	ymm4  += ymm0 * ymm2;						\
	ymm6  += ymm0 * ymm3;						\
	ymm2   =  _mm256_broadcastss_ps(_mm_load_ss(BO + 2));		\
	ymm3   =  _mm256_broadcastss_ps(_mm_load_ss(BO + 3));		\
	ymm8  += ymm0 * ymm2;						\
	ymm10 += ymm0 * ymm3;						\
	BO  += 4;							\
	AO  += 8;


#define SAVE8x4(ALPHA)							\
	ymm0   = _mm256_set1_ps(ALPHA);					\
	ymm4  *= ymm0;							\
	ymm6  *= ymm0;							\
	ymm8  *= ymm0;							\
	ymm10 *= ymm0;							\
	ymm4  += _mm256_loadu_ps(CO1 + 0 * ldc);			\
	ymm6  += _mm256_loadu_ps(CO1 + 1 * ldc);			\
	ymm8  += _mm256_loadu_ps(CO1 + 2 * ldc);			\
	ymm10 += _mm256_loadu_ps(CO1 + 3 * ldc);			\
	_mm256_storeu_ps(CO1 + 0 * ldc, ymm4);				\
	_mm256_storeu_ps(CO1 + 1 * ldc, ymm6);				\
	_mm256_storeu_ps(CO1 + 2 * ldc, ymm8);				\
	_mm256_storeu_ps(CO1 + 3 * ldc, ymm10);				\



/*******************************************************************************************/

#define INIT4x4()							\
	row0 = _mm_setzero_ps();					\
	row1 = _mm_setzero_ps();					\
	row2 = _mm_setzero_ps();					\
	row3 = _mm_setzero_ps();					\


#define KERNEL4x4_SUB() 						\
	xmm0   = _mm_loadu_ps(AO);					\
	xmm2   =  _mm_broadcastss_ps(_mm_load_ss(BO + 0));		\
	xmm3   =  _mm_broadcastss_ps(_mm_load_ss(BO + 1));		\
	row0  += xmm0 * xmm2;						\
	row1  += xmm0 * xmm3;						\
	xmm2   =  _mm_broadcastss_ps(_mm_load_ss(BO + 2));		\
	xmm3   =  _mm_broadcastss_ps(_mm_load_ss(BO + 3));		\
	row2  += xmm0 * xmm2;						\
	row3  += xmm0 * xmm3;						\
	BO  += 4;							\
	AO  += 4;


#define SAVE4x4(ALPHA)							\
	xmm0   = _mm_set1_ps(ALPHA);					\
	row0  *= xmm0;							\
	row1  *= xmm0;							\
	row2  *= xmm0;							\
	row3  *= xmm0;							\
	row0  += _mm_loadu_ps(CO1 + 0 * ldc);				\
	row1  += _mm_loadu_ps(CO1 + 1 * ldc);				\
	row2  += _mm_loadu_ps(CO1 + 2 * ldc);				\
	row3  += _mm_loadu_ps(CO1 + 3 * ldc);				\
	_mm_storeu_ps(CO1 + 0 * ldc, row0);				\
	_mm_storeu_ps(CO1 + 1 * ldc, row1);				\
	_mm_storeu_ps(CO1 + 2 * ldc, row2);				\
	_mm_storeu_ps(CO1 + 3 * ldc, row3);				\


/*******************************************************************************************/

#define INIT2x4() 	\
	row0 = 0; row0b = 0; row1 = 0; row1b = 0; 			\
	row2 = 0; row2b = 0; row3 = 0; row3b = 0;

#define KERNEL2x4_SUB()							\
	xmm0  = *(AO);							\
	xmm1  = *(AO + 1);						\
	xmm2  = *(BO + 0);						\
	xmm3  = *(BO + 1);						\
	row0 += xmm0 * xmm2;						\
	row0b += xmm1 * xmm2;						\
	row1 += xmm0 * xmm3;						\
	row1b += xmm1 * xmm3;						\
	xmm2 = *(BO + 2);						\
	xmm3 = *(BO + 3);						\
	row2 += xmm0 * xmm2;						\
	row2b += xmm1 * xmm2;						\
	row3 += xmm0 * xmm3;						\
	row3b += xmm1 * xmm3;						\
	BO += 4;							\
	AO += 2;


#define SAVE2x4(ALPHA)							\
	xmm0   = ALPHA;							\
	row0  *= xmm0;							\
	row0b *= xmm0;							\
	row1  *= xmm0;							\
	row1b *= xmm0;							\
	row2  *= xmm0;							\
	row2b *= xmm0;							\
	row3  *= xmm0;							\
	row3b *= xmm0;							\
	*(CO1 + 0 * ldc + 0) += row0;					\
	*(CO1 + 0 * ldc + 1) += row0b;					\
	*(CO1 + 1 * ldc + 0) += row1;					\
	*(CO1 + 1 * ldc + 1) += row1b;					\
	*(CO1 + 2 * ldc + 0) += row2;					\
	*(CO1 + 2 * ldc + 1) += row2b;					\
	*(CO1 + 3 * ldc + 0) += row3;					\
	*(CO1 + 3 * ldc + 1) += row3b;					\



/*******************************************************************************************/

#define INIT1x4() \
	row0 = 0; row1 = 0; row2 = 0; row3 = 0;
#define KERNEL1x4_SUB()							\
	xmm0  = *(AO );							\
	xmm2  = *(BO + 0);						\
	xmm3  = *(BO + 1);						\
	row0 += xmm0 * xmm2;						\
	row1 += xmm0 * xmm3;						\
	xmm2   = *(BO + 2);						\
	xmm3   = *(BO + 3);						\
	row2  += xmm0 * xmm2;						\
	row3 += xmm0 * xmm3;						\
	BO += 4;							\
	AO += 1;


#define SAVE1x4(ALPHA)							\
	xmm0   = ALPHA;							\
	row0  *= xmm0;							\
	row1  *= xmm0;							\
	row2  *= xmm0;							\
	row3  *= xmm0;							\
	*(CO1 + 0 * ldc) += row0;					\
	*(CO1 + 1 * ldc) += row1;					\
	*(CO1 + 2 * ldc) += row2;					\
	*(CO1 + 3 * ldc) += row3;					\



/*******************************************************************************************/

/*******************************************************************************************
* 2 lines of N
*******************************************************************************************/

#define INIT16x2()							\
	row0 = _mm512_setzero_ps();					\
	row1 = _mm512_setzero_ps();					\


#define KERNEL16x2_SUB() 						\
	zmm0   = _mm512_loadu_ps(AO);					\
	zmm2   =  _mm512_broadcastss_ps(_mm_load_ss(BO));		\
	zmm3   =  _mm512_broadcastss_ps(_mm_load_ss(BO + 1));		\
	row0  += zmm0 * zmm2;						\
	row1  += zmm0 * zmm3;						\
	BO += 2;							\
	AO += 16;


#define SAVE16x2(ALPHA)							\
	zmm0   = _mm512_set1_ps(ALPHA);					\
	row0  *= zmm0;							\
	row1  *= zmm0;							\
	row0  += _mm512_loadu_ps(CO1);					\
	row1  += _mm512_loadu_ps(CO1 + ldc);				\
	_mm512_storeu_ps(CO1      , row0);				\
	_mm512_storeu_ps(CO1 + ldc, row1);				\




/*******************************************************************************************/

#define INIT8x2()	\
	ymm4 = _mm256_setzero_ps();					\
	ymm6 = _mm256_setzero_ps();					\

#define KERNEL8x2_SUB() 						\
	ymm0   = _mm256_loadu_ps(AO);					\
	ymm2   =  _mm256_broadcastss_ps(_mm_load_ss(BO));		\
	ymm3   =  _mm256_broadcastss_ps(_mm_load_ss(BO + 1));		\
	ymm4  += ymm0 * ymm2;						\
	ymm6  += ymm0 * ymm3;						\
	BO  += 2;							\
	AO  += 8;


#define SAVE8x2(ALPHA)							\
	ymm0   = _mm256_set1_ps(ALPHA);					\
	ymm4  *= ymm0;							\
	ymm6  *= ymm0;							\
	ymm4  += _mm256_loadu_ps(CO1);					\
	ymm6  += _mm256_loadu_ps(CO1 + ldc);				\
	_mm256_storeu_ps(CO1      , ymm4);				\
	_mm256_storeu_ps(CO1 + ldc, ymm6);				\



/*******************************************************************************************/

#define INIT4x2()	\
	row0 = _mm_setzero_ps(); 					\
	row1 = _mm_setzero_ps(); 					\

#define KERNEL4x2_SUB() 						\
	xmm0   = _mm_loadu_ps(AO);					\
	xmm2   =  _mm_broadcastss_ps(_mm_load_ss(BO));			\
	xmm3   =  _mm_broadcastss_ps(_mm_load_ss(BO + 1));		\
	row0  += xmm0 * xmm2;						\
	row1  += xmm0 * xmm3;						\
	BO  += 2;							\
	AO  += 4;


#define SAVE4x2(ALPHA)							\
	xmm0   = _mm_set1_ps(ALPHA);					\
	row0  *= xmm0;							\
	row1  *= xmm0;							\
	row0  += _mm_loadu_ps(CO1);					\
	row1  += _mm_loadu_ps(CO1 + ldc);				\
	_mm_storeu_ps(CO1      , row0);					\
	_mm_storeu_ps(CO1 + ldc, row1);					\



/*******************************************************************************************/


#define INIT2x2() 	\
	row0 = 0; row0b = 0; row1 = 0; row1b = 0; 			\

#define KERNEL2x2_SUB()							\
	xmm0  = *(AO + 0);						\
	xmm1  = *(AO + 1);						\
	xmm2  = *(BO + 0);						\
	xmm3  = *(BO + 1);						\
	row0 += xmm0 * xmm2;						\
	row0b += xmm1 * xmm2;						\
	row1 += xmm0 * xmm3;						\
	row1b += xmm1 * xmm3;						\
	BO += 2;							\
	AO += 2;							\


#define SAVE2x2(ALPHA)							\
	xmm0   = ALPHA;							\
	row0  *= xmm0;							\
	row0b  *= xmm0;							\
	row1  *= xmm0;							\
	row1b  *= xmm0;							\
	*(CO1         ) += row0;					\
	*(CO1 +1      ) += row0b;					\
	*(CO1 + ldc   ) += row1;					\
	*(CO1 + ldc +1) += row1b;					\


/*******************************************************************************************/

#define INIT1x2()	\
	row0 = 0; row1 = 0;

#define KERNEL1x2_SUB()							\
	xmm0  = *(AO);							\
	xmm2  = *(BO + 0);						\
	xmm3  = *(BO + 1);						\
	row0 += xmm0 * xmm2;						\
	row1 += xmm0 * xmm3;						\
	BO += 2;							\
	AO += 1;


#define SAVE1x2(ALPHA)							\
	xmm0   = ALPHA;							\
	row0  *= xmm0;							\
	row1  *= xmm0;							\
	*(CO1         ) += row0;					\
	*(CO1 + ldc   ) += row1;					\


/*******************************************************************************************/

/*******************************************************************************************
* 1 line of N
*******************************************************************************************/

#define INIT16x1() \
	row0 = _mm512_setzero_ps();				\

#define KERNEL16x1_SUB() 						\
	zmm0   = _mm512_loadu_ps(AO);			\
	zmm2   =  _mm512_broadcastss_ps(_mm_load_ss(BO));		\
	row0  += zmm0 * zmm2;						\
	BO += 1;							\
	AO += 16;


#define SAVE16x1(ALPHA)							\
	zmm0   = _mm512_set1_ps(ALPHA);					\
	row0  *= zmm0;							\
	row0  += _mm512_loadu_ps(CO1);					\
	_mm512_storeu_ps(CO1      , row0);				\


/*******************************************************************************************/

#define INIT8x1()							\
	ymm4 = _mm256_setzero_ps();					

#define KERNEL8x1_SUB() 						\
	ymm0   = _mm256_loadu_ps(AO);					\
	ymm2   =  _mm256_broadcastss_ps(_mm_load_ss(BO));		\
	ymm4  += ymm0 * ymm2;						\
	BO  += 1;							\
	AO  += 8;


#define SAVE8x1(ALPHA)							\
	ymm0   = _mm256_set1_ps(ALPHA);					\
	ymm4  *= ymm0;							\
	ymm4  += _mm256_loadu_ps(CO1);					\
	_mm256_storeu_ps(CO1      , ymm4);				\


/*******************************************************************************************/

#define INIT4x1()							\
	row0 = _mm_setzero_ps();					\

#define KERNEL4x1_SUB() 						\
	xmm0   = _mm_loadu_ps(AO);					\
	xmm2   =  _mm_broadcastss_ps(_mm_load_ss(BO));			\
	row0  += xmm0 * xmm2;						\
	BO    += 1;							\
	AO    += 4;


#define SAVE4x1(ALPHA)							\
	xmm0   = _mm_set1_ps(ALPHA);					\
	row0  *= xmm0;							\
	row0  += _mm_loadu_ps(CO1);					\
	_mm_storeu_ps(CO1      , row0);					\



/*******************************************************************************************/

#define INIT2x1()							\
	row0 = 0; row0b = 0;

#define KERNEL2x1_SUB()							\
	xmm0  = *(AO + 0);						\
	xmm1  = *(AO + 1);						\
	xmm2  = *(BO);							\
	row0 += xmm0 * xmm2;						\
	row0b += xmm1 * xmm2;						\
	BO += 1;							\
	AO += 2;


#define SAVE2x1(ALPHA)							\
	xmm0   = ALPHA;							\
	row0  *= xmm0;							\
	row0b  *= xmm0;							\
	*(CO1         ) += row0;					\
	*(CO1 +1      ) += row0b;					\


/*******************************************************************************************/

#define INIT1x1()							\
	row0 = 0;

#define KERNEL1x1_SUB()							\
	xmm0  = *(AO);							\
	xmm2  = *(BO);							\
	row0 += xmm0 * xmm2;						\
	BO += 1;							\
	AO += 1;


#define SAVE1x1(ALPHA)							\
	xmm0   = ALPHA;							\
	row0  *= xmm0;							\
	*(CO1         ) += row0;					\


/*******************************************************************************************/


/*************************************************************************************
* GEMM Kernel
*************************************************************************************/

int __attribute__ ((noinline))
CNAME(BLASLONG m, BLASLONG n, BLASLONG k, float alpha, float * __restrict A, float * __restrict B, float * __restrict C, BLASLONG ldc)
{
	unsigned long long M = m, N = n, K = k;
	if (M == 0)
		return 0;
	if (N == 0)
		return 0;
	if (K == 0)
		return 0;


	while (N >= 4) {
		float *CO1;
		float *AO;
		int i;
		// L8_10
		CO1 = C;
		C += 4 * ldc;

		AO = A;

		i = m;
		while (i >= 64) {
			float *BO;
			float *A1, *A2, *A3;
			// L8_11
			__m512 zmm0, zmm1, zmm2, zmm3, row0, zmm5, row1, zmm7, row2, row3, row0b, row1b, row2b, row3b, row0c, row1c, row2c, row3c, row0d, row1d, row2d, row3d;
			BO = B;
			int kloop = K;

			A1 = AO + 16 * K;
			A2 = A1 + 16 * K;
			A3 = A2 + 16 * K;
	
			INIT64x4()

			while (kloop > 0) {
				// L12_17
				KERNEL64x4_SUB()
				kloop--;
			}
			// L8_19
			SAVE64x4(alpha)
			CO1 += 64;
			AO += 48 * K;
	
			i -= 64;
		}
		while (i >= 32) {
			float *BO;
			float *A1;
			// L8_11
			__m512 zmm0, zmm1, zmm2, zmm3, row0, row1, row2, row3, row0b, row1b, row2b, row3b;
			BO = B;
			int kloop = K;

			A1 = AO + 16 * K;
	
			INIT32x4()

			while (kloop > 0) {
				// L12_17
				KERNEL32x4_SUB()
				kloop--;
			}
			// L8_19
			SAVE32x4(alpha)
			CO1 += 32;
			AO += 16 * K;
	
			i -= 32;
		}
		while (i >= 16) {
			float *BO;
			// L8_11
			__m512 zmm0, zmm2, zmm3, row0, row1, row2, row3;
			BO = B;
			int kloop = K;
	
			INIT16x4()

			while (kloop > 0) {
				// L12_17
				KERNEL16x4_SUB()
				kloop--;
			}
			// L8_19
			SAVE16x4(alpha)
			CO1 += 16;
	
			i -= 16;
		}
		while (i >= 8) {
			float *BO;
			// L8_11
			__m256 ymm0, ymm2, ymm3, ymm4, ymm6,ymm8,ymm10;
			BO = B;
			int kloop = K;
	
			INIT8x4()

			while (kloop > 0) {
				// L12_17
				KERNEL8x4_SUB()
				kloop--;
			}
			// L8_19
			SAVE8x4(alpha)
			CO1 += 8;
	
			i -= 8;
		}
		while (i >= 4) {
			// L8_11
			float *BO;
			__m128 xmm0, xmm2, xmm3, row0, row1, row2, row3;
			BO = B;
			int kloop = K;

			INIT4x4()
			// L8_16
			while (kloop > 0) {
				// L12_17
				KERNEL4x4_SUB()
				kloop--;
			}
			// L8_19
			SAVE4x4(alpha)
			CO1 += 4;

			i -= 4;
		}

/**************************************************************************
* Rest of M 
***************************************************************************/

		while (i >= 2) {
			float *BO;
			float xmm0, xmm1, xmm2, xmm3, row0, row0b, row1, row1b, row2, row2b, row3, row3b;
			BO = B;

			INIT2x4()
			int kloop = K;
			
			while (kloop > 0) {
				KERNEL2x4_SUB()
				kloop--;
			}
			SAVE2x4(alpha)
			CO1 += 2;
			i -= 2;
		}
			// L13_40
		while (i >= 1) {
			float *BO;
			float xmm0, xmm2, xmm3, row0, row1, row2, row3;
			int kloop = K;
			BO = B;
			INIT1x4()
				
			while (kloop > 0) {
				KERNEL1x4_SUB()
				kloop--;
			}
			SAVE1x4(alpha)
			CO1 += 1;
			i -= 1;
		}
			
		B += K * 4;
		N -= 4;
	}

/**************************************************************************************************/

		// L8_0
	while (N >= 2) {
		float *CO1;
		float *AO;
		int i;
		// L8_10
		CO1 = C;
		C += 2 * ldc;

		AO = A;

		i = m;
		while (i >= 16) {
			float *BO;

			// L8_11
			__m512 zmm0, zmm2, zmm3, row0, row1;
			BO = B;
			int kloop = K;
	
			INIT16x2()

			while (kloop > 0) {
				// L12_17
				KERNEL16x2_SUB()
				kloop--;
			}
			// L8_19
			SAVE16x2(alpha)
			CO1 += 16;
	
			i -= 16;
		}
		while (i >= 8) {
			float *BO;
			__m256 ymm0, ymm2, ymm3, ymm4, ymm6;
			// L8_11
			BO = B;
			int kloop = K;

			INIT8x2()

			// L8_16
			while (kloop > 0) {
				// L12_17
				KERNEL8x2_SUB()
				kloop--;
			}
			// L8_19
			SAVE8x2(alpha)
			CO1 += 8;

			i-=8;
		}

		while (i >= 4) {
			float *BO;
			__m128 xmm0, xmm2, xmm3, row0, row1;
			// L8_11
			BO = B;
			int kloop = K;
	
			INIT4x2()

			// L8_16
			while (kloop > 0) {
				// L12_17
				KERNEL4x2_SUB()
				kloop--;
			}
			// L8_19
			SAVE4x2(alpha)
			CO1 += 4;
	
			i-=4;
		}

/**************************************************************************
* Rest of M 
***************************************************************************/

		while (i >= 2) {
			float *BO;
			float xmm0, xmm1, xmm2, xmm3, row0, row0b, row1, row1b;
			int kloop = K;
			BO = B;

			INIT2x2()
				
			while (kloop > 0) {
				KERNEL2x2_SUB()
				kloop--;
			}
			SAVE2x2(alpha)
			CO1 += 2;
			i -= 2;
		}
			// L13_40
		while (i >= 1) {
			float *BO;
			float xmm0, xmm2, xmm3, row0, row1;
			int kloop = K;
			BO = B;

			INIT1x2()
					
			while (kloop > 0) {
				KERNEL1x2_SUB()
				kloop--;
			}
			SAVE1x2(alpha)
			CO1 += 1;
			i -= 1;
		}
			
		B += K * 2;
		N -= 2;
	}

		// L8_0
	while (N >= 1) {
		// L8_10
		float *CO1;
		float *AO;
		int i;

		CO1 = C;
		C += ldc;

		AO = A;

		i = m;
		while (i >= 16) {
			float *BO;
			__m512 zmm0, zmm2, row0;
			// L8_11
			BO = B;
			int kloop = K;

			INIT16x1()
			// L8_16
			while (kloop > 0) {
				// L12_17
				KERNEL16x1_SUB()
				kloop--;
			}
			// L8_19
			SAVE16x1(alpha)
			CO1 += 16;

			i-= 16;
		}
		while (i >= 8) {
			float *BO;
			__m256 ymm0, ymm2, ymm4;
			// L8_11
			BO = B;
			int kloop = K;

			INIT8x1()
			// L8_16
			while (kloop > 0) {
				// L12_17
				KERNEL8x1_SUB()
				kloop--;
			}
			// L8_19
			SAVE8x1(alpha)
			CO1 += 8;

			i-= 8;
		}
		while (i >= 4) {
			float *BO;
			__m128 xmm0, xmm2, row0;
			// L8_11
			BO = B;
			int kloop = K;

			INIT4x1()
			// L8_16
			while (kloop > 0) {
				// L12_17
				KERNEL4x1_SUB()
				kloop--;
			}
			// L8_19
			SAVE4x1(alpha)
			CO1 += 4;

			i-= 4;
		}

/**************************************************************************
* Rest of M 
***************************************************************************/

		while (i >= 2) {
			float *BO;
			float xmm0, xmm1, xmm2, row0, row0b;
			int kloop = K;
			BO = B;

			INIT2x1()
				
			while (kloop > 0) {
				KERNEL2x1_SUB()
				kloop--;
			}
			SAVE2x1(alpha)
			CO1 += 2;
			i -= 2;
		}
				// L13_40
		while (i >= 1) {
			float *BO;
			float xmm0, xmm2, row0;
			int kloop = K;

			BO = B;
			INIT1x1()
				

			while (kloop > 0) {
				KERNEL1x1_SUB()
				kloop--;
			}
			SAVE1x1(alpha)
			CO1 += 1;
			i -= 1;
		}
			
		B += K * 1;
		N -= 1;
	}


	return 0;
}

#include "sgemm_direct_skylakex.c"
