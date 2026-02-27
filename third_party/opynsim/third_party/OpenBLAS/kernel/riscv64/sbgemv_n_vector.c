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

#define FLOAT_V_T               vfloat32m8_t
#define VLEV_FLOAT              RISCV_RVV(vle32_v_f32m8)
#define VLSEV_FLOAT             RISCV_RVV(vlse32_v_f32m8)
#define VSEV_FLOAT              RISCV_RVV(vse32_v_f32m8)
#define VSSEV_FLOAT             RISCV_RVV(vsse32_v_f32m8)
#define VFMULVF_FLOAT           RISCV_RVV(vfmul_vf_f32m8)
#define VFMVVF_FLOAT            RISCV_RVV(vfmv_v_f_f32m8)

#define VSETVL(n)               RISCV_RVV(vsetvl_e16m4)(n)

#if defined(HFLOAT16)
#define IFLOAT_V_T              vfloat16m4_t
#define VLEV_IFLOAT             RISCV_RVV(vle16_v_f16m4)
#define VFMACCVF_FLOAT          RISCV_RVV(vfwmacc_vf_f32m8)
#else
#define IFLOAT_V_T              vbfloat16m4_t
#define VLEV_IFLOAT             RISCV_RVV(vle16_v_bf16m4)
#define VFMACCVF_FLOAT          RISCV_RVV(vfwmaccbf16_vf_f32m8)
#endif

int CNAME(BLASLONG m, BLASLONG n, FLOAT alpha, IFLOAT *a, BLASLONG lda, IFLOAT *x, BLASLONG inc_x, FLOAT beta, FLOAT *y, BLASLONG inc_y)
{
    if (n < 0) return(0);

#if defined(HFLOAT16)
    _Float16 *a_ptr, *x_ptr, temp;
    x_ptr = (_Float16 *)(x);
#else
    __bf16 *a_ptr, *x_ptr, temp;
    x_ptr = (__bf16 *)(x);
#endif
    FLOAT *y_ptr;
    BLASLONG i, j, vl;
    IFLOAT_V_T va;
    FLOAT_V_T vy;

    y_ptr = y;
    if (inc_y == 1) {
        if (beta == 0.0) {
            for (i = m; i > 0; i -= vl) {
                vl = VSETVL(i);
                vy = VFMVVF_FLOAT(0.0, vl);
                VSEV_FLOAT(y_ptr, vy, vl);
                y_ptr += vl;
            }
        } else if (beta != 1.0) {
            for (i = m; i > 0; i -= vl) {
                vl = VSETVL(i);
                vy = VLEV_FLOAT(y_ptr, vl);
                vy = VFMULVF_FLOAT(vy, beta, vl);
                VSEV_FLOAT(y_ptr, vy, vl);
                y_ptr += vl;
            }
        }
        for (j = 0; j < n; j++) {
#if defined(HFLOAT16)
            temp = (_Float16)(alpha * (FLOAT)(x_ptr[0]));
            a_ptr = (_Float16 *)(a);
#else
            temp = (__bf16)(alpha * (FLOAT)(x_ptr[0]));
            a_ptr = (__bf16 *)(a);
#endif
            y_ptr = y;
            for (i = m; i > 0; i -= vl) {
                vl = VSETVL(i);
                vy = VLEV_FLOAT(y_ptr, vl);
                va = VLEV_IFLOAT(a_ptr, vl);
                vy = VFMACCVF_FLOAT(vy, temp, va, vl);
                VSEV_FLOAT(y_ptr, vy, vl);
                y_ptr += vl;
                a_ptr += vl;
            }
            x_ptr += inc_x;
            a += lda;
        }
    } else {
        BLASLONG stride_y = inc_y * sizeof(FLOAT);
        if (beta == 0.0) {
            for (i = m; i > 0; i -= vl) {
                vl = VSETVL(i);
                vy = VFMVVF_FLOAT(0.0, vl);
                VSSEV_FLOAT(y_ptr, stride_y, vy, vl);
                y_ptr += vl * inc_y;
            }
        } else if (beta != 1.0) {
            for (i = m; i > 0; i -= vl) {
                vl = VSETVL(i);
                vy = VLSEV_FLOAT(y_ptr, stride_y, vl);
                vy = VFMULVF_FLOAT(vy, beta, vl);
                VSSEV_FLOAT(y_ptr, stride_y, vy, vl);
                y_ptr += vl * inc_y;
            }
        }
        for (j = 0; j < n; j++) {
#if defined(HFLOAT16)
            temp = (_Float16)(alpha * (FLOAT)(x_ptr[0]));
            a_ptr = (_Float16 *)(a);
#else
            temp = (__bf16)(alpha * (FLOAT)(x_ptr[0]));
            a_ptr = (__bf16 *)(a);
#endif
            y_ptr = y;
            for (i = m; i > 0; i -= vl) {
                vl = VSETVL(i);
                vy = VLSEV_FLOAT(y_ptr, stride_y, vl);
                va = VLEV_IFLOAT(a_ptr, vl);
                vy = VFMACCVF_FLOAT(vy, temp, va, vl);
                VSSEV_FLOAT(y_ptr, stride_y, vy, vl);
                y_ptr += vl * inc_y;
                a_ptr += vl;
            }
            x_ptr += inc_x;
            a += lda;
        }
    }
    return(0);
}
