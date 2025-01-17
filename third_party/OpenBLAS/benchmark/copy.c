/***************************************************************************
Copyright (c) 2014, The OpenBLAS Project
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

#include "bench.h"

#undef COPY

#ifdef COMPLEX
#ifdef DOUBLE
#define COPY   BLASFUNC(zcopy)
#else
#define COPY   BLASFUNC(ccopy)
#endif
#else
#ifdef DOUBLE
#define COPY   BLASFUNC(dcopy)
#else
#define COPY   BLASFUNC(scopy)
#endif
#endif

int main(int argc, char *argv[]){

  FLOAT *x, *y;
  FLOAT alpha[2] = { 2.0, 2.0 };
  blasint m, i;
  blasint inc_x=1,inc_y=1;
  int loops = 1;
  int l;
  char *p;

  int from =   1;
  int to   = 200;
  int step =   1;

  double time1 = 0.0, timeg = 0.0;
  long nanos = 0;
  time_t seconds = 0;

  argc--;argv++;

  if (argc > 0) { from     = atol(*argv);		argc--; argv++;}
  if (argc > 0) { to       = MAX(atol(*argv), from);	argc--; argv++;}
  if (argc > 0) { step     = atol(*argv);		argc--; argv++;}

  if ((p = getenv("OPENBLAS_LOOPS")))  loops = atoi(p);
  if ((p = getenv("OPENBLAS_INCX")))   inc_x = atoi(p);
  if ((p = getenv("OPENBLAS_INCY")))   inc_y = atoi(p);

  fprintf(stderr, "From : %3d  To : %3d Step = %3d Inc_x = %d Inc_y = %d Loops = %d\n", from, to, step,inc_x,inc_y,loops);

  if (( x = (FLOAT *)malloc(sizeof(FLOAT) * to * abs(inc_x) * COMPSIZE)) == NULL){
    fprintf(stderr,"Out of Memory!!\n");exit(1);
  }

  if (( y = (FLOAT *)malloc(sizeof(FLOAT) * to * abs(inc_y) * COMPSIZE)) == NULL){
    fprintf(stderr,"Out of Memory!!\n");exit(1);
  }

#ifdef __linux
  srandom(getpid());
#endif

  fprintf(stderr, "   SIZE       Flops\n");

  for(m = from; m <= to; m += step)
  {

   timeg=0;

   fprintf(stderr, " %6d : ", (int)m);
   for(i = 0; i < m * COMPSIZE * abs(inc_x); i++){
       x[i] = ((FLOAT) rand() / (FLOAT) RAND_MAX) - 0.5;
   }

   for(i = 0; i < m * COMPSIZE * abs(inc_y); i++){
       y[i] = ((FLOAT) rand() / (FLOAT) RAND_MAX) - 0.5;
   }

   for (l=0; l<loops; l++)
   {
       begin();
       COPY (&m, x, &inc_x, y, &inc_y );
       end();
       timeg += getsec();
   }

      timeg /= loops;

      fprintf(stderr,
	    " %10.2f MBytes %12.9f sec\n",
	    COMPSIZE * sizeof(FLOAT) * 1. * (double)m / timeg / 1.e6, timeg);

  }

  return 0;
}

// void main(int argc, char *argv[]) __attribute__((weak, alias("MAIN__")));
