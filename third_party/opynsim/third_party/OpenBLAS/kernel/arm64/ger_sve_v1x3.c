/***************************************************************************
Copyright (c) 2025, The OpenBLAS Project
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
ARE DISCLAIMED. IN NO EVENT SHALL THE OPENBLAS PROJECT OR CONTRIBUTORS BE
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

int CNAME(BLASLONG m, BLASLONG n, BLASLONG dummy1, FLOAT alpha,
	 FLOAT *x, BLASLONG incx,
	 FLOAT *y, BLASLONG incy,
	 FLOAT *a, BLASLONG lda, FLOAT *buffer){

  FLOAT *X    = x;

  if (incx != 1) {
    X = buffer;
    COPY_K(m, x, incx, X, 1);
  }

  BLASLONG width = (n + 3 - 1) / 3;
  BLASLONG i, j;
  BLASLONG sve_size = SV_COUNT();

  FLOAT *y0_ptr = y + incy * width * 0;
  FLOAT *y1_ptr = y + incy * width * 1;
  FLOAT *y2_ptr = y + incy * width * 2;

  for (j = 0; j < width; j++) {
    svbool_t pg00 = (j + width * 0 < n) ? SV_TRUE() : svpfalse();
    svbool_t pg01 = (j + width * 1 < n) ? SV_TRUE() : svpfalse();
    svbool_t pg02 = (j + width * 2 < n) ? SV_TRUE() : svpfalse();

    SV_TYPE temp0_vec = (j + width * 0 < n) ? SV_DUP(alpha * *y0_ptr) : SV_DUP(0.0);
    SV_TYPE temp1_vec = (j + width * 1 < n) ? SV_DUP(alpha * *y1_ptr) : SV_DUP(0.0);
    SV_TYPE temp2_vec = (j + width * 2 < n) ? SV_DUP(alpha * *y2_ptr) : SV_DUP(0.0);

    FLOAT *x_ptr = X;
    FLOAT *a0_ptr = a + lda * width * 0 + lda * j;
    FLOAT *a1_ptr = a + lda * width * 1 + lda * j;
    FLOAT *a2_ptr = a + lda * width * 2 + lda * j;

    i = 0;
    while (i + sve_size * 1 - 1 < m) {
      SV_TYPE x0_vec = svld1_vnum(SV_TRUE(), x_ptr, 0);

      SV_TYPE a00_vec = svld1_vnum(pg00, a0_ptr, 0);
      SV_TYPE a01_vec = svld1_vnum(pg01, a1_ptr, 0);
      SV_TYPE a02_vec = svld1_vnum(pg02, a2_ptr, 0);

      a00_vec = svmla_x(pg00, a00_vec, temp0_vec, x0_vec);
      a01_vec = svmla_x(pg01, a01_vec, temp1_vec, x0_vec);
      a02_vec = svmla_x(pg02, a02_vec, temp2_vec, x0_vec);

      svst1_vnum(pg00, a0_ptr, 0, a00_vec);
      svst1_vnum(pg01, a1_ptr, 0, a01_vec);
      svst1_vnum(pg02, a2_ptr, 0, a02_vec);

      i += sve_size * 1;
      x_ptr += sve_size * 1;
      a0_ptr += sve_size * 1;
      a1_ptr += sve_size * 1;
      a2_ptr += sve_size * 1;
    }

    if (i < m) {
      svbool_t pg0 = SV_WHILE(i + sve_size * 0, m);

      pg00 = svand_z(SV_TRUE(), pg0, pg00);
      pg01 = svand_z(SV_TRUE(), pg0, pg01);
      pg02 = svand_z(SV_TRUE(), pg0, pg02);

      SV_TYPE x0_vec = svld1_vnum(pg0, x_ptr, 0);

      SV_TYPE a00_vec = svld1_vnum(pg00, a0_ptr, 0);
      SV_TYPE a01_vec = svld1_vnum(pg01, a1_ptr, 0);
      SV_TYPE a02_vec = svld1_vnum(pg02, a2_ptr, 0);

      a00_vec = svmla_x(pg00, a00_vec, temp0_vec, x0_vec);
      a01_vec = svmla_x(pg01, a01_vec, temp1_vec, x0_vec);
      a02_vec = svmla_x(pg02, a02_vec, temp2_vec, x0_vec);

      svst1_vnum(pg00, a0_ptr, 0, a00_vec);
      svst1_vnum(pg01, a1_ptr, 0, a01_vec);
      svst1_vnum(pg02, a2_ptr, 0, a02_vec);
    }

    y0_ptr += incy;
    y1_ptr += incy;
    y2_ptr += incy;
  }
  
  return 0;
}
