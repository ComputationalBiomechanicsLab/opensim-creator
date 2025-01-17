/*********************************************************************/
/* Copyright 2009, 2010 The University of Texas at Austin.           */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include <stdio.h>
#include "common.h"

#ifndef MINUS
#define a2	(a1 + 1)
#define a4	(a3 + 1)
#else
#define a2	(a1 - 1)
#define a4	(a3 - 1)
#endif

int CNAME(BLASLONG n, BLASLONG k1, BLASLONG k2, FLOAT dummy1, FLOAT *a, BLASLONG lda,
	 FLOAT *dummy2, BLASLONG dumy3, blasint *ipiv, BLASLONG incx){

  BLASLONG i, j, ip1, ip2, rows;
  blasint *piv;
  FLOAT *a1, *a3;
  FLOAT *b1, *b2, *b3, *b4;
  FLOAT A1, A2, B1, B2, A3, A4, B3, B4;

  a--;
  k1 --;

 ipiv += k1;
#ifdef MINUS
  ipiv -= (k2 - k1 - 1) * incx;
#endif

  if (n  <= 0) return 0;

  j = (n >> 1);
  rows = k2-k1;
  if (rows <=0) return 0;
  if (rows == 1) {
    //Only have 1 row
    ip1 = *ipiv;
    a1 = a + k1 + 1;
    b1 = a + ip1;

    if(a1 == b1) return 0;

    for(j=0; j<n; j++){
	A1 = *a1;
	B1 = *b1;
	*a1 = B1;
	*b1 = A1;

	a1 += lda;
	b1 += lda;
    }
    return 0;
  }

  if (j > 0) {
    do {
      piv = ipiv;

#ifndef MINUS
      a1 = a + k1 + 1;
#else
      a1 = a + k2;
#endif

      a3 = a1 + 1 * lda;

      ip1 = *piv;
      piv += incx;
      ip2 = *piv;
      piv += incx;

      b1 = a + ip1;
      b2 = a + ip2;

      b3 = b1 + 1 * lda;
      b4 = b2 + 1 * lda;

      i = ((rows) >> 1);

      // Loop pipeline
      i--;

      //Main Loop
      while (i > 0) {
#ifdef CORE2
#ifndef MINUS
	  asm volatile("prefetcht0  1 * 64(%0)\n"  : : "r"(b1));
	  asm volatile("prefetcht0  1 * 64(%0)\n"  : : "r"(b3));
	  asm volatile("prefetcht0  1 * 64(%0)\n"  : : "r"(a1));
	  asm volatile("prefetcht0  1 * 64(%0)\n"  : : "r"(a3));
#else
	  asm volatile("prefetcht0 -1 * 64(%0)\n"  : : "r"(b1));
	  asm volatile("prefetcht0 -1 * 64(%0)\n"  : : "r"(b3));
	  asm volatile("prefetcht0 -1 * 64(%0)\n"  : : "r"(a1));
	  asm volatile("prefetcht0 -1 * 64(%0)\n"  : : "r"(a3));
#endif
#endif
	  B1 = *b1;
	  B2 = *b2;
	  B3 = *b3;
	  B4 = *b4;

	  A1 = *a1;
	  A2 = *a2;
	  A3 = *a3;
	  A4 = *a4;

	  ip1 = *piv;
	  piv += incx;
	  ip2 = *piv;
	  piv += incx;

	  if (b1 == a1) {
	    if (b2 == a1) {
	      *a1 = A2;
	      *a2 = A1;
	      *a3 = A4;
	      *a4 = A3;
	    } else
	      if (b2 != a2) {
		*a2 = B2;
		*b2 = A2;
		*a4 = B4;
		*b4 = A4;
	      }
	  } else
	    if (b1 == a2) {
	      if (b2 != a1) {
		if (b2 == a2) {
		  *a1 = A2;
		  *a2 = A1;
		  *a3 = A4;
		  *a4 = A3;
		} else {
		  *a1 = A2;
		  *a2 = B2;
		  *b2 = A1;
		  *a3 = A4;
		  *a4 = B4;
		  *b4 = A3;
		}
	      }
	    } else {
	      if (b2 == a1) {
		*a1 = A2;
		*a2 = B1;
		*b1 = A1;
		*a3 = A4;
		*a4 = B3;
		*b3 = A3;
	      } else
		if (b2 == a2) {
		  *a1 = B1;
		  *b1 = A1;
		  *a3 = B3;
		  *b3 = A3;
		} else
		  if (b2 == b1) {
		    *a1 = B1;
		    *a2 = A1;
		    *b1 = A2;
		    *a3 = B3;
		    *a4 = A3;
		    *b3 = A4;
		  } else {
		    *a1 = B1;
		    *a2 = B2;
		    *b1 = A1;
		    *b2 = A2;
		    *a3 = B3;
		    *a4 = B4;
		    *b3 = A3;
		    *b4 = A4;
		  }
	    }

	  b1 = a + ip1;
	  b2 = a + ip2;

	  b3 = b1 + 1 * lda;
	  b4 = b2 + 1 * lda;

#ifndef MINUS
	  a1 += 2;
	  a3 += 2;
#else
	  a1 -= 2;
	  a3 -= 2;
#endif
	  i --;
      }

      //Loop Ending
      B1 = *b1;
      B2 = *b2;
      B3 = *b3;
      B4 = *b4;

      A1 = *a1;
      A2 = *a2;
      A3 = *a3;
      A4 = *a4;

      if (b1 == a1) {
	if (b2 == a1) {
	  *a1 = A2;
	  *a2 = A1;
	  *a3 = A4;
	  *a4 = A3;
	} else
	  if (b2 != a2) {
	    *a2 = B2;
	    *b2 = A2;
	    *a4 = B4;
	    *b4 = A4;
	  }
      } else
	if (b1 == a2) {
	  if (b2 != a1) {
	    if (b2 == a2) {
	      *a1 = A2;
	      *a2 = A1;
	      *a3 = A4;
	      *a4 = A3;
	    } else {
	      *a1 = A2;
	      *a2 = B2;
	      *b2 = A1;
	      *a3 = A4;
	      *a4 = B4;
	      *b4 = A3;
	    }
	  }
	} else {
	  if (b2 == a1) {
	    *a1 = A2;
	    *a2 = B1;
	    *b1 = A1;
	    *a3 = A4;
	    *a4 = B3;
	    *b3 = A3;
	  } else
	    if (b2 == a2) {
	      *a1 = B1;
	      *b1 = A1;
	      *a3 = B3;
	      *b3 = A3;
	    } else
	      if (b2 == b1) {
		*a1 = B1;
		*a2 = A1;
		*b1 = A2;
		*a3 = B3;
		*a4 = A3;
		*b3 = A4;
	      } else {
		*a1 = B1;
		*a2 = B2;
		*b1 = A1;
		*b2 = A2;
		*a3 = B3;
		*a4 = B4;
		*b3 = A3;
		*b4 = A4;
	      }
	}
#ifndef MINUS
      a1 += 2;
      a3 += 2;
#else
      a1 -= 2;
      a3 -= 2;
#endif

      //Remain
      i = ((rows) & 1);

      if (i > 0) {
	ip1 = *piv;
	b1 = a + ip1;
	b3 = b1 + 1 * lda;


	A1 = *a1;
	B1 = *b1;
	A3 = *a3;
	B3 = *b3;
	*a1 = B1;
	*b1 = A1;
	*a3 = B3;
	*b3 = A3;
      }

      a += 2 * lda;
      j --;
    } while (j > 0);
  }

  if (n & 1) {
    piv = ipiv;

#ifndef MINUS
    a1 = a + k1 + 1;
#else
    a1 = a + k2;
#endif

    ip1 = *piv;
    piv += incx;
    ip2 = *piv;
    piv += incx;

    b1 = a + ip1;
    b2 = a + ip2;

    i = ((rows) >> 1);
    i --;

    while (i > 0) {
      A1 = *a1;
      A2 = *a2;
      B1 = *b1;
      B2 = *b2;

      ip1 = *piv;
      piv += incx;
      ip2 = *piv;
      piv += incx;

      if (b1 == a1) {
	if (b2 == a1) {
	  *a1 = A2;
	  *a2 = A1;
	} else
	  if (b2 != a2) {
	    *a2 = B2;
	    *b2 = A2;
	  }
      } else
	if (b1 == a2) {
	  if (b2 != a1) {
	    if (b2 == a2) {
	      *a1 = A2;
	      *a2 = A1;
	    } else {
	      *a1 = A2;
	      *a2 = B2;
	      *b2 = A1;
	    }
	  }
	} else {
	  if (b2 == a1) {
	    *a1 = A2;
	    *a2 = B1;
	    *b1 = A1;
	  } else
	    if (b2 == a2) {
	      *a1 = B1;
	      *b1 = A1;
	    } else
	      if (b2 == b1) {
		*a1 = B1;
		*a2 = A1;
		*b1 = A2;
	      } else {
		*a1 = B1;
		*a2 = B2;
		*b1 = A1;
		*b2 = A2;
	      }
	}

      b1 = a + ip1;
      b2 = a + ip2;

#ifndef MINUS
      a1 += 2;
#else
      a1 -= 2;
#endif
      i --;
    }

    //Loop Ending (n=1)
    A1 = *a1;
    A2 = *a2;
    B1 = *b1;
    B2 = *b2;
    if (b1 == a1) {
      if (b2 == a1) {
	*a1 = A2;
	*a2 = A1;
      } else
	if (b2 != a2) {
	  *a2 = B2;
	  *b2 = A2;
	}
    } else
      if (b1 == a2) {
	if (b2 != a1) {
	  if (b2 == a2) {
	    *a1 = A2;
	    *a2 = A1;
	  } else {
	    *a1 = A2;
	    *a2 = B2;
	    *b2 = A1;
	  }
	}
      } else {
	if (b2 == a1) {
	  *a1 = A2;
	  *a2 = B1;
	  *b1 = A1;
	} else
	  if (b2 == a2) {
	    *a1 = B1;
	    *b1 = A1;
	  } else
	    if (b2 == b1) {
	      *a1 = B1;
	      *a2 = A1;
	      *b1 = A2;
	    } else {
	      *a1 = B1;
	      *a2 = B2;
	      *b1 = A1;
	      *b2 = A2;
	    }
      }

#ifndef MINUS
    a1 += 2;
#else
    a1 -= 2;
#endif

    //Remain
    i = (rows & 1);

    if (i > 0) {
      ip1 = *piv;
      b1 = a + ip1;

      A1 = *a1;
      B1 = *b1;
      *a1 = B1;
      *b1 = A1;
    }
  }

  return 0;
}

