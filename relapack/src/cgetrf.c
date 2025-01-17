#include "relapack.h"

static void RELAPACK_cgetrf_rec(const blasint *, const blasint *, float *,
    const blasint *, blasint *, blasint *);


/** CGETRF computes an LU factorization of a general M-by-N matrix A using partial pivoting with row interchanges.
 *
 * This routine is functionally equivalent to LAPACK's cgetrf.
 * For details on its interface, see
 * http://www.netlib.org/lapack/explore-html/d9/dfb/cgetrf_8f.html
 */
void RELAPACK_cgetrf(
    const blasint *m, const blasint *n,
    float *A, const blasint *ldA, blasint *ipiv,
    blasint *info
) {

    // Check arguments
    *info = 0;
    if (*m < 0)
        *info = -1;
    else if (*n < 0)
        *info = -2;
    else if (*ldA < MAX(1, *m))
        *info = -4;
    if (*info) {
        const blasint minfo = -*info;
        LAPACK(xerbla)("CGETRF", &minfo, strlen("CGETRF"));
        return;
    }

    if (*m == 0 || *n == 0) return;

    const blasint sn = MIN(*m, *n);

    RELAPACK_cgetrf_rec(m, &sn, A, ldA, ipiv, info);

    // Right remainder
    if (*m < *n) {
        // Constants
        const float ONE[]  = { 1., 0. };
        const blasint   iONE[] = { 1 };

        // Splitting
        const blasint rn = *n - *m;

        // A_L A_R
        const float *const A_L = A;
        float *const       A_R = A + 2 * *ldA * *m;

        // A_R = apply(ipiv, A_R)
        LAPACK(claswp)(&rn, A_R, ldA, iONE, m, ipiv, iONE);
        // A_R = A_L \ A_R
        BLAS(ctrsm)("L", "L", "N", "U", m, &rn, ONE, A_L, ldA, A_R, ldA);
    }
}


/** cgetrf's recursive compute kernel */
static void RELAPACK_cgetrf_rec(
    const blasint *m, const blasint *n,
    float *A, const blasint *ldA, blasint *ipiv,
    blasint *info
) {

    if (*m == 0 || *n == 0) return;

    if ( *n <= MAX(CROSSOVER_CGETRF, 1)) {
        // Unblocked
        LAPACK(cgetrf2)(m, n, A, ldA, ipiv, info);
        return;
    }

    // Constants
    const float ONE[]  = { 1., 0. };
    const float MONE[] = { -1., 0. };
    const blasint   iONE[] = { 1 };

    // Splitting
    const blasint n1 = CREC_SPLIT(*n);
    const blasint n2 = *n - n1;
    const blasint m2 = *m - n1;

    // A_L A_R
    float *const A_L = A;
    float *const A_R = A + 2 * *ldA * n1;

    // A_TL A_TR
    // A_BL A_BR
    float *const A_TL = A;
    float *const A_TR = A + 2 * *ldA * n1;
    float *const A_BL = A                 + 2 * n1;
    float *const A_BR = A + 2 * *ldA * n1 + 2 * n1;

    // ipiv_T
    // ipiv_B
    blasint *const ipiv_T = ipiv;
    blasint *const ipiv_B = ipiv + n1;

    // recursion(A_L, ipiv_T)
    RELAPACK_cgetrf_rec(m, &n1, A_L, ldA, ipiv_T, info);
    if (*info) return;
    // apply pivots to A_R
    LAPACK(claswp)(&n2, A_R, ldA, iONE, &n1, ipiv_T, iONE);

    // A_TR = A_TL \ A_TR
    BLAS(ctrsm)("L", "L", "N", "U", &n1, &n2, ONE, A_TL, ldA, A_TR, ldA);
    // A_BR = A_BR - A_BL * A_TR
    BLAS(cgemm)("N", "N", &m2, &n2, &n1, MONE, A_BL, ldA, A_TR, ldA, ONE, A_BR, ldA);

    // recursion(A_BR, ipiv_B)
    RELAPACK_cgetrf_rec(&m2, &n2, A_BR, ldA, ipiv_B, info);
    if (*info)
        *info += n1;
    // apply pivots to A_BL
    LAPACK(claswp)(&n1, A_BL, ldA, iONE, &n2, ipiv_B, iONE);
    // shift pivots
    blasint i;
    for (i = 0; i < n2; i++)
        ipiv_B[i] += n1;
}
