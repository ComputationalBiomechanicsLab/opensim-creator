#include "relapack.h"
#if XSYGST_ALLOW_MALLOC
#include "stdlib.h"
#endif

static void RELAPACK_chegst_rec(const blasint *, const char *, const blasint *,
    float *, const blasint *, const float *, const blasint *,
    float *, const blasint *, blasint *);


/** CHEGST reduces a complex Hermitian-definite generalized eigenproblem to standard form.
 *
 * This routine is functionally equivalent to LAPACK's chegst.
 * For details on its interface, see
 * http://www.netlib.org/lapack/explore-html/d7/d2a/chegst_8f.html
 * */
void RELAPACK_chegst(
    const blasint *itype, const char *uplo, const blasint *n,
    float *A, const blasint *ldA, const float *B, const blasint *ldB,
    blasint *info
) {

    // Check arguments
    const blasint lower = LAPACK(lsame)(uplo, "L");
    const blasint upper = LAPACK(lsame)(uplo, "U");
    *info = 0;
    if (*itype < 1 || *itype > 3)
        *info = -1;
    else if (!lower && !upper)
        *info = -2;
    else if (*n < 0)
        *info = -3;
    else if (*ldA < MAX(1, *n))
        *info = -5;
    else if (*ldB < MAX(1, *n))
        *info = -7;
    if (*info) {
        const blasint minfo = -*info;
        LAPACK(xerbla)("CHEGST", &minfo, strlen("CHEGST"));
        return;
    }

    if (*n == 0) return;

    // Clean char * arguments
    const char cleanuplo = lower ? 'L' : 'U';

    // Allocate work space
    float *Work = NULL;
    blasint   lWork = 0;
#if XSYGST_ALLOW_MALLOC
    const blasint n1 = CREC_SPLIT(*n);
    lWork = n1 * (*n - n1);
    Work  = malloc(lWork * 2 * sizeof(float));
    if (!Work)
        lWork = 0;
#endif

    // recursive kernel
    RELAPACK_chegst_rec(itype, &cleanuplo, n, A, ldA, B, ldB, Work, &lWork, info);

    // Free work space
#if XSYGST_ALLOW_MALLOC
    if (Work)
        free(Work);
#endif
}


/** chegst's recursive compute kernel */
static void RELAPACK_chegst_rec(
    const blasint *itype, const char *uplo, const blasint *n,
    float *A, const blasint *ldA, const float *B, const blasint *ldB,
    float *Work, const blasint *lWork, blasint *info
) {

    if (*n <= MAX(CROSSOVER_CHEGST, 1)) {
        // Unblocked
        LAPACK(chegs2)(itype, uplo, n, A, ldA, B, ldB, info);
        return;
    }

    // Constants
    const float ZERO[]  = { 0., 0. };
    const float ONE[]   = { 1., 0. };
    const float MONE[]  = { -1., 0. };
    const float HALF[]  = { .5, 0. };
    const float MHALF[] = { -.5, 0. };
    const blasint   iONE[]  = { 1 };

    // Loop iterator
    blasint i;

    // Splitting
    const blasint n1 = CREC_SPLIT(*n);
    const blasint n2 = *n - n1;

    // A_TL A_TR
    // A_BL A_BR
    float *const A_TL = A;
    float *const A_TR = A + 2 * *ldA * n1;
    float *const A_BL = A                 + 2 * n1;
    float *const A_BR = A + 2 * *ldA * n1 + 2 * n1;

    // B_TL B_TR
    // B_BL B_BR
    const float *const B_TL = B;
    const float *const B_TR = B + 2 * *ldB * n1;
    const float *const B_BL = B                 + 2 * n1;
    const float *const B_BR = B + 2 * *ldB * n1 + 2 * n1;

    // recursion(A_TL, B_TL)
    RELAPACK_chegst_rec(itype, uplo, &n1, A_TL, ldA, B_TL, ldB, Work, lWork, info);

    if (*itype == 1)
        if (*uplo == 'L') {
            // A_BL = A_BL / B_TL'
            BLAS(ctrsm)("R", "L", "C", "N", &n2, &n1, ONE, B_TL, ldB, A_BL, ldA);
            if (*lWork > n2 * n1) {
                // T = -1/2 * B_BL * A_TL
                BLAS(chemm)("R", "L", &n2, &n1, MHALF, A_TL, ldA, B_BL, ldB, ZERO, Work, &n2);
                // A_BL = A_BL + T
                for (i = 0; i < n1; i++)
                    BLAS(caxpy)(&n2, ONE, Work + 2 * n2 * i, iONE, A_BL + 2 * *ldA * i, iONE);
            } else
                // A_BL = A_BL - 1/2 B_BL * A_TL
                BLAS(chemm)("R", "L", &n2, &n1, MHALF, A_TL, ldA, B_BL, ldB, ONE, A_BL, ldA);
            // A_BR = A_BR - A_BL * B_BL' - B_BL * A_BL'
            BLAS(cher2k)("L", "N", &n2, &n1, MONE, A_BL, ldA, B_BL, ldB, ONE, A_BR, ldA);
            if (*lWork > n2 * n1)
                // A_BL = A_BL + T
                for (i = 0; i < n1; i++)
                    BLAS(caxpy)(&n2, ONE, Work + 2 * n2 * i, iONE, A_BL + 2 * *ldA * i, iONE);
            else
                // A_BL = A_BL - 1/2 B_BL * A_TL
                BLAS(chemm)("R", "L", &n2, &n1, MHALF, A_TL, ldA, B_BL, ldB, ONE, A_BL, ldA);
            // A_BL = B_BR \ A_BL
            BLAS(ctrsm)("L", "L", "N", "N", &n2, &n1, ONE, B_BR, ldB, A_BL, ldA);
        } else {
            // A_TR = B_TL' \ A_TR
            BLAS(ctrsm)("L", "U", "C", "N", &n1, &n2, ONE, B_TL, ldB, A_TR, ldA);
            if (*lWork > n2 * n1) {
                // T = -1/2 * A_TL * B_TR
                BLAS(chemm)("L", "U", &n1, &n2, MHALF, A_TL, ldA, B_TR, ldB, ZERO, Work, &n1);
                // A_TR = A_BL + T
                for (i = 0; i < n2; i++)
                    BLAS(caxpy)(&n1, ONE, Work + 2 * n1 * i, iONE, A_TR + 2 * *ldA * i, iONE);
            } else
                // A_TR = A_TR - 1/2 A_TL * B_TR
                BLAS(chemm)("L", "U", &n1, &n2, MHALF, A_TL, ldA, B_TR, ldB, ONE, A_TR, ldA);
            // A_BR = A_BR - A_TR' * B_TR - B_TR' * A_TR
            BLAS(cher2k)("U", "C", &n2, &n1, MONE, A_TR, ldA, B_TR, ldB, ONE, A_BR, ldA);
            if (*lWork > n2 * n1)
                // A_TR = A_BL + T
                for (i = 0; i < n2; i++)
                    BLAS(caxpy)(&n1, ONE, Work + 2 * n1 * i, iONE, A_TR + 2 * *ldA * i, iONE);
            else
                // A_TR = A_TR - 1/2 A_TL * B_TR
                BLAS(chemm)("L", "U", &n1, &n2, MHALF, A_TL, ldA, B_TR, ldB, ONE, A_TR, ldA);
            // A_TR = A_TR / B_BR
            BLAS(ctrsm)("R", "U", "N", "N", &n1, &n2, ONE, B_BR, ldB, A_TR, ldA);
        }
    else
        if (*uplo == 'L') {
            // A_BL = A_BL * B_TL
            BLAS(ctrmm)("R", "L", "N", "N", &n2, &n1, ONE, B_TL, ldB, A_BL, ldA);
            if (*lWork > n2 * n1) {
                // T = 1/2 * A_BR * B_BL
                BLAS(chemm)("L", "L", &n2, &n1, HALF, A_BR, ldA, B_BL, ldB, ZERO, Work, &n2);
                // A_BL = A_BL + T
                for (i = 0; i < n1; i++)
                    BLAS(caxpy)(&n2, ONE, Work + 2 * n2 * i, iONE, A_BL + 2 * *ldA * i, iONE);
            } else
                // A_BL = A_BL + 1/2 A_BR * B_BL
                BLAS(chemm)("L", "L", &n2, &n1, HALF, A_BR, ldA, B_BL, ldB, ONE, A_BL, ldA);
            // A_TL = A_TL + A_BL' * B_BL + B_BL' * A_BL
            BLAS(cher2k)("L", "C", &n1, &n2, ONE, A_BL, ldA, B_BL, ldB, ONE, A_TL, ldA);
            if (*lWork > n2 * n1)
                // A_BL = A_BL + T
                for (i = 0; i < n1; i++)
                    BLAS(caxpy)(&n2, ONE, Work + 2 * n2 * i, iONE, A_BL + 2 * *ldA * i, iONE);
            else
                // A_BL = A_BL + 1/2 A_BR * B_BL
                BLAS(chemm)("L", "L", &n2, &n1, HALF, A_BR, ldA, B_BL, ldB, ONE, A_BL, ldA);
            // A_BL = B_BR * A_BL
            BLAS(ctrmm)("L", "L", "C", "N", &n2, &n1, ONE, B_BR, ldB, A_BL, ldA);
        } else {
            // A_TR = B_TL * A_TR
            BLAS(ctrmm)("L", "U", "N", "N", &n1, &n2, ONE, B_TL, ldB, A_TR, ldA);
            if (*lWork > n2 * n1) {
                // T = 1/2 * B_TR * A_BR
                BLAS(chemm)("R", "U", &n1, &n2, HALF, A_BR, ldA, B_TR, ldB, ZERO, Work, &n1);
                // A_TR = A_TR + T
                for (i = 0; i < n2; i++)
                    BLAS(caxpy)(&n1, ONE, Work + 2 * n1 * i, iONE, A_TR + 2 * *ldA * i, iONE);
            } else
                // A_TR = A_TR + 1/2 B_TR A_BR
                BLAS(chemm)("R", "U", &n1, &n2, HALF, A_BR, ldA, B_TR, ldB, ONE, A_TR, ldA);
            // A_TL = A_TL + A_TR * B_TR' + B_TR * A_TR'
            BLAS(cher2k)("U", "N", &n1, &n2, ONE, A_TR, ldA, B_TR, ldB, ONE, A_TL, ldA);
            if (*lWork > n2 * n1)
                // A_TR = A_TR + T
                for (i = 0; i < n2; i++)
                    BLAS(caxpy)(&n1, ONE, Work + 2 * n1 * i, iONE, A_TR + 2 * *ldA * i, iONE);
            else
                // A_TR = A_TR + 1/2 B_TR * A_BR
                BLAS(chemm)("R", "U", &n1, &n2, HALF, A_BR, ldA, B_TR, ldB, ONE, A_TR, ldA);
            // A_TR = A_TR * B_BR
            BLAS(ctrmm)("R", "U", "C", "N", &n1, &n2, ONE, B_BR, ldB, A_TR, ldA);
        }

    // recursion(A_BR, B_BR)
    RELAPACK_chegst_rec(itype, uplo, &n2, A_BR, ldA, B_BR, ldB, Work, lWork, info);
}
