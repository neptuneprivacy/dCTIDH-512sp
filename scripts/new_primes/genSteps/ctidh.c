#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "fp.h"
#include "primes.h"
#include "csidh.h"
#include "ctidh.h"

#define N primes_num

static int dumb_fp_isequal(const uint8_t *a, const uint8_t *b, size_t size)
{
    return memcmp(a, b, size) == 0;
}

int main(void)
{
    uint8_t a[sizeof(private_key)], A[sizeof(public_key)], ss_a[sizeof(public_key)];
    secsidh_keygen(A, a);

    uint8_t b[sizeof(private_key)], B[sizeof(public_key)], ss_b[sizeof(public_key)];
    secsidh_keygen(B, b);

    assert(secsidh_derive(ss_a, B, a) == 0);
    assert(secsidh_derive(ss_b, A, b) == 0);
    assert(dumb_fp_isequal(ss_a, ss_b, sizeof(public_key)));
    return 0;
}
