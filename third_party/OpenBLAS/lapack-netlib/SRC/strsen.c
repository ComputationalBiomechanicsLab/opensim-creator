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
static char junk[] = "\n@(#)LIBF77 VERSION 19990503\n";
#define z_abs(z) (cabs(Cd(z)))
#define z_exp(R, Z) {pCd(R) = cexp(Cd(Z));}
#define z_sqrt(R, Z) {pCd(R) = csqrt(Cd(Z));}
#define myexit_() break;
#define mycycle() continue;
#define myceiling(w) {ceil(w)}
#define myhuge(w) {HUGE_VAL}
//#define mymaxloc_(w,s,e,n) {if (sizeof(*(w)) == sizeof(double)) dmaxloc_((w),*(s),*(e),n); else dmaxloc_((w),*(s),*(e),n);}
#define mymaxloc(w,s,e,n) {dmaxloc_(w,*(s),*(e),n)}

/* procedure parameter types for -A and -C++ */


#ifdef __cplusplus
typedef logical (*L_fp)(...);
#else
typedef logical (*L_fp)();
#endif

static float spow_ui(float x, integer n) {
	float pow=1.0; unsigned long int u;
	if(n != 0) {
		if(n < 0) n = -n, x = 1/x;
		for(u = n; ; ) {
			if(u & 01) pow *= x;
			if(u >>= 1) x *= x;
			else break;
		}
	}
	return pow;
}
static double dpow_ui(double x, integer n) {
	double pow=1.0; unsigned long int u;
	if(n != 0) {
		if(n < 0) n = -n, x = 1/x;
		for(u = n; ; ) {
			if(u & 01) pow *= x;
			if(u >>= 1) x *= x;
			else break;
		}
	}
	return pow;
}
#ifdef _MSC_VER
static _Fcomplex cpow_ui(complex x, integer n) {
	complex pow={1.0,0.0}; unsigned long int u;
		if(n != 0) {
		if(n < 0) n = -n, x.r = 1/x.r, x.i=1/x.i;
		for(u = n; ; ) {
			if(u & 01) pow.r *= x.r, pow.i *= x.i;
			if(u >>= 1) x.r *= x.r, x.i *= x.i;
			else break;
		}
	}
	_Fcomplex p={pow.r, pow.i};
	return p;
}
#else
static _Complex float cpow_ui(_Complex float x, integer n) {
	_Complex float pow=1.0; unsigned long int u;
	if(n != 0) {
		if(n < 0) n = -n, x = 1/x;
		for(u = n; ; ) {
			if(u & 01) pow *= x;
			if(u >>= 1) x *= x;
			else break;
		}
	}
	return pow;
}
#endif
#ifdef _MSC_VER
static _Dcomplex zpow_ui(_Dcomplex x, integer n) {
	_Dcomplex pow={1.0,0.0}; unsigned long int u;
	if(n != 0) {
		if(n < 0) n = -n, x._Val[0] = 1/x._Val[0], x._Val[1] =1/x._Val[1];
		for(u = n; ; ) {
			if(u & 01) pow._Val[0] *= x._Val[0], pow._Val[1] *= x._Val[1];
			if(u >>= 1) x._Val[0] *= x._Val[0], x._Val[1] *= x._Val[1];
			else break;
		}
	}
	_Dcomplex p = {pow._Val[0], pow._Val[1]};
	return p;
}
#else
static _Complex double zpow_ui(_Complex double x, integer n) {
	_Complex double pow=1.0; unsigned long int u;
	if(n != 0) {
		if(n < 0) n = -n, x = 1/x;
		for(u = n; ; ) {
			if(u & 01) pow *= x;
			if(u >>= 1) x *= x;
			else break;
		}
	}
	return pow;
}
#endif
static integer pow_ii(integer x, integer n) {
	integer pow; unsigned long int u;
	if (n <= 0) {
		if (n == 0 || x == 1) pow = 1;
		else if (x != -1) pow = x == 0 ? 1/x : 0;
		else n = -n;
	}
	if ((n > 0) || !(n == 0 || x == 1 || x != -1)) {
		u = n;
		for(pow = 1; ; ) {
			if(u & 01) pow *= x;
			if(u >>= 1) x *= x;
			else break;
		}
	}
	return pow;
}
static integer dmaxloc_(double *w, integer s, integer e, integer *n)
{
	double m; integer i, mi;
	for(m=w[s-1], mi=s, i=s+1; i<=e; i++)
		if (w[i-1]>m) mi=i ,m=w[i-1];
	return mi-s+1;
}
static integer smaxloc_(float *w, integer s, integer e, integer *n)
{
	float m; integer i, mi;
	for(m=w[s-1], mi=s, i=s+1; i<=e; i++)
		if (w[i-1]>m) mi=i ,m=w[i-1];
	return mi-s+1;
}
static inline void cdotc_(complex *z, integer *n_, complex *x, integer *incx_, complex *y, integer *incy_) {
	integer n = *n_, incx = *incx_, incy = *incy_, i;
#ifdef _MSC_VER
	_Fcomplex zdotc = {0.0, 0.0};
	if (incx == 1 && incy == 1) {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc._Val[0] += conjf(Cf(&x[i]))._Val[0] * Cf(&y[i])._Val[0];
			zdotc._Val[1] += conjf(Cf(&x[i]))._Val[1] * Cf(&y[i])._Val[1];
		}
	} else {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc._Val[0] += conjf(Cf(&x[i*incx]))._Val[0] * Cf(&y[i*incy])._Val[0];
			zdotc._Val[1] += conjf(Cf(&x[i*incx]))._Val[1] * Cf(&y[i*incy])._Val[1];
		}
	}
	pCf(z) = zdotc;
}
#else
	_Complex float zdotc = 0.0;
	if (incx == 1 && incy == 1) {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc += conjf(Cf(&x[i])) * Cf(&y[i]);
		}
	} else {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc += conjf(Cf(&x[i*incx])) * Cf(&y[i*incy]);
		}
	}
	pCf(z) = zdotc;
}
#endif
static inline void zdotc_(doublecomplex *z, integer *n_, doublecomplex *x, integer *incx_, doublecomplex *y, integer *incy_) {
	integer n = *n_, incx = *incx_, incy = *incy_, i;
#ifdef _MSC_VER
	_Dcomplex zdotc = {0.0, 0.0};
	if (incx == 1 && incy == 1) {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc._Val[0] += conj(Cd(&x[i]))._Val[0] * Cd(&y[i])._Val[0];
			zdotc._Val[1] += conj(Cd(&x[i]))._Val[1] * Cd(&y[i])._Val[1];
		}
	} else {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc._Val[0] += conj(Cd(&x[i*incx]))._Val[0] * Cd(&y[i*incy])._Val[0];
			zdotc._Val[1] += conj(Cd(&x[i*incx]))._Val[1] * Cd(&y[i*incy])._Val[1];
		}
	}
	pCd(z) = zdotc;
}
#else
	_Complex double zdotc = 0.0;
	if (incx == 1 && incy == 1) {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc += conj(Cd(&x[i])) * Cd(&y[i]);
		}
	} else {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc += conj(Cd(&x[i*incx])) * Cd(&y[i*incy]);
		}
	}
	pCd(z) = zdotc;
}
#endif	
static inline void cdotu_(complex *z, integer *n_, complex *x, integer *incx_, complex *y, integer *incy_) {
	integer n = *n_, incx = *incx_, incy = *incy_, i;
#ifdef _MSC_VER
	_Fcomplex zdotc = {0.0, 0.0};
	if (incx == 1 && incy == 1) {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc._Val[0] += Cf(&x[i])._Val[0] * Cf(&y[i])._Val[0];
			zdotc._Val[1] += Cf(&x[i])._Val[1] * Cf(&y[i])._Val[1];
		}
	} else {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc._Val[0] += Cf(&x[i*incx])._Val[0] * Cf(&y[i*incy])._Val[0];
			zdotc._Val[1] += Cf(&x[i*incx])._Val[1] * Cf(&y[i*incy])._Val[1];
		}
	}
	pCf(z) = zdotc;
}
#else
	_Complex float zdotc = 0.0;
	if (incx == 1 && incy == 1) {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc += Cf(&x[i]) * Cf(&y[i]);
		}
	} else {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc += Cf(&x[i*incx]) * Cf(&y[i*incy]);
		}
	}
	pCf(z) = zdotc;
}
#endif
static inline void zdotu_(doublecomplex *z, integer *n_, doublecomplex *x, integer *incx_, doublecomplex *y, integer *incy_) {
	integer n = *n_, incx = *incx_, incy = *incy_, i;
#ifdef _MSC_VER
	_Dcomplex zdotc = {0.0, 0.0};
	if (incx == 1 && incy == 1) {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc._Val[0] += Cd(&x[i])._Val[0] * Cd(&y[i])._Val[0];
			zdotc._Val[1] += Cd(&x[i])._Val[1] * Cd(&y[i])._Val[1];
		}
	} else {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc._Val[0] += Cd(&x[i*incx])._Val[0] * Cd(&y[i*incy])._Val[0];
			zdotc._Val[1] += Cd(&x[i*incx])._Val[1] * Cd(&y[i*incy])._Val[1];
		}
	}
	pCd(z) = zdotc;
}
#else
	_Complex double zdotc = 0.0;
	if (incx == 1 && incy == 1) {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc += Cd(&x[i]) * Cd(&y[i]);
		}
	} else {
		for (i=0;i<n;i++) { /* zdotc = zdotc + dconjg(x(i))* y(i) */
			zdotc += Cd(&x[i*incx]) * Cd(&y[i*incy]);
		}
	}
	pCd(z) = zdotc;
}
#endif
/*  -- translated by f2c (version 20000121).
   You must link the resulting object file with the libraries:
	-lf2c -lm   (in that order)
*/




/* Table of constant values */

static integer c_n1 = -1;

/* > \brief \b STRSEN */

/*  =========== DOCUMENTATION =========== */

/* Online html documentation available at */
/*            http://www.netlib.org/lapack/explore-html/ */

/* > \htmlonly */
/* > Download STRSEN + dependencies */
/* > <a href="http://www.netlib.org/cgi-bin/netlibfiles.tgz?format=tgz&filename=/lapack/lapack_routine/strsen.
f"> */
/* > [TGZ]</a> */
/* > <a href="http://www.netlib.org/cgi-bin/netlibfiles.zip?format=zip&filename=/lapack/lapack_routine/strsen.
f"> */
/* > [ZIP]</a> */
/* > <a href="http://www.netlib.org/cgi-bin/netlibfiles.txt?format=txt&filename=/lapack/lapack_routine/strsen.
f"> */
/* > [TXT]</a> */
/* > \endhtmlonly */

/*  Definition: */
/*  =========== */

/*       SUBROUTINE STRSEN( JOB, COMPQ, SELECT, N, T, LDT, Q, LDQ, WR, WI, */
/*                          M, S, SEP, WORK, LWORK, IWORK, LIWORK, INFO ) */

/*       CHARACTER          COMPQ, JOB */
/*       INTEGER            INFO, LDQ, LDT, LIWORK, LWORK, M, N */
/*       REAL               S, SEP */
/*       LOGICAL            SELECT( * ) */
/*       INTEGER            IWORK( * ) */
/*       REAL               Q( LDQ, * ), T( LDT, * ), WI( * ), WORK( * ), */
/*      $                   WR( * ) */


/* > \par Purpose: */
/*  ============= */
/* > */
/* > \verbatim */
/* > */
/* > STRSEN reorders the real Schur factorization of a real matrix */
/* > A = Q*T*Q**T, so that a selected cluster of eigenvalues appears in */
/* > the leading diagonal blocks of the upper quasi-triangular matrix T, */
/* > and the leading columns of Q form an orthonormal basis of the */
/* > corresponding right invariant subspace. */
/* > */
/* > Optionally the routine computes the reciprocal condition numbers of */
/* > the cluster of eigenvalues and/or the invariant subspace. */
/* > */
/* > T must be in Schur canonical form (as returned by SHSEQR), that is, */
/* > block upper triangular with 1-by-1 and 2-by-2 diagonal blocks; each */
/* > 2-by-2 diagonal block has its diagonal elements equal and its */
/* > off-diagonal elements of opposite sign. */
/* > \endverbatim */

/*  Arguments: */
/*  ========== */

/* > \param[in] JOB */
/* > \verbatim */
/* >          JOB is CHARACTER*1 */
/* >          Specifies whether condition numbers are required for the */
/* >          cluster of eigenvalues (S) or the invariant subspace (SEP): */
/* >          = 'N': none; */
/* >          = 'E': for eigenvalues only (S); */
/* >          = 'V': for invariant subspace only (SEP); */
/* >          = 'B': for both eigenvalues and invariant subspace (S and */
/* >                 SEP). */
/* > \endverbatim */
/* > */
/* > \param[in] COMPQ */
/* > \verbatim */
/* >          COMPQ is CHARACTER*1 */
/* >          = 'V': update the matrix Q of Schur vectors; */
/* >          = 'N': do not update Q. */
/* > \endverbatim */
/* > */
/* > \param[in] SELECT */
/* > \verbatim */
/* >          SELECT is LOGICAL array, dimension (N) */
/* >          SELECT specifies the eigenvalues in the selected cluster. To */
/* >          select a real eigenvalue w(j), SELECT(j) must be set to */
/* >          .TRUE.. To select a complex conjugate pair of eigenvalues */
/* >          w(j) and w(j+1), corresponding to a 2-by-2 diagonal block, */
/* >          either SELECT(j) or SELECT(j+1) or both must be set to */
/* >          .TRUE.; a complex conjugate pair of eigenvalues must be */
/* >          either both included in the cluster or both excluded. */
/* > \endverbatim */
/* > */
/* > \param[in] N */
/* > \verbatim */
/* >          N is INTEGER */
/* >          The order of the matrix T. N >= 0. */
/* > \endverbatim */
/* > */
/* > \param[in,out] T */
/* > \verbatim */
/* >          T is REAL array, dimension (LDT,N) */
/* >          On entry, the upper quasi-triangular matrix T, in Schur */
/* >          canonical form. */
/* >          On exit, T is overwritten by the reordered matrix T, again in */
/* >          Schur canonical form, with the selected eigenvalues in the */
/* >          leading diagonal blocks. */
/* > \endverbatim */
/* > */
/* > \param[in] LDT */
/* > \verbatim */
/* >          LDT is INTEGER */
/* >          The leading dimension of the array T. LDT >= f2cmax(1,N). */
/* > \endverbatim */
/* > */
/* > \param[in,out] Q */
/* > \verbatim */
/* >          Q is REAL array, dimension (LDQ,N) */
/* >          On entry, if COMPQ = 'V', the matrix Q of Schur vectors. */
/* >          On exit, if COMPQ = 'V', Q has been postmultiplied by the */
/* >          orthogonal transformation matrix which reorders T; the */
/* >          leading M columns of Q form an orthonormal basis for the */
/* >          specified invariant subspace. */
/* >          If COMPQ = 'N', Q is not referenced. */
/* > \endverbatim */
/* > */
/* > \param[in] LDQ */
/* > \verbatim */
/* >          LDQ is INTEGER */
/* >          The leading dimension of the array Q. */
/* >          LDQ >= 1; and if COMPQ = 'V', LDQ >= N. */
/* > \endverbatim */
/* > */
/* > \param[out] WR */
/* > \verbatim */
/* >          WR is REAL array, dimension (N) */
/* > \endverbatim */
/* > */
/* > \param[out] WI */
/* > \verbatim */
/* >          WI is REAL array, dimension (N) */
/* > */
/* >          The real and imaginary parts, respectively, of the reordered */
/* >          eigenvalues of T. The eigenvalues are stored in the same */
/* >          order as on the diagonal of T, with WR(i) = T(i,i) and, if */
/* >          T(i:i+1,i:i+1) is a 2-by-2 diagonal block, WI(i) > 0 and */
/* >          WI(i+1) = -WI(i). Note that if a complex eigenvalue is */
/* >          sufficiently ill-conditioned, then its value may differ */
/* >          significantly from its value before reordering. */
/* > \endverbatim */
/* > */
/* > \param[out] M */
/* > \verbatim */
/* >          M is INTEGER */
/* >          The dimension of the specified invariant subspace. */
/* >          0 < = M <= N. */
/* > \endverbatim */
/* > */
/* > \param[out] S */
/* > \verbatim */
/* >          S is REAL */
/* >          If JOB = 'E' or 'B', S is a lower bound on the reciprocal */
/* >          condition number for the selected cluster of eigenvalues. */
/* >          S cannot underestimate the true reciprocal condition number */
/* >          by more than a factor of sqrt(N). If M = 0 or N, S = 1. */
/* >          If JOB = 'N' or 'V', S is not referenced. */
/* > \endverbatim */
/* > */
/* > \param[out] SEP */
/* > \verbatim */
/* >          SEP is REAL */
/* >          If JOB = 'V' or 'B', SEP is the estimated reciprocal */
/* >          condition number of the specified invariant subspace. If */
/* >          M = 0 or N, SEP = norm(T). */
/* >          If JOB = 'N' or 'E', SEP is not referenced. */
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
/* >          The dimension of the array WORK. */
/* >          If JOB = 'N', LWORK >= f2cmax(1,N); */
/* >          if JOB = 'E', LWORK >= f2cmax(1,M*(N-M)); */
/* >          if JOB = 'V' or 'B', LWORK >= f2cmax(1,2*M*(N-M)). */
/* > */
/* >          If LWORK = -1, then a workspace query is assumed; the routine */
/* >          only calculates the optimal size of the WORK array, returns */
/* >          this value as the first entry of the WORK array, and no error */
/* >          message related to LWORK is issued by XERBLA. */
/* > \endverbatim */
/* > */
/* > \param[out] IWORK */
/* > \verbatim */
/* >          IWORK is INTEGER array, dimension (MAX(1,LIWORK)) */
/* >          On exit, if INFO = 0, IWORK(1) returns the optimal LIWORK. */
/* > \endverbatim */
/* > */
/* > \param[in] LIWORK */
/* > \verbatim */
/* >          LIWORK is INTEGER */
/* >          The dimension of the array IWORK. */
/* >          If JOB = 'N' or 'E', LIWORK >= 1; */
/* >          if JOB = 'V' or 'B', LIWORK >= f2cmax(1,M*(N-M)). */
/* > */
/* >          If LIWORK = -1, then a workspace query is assumed; the */
/* >          routine only calculates the optimal size of the IWORK array, */
/* >          returns this value as the first entry of the IWORK array, and */
/* >          no error message related to LIWORK is issued by XERBLA. */
/* > \endverbatim */
/* > */
/* > \param[out] INFO */
/* > \verbatim */
/* >          INFO is INTEGER */
/* >          = 0: successful exit */
/* >          < 0: if INFO = -i, the i-th argument had an illegal value */
/* >          = 1: reordering of T failed because some eigenvalues are too */
/* >               close to separate (the problem is very ill-conditioned); */
/* >               T may have been partially reordered, and WR and WI */
/* >               contain the eigenvalues in the same order as in T; S and */
/* >               SEP (if requested) are set to zero. */
/* > \endverbatim */

/*  Authors: */
/*  ======== */

/* > \author Univ. of Tennessee */
/* > \author Univ. of California Berkeley */
/* > \author Univ. of Colorado Denver */
/* > \author NAG Ltd. */

/* > \date April 2012 */

/* > \ingroup realOTHERcomputational */

/* > \par Further Details: */
/*  ===================== */
/* > */
/* > \verbatim */
/* > */
/* >  STRSEN first collects the selected eigenvalues by computing an */
/* >  orthogonal transformation Z to move them to the top left corner of T. */
/* >  In other words, the selected eigenvalues are the eigenvalues of T11 */
/* >  in: */
/* > */
/* >          Z**T * T * Z = ( T11 T12 ) n1 */
/* >                         (  0  T22 ) n2 */
/* >                            n1  n2 */
/* > */
/* >  where N = n1+n2 and Z**T means the transpose of Z. The first n1 columns */
/* >  of Z span the specified invariant subspace of T. */
/* > */
/* >  If T has been obtained from the real Schur factorization of a matrix */
/* >  A = Q*T*Q**T, then the reordered real Schur factorization of A is given */
/* >  by A = (Q*Z)*(Z**T*T*Z)*(Q*Z)**T, and the first n1 columns of Q*Z span */
/* >  the corresponding invariant subspace of A. */
/* > */
/* >  The reciprocal condition number of the average of the eigenvalues of */
/* >  T11 may be returned in S. S lies between 0 (very badly conditioned) */
/* >  and 1 (very well conditioned). It is computed as follows. First we */
/* >  compute R so that */
/* > */
/* >                         P = ( I  R ) n1 */
/* >                             ( 0  0 ) n2 */
/* >                               n1 n2 */
/* > */
/* >  is the projector on the invariant subspace associated with T11. */
/* >  R is the solution of the Sylvester equation: */
/* > */
/* >                        T11*R - R*T22 = T12. */
/* > */
/* >  Let F-norm(M) denote the Frobenius-norm of M and 2-norm(M) denote */
/* >  the two-norm of M. Then S is computed as the lower bound */
/* > */
/* >                      (1 + F-norm(R)**2)**(-1/2) */
/* > */
/* >  on the reciprocal of 2-norm(P), the true reciprocal condition number. */
/* >  S cannot underestimate 1 / 2-norm(P) by more than a factor of */
/* >  sqrt(N). */
/* > */
/* >  An approximate error bound for the computed average of the */
/* >  eigenvalues of T11 is */
/* > */
/* >                         EPS * norm(T) / S */
/* > */
/* >  where EPS is the machine precision. */
/* > */
/* >  The reciprocal condition number of the right invariant subspace */
/* >  spanned by the first n1 columns of Z (or of Q*Z) is returned in SEP. */
/* >  SEP is defined as the separation of T11 and T22: */
/* > */
/* >                     sep( T11, T22 ) = sigma-f2cmin( C ) */
/* > */
/* >  where sigma-f2cmin(C) is the smallest singular value of the */
/* >  n1*n2-by-n1*n2 matrix */
/* > */
/* >     C  = kprod( I(n2), T11 ) - kprod( transpose(T22), I(n1) ) */
/* > */
/* >  I(m) is an m by m identity matrix, and kprod denotes the Kronecker */
/* >  product. We estimate sigma-f2cmin(C) by the reciprocal of an estimate of */
/* >  the 1-norm of inverse(C). The true reciprocal 1-norm of inverse(C) */
/* >  cannot differ from sigma-f2cmin(C) by more than a factor of sqrt(n1*n2). */
/* > */
/* >  When SEP is small, small changes in T can cause large changes in */
/* >  the invariant subspace. An approximate bound on the maximum angular */
/* >  error in the computed right invariant subspace is */
/* > */
/* >                      EPS * norm(T) / SEP */
/* > \endverbatim */
/* > */
/*  ===================================================================== */
/* Subroutine */ void strsen_(char *job, char *compq, logical *select, integer 
	*n, real *t, integer *ldt, real *q, integer *ldq, real *wr, real *wi, 
	integer *m, real *s, real *sep, real *work, integer *lwork, integer *
	iwork, integer *liwork, integer *info)
{
    /* System generated locals */
    integer q_dim1, q_offset, t_dim1, t_offset, i__1, i__2;
    real r__1, r__2;

    /* Local variables */
    integer kase;
    logical pair;
    integer ierr;
    logical swap;
    integer k;
    real scale;
    extern logical lsame_(char *, char *);
    integer isave[3], lwmin;
    logical wantq, wants;
    real rnorm;
    integer n1, n2;
    extern /* Subroutine */ void slacn2_(integer *, real *, real *, integer *, 
	    real *, integer *, integer *);
    integer kk, nn, ks;
    extern real slange_(char *, integer *, integer *, real *, integer *, real 
	    *);
    extern /* Subroutine */ int xerbla_(char *, integer *, ftnlen);
    logical wantbh;
    extern /* Subroutine */ void slacpy_(char *, integer *, integer *, real *, 
	    integer *, real *, integer *);
    integer liwmin;
    extern /* Subroutine */ void strexc_(char *, integer *, real *, integer *, 
	    real *, integer *, integer *, integer *, real *, integer *);
    logical wantsp, lquery;
    extern /* Subroutine */ void strsyl_(char *, char *, integer *, integer *, 
	    integer *, real *, integer *, real *, integer *, real *, integer *
	    , real *, integer *);
    real est;


/*  -- LAPACK computational routine (version 3.7.0) -- */
/*  -- LAPACK is a software package provided by Univ. of Tennessee,    -- */
/*  -- Univ. of California Berkeley, Univ. of Colorado Denver and NAG Ltd..-- */
/*     April 2012 */


/*  ===================================================================== */


/*     Decode and test the input parameters */

    /* Parameter adjustments */
    --select;
    t_dim1 = *ldt;
    t_offset = 1 + t_dim1 * 1;
    t -= t_offset;
    q_dim1 = *ldq;
    q_offset = 1 + q_dim1 * 1;
    q -= q_offset;
    --wr;
    --wi;
    --work;
    --iwork;

    /* Function Body */
    wantbh = lsame_(job, "B");
    wants = lsame_(job, "E") || wantbh;
    wantsp = lsame_(job, "V") || wantbh;
    wantq = lsame_(compq, "V");

    *info = 0;
    lquery = *lwork == -1;
    if (! lsame_(job, "N") && ! wants && ! wantsp) {
	*info = -1;
    } else if (! lsame_(compq, "N") && ! wantq) {
	*info = -2;
    } else if (*n < 0) {
	*info = -4;
    } else if (*ldt < f2cmax(1,*n)) {
	*info = -6;
    } else if (*ldq < 1 || wantq && *ldq < *n) {
	*info = -8;
    } else {

/*        Set M to the dimension of the specified invariant subspace, */
/*        and test LWORK and LIWORK. */

	*m = 0;
	pair = FALSE_;
	i__1 = *n;
	for (k = 1; k <= i__1; ++k) {
	    if (pair) {
		pair = FALSE_;
	    } else {
		if (k < *n) {
		    if (t[k + 1 + k * t_dim1] == 0.f) {
			if (select[k]) {
			    ++(*m);
			}
		    } else {
			pair = TRUE_;
			if (select[k] || select[k + 1]) {
			    *m += 2;
			}
		    }
		} else {
		    if (select[*n]) {
			++(*m);
		    }
		}
	    }
/* L10: */
	}

	n1 = *m;
	n2 = *n - *m;
	nn = n1 * n2;

	if (wantsp) {
/* Computing MAX */
	    i__1 = 1, i__2 = nn << 1;
	    lwmin = f2cmax(i__1,i__2);
	    liwmin = f2cmax(1,nn);
	} else if (lsame_(job, "N")) {
	    lwmin = f2cmax(1,*n);
	    liwmin = 1;
	} else if (lsame_(job, "E")) {
	    lwmin = f2cmax(1,nn);
	    liwmin = 1;
	}

	if (*lwork < lwmin && ! lquery) {
	    *info = -15;
	} else if (*liwork < liwmin && ! lquery) {
	    *info = -17;
	}
    }

    if (*info == 0) {
	work[1] = (real) lwmin;
	iwork[1] = liwmin;
    }

    if (*info != 0) {
	i__1 = -(*info);
	xerbla_("STRSEN", &i__1, (ftnlen)6);
	return;
    } else if (lquery) {
	return;
    }

/*     Quick return if possible. */

    if (*m == *n || *m == 0) {
	if (wants) {
	    *s = 1.f;
	}
	if (wantsp) {
	    *sep = slange_("1", n, n, &t[t_offset], ldt, &work[1]);
	}
	goto L40;
    }

/*     Collect the selected blocks at the top-left corner of T. */

    ks = 0;
    pair = FALSE_;
    i__1 = *n;
    for (k = 1; k <= i__1; ++k) {
	if (pair) {
	    pair = FALSE_;
	} else {
	    swap = select[k];
	    if (k < *n) {
		if (t[k + 1 + k * t_dim1] != 0.f) {
		    pair = TRUE_;
		    swap = swap || select[k + 1];
		}
	    }
	    if (swap) {
		++ks;

/*              Swap the K-th block to position KS. */

		ierr = 0;
		kk = k;
		if (k != ks) {
		    strexc_(compq, n, &t[t_offset], ldt, &q[q_offset], ldq, &
			    kk, &ks, &work[1], &ierr);
		}
		if (ierr == 1 || ierr == 2) {

/*                 Blocks too close to swap: exit. */

		    *info = 1;
		    if (wants) {
			*s = 0.f;
		    }
		    if (wantsp) {
			*sep = 0.f;
		    }
		    goto L40;
		}
		if (pair) {
		    ++ks;
		}
	    }
	}
/* L20: */
    }

    if (wants) {

/*        Solve Sylvester equation for R: */

/*           T11*R - R*T22 = scale*T12 */

	slacpy_("F", &n1, &n2, &t[(n1 + 1) * t_dim1 + 1], ldt, &work[1], &n1);
	strsyl_("N", "N", &c_n1, &n1, &n2, &t[t_offset], ldt, &t[n1 + 1 + (n1 
		+ 1) * t_dim1], ldt, &work[1], &n1, &scale, &ierr);

/*        Estimate the reciprocal of the condition number of the cluster */
/*        of eigenvalues. */

	rnorm = slange_("F", &n1, &n2, &work[1], &n1, &work[1]);
	if (rnorm == 0.f) {
	    *s = 1.f;
	} else {
	    *s = scale / (sqrt(scale * scale / rnorm + rnorm) * sqrt(rnorm));
	}
    }

    if (wantsp) {

/*        Estimate sep(T11,T22). */

	est = 0.f;
	kase = 0;
L30:
	slacn2_(&nn, &work[nn + 1], &work[1], &iwork[1], &est, &kase, isave);
	if (kase != 0) {
	    if (kase == 1) {

/*              Solve  T11*R - R*T22 = scale*X. */

		strsyl_("N", "N", &c_n1, &n1, &n2, &t[t_offset], ldt, &t[n1 + 
			1 + (n1 + 1) * t_dim1], ldt, &work[1], &n1, &scale, &
			ierr);
	    } else {

/*              Solve T11**T*R - R*T22**T = scale*X. */

		strsyl_("T", "T", &c_n1, &n1, &n2, &t[t_offset], ldt, &t[n1 + 
			1 + (n1 + 1) * t_dim1], ldt, &work[1], &n1, &scale, &
			ierr);
	    }
	    goto L30;
	}

	*sep = scale / est;
    }

L40:

/*     Store the output eigenvalues in WR and WI. */

    i__1 = *n;
    for (k = 1; k <= i__1; ++k) {
	wr[k] = t[k + k * t_dim1];
	wi[k] = 0.f;
/* L50: */
    }
    i__1 = *n - 1;
    for (k = 1; k <= i__1; ++k) {
	if (t[k + 1 + k * t_dim1] != 0.f) {
	    wi[k] = sqrt((r__1 = t[k + (k + 1) * t_dim1], abs(r__1))) * sqrt((
		    r__2 = t[k + 1 + k * t_dim1], abs(r__2)));
	    wi[k + 1] = -wi[k];
	}
/* L60: */
    }

    work[1] = (real) lwmin;
    iwork[1] = liwmin;

    return;

/*     End of STRSEN */

} /* strsen_ */

