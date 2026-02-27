/***************************************************************************
Copyright (c) 2013, 2025 The OpenBLAS Project
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

#include "conversion_macros.h"

int CNAME(BLASLONG m, BLASLONG n, FLOAT alpha, IFLOAT *a, BLASLONG lda, IFLOAT *x, BLASLONG inc_x, FLOAT beta, FLOAT *y, BLASLONG inc_y)
{
    BLASLONG i;
    BLASLONG ix, iy;
    BLASLONG j;
    IFLOAT *a_ptr;
#if defined(BGEMM) || defined(HGEMM)
    float temp;
#else
    FLOAT temp;
#endif

    iy = 0;
    a_ptr = a;

    for (j = 0; j < n; j++)
    {
        temp = 0.0;
        ix = 0;
        for (i = 0; i < m; i++)
        {
            temp += TO_F32(a_ptr[i]) * TO_F32(x[ix]);
            ix += inc_x;
        }
        if (BETA == ZERO)
        {
            y[iy] = TO_OUTPUT(ALPHA * temp);
        }
        else
        {
            y[iy] = TO_OUTPUT(ALPHA * temp + BETA * TO_F32(y[iy]));
        }
        iy += inc_y;
        a_ptr += lda;
    }
    return (0);
}
