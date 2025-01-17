/*********************************************************************/
/* Copyright 2009, 2010 The University of Texas at Austin.           */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include "common.h"

#include <immintrin.h>

int CNAME(BLASLONG m, BLASLONG n, BLASLONG dummy1, FLOAT beta,
	  IFLOAT *dummy2, BLASLONG dummy3, IFLOAT *dummy4, BLASLONG dummy5,
	  FLOAT *c, BLASLONG ldc){

  BLASLONG i, j;
  FLOAT *c_offset1, *c_offset;
  FLOAT ctemp1, ctemp2, ctemp3, ctemp4;
  FLOAT ctemp5, ctemp6, ctemp7, ctemp8;

  /* fast path.. just zero the whole matrix */
  if (m == ldc && beta == ZERO) {
	memset(c, 0, m * n * sizeof(FLOAT));
	return 0;
  }

  if (n == 0 || m == 0)
	return 0;

  c_offset = c;

  if (beta == ZERO){

    j = n;
    do {
      c_offset1 = c_offset;
      c_offset += ldc;

      i = m;
#ifdef __AVX2__
      while (i >= 32) {
#ifdef __AVX512CD__
	  __m512 z_zero = _mm512_setzero_ps();
	  _mm512_storeu_ps(c_offset1, z_zero);
	  _mm512_storeu_ps(c_offset1 + 16, z_zero);
#else
	  __m256 y_zero = _mm256_setzero_ps();
	  _mm256_storeu_ps(c_offset1, y_zero);
	  _mm256_storeu_ps(c_offset1 + 8, y_zero);
	  _mm256_storeu_ps(c_offset1 + 16, y_zero);
	  _mm256_storeu_ps(c_offset1 + 24, y_zero);
#endif
	  c_offset1 += 32;
	  i -= 32;
      }
      while (i >= 8) {
	    __m256 y_zero = _mm256_setzero_ps();
	  _mm256_storeu_ps(c_offset1, y_zero);
	  c_offset1 += 8;
	  i -= 8;
      }
#endif
      while (i > 0) {
	  *c_offset1 = ZERO;
	  c_offset1 ++;
	  i --;
      }
      j --;
    } while (j > 0);

  } else {

    j = n;
    do {
      c_offset1 = c_offset;
      c_offset += ldc;

      i = (m >> 3);
      if (i > 0){
	do {
	  ctemp1 = *(c_offset1 + 0);
	  ctemp2 = *(c_offset1 + 1);
	  ctemp3 = *(c_offset1 + 2);
	  ctemp4 = *(c_offset1 + 3);
	  ctemp5 = *(c_offset1 + 4);
	  ctemp6 = *(c_offset1 + 5);
	  ctemp7 = *(c_offset1 + 6);
	  ctemp8 = *(c_offset1 + 7);

	  ctemp1 *= beta;
	  ctemp2 *= beta;
	  ctemp3 *= beta;
	  ctemp4 *= beta;
	  ctemp5 *= beta;
	  ctemp6 *= beta;
	  ctemp7 *= beta;
	  ctemp8 *= beta;

	  *(c_offset1 + 0) = ctemp1;
	  *(c_offset1 + 1) = ctemp2;
	  *(c_offset1 + 2) = ctemp3;
	  *(c_offset1 + 3) = ctemp4;
	  *(c_offset1 + 4) = ctemp5;
	  *(c_offset1 + 5) = ctemp6;
	  *(c_offset1 + 6) = ctemp7;
	  *(c_offset1 + 7) = ctemp8;
	  c_offset1 += 8;
	  i --;
	} while (i > 0);
      }

      i = (m & 7);
      if (i > 0){
	do {
	  ctemp1 = *c_offset1;
	  ctemp1 *= beta;
	  *c_offset1 = ctemp1;
	  c_offset1 ++;
	  i --;
	} while (i > 0);
      }
      j --;
    } while (j > 0);

  }
  return 0;
};
