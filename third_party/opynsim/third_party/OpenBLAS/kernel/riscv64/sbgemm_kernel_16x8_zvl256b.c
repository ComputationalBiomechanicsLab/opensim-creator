#include "common.h"
#include <riscv_vector.h>

int CNAME(BLASLONG M, BLASLONG N, BLASLONG K, FLOAT alpha, IFLOAT *A, IFLOAT *B, FLOAT *C, BLASLONG ldc)
{
    BLASLONG gvl = 0;
    BLASLONG m_top = 0;
    BLASLONG n_top = 0;
    __bf16 *BB = (__bf16 *)(B);
    __bf16 *AA = (__bf16 *)(A);

    // -- MAIN PASS
    for (BLASLONG j=0; j<N/8; j+=1) {
        m_top = 0;
        BLASLONG gvl = __riscv_vsetvl_e16m1(16);

        for (BLASLONG i=0; i<M/16; i+=1) {
            BLASLONG ai=m_top*K;
            BLASLONG bi=n_top*K;

            vfloat32m2_t result0 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);
            vfloat32m2_t result1 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);
            vfloat32m2_t result2 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);
            vfloat32m2_t result3 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);
            vfloat32m2_t result4 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);
            vfloat32m2_t result5 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);
            vfloat32m2_t result6 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);
            vfloat32m2_t result7 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);

            for (BLASLONG k=0; k<K; k++) {
                __bf16 B0 = BB[bi+0];
                __bf16 B1 = BB[bi+1];
                __bf16 B2 = BB[bi+2];
                __bf16 B3 = BB[bi+3];
                __bf16 B4 = BB[bi+4];
                __bf16 B5 = BB[bi+5];
                __bf16 B6 = BB[bi+6];
                __bf16 B7 = BB[bi+7];
                bi += 8;

                vbfloat16m1_t A0 = __riscv_vle16_v_bf16m1( &AA[ai+0*gvl], gvl );
                ai += 16;

                result0 = __riscv_vfwmaccbf16_vf_f32m2(result0, B0, A0, gvl);
                result1 = __riscv_vfwmaccbf16_vf_f32m2(result1, B1, A0, gvl);
                result2 = __riscv_vfwmaccbf16_vf_f32m2(result2, B2, A0, gvl);
                result3 = __riscv_vfwmaccbf16_vf_f32m2(result3, B3, A0, gvl);
                result4 = __riscv_vfwmaccbf16_vf_f32m2(result4, B4, A0, gvl);
                result5 = __riscv_vfwmaccbf16_vf_f32m2(result5, B5, A0, gvl);
                result6 = __riscv_vfwmaccbf16_vf_f32m2(result6, B6, A0, gvl);
                result7 = __riscv_vfwmaccbf16_vf_f32m2(result7, B7, A0, gvl);
            }

            BLASLONG ci=n_top*ldc+m_top;

            vfloat32m2_t c0 = __riscv_vle32_v_f32m2( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m2_t c1 = __riscv_vle32_v_f32m2( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m2_t c2 = __riscv_vle32_v_f32m2( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m2_t c3 = __riscv_vle32_v_f32m2( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m2_t c4 = __riscv_vle32_v_f32m2( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m2_t c5 = __riscv_vle32_v_f32m2( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m2_t c6 = __riscv_vle32_v_f32m2( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m2_t c7 = __riscv_vle32_v_f32m2( &C[ci], gvl);

            c0 = __riscv_vfmacc_vf_f32m2(c0, alpha, result0, gvl);
            c1 = __riscv_vfmacc_vf_f32m2(c1, alpha, result1, gvl);
            c2 = __riscv_vfmacc_vf_f32m2(c2, alpha, result2, gvl);
            c3 = __riscv_vfmacc_vf_f32m2(c3, alpha, result3, gvl);
            c4 = __riscv_vfmacc_vf_f32m2(c4, alpha, result4, gvl);
            c5 = __riscv_vfmacc_vf_f32m2(c5, alpha, result5, gvl);
            c6 = __riscv_vfmacc_vf_f32m2(c6, alpha, result6, gvl);
            c7 = __riscv_vfmacc_vf_f32m2(c7, alpha, result7, gvl);

            ci=n_top*ldc+m_top;

            __riscv_vse32_v_f32m2( &C[ci], c0, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m2( &C[ci], c1, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m2( &C[ci], c2, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m2( &C[ci], c3, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m2( &C[ci], c4, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m2( &C[ci], c5, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m2( &C[ci], c6, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m2( &C[ci], c7, gvl);
            m_top += 16;
        }

        // -- tails for main pass

        if ( M & 8 ) {
            gvl = __riscv_vsetvl_e16mf2(8);

            BLASLONG ai=m_top*K;
            BLASLONG bi=n_top*K;

            vfloat32m1_t result0 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result1 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result2 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result3 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result4 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result5 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result6 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result7 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);

            for (BLASLONG k=0; k<K; k++) {
                __bf16 B0 = BB[bi+0];
                __bf16 B1 = BB[bi+1];
                __bf16 B2 = BB[bi+2];
                __bf16 B3 = BB[bi+3];
                __bf16 B4 = BB[bi+4];
                __bf16 B5 = BB[bi+5];
                __bf16 B6 = BB[bi+6];
                __bf16 B7 = BB[bi+7];
                bi += 8;

                vbfloat16mf2_t A0 = __riscv_vle16_v_bf16mf2( &AA[ai+0*gvl], gvl );
                ai += 8;

                result0 = __riscv_vfwmaccbf16_vf_f32m1(result0, B0, A0, gvl);
                result1 = __riscv_vfwmaccbf16_vf_f32m1(result1, B1, A0, gvl);
                result2 = __riscv_vfwmaccbf16_vf_f32m1(result2, B2, A0, gvl);
                result3 = __riscv_vfwmaccbf16_vf_f32m1(result3, B3, A0, gvl);
                result4 = __riscv_vfwmaccbf16_vf_f32m1(result4, B4, A0, gvl);
                result5 = __riscv_vfwmaccbf16_vf_f32m1(result5, B5, A0, gvl);
                result6 = __riscv_vfwmaccbf16_vf_f32m1(result6, B6, A0, gvl);
                result7 = __riscv_vfwmaccbf16_vf_f32m1(result7, B7, A0, gvl);
            }

            BLASLONG ci=n_top*ldc+m_top;

            vfloat32m1_t c0 = __riscv_vle32_v_f32m1( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m1_t c1 = __riscv_vle32_v_f32m1( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m1_t c2 = __riscv_vle32_v_f32m1( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m1_t c3 = __riscv_vle32_v_f32m1( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m1_t c4 = __riscv_vle32_v_f32m1( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m1_t c5 = __riscv_vle32_v_f32m1( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m1_t c6 = __riscv_vle32_v_f32m1( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m1_t c7 = __riscv_vle32_v_f32m1( &C[ci], gvl);

            c0 = __riscv_vfmacc_vf_f32m1(c0, alpha, result0, gvl);
            c1 = __riscv_vfmacc_vf_f32m1(c1, alpha, result1, gvl);
            c2 = __riscv_vfmacc_vf_f32m1(c2, alpha, result2, gvl);
            c3 = __riscv_vfmacc_vf_f32m1(c3, alpha, result3, gvl);
            c4 = __riscv_vfmacc_vf_f32m1(c4, alpha, result4, gvl);
            c5 = __riscv_vfmacc_vf_f32m1(c5, alpha, result5, gvl);
            c6 = __riscv_vfmacc_vf_f32m1(c6, alpha, result6, gvl);
            c7 = __riscv_vfmacc_vf_f32m1(c7, alpha, result7, gvl);

            ci=n_top*ldc+m_top;

            __riscv_vse32_v_f32m1(&C[ci], c0, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c1, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c2, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c3, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c4, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c5, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c6, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c7, gvl);
            m_top += 8;
        }

        if ( M & 4 ) {
            gvl = __riscv_vsetvl_e16mf2(4);

            BLASLONG ai=m_top*K;
            BLASLONG bi=n_top*K;

            vfloat32m1_t result0 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result1 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result2 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result3 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result4 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result5 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result6 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result7 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);

            for (BLASLONG k=0; k < K; ++k) {
                __bf16 B0 = BB[bi+0];
                __bf16 B1 = BB[bi+1];
                __bf16 B2 = BB[bi+2];
                __bf16 B3 = BB[bi+3];
                __bf16 B4 = BB[bi+4];
                __bf16 B5 = BB[bi+5];
                __bf16 B6 = BB[bi+6];
                __bf16 B7 = BB[bi+7];
                bi += 8;

                vbfloat16mf2_t A0 = __riscv_vle16_v_bf16mf2( &AA[ai+0*gvl], gvl );
                ai += 4;

                result0 = __riscv_vfwmaccbf16_vf_f32m1(result0, B0, A0, gvl);
                result1 = __riscv_vfwmaccbf16_vf_f32m1(result1, B1, A0, gvl);
                result2 = __riscv_vfwmaccbf16_vf_f32m1(result2, B2, A0, gvl);
                result3 = __riscv_vfwmaccbf16_vf_f32m1(result3, B3, A0, gvl);
                result4 = __riscv_vfwmaccbf16_vf_f32m1(result4, B4, A0, gvl);
                result5 = __riscv_vfwmaccbf16_vf_f32m1(result5, B5, A0, gvl);
                result6 = __riscv_vfwmaccbf16_vf_f32m1(result6, B6, A0, gvl);
                result7 = __riscv_vfwmaccbf16_vf_f32m1(result7, B7, A0, gvl);
            }

            BLASLONG ci = n_top * ldc + m_top;

            vfloat32m1_t c0 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            ci += ldc - gvl * 0;
            vfloat32m1_t c1 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            ci += ldc - gvl * 0;
            vfloat32m1_t c2 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            ci += ldc - gvl * 0;
            vfloat32m1_t c3 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            ci += ldc - gvl * 0;
            vfloat32m1_t c4 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            ci += ldc - gvl * 0;
            vfloat32m1_t c5 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            ci += ldc - gvl * 0;
            vfloat32m1_t c6 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            ci += ldc - gvl * 0;
            vfloat32m1_t c7 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            c0 = __riscv_vfmacc_vf_f32m1(c0, alpha, result0, gvl);
            c1 = __riscv_vfmacc_vf_f32m1(c1, alpha, result1, gvl);
            c2 = __riscv_vfmacc_vf_f32m1(c2, alpha, result2, gvl);
            c3 = __riscv_vfmacc_vf_f32m1(c3, alpha, result3, gvl);
            c4 = __riscv_vfmacc_vf_f32m1(c4, alpha, result4, gvl);
            c5 = __riscv_vfmacc_vf_f32m1(c5, alpha, result5, gvl);
            c6 = __riscv_vfmacc_vf_f32m1(c6, alpha, result6, gvl);
            c7 = __riscv_vfmacc_vf_f32m1(c7, alpha, result7, gvl);

            ci= n_top * ldc + m_top;

            __riscv_vse32_v_f32m1(&C[ci], c0, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c1, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c2, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c3, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c4, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c5, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c6, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c7, gvl);

            m_top += 4;
        }

        if ( M & 2 ) {
            float result0 = 0;
            float result1 = 0;
            float result2 = 0;
            float result3 = 0;
            float result4 = 0;
            float result5 = 0;
            float result6 = 0;
            float result7 = 0;
            float result8 = 0;
            float result9 = 0;
            float result10 = 0;
            float result11 = 0;
            float result12 = 0;
            float result13 = 0;
            float result14 = 0;
            float result15 = 0;
            BLASLONG ai = m_top * K;
            BLASLONG bi = n_top * K;

            for (BLASLONG k=0; k<K; k++) {
                result0+=(float)(AA[ai+0])*(float)(BB[bi+0]);
                result1+=(float)(AA[ai+1])*(float)(BB[bi+0]);
                result2+=(float)(AA[ai+0])*(float)(BB[bi+1]);
                result3+=(float)(AA[ai+1])*(float)(BB[bi+1]);
                result4+=(float)(AA[ai+0])*(float)(BB[bi+2]);
                result5+=(float)(AA[ai+1])*(float)(BB[bi+2]);
                result6+=(float)(AA[ai+0])*(float)(BB[bi+3]);
                result7+=(float)(AA[ai+1])*(float)(BB[bi+3]);
                result8+=(float)(AA[ai+0])*(float)(BB[bi+4]);
                result9+=(float)(AA[ai+1])*(float)(BB[bi+4]);
                result10+=(float)(AA[ai+0])*(float)(BB[bi+5]);
                result11+=(float)(AA[ai+1])*(float)(BB[bi+5]);
                result12+=(float)(AA[ai+0])*(float)(BB[bi+6]);
                result13+=(float)(AA[ai+1])*(float)(BB[bi+6]);
                result14+=(float)(AA[ai+0])*(float)(BB[bi+7]);
                result15+=(float)(AA[ai+1])*(float)(BB[bi+7]);
                ai+=2;
                bi+=8;
            }

            BLASLONG ci=n_top*ldc+m_top;

            C[ci + 0 * ldc + 0] += alpha * result0;
            C[ci + 0 * ldc + 1] += alpha * result1;
            C[ci + 1 * ldc + 0] += alpha * result2;
            C[ci + 1 * ldc + 1] += alpha * result3;
            C[ci + 2 * ldc + 0] += alpha * result4;
            C[ci + 2 * ldc + 1] += alpha * result5;
            C[ci + 3 * ldc + 0] += alpha * result6;
            C[ci + 3 * ldc + 1] += alpha * result7;
            C[ci + 4 * ldc + 0] += alpha * result8;
            C[ci + 4 * ldc + 1] += alpha * result9;
            C[ci + 5 * ldc + 0] += alpha * result10;
            C[ci + 5 * ldc + 1] += alpha * result11;
            C[ci + 6 * ldc + 0] += alpha * result12;
            C[ci + 6 * ldc + 1] += alpha * result13;
            C[ci + 7 * ldc + 0] += alpha * result14;
            C[ci + 7 * ldc + 1] += alpha * result15;

            m_top+=2;
        }


        if ( M & 1 ) {

            float result0 = 0;
            float result1 = 0;
            float result2 = 0;
            float result3 = 0;
            float result4 = 0;
            float result5 = 0;
            float result6 = 0;
            float result7 = 0;

            BLASLONG ai = m_top * K;
            BLASLONG bi = n_top * K;

            for (BLASLONG k=0; k<K; k++) {
                result0+=(float)(AA[ai+0])*(float)(BB[bi+0]);
                result1+=(float)(AA[ai+0])*(float)(BB[bi+1]);
                result2+=(float)(AA[ai+0])*(float)(BB[bi+2]);
                result3+=(float)(AA[ai+0])*(float)(BB[bi+3]);
                result4+=(float)(AA[ai+0])*(float)(BB[bi+4]);
                result5+=(float)(AA[ai+0])*(float)(BB[bi+5]);
                result6+=(float)(AA[ai+0])*(float)(BB[bi+6]);
                result7+=(float)(AA[ai+0])*(float)(BB[bi+7]);
                ai+=1;
                bi+=8;
            }

            BLASLONG ci = n_top * ldc + m_top;
            C[ci + 0 * ldc + 0] += alpha * result0;
            C[ci + 1 * ldc + 0] += alpha * result1;
            C[ci + 2 * ldc + 0] += alpha * result2;
            C[ci + 3 * ldc + 0] += alpha * result3;
            C[ci + 4 * ldc + 0] += alpha * result4;
            C[ci + 5 * ldc + 0] += alpha * result5;
            C[ci + 6 * ldc + 0] += alpha * result6;
            C[ci + 7 * ldc + 0] += alpha * result7;
            m_top+=1;
        }
        n_top += 8;
    }

    if ( N & 4 ) {
        gvl = __riscv_vsetvl_e16m1(16);
        m_top = 0;

        for (BLASLONG i=0; i<M/16; i+=1) {
            BLASLONG ai=m_top*K;
            BLASLONG bi=n_top*K;

            vfloat32m2_t result0 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);
            vfloat32m2_t result1 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);
            vfloat32m2_t result2 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);
            vfloat32m2_t result3 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);

            for (BLASLONG k=0; k<K; k++) {
                __bf16 B0 = BB[bi+0];
                __bf16 B1 = BB[bi+1];
                __bf16 B2 = BB[bi+2];
                __bf16 B3 = BB[bi+3];
                bi += 4;

                vbfloat16m1_t A0 = __riscv_vle16_v_bf16m1( &AA[ai+0*gvl], gvl );
                ai += 16;

                result0 = __riscv_vfwmaccbf16_vf_f32m2(result0, B0, A0, gvl);
                result1 = __riscv_vfwmaccbf16_vf_f32m2(result1, B1, A0, gvl);
                result2 = __riscv_vfwmaccbf16_vf_f32m2(result2, B2, A0, gvl);
                result3 = __riscv_vfwmaccbf16_vf_f32m2(result3, B3, A0, gvl);
            }

            BLASLONG ci=n_top*ldc+m_top;

            vfloat32m2_t c0 = __riscv_vle32_v_f32m2( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m2_t c1 = __riscv_vle32_v_f32m2( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m2_t c2 = __riscv_vle32_v_f32m2( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m2_t c3 = __riscv_vle32_v_f32m2( &C[ci], gvl);

            c0 = __riscv_vfmacc_vf_f32m2(c0, alpha, result0, gvl);
            c1 = __riscv_vfmacc_vf_f32m2(c1, alpha, result1, gvl);
            c2 = __riscv_vfmacc_vf_f32m2(c2, alpha, result2, gvl);
            c3 = __riscv_vfmacc_vf_f32m2(c3, alpha, result3, gvl);

            ci=n_top*ldc+m_top;

            __riscv_vse32_v_f32m2( &C[ci], c0, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m2( &C[ci], c1, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m2( &C[ci], c2, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m2( &C[ci], c3, gvl);
            m_top += 16;
        }

        if ( M & 8 ) {
            gvl = __riscv_vsetvl_e16mf2(8);
            BLASLONG ai=m_top*K;
            BLASLONG bi=n_top*K;

            vfloat32m1_t result0 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result1 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result2 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result3 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);

            for (BLASLONG k=0; k<K; k++) {
                __bf16 B0 = BB[bi+0];
                __bf16 B1 = BB[bi+1];
                __bf16 B2 = BB[bi+2];
                __bf16 B3 = BB[bi+3];
                bi += 4;

                vbfloat16mf2_t A0 = __riscv_vle16_v_bf16mf2( &AA[ai+0*gvl], gvl );
                ai += 8;

                result0 = __riscv_vfwmaccbf16_vf_f32m1(result0, B0, A0, gvl);
                result1 = __riscv_vfwmaccbf16_vf_f32m1(result1, B1, A0, gvl);
                result2 = __riscv_vfwmaccbf16_vf_f32m1(result2, B2, A0, gvl);
                result3 = __riscv_vfwmaccbf16_vf_f32m1(result3, B3, A0, gvl);
            }

            BLASLONG ci=n_top*ldc+m_top;

            vfloat32m1_t c0 = __riscv_vle32_v_f32m1( &C[ci], gvl); ci += ldc - gvl * 0;
            vfloat32m1_t c1 = __riscv_vle32_v_f32m1( &C[ci], gvl); ci += ldc - gvl * 0;
            vfloat32m1_t c2 = __riscv_vle32_v_f32m1( &C[ci], gvl); ci += ldc - gvl * 0;
            vfloat32m1_t c3 = __riscv_vle32_v_f32m1( &C[ci], gvl);

            c0 = __riscv_vfmacc_vf_f32m1(c0, alpha, result0, gvl);
            c1 = __riscv_vfmacc_vf_f32m1(c1, alpha, result1, gvl);
            c2 = __riscv_vfmacc_vf_f32m1(c2, alpha, result2, gvl);
            c3 = __riscv_vfmacc_vf_f32m1(c3, alpha, result3, gvl);

            ci = n_top * ldc + m_top;

            __riscv_vse32_v_f32m1( &C[ci], c0, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m1( &C[ci], c1, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m1( &C[ci], c2, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m1( &C[ci], c3, gvl);
            m_top += 8;
        }

        if ( M & 4 ) {
            gvl = __riscv_vsetvl_e16mf2(4);

            BLASLONG ai=m_top*K;
            BLASLONG bi=n_top*K;

            vfloat32m1_t result0 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result1 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result2 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result3 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);

            for (BLASLONG k=0; k < K; ++k) {
                __bf16 B0 = BB[bi+0];
                __bf16 B1 = BB[bi+1];
                __bf16 B2 = BB[bi+2];
                __bf16 B3 = BB[bi+3];
                bi += 4;

                vbfloat16mf2_t A0 = __riscv_vle16_v_bf16mf2( &AA[ai+0*gvl], gvl );
                ai += 4;

                result0 = __riscv_vfwmaccbf16_vf_f32m1(result0, B0, A0, gvl);
                result1 = __riscv_vfwmaccbf16_vf_f32m1(result1, B1, A0, gvl);
                result2 = __riscv_vfwmaccbf16_vf_f32m1(result2, B2, A0, gvl);
                result3 = __riscv_vfwmaccbf16_vf_f32m1(result3, B3, A0, gvl);
            }

            BLASLONG ci = n_top * ldc + m_top;

            vfloat32m1_t c0 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            ci += ldc - gvl * 0;
            vfloat32m1_t c1 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            ci += ldc - gvl * 0;
            vfloat32m1_t c2 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            ci += ldc - gvl * 0;
            vfloat32m1_t c3 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            c0 = __riscv_vfmacc_vf_f32m1(c0, alpha, result0, gvl);
            c1 = __riscv_vfmacc_vf_f32m1(c1, alpha, result1, gvl);
            c2 = __riscv_vfmacc_vf_f32m1(c2, alpha, result2, gvl);
            c3 = __riscv_vfmacc_vf_f32m1(c3, alpha, result3, gvl);

            ci= n_top * ldc + m_top;

            __riscv_vse32_v_f32m1(&C[ci], c0, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c1, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c2, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c3, gvl);

            m_top += 4;
        }

        if ( M & 2 ) {
            float result0 = 0;
            float result1 = 0;
            float result2 = 0;
            float result3 = 0;
            float result4 = 0;
            float result5 = 0;
            float result6 = 0;
            float result7 = 0;
            BLASLONG ai = m_top * K;
            BLASLONG bi = n_top * K;

            for (BLASLONG k=0; k<K; k++) {
                result0+=(float)(AA[ai+0])*(float)(BB[bi+0]);
                result1+=(float)(AA[ai+1])*(float)(BB[bi+0]);
                result2+=(float)(AA[ai+0])*(float)(BB[bi+1]);
                result3+=(float)(AA[ai+1])*(float)(BB[bi+1]);
                result4+=(float)(AA[ai+0])*(float)(BB[bi+2]);
                result5+=(float)(AA[ai+1])*(float)(BB[bi+2]);
                result6+=(float)(AA[ai+0])*(float)(BB[bi+3]);
                result7+=(float)(AA[ai+1])*(float)(BB[bi+3]);
                ai+=2;
                bi+=4;
            }

            BLASLONG ci=n_top*ldc+m_top;
            C[ci + 0 * ldc + 0] += alpha * result0;
            C[ci + 0 * ldc + 1] += alpha * result1;
            C[ci + 1 * ldc + 0] += alpha * result2;
            C[ci + 1 * ldc + 1] += alpha * result3;
            C[ci + 2 * ldc + 0] += alpha * result4;
            C[ci + 2 * ldc + 1] += alpha * result5;
            C[ci + 3 * ldc + 0] += alpha * result6;
            C[ci + 3 * ldc + 1] += alpha * result7;

            m_top += 2;
        }

        if ( M & 1 ) {

            float result0 = 0;
            float result1 = 0;
            float result2 = 0;
            float result3 = 0;

            BLASLONG ai = m_top * K;
            BLASLONG bi = n_top * K;

            for (BLASLONG k=0; k<K; k++) {
                result0+=(float)(AA[ai+0])*(float)(BB[bi+0]);
                result1+=(float)(AA[ai+0])*(float)(BB[bi+1]);
                result2+=(float)(AA[ai+0])*(float)(BB[bi+2]);
                result3+=(float)(AA[ai+0])*(float)(BB[bi+3]);
                ai+=1;
                bi+=4;
            }

            BLASLONG ci = n_top * ldc + m_top;
            C[ci + 0 * ldc + 0] += alpha * result0;
            C[ci + 1 * ldc + 0] += alpha * result1;
            C[ci + 2 * ldc + 0] += alpha * result2;
            C[ci + 3 * ldc + 0] += alpha * result3;
            m_top += 1;
        }

        n_top += 4;
    }

    // -- tails for N=2
    if ( N & 2 ) {
        gvl = __riscv_vsetvl_e16m1(16);
        m_top = 0;

        for (BLASLONG i=0; i<M/16; i+=1) {
            BLASLONG ai=m_top*K;
            BLASLONG bi=n_top*K;

            vfloat32m2_t result0 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);
            vfloat32m2_t result1 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);

            for (BLASLONG k=0; k<K; k++) {
                __bf16 B0 = BB[bi+0];
                __bf16 B1 = BB[bi+1];
                bi += 2;

                vbfloat16m1_t A0 = __riscv_vle16_v_bf16m1( &AA[ai+0*gvl], gvl );
                ai += 16;

                result0 = __riscv_vfwmaccbf16_vf_f32m2(result0, B0, A0, gvl);
                result1 = __riscv_vfwmaccbf16_vf_f32m2(result1, B1, A0, gvl);
            }

            BLASLONG ci=n_top*ldc+m_top;

            vfloat32m2_t c0 = __riscv_vle32_v_f32m2( &C[ci], gvl); ci += ldc-gvl*0;
            vfloat32m2_t c1 = __riscv_vle32_v_f32m2( &C[ci], gvl);
            c0 = __riscv_vfmacc_vf_f32m2(c0, alpha, result0, gvl);
            c1 = __riscv_vfmacc_vf_f32m2(c1, alpha, result1, gvl);

            ci=n_top*ldc+m_top;

            __riscv_vse32_v_f32m2( &C[ci], c0, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m2( &C[ci], c1, gvl);
            m_top += 16;
        }

        if ( M & 8 ) {
            gvl = __riscv_vsetvl_e16mf2(8);
            BLASLONG ai=m_top*K;
            BLASLONG bi=n_top*K;

            vfloat32m1_t result0 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result1 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);

            for (BLASLONG k=0; k<K; k++) {
                __bf16 B0 = BB[bi+0];
                __bf16 B1 = BB[bi+1];
                bi += 2;

                vbfloat16mf2_t A0 = __riscv_vle16_v_bf16mf2( &AA[ai+0*gvl], gvl );
                ai += 8;

                result0 = __riscv_vfwmaccbf16_vf_f32m1(result0, B0, A0, gvl);
                result1 = __riscv_vfwmaccbf16_vf_f32m1(result1, B1, A0, gvl);
            }

            BLASLONG ci=n_top*ldc+m_top;

            vfloat32m1_t c0 = __riscv_vle32_v_f32m1( &C[ci], gvl); ci += ldc - gvl * 0;
            vfloat32m1_t c1 = __riscv_vle32_v_f32m1( &C[ci], gvl);

            c0 = __riscv_vfmacc_vf_f32m1(c0, alpha, result0, gvl);
            c1 = __riscv_vfmacc_vf_f32m1(c1, alpha, result1, gvl);

            ci = n_top * ldc + m_top;

            __riscv_vse32_v_f32m1( &C[ci], c0, gvl); ci += ldc-gvl*0;
            __riscv_vse32_v_f32m1( &C[ci], c1, gvl);
            m_top += 8;
        }

        if ( M & 4 ) {
            gvl = __riscv_vsetvl_e16mf2(4);

            BLASLONG ai=m_top*K;
            BLASLONG bi=n_top*K;

            vfloat32m1_t result0 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);
            vfloat32m1_t result1 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);

            for (BLASLONG k=0; k < K; ++k) {
                __bf16 B0 = BB[bi+0];
                __bf16 B1 = BB[bi+1];
                bi += 2;

                vbfloat16mf2_t A0 = __riscv_vle16_v_bf16mf2( &AA[ai+0*gvl], gvl );
                ai += 4;

                result0 = __riscv_vfwmaccbf16_vf_f32m1(result0, B0, A0, gvl);
                result1 = __riscv_vfwmaccbf16_vf_f32m1(result1, B1, A0, gvl);
            }

            BLASLONG ci = n_top * ldc + m_top;

            vfloat32m1_t c0 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            ci += ldc - gvl * 0;
            vfloat32m1_t c1 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            c0 = __riscv_vfmacc_vf_f32m1(c0, alpha, result0, gvl);
            c1 = __riscv_vfmacc_vf_f32m1(c1, alpha, result1, gvl);

            ci= n_top * ldc + m_top;

            __riscv_vse32_v_f32m1(&C[ci], c0, gvl); ci += ldc - gvl * 0;
            __riscv_vse32_v_f32m1(&C[ci], c1, gvl);

            m_top += 4;
        }

        if ( M & 2 ) {
            float result0 = 0;
            float result1 = 0;
            float result2 = 0;
            float result3 = 0;
            BLASLONG ai = m_top * K;
            BLASLONG bi = n_top * K;

            for (BLASLONG k=0; k<K; k++) {
                result0+=(float)(AA[ai+0])*(float)(BB[bi+0]);
                result1+=(float)(AA[ai+1])*(float)(BB[bi+0]);
                result2+=(float)(AA[ai+0])*(float)(BB[bi+1]);
                result3+=(float)(AA[ai+1])*(float)(BB[bi+1]);
                ai+=2;
                bi+=2;
            }

            BLASLONG ci=n_top*ldc+m_top;
            C[ci + 0 * ldc + 0] += alpha * result0;
            C[ci + 0 * ldc + 1] += alpha * result1;
            C[ci + 1 * ldc + 0] += alpha * result2;
            C[ci + 1 * ldc + 1] += alpha * result3;

            m_top += 2;
        }

        if ( M & 1 ) {

            float result0 = 0;
            float result1 = 0;

            BLASLONG ai = m_top * K;
            BLASLONG bi = n_top * K;

            for (BLASLONG k=0; k<K; k++) {
                result0+=(float)(AA[ai+0])*(float)(BB[bi+0]);
                result1+=(float)(AA[ai+0])*(float)(BB[bi+1]);
                ai+=1;
                bi+=2;
            }

            BLASLONG ci = n_top * ldc + m_top;
            C[ci + 0 * ldc + 0] += alpha * result0;
            C[ci + 1 * ldc + 0] += alpha * result1;
            m_top += 1;
        }

        n_top += 2;
    }

    // -- tails for N=1
    if ( N & 1 ) {
        gvl = __riscv_vsetvl_e16m1(16);
        m_top = 0;

        for (BLASLONG i=0; i<M/16; i+=1) {
            BLASLONG ai=m_top*K;
            BLASLONG bi=n_top*K;

            vfloat32m2_t result0 = __riscv_vfmv_v_f_f32m2(0.0f, gvl);

            for (BLASLONG k=0; k<K; k++) {
                __bf16 B0 = BB[bi+0];
                bi += 1;

                vbfloat16m1_t A0 = __riscv_vle16_v_bf16m1( &AA[ai+0*gvl], gvl );
                ai += 16;

                result0 = __riscv_vfwmaccbf16_vf_f32m2(result0, B0, A0, gvl);
            }

            BLASLONG ci=n_top*ldc+m_top;

            vfloat32m2_t c0 = __riscv_vle32_v_f32m2( &C[ci], gvl);

            c0 = __riscv_vfmacc_vf_f32m2(c0, alpha, result0, gvl);

            ci=n_top*ldc+m_top;

            __riscv_vse32_v_f32m2( &C[ci], c0, gvl);
            m_top += 16;
        }

        if ( M & 8 ) {
            gvl = __riscv_vsetvl_e16mf2(8);
            BLASLONG ai=m_top*K;
            BLASLONG bi=n_top*K;

            vfloat32m1_t result0 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);

            for (BLASLONG k=0; k<K; k++) {
                __bf16 B0 = BB[bi+0];
                bi += 1;

                vbfloat16mf2_t A0 = __riscv_vle16_v_bf16mf2( &AA[ai+0*gvl], gvl );
                ai += 8;

                result0 = __riscv_vfwmaccbf16_vf_f32m1(result0, B0, A0, gvl);
            }

            BLASLONG ci=n_top*ldc+m_top;

            vfloat32m1_t c0 = __riscv_vle32_v_f32m1( &C[ci], gvl);

            c0 = __riscv_vfmacc_vf_f32m1(c0, alpha, result0, gvl);

            ci = n_top * ldc + m_top;

            __riscv_vse32_v_f32m1( &C[ci], c0, gvl);
            m_top += 8;
        }

        if ( M & 4 ) {
            gvl = __riscv_vsetvl_e16mf2(4);

            BLASLONG ai=m_top*K;
            BLASLONG bi=n_top*K;

            vfloat32m1_t result0 = __riscv_vfmv_v_f_f32m1(0.0f, gvl);

            for (BLASLONG k=0; k < K; ++k) {
                __bf16 B0 = BB[bi+0];
                bi += 1;

                vbfloat16mf2_t A0 = __riscv_vle16_v_bf16mf2( &AA[ai+0*gvl], gvl );
                ai += 4;

                result0 = __riscv_vfwmaccbf16_vf_f32m1(result0, B0, A0, gvl);
            }

            BLASLONG ci = n_top * ldc + m_top;

            vfloat32m1_t c0 = __riscv_vle32_v_f32m1(&C[ci], gvl);
            c0 = __riscv_vfmacc_vf_f32m1(c0, alpha, result0, gvl);

            ci= n_top * ldc + m_top;

            __riscv_vse32_v_f32m1(&C[ci], c0, gvl);
            m_top += 4;
        }

        if ( M & 2 ) {
            float result0 = 0;
            float result1 = 0;
            BLASLONG ai = m_top * K;
            BLASLONG bi = n_top * K;

            for (BLASLONG k=0; k<K; k++) {
                result0+=(float)(AA[ai+0])*(float)(BB[bi+0]);
                result1+=(float)(AA[ai+1])*(float)(BB[bi+0]);
                ai+=2;
                bi+=1;
            }

            BLASLONG ci=n_top*ldc+m_top;
            C[ci + 0 * ldc + 0] += alpha * result0;
            C[ci + 0 * ldc + 1] += alpha * result1;

            m_top += 2;
        }

        if ( M & 1 ) {

            float result0 = 0;

            BLASLONG ai = m_top * K;
            BLASLONG bi = n_top * K;

            for (BLASLONG k=0; k<K; k++) {
                result0+=(float)(AA[ai+0])*(float)(BB[bi+0]);
                ai+=1;
                bi+=1;
            }

            BLASLONG ci = n_top * ldc + m_top;
            C[ci + 0 * ldc + 0] += alpha * result0;
            m_top += 1;
        }

        n_top += 1;
    }
    return 0;
}
