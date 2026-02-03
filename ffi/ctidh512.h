/**
 * CTIDH-512 FFI header for Rust (and other languages).
 * Only the 512-bit parameter set.
 *
 * Build the library from scripts/new_primes/genSteps (make ctidh-512.main
 * or the combined lib used by the Rust crate).
 */
#ifndef CTIDH512_FFI_H
#define CTIDH512_FFI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Key sizes in bytes (512-bit parameter set) */
#define CTIDH512_SK_SIZE 74
#define CTIDH512_PK_SIZE 64
#define CTIDH512_SS_SIZE 64

/**
 * Generate a key pair (512-bit parameter set).
 * pk: output public key (CTIDH512_PK_SIZE bytes)
 * sk: output secret key (CTIDH512_SK_SIZE bytes)
 * Returns 0 on success, non-zero on failure.
 */
int secsidh_keygen(uint8_t *pk, uint8_t *sk);

/**
 * Compute public key from secret key (sk -> pk).
 * pk: output public key (CTIDH512_PK_SIZE bytes)
 * sk: secret key (CTIDH512_SK_SIZE bytes)
 */
int secsidh_sk_to_pk(uint8_t *pk, const uint8_t *sk);

/**
 * Derive shared secret from peer's public key and own secret key.
 * ss: output shared secret (CTIDH512_SS_SIZE bytes)
 * pk: peer's public key (CTIDH512_PK_SIZE bytes)
 * sk: own secret key (CTIDH512_SK_SIZE bytes)
 * Returns 0 on success, -1 if pk is invalid (reject invalid curves).
 */
int secsidh_derive(uint8_t *ss, const uint8_t *pk, const uint8_t *sk);

#ifdef __cplusplus
}
#endif

#endif /* CTIDH512_FFI_H */
