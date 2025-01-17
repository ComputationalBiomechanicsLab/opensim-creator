/***************************************************************************
Copyright (c) 2014-2015, The OpenBLAS Project
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

#define HAVE_KERNEL_8 1

static void dscal_kernel_8( BLASLONG n, FLOAT *alpha, FLOAT *x) __attribute__ ((noinline));

static void dscal_kernel_8( BLASLONG n, FLOAT *alpha, FLOAT *x)
{


	BLASLONG n1 = n >> 4 ;
	BLASLONG n2 = n & 8   ;

	__asm__  __volatile__
	(
	"vmovddup		(%2), %%xmm0		    \n\t"  // alpha	

	"addq	$128, %1				    \n\t"

	"cmpq 	$0, %0					    \n\t"
	"je	4f					    \n\t" 

	"vmulpd 	-128(%1), %%xmm0, %%xmm4	    \n\t"
	"vmulpd 	-112(%1), %%xmm0, %%xmm5	    \n\t"
	"vmulpd 	 -96(%1), %%xmm0, %%xmm6	    \n\t"
	"vmulpd 	 -80(%1), %%xmm0, %%xmm7	    \n\t"

	"vmulpd 	 -64(%1), %%xmm0, %%xmm8	    \n\t"
	"vmulpd 	 -48(%1), %%xmm0, %%xmm9	    \n\t"
	"vmulpd 	 -32(%1), %%xmm0, %%xmm10    	    \n\t"
	"vmulpd 	 -16(%1), %%xmm0, %%xmm11           \n\t"

	"subq	        $1 , %0			            \n\t"		
	"jz		2f		             	    \n\t"

	".align 16				            \n\t"
	"1:				            	    \n\t"
	"prefetcht0     256(%1)				    \n\t" 

	"vmovups	%%xmm4  ,-128(%1)		    \n\t"
	"vmovups	%%xmm5  ,-112(%1)		    \n\t"
	"vmulpd 	   0(%1), %%xmm0, %%xmm4	    \n\t"
	"vmovups	%%xmm6  , -96(%1)		    \n\t"
	"vmulpd 	  16(%1), %%xmm0, %%xmm5	    \n\t"
	"vmovups	%%xmm7  , -80(%1)		    \n\t"
	"vmulpd 	  32(%1), %%xmm0, %%xmm6	    \n\t"

	"prefetcht0     320(%1)				    \n\t" 

	"vmovups	%%xmm8  , -64(%1)		    \n\t"
	"vmulpd 	  48(%1), %%xmm0, %%xmm7	    \n\t"
	"vmovups	%%xmm9  , -48(%1)		    \n\t"
	"vmulpd 	  64(%1), %%xmm0, %%xmm8	    \n\t"
	"vmovups	%%xmm10 , -32(%1)		    \n\t"
	"vmulpd 	  80(%1), %%xmm0, %%xmm9	    \n\t"
	"vmovups	%%xmm11 , -16(%1)		    \n\t"

	"vmulpd 	  96(%1), %%xmm0, %%xmm10    	    \n\t"
	"vmulpd 	 112(%1), %%xmm0, %%xmm11           \n\t"


	"addq		$128, %1	  	 	    \n\t"
	"subq	        $1 , %0			            \n\t"		
	"jnz		1b		             	    \n\t"

	"2:				            	    \n\t"
 
	"vmovups	%%xmm4  ,-128(%1)		    \n\t"
	"vmovups	%%xmm5  ,-112(%1)		    \n\t"
	"vmovups	%%xmm6  , -96(%1)		    \n\t"
	"vmovups	%%xmm7  , -80(%1)		    \n\t"

	"vmovups	%%xmm8  , -64(%1)		    \n\t"
	"vmovups	%%xmm9  , -48(%1)		    \n\t"
	"vmovups	%%xmm10 , -32(%1)		    \n\t"
	"vmovups	%%xmm11 , -16(%1)		    \n\t"

	"addq		$128, %1	  	 	    \n\t"

	"4:				            	    \n\t"

	"cmpq	$8  ,%3					    \n\t"
	"jne	5f					    \n\t"

	"vmulpd	    -128(%1), %%xmm0, %%xmm4	    \n\t"
	"vmulpd	    -112(%1), %%xmm0, %%xmm5	    \n\t"
	"vmulpd	     -96(%1), %%xmm0, %%xmm6     	    \n\t"
	"vmulpd	     -80(%1), %%xmm0, %%xmm7     	    \n\t"

	"vmovups	%%xmm4  ,-128(%1)		    \n\t"
	"vmovups	%%xmm5  ,-112(%1)		    \n\t"
	"vmovups	%%xmm6  , -96(%1)		    \n\t"
	"vmovups	%%xmm7  , -80(%1)		    \n\t"

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


static void dscal_kernel_8_zero( BLASLONG n, FLOAT *alpha, FLOAT *x) __attribute__ ((noinline));

static void dscal_kernel_8_zero( BLASLONG n, FLOAT *alpha, FLOAT *x)
{


	BLASLONG n1 = n >> 4 ;
	BLASLONG n2 = n & 8 ;

	__asm__  __volatile__
	(
	"vxorpd		%%xmm0, %%xmm0 , %%xmm0		    \n\t"  

	"addq	$128, %1				    \n\t"

	"cmpq 	$0, %0					    \n\t"
	"je	2f					    \n\t" 

	".align 16				            \n\t"
	"1:				            	    \n\t"

	"vmovups	%%xmm0  ,-128(%1)		    \n\t"
	"vmovups	%%xmm0  ,-112(%1)		    \n\t"
	"vmovups	%%xmm0  , -96(%1)		    \n\t"
	"vmovups	%%xmm0  , -80(%1)		    \n\t"

	"vmovups	%%xmm0  , -64(%1)		    \n\t"
	"vmovups	%%xmm0  , -48(%1)		    \n\t"
	"vmovups	%%xmm0  , -32(%1)		    \n\t"
	"vmovups	%%xmm0  , -16(%1)		    \n\t"

	"addq		$128, %1	  	 	    \n\t"
	"subq	        $1 , %0			            \n\t"		
	"jnz		1b		             	    \n\t"

	"2:				            	    \n\t"

	"cmpq	$8  ,%3					    \n\t"
	"jne	4f					    \n\t"

	"vmovups	%%xmm0  ,-128(%1)		    \n\t"
	"vmovups	%%xmm0  ,-112(%1)		    \n\t"
	"vmovups	%%xmm0  , -96(%1)		    \n\t"
	"vmovups	%%xmm0  , -80(%1)		    \n\t"

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


