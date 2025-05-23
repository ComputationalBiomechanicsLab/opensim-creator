/*****************************************************************************
Copyright (c) 2023, The OpenBLAS Project
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
      derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**********************************************************************************/

#include "utest/openblas_utest.h"
#include <cblas.h>
#include "common.h"

#define DATASIZE 300
#define INCREMENT 2

struct DATA_CTRMV {
	float a_test[DATASIZE * DATASIZE * 2];
	float a_verify[DATASIZE * DATASIZE * 2];
	float x_test[DATASIZE * INCREMENT * 2];
	float x_verify[DATASIZE * INCREMENT * 2];
};

#ifdef BUILD_COMPLEX
static struct DATA_CTRMV data_ctrmv;

/**
 * Test ctrmv with the conjugate and not-transposed matrix A by conjugating matrix A
 * and comparing it with the non-conjugate ctrmv.
 *
 * param uplo specifies whether A is upper or lower triangular
 * param trans specifies op(A), the transposition (conjugation) operation applied to A
 * param diag specifies whether the matrix A is unit triangular or not.
 * param n - numbers of rows and columns of A
 * param lda - leading dimension of matrix A
 * param incx - increment for the elements of x
 * return norm of difference
 */
static float check_ctrmv(char uplo, char trans, char diag, blasint n, blasint lda, blasint incx)
{
	blasint i;
	float alpha_conj[] = {1.0f, 0.0f}; 
	char trans_verify = trans;
	char cc[2]="C", cr[2]="R";

	srand_generate(data_ctrmv.a_test, n * lda * 2);
	srand_generate(data_ctrmv.x_test, n * incx * 2);

	for (i = 0; i < n * lda * 2; i++)
		data_ctrmv.a_verify[i] = data_ctrmv.a_test[i];

	for (i = 0; i < n * incx * 2; i++)
		data_ctrmv.x_verify[i] = data_ctrmv.x_test[i];

	if (trans == 'R'){
		BLASFUNC(cimatcopy)(cc, cr, &n, &n, alpha_conj, data_ctrmv.a_verify, &lda, &lda);
		trans_verify = 'N';
	}

	BLASFUNC(ctrmv)(&uplo, &trans_verify, &diag, &n, data_ctrmv.a_verify, &lda,
	 				data_ctrmv.x_verify, &incx);

	BLASFUNC(ctrmv)(&uplo, &trans, &diag, &n, data_ctrmv.a_test, &lda,
	 				data_ctrmv.x_test, &incx);

	for (i = 0; i < n * incx * 2; i++)
		data_ctrmv.x_verify[i] -= data_ctrmv.x_test[i];

	return BLASFUNC(scnrm2)(&n, data_ctrmv.x_verify, &incx);
}

/**
 * Test ctrmv with the conjugate and not-transposed matrix A by conjugating matrix A 
 * and comparing it with the non-conjugate ctrmv.
 * Test with the following options:
 *
 * matrix A is conjugate and not-trans
 * matrix A is upper triangular
 * matrix A is not unit triangular
 */
CTEST(ctrmv, conj_notrans_upper_not_unit_triangular)
{
	blasint n = DATASIZE, incx = 1, lda = DATASIZE;
	char uplo = 'U';
	char diag = 'N';
	char trans = 'R';

	float norm = check_ctrmv(uplo, trans, diag, n, lda, incx);

	ASSERT_DBL_NEAR_TOL(0.0f, norm, SINGLE_EPS);
}

/**
 * Test ctrmv with the conjugate and not-transposed matrix A by conjugating matrix A 
 * and comparing it with the non-conjugate ctrmv.
 * Test with the following options:
 *
 * matrix A is conjugate and not-trans
 * matrix A is upper triangular
 * matrix A is unit triangular
 */
CTEST(ctrmv, conj_notrans_upper_unit_triangular)
{
	blasint n = DATASIZE, incx = 1, lda = DATASIZE;
	char uplo = 'U';
	char diag = 'U';
	char trans = 'R';

	float norm = check_ctrmv(uplo, trans, diag, n, lda, incx);

	ASSERT_DBL_NEAR_TOL(0.0f, norm, SINGLE_EPS);
}

/**
 * Test ctrmv with the conjugate and not-transposed matrix A by conjugating matrix A 
 * and comparing it with the non-conjugate ctrmv.
 * Test with the following options:
 *
 * matrix A is conjugate and not-trans
 * matrix A is lower triangular
 * matrix A is not unit triangular
 */
CTEST(ctrmv, conj_notrans_lower_not_triangular)
{
	blasint n = DATASIZE, incx = 1, lda = DATASIZE;
	char uplo = 'L';
	char diag = 'N';
	char trans = 'R';

	float norm = check_ctrmv(uplo, trans, diag, n, lda, incx);

	ASSERT_DBL_NEAR_TOL(0.0f, norm, SINGLE_EPS);
}

/**
 * Test ctrmv with the conjugate and not-transposed matrix A by conjugating matrix A 
 * and comparing it with the non-conjugate ctrmv.
 * Test with the following options:
 *
 * matrix A is conjugate and not-trans
 * matrix A is lower triangular
 * matrix A is unit triangular
 */
CTEST(ctrmv, conj_notrans_lower_unit_triangular)
{
	blasint n = DATASIZE, incx = 1, lda = DATASIZE;
	char uplo = 'L';
	char diag = 'U';
	char trans = 'R';

	float norm = check_ctrmv(uplo, trans, diag, n, lda, incx);

	ASSERT_DBL_NEAR_TOL(0.0f, norm, SINGLE_EPS);
}

/**
 * Test ctrmv with the conjugate and not-transposed matrix A by conjugating matrix A 
 * and comparing it with the non-conjugate ctrmv.
 * Test with the following options:
 *
 * matrix A is conjugate and not-trans
 * matrix A is upper triangular
 * matrix A is not unit triangular
 * vector x stride is 2
 */
CTEST(ctrmv, conj_notrans_upper_not_unit_triangular_incx_2)
{
	blasint n = DATASIZE, incx = 2, lda = DATASIZE;
	char uplo = 'U';
	char diag = 'N';
	char trans = 'R';

	float norm = check_ctrmv(uplo, trans, diag, n, lda, incx);

	ASSERT_DBL_NEAR_TOL(0.0f, norm, SINGLE_EPS);
}

/**
 * Test ctrmv with the conjugate and not-transposed matrix A by conjugating matrix A 
 * and comparing it with the non-conjugate ctrmv.
 * Test with the following options:
 *
 * matrix A is conjugate and not-trans
 * matrix A is upper triangular
 * matrix A is unit triangular
 * vector x stride is 2
 */
CTEST(ctrmv, conj_notrans_upper_unit_triangular_incx_2)
{
	blasint n = DATASIZE, incx = 2, lda = DATASIZE;
	char uplo = 'U';
	char diag = 'U';
	char trans = 'R';

	float norm = check_ctrmv(uplo, trans, diag, n, lda, incx);

	ASSERT_DBL_NEAR_TOL(0.0f, norm, SINGLE_EPS);
}

/**
 * Test ctrmv with the conjugate and not-transposed matrix A by conjugating matrix A 
 * and comparing it with the non-conjugate ctrmv.
 * Test with the following options:
 *
 * matrix A is conjugate and not-trans
 * matrix A is lower triangular
 * matrix A is not unit triangular
 * vector x stride is 2
 */
CTEST(ctrmv, conj_notrans_lower_not_triangular_incx_2)
{
	blasint n = DATASIZE, incx = 2, lda = DATASIZE;
	char uplo = 'L';
	char diag = 'N';
	char trans = 'R';

	float norm = check_ctrmv(uplo, trans, diag, n, lda, incx);

	ASSERT_DBL_NEAR_TOL(0.0f, norm, SINGLE_EPS);
}

/**
 * Test ctrmv with the conjugate and not-transposed matrix A by conjugating matrix A 
 * and comparing it with the non-conjugate ctrmv.
 * Test with the following options:
 *
 * matrix A is conjugate and not-trans
 * matrix A is lower triangular
 * matrix A is unit triangular
 * vector x stride is 2
 */
CTEST(ctrmv, conj_notrans_lower_unit_triangular_incx_2)
{
	blasint n = DATASIZE, incx = 2, lda = DATASIZE;
	char uplo = 'L';
	char diag = 'U';
	char trans = 'R';

	float norm = check_ctrmv(uplo, trans, diag, n, lda, incx);

	ASSERT_DBL_NEAR_TOL(0.0f, norm, SINGLE_EPS);
}
#endif
