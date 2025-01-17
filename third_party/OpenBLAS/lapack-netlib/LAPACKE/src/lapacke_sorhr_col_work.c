#include "lapacke_utils.h"

lapack_int LAPACKE_sorhr_col_work( int matrix_layout, lapack_int m, lapack_int n,
                                   lapack_int nb, float* a,
                                   lapack_int lda, float* t,
                                   lapack_int ldt, float* d )
{
    lapack_int info = 0;
    if( matrix_layout == LAPACK_COL_MAJOR ) {
        /* Call LAPACK function and adjust info */
        LAPACK_sorhr_col( &m, &n, &nb, a, &lda, t, &ldt, d, &info );
        if( info < 0 ) {
            info = info - 1;
        }
    } else if( matrix_layout == LAPACK_ROW_MAJOR ) {
        lapack_int lda_t = MAX(1,m);
        lapack_int ldt_t = MAX(1,MIN(nb,n));
        float* a_t = NULL;
        float* t_t = NULL;
        /* Check leading dimension(s) */
        if( lda < n ) {
            info = -6;
            LAPACKE_xerbla( "LAPACKE_sorhr_col_work", info );
            return info;
        }
        if( ldt < n ) {
            info = -8;
            LAPACKE_xerbla( "LAPACKE_sorhr_col_work", info );
            return info;
        }
        /* Allocate memory for temporary array(s) */
        a_t = (float*)
            LAPACKE_malloc( sizeof(float) * lda_t * MAX(1,n) );
        if( a_t == NULL ) {
            info = LAPACK_TRANSPOSE_MEMORY_ERROR;
            goto exit_level_0;
        }
        t_t = (float*)
            LAPACKE_malloc( sizeof(float) *
                            ldt_t * MAX(1,n) );
        if( t_t == NULL ) {
            info = LAPACK_TRANSPOSE_MEMORY_ERROR;
            goto exit_level_1;
        }
        /* Transpose input matrices */
        LAPACKE_sge_trans( matrix_layout, m, n, a, lda, a_t, lda_t );
        /* Call LAPACK function and adjust info */
        LAPACK_sorhr_col( &m, &n, &nb, a_t, &lda_t, t_t, &ldt_t, d, &info );
        if( info < 0 ) {
            info = info - 1;
        }
        /* Transpose output matrices */
        LAPACKE_sge_trans( LAPACK_COL_MAJOR, m, n, a_t, lda_t, a, lda );
        LAPACKE_sge_trans( LAPACK_COL_MAJOR, ldt, n, t_t, ldt_t, t,
                           ldt );
        /* Release memory and exit */
        LAPACKE_free( t_t );
exit_level_1:
        LAPACKE_free( a_t );
exit_level_0:
        if( info == LAPACK_TRANSPOSE_MEMORY_ERROR ) {
            LAPACKE_xerbla( "LAPACKE_sorhr_col_work", info );
        }
    } else {
        info = -1;
        LAPACKE_xerbla( "LAPACKE_sorhr_col_work", info );
    }
    return info;
}
