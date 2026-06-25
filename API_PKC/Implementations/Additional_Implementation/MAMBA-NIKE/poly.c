#include "poly.h"
#include "reduce.h"
#include "randombytes.h"
#include "fips202.h"
#include "crypto_stream_chacha20.h"
#include "toom.h"
#include <stdlib.h>

/* ================================================================
 * Serialization: each coefficient is exactly 16 bits (2 bytes)
 * For q = 2^16, no bit-packing needed.
 * ================================================================ */

void poly_frombytes(poly *r, const unsigned char *a)
{
  int i;
  for(i = 0; i < PARAM_N; i++)
    r->coeffs[i] = ((uint16_t)a[2*i]) | ((uint16_t)a[2*i+1] << 8);
}

void poly_tobytes(unsigned char *r, const poly *p)
{
  int i;
  uint16_t t;
  for(i = 0; i < PARAM_N; i++)
  {
    t = p->coeffs[i] & (PARAM_Q - 1);
    r[2*i]   = t & 0xFF;
    r[2*i+1] = (t >> 8) & 0xFF;
  }
}

/* ================================================================
 * Uniform sampling: with q=2^16 all 16-bit values are < q.
 * No rejection sampling needed.
 * ================================================================ */

void poly_uniform(poly *a, const unsigned char *seed)
{
  unsigned int pos = 0, ctr = 0;
  uint16_t val;
  uint64_t state[25];
  unsigned int nblocks = 16;
  uint8_t buf[SHAKE128_RATE * nblocks];

  shake128_absorb(state, seed, NEWHOPE_SEEDBYTES);
  shake128_squeezeblocks((unsigned char *)buf, nblocks, state);

  while(ctr < PARAM_N)
  {
    val = buf[pos] | ((uint16_t)buf[pos+1] << 8);
    if(val < PARAM_Q)
      a->coeffs[ctr++] = val;
    pos += 2;
    if(pos > SHAKE128_RATE * nblocks - 2)
    {
      nblocks = 1;
      shake128_squeezeblocks((unsigned char *)buf, nblocks, state);
      pos = 0;
    }
  }
}

/* ================================================================
 * Noise sampling: centered binomial Binomial(2*PARAM_K) - PARAM_K
 * For PARAM_K = 2: sum of 4 random bits - 2, range [-2, 2]
 * ================================================================ */

void poly_getnoise(poly *r, unsigned char *seed, unsigned char nonce)
{
  unsigned char buf[PARAM_N];
  uint32_t t, d;
  unsigned char n[8];
  int i, j;

  for(i = 1; i < 8; i++)
    n[i] = 0;
  n[0] = nonce;

  crypto_stream_chacha20(buf, PARAM_N, n, seed);

  for(i = 0; i < PARAM_N; i++)
  {
    t = buf[i];
    d = 0;
    for(j = 0; j < 2 * PARAM_K; j++)
      d += (t >> j) & 1;

    /* d ∈ [0, 2k]; centered: (d - k) mod q */
    r->coeffs[i] = ((uint32_t)(d + PARAM_Q - PARAM_K)) & (PARAM_Q - 1);
  }
}

/* ================================================================
 * Negacyclic convolution in Z_q[x]/(x^n+1)
 * Uses Toom-Cook-4 for the full product, then folds modulo x^n+1.
 * ================================================================ */

void poly_convolution(poly *r, const poly *a, const poly *b)
{
  int i;
  int n = PARAM_N;
  int rlen = 2 * n - 1;
  int64_t *aa = (int64_t *)malloc((size_t)n * sizeof(int64_t));
  int64_t *bb = (int64_t *)malloc((size_t)n * sizeof(int64_t));
  int64_t *prod = (int64_t *)malloc((size_t)rlen * sizeof(int64_t));

  /* Convert uint16_t -> int64_t */
  for(i = 0; i < n; i++) { aa[i] = a->coeffs[i]; bb[i] = b->coeffs[i]; }

  /* Full product over Z via Toom-Cook-4 */
  toom4_mul(prod, aa, bb, n);

  /* Fold into Z_q[x]/(x^n+1): r[i] = prod[i] - prod[i+n] for i=0..n-1 */
  for(i = 0; i < n; i++) {
    int64_t val = prod[i];
    if (i + n < rlen)
      val -= prod[i + n];
    r->coeffs[i] = (uint16_t)(val & (PARAM_Q - 1));
  }

  free(aa); free(bb); free(prod);
}

/* ================================================================
 * Pointwise multiply — aliased to convolution (no NTT domain)
 * ================================================================ */

void poly_pointwise(poly *r, const poly *a, const poly *b)
{
  poly_convolution(r, a, b);
}

/* ================================================================
 * Addition: r = a + b mod q
 * ================================================================ */

void poly_add(poly *r, const poly *a, const poly *b)
{
  int i;
  for(i = 0; i < PARAM_N; i++)
    r->coeffs[i] = (uint16_t)(((uint32_t)a->coeffs[i] + (uint32_t)b->coeffs[i]) & (PARAM_Q - 1));
}

/* ================================================================
 * NTT wrappers — no-ops (polynomials stay in time domain)
 * ================================================================ */

void poly_ntt(poly *r)
{
  (void)r;
}

void poly_invntt(poly *r)
{
  (void)r;
}
