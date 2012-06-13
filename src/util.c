#include <R.h>
#include <Rdefines.h>
#include <R_ext/Complex.h>
#include <time.h>

// ceeboo 2012/3+4
//

SEXP _part_index(SEXP x) {
    if (!inherits(x, "factor"))
	error("'x' not a factor");
    int k;
    SEXP r, t;

    k = LENGTH(getAttrib(x, R_LevelsSymbol));

    r = PROTECT(allocVector(INTSXP, LENGTH(x)));
    setAttrib(r, install("table"), (t = allocVector(INTSXP, k)));

    memset(INTEGER(t), 0, sizeof(int) * k);

    for (int i = 0; i < LENGTH(x); i++) {
	k = INTEGER(x)[i];
	if (k == NA_INTEGER)
	    INTEGER(r)[i] = k;
	else {
	    k--;
	    INTEGER(t)[k]++;
	    INTEGER(r)[i] = INTEGER(t)[k];
	}
    }

    UNPROTECT(1);
    return r;
}

SEXP _vector_index(SEXP d, SEXP x) {
    if (TYPEOF(d) != INTSXP ||
	TYPEOF(x) != INTSXP)
	error("'d, x' not integer");
    int n, m;
    SEXP r, dd;

    if (!isMatrix(x))
	error("'x' not a matrix");

    r = getAttrib(x, R_DimSymbol);
    n = INTEGER(r)[0];
    m = INTEGER(r)[1];
    if (m != LENGTH(d))
	error("'x' and 'd' do not conform");

    r = PROTECT(allocVector(INTSXP, n));

    if (m > 2) {
	dd = PROTECT(duplicate(d));
	for (int i = 1; i < m - 1; i++)
	    INTEGER(dd)[i] *= INTEGER(dd)[i-1];
    } else
	dd = d;

    for (int i = 0; i < n; i++) {
	int k = i;
	int l = INTEGER(x)[i];
	if (l != NA_INTEGER) {
	    if (l < 1 || l > INTEGER(d)[0])
		error("'x' invalid");
	    for (int j = 1; j < m; j++) {
		k += n;
		int ll = INTEGER(x)[k];
		if (ll == NA_INTEGER) {
		    l = ll;
		    break;
		}
		if (ll < 1 || ll > INTEGER(d)[j])
		    error("'x' invalid");
		l += INTEGER(dd)[j - 1] * (ll - 1);
	    }
	}
	INTEGER(r)[i] = l;
    }

    UNPROTECT(1 + (m > 2));
    return r;
}

SEXP _ini_array(SEXP d, SEXP p, SEXP v, SEXP s) {
    if (TYPEOF(d) != INTSXP ||
	TYPEOF(p) != INTSXP ||
	TYPEOF(s) != INTSXP)
	error("'d, p, s' not integer");
    int n, m;
    SEXP r, dd;

    if (!isVector(v))
	error("'v' not a vector");
    if (isMatrix(p)) {
	r = getAttrib(p, R_DimSymbol);
	n = INTEGER(r)[0];
	if (n != LENGTH(v))
	    error("'p' and 'v' do not conform");
	m = INTEGER(r)[1];
	if (m != LENGTH(d))
	    error("'p' and 'd' do not conform");

	r = PROTECT(allocArray(TYPEOF(v), d));
    } else {
	n = LENGTH(p);
	if (n != LENGTH(v))
	    error("'p' and 'v' do not conform");
	m = 1;
	if (m != LENGTH(d))
	    error("'p' and 'd' do not conform");

	r = PROTECT(allocVector(TYPEOF(v), INTEGER(d)[0]));
    }
    switch(TYPEOF(v)) {
	case LGLSXP:
	case INTSXP:
	    memset(INTEGER(r), 0, sizeof(int) * LENGTH(r));
	    break;
	case REALSXP:
	    memset(REAL(r), 0, sizeof(double) * LENGTH(r));
	    break;
	case RAWSXP:
	    memset(RAW(r), 0, sizeof(char) * LENGTH(r));
	    break;
	case CPLXSXP:
	    memset(COMPLEX(r), 0, sizeof(Rcomplex) * LENGTH(r));
	    break;
	case EXPRSXP:
	case VECSXP:
	    for (int i = 0; i < LENGTH(r); i++)
		SET_VECTOR_ELT(r, i, R_NilValue);
	    break;
	case STRSXP:
	    for (int i = 0; i < LENGTH(r); i++)
		SET_STRING_ELT(r, i, R_BlankString);
	    break;
	default:
	    error("type of 'v' not supported");
    }

    if (m > 2) {
	dd = PROTECT(duplicate(d));
	for (int i = 1; i < m - 1; i++)
	    INTEGER(dd)[i] *= INTEGER(dd)[i-1];
    } else
	dd = d;

    for (int i = 0; i < LENGTH(s); i++) {
	int k = INTEGER(s)[i];
	if (k < 1 || k > n)
	    error("'s' invalid");
	k--;
	int h = k;
	int l = INTEGER(p)[k];
	if (l < 1 || l > INTEGER(d)[0])
	    error("'p' invalid");
	l--;
	for (int j = 1; j < m; j++) {
	    k += n;
	    int ll = INTEGER(p)[k];
	    if (ll < 1 || ll > INTEGER(d)[j])
		error("'p' invalid");
	    ll--;
	    l += INTEGER(dd)[j - 1] * ll;
	}
	switch(TYPEOF(v)) {
	    case LGLSXP:
	    case INTSXP:
		INTEGER(r)[l] = INTEGER(v)[h];
		break;
	    case REALSXP:
		REAL(r)[l] = REAL(v)[h];
		break;
	    case RAWSXP:
		RAW(r)[l] = RAW(v)[h];
		break;
	    case CPLXSXP:
		COMPLEX(r)[l] = COMPLEX(v)[h];
		break;
	    case EXPRSXP:
	    case VECSXP:
		SET_VECTOR_ELT(r, l, VECTOR_ELT(v, h));
		break;
	    case STRSXP:
		SET_STRING_ELT(r, l, STRING_ELT(v, h));
		break;
	    default:
		error("type of 'v' not supported");
	}

    }

    UNPROTECT(1 + (m > 2));
    return r;
}

// FIXME other storage types
SEXP _split_row(SEXP x) {
    if (TYPEOF(x) != INTSXP)
	error("'x' not integer");
    int n, m;
    SEXP r;

    if (!isMatrix(x))
	error("'x' not a matrix");
    r = getAttrib(x, R_DimSymbol);
    n = INTEGER(r)[0];
    m = INTEGER(r)[1];

    r = PROTECT(allocVector(VECSXP, n));

    for (int i = 0; i < n; i++) {
	SEXP s;
	SET_VECTOR_ELT(r, i, (s = allocVector(INTSXP, m)));
	for (int j = 0, k = i; j < m; j++, k += n)
	    INTEGER(s)[j] = INTEGER(x)[k];
    }

    UNPROTECT(1);
    return r;
}

SEXP _split_col(SEXP x) {
    if (TYPEOF(x) != INTSXP)
	error("'x' not integer");
    int n, m;
    SEXP r;

    if (!isMatrix(x))
	error("'x' not a matrix");
    r = getAttrib(x, R_DimSymbol);
    n = INTEGER(r)[0];
    m = INTEGER(r)[1];

    r = PROTECT(allocVector(VECSXP, m));

    int k = 0;
    for (int i = 0; i < m; i++) {
	SEXP s;
	SET_VECTOR_ELT(r, i, (s = allocVector(INTSXP, n)));
	for (int j = 0; j < n; j++, k++)
	    INTEGER(s)[j] = INTEGER(x)[k];
    }

    UNPROTECT(1);
    return r;
}


// See src/main/unique.c in the R source code.

// Compare integer.
static int _ieq(int *x, int *y, int i, int l) {
    while (l-- > 0) {
	if (*x != *y)
	    return 0;
	x += i;
	y += i;
    }
    return 1;
}

// Hash function for integer.
static int _ihash(int *x, int i, int l, int k) {
    unsigned int j = l * 100;

    k = 32 - k;
    while (l-- > 0) {
	j ^= 3141592653U * (unsigned int) *x >> k;
	j *= 97;
	x += i;
    }
   return 3141592653U * j >> k; 
}

// Add index to hash table for integer.
static int _ihadd(int *x, int nr, int nc, int i, SEXP h, int k) {
    int *s, j;

    s = x + i;
    k = _ihash(s, nr, nc, k);
    while ((j = INTEGER(h)[k]) > -1) {
	if (_ieq(x + j, s, nr, nc))
	    return j;
	k = (k + 1) % LENGTH(h);
    }
    INTEGER(h)[k] = i;

    return -1;
}

SEXP _match_matrix(SEXP x) {
    if (TYPEOF(x) != INTSXP)
	error("'x' not integer");
    int nr, nc;
    SEXP r;

    if (!isMatrix(x))
	error("'x' not a matrix");
    r = getAttrib(x, R_DimSymbol);

    nr = INTEGER(r)[0];
    nc = INTEGER(r)[1];

    // Initialize hash table.
    int hk, k, n;
    SEXP ht;

    if (nr > 1073741824)
	error("size %d too large for hashing", nr);
    k  = 2 * nr;
    n  = 2;
    hk = 1;
    while (k > n) {
	n  *= 2;
	hk += 1;
    }
    ht = PROTECT(allocVector(INTSXP, n));
    for (k = 0; k < n; k++)
	INTEGER(ht)[k] = -1;

    // Match.
    SEXP s;
    r = PROTECT(allocVector(VECSXP, 2));
    SET_VECTOR_ELT(r, 0, (s = allocVector(INTSXP, nr)));

    n = 0;
    for (k = 0; k < nr; k++) {
	int j = _ihadd(INTEGER(x), nr, nc, k, ht, hk);
	if (j > -1)
	    INTEGER(s)[k] = INTEGER(s)[j];
	else {
	    n++;
	    INTEGER(s)[k] = n;
	}
    }
    UNPROTECT_PTR(ht);

    SEXP t;
    SET_VECTOR_ELT(r, 1, (t = allocVector(INTSXP, n)));

    n = 1;
    for (k = 0; k < nr; k++)
	if (INTEGER(s)[k] == n) {
	    INTEGER(t)[n - 1] = k + 1;
	    n++;
	}

    UNPROTECT(1);
    return r;
}
