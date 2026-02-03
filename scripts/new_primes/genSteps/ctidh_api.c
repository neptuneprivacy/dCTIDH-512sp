/*
 * CTIDH API: keygen, derive, and helpers (no main).
 * Used by ctidh.c (minimal test) and ctidh_demo.c (verbose test like main/ctidh.c).
 */
#include <string.h>
#include "fp.h"
#include "primes.h"
#include "csidh.h"
#include "ctidh.h"

#define N primes_num

void internal_keygen(fp *pk, int8_t *sk)
{
    csidh_private((private_key *)sk);
    action((public_key *)*pk, &base, (private_key *)sk);
}

bool internal_derive(fp *ss, fp *const pk, int8_t *const sk)
{
    if (!validate((public_key *)pk))
        return 0;
    action((public_key *)ss, (public_key *)*pk, (private_key *)sk);
    return 1;
}

#define SECSIDH_SUCCESS 0
#define SECSIDH_FAILURE -1

static inline void secsidh_clear(void *b, size_t s)
{
    memset(b, 0, s);
}

static inline void secsidh_sk2oct(uint8_t *buf, const int8_t sk[N])
{
    memcpy(buf, sk, N * sizeof(int8_t));
}

static inline void secsidh_pk2oct(uint8_t *buf, const fp pk[1])
{
    memcpy(buf, pk, sizeof(fp));
}

static inline void secsidh_ss2oct(uint8_t *buf, const fp ss[1])
{
    memcpy(buf, ss, sizeof(fp));
}

static inline void secsidh_oct2pk(fp pk[1], const uint8_t *buf)
{
    memcpy(pk, buf, sizeof(fp));
}

static inline void secsidh_oct2sk(int8_t sk[N], const uint8_t *buf)
{
    memcpy(sk, buf, N * sizeof(int8_t));
}

int secsidh_keygen(uint8_t *pk, uint8_t *sk)
{
    fp ipk[2];
    int8_t isk[N];

    internal_keygen(ipk, isk);
    secsidh_pk2oct(pk, (const fp *)ipk);
    secsidh_sk2oct(sk, isk);
    secsidh_clear(isk, sizeof(isk));
    return SECSIDH_SUCCESS;
}

/**
 * Compute public key from secret key (sk -> pk).
 * pk: output public key (CTIDH512_PK_SIZE bytes)
 * sk: secret key (CTIDH512_SK_SIZE bytes)
 */
int secsidh_sk_to_pk(uint8_t *pk, const uint8_t *sk)
{
    fp opk;
    int8_t isk[N];

    secsidh_oct2sk(isk, sk);
    action((public_key *)&opk, &base, (private_key *)isk);
    secsidh_clear(isk, sizeof(isk));
    secsidh_pk2oct(pk, &opk);
    return SECSIDH_SUCCESS;
}

int secsidh_derive(uint8_t *ss, const uint8_t *peer_pk, const uint8_t *sk)
{
    fp ipeer_pk[2], iss[1];
    int8_t isk[N];

    secsidh_oct2pk(ipeer_pk, peer_pk);
    secsidh_oct2sk(isk, sk);
    if (internal_derive(iss, ipeer_pk, isk) != 1) {
        secsidh_clear(isk, sizeof(isk));
        return SECSIDH_FAILURE;
    }
    secsidh_clear(isk, sizeof(isk));
    secsidh_ss2oct(ss, (const fp *)iss);
    secsidh_clear(iss, sizeof(iss));
    return SECSIDH_SUCCESS;
}
