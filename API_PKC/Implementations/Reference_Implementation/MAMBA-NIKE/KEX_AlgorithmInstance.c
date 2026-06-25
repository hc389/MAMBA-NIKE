/*
MAMBA-NIKE KEX Algorithm Instance
Bridges NewHope-based MAMBA-NIKE to the NGCC KEX API.

Protocol: 1-pass NIKE.
- init_a: generates long-term keypair (pk_a, sk_a)
- init_b: generates long-term keypair (pk_b, sk_b)
- pass1: initiator takes own sk and responder's pk, outputs m1 and shared secret
- derive_ss_a: returns the pre-computed shared secret
- derive_ss_b: responder takes own sk and received m1, outputs shared secret
*/

#include "KEX_AlgorithmInstance.h"
#include "drng.h"
#include <stdint.h>
#include <string.h>

/* Protocol parameters (from ref/params.h, NIKE-128 default) */
#define PARAM_N    1024
#define PARAM_K    2
#define PARAM_Q    65536
#define LOG2Q      16
#define POLY_BYTES        2048
#define SEEDBYTES         32
#define RECBYTES          256
#define SENDA_BYTES       (POLY_BYTES + SEEDBYTES)   /* pk */
#define SENDB_BYTES       (POLY_BYTES + RECBYTES)    /* m1 */
#define SS_BYTES          32
#define STA_BYTES         32

/* ----- Protocol forward declarations (from ref/newhope.h) ----- */
typedef struct { uint16_t coeffs[PARAM_N]; } poly;

void newhope_keygen(unsigned char *send, poly *sk);
void newhope_sharedb(unsigned char *sharedkey, unsigned char *send, const unsigned char *received);
void newhope_shareda(unsigned char *sharedkey, const poly *sk, const unsigned char *received);
void poly_getnoise(poly *r, unsigned char *seed, unsigned char nonce);

/* ----- DRNG-based randombytes (replaces /dev/urandom version) ----- */
extern DRNG_ctx drng_algorithm;

void randombytes(unsigned char *x, unsigned long long xlen)
{
	/* Round up to full bytes */
	unsigned long long bits = xlen * 8;
	get_random_number(&drng_algorithm, x, bits);
}

/* ================================================================
 *  KEX API implementation
 * ================================================================ */

unsigned long long kex_get_passes_num()     { return 1; }
unsigned long long kex_get_pk_len_bytes()   { return SENDA_BYTES; }
unsigned long long kex_get_sk_len_bytes()   { return POLY_BYTES; }
unsigned long long kex_get_sta_len_bytes()  { return STA_BYTES; }
unsigned long long kex_get_stb_len_bytes()  { return STA_BYTES; }
unsigned long long kex_get_ss_len_bytes()   { return SS_BYTES; }
unsigned long long kex_get_total_msg_len_bytes() { return SENDB_BYTES; }

/* ----- Helper: serialize/deserialize poly ----- */
static void poly_to_bytes(unsigned char *out, const poly *p)
{
	for (int i = 0; i < PARAM_N; i++) {
		uint16_t t = p->coeffs[i] & (PARAM_Q - 1);
		out[2*i]   = (unsigned char)(t & 0xFF);
		out[2*i+1] = (unsigned char)((t >> 8) & 0xFF);
	}
}

static void bytes_to_poly(poly *p, const unsigned char *in)
{
	for (int i = 0; i < PARAM_N; i++)
		p->coeffs[i] = ((uint16_t)in[2*i]) | ((uint16_t)in[2*i+1] << 8);
}

/* ----- kex_init_a: generate initiator keypair ----- */
int kex_init_a(
	unsigned char *pka, unsigned long long *pka_len_bytes,
	unsigned char *ska, unsigned long long *ska_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes)
{
	poly sk_poly;
	newhope_keygen(pka, &sk_poly);

	*pka_len_bytes = SENDA_BYTES;
	*ska_len_bytes = POLY_BYTES;
	poly_to_bytes(ska, &sk_poly);

	*sta_len_bytes = 0;  /* no state after init */
	(void)sta;
	return 0;
}

/* ----- kex_init_b: generate responder keypair ----- */
int kex_init_b(
	unsigned char *pkb, unsigned long long *pkb_len_bytes,
	unsigned char *skb, unsigned long long *skb_len_bytes,
	unsigned char *stb, unsigned long long *stb_len_bytes)
{
	poly sk_poly;
	newhope_keygen(pkb, &sk_poly);

	*pkb_len_bytes = SENDA_BYTES;
	*skb_len_bytes = POLY_BYTES;
	poly_to_bytes(skb, &sk_poly);

	*stb_len_bytes = 0;
	(void)stb;
	return 0;
}

/* ----- pass1: initiator generates message and shared secret ----- */
int kex_generate_pass1_msg_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes,
	unsigned char *m1, unsigned long long *m1_len_bytes)
{
	(void)ska;           /* ska not used by sharedb — it generates fresh ephemeral */
	(void)ska_len_bytes;
	(void)pkb_len_bytes;

	unsigned char ss[SS_BYTES];
	newhope_sharedb(ss, m1, pkb);

	*m1_len_bytes = SENDB_BYTES;

	/* Stash shared secret in state */
	memcpy(sta, ss, SS_BYTES);
	*sta_len_bytes = SS_BYTES;

	return 1;  /* protocol finished (1-pass) */
}

/* ----- pass2: not used (1-pass) ----- */
int kex_generate_pass2_msg_b(
	unsigned char *skb, unsigned long long skb_len_bytes,
	unsigned char *pka, unsigned long long pka_len_bytes,
	unsigned char *m1, unsigned long long m1_len_bytes,
	unsigned char *stb, unsigned long long *stb_len_bytes,
	unsigned char *m2, unsigned long long *m2_len_bytes)
{
	(void)skb; (void)skb_len_bytes;
	(void)pka; (void)pka_len_bytes;
	(void)m1; (void)m1_len_bytes;
	(void)stb; (void)stb_len_bytes;
	(void)m2; (void)m2_len_bytes;
	return 0;
}

/* ----- pass3: not used (1-pass) ----- */
int kex_generate_pass3_msg_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *m2, unsigned long long m2_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes,
	unsigned char *m3, unsigned long long *m3_len_bytes)
{
	(void)ska; (void)ska_len_bytes;
	(void)pkb; (void)pkb_len_bytes;
	(void)m2; (void)m2_len_bytes;
	(void)sta; (void)sta_len_bytes;
	(void)m3; (void)m3_len_bytes;
	return 1;
}

/* ----- derive_ss_a: initiator retrieves pre-computed shared secret ----- */
int kex_derive_ss_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *mb, unsigned long long mb_len_bytes,
	unsigned char *sta, unsigned long long sta_len_bytes,
	unsigned char *ssa, unsigned long long *ssa_len_bytes)
{
	(void)ska; (void)ska_len_bytes;
	(void)pkb; (void)pkb_len_bytes;
	(void)mb; (void)mb_len_bytes;

	/* Shared secret was stashed in sta during pass1 */
	memcpy(ssa, sta, SS_BYTES);
	*ssa_len_bytes = SS_BYTES;

	(void)sta_len_bytes;
	return 0;
}

/* ----- derive_ss_b: responder computes shared secret from received m1 ----- */
int kex_derive_ss_b(
	unsigned char *skb, unsigned long long skb_len_bytes,
	unsigned char *pka, unsigned long long pka_len_bytes,
	unsigned char *ma, unsigned long long ma_len_bytes,
	unsigned char *stb, unsigned long long stb_len_bytes,
	unsigned char *ssb, unsigned long long *ssb_len_bytes)
{
	(void)pka; (void)pka_len_bytes;
	(void)stb; (void)stb_len_bytes;

	poly sk_poly;
	if (skb_len_bytes < POLY_BYTES) return -1;
	if (ma_len_bytes < SENDB_BYTES)  return -2;

	bytes_to_poly(&sk_poly, skb);
	newhope_shareda(ssb, &sk_poly, ma);

	*ssb_len_bytes = SS_BYTES;
	return 0;
}
