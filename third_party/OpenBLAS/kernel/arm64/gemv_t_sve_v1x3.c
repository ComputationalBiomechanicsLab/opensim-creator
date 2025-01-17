/***************************************************************************
Copyright (c) 2024, The OpenBLAS Project
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
      derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include <arm_sve.h>

#include "common.h"

#ifdef DOUBLE
#define SV_COUNT svcntd
#define SV_TYPE svfloat64_t
#define SV_TRUE svptrue_b64
#define SV_WHILE svwhilelt_b64_s64
#define SV_DUP svdup_f64
#else
#define SV_COUNT svcntw
#define SV_TYPE svfloat32_t
#define SV_TRUE svptrue_b32
#define SV_WHILE svwhilelt_b32_s64
#define SV_DUP svdup_f32
#endif

int CNAME(BLASLONG m, BLASLONG n, BLASLONG dummy1, FLOAT alpha, FLOAT *a,
          BLASLONG lda, FLOAT *x, BLASLONG inc_x, FLOAT *y, BLASLONG inc_y,
          FLOAT *buffer)
{
  BLASLONG i;
  BLASLONG ix,iy;
  BLASLONG j;
  FLOAT *a_ptr;
  FLOAT temp;

  iy = 0;

  if (inc_x == 1) {
    BLASLONG width = (n + 3 - 1) / 3;

    FLOAT *a0_ptr = a + lda * width * 0;
    FLOAT *a1_ptr = a + lda * width * 1;
    FLOAT *a2_ptr = a + lda * width * 2;

    FLOAT *y0_ptr = y + inc_y * width * 0;
    FLOAT *y1_ptr = y + inc_y * width * 1;
    FLOAT *y2_ptr = y + inc_y * width * 2;

    for (j = 0; j < width; j++) {
      svbool_t pg00 = ((j + width * 0) < n) ? SV_TRUE() : svpfalse();
      svbool_t pg01 = ((j + width * 1) < n) ? SV_TRUE() : svpfalse();
      svbool_t pg02 = ((j + width * 2) < n) ? SV_TRUE() : svpfalse();

      SV_TYPE temp00_vec = SV_DUP(0.0);
      SV_TYPE temp01_vec = SV_DUP(0.0);
      SV_TYPE temp02_vec = SV_DUP(0.0);

      i = 0;
      BLASLONG sve_size = SV_COUNT();
      while ((i + sve_size * 1 - 1) < m) {
        SV_TYPE x0_vec = svld1_vnum(SV_TRUE(), x + i, 0);

        SV_TYPE a00_vec = svld1_vnum(pg00, a0_ptr + i, 0);
        SV_TYPE a01_vec = svld1_vnum(pg01, a1_ptr + i, 0);
        SV_TYPE a02_vec = svld1_vnum(pg02, a2_ptr + i, 0);

        temp00_vec = svmla_m(pg00, temp00_vec, a00_vec, x0_vec);
        temp01_vec = svmla_m(pg01, temp01_vec, a01_vec, x0_vec);
        temp02_vec = svmla_m(pg02, temp02_vec, a02_vec, x0_vec);

        i += sve_size * 1;
      }

      if (i < m) {
        svbool_t pg0 = SV_WHILE(i + sve_size * 0, m);

        pg00 = svand_z(SV_TRUE(), pg0, pg00);
        pg01 = svand_z(SV_TRUE(), pg0, pg01);
        pg02 = svand_z(SV_TRUE(), pg0, pg02);

        SV_TYPE x0_vec = svld1_vnum(pg0, x + i, 0);

        SV_TYPE a00_vec = svld1_vnum(pg00, a0_ptr + i, 0);
        SV_TYPE a01_vec = svld1_vnum(pg01, a1_ptr + i, 0);
        SV_TYPE a02_vec = svld1_vnum(pg02, a2_ptr + i, 0);

        temp00_vec = svmla_m(pg00, temp00_vec, a00_vec, x0_vec);
        temp01_vec = svmla_m(pg01, temp01_vec, a01_vec, x0_vec);
        temp02_vec = svmla_m(pg02, temp02_vec, a02_vec, x0_vec);
      }

      if ((j + width * 0) < n) {
        temp = svaddv(SV_TRUE(), temp00_vec);
        y0_ptr[iy] += alpha * temp;
      }
      if ((j + width * 1) < n) {
        temp = svaddv(SV_TRUE(), temp01_vec);
        y1_ptr[iy] += alpha * temp;
      }
      if ((j + width * 2) < n) {
        temp = svaddv(SV_TRUE(), temp02_vec);
        y2_ptr[iy] += alpha * temp;
      }
      iy += inc_y;

      a0_ptr += lda;
      a1_ptr += lda;
      a2_ptr += lda;
    }

    return(0);
  }

  a_ptr = a;
  for (j = 0; j < n; j++) {
    temp = 0.0;
    ix = 0;
    for (i = 0; i < m; i++) {
      temp += a_ptr[i] * x[ix];
      ix += inc_x;
    }
    y[iy] += alpha * temp;
    iy += inc_y;
    a_ptr += lda;
  }
  return(0);
}
