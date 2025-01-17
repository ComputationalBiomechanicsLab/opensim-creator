/***************************************************************************
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
*****************************************************************************/

/**************************************************************************************
* 2013/09/14 Saar
*	 BLASTEST float		: FAIL
* 	 BLASTEST double	: FAIL
* 	 CTEST			: OK
* 	 TEST			: OK
*
**************************************************************************************/

#include "common.h"

OPENBLAS_COMPLEX_FLOAT CNAME(BLASLONG n, FLOAT *x, BLASLONG inc_x, FLOAT *y, BLASLONG inc_y)

{
	BLASLONG i=0;
	BLASLONG ix=0,iy=0;
	FLOAT dot[2];
	OPENBLAS_COMPLEX_FLOAT result;
	BLASLONG inc_x2;
	BLASLONG inc_y2;

	dot[0]=0.0;
	dot[1]=0.0;
#if !defined(__PPC__) && !defined(__SunOS) && !defined(__PGI)
	CREAL(result) = 0.0 ;
	CIMAG(result) = 0.0 ;
#else
	result = OPENBLAS_MAKE_COMPLEX_FLOAT(0.0,0.0);
#endif
	if ( n < 1 )  return(result);

	inc_x2 = 2 * inc_x ;
	inc_y2 = 2 * inc_y ;

	while(i < n)
	{
#if !defined(CONJ)
		dot[0] += ( x[ix]   * y[iy] - x[ix+1] * y[iy+1] ) ;
		dot[1] += ( x[ix+1] * y[iy] + x[ix]   * y[iy+1] ) ;
#else
		dot[0] += ( x[ix]   * y[iy] + x[ix+1] * y[iy+1] ) ;
		dot[1] -= ( x[ix+1] * y[iy] - x[ix]   * y[iy+1] ) ;
#endif
		ix  += inc_x2 ;
		iy  += inc_y2 ;
		i++ ;

	}
#if !defined(__PPC__)	&& !defined(__SunOS) && !defined(__PGI)
        CREAL(result) = dot[0];
	CIMAG(result) = dot[1];
#else
	result = OPENBLAS_MAKE_COMPLEX_FLOAT(dot[0],dot[1]);
#endif
	return(result);

}


