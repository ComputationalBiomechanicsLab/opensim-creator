#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <complex.h>
#ifdef complex
#undef complex
#endif
#ifdef I
#undef I
#endif

#if defined(_WIN64)
typedef long long BLASLONG;
typedef unsigned long long BLASULONG;
#else
typedef long BLASLONG;
typedef unsigned long BLASULONG;
#endif

#ifdef LAPACK_ILP64
typedef BLASLONG blasint;
#if defined(_WIN64)
#define blasabs(x) llabs(x)
#else
#define blasabs(x) labs(x)
#endif
#else
typedef int blasint;
#define blasabs(x) abs(x)
#endif

typedef blasint integer;

typedef unsigned int uinteger;
typedef char *address;
typedef short int shortint;
typedef float real;
typedef double doublereal;
typedef struct { real r, i; } complex;
typedef struct { doublereal r, i; } doublecomplex;
#ifdef _MSC_VER
static inline _Fcomplex Cf(complex *z) {_Fcomplex zz={z->r , z->i}; return zz;}
static inline _Dcomplex Cd(doublecomplex *z) {_Dcomplex zz={z->r , z->i};return zz;}
static inline _Fcomplex * _pCf(complex *z) {return (_Fcomplex*)z;}
static inline _Dcomplex * _pCd(doublecomplex *z) {return (_Dcomplex*)z;}
#else
static inline _Complex float Cf(complex *z) {return z->r + z->i*_Complex_I;}
static inline _Complex double Cd(doublecomplex *z) {return z->r + z->i*_Complex_I;}
static inline _Complex float * _pCf(complex *z) {return (_Complex float*)z;}
static inline _Complex double * _pCd(doublecomplex *z) {return (_Complex double*)z;}
#endif
#define pCf(z) (*_pCf(z))
#define pCd(z) (*_pCd(z))
typedef blasint logical;

typedef char logical1;
typedef char integer1;

#define TRUE_ (1)
#define FALSE_ (0)

/* Extern is for use with -E */
#ifndef Extern
#define Extern extern
#endif

/* I/O stuff */

typedef int flag;
typedef int ftnlen;
typedef int ftnint;

/*external read, write*/
typedef struct
{	flag cierr;
	ftnint ciunit;
	flag ciend;
	char *cifmt;
	ftnint cirec;
} cilist;

/*internal read, write*/
typedef struct
{	flag icierr;
	char *iciunit;
	flag iciend;
	char *icifmt;
	ftnint icirlen;
	ftnint icirnum;
} icilist;

/*open*/
typedef struct
{	flag oerr;
	ftnint ounit;
	char *ofnm;
	ftnlen ofnmlen;
	char *osta;
	char *oacc;
	char *ofm;
	ftnint orl;
	char *oblnk;
} olist;

/*close*/
typedef struct
{	flag cerr;
	ftnint cunit;
	char *csta;
} cllist;

/*rewind, backspace, endfile*/
typedef struct
{	flag aerr;
	ftnint aunit;
} alist;

/* inquire */
typedef struct
{	flag inerr;
	ftnint inunit;
	char *infile;
	ftnlen infilen;
	ftnint	*inex;	/*parameters in standard's order*/
	ftnint	*inopen;
	ftnint	*innum;
	ftnint	*innamed;
	char	*inname;
	ftnlen	innamlen;
	char	*inacc;
	ftnlen	inacclen;
	char	*inseq;
	ftnlen	inseqlen;
	char 	*indir;
	ftnlen	indirlen;
	char	*infmt;
	ftnlen	infmtlen;
	char	*inform;
	ftnint	informlen;
	char	*inunf;
	ftnlen	inunflen;
	ftnint	*inrecl;
	ftnint	*innrec;
	char	*inblank;
	ftnlen	inblanklen;
} inlist;

#define VOID void

union Multitype {	/* for multiple entry points */
	integer1 g;
	shortint h;
	integer i;
	/* longint j; */
	real r;
	doublereal d;
	complex c;
	doublecomplex z;
	};

typedef union Multitype Multitype;

struct Vardesc {	/* for Namelist */
	char *name;
	char *addr;
	ftnlen *dims;
	int  type;
	};
typedef struct Vardesc Vardesc;

struct Namelist {
	char *name;
	Vardesc **vars;
	int nvars;
	};
typedef struct Namelist Namelist;

#define abs(x) ((x) >= 0 ? (x) : -(x))
#define dabs(x) (fabs(x))
#define f2cmin(a,b) ((a) <= (b) ? (a) : (b))
#define f2cmax(a,b) ((a) >= (b) ? (a) : (b))
#define dmin(a,b) (f2cmin(a,b))
#define dmax(a,b) (f2cmax(a,b))
#define bit_test(a,b)	((a) >> (b) & 1)
#define bit_clear(a,b)	((a) & ~((uinteger)1 << (b)))
#define bit_set(a,b)	((a) |  ((uinteger)1 << (b)))

#define abort_() { sig_die("Fortran abort routine called", 1); }
#define c_abs(z) (cabsf(Cf(z)))
#define c_cos(R,Z) { pCf(R)=ccos(Cf(Z)); }
#ifdef _MSC_VER
#define c_div(c, a, b) {Cf(c)._Val[0] = (Cf(a)._Val[0]/Cf(b)._Val[0]); Cf(c)._Val[1]=(Cf(a)._Val[1]/Cf(b)._Val[1]);}
#define z_div(c, a, b) {Cd(c)._Val[0] = (Cd(a)._Val[0]/Cd(b)._Val[0]); Cd(c)._Val[1]=(Cd(a)._Val[1]/df(b)._Val[1]);}
#else
#define c_div(c, a, b) {pCf(c) = Cf(a)/Cf(b);}
#define z_div(c, a, b) {pCd(c) = Cd(a)/Cd(b);}
#endif
#define c_exp(R, Z) {pCf(R) = cexpf(Cf(Z));}
#define c_log(R, Z) {pCf(R) = clogf(Cf(Z));}
#define c_sin(R, Z) {pCf(R) = csinf(Cf(Z));}
//#define c_sqrt(R, Z) {*(R) = csqrtf(Cf(Z));}
#define c_sqrt(R, Z) {pCf(R) = csqrtf(Cf(Z));}
#define d_abs(x) (fabs(*(x)))
#define d_acos(x) (acos(*(x)))
#define d_asin(x) (asin(*(x)))
#define d_atan(x) (atan(*(x)))
#define d_atn2(x, y) (atan2(*(x),*(y)))
#define d_cnjg(R, Z) { pCd(R) = conj(Cd(Z)); }
#define r_cnjg(R, Z) { pCf(R) = conjf(Cf(Z)); }
#define d_cos(x) (cos(*(x)))
#define d_cosh(x) (cosh(*(x)))
#define d_dim(__a, __b) ( *(__a) > *(__b) ? *(__a) - *(__b) : 0.0 )
#define d_exp(x) (exp(*(x)))
#define d_imag(z) (cimag(Cd(z)))
#define r_imag(z) (cimagf(Cf(z)))
#define d_int(__x) (*(__x)>0 ? floor(*(__x)) : -floor(- *(__x)))
#define r_int(__x) (*(__x)>0 ? floor(*(__x)) : -floor(- *(__x)))
#define d_lg10(x) ( 0.43429448190325182765 * log(*(x)) )
#define r_lg10(x) ( 0.43429448190325182765 * log(*(x)) )
#define d_log(x) (log(*(x)))
#define d_mod(x, y) (fmod(*(x), *(y)))
#define u_nint(__x) ((__x)>=0 ? floor((__x) + .5) : -floor(.5 - (__x)))
#define d_nint(x) u_nint(*(x))
#define u_sign(__a,__b) ((__b) >= 0 ? ((__a) >= 0 ? (__a) : -(__a)) : -((__a) >= 0 ? (__a) : -(__a)))
#define d_sign(a,b) u_sign(*(a),*(b))
#define r_sign(a,b) u_sign(*(a),*(b))
#define d_sin(x) (sin(*(x)))
#define d_sinh(x) (sinh(*(x)))
#define d_sqrt(x) (sqrt(*(x)))
#define d_tan(x) (tan(*(x)))
#define d_tanh(x) (tanh(*(x)))
#define i_abs(x) abs(*(x))
#define i_dnnt(x) ((integer)u_nint(*(x)))
#define i_len(s, n) (n)
#define i_nint(x) ((integer)u_nint(*(x)))
#define i_sign(a,b) ((integer)u_sign((integer)*(a),(integer)*(b)))
#define s_cat(lpp, rpp, rnp, np, llp) { 	ftnlen i, nc, ll; char *f__rp, *lp; 	ll = (llp); lp = (lpp); 	for(i=0; i < (int)*(np); ++i) {         	nc = ll; 	        if((rnp)[i] < nc) nc = (rnp)[i]; 	        ll -= nc;         	f__rp = (rpp)[i]; 	        while(--nc >= 0) *lp++ = *(f__rp)++;         } 	while(--ll >= 0) *lp++ = ' '; }
#define s_cmp(a,b,c,d) ((integer)strncmp((a),(b),f2cmin((c),(d))))
#define s_copy(A,B,C,D) { int __i,__m; for (__i=0, __m=f2cmin((C),(D)); __i<__m && (B)[__i] != 0; ++__i) (A)[__i] = (B)[__i]; }
#define sig_die(s, kill) { exit(1); }
#define s_stop(s, n) {exit(0);}
#define z_abs(z) (cabs(Cd(z)))
#define z_exp(R, Z) {pCd(R) = cexp(Cd(Z));}
#define z_sqrt(R, Z) {pCd(R) = csqrt(Cd(Z));}
#define myexit_() break;
#define mycycle() continue;
#define myceiling(w) {ceil(w)}
#define myhuge(w) {HUGE_VAL}
//#define mymaxloc_(w,s,e,n) {if (sizeof(*(w)) == sizeof(double)) dmaxloc_((w),*(s),*(e),n); else dmaxloc_((w),*(s),*(e),n);}
#define mymaxloc(w,s,e,n) {dmaxloc_(w,*(s),*(e),n)}

/*  -- translated by f2c (version 20000121).
   You must link the resulting object file with the libraries:
	-lf2c -lm   (in that order)
*/




/* Table of constant values */

static integer c__6 = 6;
static integer c__0 = 0;
static integer c__2 = 2;
static integer c__1 = 1;
static integer c_n1 = -1;
static real c_b109 = 0.f;

/* > \brief <b> SGESVDX computes the singular value decomposition (SVD) for GE matrices</b> */

/*  =========== DOCUMENTATION =========== */

/* Online html documentation available at */
/*            http://www.netlib.org/lapack/explore-html/ */

/* > \htmlonly */
/* > Download SGESVDX + dependencies */
/* > <a href="http://www.netlib.org/cgi-bin/netlibfiles.tgz?format=tgz&filename=/lapack/lapack_routine/sgesvdx
.f"> */
/* > [TGZ]</a> */
/* > <a href="http://www.netlib.org/cgi-bin/netlibfiles.zip?format=zip&filename=/lapack/lapack_routine/sgesvdx
.f"> */
/* > [ZIP]</a> */
/* > <a href="http://www.netlib.org/cgi-bin/netlibfiles.txt?format=txt&filename=/lapack/lapack_routine/sgesvdx
.f"> */
/* > [TXT]</a> */
/* > \endhtmlonly */

/*  Definition: */
/*  =========== */

/*     SUBROUTINE SGESVDX( JOBU, JOBVT, RANGE, M, N, A, LDA, VL, VU, */
/*    $                    IL, IU, NS, S, U, LDU, VT, LDVT, WORK, */
/*    $                    LWORK, IWORK, INFO ) */


/*      CHARACTER          JOBU, JOBVT, RANGE */
/*      INTEGER            IL, INFO, IU, LDA, LDU, LDVT, LWORK, M, N, NS */
/*      REAL               VL, VU */
/*     INTEGER            IWORK( * ) */
/*     REAL               A( LDA, * ), S( * ), U( LDU, * ), */
/*    $                   VT( LDVT, * ), WORK( * ) */


/* > \par Purpose: */
/*  ============= */
/* > */
/* > \verbatim */
/* > */
/* >  SGESVDX computes the singular value decomposition (SVD) of a real */
/* >  M-by-N matrix A, optionally computing the left and/or right singular */
/* >  vectors. The SVD is written */
/* > */
/* >      A = U * SIGMA * transpose(V) */
/* > */
/* >  where SIGMA is an M-by-N matrix which is zero except for its */
/* >  f2cmin(m,n) diagonal elements, U is an M-by-M orthogonal matrix, and */
/* >  V is an N-by-N orthogonal matrix.  The diagonal elements of SIGMA */
/* >  are the singular values of A; they are real and non-negative, and */
/* >  are returned in descending order.  The first f2cmin(m,n) columns of */
/* >  U and V are the left and right singular vectors of A. */
/* > */
/* >  SGESVDX uses an eigenvalue problem for obtaining the SVD, which */
/* >  allows for the computation of a subset of singular values and */
/* >  vectors. See SBDSVDX for details. */
/* > */
/* >  Note that the routine returns V**T, not V. */
/* > \endverbatim */

/*  Arguments: */
/*  ========== */

/* > \param[in] JOBU */
/* > \verbatim */
/* >          JOBU is CHARACTER*1 */
/* >          Specifies options for computing all or part of the matrix U: */
/* >          = 'V':  the first f2cmin(m,n) columns of U (the left singular */
/* >                  vectors) or as specified by RANGE are returned in */
/* >                  the array U; */
/* >          = 'N':  no columns of U (no left singular vectors) are */
/* >                  computed. */
/* > \endverbatim */
/* > */
/* > \param[in] JOBVT */
/* > \verbatim */
/* >          JOBVT is CHARACTER*1 */
/* >           Specifies options for computing all or part of the matrix */
/* >           V**T: */
/* >           = 'V':  the first f2cmin(m,n) rows of V**T (the right singular */
/* >                   vectors) or as specified by RANGE are returned in */
/* >                   the array VT; */
/* >           = 'N':  no rows of V**T (no right singular vectors) are */
/* >                   computed. */
/* > \endverbatim */
/* > */
/* > \param[in] RANGE */
/* > \verbatim */
/* >          RANGE is CHARACTER*1 */
/* >          = 'A': all singular values will be found. */
/* >          = 'V': all singular values in the half-open interval (VL,VU] */
/* >                 will be found. */
/* >          = 'I': the IL-th through IU-th singular values will be found. */
/* > \endverbatim */
/* > */
/* > \param[in] M */
/* > \verbatim */
/* >          M is INTEGER */
/* >          The number of rows of the input matrix A.  M >= 0. */
/* > \endverbatim */
/* > */
/* > \param[in] N */
/* > \verbatim */
/* >          N is INTEGER */
/* >          The number of columns of the input matrix A.  N >= 0. */
/* > \endverbatim */
/* > */
/* > \param[in,out] A */
/* > \verbatim */
/* >          A is REAL array, dimension (LDA,N) */
/* >          On entry, the M-by-N matrix A. */
/* >          On exit, the contents of A are destroyed. */
/* > \endverbatim */
/* > */
/* > \param[in] LDA */
/* > \verbatim */
/* >          LDA is INTEGER */
/* >          The leading dimension of the array A.  LDA >= f2cmax(1,M). */
/* > \endverbatim */
/* > */
/* > \param[in] VL */
/* > \verbatim */
/* >          VL is REAL */
/* >          If RANGE='V', the lower bound of the interval to */
/* >          be searched for singular values. VU > VL. */
/* >          Not referenced if RANGE = 'A' or 'I'. */
/* > \endverbatim */
/* > */
/* > \param[in] VU */
/* > \verbatim */
/* >          VU is REAL */
/* >          If RANGE='V', the upper bound of the interval to */
/* >          be searched for singular values. VU > VL. */
/* >          Not referenced if RANGE = 'A' or 'I'. */
/* > \endverbatim */
/* > */
/* > \param[in] IL */
/* > \verbatim */
/* >          IL is INTEGER */
/* >          If RANGE='I', the index of the */
/* >          smallest singular value to be returned. */
/* >          1 <= IL <= IU <= f2cmin(M,N), if f2cmin(M,N) > 0. */
/* >          Not referenced if RANGE = 'A' or 'V'. */
/* > \endverbatim */
/* > */
/* > \param[in] IU */
/* > \verbatim */
/* >          IU is INTEGER */
/* >          If RANGE='I', the index of the */
/* >          largest singular value to be returned. */
/* >          1 <= IL <= IU <= f2cmin(M,N), if f2cmin(M,N) > 0. */
/* >          Not referenced if RANGE = 'A' or 'V'. */
/* > \endverbatim */
/* > */
/* > \param[out] NS */
/* > \verbatim */
/* >          NS is INTEGER */
/* >          The total number of singular values found, */
/* >          0 <= NS <= f2cmin(M,N). */
/* >          If RANGE = 'A', NS = f2cmin(M,N); if RANGE = 'I', NS = IU-IL+1. */
/* > \endverbatim */
/* > */
/* > \param[out] S */
/* > \verbatim */
/* >          S is REAL array, dimension (f2cmin(M,N)) */
/* >          The singular values of A, sorted so that S(i) >= S(i+1). */
/* > \endverbatim */
/* > */
/* > \param[out] U */
/* > \verbatim */
/* >          U is REAL array, dimension (LDU,UCOL) */
/* >          If JOBU = 'V', U contains columns of U (the left singular */
/* >          vectors, stored columnwise) as specified by RANGE; if */
/* >          JOBU = 'N', U is not referenced. */
/* >          Note: The user must ensure that UCOL >= NS; if RANGE = 'V', */
/* >          the exact value of NS is not known in advance and an upper */
/* >          bound must be used. */
/* > \endverbatim */
/* > */
/* > \param[in] LDU */
/* > \verbatim */
/* >          LDU is INTEGER */
/* >          The leading dimension of the array U.  LDU >= 1; if */
/* >          JOBU = 'V', LDU >= M. */
/* > \endverbatim */
/* > */
/* > \param[out] VT */
/* > \verbatim */
/* >          VT is REAL array, dimension (LDVT,N) */
/* >          If JOBVT = 'V', VT contains the rows of V**T (the right singular */
/* >          vectors, stored rowwise) as specified by RANGE; if JOBVT = 'N', */
/* >          VT is not referenced. */
/* >          Note: The user must ensure that LDVT >= NS; if RANGE = 'V', */
/* >          the exact value of NS is not known in advance and an upper */
/* >          bound must be used. */
/* > \endverbatim */
/* > */
/* > \param[in] LDVT */
/* > \verbatim */
/* >          LDVT is INTEGER */
/* >          The leading dimension of the array VT.  LDVT >= 1; if */
/* >          JOBVT = 'V', LDVT >= NS (see above). */
/* > \endverbatim */
/* > */
/* > \param[out] WORK */
/* > \verbatim */
/* >          WORK is REAL array, dimension (MAX(1,LWORK)) */
/* >          On exit, if INFO = 0, WORK(1) returns the optimal LWORK; */
/* > \endverbatim */
/* > */
/* > \param[in] LWORK */
/* > \verbatim */
/* >          LWORK is INTEGER */
/* >          The dimension of the array WORK. */
/* >          LWORK >= MAX(1,MIN(M,N)*(MIN(M,N)+4)) for the paths (see */
/* >          comments inside the code): */
/* >             - PATH 1  (M much larger than N) */
/* >             - PATH 1t (N much larger than M) */
/* >          LWORK >= MAX(1,MIN(M,N)*2+MAX(M,N)) for the other paths. */
/* >          For good performance, LWORK should generally be larger. */
/* > */
/* >          If LWORK = -1, then a workspace query is assumed; the routine */
/* >          only calculates the optimal size of the WORK array, returns */
/* >          this value as the first entry of the WORK array, and no error */
/* >          message related to LWORK is issued by XERBLA. */
/* > \endverbatim */
/* > */
/* > \param[out] IWORK */
/* > \verbatim */
/* >          IWORK is INTEGER array, dimension (12*MIN(M,N)) */
/* >          If INFO = 0, the first NS elements of IWORK are zero. If INFO > 0, */
/* >          then IWORK contains the indices of the eigenvectors that failed */
/* >          to converge in SBDSVDX/SSTEVX. */
/* > \endverbatim */
/* > */
/* > \param[out] INFO */
/* > \verbatim */
/* >     INFO is INTEGER */
/* >           = 0:  successful exit */
/* >           < 0:  if INFO = -i, the i-th argument had an illegal value */
/* >           > 0:  if INFO = i, then i eigenvectors failed to converge */
/* >                 in SBDSVDX/SSTEVX. */
/* >                 if INFO = N*2 + 1, an internal error occurred in */
/* >                 SBDSVDX */
/* > \endverbatim */

/*  Authors: */
/*  ======== */

/* > \author Univ. of Tennessee */
/* > \author Univ. of California Berkeley */
/* > \author Univ. of Colorado Denver */
/* > \author NAG Ltd. */

/* > \date June 2016 */

/* > \ingroup realGEsing */

/*  ===================================================================== */
/* Subroutine */ void sgesvdx_(char *jobu, char *jobvt, char *range, integer *
	m, integer *n, real *a, integer *lda, real *vl, real *vu, integer *il,
	 integer *iu, integer *ns, real *s, real *u, integer *ldu, real *vt, 
	integer *ldvt, real *work, integer *lwork, integer *iwork, integer *
	info)
{
    /* System generated locals */
    address a__1[2];
    integer a_dim1, a_offset, u_dim1, u_offset, vt_dim1, vt_offset, i__1[2], 
	    i__2, i__3;
    char ch__1[2];

    /* Local variables */
    integer iscl;
    logical alls, inds;
    integer ilqf;
    real anrm;
    integer ierr, iqrf, itau;
    char jobz[1];
    logical vals;
    integer i__, j;
    extern logical lsame_(char *, char *);
    integer iltgk, itemp, minmn, itaup, itauq, iutgk, itgkz, mnthr;
    extern /* Subroutine */ void scopy_(integer *, real *, integer *, real *, 
	    integer *);
    logical wantu;
    integer id, ie;
    extern /* Subroutine */ void sgebrd_(integer *, integer *, real *, integer 
	    *, real *, real *, real *, real *, real *, integer *, integer *);
    extern real slamch_(char *), slange_(char *, integer *, integer *,
	     real *, integer *, real *);
    extern /* Subroutine */ int xerbla_(char *, integer *, ftnlen);
    extern integer ilaenv_(integer *, char *, char *, integer *, integer *, 
	    integer *, integer *, ftnlen, ftnlen);
    real bignum;
    extern /* Subroutine */ void sgelqf_(integer *, integer *, real *, integer 
	    *, real *, real *, integer *, integer *), slascl_(char *, integer 
	    *, integer *, real *, real *, integer *, integer *, real *, 
	    integer *, integer *);
    real abstol;
    extern /* Subroutine */ void sgeqrf_(integer *, integer *, real *, integer 
	    *, real *, real *, integer *, integer *), slacpy_(char *, integer 
	    *, integer *, real *, integer *, real *, integer *);
    char rngtgk[1];
    extern /* Subroutine */ void slaset_(char *, integer *, integer *, real *, 
	    real *, real *, integer *), sormbr_(char *, char *, char *
	    , integer *, integer *, integer *, real *, integer *, real *, 
	    real *, integer *, real *, integer *, integer *);
    integer minwrk, maxwrk;
    real smlnum;
    extern /* Subroutine */ void sormlq_(char *, char *, integer *, integer *, 
	    integer *, real *, integer *, real *, real *, integer *, real *, 
	    integer *, integer *);
    logical lquery, wantvt;
    extern /* Subroutine */ void sormqr_(char *, char *, integer *, integer *, 
	    integer *, real *, integer *, real *, real *, integer *, real *, 
	    integer *, integer *);
    real dum[1], eps;
    extern /* Subroutine */ void sbdsvdx_(char *, char *, char *, integer *, 
	    real *, real *, real *, real *, integer *, integer *, integer *, 
	    real *, real *, integer *, real *, integer *, integer *);


/*  -- LAPACK driver routine (version 3.8.0) -- */
/*  -- LAPACK is a software package provided by Univ. of Tennessee,    -- */
/*  -- Univ. of California Berkeley, Univ. of Colorado Denver and NAG Ltd..-- */
/*     June 2016 */


/*  ===================================================================== */


/*     Test the input arguments. */

    /* Parameter adjustments */
    a_dim1 = *lda;
    a_offset = 1 + a_dim1 * 1;
    a -= a_offset;
    --s;
    u_dim1 = *ldu;
    u_offset = 1 + u_dim1 * 1;
    u -= u_offset;
    vt_dim1 = *ldvt;
    vt_offset = 1 + vt_dim1 * 1;
    vt -= vt_offset;
    --work;
    --iwork;

    /* Function Body */
    *ns = 0;
    *info = 0;
    abstol = slamch_("S") * 2;
    lquery = *lwork == -1;
    minmn = f2cmin(*m,*n);
    wantu = lsame_(jobu, "V");
    wantvt = lsame_(jobvt, "V");
    if (wantu || wantvt) {
	*(unsigned char *)jobz = 'V';
    } else {
	*(unsigned char *)jobz = 'N';
    }
    alls = lsame_(range, "A");
    vals = lsame_(range, "V");
    inds = lsame_(range, "I");

    *info = 0;
    if (! lsame_(jobu, "V") && ! lsame_(jobu, "N")) {
	*info = -1;
    } else if (! lsame_(jobvt, "V") && ! lsame_(jobvt, 
	    "N")) {
	*info = -2;
    } else if (! (alls || vals || inds)) {
	*info = -3;
    } else if (*m < 0) {
	*info = -4;
    } else if (*n < 0) {
	*info = -5;
    } else if (*m > *lda) {
	*info = -7;
    } else if (minmn > 0) {
	if (vals) {
	    if (*vl < 0.f) {
		*info = -8;
	    } else if (*vu <= *vl) {
		*info = -9;
	    }
	} else if (inds) {
	    if (*il < 1 || *il > f2cmax(1,minmn)) {
		*info = -10;
	    } else if (*iu < f2cmin(minmn,*il) || *iu > minmn) {
		*info = -11;
	    }
	}
	if (*info == 0) {
	    if (wantu && *ldu < *m) {
		*info = -15;
	    } else if (wantvt) {
		if (inds) {
		    if (*ldvt < *iu - *il + 1) {
			*info = -17;
		    }
		} else if (*ldvt < minmn) {
		    *info = -17;
		}
	    }
	}
    }

/*     Compute workspace */
/*     (Note: Comments in the code beginning "Workspace:" describe the */
/*     minimal amount of workspace needed at that point in the code, */
/*     as well as the preferred amount for good performance. */
/*     NB refers to the optimal block size for the immediately */
/*     following subroutine, as returned by ILAENV.) */

    if (*info == 0) {
	minwrk = 1;
	maxwrk = 1;
	if (minmn > 0) {
	    if (*m >= *n) {
/* Writing concatenation */
		i__1[0] = 1, a__1[0] = jobu;
		i__1[1] = 1, a__1[1] = jobvt;
		s_cat(ch__1, a__1, i__1, &c__2, (ftnlen)2);
		mnthr = ilaenv_(&c__6, "SGESVD", ch__1, m, n, &c__0, &c__0, (
			ftnlen)6, (ftnlen)2);
		if (*m >= mnthr) {

/*                 Path 1 (M much larger than N) */

		    maxwrk = *n + *n * ilaenv_(&c__1, "SGEQRF", " ", m, n, &
			    c_n1, &c_n1, (ftnlen)6, (ftnlen)1);
/* Computing MAX */
		    i__2 = maxwrk, i__3 = *n * (*n + 5) + (*n << 1) * ilaenv_(
			    &c__1, "SGEBRD", " ", n, n, &c_n1, &c_n1, (ftnlen)
			    6, (ftnlen)1);
		    maxwrk = f2cmax(i__2,i__3);
		    if (wantu) {
/* Computing MAX */
			i__2 = maxwrk, i__3 = *n * (*n * 3 + 6) + *n * 
				ilaenv_(&c__1, "SORMQR", " ", n, n, &c_n1, &
				c_n1, (ftnlen)6, (ftnlen)1);
			maxwrk = f2cmax(i__2,i__3);
		    }
		    if (wantvt) {
/* Computing MAX */
			i__2 = maxwrk, i__3 = *n * (*n * 3 + 6) + *n * 
				ilaenv_(&c__1, "SORMLQ", " ", n, n, &c_n1, &
				c_n1, (ftnlen)6, (ftnlen)1);
			maxwrk = f2cmax(i__2,i__3);
		    }
		    minwrk = *n * (*n * 3 + 20);
		} else {

/*                 Path 2 (M at least N, but not much larger) */

		    maxwrk = (*n << 2) + (*m + *n) * ilaenv_(&c__1, "SGEBRD", 
			    " ", m, n, &c_n1, &c_n1, (ftnlen)6, (ftnlen)1);
		    if (wantu) {
/* Computing MAX */
			i__2 = maxwrk, i__3 = *n * ((*n << 1) + 5) + *n * 
				ilaenv_(&c__1, "SORMQR", " ", n, n, &c_n1, &
				c_n1, (ftnlen)6, (ftnlen)1);
			maxwrk = f2cmax(i__2,i__3);
		    }
		    if (wantvt) {
/* Computing MAX */
			i__2 = maxwrk, i__3 = *n * ((*n << 1) + 5) + *n * 
				ilaenv_(&c__1, "SORMLQ", " ", n, n, &c_n1, &
				c_n1, (ftnlen)6, (ftnlen)1);
			maxwrk = f2cmax(i__2,i__3);
		    }
/* Computing MAX */
		    i__2 = *n * ((*n << 1) + 19), i__3 = (*n << 2) + *m;
		    minwrk = f2cmax(i__2,i__3);
		}
	    } else {
/* Writing concatenation */
		i__1[0] = 1, a__1[0] = jobu;
		i__1[1] = 1, a__1[1] = jobvt;
		s_cat(ch__1, a__1, i__1, &c__2, (ftnlen)2);
		mnthr = ilaenv_(&c__6, "SGESVD", ch__1, m, n, &c__0, &c__0, (
			ftnlen)6, (ftnlen)2);
		if (*n >= mnthr) {

/*                 Path 1t (N much larger than M) */

		    maxwrk = *m + *m * ilaenv_(&c__1, "SGELQF", " ", m, n, &
			    c_n1, &c_n1, (ftnlen)6, (ftnlen)1);
/* Computing MAX */
		    i__2 = maxwrk, i__3 = *m * (*m + 5) + (*m << 1) * ilaenv_(
			    &c__1, "SGEBRD", " ", m, m, &c_n1, &c_n1, (ftnlen)
			    6, (ftnlen)1);
		    maxwrk = f2cmax(i__2,i__3);
		    if (wantu) {
/* Computing MAX */
			i__2 = maxwrk, i__3 = *m * (*m * 3 + 6) + *m * 
				ilaenv_(&c__1, "SORMQR", " ", m, m, &c_n1, &
				c_n1, (ftnlen)6, (ftnlen)1);
			maxwrk = f2cmax(i__2,i__3);
		    }
		    if (wantvt) {
/* Computing MAX */
			i__2 = maxwrk, i__3 = *m * (*m * 3 + 6) + *m * 
				ilaenv_(&c__1, "SORMLQ", " ", m, m, &c_n1, &
				c_n1, (ftnlen)6, (ftnlen)1);
			maxwrk = f2cmax(i__2,i__3);
		    }
		    minwrk = *m * (*m * 3 + 20);
		} else {

/*                 Path 2t (N at least M, but not much larger) */

		    maxwrk = (*m << 2) + (*m + *n) * ilaenv_(&c__1, "SGEBRD", 
			    " ", m, n, &c_n1, &c_n1, (ftnlen)6, (ftnlen)1);
		    if (wantu) {
/* Computing MAX */
			i__2 = maxwrk, i__3 = *m * ((*m << 1) + 5) + *m * 
				ilaenv_(&c__1, "SORMQR", " ", m, m, &c_n1, &
				c_n1, (ftnlen)6, (ftnlen)1);
			maxwrk = f2cmax(i__2,i__3);
		    }
		    if (wantvt) {
/* Computing MAX */
			i__2 = maxwrk, i__3 = *m * ((*m << 1) + 5) + *m * 
				ilaenv_(&c__1, "SORMLQ", " ", m, m, &c_n1, &
				c_n1, (ftnlen)6, (ftnlen)1);
			maxwrk = f2cmax(i__2,i__3);
		    }
/* Computing MAX */
		    i__2 = *m * ((*m << 1) + 19), i__3 = (*m << 2) + *n;
		    minwrk = f2cmax(i__2,i__3);
		}
	    }
	}
	maxwrk = f2cmax(maxwrk,minwrk);
	work[1] = (real) maxwrk;

	if (*lwork < minwrk && ! lquery) {
	    *info = -19;
	}
    }

    if (*info != 0) {
	i__2 = -(*info);
	xerbla_("SGESVDX", &i__2, (ftnlen)7);
	return;
    } else if (lquery) {
	return;
    }

/*     Quick return if possible */

    if (*m == 0 || *n == 0) {
	return;
    }

/*     Set singular values indices accord to RANGE. */

    if (alls) {
	*(unsigned char *)rngtgk = 'I';
	iltgk = 1;
	iutgk = f2cmin(*m,*n);
    } else if (inds) {
	*(unsigned char *)rngtgk = 'I';
	iltgk = *il;
	iutgk = *iu;
    } else {
	*(unsigned char *)rngtgk = 'V';
	iltgk = 0;
	iutgk = 0;
    }

/*     Get machine constants */

    eps = slamch_("P");
    smlnum = sqrt(slamch_("S")) / eps;
    bignum = 1.f / smlnum;

/*     Scale A if f2cmax element outside range [SMLNUM,BIGNUM] */

    anrm = slange_("M", m, n, &a[a_offset], lda, dum);
    iscl = 0;
    if (anrm > 0.f && anrm < smlnum) {
	iscl = 1;
	slascl_("G", &c__0, &c__0, &anrm, &smlnum, m, n, &a[a_offset], lda, 
		info);
    } else if (anrm > bignum) {
	iscl = 1;
	slascl_("G", &c__0, &c__0, &anrm, &bignum, m, n, &a[a_offset], lda, 
		info);
    }

    if (*m >= *n) {

/*        A has at least as many rows as columns. If A has sufficiently */
/*        more rows than columns, first reduce A using the QR */
/*        decomposition. */

	if (*m >= mnthr) {

/*           Path 1 (M much larger than N): */
/*           A = Q * R = Q * ( QB * B * PB**T ) */
/*                     = Q * ( QB * ( UB * S * VB**T ) * PB**T ) */
/*           U = Q * QB * UB; V**T = VB**T * PB**T */

/*           Compute A=Q*R */
/*           (Workspace: need 2*N, prefer N+N*NB) */

	    itau = 1;
	    itemp = itau + *n;
	    i__2 = *lwork - itemp + 1;
	    sgeqrf_(m, n, &a[a_offset], lda, &work[itau], &work[itemp], &i__2,
		     info);

/*           Copy R into WORK and bidiagonalize it: */
/*           (Workspace: need N*N+5*N, prefer N*N+4*N+2*N*NB) */

	    iqrf = itemp;
	    id = iqrf + *n * *n;
	    ie = id + *n;
	    itauq = ie + *n;
	    itaup = itauq + *n;
	    itemp = itaup + *n;
	    slacpy_("U", n, n, &a[a_offset], lda, &work[iqrf], n);
	    i__2 = *n - 1;
	    i__3 = *n - 1;
	    slaset_("L", &i__2, &i__3, &c_b109, &c_b109, &work[iqrf + 1], n);
	    i__2 = *lwork - itemp + 1;
	    sgebrd_(n, n, &work[iqrf], n, &work[id], &work[ie], &work[itauq], 
		    &work[itaup], &work[itemp], &i__2, info);

/*           Solve eigenvalue problem TGK*Z=Z*S. */
/*           (Workspace: need 14*N + 2*N*(N+1)) */

	    itgkz = itemp;
	    itemp = itgkz + *n * ((*n << 1) + 1);
	    i__2 = *n << 1;
	    sbdsvdx_("U", jobz, rngtgk, n, &work[id], &work[ie], vl, vu, &
		    iltgk, &iutgk, ns, &s[1], &work[itgkz], &i__2, &work[
		    itemp], &iwork[1], info);

/*           If needed, compute left singular vectors. */

	    if (wantu) {
		j = itgkz;
		i__2 = *ns;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    scopy_(n, &work[j], &c__1, &u[i__ * u_dim1 + 1], &c__1);
		    j += *n << 1;
		}
		i__2 = *m - *n;
		slaset_("A", &i__2, ns, &c_b109, &c_b109, &u[*n + 1 + u_dim1],
			 ldu);

/*              Call SORMBR to compute QB*UB. */
/*              (Workspace in WORK( ITEMP ): need N, prefer N*NB) */

		i__2 = *lwork - itemp + 1;
		sormbr_("Q", "L", "N", n, ns, n, &work[iqrf], n, &work[itauq],
			 &u[u_offset], ldu, &work[itemp], &i__2, info);

/*              Call SORMQR to compute Q*(QB*UB). */
/*              (Workspace in WORK( ITEMP ): need N, prefer N*NB) */

		i__2 = *lwork - itemp + 1;
		sormqr_("L", "N", m, ns, n, &a[a_offset], lda, &work[itau], &
			u[u_offset], ldu, &work[itemp], &i__2, info);
	    }

/*           If needed, compute right singular vectors. */

	    if (wantvt) {
		j = itgkz + *n;
		i__2 = *ns;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    scopy_(n, &work[j], &c__1, &vt[i__ + vt_dim1], ldvt);
		    j += *n << 1;
		}

/*              Call SORMBR to compute VB**T * PB**T */
/*              (Workspace in WORK( ITEMP ): need N, prefer N*NB) */

		i__2 = *lwork - itemp + 1;
		sormbr_("P", "R", "T", ns, n, n, &work[iqrf], n, &work[itaup],
			 &vt[vt_offset], ldvt, &work[itemp], &i__2, info);
	    }
	} else {

/*           Path 2 (M at least N, but not much larger) */
/*           Reduce A to bidiagonal form without QR decomposition */
/*           A = QB * B * PB**T = QB * ( UB * S * VB**T ) * PB**T */
/*           U = QB * UB; V**T = VB**T * PB**T */

/*           Bidiagonalize A */
/*           (Workspace: need 4*N+M, prefer 4*N+(M+N)*NB) */

	    id = 1;
	    ie = id + *n;
	    itauq = ie + *n;
	    itaup = itauq + *n;
	    itemp = itaup + *n;
	    i__2 = *lwork - itemp + 1;
	    sgebrd_(m, n, &a[a_offset], lda, &work[id], &work[ie], &work[
		    itauq], &work[itaup], &work[itemp], &i__2, info);

/*           Solve eigenvalue problem TGK*Z=Z*S. */
/*           (Workspace: need 14*N + 2*N*(N+1)) */

	    itgkz = itemp;
	    itemp = itgkz + *n * ((*n << 1) + 1);
	    i__2 = *n << 1;
	    sbdsvdx_("U", jobz, rngtgk, n, &work[id], &work[ie], vl, vu, &
		    iltgk, &iutgk, ns, &s[1], &work[itgkz], &i__2, &work[
		    itemp], &iwork[1], info);

/*           If needed, compute left singular vectors. */

	    if (wantu) {
		j = itgkz;
		i__2 = *ns;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    scopy_(n, &work[j], &c__1, &u[i__ * u_dim1 + 1], &c__1);
		    j += *n << 1;
		}
		i__2 = *m - *n;
		slaset_("A", &i__2, ns, &c_b109, &c_b109, &u[*n + 1 + u_dim1],
			 ldu);

/*              Call SORMBR to compute QB*UB. */
/*              (Workspace in WORK( ITEMP ): need N, prefer N*NB) */

		i__2 = *lwork - itemp + 1;
		sormbr_("Q", "L", "N", m, ns, n, &a[a_offset], lda, &work[
			itauq], &u[u_offset], ldu, &work[itemp], &i__2, &ierr);
	    }

/*           If needed, compute right singular vectors. */

	    if (wantvt) {
		j = itgkz + *n;
		i__2 = *ns;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    scopy_(n, &work[j], &c__1, &vt[i__ + vt_dim1], ldvt);
		    j += *n << 1;
		}

/*              Call SORMBR to compute VB**T * PB**T */
/*              (Workspace in WORK( ITEMP ): need N, prefer N*NB) */

		i__2 = *lwork - itemp + 1;
		sormbr_("P", "R", "T", ns, n, n, &a[a_offset], lda, &work[
			itaup], &vt[vt_offset], ldvt, &work[itemp], &i__2, &
			ierr);
	    }
	}
    } else {

/*        A has more columns than rows. If A has sufficiently more */
/*        columns than rows, first reduce A using the LQ decomposition. */

	if (*n >= mnthr) {

/*           Path 1t (N much larger than M): */
/*           A = L * Q = ( QB * B * PB**T ) * Q */
/*                     = ( QB * ( UB * S * VB**T ) * PB**T ) * Q */
/*           U = QB * UB ; V**T = VB**T * PB**T * Q */

/*           Compute A=L*Q */
/*           (Workspace: need 2*M, prefer M+M*NB) */

	    itau = 1;
	    itemp = itau + *m;
	    i__2 = *lwork - itemp + 1;
	    sgelqf_(m, n, &a[a_offset], lda, &work[itau], &work[itemp], &i__2,
		     info);
/*           Copy L into WORK and bidiagonalize it: */
/*           (Workspace in WORK( ITEMP ): need M*M+5*N, prefer M*M+4*M+2*M*NB) */

	    ilqf = itemp;
	    id = ilqf + *m * *m;
	    ie = id + *m;
	    itauq = ie + *m;
	    itaup = itauq + *m;
	    itemp = itaup + *m;
	    slacpy_("L", m, m, &a[a_offset], lda, &work[ilqf], m);
	    i__2 = *m - 1;
	    i__3 = *m - 1;
	    slaset_("U", &i__2, &i__3, &c_b109, &c_b109, &work[ilqf + *m], m);
	    i__2 = *lwork - itemp + 1;
	    sgebrd_(m, m, &work[ilqf], m, &work[id], &work[ie], &work[itauq], 
		    &work[itaup], &work[itemp], &i__2, info);

/*           Solve eigenvalue problem TGK*Z=Z*S. */
/*           (Workspace: need 2*M*M+14*M) */

	    itgkz = itemp;
	    itemp = itgkz + *m * ((*m << 1) + 1);
	    i__2 = *m << 1;
	    sbdsvdx_("U", jobz, rngtgk, m, &work[id], &work[ie], vl, vu, &
		    iltgk, &iutgk, ns, &s[1], &work[itgkz], &i__2, &work[
		    itemp], &iwork[1], info);

/*           If needed, compute left singular vectors. */

	    if (wantu) {
		j = itgkz;
		i__2 = *ns;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    scopy_(m, &work[j], &c__1, &u[i__ * u_dim1 + 1], &c__1);
		    j += *m << 1;
		}

/*              Call SORMBR to compute QB*UB. */
/*              (Workspace in WORK( ITEMP ): need M, prefer M*NB) */

		i__2 = *lwork - itemp + 1;
		sormbr_("Q", "L", "N", m, ns, m, &work[ilqf], m, &work[itauq],
			 &u[u_offset], ldu, &work[itemp], &i__2, info);
	    }

/*           If needed, compute right singular vectors. */

	    if (wantvt) {
		j = itgkz + *m;
		i__2 = *ns;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    scopy_(m, &work[j], &c__1, &vt[i__ + vt_dim1], ldvt);
		    j += *m << 1;
		}
		i__2 = *n - *m;
		slaset_("A", ns, &i__2, &c_b109, &c_b109, &vt[(*m + 1) * 
			vt_dim1 + 1], ldvt);

/*              Call SORMBR to compute (VB**T)*(PB**T) */
/*              (Workspace in WORK( ITEMP ): need M, prefer M*NB) */

		i__2 = *lwork - itemp + 1;
		sormbr_("P", "R", "T", ns, m, m, &work[ilqf], m, &work[itaup],
			 &vt[vt_offset], ldvt, &work[itemp], &i__2, info);

/*              Call SORMLQ to compute ((VB**T)*(PB**T))*Q. */
/*              (Workspace in WORK( ITEMP ): need M, prefer M*NB) */

		i__2 = *lwork - itemp + 1;
		sormlq_("R", "N", ns, n, m, &a[a_offset], lda, &work[itau], &
			vt[vt_offset], ldvt, &work[itemp], &i__2, info);
	    }
	} else {

/*           Path 2t (N greater than M, but not much larger) */
/*           Reduce to bidiagonal form without LQ decomposition */
/*           A = QB * B * PB**T = QB * ( UB * S * VB**T ) * PB**T */
/*           U = QB * UB; V**T = VB**T * PB**T */

/*           Bidiagonalize A */
/*           (Workspace: need 4*M+N, prefer 4*M+(M+N)*NB) */

	    id = 1;
	    ie = id + *m;
	    itauq = ie + *m;
	    itaup = itauq + *m;
	    itemp = itaup + *m;
	    i__2 = *lwork - itemp + 1;
	    sgebrd_(m, n, &a[a_offset], lda, &work[id], &work[ie], &work[
		    itauq], &work[itaup], &work[itemp], &i__2, info);

/*           Solve eigenvalue problem TGK*Z=Z*S. */
/*           (Workspace: need 2*M*M+14*M) */

	    itgkz = itemp;
	    itemp = itgkz + *m * ((*m << 1) + 1);
	    i__2 = *m << 1;
	    sbdsvdx_("L", jobz, rngtgk, m, &work[id], &work[ie], vl, vu, &
		    iltgk, &iutgk, ns, &s[1], &work[itgkz], &i__2, &work[
		    itemp], &iwork[1], info);

/*           If needed, compute left singular vectors. */

	    if (wantu) {
		j = itgkz;
		i__2 = *ns;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    scopy_(m, &work[j], &c__1, &u[i__ * u_dim1 + 1], &c__1);
		    j += *m << 1;
		}

/*              Call SORMBR to compute QB*UB. */
/*              (Workspace in WORK( ITEMP ): need M, prefer M*NB) */

		i__2 = *lwork - itemp + 1;
		sormbr_("Q", "L", "N", m, ns, n, &a[a_offset], lda, &work[
			itauq], &u[u_offset], ldu, &work[itemp], &i__2, info);
	    }

/*           If needed, compute right singular vectors. */

	    if (wantvt) {
		j = itgkz + *m;
		i__2 = *ns;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    scopy_(m, &work[j], &c__1, &vt[i__ + vt_dim1], ldvt);
		    j += *m << 1;
		}
		i__2 = *n - *m;
		slaset_("A", ns, &i__2, &c_b109, &c_b109, &vt[(*m + 1) * 
			vt_dim1 + 1], ldvt);

/*              Call SORMBR to compute VB**T * PB**T */
/*              (Workspace in WORK( ITEMP ): need M, prefer M*NB) */

		i__2 = *lwork - itemp + 1;
		sormbr_("P", "R", "T", ns, n, m, &a[a_offset], lda, &work[
			itaup], &vt[vt_offset], ldvt, &work[itemp], &i__2, 
			info);
	    }
	}
    }

/*     Undo scaling if necessary */

    if (iscl == 1) {
	if (anrm > bignum) {
	    slascl_("G", &c__0, &c__0, &bignum, &anrm, &minmn, &c__1, &s[1], &
		    minmn, info);
	}
	if (anrm < smlnum) {
	    slascl_("G", &c__0, &c__0, &smlnum, &anrm, &minmn, &c__1, &s[1], &
		    minmn, info);
	}
    }

/*     Return optimal workspace in WORK(1) */

    work[1] = (real) maxwrk;

    return;

/*     End of SGESVDX */

} /* sgesvdx_ */

