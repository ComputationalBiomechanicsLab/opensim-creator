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

#define VSETVL(n)               __riscv_vsetvl_e16m1(n)
#define VSETVL2(n)              __riscv_vsetvl_e16m2(n)
#define VSETVL4(n)              __riscv_vsetvl_e16m4(n)
#define VSETVL8(n)              __riscv_vsetvl_e16m8(n)
#if defined(HFLOAT16)
#define FLOAT_V_T               vfloat16m1_t
#define FLOAT_V2_T              vfloat16m2_t
#define FLOAT_V4_T              vfloat16m4_t
#define FLOAT_V8_T              vfloat16m8_t
#define FLOAT_VX2_T             vfloat16m1x2_t
#define FLOAT_VX4_T             vfloat16m1x4_t
#define FLOAT_VX8_T             vfloat16m1x8_t
#define FLOAT_VX24_T            vfloat16m4x2_t
#define FLOAT_VX42_T            vfloat16m2x4_t
#define VLSEG2_FLOAT            __riscv_vlse16_v_f16m2
#define VLSSEG2_FLOAT           __riscv_vlsseg2e16_v_f16m1x2
#define VLSSEG4_FLOAT           __riscv_vlsseg4e16_v_f16m1x4
#define VLSSEG8_FLOAT           __riscv_vlsseg8e16_v_f16m1x8
#define VGET_VX2                __riscv_vget_v_f16m1x2_f16m1
#define VGET_VX4                __riscv_vget_v_f16m1x4_f16m1
#define VGET_VX8                __riscv_vget_v_f16m1x8_f16m1
#define VSET_VX2                __riscv_vset_v_f16m4_f16m4x2
#define VSET_VX4                __riscv_vset_v_f16m2_f16m2x4
#define VSET_VX8                __riscv_vset_v_f16m1_f16m1x8
#define VLEV_FLOAT              __riscv_vle16_v_f16m1
#define VLEV_FLOAT2             __riscv_vle16_v_f16m2
#define VLEV_FLOAT4             __riscv_vle16_v_f16m4
#define VLEV_FLOAT8             __riscv_vle16_v_f16m8
#define VSEV_FLOAT              __riscv_vse16_v_f16m1
#define VSEV_FLOAT2             __riscv_vse16_v_f16m2
#define VSEV_FLOAT8             __riscv_vse16_v_f16m8
#define VSSEG2_FLOAT            __riscv_vsseg2e16_v_f16m4x2
#define VSSEG4_FLOAT            __riscv_vsseg4e16_v_f16m2x4
#define VSSEG8_FLOAT            __riscv_vsseg8e16_v_f16m1x8
#else
#define FLOAT_V_T               vbfloat16m1_t
#define FLOAT_V2_T              vbfloat16m2_t
#define FLOAT_V4_T              vbfloat16m4_t
#define FLOAT_V8_T              vbfloat16m8_t
#define FLOAT_VX2_T             vbfloat16m1x2_t
#define FLOAT_VX4_T             vbfloat16m1x4_t
#define FLOAT_VX8_T             vbfloat16m1x8_t
#define FLOAT_VX24_T            vbfloat16m4x2_t
#define FLOAT_VX42_T            vbfloat16m2x4_t
#define VLSEG2_FLOAT            __riscv_vlse16_v_bf16m2
#define VLSSEG2_FLOAT           __riscv_vlsseg2e16_v_bf16m1x2
#define VLSSEG4_FLOAT           __riscv_vlsseg4e16_v_bf16m1x4
#define VLSSEG8_FLOAT           __riscv_vlsseg8e16_v_bf16m1x8
#define VGET_VX2                __riscv_vget_v_bf16m1x2_bf16m1
#define VGET_VX4                __riscv_vget_v_bf16m1x4_bf16m1
#define VGET_VX8                __riscv_vget_v_bf16m1x8_bf16m1
#define VSET_VX2                __riscv_vset_v_bf16m4_bf16m4x2
#define VSET_VX4                __riscv_vset_v_bf16m2_bf16m2x4
#define VSET_VX8                __riscv_vset_v_bf16m1_bf16m1x8
#define VLEV_FLOAT              __riscv_vle16_v_bf16m1
#define VLEV_FLOAT2             __riscv_vle16_v_bf16m2
#define VLEV_FLOAT4             __riscv_vle16_v_bf16m4
#define VLEV_FLOAT8             __riscv_vle16_v_bf16m8
#define VSEV_FLOAT              __riscv_vse16_v_bf16m1
#define VSEV_FLOAT2             __riscv_vse16_v_bf16m2
#define VSEV_FLOAT8             __riscv_vse16_v_bf16m8
#define VSSEG2_FLOAT            __riscv_vsseg2e16_v_bf16m4x2
#define VSSEG4_FLOAT            __riscv_vsseg4e16_v_bf16m2x4
#define VSSEG8_FLOAT            __riscv_vsseg8e16_v_bf16m1x8
#endif

// Optimizes the implementation in ../generic/gemm_ncopy_16.c

int CNAME(BLASLONG m, BLASLONG n, IFLOAT *a, BLASLONG lda, IFLOAT *b)
{
    BLASLONG i, j;

    IFLOAT *a_offset;
    IFLOAT *a_offset1, *a_offset2, *a_offset3, *a_offset4;
    IFLOAT *a_offset5, *a_offset6, *a_offset7, *a_offset8;
    IFLOAT *b_offset;

    FLOAT_V_T v1, v2, v3, v4, v5, v6, v7, v8;
    FLOAT_V_T v9, v10, v11, v12, v13, v14, v15, v16;
    FLOAT_V2_T v21, v22, v23, v24;
    FLOAT_V4_T v41, v42;
    FLOAT_V8_T v81;

    FLOAT_VX2_T vx2, vx21;
    FLOAT_VX4_T vx4, vx41;
    FLOAT_VX8_T vx8, vx81;
    FLOAT_VX42_T vx24;
    FLOAT_VX24_T vx42;

    size_t vl;

    //fprintf(stderr, "gemm_ncopy_16 m=%ld n=%ld lda=%ld\n", m, n, lda);

    a_offset = a;
    b_offset = b;

    for (j = (n >> 4); j > 0; j--) {
        vl = VSETVL(8);

        a_offset1  = a_offset;
        a_offset2  = a_offset1  + lda * 8;
        a_offset += 16 * lda;

        for (i = m >> 3; i > 0; i--) {
            vx8 = VLSSEG8_FLOAT(a_offset1, lda * sizeof(IFLOAT), vl);
            vx81 = VLSSEG8_FLOAT(a_offset2, lda * sizeof(IFLOAT), vl);

            v1 = VGET_VX8(vx8, 0);
            v2 = VGET_VX8(vx8, 1);
            v3 = VGET_VX8(vx8, 2);
            v4 = VGET_VX8(vx8, 3);
            v5 = VGET_VX8(vx8, 4);
            v6 = VGET_VX8(vx8, 5);
            v7 = VGET_VX8(vx8, 6);
            v8 = VGET_VX8(vx8, 7);
            v9 = VGET_VX8(vx81, 0);
            v10 = VGET_VX8(vx81, 1);
            v11 = VGET_VX8(vx81, 2);
            v12 = VGET_VX8(vx81, 3);
            v13 = VGET_VX8(vx81, 4);
            v14 = VGET_VX8(vx81, 5);
            v15 = VGET_VX8(vx81, 6);
            v16 = VGET_VX8(vx81, 7);

            VSEV_FLOAT(b_offset, v1, vl);
            VSEV_FLOAT(b_offset + 8, v9, vl);
            VSEV_FLOAT(b_offset + 16, v2, vl);
            VSEV_FLOAT(b_offset + 24, v10, vl);
            VSEV_FLOAT(b_offset + 32, v3, vl);
            VSEV_FLOAT(b_offset + 40, v11, vl);
            VSEV_FLOAT(b_offset + 48, v4, vl);
            VSEV_FLOAT(b_offset + 56, v12, vl);
            VSEV_FLOAT(b_offset + 64, v5, vl);
            VSEV_FLOAT(b_offset + 72, v13, vl);
            VSEV_FLOAT(b_offset + 80, v6, vl);
            VSEV_FLOAT(b_offset + 88, v14, vl);
            VSEV_FLOAT(b_offset + 96, v7, vl);
            VSEV_FLOAT(b_offset + 104, v15, vl);
            VSEV_FLOAT(b_offset + 112, v8, vl);
            VSEV_FLOAT(b_offset + 120, v16, vl);

            a_offset1 += 8;
            a_offset2 += 8;
            b_offset += 128;
        }

        if (m & 4) {
            vx4 = VLSSEG4_FLOAT(a_offset1, lda * sizeof(IFLOAT), vl);
            vx41 = VLSSEG4_FLOAT(a_offset2, lda * sizeof(IFLOAT), vl);

            v1 = VGET_VX4(vx4, 0);
            v2 = VGET_VX4(vx4, 1);
            v3 = VGET_VX4(vx4, 2);
            v4 = VGET_VX4(vx4, 3);
            v5 = VGET_VX4(vx41, 0);
            v6 = VGET_VX4(vx41, 1);
            v7 = VGET_VX4(vx41, 2);
            v8 = VGET_VX4(vx41, 3);

            VSEV_FLOAT(b_offset, v1, vl);
            VSEV_FLOAT(b_offset + 8, v5, vl);
            VSEV_FLOAT(b_offset + 16, v2, vl);
            VSEV_FLOAT(b_offset + 24, v6, vl);
            VSEV_FLOAT(b_offset + 32, v3, vl);
            VSEV_FLOAT(b_offset + 40, v7, vl);
            VSEV_FLOAT(b_offset + 48, v4, vl);
            VSEV_FLOAT(b_offset + 56, v8, vl);

            a_offset1 += 4;
            a_offset2 += 4;
            b_offset += 64;
        }

        if (m & 2) {
            vx2 = VLSSEG2_FLOAT(a_offset1, lda * sizeof(IFLOAT), vl);
            vx21 = VLSSEG2_FLOAT(a_offset2, lda * sizeof(IFLOAT), vl);

            v1 = VGET_VX2(vx2, 0);
            v2 = VGET_VX2(vx2, 1);
            v3 = VGET_VX2(vx21, 0);
            v4 = VGET_VX2(vx21, 1);

            VSEV_FLOAT(b_offset, v1, vl);
            VSEV_FLOAT(b_offset + 8, v3, vl);
            VSEV_FLOAT(b_offset + 16, v2, vl);
            VSEV_FLOAT(b_offset + 24, v4, vl);

            a_offset1 += 2;
            a_offset2 += 2;
            b_offset += 32;
        }

        if (m & 1) {
            v21 = VLSEG2_FLOAT(a_offset1, lda * sizeof(IFLOAT), vl * 2);

            VSEV_FLOAT2(b_offset, v21, vl * 2);

            b_offset += 16;
        }
    }

    if (n & 8) {
        a_offset1  = a_offset;
        a_offset2  = a_offset1 + lda;
        a_offset3  = a_offset2 + lda;
        a_offset4  = a_offset3 + lda;
        a_offset5  = a_offset4 + lda;
        a_offset6  = a_offset5 + lda;
        a_offset7  = a_offset6 + lda;
        a_offset8  = a_offset7 + lda;
        a_offset += 8 * lda;

        for(i = m; i > 0; i -= vl) {
            vl = VSETVL(i);

            v1 = VLEV_FLOAT(a_offset1, vl);
            v2 = VLEV_FLOAT(a_offset2, vl);
            v3 = VLEV_FLOAT(a_offset3, vl);
            v4 = VLEV_FLOAT(a_offset4, vl);
            v5 = VLEV_FLOAT(a_offset5, vl);
            v6 = VLEV_FLOAT(a_offset6, vl);
            v7 = VLEV_FLOAT(a_offset7, vl);
            v8 = VLEV_FLOAT(a_offset8, vl);

            vx8 = VSET_VX8(vx8, 0, v1);
            vx8 = VSET_VX8(vx8, 1, v2);
            vx8 = VSET_VX8(vx8, 2, v3);
            vx8 = VSET_VX8(vx8, 3, v4);
            vx8 = VSET_VX8(vx8, 4, v5);
            vx8 = VSET_VX8(vx8, 5, v6);
            vx8 = VSET_VX8(vx8, 6, v7);
            vx8 = VSET_VX8(vx8, 7, v8);

            VSSEG8_FLOAT(b_offset, vx8, vl);

            a_offset1 += vl;
            a_offset2 += vl;
            a_offset3 += vl;
            a_offset4 += vl;
            a_offset5 += vl;
            a_offset6 += vl;
            a_offset7 += vl;
            a_offset8 += vl;
            b_offset += vl*8;
        }
    }

    if (n & 4) {
        a_offset1  = a_offset;
        a_offset2  = a_offset1 + lda;
        a_offset3  = a_offset2 + lda;
        a_offset4  = a_offset3 + lda;
        a_offset += 4 * lda;

        for(i = m; i > 0; i -= vl) {
            vl = VSETVL2(i);

            v21 = VLEV_FLOAT2(a_offset1, vl);
            v22 = VLEV_FLOAT2(a_offset2, vl);
            v23 = VLEV_FLOAT2(a_offset3, vl);
            v24 = VLEV_FLOAT2(a_offset4, vl);

            vx24 = VSET_VX4(vx24, 0, v21);
            vx24 = VSET_VX4(vx24, 1, v22);
            vx24 = VSET_VX4(vx24, 2, v23);
            vx24 = VSET_VX4(vx24, 3, v24);

            VSSEG4_FLOAT(b_offset, vx24, vl);

            a_offset1 += vl;
            a_offset2 += vl;
            a_offset3 += vl;
            a_offset4 += vl;
            b_offset += vl*4;
        }
    }

    if (n & 2) {
        a_offset1  = a_offset;
        a_offset2  = a_offset1 + lda;
        a_offset += 2 * lda;

        for(i = m; i > 0; i -= vl) {
            vl = VSETVL4(i);

            v41 = VLEV_FLOAT4(a_offset1, vl);
            v42 = VLEV_FLOAT4(a_offset2, vl);

            vx42 = VSET_VX2(vx42, 0, v41);
            vx42 = VSET_VX2(vx42, 1, v42);

            VSSEG2_FLOAT(b_offset, vx42, vl);

            a_offset1 += vl;
            a_offset2 += vl;
            b_offset += vl*2;
        }
    }

    if (n & 1) {
        a_offset1  = a_offset;

        for(i = m; i > 0; i -= vl) {
            vl = VSETVL8(i);

            v81 = VLEV_FLOAT8(a_offset1, vl);

            VSEV_FLOAT8(b_offset, v81, vl);

            a_offset1 += vl;
            b_offset += vl;
        }
    }

    return 0;
}
