/*
 * KAT_KEX_nucleo.c — KAT test-vector generation for NUCLEO embedded platform.
 *
 * Adapted from the NGCC KAT_KEX.c framework.
 * Differences from the standard KAT_KEX:
 *   - File I/O replaced by UART (printf) output.
 *   - No "output/" directory created.
 *   - All seed-generated test vectors printed over serial (115200-8N1).
 *   - DRNG is used for both seed generation and algorithm randomness.
 *
 * This file SHALL NOT be modified beyond the UART adaptation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "drng.h"
#include "KEX_AlgorithmInstance.h"

#define SEED_LEN_BYTES 64
#define KAT_KEX_SUCCESS 0
#define KAT_ALGORITHM_INSTANCE_NAME_INVALID -1
#define KAT_KEX_CRYPTO_FAILURE -3
#define KAT_MEMORY_ALLOCATION_FAILED -4
#define KAT_KEX_PASS_ERROR -5
#define KAT_KEX_SS_UNEQUAL -6

static int validate_algorithm_instance_name(const char *algorithm);
static void fprintlen_uart(char *identifier, unsigned long long len);
static void fprintstr_uart(char *identifier, unsigned char *msg, unsigned long long len);

/* DRNG_ctx for generating pseudorandom numbers within the KEX protocol */
DRNG_ctx drng_algorithm;

/*
 * main — Generate 10 KAT vectors and print them over UART.
 */
int main()
{
	unsigned char *nonce;
	DRNG_ctx drng_seed;
	unsigned char *seed, *pka, *ska, *pkb, *skb, *sta, *stb, *ssa, *ssb, *m1, *m2, *m3, *ma, *mb;
	unsigned long long pass, pka_len_bytes, ska_len_bytes, pkb_len_bytes,
		skb_len_bytes, sta_len_bytes, stb_len_bytes, ssa_len_bytes, ssb_len_bytes,
		m1_len_bytes, m2_len_bytes, m3_len_bytes, ma_len_bytes, mb_len_bytes, total_len_bytes;
	int rtn;

	if (validate_algorithm_instance_name(ALGORITHM_INSTANCE))
	{
		printf("ERROR: Invalid algorithm instance name.\n");
		return KAT_ALGORITHM_INSTANCE_NAME_INVALID;
	}

	/* Init drng_seed using nonce */
	nonce = (unsigned char *)calloc(SEED_LEN_BYTES, sizeof(unsigned char));
	if (!nonce) return KAT_MEMORY_ALLOCATION_FAILED;
	for (int i = 0; i < SEED_LEN_BYTES / 4; i++)
		memcpy(nonce + 4 * i, "seed", 4);
	init_random_number(&drng_seed, nonce, SEED_LEN_BYTES);

	printf("\n=== KAT_KEX_%s ===\n\n", ALGORITHM_INSTANCE);

	pass = kex_get_passes_num();
	pka_len_bytes = kex_get_pk_len_bytes();
	ska_len_bytes = kex_get_sk_len_bytes();
	pkb_len_bytes = kex_get_pk_len_bytes();
	skb_len_bytes = kex_get_sk_len_bytes();
	sta_len_bytes = kex_get_sta_len_bytes();
	stb_len_bytes = kex_get_stb_len_bytes();
	ssa_len_bytes = kex_get_ss_len_bytes();
	ssb_len_bytes = kex_get_ss_len_bytes();
	total_len_bytes = kex_get_total_msg_len_bytes();
	m1_len_bytes = 0;
	m2_len_bytes = 0;
	m3_len_bytes = 0;
	ma_len_bytes = 0;
	mb_len_bytes = 0;

	pka = (unsigned char *)calloc(pka_len_bytes, sizeof(unsigned char));
	ska = (unsigned char *)calloc(ska_len_bytes, sizeof(unsigned char));
	pkb = (unsigned char *)calloc(pkb_len_bytes, sizeof(unsigned char));
	skb = (unsigned char *)calloc(skb_len_bytes, sizeof(unsigned char));
	sta = (unsigned char *)calloc(sta_len_bytes, sizeof(unsigned char));
	stb = (unsigned char *)calloc(stb_len_bytes, sizeof(unsigned char));
	ssa = (unsigned char *)calloc(ssa_len_bytes, sizeof(unsigned char));
	ssb = (unsigned char *)calloc(ssb_len_bytes, sizeof(unsigned char));
	m1  = (unsigned char *)calloc(total_len_bytes, sizeof(unsigned char));
	m2  = (unsigned char *)calloc(total_len_bytes, sizeof(unsigned char));
	m3  = (unsigned char *)calloc(total_len_bytes, sizeof(unsigned char));
	ma  = NULL;
	mb  = NULL;
	seed = (unsigned char *)calloc(SEED_LEN_BYTES, sizeof(unsigned char));

	if (!pka || !ska || !pkb || !skb || !sta || !stb || !ssa || !ssb || !m1 || !m2 || !m3 || !seed)
	{
		printf("ERROR: Memory allocation failed.\n");
		return KAT_MEMORY_ALLOCATION_FAILED;
	}

	for (int i = 0; i < 10; i++)
	{
		printf("Count = %d\n", i);
		get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);
		printf("Seed_Len = %d\n", SEED_LEN_BYTES);
		fprintstr_uart("Seed = ", seed, SEED_LEN_BYTES);

		/* Init drng_algorithm using seed */
		init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

		fprintlen_uart("Pass_Num = ", pass);

		/**************** Initialize by the initiator ****************/
		rtn = kex_init_a(pka, &pka_len_bytes, ska, &ska_len_bytes,
		                 sta, &sta_len_bytes);
		if (rtn < KAT_KEX_SUCCESS)
		{
			printf("ERROR: kex_init_a returned %d\n", rtn);
			return KAT_KEX_CRYPTO_FAILURE;
		}
		fprintlen_uart("PKa_Len = ", pka_len_bytes);
		fprintstr_uart("PKa = ", pka, pka_len_bytes);
		fprintlen_uart("SKa_Len = ", ska_len_bytes);
		fprintstr_uart("SKa = ", ska, ska_len_bytes);
		fprintlen_uart("Init_Sta_Len = ", sta_len_bytes);
		fprintstr_uart("Init_Sta = ", sta, sta_len_bytes);

		/**************** Initialize by the responder ****************/
		rtn = kex_init_b(pkb, &pkb_len_bytes, skb, &skb_len_bytes,
		                 stb, &stb_len_bytes);
		if (rtn < KAT_KEX_SUCCESS)
		{
			printf("ERROR: kex_init_b returned %d\n", rtn);
			return KAT_KEX_CRYPTO_FAILURE;
		}
		fprintlen_uart("PKb_Len = ", pkb_len_bytes);
		fprintstr_uart("PKb = ", pkb, pkb_len_bytes);
		fprintlen_uart("SKb_Len = ", skb_len_bytes);
		fprintstr_uart("SKb = ", skb, skb_len_bytes);
		fprintlen_uart("Init_Stb_Len = ", stb_len_bytes);
		fprintstr_uart("Init_Stb = ", stb, stb_len_bytes);

		for (;;)
		{
			/*** First-pass message sent by the initiator ***/
			rtn = kex_generate_pass1_msg_a(
				ska, ska_len_bytes, pkb, pkb_len_bytes,
				sta, &sta_len_bytes, m1, &m1_len_bytes);
			if (rtn < KAT_KEX_SUCCESS)
			{
				printf("ERROR: kex_generate_pass1_msg_a returned %d\n", rtn);
				return KAT_KEX_CRYPTO_FAILURE;
			}
			fprintlen_uart("Pass1_Sta_Len = ", sta_len_bytes);
			fprintstr_uart("Pass1_Sta = ", sta, sta_len_bytes);
			fprintlen_uart("M1_Len = ", m1_len_bytes);
			fprintstr_uart("M1 = ", m1, m1_len_bytes);
			ma = m1;
			ma_len_bytes = m1_len_bytes;
			if (rtn == 1)
			{
				if (pass != 1)
				{
					printf("ERROR: pass count mismatch (expected 1)\n");
					return KAT_KEX_PASS_ERROR;
				}
				break;
			}

			/*** Second-pass message sent by the responder ***/
			rtn = kex_generate_pass2_msg_b(
				skb, skb_len_bytes, pka, pka_len_bytes,
				m1, m1_len_bytes, stb, &stb_len_bytes, m2, &m2_len_bytes);
			if (rtn < KAT_KEX_SUCCESS)
			{
				printf("ERROR: kex_generate_pass2_msg_b returned %d\n", rtn);
				return KAT_KEX_CRYPTO_FAILURE;
			}
			fprintlen_uart("Pass2_Stb_Len = ", stb_len_bytes);
			fprintstr_uart("Pass2_Stb = ", stb, stb_len_bytes);
			fprintlen_uart("M2_Len = ", m2_len_bytes);
			fprintstr_uart("M2 = ", m2, m2_len_bytes);
			mb = m2;
			mb_len_bytes = m2_len_bytes;
			if (rtn == 1)
			{
				if (pass != 2)
				{
					printf("ERROR: pass count mismatch (expected 2)\n");
					return KAT_KEX_PASS_ERROR;
				}
				break;
			}

			/*** Third-pass message sent by the initiator ***/
			rtn = kex_generate_pass3_msg_a(
				ska, ska_len_bytes, pkb, pkb_len_bytes,
				m2, m2_len_bytes, sta, &sta_len_bytes, m3, &m3_len_bytes);
			if (rtn < KAT_KEX_SUCCESS)
			{
				printf("ERROR: kex_generate_pass3_msg_a returned %d\n", rtn);
				return KAT_KEX_CRYPTO_FAILURE;
			}
			fprintlen_uart("Pass3_Sta_Len = ", sta_len_bytes);
			fprintstr_uart("Pass3_Sta = ", sta, sta_len_bytes);
			fprintlen_uart("M3_Len = ", m3_len_bytes);
			fprintstr_uart("M3 = ", m3, m3_len_bytes);
			ma = m3;
			ma_len_bytes = m3_len_bytes;
			if (rtn == 1)
			{
				if (pass != 3)
				{
					printf("ERROR: pass count mismatch (expected 3)\n");
					return KAT_KEX_PASS_ERROR;
				}
				break;
			}
			break;
		}

		rtn = kex_derive_ss_a(ska, ska_len_bytes, pkb, pkb_len_bytes,
		                      mb, mb_len_bytes, sta, sta_len_bytes,
		                      ssa, &ssa_len_bytes);
		if (rtn < KAT_KEX_SUCCESS)
		{
			printf("ERROR: kex_derive_ss_a returned %d\n", rtn);
			return KAT_KEX_CRYPTO_FAILURE;
		}

		rtn = kex_derive_ss_b(skb, skb_len_bytes, pka, pka_len_bytes,
		                      ma, ma_len_bytes, stb, stb_len_bytes,
		                      ssb, &ssb_len_bytes);
		if (rtn < KAT_KEX_SUCCESS)
		{
			printf("ERROR: kex_derive_ss_b returned %d\n", rtn);
			return KAT_KEX_CRYPTO_FAILURE;
		}

		/* Verify two shared secret keys */
		if (ssa_len_bytes != ssb_len_bytes || memcmp(ssa, ssb, ssa_len_bytes))
		{
			printf("ERROR: initiator SS != responder SS\n");
			return KAT_KEX_SS_UNEQUAL;
		}
		fprintlen_uart("SS_Len = ", ssa_len_bytes);
		fprintstr_uart("SS = ", ssa, ssa_len_bytes);
		printf("\n");
	}

	free(seed); free(m1); free(m2); free(m3);
	free(ssa); free(ssb); free(stb); free(sta);
	free(skb); free(pkb); free(ska); free(pka);
	free(nonce);

	printf("\n=== KAT_KEX_%s COMPLETE (10/10) ===\n", ALGORITHM_INSTANCE);
	return KAT_KEX_SUCCESS;
}

/* ----- Validate algorithm instance name ----- */
static int validate_algorithm_instance_name(const char *algorithm)
{
	if (strlen(algorithm) > 64)
		return KAT_ALGORITHM_INSTANCE_NAME_INVALID;
	for (int i = 0; algorithm[i] != '\0'; i++)
	{
		char c = algorithm[i];
		if (!(isalnum(c) || '-' == c || '_' == c))
			return KAT_ALGORITHM_INSTANCE_NAME_INVALID;
	}
	return 0;
}

/* ----- Print a length field to UART ----- */
static void fprintlen_uart(char *identifier, unsigned long long len)
{
	printf("%s%llu\n", identifier, len);
}

/* ----- Print a hex string to UART ----- */
static void fprintstr_uart(char *identifier, unsigned char *msg, unsigned long long len)
{
	printf("%s", identifier);
	for (unsigned long long i = 0; i < len; i++)
		printf("%02X", msg[i]);
	printf("\n");
}
