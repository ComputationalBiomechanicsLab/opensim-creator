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
#define z_div(c, a, b) {Cd(c)._Val[0] = (Cd(a)._Val[0]/Cd(b)._Val[0]); Cd(c)._Val[1]=(Cd(a)._Val[1]/Cd(b)._Val[1]);}
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
#define pow_dd(ap, bp) ( pow(*(ap), *(bp)))
#define pow_si(B,E) spow_ui(*(B),*(E))
#define pow_ri(B,E) spow_ui(*(B),*(E))
#define pow_di(B,E) dpow_ui(*(B),*(E))
#define pow_zi(p, a, b) {pCd(p) = zpow_ui(Cd(a), *(b));}
#define pow_ci(p, a, b) {pCf(p) = cpow_ui(Cf(a), *(b));}
#define pow_zz(R,A,B) {pCd(R) = cpow(Cd(A),*(B));}
#define s_cat(lpp, rpp, rnp, np, llp) { 	ftnlen i, nc, ll; char *f__rp, *lp; 	ll = (llp); lp = (lpp); 	for(i=0; i < (int)*(np); ++i) {         	nc = ll; 	        if((rnp)[i] < nc) nc = (rnp)[i]; 	        ll -= nc;         	f__rp = (rpp)[i]; 	        while(--nc >= 0) *lp++ = *(f__rp)++;         } 	while(--ll >= 0) *lp++ = ' '; }
#define s_cmp(a,b,c,d) ((integer)strncmp((a),(b),f2cmin((c),(d))))
#define s_copy(A,B,C,D) { int __i,__m; for (__i=0, __m=f2cmin((C),(D)); __i<__m && (B)[__i] != 0; ++__i) (A)[__i] = (B)[__i]; }
#define sig_die(s, kill) { exit(1); }
#define s_stop(s, n) {exit(0);}
#define z_abs(z) (cabs(Cd(z)))
#define z_exp(R, Z) {pCd(R) = cexp(Cd(Z));}
#define z_sqrt(R, Z) {pCd(R) = csqrt(Cd(Z));}
#define myexit_() break;
#define mycycle_() continue;
#define myceiling_(w) {ceil(w)}
#define myhuge_(w) {HUGE_VAL}
//#define mymaxloc_(w,s,e,n) {if (sizeof(*(w)) == sizeof(double)) dmaxloc_((w),*(s),*(e),n); else dmaxloc_((w),*(s),*(e),n);}
#define mymaxloc_(w,s,e,n) {dmaxloc_(w,*(s),*(e),n)}

/* procedure parameter types for -A and -C++ */




/* Table of constant values */

static integer c__2 = 2;
static doublecomplex c_b6 = {0.,0.};

/* > \brief \b ZLAHILB */

/*  =========== DOCUMENTATION =========== */

/* Online html documentation available at */
/*            http://www.netlib.org/lapack/explore-html/ */

/*  Definition: */
/*  =========== */

/*       SUBROUTINE ZLAHILB( N, NRHS, A, LDA, X, LDX, B, LDB, WORK, */
/*            INFO, PATH) */

/*       INTEGER N, NRHS, LDA, LDX, LDB, INFO */
/*       DOUBLE PRECISION WORK(N) */
/*       COMPLEX*16 A(LDA,N), X(LDX, NRHS), B(LDB, NRHS) */
/*       CHARACTER*3 PATH */


/* > \par Purpose: */
/*  ============= */
/* > */
/* > \verbatim */
/* > */
/* > ZLAHILB generates an N by N scaled Hilbert matrix in A along with */
/* > NRHS right-hand sides in B and solutions in X such that A*X=B. */
/* > */
/* > The Hilbert matrix is scaled by M = LCM(1, 2, ..., 2*N-1) so that all */
/* > entries are integers.  The right-hand sides are the first NRHS */
/* > columns of M * the identity matrix, and the solutions are the */
/* > first NRHS columns of the inverse Hilbert matrix. */
/* > */
/* > The condition number of the Hilbert matrix grows exponentially with */
/* > its size, roughly as O(e ** (3.5*N)).  Additionally, the inverse */
/* > Hilbert matrices beyond a relatively small dimension cannot be */
/* > generated exactly without extra precision.  Precision is exhausted */
/* > when the largest entry in the inverse Hilbert matrix is greater than */
/* > 2 to the power of the number of bits in the fraction of the data type */
/* > used plus one, which is 24 for single precision. */
/* > */
/* > In single, the generated solution is exact for N <= 6 and has */
/* > small componentwise error for 7 <= N <= 11. */
/* > \endverbatim */

/*  Arguments: */
/*  ========== */

/* > \param[in] N */
/* > \verbatim */
/* >          N is INTEGER */
/* >          The dimension of the matrix A. */
/* > \endverbatim */
/* > */
/* > \param[in] NRHS */
/* > \verbatim */
/* >          NRHS is INTEGER */
/* >          The requested number of right-hand sides. */
/* > \endverbatim */
/* > */
/* > \param[out] A */
/* > \verbatim */
/* >          A is COMPLEX array, dimension (LDA, N) */
/* >          The generated scaled Hilbert matrix. */
/* > \endverbatim */
/* > */
/* > \param[in] LDA */
/* > \verbatim */
/* >          LDA is INTEGER */
/* >          The leading dimension of the array A.  LDA >= N. */
/* > \endverbatim */
/* > */
/* > \param[out] X */
/* > \verbatim */
/* >          X is COMPLEX array, dimension (LDX, NRHS) */
/* >          The generated exact solutions.  Currently, the first NRHS */
/* >          columns of the inverse Hilbert matrix. */
/* > \endverbatim */
/* > */
/* > \param[in] LDX */
/* > \verbatim */
/* >          LDX is INTEGER */
/* >          The leading dimension of the array X.  LDX >= N. */
/* > \endverbatim */
/* > */
/* > \param[out] B */
/* > \verbatim */
/* >          B is REAL array, dimension (LDB, NRHS) */
/* >          The generated right-hand sides.  Currently, the first NRHS */
/* >          columns of LCM(1, 2, ..., 2*N-1) * the identity matrix. */
/* > \endverbatim */
/* > */
/* > \param[in] LDB */
/* > \verbatim */
/* >          LDB is INTEGER */
/* >          The leading dimension of the array B.  LDB >= N. */
/* > \endverbatim */
/* > */
/* > \param[out] WORK */
/* > \verbatim */
/* >          WORK is REAL array, dimension (N) */
/* > \endverbatim */
/* > */
/* > \param[out] INFO */
/* > \verbatim */
/* >          INFO is INTEGER */
/* >          = 0: successful exit */
/* >          = 1: N is too large; the data is still generated but may not */
/* >               be not exact. */
/* >          < 0: if INFO = -i, the i-th argument had an illegal value */
/* > \endverbatim */
/* > */
/* > \param[in] PATH */
/* > \verbatim */
/* >          PATH is CHARACTER*3 */
/* >          The LAPACK path name. */
/* > \endverbatim */

/*  Authors: */
/*  ======== */

/* > \author Univ. of Tennessee */
/* > \author Univ. of California Berkeley */
/* > \author Univ. of Colorado Denver */
/* > \author NAG Ltd. */

/* > \date November 2017 */

/* > \ingroup complex16_matgen */

/*  ===================================================================== */
/* Subroutine */ void zlahilb_(integer *n, integer *nrhs, doublecomplex *a, 
	integer *lda, doublecomplex *x, integer *ldx, doublecomplex *b, 
	integer *ldb, doublereal *work, integer *info, char *path)
{
    /* Initialized data */

    static doublecomplex d1[8] = { {-1.,0.},{0.,1.},{-1.,-1.},{0.,-1.},{1.,0.}
	    ,{-1.,1.},{1.,1.},{1.,-1.} };
    static doublecomplex d2[8] = { {-1.,0.},{0.,-1.},{-1.,1.},{0.,1.},{1.,0.},
	    {-1.,-1.},{1.,-1.},{1.,1.} };
    static doublecomplex invd1[8] = { {-1.,0.},{0.,-1.},{-.5,.5},{0.,1.},{1.,
	    0.},{-.5,-.5},{.5,-.5},{.5,.5} };
    static doublecomplex invd2[8] = { {-1.,0.},{0.,1.},{-.5,-.5},{0.,-1.},{1.,
	    0.},{-.5,.5},{.5,.5},{.5,-.5} };

    /* System generated locals */
    integer a_dim1, a_offset, x_dim1, x_offset, b_dim1, b_offset, i__1, i__2, 
	    i__3, i__4, i__5;
    doublereal d__1;
    doublecomplex z__1, z__2;

    /* Local variables */
    integer i__, j, m, r__;
    char c2[2];
    integer ti, tm;
    extern /* Subroutine */ int xerbla_(char *, integer *, ftnlen);
    extern logical lsamen_(integer *, char *, char *);
    extern /* Subroutine */ void zlaset_(char *, integer *, integer *, 
	    doublecomplex *, doublecomplex *, doublecomplex *, integer *);
    doublecomplex tmp;


/*  -- LAPACK test routine (version 3.8.0) -- */
/*  -- LAPACK is a software package provided by Univ. of Tennessee,    -- */
/*  -- Univ. of California Berkeley, Univ. of Colorado Denver and NAG Ltd..-- */
/*     November 2017 */


/*  ===================================================================== */
/*     NMAX_EXACT   the largest dimension where the generated data is */
/*                  exact. */
/*     NMAX_APPROX  the largest dimension where the generated data has */
/*                  a small componentwise relative error. */
/*     ??? complex uses how many bits ??? */

/*     d's are generated from random permutation of those eight elements. */
    /* Parameter adjustments */
    --work;
    a_dim1 = *lda;
    a_offset = 1 + a_dim1 * 1;
    a -= a_offset;
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1 * 1;
    x -= x_offset;
    b_dim1 = *ldb;
    b_offset = 1 + b_dim1 * 1;
    b -= b_offset;

    /* Function Body */
    s_copy(c2, path + 1, (ftnlen)2, (ftnlen)2);

/*     Test the input arguments */

    *info = 0;
    if (*n < 0 || *n > 11) {
	*info = -1;
    } else if (*nrhs < 0) {
	*info = -2;
    } else if (*lda < *n) {
	*info = -4;
    } else if (*ldx < *n) {
	*info = -6;
    } else if (*ldb < *n) {
	*info = -8;
    }
    if (*info < 0) {
	i__1 = -(*info);
	xerbla_("ZLAHILB", &i__1, 7);
	return;
    }
    if (*n > 6) {
	*info = 1;
    }

/*     Compute M = the LCM of the integers [1, 2*N-1].  The largest */
/*     reasonable N is small enough that integers suffice (up to N = 11). */
    m = 1;
    i__1 = (*n << 1) - 1;
    for (i__ = 2; i__ <= i__1; ++i__) {
	tm = m;
	ti = i__;
	r__ = tm % ti;
	while(r__ != 0) {
	    tm = ti;
	    ti = r__;
	    r__ = tm % ti;
	}
	m = m / ti * i__;
    }

/*     Generate the scaled Hilbert matrix in A */
/*     If we are testing SY routines, */
/*        take D1_i = D2_i, else, D1_i = D2_i* */
    if (lsamen_(&c__2, c2, "SY")) {
	i__1 = *n;
	for (j = 1; j <= i__1; ++j) {
	    i__2 = *n;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		i__3 = i__ + j * a_dim1;
		i__4 = j % 8;
		d__1 = (doublereal) m / (i__ + j - 1);
		z__2.r = d__1 * d1[i__4].r, z__2.i = d__1 * d1[i__4].i;
		i__5 = i__ % 8;
		z__1.r = z__2.r * d1[i__5].r - z__2.i * d1[i__5].i, z__1.i = 
			z__2.r * d1[i__5].i + z__2.i * d1[i__5].r;
		a[i__3].r = z__1.r, a[i__3].i = z__1.i;
	    }
	}
    } else {
	i__1 = *n;
	for (j = 1; j <= i__1; ++j) {
	    i__2 = *n;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		i__3 = i__ + j * a_dim1;
		i__4 = j % 8;
		d__1 = (doublereal) m / (i__ + j - 1);
		z__2.r = d__1 * d1[i__4].r, z__2.i = d__1 * d1[i__4].i;
		i__5 = i__ % 8;
		z__1.r = z__2.r * d2[i__5].r - z__2.i * d2[i__5].i, z__1.i = 
			z__2.r * d2[i__5].i + z__2.i * d2[i__5].r;
		a[i__3].r = z__1.r, a[i__3].i = z__1.i;
	    }
	}
    }

/*     Generate matrix B as simply the first NRHS columns of M * the */
/*     identity. */
    d__1 = (doublereal) m;
    tmp.r = d__1, tmp.i = 0.;
    zlaset_("Full", n, nrhs, &c_b6, &tmp, &b[b_offset], ldb);

/*     Generate the true solutions in X.  Because B = the first NRHS */
/*     columns of M*I, the true solutions are just the first NRHS columns */
/*     of the inverse Hilbert matrix. */
    work[1] = (doublereal) (*n);
    i__1 = *n;
    for (j = 2; j <= i__1; ++j) {
	work[j] = work[j - 1] / (j - 1) * (j - 1 - *n) / (j - 1) * (*n + j - 
		1);
    }
/*     If we are testing SY routines, */
/*           take D1_i = D2_i, else, D1_i = D2_i* */
    if (lsamen_(&c__2, c2, "SY")) {
	i__1 = *nrhs;
	for (j = 1; j <= i__1; ++j) {
	    i__2 = *n;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		i__3 = i__ + j * x_dim1;
		i__4 = j % 8;
		d__1 = work[i__] * work[j] / (i__ + j - 1);
		z__2.r = d__1 * invd1[i__4].r, z__2.i = d__1 * invd1[i__4].i;
		i__5 = i__ % 8;
		z__1.r = z__2.r * invd1[i__5].r - z__2.i * invd1[i__5].i, 
			z__1.i = z__2.r * invd1[i__5].i + z__2.i * invd1[i__5]
			.r;
		x[i__3].r = z__1.r, x[i__3].i = z__1.i;
	    }
	}
    } else {
	i__1 = *nrhs;
	for (j = 1; j <= i__1; ++j) {
	    i__2 = *n;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		i__3 = i__ + j * x_dim1;
		i__4 = j % 8;
		d__1 = work[i__] * work[j] / (i__ + j - 1);
		z__2.r = d__1 * invd2[i__4].r, z__2.i = d__1 * invd2[i__4].i;
		i__5 = i__ % 8;
		z__1.r = z__2.r * invd1[i__5].r - z__2.i * invd1[i__5].i, 
			z__1.i = z__2.r * invd1[i__5].i + z__2.i * invd1[i__5]
			.r;
		x[i__3].r = z__1.r, x[i__3].i = z__1.i;
	    }
	}
    }
    return;
} /* zlahilb_ */

