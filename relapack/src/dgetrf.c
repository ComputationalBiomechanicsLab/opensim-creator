#include "relapack.h"

static void RELAPACK_dgetrf_rec(const blasint *, const blasint *, double *,
    const blasint *, blasint *, blasint *);


/** DGETRF computes an LU factorization of a general M-by-N matrix A using partial pivoting with row interchanges.
 *
 * This routine is functionally equivalent to LAPACK's dgetrf.
 * For details on its interface, see
 * http://www.netlib.org/lapack/explore-html/d3/d6a/dgetrf_8f.html
 * */
void RELAPACK_dgetrf(
    const blasint *m, const blasint *n,
    double *A, const blasint *ldA, blasint *ipiv,
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
    if (*info!=0) {
        const blasint minfo = -*info;
        LAPACK(xerbla)("DGETRF", &minfo, strlen("DGETRF"));
        return;
    }

    if (*m == 0 || *n == 0) return;

    const blasint sn = MIN(*m, *n);
    RELAPACK_dgetrf_rec(m, &sn, A, ldA, ipiv, info);

    // Right remainder
    if (*m < *n) {
        // Constants
        const double ONE[] = { 1. };
        const blasint   iONE[] = { 1 };

        // Splitting
        const blasint rn = *n - *m;

        // A_L A_R
        const double *const A_L = A;
        double *const       A_R = A + *ldA * *m;

        // A_R = apply(ipiv, A_R)
        LAPACK(dlaswp)(&rn, A_R, ldA, iONE, m, ipiv, iONE);
        // A_R = A_S \ A_R
        BLAS(dtrsm)("L", "L", "N", "U", m, &rn, ONE, A_L, ldA, A_R, ldA);
    }
}


/** dgetrf's recursive compute kernel */
static void RELAPACK_dgetrf_rec(
    const blasint *m, const blasint *n,
    double *A, const blasint *ldA, blasint *ipiv,
    blasint *info
) {
    if ( *n <= MAX(CROSSOVER_DGETRF, 1)) {
        // Unblocked
        LAPACK(dgetrf2)(m, n, A, ldA, ipiv, info);
        return;
    }
    // Constants
    const double ONE[]  = { 1. };
    const double MONE[] = { -1. };
    const blasint    iONE[] = { 1 };

    // Splitting
    const blasint n1 = DREC_SPLIT(*n);
    const blasint n2 = *n - n1;
    const blasint m2 = *m - n1;

    // A_L A_R
    double *const A_L = A;
    double *const A_R = A + *ldA * n1;

    // A_TL A_TR
    // A_BL A_BR
    double *const A_TL = A;
    double *const A_TR = A + *ldA * n1;
    double *const A_BL = A             + n1;
    double *const A_BR = A + *ldA * n1 + n1;

    // ipiv_T
    // ipiv_B
    blasint *const ipiv_T = ipiv;
    blasint *const ipiv_B = ipiv + n1;

    // recursion(A_L, ipiv_T)
    RELAPACK_dgetrf_rec(m, &n1, A_L, ldA, ipiv_T, info);
    if (*info) return;
    // apply pivots to A_R
    LAPACK(dlaswp)(&n2, A_R, ldA, iONE, &n1, ipiv_T, iONE);

    // A_TR = A_TL \ A_TR
    BLAS(dtrsm)("L", "L", "N", "U", &n1, &n2, ONE, A_TL, ldA, A_TR, ldA);
    // A_BR = A_BR - A_BL * A_TR
    BLAS(dgemm)("N", "N", &m2, &n2, &n1, MONE, A_BL, ldA, A_TR, ldA, ONE, A_BR, ldA);

    // recursion(A_BR, ipiv_B)
    RELAPACK_dgetrf_rec(&m2, &n2, A_BR, ldA, ipiv_B, info);
    if (*info)
        *info += n1;
    // apply pivots to A_BL
    LAPACK(dlaswp)(&n1, A_BL, ldA, iONE, &n2, ipiv_B, iONE);
    // shift pivots
    blasint i;
    for (i = 0; i < n2; i++)
        ipiv_B[i] += n1;
}
