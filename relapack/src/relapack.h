#ifndef RELAPACK_INT_H
#define RELAPACK_INT_H
#include <string.h>
#include "config.h"
#if defined(OS_WINDOWS) && defined(__64BIT__)
typedef long long BLASLONG;
typedef unsigned long long BLASULONG;
#else
typedef long BLASLONG;
typedef unsigned long BLASULONG;
#endif
#include "relapack_config.h"

#include "../inc/relapack.h"

// add an underscore to BLAS routines (or not)
#if BLAS_UNDERSCORE
#define BLAS(routine) routine ## _
#else
#define BLAS(routine) routine
#endif

// add an underscore to LAPACK routines (or not)
#if LAPACK_UNDERSCORE
#define LAPACK(routine) routine ## _
#else
#define LAPACK(routine) routine
#endif

// minimum and maximum macros
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// REC_SPLIT(n) returns how a problem of size n is split recursively.
// If n >= 16, we ensure that the size of at least one of the halves is
// divisible by 8 (the cache line size in most CPUs), while both halves are
// still as close as possible in size.
// If n < 16 the problem is simply split in the middle. (Note that the
// crossoversize is usually larger than 16.)
#define SREC_SPLIT(n) ((n >= 32) ? ((n + 16) / 32) * 16 : n / 2)
#define DREC_SPLIT(n) ((n >= 16) ? ((n + 8) / 16) * 8 : n / 2)
#define CREC_SPLIT(n) ((n >= 16) ? ((n + 8) / 16) * 8 : n / 2)
#define ZREC_SPLIT(n) ((n >= 8) ? ((n + 4) / 8) * 4 : n / 2)

#include "lapack.h"
#include "blas.h"

// sytrf helper routines
void RELAPACK_ssytrf_rec2(const char *, const blasint *, const blasint *, blasint *, float *, const blasint *, blasint *, float *, const blasint *, blasint *);
void RELAPACK_dsytrf_rec2(const char *, const blasint *, const blasint *, blasint *, double *, const blasint *, blasint *, double *, const blasint *, blasint *);
void RELAPACK_csytrf_rec2(const char *, const blasint *, const blasint *, blasint *, float *, const blasint *, blasint *, float *, const blasint *, blasint *);
void RELAPACK_chetrf_rec2(const char *, const blasint *, const blasint *, blasint *, float *, const blasint *, blasint *, float *, const blasint *, blasint *);
void RELAPACK_zsytrf_rec2(const char *, const blasint *, const blasint *, blasint *, double *, const blasint *, blasint *, double *, const blasint *, blasint *);
void RELAPACK_zhetrf_rec2(const char *, const blasint *, const blasint *, blasint *, double *, const blasint *, blasint *, double *, const blasint *, blasint *);
void RELAPACK_ssytrf_rook_rec2(const char *, const blasint *, const blasint *, blasint *, float *, const blasint *, blasint *, float *, const blasint *, blasint *);
void RELAPACK_dsytrf_rook_rec2(const char *, const blasint *, const blasint *, blasint *, double *, const blasint *, blasint *, double *, const blasint *, blasint *);
void RELAPACK_csytrf_rook_rec2(const char *, const blasint *, const blasint *, blasint *, float *, const blasint *, blasint *, float *, const blasint *, blasint *);
void RELAPACK_chetrf_rook_rec2(const char *, const blasint *, const blasint *, blasint *, float *, const blasint *, blasint *, float *, const blasint *, blasint *);
void RELAPACK_zsytrf_rook_rec2(const char *, const blasint *, const blasint *, blasint *, double *, const blasint *, blasint *, double *, const blasint *, blasint *);
void RELAPACK_zhetrf_rook_rec2(const char *, const blasint *, const blasint *, blasint *, double *, const blasint *, blasint *, double *, const blasint *, blasint *);

// trsyl helper routines
void RELAPACK_strsyl_rec2(const char *, const char *, const blasint *, const blasint *, const blasint *, const float *, const blasint *, const float *, const blasint *, float *, const blasint *, float *, blasint *);
void RELAPACK_dtrsyl_rec2(const char *, const char *, const blasint *, const blasint *, const blasint *, const double *, const blasint *, const double *, const blasint *, double *, const blasint *, double *, blasint *);
void RELAPACK_ctrsyl_rec2(const char *, const char *, const blasint *, const blasint *, const blasint *, const float *, const blasint *, const float *, const blasint *, float *, const blasint *, float *, blasint *);
void RELAPACK_ztrsyl_rec2(const char *, const char *, const blasint *, const blasint *, const blasint *, const double *, const blasint *, const double *, const blasint *, double *, const blasint *, double *, blasint *);

#endif /*  RELAPACK_INT_H */
