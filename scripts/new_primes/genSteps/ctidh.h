#ifndef _CTIDH_H_
#define _CTIDH_H_

#include "csidh_namespace.h"
#include "primes.h"
#include "fp.h"

// extern const size_t NSAPI(pk_size);
// extern const size_t NSAPI(sk_size);
// extern const size_t NSAPI(ss_size);

// typedef struct private_key {
//     int8_t e[primes_num];
// } private_key;

// typedef struct public_key {
//     fp A; /* Montgomery coefficient: represents y^2 = x^3 + Ax^2 + x */
// } public_key;

// extern const public_key base;

// #define gae NS(gae)
// void gae(fp b, int8_t const v[], fp* const a);		// group action evaluation

// void action(public_key *out, public_key const *in, private_key const *priv);

// #define skgen NAMESPACEBITS(skgen)
// void skgen(int8_t* sk);					// secret key generation
// #define pkgen NAMESPACEBITS(pkgen)
// void pkgen(fp* pk, int8_t* const sk);			// public key generation
#define internal_keygen NAMESPACEBITS(keygen)
void internal_keygen(fp* pk, int8_t* sk);			// key generation (both secret and public keys are generated)
#define internal_derive NAMESPACEBITS(derive)
bool internal_derive(fp* ss, fp* const pk, int8_t* const sk);	// secret sharing derivation

int secsidh_keygen(uint8_t *pk, uint8_t *sk);
/** Compute public key from secret key. */
int secsidh_sk_to_pk(uint8_t *pk, const uint8_t *sk);
int secsidh_derive(uint8_t *ss, const uint8_t *peer_pk, const uint8_t *sk);

// #define sk_print NAMESPACEBITS(sk_print)
// void sk_print(int8_t* const a, char *c);
// #define pk_print NAMESPACEBITS(pk_print)
// void pk_print(fp* const a, char *c);
// #define init_keys NAMESPACEBITS(init_keys)
// void init_keys(fp** pk, int8_t** sk);
// #define free_keys NAMESPACEBITS(free_keys)
// void free_keys(fp** pk, int8_t** sk);

// // ----------------------------------------------------
// void fulltorsion_points(fp u, fp const a);
// // ----------------------------------------------------

// // decision bit b has to be either 0 or 1
// static inline void cmov(int8_t *r, const int8_t a, uint32_t b)
// {
//         uint32_t t;
//         b = -b;                 //  Now b is either 0 or 0xffffffff
//         t = (*r ^ a) & b;
//         *r ^= t;
// }

// // check if a and b are equal in constant time
// static inline uint32_t isequal(uint32_t a, uint32_t b)
// {
//         //size_t i;
//         uint32_t r = 0;
//         unsigned char *ta = (unsigned char *)&a;
//         unsigned char *tb = (unsigned char *)&b;
//         r = (ta[0] ^ tb[0]) | (ta[1] ^ tb[1]) | (ta[2] ^ tb[2]) |  (ta[3] ^ tb[3]);
//         r = (-r);
//         r = r >> 31;
//         return (uint32_t)(1-r);
// }

// // get priv[pos] in constant time
// static inline uint32_t lookup(size_t pos, int8_t const priv[])
// {
//         int b;
//         int8_t r = priv[0];
//         for(size_t i = 1; i < N; i++)
//         {
//                 b = isequal(i, pos);
//                 cmov(&r, priv[i], b);
//         }
//         return r;
// }

// // constant-time comparison: 1 if x < y, 0 otherwise.
// static inline int32_t issmaller(int32_t x, int32_t y)
// {
//         int32_t xy = x ^ y;
//         int32_t c = x - y;
//         c ^= xy & (c ^ x);
//         c = c >> 31;
//         return 1 - (uint32_t)(1 + c);
// }

// // constant-time sign computation
// static inline int8_t sign(int8_t const e)
// {
//         int b;
//         int8_t r = 0;

//         // Is e a negative integer?
//         b = issmaller(e, 0);
//         cmov(&r, -1, b);

//         // Is e a positive integer?
//         b = issmaller(0, e);
//         cmov(&r, 1, b);

//         return r;       // Now, r has the sign of e
// }

// // priv[pos] is updated in constant-time with the value in e
// static inline void update(size_t pos, int8_t const e, int8_t priv[])
// {
//         int b;
//         for(size_t i = 0; i < N; i++)
//         {
//                 b = isequal(i, pos);
//                 cmov(&priv[i], e, b);
//         }
// }

#endif
