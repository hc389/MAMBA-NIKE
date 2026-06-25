/*
 * toom.c — Toom-Cook-4 polynomial multiplication over Z
 *
 * Reference: M. Bodrato, A. Zanoni, "Towards Optimal Toom-Cook
 *   Multiplication for Univariate and Multivariate Polynomials"
 */

#include "toom.h"
#include <stdlib.h>
#include <string.h>

#define TOOM4_CUTOFF 48

/* ------------------------------------------------------------------ */
/*  Helper: element-wise add/sub/shr for int64_t vectors              */
/* ------------------------------------------------------------------ */

static void i64_add(int64_t *r, const int64_t *a, const int64_t *b, int n)
{ for (int i = 0; i < n; i++) r[i] = a[i] + b[i]; }
static void i64_sub(int64_t *r, const int64_t *a, const int64_t *b, int n)
{ for (int i = 0; i < n; i++) r[i] = a[i] - b[i]; }
static void i64_subfrom(int64_t *r, const int64_t *a, int n)
{ for (int i = 0; i < n; i++) r[i] -= a[i]; }
static void i64_shr(int64_t *r, int n, int shift)
{ for (int i = 0; i < n; i++) r[i] >>= shift; }
static void i64_mul_scalar(int64_t *r, int n, int64_t s)
{ for (int i = 0; i < n; i++) r[i] *= s; }
static void i64_copy(int64_t *r, const int64_t *a, int n)
{ memcpy(r, a, (size_t)n * sizeof(int64_t)); }

/* ------------------------------------------------------------------ */
/*  Schoolbook multiplication (base case)                             */
/* ------------------------------------------------------------------ */

static void schoolbook_mul(int64_t *r,
                           const int64_t *a, const int64_t *b, int n)
{
  memset(r, 0, (size_t)(2 * n - 1) * sizeof(int64_t));
  for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++)
      r[i + j] += a[i] * b[j];
}

/* ------------------------------------------------------------------ */
/*  Toom-Cook-4 recursive multiplication  (r = a * b over Z)          */
/* ------------------------------------------------------------------ */

void toom4_mul(int64_t *r, const int64_t *a, const int64_t *b, int n)
{
  /* --- base case --- */
  if (n <= TOOM4_CUTOFF) {
    schoolbook_mul(r, a, b, n);
    return;
  }

  int k = (n + 3) / 4;
  int lp = 2 * k - 1;

  /* --- extract 4 slices --- */
  const int64_t *a0 = a, *a1 = n > k ? a + k : NULL;
  const int64_t *a2 = n > 2 * k ? a + 2 * k : NULL;
  const int64_t *a3 = n > 3 * k ? a + 3 * k : NULL;
  int l0 = n < k ? n : k;
  int l1 = (n < 2*k) ? (n > k ? n - k : 0) : k;
  int l2 = (n < 3*k) ? (n > 2*k ? n - 2*k : 0) : k;
  int l3 = n > 3*k ? n - 3*k : 0;

  const int64_t *b0 = b, *b1 = n > k ? b + k : NULL;
  const int64_t *b2 = n > 2*k ? b + 2*k : NULL;
  const int64_t *b3 = n > 3*k ? b + 3*k : NULL;

  /* --- allocate memory --- */
  int64_t *av[7], *bv[7], *p[7], *C[7];
  for (int i = 0; i < 7; i++) {
    av[i] = (int64_t *)calloc((size_t)k, sizeof(int64_t));
    bv[i] = (int64_t *)calloc((size_t)k, sizeof(int64_t));
    p[i]  = (int64_t *)calloc((size_t)lp, sizeof(int64_t));
    C[i]  = (int64_t *)calloc((size_t)lp, sizeof(int64_t));
  }
  int64_t *t1 = (int64_t *)calloc((size_t)lp, sizeof(int64_t));
  int64_t *t2 = (int64_t *)calloc((size_t)lp, sizeof(int64_t));
  int64_t *t3 = (int64_t *)calloc((size_t)lp, sizeof(int64_t));
  int64_t *t4 = (int64_t *)calloc((size_t)lp, sizeof(int64_t));
  int64_t *fh = (int64_t *)calloc((size_t)lp, sizeof(int64_t));
  int64_t *rd = (int64_t *)calloc((size_t)lp, sizeof(int64_t));
  int64_t *re = (int64_t *)calloc((size_t)lp, sizeof(int64_t));
  int64_t *tt = (int64_t *)calloc((size_t)lp, sizeof(int64_t));

  /* --- Step 1: Evaluate at 7 points --- */
  for (int i = 0; i < k; i++) {
    int64_t a0v = i < l0 ? a0[i] : 0;
    int64_t a1v = i < l1 ? a1[i] : 0;
    int64_t a2v = i < l2 ? a2[i] : 0;
    int64_t a3v = i < l3 ? a3[i] : 0;
    int64_t b0v = i < l0 ? b0[i] : 0;
    int64_t b1v = i < l1 ? b1[i] : 0;
    int64_t b2v = i < l2 ? b2[i] : 0;
    int64_t b3v = i < l3 ? b3[i] : 0;

    av[0][i] = a0v;                                     /* w=0   */
    av[1][i] = a3v;                                     /* w=∞   */
    av[2][i] = a0v + a1v + a2v + a3v;                  /* w=1   */
    av[3][i] = a0v - a1v + a2v - a3v;                  /* w=-1  */
    av[4][i] = a0v + 2*a1v + 4*a2v + 8*a3v;           /* w=2   */
    av[5][i] = a0v - 2*a1v + 4*a2v - 8*a3v;           /* w=-2  */
    av[6][i] = 8*a0v + 4*a1v + 2*a2v + a3v;           /* w=1/2 */

    bv[0][i] = b0v;
    bv[1][i] = b3v;
    bv[2][i] = b0v + b1v + b2v + b3v;
    bv[3][i] = b0v - b1v + b2v - b3v;
    bv[4][i] = b0v + 2*b1v + 4*b2v + 8*b3v;
    bv[5][i] = b0v - 2*b1v + 4*b2v - 8*b3v;
    bv[6][i] = 8*b0v + 4*b1v + 2*b2v + b3v;
  }

  /* --- Step 2: Recursively multiply 7 pairs --- */
  for (int i = 0; i < 7; i++)
    toom4_mul(p[i], av[i], bv[i], k);

  /* --- Step 3: Interpolation --- */
  i64_copy(C[0], p[0], lp);
  i64_copy(C[6], p[1], lp);

  /* even1/odd1 from w=1,-1 */
  i64_add(t1, p[2], p[3], lp);  i64_shr(t1, lp, 1);   /* even1 */
  i64_sub(t2, p[2], p[3], lp);  i64_shr(t2, lp, 1);   /* odd1  */

  /* even2/odd2 from w=2,-2 */
  i64_add(t3, p[4], p[5], lp);  i64_shr(t3, lp, 1);   /* even2 */
  i64_sub(t4, p[4], p[5], lp);  i64_shr(t4, lp, 2);   /* odd2  */

  /* g1 = even1 - C0 - C6   (= C2 + C4) */
  int64_t *g1 = t1;  i64_subfrom(g1, C[0], lp);  i64_subfrom(g1, C[6], lp);
  /* h1 = odd1            (= C1 + C3 + C5) */
  int64_t *h1 = t2;
  /* g2 = even2 - C0 - 64*C6  (= 4*C2 + 16*C4) */
  int64_t *g2 = t3;  i64_subfrom(g2, C[0], lp);
  i64_copy(tt, C[6], lp);  i64_mul_scalar(tt, lp, 64);  i64_subfrom(g2, tt, lp);
  /* h2 = odd2            (= C1 + 4*C3 + 16*C5) */
  int64_t *h2 = t4;

  /* C4 = (g2 - 4*g1) / 12 */
  i64_copy(C[4], g2, lp);
  i64_copy(tt, g1, lp);  i64_mul_scalar(tt, lp, 4);  i64_subfrom(C[4], tt, lp);
  for (int i = 0; i < lp; i++) C[4][i] /= 12;

  /* C2 = g1 - C4 */
  i64_copy(C[2], g1, lp);  i64_subfrom(C[2], C[4], lp);

  /* fh = ph - 64*C0 - C6  (= 32*C1 + 16*C2 + 8*C3 + 4*C4 + 2*C5) */
  i64_copy(fh, p[6], lp);
  i64_copy(tt, C[0], lp);  i64_mul_scalar(tt, lp, 64);  i64_subfrom(fh, tt, lp);
  i64_subfrom(fh, C[6], lp);

  /* rhsD = 16*h1 - h2 */
  i64_copy(rd, h1, lp);  i64_mul_scalar(rd, lp, 16);  i64_subfrom(rd, h2, lp);

  /* rhsE = fh - 16*C2 - 4*C4 - 2*h1 */
  i64_copy(re, fh, lp);
  i64_copy(tt, C[2], lp);  i64_mul_scalar(tt, lp, 16);  i64_subfrom(re, tt, lp);
  i64_copy(tt, C[4], lp);  i64_mul_scalar(tt, lp,  4);  i64_subfrom(re, tt, lp);
  i64_copy(tt, h1, lp);    i64_mul_scalar(tt, lp,  2);  i64_subfrom(re, tt, lp);

  /* C3 = (2*rhsD - rhsE) / 18 */
  i64_copy(C[3], rd, lp);  i64_mul_scalar(C[3], lp, 2);  i64_subfrom(C[3], re, lp);
  for (int i = 0; i < lp; i++) C[3][i] /= 18;

  /* C1 = (rhsD - 12*C3) / 15 */
  i64_copy(C[1], rd, lp);
  i64_copy(tt, C[3], lp);  i64_mul_scalar(tt, lp, 12);  i64_subfrom(C[1], tt, lp);
  for (int i = 0; i < lp; i++) C[1][i] /= 15;

  /* C5 = h1 - C1 - C3 */
  i64_copy(C[5], h1, lp);  i64_subfrom(C[5], C[1], lp);  i64_subfrom(C[5], C[3], lp);

  /* --- Step 4: Assemble the full product --- */
  int rlen = 2 * n - 1;
  memset(r, 0, (size_t)rlen * sizeof(int64_t));
  for (int i = 0; i < lp; i++) {
    if (i           < rlen) r[i]          += C[0][i];
    if (i + k       < rlen) r[i + k]      += C[1][i];
    if (i + 2 * k   < rlen) r[i + 2 * k]  += C[2][i];
    if (i + 3 * k   < rlen) r[i + 3 * k]  += C[3][i];
    if (i + 4 * k   < rlen) r[i + 4 * k]  += C[4][i];
    if (i + 5 * k   < rlen) r[i + 5 * k]  += C[5][i];
    if (i + 6 * k   < rlen) r[i + 6 * k]  += C[6][i];
  }

  /* --- free memory --- */
  for (int i = 0; i < 7; i++) {
    free(av[i]); free(bv[i]); free(p[i]); free(C[i]);
  }
  free(t1); free(t2); free(t3); free(t4);
  free(fh); free(rd); free(re); free(tt);
}
