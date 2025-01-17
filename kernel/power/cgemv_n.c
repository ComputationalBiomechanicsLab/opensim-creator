/***************************************************************************
Copyright (c) 2019, The OpenBLAS Project
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
#if !defined(__VEC__) || !defined(__ALTIVEC__)
#include "../arm/zgemv_n.c"
#else

#include <stdlib.h>
#include <stdio.h>
#include "common.h" 
#include <altivec.h>   
#define NBMAX 1024


static const unsigned char __attribute__((aligned(16))) swap_mask_arr[]={ 4,5,6,7,0,1,2,3, 12,13,14,15, 8,9,10,11};

 
static void cgemv_kernel_4x4(BLASLONG n, BLASLONG lda, FLOAT *ap, FLOAT *x, FLOAT *y) {
  
 FLOAT *a0, *a1, *a2, *a3;
    a0 = ap;
    a1 = ap + lda;
    a2 = a1 + lda;
    a3 = a2 + lda;
    __vector unsigned char swap_mask = *((__vector unsigned char*)swap_mask_arr);
#if ( !defined(CONJ) && !defined(XCONJ) ) || ( defined(CONJ) && defined(XCONJ) )
    register __vector float vx0_r = {x[0], x[0],x[0], x[0]};
    register __vector float vx0_i = {-x[1], x[1],-x[1], x[1]};
    register __vector float vx1_r = {x[2], x[2],x[2], x[2]};
    register __vector float vx1_i = {-x[3], x[3],-x[3], x[3]};
    register __vector float vx2_r = {x[4], x[4],x[4], x[4]};
    register __vector float vx2_i = {-x[5], x[5],-x[5], x[5]};
    register __vector float vx3_r = {x[6], x[6],x[6], x[6]};
    register __vector float vx3_i = {-x[7], x[7],-x[7], x[7]};
#else
    register __vector float vx0_r = {x[0], -x[0],x[0], -x[0]};
    register __vector float vx0_i = {x[1], x[1],x[1], x[1]};
    register __vector float vx1_r = {x[2], -x[2],x[2], -x[2]};
    register __vector float vx1_i = {x[3], x[3],x[3], x[3]};
    register __vector float vx2_r = {x[4], -x[4],x[4], -x[4]};
    register __vector float vx2_i = {x[5], x[5],x[5], x[5]};
    register __vector float vx3_r = {x[6], -x[6],x[6], -x[6]};
    register __vector float vx3_i = {x[7], x[7],x[7], x[7]};
#endif
    register __vector float *vptr_y = (__vector float *) y;
    register __vector float *vptr_a0 = (__vector float *) a0;
    register __vector float *vptr_a1 = (__vector float *) a1;
    register __vector float *vptr_a2 = (__vector float *) a2;
    register __vector float *vptr_a3 = (__vector float *) a3; 
    BLASLONG  i = 0; 
    BLASLONG i2=16;
    for (;i< n * 8; i+=32,i2+=32) {
        register __vector float vy_0  = vec_vsx_ld(i,vptr_y);
        register __vector float vy_1  = vec_vsx_ld(i2,vptr_y);
        register __vector float va0   = vec_vsx_ld(i,vptr_a0);
        register __vector float va1   = vec_vsx_ld(i, vptr_a1);
        register __vector float va2   = vec_vsx_ld(i ,vptr_a2);
        register __vector float va3   = vec_vsx_ld(i ,vptr_a3);
        register __vector float va0_1 = vec_vsx_ld(i2 ,vptr_a0);
        register __vector float va1_1 = vec_vsx_ld(i2 ,vptr_a1);
        register __vector float va2_1 = vec_vsx_ld(i2 ,vptr_a2);
        register __vector float va3_1 = vec_vsx_ld(i2 ,vptr_a3);

        vy_0 += va0*vx0_r + va1*vx1_r + va2*vx2_r + va3*vx3_r;
        vy_1 += va0_1*vx0_r + va1_1*vx1_r + va2_1*vx2_r + va3_1*vx3_r;
        va0   = vec_perm(va0, va0,swap_mask);
        va0_1 = vec_perm(va0_1, va0_1,swap_mask);
        va1   = vec_perm(va1, va1,swap_mask);
        va1_1 = vec_perm(va1_1, va1_1,swap_mask);
        va2   = vec_perm(va2, va2,swap_mask);
        va2_1 = vec_perm(va2_1, va2_1,swap_mask);
        va3   = vec_perm(va3, va3,swap_mask);
        va3_1 = vec_perm(va3_1, va3_1,swap_mask);
        vy_0 += va0*vx0_i + va1*vx1_i + va2*vx2_i + va3*vx3_i;
        vy_1 += va0_1*vx0_i + va1_1*vx1_i + va2_1*vx2_i + va3_1*vx3_i;

        vec_vsx_st(vy_0 ,i,  vptr_y);
        vec_vsx_st(vy_1,i2,vptr_y);
    }

}	
 


static void cgemv_kernel_4x2(BLASLONG n, BLASLONG lda, FLOAT *ap, FLOAT *x, FLOAT *y) {
 
    FLOAT *a0, *a1;
    a0 = ap;
    a1 = ap + lda; 
    __vector unsigned char swap_mask = *((__vector unsigned char*)swap_mask_arr);
#if ( !defined(CONJ) && !defined(XCONJ) ) || ( defined(CONJ) && defined(XCONJ) )
    register __vector float vx0_r = {x[0], x[0],x[0], x[0]};
    register __vector float vx0_i = {-x[1], x[1],-x[1], x[1]};
    register __vector float vx1_r = {x[2], x[2],x[2], x[2]};
    register __vector float vx1_i = {-x[3], x[3],-x[3], x[3]}; 
#else
    register __vector float vx0_r = {x[0], -x[0],x[0], -x[0]};
    register __vector float vx0_i = {x[1], x[1],x[1], x[1]};
    register __vector float vx1_r = {x[2], -x[2],x[2], -x[2]};
    register __vector float vx1_i = {x[3], x[3],x[3], x[3]}; 
#endif
    register __vector float *vptr_y = (__vector float *) y;
    register __vector float *vptr_a0 = (__vector float *) a0;
    register __vector float *vptr_a1 = (__vector float *) a1; 
    BLASLONG  i = 0;
    BLASLONG  i2 = 16;  
    for (;i< n * 8; i+=32, i2+=32) { 
        register __vector float vy_0  = vec_vsx_ld(i,vptr_y);
        register __vector float vy_1  = vec_vsx_ld(i2,vptr_y);
        register __vector float va0   = vec_vsx_ld(i,vptr_a0);
        register __vector float va1   = vec_vsx_ld(i, vptr_a1); 
        register __vector float va0_1 = vec_vsx_ld(i2 ,vptr_a0);
        register __vector float va1_1 = vec_vsx_ld(i2 ,vptr_a1); 

        register __vector float va0x   = vec_perm(va0, va0,swap_mask);
        register __vector float va0x_1 = vec_perm(va0_1, va0_1,swap_mask);
        register __vector float va1x   = vec_perm(va1, va1,swap_mask);
        register __vector float va1x_1 = vec_perm(va1_1, va1_1,swap_mask);
        vy_0 += va0*vx0_r + va1*vx1_r + va0x*vx0_i + va1x*vx1_i;
        vy_1 += va0_1*vx0_r + va1_1*vx1_r + va0x_1*vx0_i + va1x_1*vx1_i; 

        vec_vsx_st(vy_0 ,i,  vptr_y);
        vec_vsx_st(vy_1,i2,vptr_y);
    }

}

 

static void cgemv_kernel_4x1(BLASLONG n, FLOAT *ap, FLOAT *x, FLOAT *y) {

    __vector unsigned char swap_mask = *((__vector unsigned char*)swap_mask_arr);
#if ( !defined(CONJ) && !defined(XCONJ) ) || ( defined(CONJ) && defined(XCONJ) )
    register __vector float vx0_r = {x[0], x[0],x[0], x[0]};
    register __vector float vx0_i = {-x[1], x[1],-x[1], x[1]}; 
#else
    register __vector float vx0_r = {x[0], -x[0],x[0], -x[0]};
    register __vector float vx0_i = {x[1], x[1],x[1], x[1]}; 
#endif
    register __vector float *vptr_y = (__vector float *) y;
    register __vector float *vptr_a0 = (__vector float *) ap; 
    BLASLONG  i = 0; 
    BLASLONG  i2 = 16;  
    for (;i< n * 8; i+=32, i2+=32) { 
        register __vector float vy_0  = vec_vsx_ld(i,vptr_y);
        register __vector float vy_1  = vec_vsx_ld(i2,vptr_y);
        register __vector float va0   = vec_vsx_ld(i,vptr_a0); 
        register __vector float va0_1 = vec_vsx_ld(i2 ,vptr_a0); 

        register __vector float va0x   = vec_perm(va0, va0,swap_mask);
        register __vector float va0x_1 = vec_perm(va0_1, va0_1,swap_mask);
        vy_0 += va0*vx0_r + va0x*vx0_i;
        vy_1 += va0_1*vx0_r + va0x_1*vx0_i; 

        vec_vsx_st(vy_0 ,i,  vptr_y);
        vec_vsx_st(vy_1,i2,vptr_y);
    }
}




static void add_y(BLASLONG n, FLOAT *src, FLOAT *dest, BLASLONG inc_dest, FLOAT alpha_r, FLOAT alpha_i) {
    BLASLONG i=0;


    if (inc_dest != 2) {
 		FLOAT temp_r;
		FLOAT temp_i;
		for ( i=0; i<n; i++ )
		{
#if !defined(XCONJ) 
			temp_r = alpha_r * src[0] - alpha_i * src[1];
			temp_i = alpha_r * src[1] + alpha_i * src[0];
#else
			temp_r =  alpha_r * src[0] + alpha_i * src[1];
			temp_i = -alpha_r * src[1] + alpha_i * src[0];
#endif

			*dest += temp_r;
			*(dest+1) += temp_i;

			src+=2;
			dest += inc_dest;
		}
        return;
    } else {
        __vector unsigned char swap_mask = *((__vector unsigned char*)swap_mask_arr);
#if   !defined(XCONJ) 

        register __vector float valpha_r = {alpha_r, alpha_r, alpha_r, alpha_r};
        register __vector float valpha_i = {-alpha_i, alpha_i, -alpha_i, alpha_i};

#else
        register __vector float valpha_r = {alpha_r, -alpha_r, alpha_r, -alpha_r};
        register __vector float valpha_i = {alpha_i, alpha_i, alpha_i, alpha_i};
#endif

        register __vector float *vptr_src = (__vector float *) src;
        register __vector float *vptr_y = (__vector float *) dest; 

    BLASLONG  i2 = 16;  
    for (;i< n * 8; i+=32, i2+=32) { 
        register __vector float vy_0  = vec_vsx_ld(i,vptr_y);
        register __vector float vy_1  = vec_vsx_ld(i2,vptr_y);


        register __vector float vsrc  = vec_vsx_ld(i,vptr_src);
        register __vector float vsrc_1  = vec_vsx_ld(i2,vptr_src);

        register __vector float vsrcx = vec_perm(vsrc, vsrc, swap_mask);
        register __vector float vsrcx_1 = vec_perm(vsrc_1, vsrc_1, swap_mask);

        vy_0 += vsrc*valpha_r + vsrcx*valpha_i;
        vy_1 += vsrc_1*valpha_r +  vsrcx_1*valpha_i;  

        vec_vsx_st(vy_0 ,i,  vptr_y);
        vec_vsx_st(vy_1,i2,vptr_y);

        }
 
    }
    return;
}



int CNAME(BLASLONG m, BLASLONG n, BLASLONG dummy1, FLOAT alpha_r, FLOAT alpha_i, FLOAT *a, BLASLONG lda, FLOAT *x, BLASLONG inc_x, FLOAT *y, BLASLONG inc_y, FLOAT * buffer) {
    BLASLONG i=0;
    FLOAT *a_ptr;
    FLOAT *x_ptr;
    FLOAT *y_ptr;

    BLASLONG n1;
    BLASLONG m1;
    BLASLONG m2;
    BLASLONG m3;
    BLASLONG n2;
    FLOAT xbuffer[8] __attribute__((aligned(16)));
    FLOAT *ybuffer;

    if (m < 1) return (0);
    if (n < 1) return (0);

    ybuffer = buffer;

    inc_x *= 2;
    inc_y *= 2;
    lda *= 2;

    n1 = n / 4;
    n2 = n % 4;

    m3 = m % 4;
    m1 = m - (m % 4);
    m2 = (m % NBMAX) - (m % 4);

    y_ptr = y;

    BLASLONG NB = NBMAX;

    while (NB == NBMAX) {

        m1 -= NB;
        if (m1 < 0) {
            if (m2 == 0) break;
            NB = m2;
        }

        a_ptr = a;

        x_ptr = x; 

        memset(ybuffer, 0, NB * 2*sizeof(FLOAT));  

        if (inc_x == 2) {

            for (i = 0; i < n1; i++) {
                cgemv_kernel_4x4(NB, lda, a_ptr, x_ptr, ybuffer);

                a_ptr += lda << 2;
                x_ptr += 8;
            }

            if (n2 & 2) {
                cgemv_kernel_4x2(NB, lda, a_ptr, x_ptr, ybuffer);
                x_ptr += 4;
                a_ptr += 2 * lda;

            }

            if (n2 & 1) {
                cgemv_kernel_4x1(NB, a_ptr, x_ptr, ybuffer);
                x_ptr += 2;
                a_ptr += lda;

            }
        } else {

            for (i = 0; i < n1; i++) {

                xbuffer[0] = x_ptr[0];
                xbuffer[1] = x_ptr[1];
                x_ptr += inc_x;
                xbuffer[2] = x_ptr[0];
                xbuffer[3] = x_ptr[1];
                x_ptr += inc_x;
                xbuffer[4] = x_ptr[0];
                xbuffer[5] = x_ptr[1];
                x_ptr += inc_x;
                xbuffer[6] = x_ptr[0];
                xbuffer[7] = x_ptr[1];
                x_ptr += inc_x;

                cgemv_kernel_4x4(NB, lda, a_ptr, xbuffer, ybuffer);

                a_ptr += lda << 2;
            }

            for (i = 0; i < n2; i++) {
                xbuffer[0] = x_ptr[0];
                xbuffer[1] = x_ptr[1];
                x_ptr += inc_x;
                cgemv_kernel_4x1(NB, a_ptr, xbuffer, ybuffer);
                a_ptr += lda;

            }

        }

        add_y(NB, ybuffer, y_ptr, inc_y, alpha_r, alpha_i);
        a += 2 * NB;
        y_ptr += NB * inc_y;
    }

    if (m3 == 0) return (0);

    if (m3 == 1) {
        a_ptr = a;
        x_ptr = x;
        FLOAT temp_r = 0.0;
        FLOAT temp_i = 0.0;

        if (lda == 2 && inc_x == 2) {

            for (i = 0; i < (n & -2); i += 2) {
#if ( !defined(CONJ) && !defined(XCONJ) ) || ( defined(CONJ) && defined(XCONJ) )
                temp_r += a_ptr[0] * x_ptr[0] - a_ptr[1] * x_ptr[1];
                temp_i += a_ptr[0] * x_ptr[1] + a_ptr[1] * x_ptr[0];
                temp_r += a_ptr[2] * x_ptr[2] - a_ptr[3] * x_ptr[3];
                temp_i += a_ptr[2] * x_ptr[3] + a_ptr[3] * x_ptr[2];
#else
                temp_r += a_ptr[0] * x_ptr[0] + a_ptr[1] * x_ptr[1];
                temp_i += a_ptr[0] * x_ptr[1] - a_ptr[1] * x_ptr[0];
                temp_r += a_ptr[2] * x_ptr[2] + a_ptr[3] * x_ptr[3];
                temp_i += a_ptr[2] * x_ptr[3] - a_ptr[3] * x_ptr[2];
#endif

                a_ptr += 4;
                x_ptr += 4;
            }

            for (; i < n; i++) {
#if ( !defined(CONJ) && !defined(XCONJ) ) || ( defined(CONJ) && defined(XCONJ) )
                temp_r += a_ptr[0] * x_ptr[0] - a_ptr[1] * x_ptr[1];
                temp_i += a_ptr[0] * x_ptr[1] + a_ptr[1] * x_ptr[0];
#else
                temp_r += a_ptr[0] * x_ptr[0] + a_ptr[1] * x_ptr[1];
                temp_i += a_ptr[0] * x_ptr[1] - a_ptr[1] * x_ptr[0];
#endif

                a_ptr += 2;
                x_ptr += 2;
            }

        } else {

            for (i = 0; i < n; i++) {
#if ( !defined(CONJ) && !defined(XCONJ) ) || ( defined(CONJ) && defined(XCONJ) )
                temp_r += a_ptr[0] * x_ptr[0] - a_ptr[1] * x_ptr[1];
                temp_i += a_ptr[0] * x_ptr[1] + a_ptr[1] * x_ptr[0];
#else
                temp_r += a_ptr[0] * x_ptr[0] + a_ptr[1] * x_ptr[1];
                temp_i += a_ptr[0] * x_ptr[1] - a_ptr[1] * x_ptr[0];
#endif

                a_ptr += lda;
                x_ptr += inc_x;
            }

        }
#if !defined(XCONJ) 
        y_ptr[0] += alpha_r * temp_r - alpha_i * temp_i;
        y_ptr[1] += alpha_r * temp_i + alpha_i * temp_r;
#else
        y_ptr[0] += alpha_r * temp_r + alpha_i * temp_i;
        y_ptr[1] -= alpha_r * temp_i - alpha_i * temp_r;
#endif
        return (0);
    }

    if (m3 == 2) {
        a_ptr = a;
        x_ptr = x;
        FLOAT temp_r0 = 0.0;
        FLOAT temp_i0 = 0.0;
        FLOAT temp_r1 = 0.0;
        FLOAT temp_i1 = 0.0;

        if (lda == 4 && inc_x == 2) {

            for (i = 0; i < (n & -2); i += 2) {
#if ( !defined(CONJ) && !defined(XCONJ) ) || ( defined(CONJ) && defined(XCONJ) )

                temp_r0 += a_ptr[0] * x_ptr[0] - a_ptr[1] * x_ptr[1];
                temp_i0 += a_ptr[0] * x_ptr[1] + a_ptr[1] * x_ptr[0];
                temp_r1 += a_ptr[2] * x_ptr[0] - a_ptr[3] * x_ptr[1];
                temp_i1 += a_ptr[2] * x_ptr[1] + a_ptr[3] * x_ptr[0];

                temp_r0 += a_ptr[4] * x_ptr[2] - a_ptr[5] * x_ptr[3];
                temp_i0 += a_ptr[4] * x_ptr[3] + a_ptr[5] * x_ptr[2];
                temp_r1 += a_ptr[6] * x_ptr[2] - a_ptr[7] * x_ptr[3];
                temp_i1 += a_ptr[6] * x_ptr[3] + a_ptr[7] * x_ptr[2];
#else
                temp_r0 += a_ptr[0] * x_ptr[0] + a_ptr[1] * x_ptr[1];
                temp_i0 += a_ptr[0] * x_ptr[1] - a_ptr[1] * x_ptr[0];
                temp_r1 += a_ptr[2] * x_ptr[0] + a_ptr[3] * x_ptr[1];
                temp_i1 += a_ptr[2] * x_ptr[1] - a_ptr[3] * x_ptr[0];

                temp_r0 += a_ptr[4] * x_ptr[2] + a_ptr[5] * x_ptr[3];
                temp_i0 += a_ptr[4] * x_ptr[3] - a_ptr[5] * x_ptr[2];
                temp_r1 += a_ptr[6] * x_ptr[2] + a_ptr[7] * x_ptr[3];
                temp_i1 += a_ptr[6] * x_ptr[3] - a_ptr[7] * x_ptr[2];
#endif

                a_ptr += 8;
                x_ptr += 4;
            }

            for (; i < n; i++) {
#if ( !defined(CONJ) && !defined(XCONJ) ) || ( defined(CONJ) && defined(XCONJ) )
                temp_r0 += a_ptr[0] * x_ptr[0] - a_ptr[1] * x_ptr[1];
                temp_i0 += a_ptr[0] * x_ptr[1] + a_ptr[1] * x_ptr[0];
                temp_r1 += a_ptr[2] * x_ptr[0] - a_ptr[3] * x_ptr[1];
                temp_i1 += a_ptr[2] * x_ptr[1] + a_ptr[3] * x_ptr[0];
#else
                temp_r0 += a_ptr[0] * x_ptr[0] + a_ptr[1] * x_ptr[1];
                temp_i0 += a_ptr[0] * x_ptr[1] - a_ptr[1] * x_ptr[0];
                temp_r1 += a_ptr[2] * x_ptr[0] + a_ptr[3] * x_ptr[1];
                temp_i1 += a_ptr[2] * x_ptr[1] - a_ptr[3] * x_ptr[0];
#endif

                a_ptr += 4;
                x_ptr += 2;
            }

        } else {

            for (i = 0; i < n; i++) {
#if ( !defined(CONJ) && !defined(XCONJ) ) || ( defined(CONJ) && defined(XCONJ) )
                temp_r0 += a_ptr[0] * x_ptr[0] - a_ptr[1] * x_ptr[1];
                temp_i0 += a_ptr[0] * x_ptr[1] + a_ptr[1] * x_ptr[0];
                temp_r1 += a_ptr[2] * x_ptr[0] - a_ptr[3] * x_ptr[1];
                temp_i1 += a_ptr[2] * x_ptr[1] + a_ptr[3] * x_ptr[0];
#else
                temp_r0 += a_ptr[0] * x_ptr[0] + a_ptr[1] * x_ptr[1];
                temp_i0 += a_ptr[0] * x_ptr[1] - a_ptr[1] * x_ptr[0];
                temp_r1 += a_ptr[2] * x_ptr[0] + a_ptr[3] * x_ptr[1];
                temp_i1 += a_ptr[2] * x_ptr[1] - a_ptr[3] * x_ptr[0];
#endif

                a_ptr += lda;
                x_ptr += inc_x;
            }

        }
#if !defined(XCONJ) 
        y_ptr[0] += alpha_r * temp_r0 - alpha_i * temp_i0;
        y_ptr[1] += alpha_r * temp_i0 + alpha_i * temp_r0;
        y_ptr += inc_y;
        y_ptr[0] += alpha_r * temp_r1 - alpha_i * temp_i1;
        y_ptr[1] += alpha_r * temp_i1 + alpha_i * temp_r1;
#else
        y_ptr[0] += alpha_r * temp_r0 + alpha_i * temp_i0;
        y_ptr[1] -= alpha_r * temp_i0 - alpha_i * temp_r0;
        y_ptr += inc_y;
        y_ptr[0] += alpha_r * temp_r1 + alpha_i * temp_i1;
        y_ptr[1] -= alpha_r * temp_i1 - alpha_i * temp_r1;
#endif
        return (0);
    }

    if (m3 == 3) {
        a_ptr = a;
        x_ptr = x;
        FLOAT temp_r0 = 0.0;
        FLOAT temp_i0 = 0.0;
        FLOAT temp_r1 = 0.0;
        FLOAT temp_i1 = 0.0;
        FLOAT temp_r2 = 0.0;
        FLOAT temp_i2 = 0.0;

        if (lda == 6 && inc_x == 2) {

            for (i = 0; i < n; i++) {
#if ( !defined(CONJ) && !defined(XCONJ) ) || ( defined(CONJ) && defined(XCONJ) )
                temp_r0 += a_ptr[0] * x_ptr[0] - a_ptr[1] * x_ptr[1];
                temp_i0 += a_ptr[0] * x_ptr[1] + a_ptr[1] * x_ptr[0];
                temp_r1 += a_ptr[2] * x_ptr[0] - a_ptr[3] * x_ptr[1];
                temp_i1 += a_ptr[2] * x_ptr[1] + a_ptr[3] * x_ptr[0];
                temp_r2 += a_ptr[4] * x_ptr[0] - a_ptr[5] * x_ptr[1];
                temp_i2 += a_ptr[4] * x_ptr[1] + a_ptr[5] * x_ptr[0];
#else
                temp_r0 += a_ptr[0] * x_ptr[0] + a_ptr[1] * x_ptr[1];
                temp_i0 += a_ptr[0] * x_ptr[1] - a_ptr[1] * x_ptr[0];
                temp_r1 += a_ptr[2] * x_ptr[0] + a_ptr[3] * x_ptr[1];
                temp_i1 += a_ptr[2] * x_ptr[1] - a_ptr[3] * x_ptr[0];
                temp_r2 += a_ptr[4] * x_ptr[0] + a_ptr[5] * x_ptr[1];
                temp_i2 += a_ptr[4] * x_ptr[1] - a_ptr[5] * x_ptr[0];
#endif

                a_ptr += 6;
                x_ptr += 2;
            }

        } else {

            for (i = 0; i < n; i++) {
#if ( !defined(CONJ) && !defined(XCONJ) ) || ( defined(CONJ) && defined(XCONJ) )
                temp_r0 += a_ptr[0] * x_ptr[0] - a_ptr[1] * x_ptr[1];
                temp_i0 += a_ptr[0] * x_ptr[1] + a_ptr[1] * x_ptr[0];
                temp_r1 += a_ptr[2] * x_ptr[0] - a_ptr[3] * x_ptr[1];
                temp_i1 += a_ptr[2] * x_ptr[1] + a_ptr[3] * x_ptr[0];
                temp_r2 += a_ptr[4] * x_ptr[0] - a_ptr[5] * x_ptr[1];
                temp_i2 += a_ptr[4] * x_ptr[1] + a_ptr[5] * x_ptr[0];
#else
                temp_r0 += a_ptr[0] * x_ptr[0] + a_ptr[1] * x_ptr[1];
                temp_i0 += a_ptr[0] * x_ptr[1] - a_ptr[1] * x_ptr[0];
                temp_r1 += a_ptr[2] * x_ptr[0] + a_ptr[3] * x_ptr[1];
                temp_i1 += a_ptr[2] * x_ptr[1] - a_ptr[3] * x_ptr[0];
                temp_r2 += a_ptr[4] * x_ptr[0] + a_ptr[5] * x_ptr[1];
                temp_i2 += a_ptr[4] * x_ptr[1] - a_ptr[5] * x_ptr[0];
#endif

                a_ptr += lda;
                x_ptr += inc_x;
            }

        }
#if !defined(XCONJ) 
        y_ptr[0] += alpha_r * temp_r0 - alpha_i * temp_i0;
        y_ptr[1] += alpha_r * temp_i0 + alpha_i * temp_r0;
        y_ptr += inc_y;
        y_ptr[0] += alpha_r * temp_r1 - alpha_i * temp_i1;
        y_ptr[1] += alpha_r * temp_i1 + alpha_i * temp_r1;
        y_ptr += inc_y;
        y_ptr[0] += alpha_r * temp_r2 - alpha_i * temp_i2;
        y_ptr[1] += alpha_r * temp_i2 + alpha_i * temp_r2;
#else
        y_ptr[0] += alpha_r * temp_r0 + alpha_i * temp_i0;
        y_ptr[1] -= alpha_r * temp_i0 - alpha_i * temp_r0;
        y_ptr += inc_y;
        y_ptr[0] += alpha_r * temp_r1 + alpha_i * temp_i1;
        y_ptr[1] -= alpha_r * temp_i1 - alpha_i * temp_r1;
        y_ptr += inc_y;
        y_ptr[0] += alpha_r * temp_r2 + alpha_i * temp_i2;
        y_ptr[1] -= alpha_r * temp_i2 - alpha_i * temp_r2;
#endif
        return (0);
    }

    return (0);
}
#endif
