/*********************************************************************/
/* Copyright 2009, 2010 The University of Texas at Austin.           */
/* Copyright 2023 The OpenBLAS Project                               */
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

#include <stdio.h>
#include "common.h"
#include "arm_sve.h"

int CNAME(BLASLONG m, BLASLONG n, FLOAT *a, BLASLONG lda, BLASLONG offset, FLOAT *b){

  BLASLONG i, ii, jj;

  FLOAT *ao;

  lda *= 2;

  jj = offset;
#ifdef DOUBLE
  int64_t js = 0;
  svint64_t index = svindex_s64(0LL, lda);
  svbool_t pn = svwhilelt_b64((uint64_t)js, (uint64_t)n);
  int n_active = svcntp_b64(svptrue_b64(), pn);
#else
  int32_t N = n;
  int32_t js = 0;
  svint32_t index = svindex_s32(0, lda);
  svbool_t pn = svwhilelt_b32((uint32_t)js, (uint32_t)N);
  int n_active = svcntp_b32(svptrue_b32(), pn);
#endif
  do {

    ao = a;

    i = 0;
    ii = 0;
    do {

      if (ii == jj) {
        for (int j = 0; j < n_active; j++) {
          compinv(b + 2*j * n_active + 2*j, *(ao + j * lda + 2*j), *(ao + j * lda + 2*j+1));
          //*(b + j * n_active + j) = INV(*(ao + j * lda + j));
          for (int k = j+1; k < n_active; k++) {
            *(b + 2*j * n_active + 2*k) = *(ao + k * lda + 2*j);
            *(b + 2*j * n_active + 2*k + 1) = *(ao + k * lda + 2*j + 1);
          }
        }
        ao += n_active * 2;
        b += n_active * n_active * 2;
        i += n_active;
        ii += n_active;
      } else {
        if (ii < jj) {
#ifdef DOUBLE
          svfloat64_t aj_vec_real = svld1_gather_index(pn, ao, index);
          svfloat64_t aj_vec_imag = svld1_gather_index(pn, ao+1, index);
#else
          svfloat32_t aj_vec_real = svld1_gather_index(pn, ao, index);
          svfloat32_t aj_vec_imag = svld1_gather_index(pn, ao+1, index);
#endif
          svst2(pn, b, svcreate2(aj_vec_real, aj_vec_imag));
        }
        ao += 2;
        b += n_active * 2;
        i++;
        ii++;
      }
    } while (i < m);


    a += n_active * lda;
    jj += n_active;

    js += n_active;
#ifdef DOUBLE
    pn = svwhilelt_b64((uint64_t)js, (uint64_t)n);
    n_active = svcntp_b64(svptrue_b64(), pn);
  } while (svptest_any(svptrue_b64(), pn));
#else
    pn = svwhilelt_b32((uint32_t)js, (uint32_t)N);
    n_active = svcntp_b32(svptrue_b32(), pn);
  } while (svptest_any(svptrue_b32(), pn));
#endif

return 0;
}
