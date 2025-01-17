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

#undef TIMING

#ifndef RSIDE
#ifndef LOWER
#define ICOPY_OPERATION(M, N, A, LDA, X, Y, BUFFER) SYMM_IUTCOPY(M, N, A, LDA, Y, X, BUFFER);
#else
#define ICOPY_OPERATION(M, N, A, LDA, X, Y, BUFFER) SYMM_ILTCOPY(M, N, A, LDA, Y, X, BUFFER);
#endif
#endif

#ifdef RSIDE
#ifndef LOWER
#define OCOPY_OPERATION(M, N, A, LDA, X, Y, BUFFER) SYMM_OUTCOPY(M, N, A, LDA, Y, X, BUFFER);
#else
#define OCOPY_OPERATION(M, N, A, LDA, X, Y, BUFFER) SYMM_OLTCOPY(M, N, A, LDA, Y, X, BUFFER);
#endif
#endif

#ifndef RSIDE
#define K		args -> m
#ifndef LOWER
#define GEMM_LOCAL    SYMM_LU
#else
#define GEMM_LOCAL    SYMM_LL
#endif
#else
#define K		args -> n
#ifndef LOWER
#define GEMM_LOCAL    SYMM_RU
#else
#define GEMM_LOCAL    SYMM_RL
#endif
#endif

#ifdef THREADED_LEVEL3
#include "level3_thread.c"
#else
#include "level3.c"
#endif
