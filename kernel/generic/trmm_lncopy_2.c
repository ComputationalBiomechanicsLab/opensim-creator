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

int CNAME(BLASLONG m, BLASLONG n, FLOAT *a, BLASLONG lda, BLASLONG posX, BLASLONG posY, FLOAT *b){

  BLASLONG i, js;
  BLASLONG X;

  FLOAT data01, data02, data03, data04;
  FLOAT *ao1, *ao2;

  js = (n >> 1);

  if (js > 0){
    do {
      X = posX;

      if (posX <= posY) {
	ao1 = a + posY + (posX + 0) * lda;
	ao2 = a + posY + (posX + 1) * lda;
      } else {
	ao1 = a + posX + (posY + 0) * lda;
	ao2 = a + posX + (posY + 1) * lda;
      }

      i = (m >> 1);
      if (i > 0) {
	do {
	  if (X > posY) {
	    data01 = *(ao1 + 0);
	    data02 = *(ao1 + 1);
	    data03 = *(ao2 + 0);
	    data04 = *(ao2 + 1);

	    b[ 0] = data01;
	    b[ 1] = data03;
	    b[ 2] = data02;
	    b[ 3] = data04;

	    ao1 += 2;
	    ao2 += 2;
	    b += 4;
	  } else
	    if (X < posY) {
	      ao1 += 2 * lda;
	      ao2 += 2 * lda;
	      b += 4;

	    } else {
#ifdef UNIT
	      data02 = *(ao1 + 1);

	      b[ 0] = ONE;
	      b[ 1] = ZERO;
	      b[ 2] = data02;
	      b[ 3] = ONE;
#else
	      data01 = *(ao1 + 0);
	      data02 = *(ao1 + 1);
	      data04 = *(ao2 + 1);

	      b[ 0] = data01;
	      b[ 1] = ZERO;
	      b[ 2] = data02;
	      b[ 3] = data04;
#endif
	      ao1 += 2;
	      ao2 += 2;
	      b += 4;
	    }

	  X += 2;
	  i --;
	} while (i > 0);
      }

      if (m & 1) {

	if (X > posY) {
	  data01 = *(ao1 + 0);
	  data03 = *(ao2 + 0);

	  b[ 0] = data01;
	  b[ 1] = data03;

	  /* ao1 += 1;
	  ao2 += 1; */
	  b += 2;
	} else
	  if (X < posY) {
	    // ao1 += lda;
	    b += 2;
	  } else {
#ifdef UNIT
	    data03 = *(ao2 + 0);

	    b[ 0] = ONE;
	    b[ 1] = data03;
#else
	    data01 = *(ao1 + 0);
	    data03 = *(ao2 + 0);

	    b[ 0] = data01;
	    b[ 1] = data03;
#endif
	    /* ao1 += 1;
	    ao2 += 1; */
	    b += 2;
	  }
      }

      posY += 2;
      js --;
    } while (js > 0);
  } /* End of main loop */


  if (n & 1){
      X = posX;

      if (posX <= posY) {
	ao1 = a + posY + (posX + 0) * lda;
      } else {
	ao1 = a + posX + (posY + 0) * lda;
      }

      i = m;
      if (i > 0) {
	do {

	  if (X > posY) {
	    data01 = *(ao1 + 0);
	    b[ 0] = data01;
	    ao1 += 1;
	    b += 1;
	  } else
	    if (X < posY) {
	      ao1 += lda;
	      b += 1;
	    } else {
#ifdef UNIT
	      b[ 0] = ONE;
#else
	      data01 = *(ao1 + 0);
	      b[ 0] = data01;
#endif
	      b += 1;
	      ao1 += 1;
	    }

	  X ++;
	  i --;
	} while (i > 0);
      }

      // posY += 1;
  }

  return 0;
}
