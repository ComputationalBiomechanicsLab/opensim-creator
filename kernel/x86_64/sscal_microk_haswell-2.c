/***************************************************************************
Copyright (c) 2014-2022, The OpenBLAS Project
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

#define HAVE_KERNEL_16 1

static void sscal_kernel_16( BLASLONG n, FLOAT *alpha, FLOAT *x) __attribute__ ((noinline));

static void sscal_kernel_16( BLASLONG n, FLOAT *alpha, FLOAT *x)
{


	BLASLONG n1 = n >> 5 ;
	BLASLONG n2 = n & 16 ;

	__asm__  __volatile__
	(
	"vbroadcastss		(%2), %%ymm0		    \n\t"  // alpha

	"addq	$128, %1				    \n\t"

	"cmpq 	$0, %0					    \n\t"
	"je	4f					    \n\t"

	"vmulps 	-128(%1), %%ymm0, %%ymm4	    \n\t"
	"vmulps 	 -96(%1), %%ymm0, %%ymm5	    \n\t"

	"vmulps 	 -64(%1), %%ymm0, %%ymm6	    \n\t"
	"vmulps 	 -32(%1), %%ymm0, %%ymm7	    \n\t"

	"subq	        $1 , %0			            \n\t"
	"jz		2f		             	    \n\t"

	".p2align 4				            \n\t"
	"1:				            	    \n\t"
	// "prefetcht0     640(%1)				    \n\t"

	"vmovups	%%ymm4  ,-128(%1)		    \n\t"
	"vmovups	%%ymm5  , -96(%1)		    \n\t"
	"vmulps 	   0(%1), %%ymm0, %%ymm4	    \n\t"

	// "prefetcht0     704(%1)				    \n\t"

	"vmovups	%%ymm6  , -64(%1)		    \n\t"
	"vmulps 	  32(%1), %%ymm0, %%ymm5	    \n\t"
	"vmovups	%%ymm7  , -32(%1)		    \n\t"

	"vmulps 	  64(%1), %%ymm0, %%ymm6	    \n\t"
	"vmulps 	  96(%1), %%ymm0, %%ymm7	    \n\t"


	"addq		$128, %1	  	 	    \n\t"
	"subq	        $1 , %0			            \n\t"
	"jnz		1b		             	    \n\t"

	"2:				            	    \n\t"

	"vmovups	%%ymm4  ,-128(%1)		    \n\t"
	"vmovups	%%ymm5  , -96(%1)		    \n\t"

	"vmovups	%%ymm6  , -64(%1)		    \n\t"
	"vmovups	%%ymm7  , -32(%1)		    \n\t"

	"addq		$128, %1	  	 	    \n\t"

	"4:				            	    \n\t"

	"cmpq	$16 ,%3					    \n\t"
	"jne	5f					    \n\t"

	"vmulps	    -128(%1), %%ymm0, %%ymm4	    \n\t"
	"vmulps	     -96(%1), %%ymm0, %%ymm5     	    \n\t"

	"vmovups	%%ymm4  ,-128(%1)		    \n\t"
	"vmovups	%%ymm5  , -96(%1)		    \n\t"

	"5:						    \n\t"

	"vzeroupper					    \n\t"

	:
	  "+r" (n1),  	// 0
          "+r" (x)      // 1
	:
          "r" (alpha),  // 2
	  "r" (n2)   	// 3
	: "cc",
	  "%xmm0", "%xmm1", "%xmm2", "%xmm3",
	  "%xmm4", "%xmm5", "%xmm6", "%xmm7",
	  "%xmm8", "%xmm9", "%xmm10", "%xmm11",
	  "%xmm12", "%xmm13", "%xmm14", "%xmm15",
	  "memory"
	);

}


static void sscal_kernel_16_zero( BLASLONG n, FLOAT *alpha, FLOAT *x) __attribute__ ((noinline));

static void sscal_kernel_16_zero( BLASLONG n, FLOAT *alpha, FLOAT *x)
{


	BLASLONG n1 = n >> 5 ;
	BLASLONG n2 = n & 16 ;

	__asm__  __volatile__
	(
	"vxorpd		%%ymm0, %%ymm0 , %%ymm0		    \n\t"

	"addq	$128, %1				    \n\t"

	"cmpq 	$0, %0					    \n\t"
	"je	2f					    \n\t"

	".p2align 4				            \n\t"
	"1:				            	    \n\t"

	"vmovups	%%ymm0  ,-128(%1)		    \n\t"
	"vmovups	%%ymm0  , -96(%1)		    \n\t"

	"vmovups	%%ymm0  , -64(%1)		    \n\t"
	"vmovups	%%ymm0  , -32(%1)		    \n\t"

	"addq		$128, %1	  	 	    \n\t"
	"subq	        $1 , %0			            \n\t"
	"jnz		1b		             	    \n\t"

	"2:				            	    \n\t"

	"cmpq	$16 ,%3					    \n\t"
	"jne	4f					    \n\t"

	"vmovups	%%ymm0  ,-128(%1)		    \n\t"
	"vmovups	%%ymm0  , -96(%1)		    \n\t"

	"4:						    \n\t"

	"vzeroupper					    \n\t"

	:
	  "+r" (n1),  	// 0
          "+r" (x)      // 1
	:
          "r" (alpha),  // 2
	  "r" (n2)   	// 3
	: "cc",
	  "%xmm0", "%xmm1", "%xmm2", "%xmm3",
	  "%xmm4", "%xmm5", "%xmm6", "%xmm7",
	  "%xmm8", "%xmm9", "%xmm10", "%xmm11",
	  "%xmm12", "%xmm13", "%xmm14", "%xmm15",
	  "memory"
	);

}


