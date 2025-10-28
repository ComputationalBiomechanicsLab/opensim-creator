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

/**************************************************************************************
* 2013/09/14 Saar
*    BLASTEST float     : OK
*    BLASTEST double    : OK
*    CTEST          : OK
*    TEST           : OK
*
**************************************************************************************/

#include "common.h"

// The c/zscal_k function is called not only by cblas_c/zscal but also by other upper-level interfaces.
// In certain cases, the expected return values for cblas_s/zscal differ from those of other upper-level interfaces.
// To handle this, we use the dummy2 parameter to differentiate between them.
int CNAME(BLASLONG n, BLASLONG dummy0, BLASLONG dummy1, FLOAT da_r,FLOAT da_i, FLOAT *x, BLASLONG inc_x, FLOAT *y, BLASLONG inc_y, FLOAT *dummy, BLASLONG dummy2)
{
    BLASLONG i = 0;
    BLASLONG inc_x2;
    BLASLONG ip = 0;
    FLOAT temp;

    if ((n <= 0) || (inc_x <= 0))
        return(0);

    inc_x2 = 2 * inc_x;
    if (dummy2 == 0) {
        for (i = 0; i < n; i++)
        {
            if (da_r == 0.0 && da_i == 0.0)
            {
                x[ip] = 0.0;
                x[ip+1] = 0.0;
            }
            else
            {
                temp    = da_r * x[ip]   - da_i * x[ip+1];
                x[ip+1] = da_r * x[ip+1] + da_i * x[ip]  ;
                x[ip] = temp;
            }

            ip += inc_x2;
        }
        return(0);
    }
    for (i = 0; i < n; i++)
    {
        temp    = da_r * x[ip]   - da_i * x[ip+1];
        x[ip+1] = da_r * x[ip+1] + da_i * x[ip]  ;

        x[ip]   = temp;
        ip += inc_x2;
    }

    return(0);
}
