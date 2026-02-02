/*
 * Demo program for CTIDH-512: same output format as main/ctidh.c (e.g. ctidh-2047m4l205.main)
 * Build with: make ctidh_demo  (in genSteps, BITS=512)
 * Output: Key generation, Alice/Bob pk/sk, Secret sharing, ss_a/ss_b, success message.
 */
#undef NDEBUG
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "fp.h"
#include "primes.h"
#include "csidh.h"
#include "ctidh.h"
#include "cpucycles.h"

#define SK_SIZE (sizeof(private_key))
#define PK_SIZE (sizeof(public_key))
#define SS_SIZE (sizeof(public_key))

static void ss_print(const uint8_t *ss, const char *name, size_t size)
{
    printf("%s := 0x", name);
    for (int i = (int)size - 1; i >= 0; i--)
        printf("%02" PRIX8 "", ss[i]);
    printf(";\n");
}

static int dumb_fp_isequal(const uint8_t *a, const uint8_t *b, size_t size)
{
    return memcmp(a, b, size) == 0;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    unsigned long long cc0, cc1;

    printf("\033[0;33m// --------------\033[0m\n");
    printf("\033[0;33m// Key generation\033[0m\n");
    printf("sizeof private key = %zu \n", SK_SIZE);
    printf("sizeof public key = %zu \n", PK_SIZE);

    uint8_t a[SK_SIZE], A[PK_SIZE], ss_a[SS_SIZE];
    uint8_t b[SK_SIZE], B[PK_SIZE], ss_b[SS_SIZE];

    printf("\n\033[0;35m// Alice\033[0m\n");
    cc0 = cpucycles();
    secsidh_keygen(A, a);
    cc1 = cpucycles();
    ss_print(a, "sk_a", SK_SIZE);
    ss_print(A, "pk_a", PK_SIZE);
    printf("Running-time (millions): 0.000M + 0.000S + 0.000a = \033[1;35m0.000M\033[0m\n");
    printf("Clock cycles (millions): \033[1;35m%7.03lf\033[0m\n", (double)(cc1 - cc0) / 1e6);

    printf("\n\033[0;34m// Bob\033[0m\n");
    cc0 = cpucycles();
    secsidh_keygen(B, b);
    cc1 = cpucycles();
    ss_print(b, "sk_b", SK_SIZE);
    ss_print(B, "pk_b", PK_SIZE);
    printf("Running-time (millions): 0.000M + 0.000S + 0.000a = \033[1;34m0.000M\033[0m\n");
    printf("Clock cycles (millions): \033[1;34m%7.03lf\033[0m\n", (double)(cc1 - cc0) / 1e6);

    fflush(stdout);

    printf("\n\033[0;33m// -------------------------\033[0m\n");
    printf("\033[0;33m// Secret sharing generation\033[0m\n");

    printf("\n\033[0;35m// Alice\033[0m\n");
    cc0 = cpucycles();
    assert(secsidh_derive(ss_a, B, a) == 0);
    cc1 = cpucycles();
    ss_print(ss_a, "ss_a", SS_SIZE);
    printf("Running-time (millions) [without validation]: 0.000M + 0.000S + 0.000a = \033[1;35m0.000M\033[0m\n");
    printf("Clock cycles (millions) [including validation]: \033[1;35m%7.03lf\033[0m\n", (double)(cc1 - cc0) / 1e6);

    printf("\n\033[0;34m// Bob\033[0m\n");
    cc0 = cpucycles();
    assert(secsidh_derive(ss_b, A, b) == 0);
    cc1 = cpucycles();
    ss_print(ss_b, "ss_b", SS_SIZE);
    printf("Running-time (millions) [without validation]: 0.000M + 0.000S + 0.000a = \033[1;34m0\033[0m\n");
    printf("Clock cycles (millions) [including validation]: \033[1;34m%7.03lf\033[0m\n", (double)(cc1 - cc0) / 1e6);

    fflush(stdout);

    assert(dumb_fp_isequal(ss_a, ss_b, SS_SIZE));
    printf("\n\033[0;32m// Successfully secret sharing computation!\033[0m\n");
    return 0;
}
