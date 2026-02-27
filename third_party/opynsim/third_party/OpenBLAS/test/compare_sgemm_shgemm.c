/***************************************************************************
Copyright (c) 2020,2025 The OpenBLAS Project
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
#include <stdio.h>
#include <stdint.h>
#include "../common.h"

#include "test_helpers.h"

#define SGEMM   BLASFUNC(sgemm)
#define SHGEMM   BLASFUNC(shgemm)
#define SHGEMM_LARGEST  256

int
main (int argc, char *argv[])
{
  blasint m, n, k;
  int i, j, l;
  blasint x, y;
  int ret = 0;
  int rret = 0;
  int loop = SHGEMM_LARGEST;
  char transA = 'N', transB = 'N';
  float alpha = 1.0, beta = 0.0;
  int xvals[6]={3,24,55,71,SHGEMM_LARGEST/2,SHGEMM_LARGEST};

  for (x = 0; x <= loop; x++)
  {
    if ((x > 100) && (x != SHGEMM_LARGEST)) continue;
    m = k = n = x;
    float *A = (float *)malloc_safe(m * k * sizeof(FLOAT));
    float *B = (float *)malloc_safe(k * n * sizeof(FLOAT));
    float *C = (float *)malloc_safe(m * n * sizeof(FLOAT));
    _Float16 *AA = (_Float16 *)malloc_safe(m * k * sizeof(_Float16));
    _Float16 *BB = (_Float16 *)malloc_safe(k * n * sizeof(_Float16));
    float *DD = (float *)malloc_safe(m * n * sizeof(FLOAT));
    float *CC = (float *)malloc_safe(m * n * sizeof(FLOAT));
    if ((A == NULL) || (B == NULL) || (C == NULL) || (AA == NULL) || (BB == NULL) ||
        (DD == NULL) || (CC == NULL))
      return 1;

    for (j = 0; j < m; j++)
    {
      for (i = 0; i < k; i++)
      {
        A[j * k + i] = ((FLOAT) rand () / (FLOAT) RAND_MAX) + 0.5;
        AA[j * k + i] = (_Float16) A[j * k + i];
      }
    }
    for (j = 0; j < n; j++)
    {
      for (i = 0; i < k; i++)
      {
        B[j * k + i] = ((FLOAT) rand () / (FLOAT) RAND_MAX) + 0.5;
        BB[j * k + i] = (_Float16) B[j * k + i];
      }
    }
    for (y = 0; y < 4; y++)
    {
      if ((y == 0) || (y == 2)) {
        transA = 'N';
      } else {
        transA = 'T';
      }
      if ((y == 0) || (y == 1)) {
        transB = 'N';
      } else {
        transB = 'T';
      }

      memset(CC, 0, m * n * sizeof(FLOAT));
      memset(DD, 0, m * n * sizeof(FLOAT));
      memset(C, 0, m * n * sizeof(FLOAT));

      SGEMM (&transA, &transB, &m, &n, &k, &alpha, A,
        &m, B, &k, &beta, C, &m);
      SHGEMM (&transA, &transB, &m, &n, &k, &alpha, (_Float16*) AA,
        &m, (_Float16*)BB, &k, &beta, CC, &m);

      for (i = 0; i < n; i++)
        for (j = 0; j < m; j++)
        {
          for (l = 0; l < k; l++)
            if (transA == 'N' && transB == 'N')
            {
              DD[i * m + j] +=
                (float) AA[l * m + j] * (float)BB[l + k * i];
            } else if (transA == 'T' && transB == 'N')
            {
              DD[i * m + j] +=
                (float)AA[k * j + l] * (float)BB[l + k * i];
            } else if (transA == 'N' && transB == 'T')
            {
              DD[i * m + j] +=
                (float)AA[l * m + j] * (float)BB[i + l * n];
            } else if (transA == 'T' && transB == 'T')
            {
              DD[i * m + j] +=
                (float)AA[k * j + l] * (float)BB[i + l * n];
            }
          if (!is_close(CC[i * m + j], C[i * m + j], 0.01, 0.001)) {
		  fprintf(stderr,"CC %f C %f \n",(float)CC[i*m+j],C[i*m+j]);
            ret++;
          }
          if (!is_close(CC[i * m + j], DD[i * m + j], 0.001, 0.0001)) {
		  fprintf(stderr,"CC %f DD  %f \n",(float)CC[i*m+j],(float)DD[i*m+j]);
            ret++;
          }
        }
    }
    free(A);
    free(B);
    free(C);
    free(AA);
    free(BB);
    free(DD);
    free(CC);
  }
  if (ret != 0) {
    fprintf(stderr, "SHGEMM FAILURES: %d!!!\n", ret);
    return 1;
  }

 
  for (loop = 0; loop<6; loop++) {
  x=xvals[loop];
  for (alpha=0.;alpha<=1.;alpha+=0.5)  
  {
   for (beta = 0.0; beta <=1.; beta+=0.5) {
   
    m = k = n = x;
    float *A = (float *)malloc_safe(m * k * sizeof(FLOAT));
    float *B = (float *)malloc_safe(k * n * sizeof(FLOAT));
    float *C = (float *)malloc_safe(m * n * sizeof(FLOAT));
    _Float16 *AA = (_Float16 *)malloc_safe(m * k * sizeof(_Float16));
    _Float16 *BB = (_Float16 *)malloc_safe(k * n * sizeof(_Float16));
    float *CC = (float *)malloc_safe(m * n * sizeof(FLOAT));
    if ((A == NULL) || (B == NULL) || (C == NULL) || (AA == NULL) || (BB == NULL) ||
       (CC == NULL))
      return 1;

    for (j = 0; j < m; j++)
    {
      for (i = 0; i < k; i++)
      {
        A[j * k + i] = ((FLOAT) rand () / (FLOAT) RAND_MAX) + 0.5;
        AA[j * k + i] = (_Float16) A[j * k + i];
      }
    }
    for (j = 0; j < n; j++)
    {
      for (i = 0; i < k; i++)
      {
        B[j * k + i] = ((FLOAT) rand () / (FLOAT) RAND_MAX) + 0.5;
        BB[j * k + i] = (_Float16) B[j * k + i];
      }
    }

    for (y = 0; y < 4; y++)
    {
      if ((y == 0) || (y == 2)) {
        transA = 'N';
      } else {
        transA = 'T';
      }
      if ((y == 0) || (y == 1)) {
        transB = 'N';
      } else {
        transB = 'T';
      }

      memset(CC, 0, m * n * sizeof(FLOAT));
      memset(C, 0, m * n * sizeof(FLOAT));

      SGEMM (&transA, &transB, &m, &n, &k, &alpha, A,
        &m, B, &k, &beta, C, &m);
      SHGEMM (&transA, &transB, &m, &n, &k, &alpha, (_Float16*) AA,
        &m, (_Float16*)BB, &k, &beta, CC, &m);

      for (i = 0; i < n; i++)
        for (j = 0; j < m; j++)
        {
          if (!is_close(CC[i * m + j], C[i * m + j], 0.01, 0.001)) {
            ret++;
          }
        }
    }
    free(A);
    free(B);
    free(C);
    free(AA);
    free(BB);
    free(CC);

    if (ret != 0) {
/*
 * fprintf(stderr, "SHGEMM FAILURES FOR n=%d, alpha=%f beta=%f : %d\n", x, alpha, beta, ret);
   */
      rret++;
      ret=0;
/*    }  else {
      fprintf(stderr, "SHGEMM SUCCEEDED FOR n=%d, alpha=%f beta=%f : %d\n", x, alpha, beta, ret);
*/
    }
  }
  
  }
  }
  if (rret > 0) return(1);
  return(0);
}
