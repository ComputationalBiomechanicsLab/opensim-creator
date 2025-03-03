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

static integer c__9 = 9;
static integer c__0 = 0;
static integer c__6 = 6;
static integer c_n1 = -1;
static integer c__1 = 1;
static real c_b81 = 0.f;

/* > \brief <b> SGELSD computes the minimum-norm solution to a linear least squares problem for GE matrices</b
> */

/*  =========== DOCUMENTATION =========== */

/* Online html documentation available at */
/*            http://www.netlib.org/lapack/explore-html/ */

/* > \htmlonly */
/* > Download SGELSD + dependencies */
/* > <a href="http://www.netlib.org/cgi-bin/netlibfiles.tgz?format=tgz&filename=/lapack/lapack_routine/sgelsd.
f"> */
/* > [TGZ]</a> */
/* > <a href="http://www.netlib.org/cgi-bin/netlibfiles.zip?format=zip&filename=/lapack/lapack_routine/sgelsd.
f"> */
/* > [ZIP]</a> */
/* > <a href="http://www.netlib.org/cgi-bin/netlibfiles.txt?format=txt&filename=/lapack/lapack_routine/sgelsd.
f"> */
/* > [TXT]</a> */
/* > \endhtmlonly */

/*  Definition: */
/*  =========== */

/*       SUBROUTINE SGELSD( M, N, NRHS, A, LDA, B, LDB, S, RCOND, */
/*                          RANK, WORK, LWORK, IWORK, INFO ) */

/*       INTEGER            INFO, LDA, LDB, LWORK, M, N, NRHS, RANK */
/*       REAL               RCOND */
/*       INTEGER            IWORK( * ) */
/*       REAL               A( LDA, * ), B( LDB, * ), S( * ), WORK( * ) */


/* > \par Purpose: */
/*  ============= */
/* > */
/* > \verbatim */
/* > */
/* > SGELSD computes the minimum-norm solution to a real linear least */
/* > squares problem: */
/* >     minimize 2-norm(| b - A*x |) */
/* > using the singular value decomposition (SVD) of A. A is an M-by-N */
/* > matrix which may be rank-deficient. */
/* > */
/* > Several right hand side vectors b and solution vectors x can be */
/* > handled in a single call; they are stored as the columns of the */
/* > M-by-NRHS right hand side matrix B and the N-by-NRHS solution */
/* > matrix X. */
/* > */
/* > The problem is solved in three steps: */
/* > (1) Reduce the coefficient matrix A to bidiagonal form with */
/* >     Householder transformations, reducing the original problem */
/* >     into a "bidiagonal least squares problem" (BLS) */
/* > (2) Solve the BLS using a divide and conquer approach. */
/* > (3) Apply back all the Householder transformations to solve */
/* >     the original least squares problem. */
/* > */
/* > The effective rank of A is determined by treating as zero those */
/* > singular values which are less than RCOND times the largest singular */
/* > value. */
/* > */
/* > The divide and conquer algorithm makes very mild assumptions about */
/* > floating point arithmetic. It will work on machines with a guard */
/* > digit in add/subtract, or on those binary machines without guard */
/* > digits which subtract like the Cray X-MP, Cray Y-MP, Cray C-90, or */
/* > Cray-2. It could conceivably fail on hexadecimal or decimal machines */
/* > without guard digits, but we know of none. */
/* > \endverbatim */

/*  Arguments: */
/*  ========== */

/* > \param[in] M */
/* > \verbatim */
/* >          M is INTEGER */
/* >          The number of rows of A. M >= 0. */
/* > \endverbatim */
/* > */
/* > \param[in] N */
/* > \verbatim */
/* >          N is INTEGER */
/* >          The number of columns of A. N >= 0. */
/* > \endverbatim */
/* > */
/* > \param[in] NRHS */
/* > \verbatim */
/* >          NRHS is INTEGER */
/* >          The number of right hand sides, i.e., the number of columns */
/* >          of the matrices B and X. NRHS >= 0. */
/* > \endverbatim */
/* > */
/* > \param[in,out] A */
/* > \verbatim */
/* >          A is REAL array, dimension (LDA,N) */
/* >          On entry, the M-by-N matrix A. */
/* >          On exit, A has been destroyed. */
/* > \endverbatim */
/* > */
/* > \param[in] LDA */
/* > \verbatim */
/* >          LDA is INTEGER */
/* >          The leading dimension of the array A.  LDA >= f2cmax(1,M). */
/* > \endverbatim */
/* > */
/* > \param[in,out] B */
/* > \verbatim */
/* >          B is REAL array, dimension (LDB,NRHS) */
/* >          On entry, the M-by-NRHS right hand side matrix B. */
/* >          On exit, B is overwritten by the N-by-NRHS solution */
/* >          matrix X.  If m >= n and RANK = n, the residual */
/* >          sum-of-squares for the solution in the i-th column is given */
/* >          by the sum of squares of elements n+1:m in that column. */
/* > \endverbatim */
/* > */
/* > \param[in] LDB */
/* > \verbatim */
/* >          LDB is INTEGER */
/* >          The leading dimension of the array B. LDB >= f2cmax(1,f2cmax(M,N)). */
/* > \endverbatim */
/* > */
/* > \param[out] S */
/* > \verbatim */
/* >          S is REAL array, dimension (f2cmin(M,N)) */
/* >          The singular values of A in decreasing order. */
/* >          The condition number of A in the 2-norm = S(1)/S(f2cmin(m,n)). */
/* > \endverbatim */
/* > */
/* > \param[in] RCOND */
/* > \verbatim */
/* >          RCOND is REAL */
/* >          RCOND is used to determine the effective rank of A. */
/* >          Singular values S(i) <= RCOND*S(1) are treated as zero. */
/* >          If RCOND < 0, machine precision is used instead. */
/* > \endverbatim */
/* > */
/* > \param[out] RANK */
/* > \verbatim */
/* >          RANK is INTEGER */
/* >          The effective rank of A, i.e., the number of singular values */
/* >          which are greater than RCOND*S(1). */
/* > \endverbatim */
/* > */
/* > \param[out] WORK */
/* > \verbatim */
/* >          WORK is REAL array, dimension (MAX(1,LWORK)) */
/* >          On exit, if INFO = 0, WORK(1) returns the optimal LWORK. */
/* > \endverbatim */
/* > */
/* > \param[in] LWORK */
/* > \verbatim */
/* >          LWORK is INTEGER */
/* >          The dimension of the array WORK. LWORK must be at least 1. */
/* >          The exact minimum amount of workspace needed depends on M, */
/* >          N and NRHS. As long as LWORK is at least */
/* >              12*N + 2*N*SMLSIZ + 8*N*NLVL + N*NRHS + (SMLSIZ+1)**2, */
/* >          if M is greater than or equal to N or */
/* >              12*M + 2*M*SMLSIZ + 8*M*NLVL + M*NRHS + (SMLSIZ+1)**2, */
/* >          if M is less than N, the code will execute correctly. */
/* >          SMLSIZ is returned by ILAENV and is equal to the maximum */
/* >          size of the subproblems at the bottom of the computation */
/* >          tree (usually about 25), and */
/* >             NLVL = MAX( 0, INT( LOG_2( MIN( M,N )/(SMLSIZ+1) ) ) + 1 ) */
/* >          For good performance, LWORK should generally be larger. */
/* > */
/* >          If LWORK = -1, then a workspace query is assumed; the routine */
/* >          only calculates the optimal size of the array WORK and the */
/* >          minimum size of the array IWORK, and returns these values as */
/* >          the first entries of the WORK and IWORK arrays, and no error */
/* >          message related to LWORK is issued by XERBLA. */
/* > \endverbatim */
/* > */
/* > \param[out] IWORK */
/* > \verbatim */
/* >          IWORK is INTEGER array, dimension (MAX(1,LIWORK)) */
/* >          LIWORK >= f2cmax(1, 3*MINMN*NLVL + 11*MINMN), */
/* >          where MINMN = MIN( M,N ). */
/* >          On exit, if INFO = 0, IWORK(1) returns the minimum LIWORK. */
/* > \endverbatim */
/* > */
/* > \param[out] INFO */
/* > \verbatim */
/* >          INFO is INTEGER */
/* >          = 0:  successful exit */
/* >          < 0:  if INFO = -i, the i-th argument had an illegal value. */
/* >          > 0:  the algorithm for computing the SVD failed to converge; */
/* >                if INFO = i, i off-diagonal elements of an intermediate */
/* >                bidiagonal form did not converge to zero. */
/* > \endverbatim */

/*  Authors: */
/*  ======== */

/* > \author Univ. of Tennessee */
/* > \author Univ. of California Berkeley */
/* > \author Univ. of Colorado Denver */
/* > \author NAG Ltd. */

/* > \date June 2017 */

/* > \ingroup realGEsolve */

/* > \par Contributors: */
/*  ================== */
/* > */
/* >     Ming Gu and Ren-Cang Li, Computer Science Division, University of */
/* >       California at Berkeley, USA \n */
/* >     Osni Marques, LBNL/NERSC, USA \n */

/*  ===================================================================== */
/* Subroutine */ void sgelsd_(integer *m, integer *n, integer *nrhs, real *a, 
	integer *lda, real *b, integer *ldb, real *s, real *rcond, integer *
	rank, real *work, integer *lwork, integer *iwork, integer *info)
{
    /* System generated locals */
    integer a_dim1, a_offset, b_dim1, b_offset, i__1, i__2, i__3, i__4;

    /* Local variables */
    real anrm, bnrm;
    integer itau, nlvl, iascl, ibscl;
    real sfmin;
    integer minmn, maxmn, itaup, itauq, mnthr, nwork, ie, il;
    extern /* Subroutine */ void slabad_(real *, real *);
    integer mm;
    extern /* Subroutine */ void sgebrd_(integer *, integer *, real *, integer 
	    *, real *, real *, real *, real *, real *, integer *, integer *);
    extern real slamch_(char *), slange_(char *, integer *, integer *,
	     real *, integer *, real *);
    extern /* Subroutine */ int xerbla_(char *, integer *, ftnlen);
    extern integer ilaenv_(integer *, char *, char *, integer *, integer *, 
	    integer *, integer *, ftnlen, ftnlen);
    real bignum;
    extern /* Subroutine */ void sgelqf_(integer *, integer *, real *, integer 
	    *, real *, real *, integer *, integer *), slalsd_(char *, integer 
	    *, integer *, integer *, real *, real *, real *, integer *, real *
	    , integer *, real *, integer *, integer *), slascl_(char *
	    , integer *, integer *, real *, real *, integer *, integer *, 
	    real *, integer *, integer *);
    integer wlalsd;
    extern /* Subroutine */ void sgeqrf_(integer *, integer *, real *, integer 
	    *, real *, real *, integer *, integer *), slacpy_(char *, integer 
	    *, integer *, real *, integer *, real *, integer *), 
	    slaset_(char *, integer *, integer *, real *, real *, real *, 
	    integer *);
    integer ldwork;
    extern /* Subroutine */ void sormbr_(char *, char *, char *, integer *, 
	    integer *, integer *, real *, integer *, real *, real *, integer *
	    , real *, integer *, integer *);
    integer liwork, minwrk, maxwrk;
    real smlnum;
    extern /* Subroutine */ void sormlq_(char *, char *, integer *, integer *, 
	    integer *, real *, integer *, real *, real *, integer *, real *, 
	    integer *, integer *);
    logical lquery;
    integer smlsiz;
    extern /* Subroutine */ void sormqr_(char *, char *, integer *, integer *, 
	    integer *, real *, integer *, real *, real *, integer *, real *, 
	    integer *, integer *);
    real eps;


/*  -- LAPACK driver routine (version 3.7.1) -- */
/*  -- LAPACK is a software package provided by Univ. of Tennessee,    -- */
/*  -- Univ. of California Berkeley, Univ. of Colorado Denver and NAG Ltd..-- */
/*     June 2017 */


/*  ===================================================================== */


/*     Test the input arguments. */

    /* Parameter adjustments */
    a_dim1 = *lda;
    a_offset = 1 + a_dim1;
    a -= a_offset;
    b_dim1 = *ldb;
    b_offset = 1 + b_dim1;
    b -= b_offset;
    --s;
    --work;
    --iwork;
fprintf(stdout,"start of SGELSD\n");
    /* Function Body */
    *info = 0;
    minmn = f2cmin(*m,*n);
    maxmn = f2cmax(*m,*n);
    lquery = *lwork == -1;
    if (*m < 0) {
	*info = -1;
    } else if (*n < 0) {
	*info = -2;
    } else if (*nrhs < 0) {
	*info = -3;
    } else if (*lda < f2cmax(1,*m)) {
	*info = -5;
    } else if (*ldb < f2cmax(1,maxmn)) {
	*info = -7;
    }

/*     Compute workspace. */
/*     (Note: Comments in the code beginning "Workspace:" describe the */
/*     minimal amount of workspace needed at that point in the code, */
/*     as well as the preferred amount for good performance. */
/*     NB refers to the optimal block size for the immediately */
/*     following subroutine, as returned by ILAENV.) */

    if (*info == 0) {
	minwrk = 1;
	maxwrk = 1;
	liwork = 1;
	if (minmn > 0) {
	    smlsiz = ilaenv_(&c__9, "SGELSD", " ", &c__0, &c__0, &c__0, &c__0,
		     (ftnlen)6, (ftnlen)1);
	    mnthr = ilaenv_(&c__6, "SGELSD", " ", m, n, nrhs, &c_n1, (ftnlen)
		    6, (ftnlen)1);
/* Computing MAX */
	    i__1 = (integer) (logf((real) minmn / (real) (smlsiz + 1)) / logf(
		    2.f)) + 1;
	    nlvl = f2cmax(i__1,0);
	    liwork = minmn * 3 * nlvl + minmn * 11;
	    mm = *m;
	    if (*m >= *n && *m >= mnthr) {

/*              Path 1a - overdetermined, with many more rows than */
/*                        columns. */

		mm = *n;
/* Computing MAX */
		i__1 = maxwrk, i__2 = *n + *n * ilaenv_(&c__1, "SGEQRF", 
			" ", m, n, &c_n1, &c_n1, (ftnlen)6, (ftnlen)1);
		maxwrk = f2cmax(i__1,i__2);
/* Computing MAX */
		i__1 = maxwrk, i__2 = *n + *nrhs * ilaenv_(&c__1, "SORMQR", 
			"LT", m, nrhs, n, &c_n1, (ftnlen)6, (ftnlen)2);
		maxwrk = f2cmax(i__1,i__2);
	    }
	    if (*m >= *n) {

/*              Path 1 - overdetermined or exactly determined. */

/* Computing MAX */
		i__1 = maxwrk, i__2 = *n * 3 + (mm + *n) * ilaenv_(&c__1, 
			"SGEBRD", " ", &mm, n, &c_n1, &c_n1, (ftnlen)6, (
			ftnlen)1);
		maxwrk = f2cmax(i__1,i__2);
/* Computing MAX */
		i__1 = maxwrk, i__2 = *n * 3 + *nrhs * ilaenv_(&c__1, "SORMBR"
			, "QLT", &mm, nrhs, n, &c_n1, (ftnlen)6, (ftnlen)3);
		maxwrk = f2cmax(i__1,i__2);
/* Computing MAX */
		i__1 = maxwrk, i__2 = *n * 3 + (*n - 1) * ilaenv_(&c__1, 
			"SORMBR", "PLN", n, nrhs, n, &c_n1, (ftnlen)6, (
			ftnlen)3);
		maxwrk = f2cmax(i__1,i__2);
/* Computing 2nd power */
		i__1 = smlsiz + 1;
		wlalsd = *n * 9 + (*n << 1) * smlsiz + (*n << 3) * nlvl + *n *
			 *nrhs + i__1 * i__1;
/* Computing MAX */
		i__1 = maxwrk, i__2 = *n * 3 + wlalsd;
		maxwrk = f2cmax(i__1,i__2);
/* Computing MAX */
		i__1 = *n * 3 + mm, i__2 = *n * 3 + *nrhs, i__1 = f2cmax(i__1,
			i__2), i__2 = *n * 3 + wlalsd;
		minwrk = f2cmax(i__1,i__2);
	    }
	    if (*n > *m) {
/* Computing 2nd power */
		i__1 = smlsiz + 1;
		wlalsd = *m * 9 + (*m << 1) * smlsiz + (*m << 3) * nlvl + *m *
			 *nrhs + i__1 * i__1;
		if (*n >= mnthr) {

/*                 Path 2a - underdetermined, with many more columns */
/*                           than rows. */

		    maxwrk = *m + *m * ilaenv_(&c__1, "SGELQF", " ", m, n, &
			    c_n1, &c_n1, (ftnlen)6, (ftnlen)1);
/* Computing MAX */
		    i__1 = maxwrk, i__2 = *m * *m + (*m << 2) + (*m << 1) * 
			    ilaenv_(&c__1, "SGEBRD", " ", m, m, &c_n1, &c_n1, 
			    (ftnlen)6, (ftnlen)1);
		    maxwrk = f2cmax(i__1,i__2);
/* Computing MAX */
		    i__1 = maxwrk, i__2 = *m * *m + (*m << 2) + *nrhs * 
			    ilaenv_(&c__1, "SORMBR", "QLT", m, nrhs, m, &c_n1,
			     (ftnlen)6, (ftnlen)3);
		    maxwrk = f2cmax(i__1,i__2);
/* Computing MAX */
		    i__1 = maxwrk, i__2 = *m * *m + (*m << 2) + (*m - 1) * 
			    ilaenv_(&c__1, "SORMBR", "PLN", m, nrhs, m, &c_n1,
			     (ftnlen)6, (ftnlen)3);
		    maxwrk = f2cmax(i__1,i__2);
		    if (*nrhs > 1) {
/* Computing MAX */
			i__1 = maxwrk, i__2 = *m * *m + *m + *m * *nrhs;
			maxwrk = f2cmax(i__1,i__2);
		    } else {
/* Computing MAX */
			i__1 = maxwrk, i__2 = *m * *m + (*m << 1);
			maxwrk = f2cmax(i__1,i__2);
		    }
/* Computing MAX */
		    i__1 = maxwrk, i__2 = *m + *nrhs * ilaenv_(&c__1, "SORMLQ"
			    , "LT", n, nrhs, m, &c_n1, (ftnlen)6, (ftnlen)2);
		    maxwrk = f2cmax(i__1,i__2);
/* Computing MAX */
		    i__1 = maxwrk, i__2 = *m * *m + (*m << 2) + wlalsd;
		    maxwrk = f2cmax(i__1,i__2);
/*     XXX: Ensure the Path 2a case below is triggered.  The workspace */
/*     calculation should use queries for all routines eventually. */
/* Computing MAX */
/* Computing MAX */
		    i__3 = *m, i__4 = (*m << 1) - 4, i__3 = f2cmax(i__3,i__4), 
			    i__3 = f2cmax(i__3,*nrhs), i__4 = *n - *m * 3;
		    i__1 = maxwrk, i__2 = (*m << 2) + *m * *m + f2cmax(i__3,i__4)
			    ;
		    maxwrk = f2cmax(i__1,i__2);
		} else {

/*                 Path 2 - remaining underdetermined cases. */

		    maxwrk = *m * 3 + (*n + *m) * ilaenv_(&c__1, "SGEBRD", 
			    " ", m, n, &c_n1, &c_n1, (ftnlen)6, (ftnlen)1);
/* Computing MAX */
		    i__1 = maxwrk, i__2 = *m * 3 + *nrhs * ilaenv_(&c__1, 
			    "SORMBR", "QLT", m, nrhs, n, &c_n1, (ftnlen)6, (
			    ftnlen)3);
		    maxwrk = f2cmax(i__1,i__2);
/* Computing MAX */
		    i__1 = maxwrk, i__2 = *m * 3 + *m * ilaenv_(&c__1, "SORM"
			    "BR", "PLN", n, nrhs, m, &c_n1, (ftnlen)6, (ftnlen)
			    3);
		    maxwrk = f2cmax(i__1,i__2);
/* Computing MAX */
		    i__1 = maxwrk, i__2 = *m * 3 + wlalsd;
		    maxwrk = f2cmax(i__1,i__2);
		}
/* Computing MAX */
		i__1 = *m * 3 + *nrhs, i__2 = *m * 3 + *m, i__1 = f2cmax(i__1,
			i__2), i__2 = *m * 3 + wlalsd;
		minwrk = f2cmax(i__1,i__2);
	    }
	}
	minwrk = f2cmin(minwrk,maxwrk);
	work[1] = (real) maxwrk;
	iwork[1] = liwork;

	if (*lwork < minwrk && ! lquery) {
	    *info = -12;
	}
    }

    if (*info != 0) {
	i__1 = -(*info);
	xerbla_("SGELSD", &i__1, (ftnlen)6);
	return;
    } else if (lquery) {
	return;
    }

/*     Quick return if possible. */

    if (*m == 0 || *n == 0) {
	    fprintf(stdout,"SGELSD quickreturn rank=0\n");
	*rank = 0;
	return;
    }

/*     Get machine parameters. */

    eps = slamch_("P");
    sfmin = slamch_("S");
    smlnum = sfmin / eps;
    bignum = 1.f / smlnum;
//    FILE *bla=fopen("/tmp/bla","w");
//fprintf(bla,"SGELSD eps=%g sfmin=%g smlnum=%g bignum=%g\n",eps,sfmin,smlnum,bignum);
//fclose(bla);
    slabad_(&smlnum, &bignum);

/*     Scale A if f2cmax entry outside range [SMLNUM,BIGNUM]. */

    anrm = slange_("M", m, n, &a[a_offset], lda, &work[1]);
    iascl = 0;
    if (anrm > 0.f && anrm < smlnum) {

/*        Scale matrix norm up to SMLNUM. */
fprintf(stdout,"scaling A up to SML\n");
	slascl_("G", &c__0, &c__0, &anrm, &smlnum, m, n, &a[a_offset], lda, 
		info);
	iascl = 1;
    } else if (anrm > bignum) {

/*        Scale matrix norm down to BIGNUM. */

fprintf(stdout,"scaling A down to BIG\n");
	slascl_("G", &c__0, &c__0, &anrm, &bignum, m, n, &a[a_offset], lda, 
		info);
	iascl = 2;
    } else if (anrm == 0.f) {

/*        Matrix all zero. Return zero solution. */

fprintf(stdout,"A is zero soln\n");
	i__1 = f2cmax(*m,*n);
	slaset_("F", &i__1, nrhs, &c_b81, &c_b81, &b[b_offset], ldb);
	slaset_("F", &minmn, &c__1, &c_b81, &c_b81, &s[1], &c__1);
	*rank = 0;
	goto L10;
    }

/*     Scale B if f2cmax entry outside range [SMLNUM,BIGNUM]. */

    bnrm = slange_("M", m, nrhs, &b[b_offset], ldb, &work[1]);
    ibscl = 0;
    if (bnrm > 0.f && bnrm < smlnum) {

/*        Scale matrix norm up to SMLNUM. */
fprintf(stdout,"scaling B up to SML\n");

	slascl_("G", &c__0, &c__0, &bnrm, &smlnum, m, nrhs, &b[b_offset], ldb,
		 info);
	ibscl = 1;
    } else if (bnrm > bignum) {

/*        Scale matrix norm down to BIGNUM. */
fprintf(stdout,"scaling B down to BIG\n");

	slascl_("G", &c__0, &c__0, &bnrm, &bignum, m, nrhs, &b[b_offset], ldb,
		 info);
	ibscl = 2;
    }

/*     If M < N make sure certain entries of B are zero. */

    if (*m < *n) {
	i__1 = *n - *m;
fprintf(stdout,"zeroing parts of B \n");
	slaset_("F", &i__1, nrhs, &c_b81, &c_b81, &b[*m + 1 + b_dim1], ldb);
    }

/*     Overdetermined case. */

    if (*m >= *n) {
fprintf(stdout,"overdetermined, path 1 \n");

/*        Path 1 - overdetermined or exactly determined. */

	mm = *m;
	if (*m >= mnthr) {

/*           Path 1a - overdetermined, with many more rows than columns. */
fprintf(stdout,"overdetermined, path 1a \n");

	    mm = *n;
	    itau = 1;
	    nwork = itau + *n;

/*           Compute A=Q*R. */
/*           (Workspace: need 2*N, prefer N+N*NB) */

	    i__1 = *lwork - nwork + 1;
	    sgeqrf_(m, n, &a[a_offset], lda, &work[itau], &work[nwork], &i__1,
		     info);

/*           Multiply B by transpose(Q). */
/*           (Workspace: need N+NRHS, prefer N+NRHS*NB) */

	    i__1 = *lwork - nwork + 1;
	    sormqr_("L", "T", m, nrhs, n, &a[a_offset], lda, &work[itau], &b[
		    b_offset], ldb, &work[nwork], &i__1, info);

/*           Zero out below R. */

	    if (*n > 1) {
		i__1 = *n - 1;
		i__2 = *n - 1;
		slaset_("L", &i__1, &i__2, &c_b81, &c_b81, &a[a_dim1 + 2], 
			lda);
	    }
	}

	ie = 1;
	itauq = ie + *n;
	itaup = itauq + *n;
	nwork = itaup + *n;

/*        Bidiagonalize R in A. */
/*        (Workspace: need 3*N+MM, prefer 3*N+(MM+N)*NB) */

	i__1 = *lwork - nwork + 1;
	sgebrd_(&mm, n, &a[a_offset], lda, &s[1], &work[ie], &work[itauq], &
		work[itaup], &work[nwork], &i__1, info);

/*        Multiply B by transpose of left bidiagonalizing vectors of R. */
/*        (Workspace: need 3*N+NRHS, prefer 3*N+NRHS*NB) */

	i__1 = *lwork - nwork + 1;
	sormbr_("Q", "L", "T", &mm, nrhs, n, &a[a_offset], lda, &work[itauq], 
		&b[b_offset], ldb, &work[nwork], &i__1, info);

/*        Solve the bidiagonal least squares problem. */

	slalsd_("U", &smlsiz, n, nrhs, &s[1], &work[ie], &b[b_offset], ldb, 
		rcond, rank, &work[nwork], &iwork[1], info);
	if (*info != 0) {
		fprintf(stdout,"info !=0 nach slalsd\n");
	    goto L10;
	}

/*        Multiply B by right bidiagonalizing vectors of R. */

	i__1 = *lwork - nwork + 1;
	sormbr_("P", "L", "N", n, nrhs, n, &a[a_offset], lda, &work[itaup], &
		b[b_offset], ldb, &work[nwork], &i__1, info);

    } else /* if(complicated condition) */ {
fprintf(stdout,"not overdetermined \n");
/* Computing MAX */
	i__1 = *m, i__2 = (*m << 1) - 4, i__1 = f2cmax(i__1,i__2), i__1 = f2cmax(
		i__1,*nrhs), i__2 = *n - *m * 3, i__1 = f2cmax(i__1,i__2);
	if (*n >= mnthr && *lwork >= (*m << 2) + *m * *m + f2cmax(i__1,wlalsd)) {

/*        Path 2a - underdetermined, with many more columns than rows */
/*        and sufficient workspace for an efficient algorithm. */

fprintf(stdout,"not overdetermined, path 2a\n");

	    ldwork = *m;
/* Computing MAX */
/* Computing MAX */
	    i__3 = *m, i__4 = (*m << 1) - 4, i__3 = f2cmax(i__3,i__4), i__3 = 
		    f2cmax(i__3,*nrhs), i__4 = *n - *m * 3;
	    i__1 = (*m << 2) + *m * *lda + f2cmax(i__3,i__4), i__2 = *m * *lda + 
		    *m + *m * *nrhs, i__1 = f2cmax(i__1,i__2), i__2 = (*m << 2) 
		    + *m * *lda + wlalsd;
	    if (*lwork >= f2cmax(i__1,i__2)) {
		ldwork = *lda;
	    }
	    itau = 1;
	    nwork = *m + 1;

/*        Compute A=L*Q. */
/*        (Workspace: need 2*M, prefer M+M*NB) */

	    i__1 = *lwork - nwork + 1;
	    sgelqf_(m, n, &a[a_offset], lda, &work[itau], &work[nwork], &i__1,
		     info);
	    il = nwork;

/*        Copy L to WORK(IL), zeroing out above its diagonal. */

	    slacpy_("L", m, m, &a[a_offset], lda, &work[il], &ldwork);
	    i__1 = *m - 1;
	    i__2 = *m - 1;
	    slaset_("U", &i__1, &i__2, &c_b81, &c_b81, &work[il + ldwork], &
		    ldwork);
	    ie = il + ldwork * *m;
	    itauq = ie + *m;
	    itaup = itauq + *m;
	    nwork = itaup + *m;

/*        Bidiagonalize L in WORK(IL). */
/*        (Workspace: need M*M+5*M, prefer M*M+4*M+2*M*NB) */

	    i__1 = *lwork - nwork + 1;
	    sgebrd_(m, m, &work[il], &ldwork, &s[1], &work[ie], &work[itauq], 
		    &work[itaup], &work[nwork], &i__1, info);

/*        Multiply B by transpose of left bidiagonalizing vectors of L. */
/*        (Workspace: need M*M+4*M+NRHS, prefer M*M+4*M+NRHS*NB) */

	    i__1 = *lwork - nwork + 1;
	    sormbr_("Q", "L", "T", m, nrhs, m, &work[il], &ldwork, &work[
		    itauq], &b[b_offset], ldb, &work[nwork], &i__1, info);

/*        Solve the bidiagonal least squares problem. */

	    slalsd_("U", &smlsiz, m, nrhs, &s[1], &work[ie], &b[b_offset], 
		    ldb, rcond, rank, &work[nwork], &iwork[1], info);
	    if (*info != 0) {
		goto L10;
	    }

/*        Multiply B by right bidiagonalizing vectors of L. */

	    i__1 = *lwork - nwork + 1;
	    sormbr_("P", "L", "N", m, nrhs, m, &work[il], &ldwork, &work[
		    itaup], &b[b_offset], ldb, &work[nwork], &i__1, info);

/*        Zero out below first M rows of B. */

	    i__1 = *n - *m;
	    slaset_("F", &i__1, nrhs, &c_b81, &c_b81, &b[*m + 1 + b_dim1], 
		    ldb);
	    nwork = itau + *m;

/*        Multiply transpose(Q) by B. */
/*        (Workspace: need M+NRHS, prefer M+NRHS*NB) */

	    i__1 = *lwork - nwork + 1;
	    sormlq_("L", "T", n, nrhs, m, &a[a_offset], lda, &work[itau], &b[
		    b_offset], ldb, &work[nwork], &i__1, info);

	} else {

/*        Path 2 - remaining underdetermined cases. */
fprintf(stdout,"other underdetermined, path 2");

	    ie = 1;
	    itauq = ie + *m;
	    itaup = itauq + *m;
	    nwork = itaup + *m;

/*        Bidiagonalize A. */
/*        (Workspace: need 3*M+N, prefer 3*M+(M+N)*NB) */

	    i__1 = *lwork - nwork + 1;
	    sgebrd_(m, n, &a[a_offset], lda, &s[1], &work[ie], &work[itauq], &
		    work[itaup], &work[nwork], &i__1, info);

/*        Multiply B by transpose of left bidiagonalizing vectors. */
/*        (Workspace: need 3*M+NRHS, prefer 3*M+NRHS*NB) */

	    i__1 = *lwork - nwork + 1;
	    sormbr_("Q", "L", "T", m, nrhs, n, &a[a_offset], lda, &work[itauq]
		    , &b[b_offset], ldb, &work[nwork], &i__1, info);

/*        Solve the bidiagonal least squares problem. */

	    slalsd_("L", &smlsiz, m, nrhs, &s[1], &work[ie], &b[b_offset], 
		    ldb, rcond, rank, &work[nwork], &iwork[1], info);
	    if (*info != 0) {
		goto L10;
	    }

/*        Multiply B by right bidiagonalizing vectors of A. */

	    i__1 = *lwork - nwork + 1;
	    sormbr_("P", "L", "N", n, nrhs, m, &a[a_offset], lda, &work[itaup]
		    , &b[b_offset], ldb, &work[nwork], &i__1, info);

	}
    }

/*     Undo scaling. */

    if (iascl == 1) {
	    fprintf(stdout," unscaling a1\n");
	slascl_("G", &c__0, &c__0, &anrm, &smlnum, n, nrhs, &b[b_offset], ldb,
		 info);
	slascl_("G", &c__0, &c__0, &smlnum, &anrm, &minmn, &c__1, &s[1], &
		minmn, info);
    } else if (iascl == 2) {
	    fprintf(stdout," unscaling a2\n");
	slascl_("G", &c__0, &c__0, &anrm, &bignum, n, nrhs, &b[b_offset], ldb,
		 info);
	slascl_("G", &c__0, &c__0, &bignum, &anrm, &minmn, &c__1, &s[1], &
		minmn, info);
    }
    if (ibscl == 1) {
	    fprintf(stdout," unscaling b1\n");
	slascl_("G", &c__0, &c__0, &smlnum, &bnrm, n, nrhs, &b[b_offset], ldb,
		 info);
    } else if (ibscl == 2) {
	    fprintf(stdout," unscaling b2\n");
	slascl_("G", &c__0, &c__0, &bignum, &bnrm, n, nrhs, &b[b_offset], ldb,
		 info);
    }

L10:
    work[1] = (real) maxwrk;
    iwork[1] = liwork;
fprintf(stdout, "end of SGELSD\n");
    return;

/*     End of SGELSD */

} /* sgelsd_ */

