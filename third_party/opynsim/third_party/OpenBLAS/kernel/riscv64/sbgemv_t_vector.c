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

#include "common.h"

#define FLOAT_V_T               vfloat32m8_t
#define FLOAT_V_T_M1            vfloat32m1_t
#define VLEV_FLOAT              RISCV_RVV(vle32_v_f32m8)
#define VLSEV_FLOAT             RISCV_RVV(vlse32_v_f32m8)

#define VSETVL(n)               RISCV_RVV(vsetvl_e16m4)(n)

#if defined(HFLOAT16)
#define IFLOAT_V_T              vfloat16m4_t
#define VLEV_IFLOAT             RISCV_RVV(vle16_v_f16m4)
#define VLSEV_IFLOAT            RISCV_RVV(vlse16_v_f16m4)
#define VFMACCVV_FLOAT(a,b,c,d) RISCV_RVV(vfwmul_vv_f32m8)(b,c,d)
#else
#define IFLOAT_V_T              vbfloat16m4_t
#define VLEV_IFLOAT             RISCV_RVV(vle16_v_bf16m4)
#define VLSEV_IFLOAT            RISCV_RVV(vlse16_v_bf16m4)
#define VFMACCVV_FLOAT          RISCV_RVV(vfwmaccbf16_vv_f32m8)
#endif

#ifdef RISCV_0p10_INTRINSICS
#define VFREDSUM_FLOAT(va, vb, gvl) vfredusum_vs_f32m8_f32m1(v_res, va, vb, gvl)
#else
#define VFREDSUM_FLOAT          RISCV_RVV(vfredusum_vs_f32m8_f32m1)
#endif
#define VFMVVF_FLOAT            RISCV_RVV(vfmv_v_f_f32m8)
#define VFMVVF_FLOAT_M1         RISCV_RVV(vfmv_v_f_f32m1)

int CNAME(BLASLONG m, BLASLONG n, FLOAT alpha, IFLOAT *a, BLASLONG lda, IFLOAT *x, BLASLONG inc_x, FLOAT beta, FLOAT *y, BLASLONG inc_y)
{
    BLASLONG i = 0, j = 0, k = 0;
    BLASLONG ix = 0, iy = 0;
#if defined(HFLOAT16)
    _Float16 *a_ptr = (_Float16 *)(a);
    _Float16 *x_ptr = (_Float16 *)(x);
#else
    __bf16 *a_ptr = (__bf16 *)(a);
    __bf16 *x_ptr = (__bf16 *)(x);
#endif
    FLOAT temp;

    IFLOAT_V_T va, vx;
#if !defined(HFLOAT16)
    FLOAT_V_T vz;
#endif
    FLOAT_V_T vr;
    BLASLONG gvl = 0;
    FLOAT_V_T_M1 v_res;

    if (inc_x == 1) {
        for (i = 0; i < n; i++) {
            v_res = VFMVVF_FLOAT_M1(0, 1);
            gvl = VSETVL(m);
            j = 0;
#if !defined(HFLOAT16)
            vz = VFMVVF_FLOAT(0, gvl);
#endif
            for (k = 0; k < m/gvl; k++) {
                va = VLEV_IFLOAT(&a_ptr[j], gvl);
                vx = VLEV_IFLOAT(&x_ptr[j], gvl);
                vr = VFMACCVV_FLOAT(vz, va, vx, gvl);           // could vfmacc here and reduce outside loop
                v_res = VFREDSUM_FLOAT(vr, v_res, gvl);         // but that reordering diverges far enough from scalar path to make tests fail
                j += gvl;
            }
            if (j < m) {
                gvl = VSETVL(m-j);
                va = VLEV_IFLOAT(&a_ptr[j], gvl);
                vx = VLEV_IFLOAT(&x_ptr[j], gvl);
                vr = VFMACCVV_FLOAT(vz, va, vx, gvl);
                v_res = VFREDSUM_FLOAT(vr, v_res, gvl);
            }
            temp = (FLOAT)EXTRACT_FLOAT(v_res);
            y[iy] = y[iy] * beta + alpha * temp;

            iy += inc_y;
            a_ptr += lda;
        }
    } else {
        BLASLONG stride_x = inc_x * sizeof(IFLOAT);
        for (i = 0; i < n; i++) {
            v_res = VFMVVF_FLOAT_M1(0, 1);
            gvl = VSETVL(m);
            j = 0;
            ix = 0;
#if !defined(HFLOAT16)
            vz = VFMVVF_FLOAT(0, gvl);
#endif
            for (k = 0; k < m/gvl; k++) {
                va = VLEV_IFLOAT(&a_ptr[j], gvl);
                vx = VLSEV_IFLOAT(&x_ptr[ix], stride_x, gvl);
                vr = VFMACCVV_FLOAT(vz, va, vx, gvl);
                v_res = VFREDSUM_FLOAT(vr, v_res, gvl);
                j += gvl;
                ix += inc_x * gvl;
            }
            if (j < m) {
                gvl = VSETVL(m-j);
                va = VLEV_IFLOAT(&a_ptr[j], gvl);
                vx = VLSEV_IFLOAT(&x_ptr[ix], stride_x, gvl);
                vr = VFMACCVV_FLOAT(vz, va, vx, gvl);
                v_res = VFREDSUM_FLOAT(vr, v_res, gvl);
            }
            temp = (FLOAT)EXTRACT_FLOAT(v_res);
            y[iy] = y[iy] * beta + alpha * temp;

            iy += inc_y;
            a_ptr += lda;
        }
    }

    return (0);
}
