/*
 * Compare public key generation across CTIDH parameter sets (512, 2048).
 * Build once per BITS (test_pubkey_compare512, test_pubkey_compare2048).
 * Run both and compare output; or use "make test_pubkey_compare" to run both.
 *
 * Output format (one line): BITS=<n> PK_SIZE=<bytes> SK_SIZE=<bytes> SS_SIZE=<bytes> keygen_ok=1 derive_match=1
 */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ctidh.h"
#include "csidh.h"
#include "primes.h"

#define PK_SIZE (sizeof(public_key))
#define SK_SIZE (sizeof(private_key))
#define SS_SIZE (sizeof(public_key))

/* Print buf as hex in big-endian order with "0x" prefix (same as ctidh_demo.c).
 * So for 64 bytes we get "0x" + 128 hex chars = 130 chars total.
 * Rust hex::encode() uses little-endian (byte 0 first) and no "0x", so same key
 * looks different and is 2 chars shorter in Rust unless you add "0x" and reverse. */
static void print_hex(const char *name, const uint8_t *buf, size_t len)
{
    printf("%s=0x", name);
    for (size_t i = len; i > 0; i--)
        printf("%02x", buf[i - 1]);
    printf("\n");
}

int main(void)
{
    uint8_t pk_a[PK_SIZE], sk_a[SK_SIZE];
    uint8_t pk_b[PK_SIZE], sk_b[SK_SIZE];
    uint8_t ss_a[SS_SIZE], ss_b[SS_SIZE];
    int keygen_ok = 1;
    int derive_match = 0;

    /* Alice keygen */
    if (secsidh_keygen(pk_a, sk_a) != 0)
        keygen_ok = 0;

    /* Bob keygen */
    if (secsidh_keygen(pk_b, sk_b) != 0)
        keygen_ok = 0;

    /* Derive shared secret: Alice with Bob's pk, Bob with Alice's pk */
    if (keygen_ok) {
        if (secsidh_derive(ss_a, pk_b, sk_a) != 0)
            keygen_ok = 0;
        if (secsidh_derive(ss_b, pk_a, sk_b) != 0)
            keygen_ok = 0;
        derive_match = (keygen_ok && memcmp(ss_a, ss_b, SS_SIZE) == 0);
    }

    /* Single line for easy parsing / comparison */
    printf("BITS=%d PK_SIZE=%zu SK_SIZE=%zu SS_SIZE=%zu keygen_ok=%d derive_match=%d\n",
           (int)BITS, (size_t)PK_SIZE, (size_t)SK_SIZE, (size_t)SS_SIZE,
           keygen_ok, derive_match);

    print_hex("pk_alice", pk_a, PK_SIZE);
    print_hex("pk_bob", pk_b, PK_SIZE);

    assert(keygen_ok);
    assert(derive_match);
    return 0;
}
