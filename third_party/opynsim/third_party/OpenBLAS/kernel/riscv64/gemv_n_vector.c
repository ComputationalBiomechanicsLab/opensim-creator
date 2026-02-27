/***************************************************************************
Copyright (c) 2020, The OpenBLAS Project
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

#include "common.h"

#if !defined(DOUBLE)
#define VSETVL(n)               RISCV_RVV(vsetvl_e32m8)(n)
#define FLOAT_V_T               vfloat32m8_t
#define VLEV_FLOAT              RISCV_RVV(vle32_v_f32m8)
#define VLSEV_FLOAT             RISCV_RVV(vlse32_v_f32m8)
#define VSEV_FLOAT              RISCV_RVV(vse32_v_f32m8)
#define VSSEV_FLOAT             RISCV_RVV(vsse32_v_f32m8)
#define VFMACCVF_FLOAT          RISCV_RVV(vfmacc_vf_f32m8)
#else
#define VSETVL(n)               RISCV_RVV(vsetvl_e64m8)(n)
#define FLOAT_V_T               vfloat64m8_t
#define VLEV_FLOAT              RISCV_RVV(vle64_v_f64m8)
#define VLSEV_FLOAT             RISCV_RVV(vlse64_v_f64m8)
#define VSEV_FLOAT              RISCV_RVV(vse64_v_f64m8)
#define VSSEV_FLOAT             RISCV_RVV(vsse64_v_f64m8)
#define VFMACCVF_FLOAT          RISCV_RVV(vfmacc_vf_f64m8)
#endif

int CNAME(BLASLONG m, BLASLONG n, BLASLONG dummy1, FLOAT alpha, FLOAT *a, BLASLONG lda, FLOAT *x, BLASLONG inc_x, FLOAT *y, BLASLONG inc_y, FLOAT *buffer)
{
    if (n < 0) return(0);

    FLOAT *a_ptr, *y_ptr, temp;
    BLASLONG i, j, vl;
    FLOAT_V_T va, vy;

    if (inc_y == 1) {
        for (j = 0; j < n; j++) {
            temp = alpha * x[0];
            y_ptr = y;
            a_ptr = a;
            for (i = m; i > 0; i -= vl) {
                vl = VSETVL(i);
                vy = VLEV_FLOAT(y_ptr, vl);
                va = VLEV_FLOAT(a_ptr, vl);
                vy = VFMACCVF_FLOAT(vy, temp, va, vl);
                VSEV_FLOAT(y_ptr, vy, vl);
                y_ptr += vl;
                a_ptr += vl;
            }
            x += inc_x;
            a += lda;
        }
    } else {
        BLASLONG stride_y = inc_y * sizeof(FLOAT);
        for (j = 0; j < n; j++) {
            temp = alpha * x[0];
            y_ptr = y;
            a_ptr = a;
            for (i = m; i > 0; i -= vl) {
                vl = VSETVL(i);
                vy = VLSEV_FLOAT(y_ptr, stride_y, vl);
                va = VLEV_FLOAT(a_ptr, vl);
                vy = VFMACCVF_FLOAT(vy, temp, va, vl);
                VSSEV_FLOAT(y_ptr, stride_y, vy, vl);
                y_ptr += vl * inc_y;
                a_ptr += vl;
            }
            x += inc_x;
            a += lda;
        }
    }
    return(0);
}
