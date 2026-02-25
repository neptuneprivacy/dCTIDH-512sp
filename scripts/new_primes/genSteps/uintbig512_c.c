/* C implementation of uintbig for 512-bit (8 limbs). Use for ARM / mobile when .S is not available.
 * Same API and constants as uintbig512.S; compile with -DBITS=512 -D'NAMESPACEBITS(x)=highctidh_512_##x'. */
#include "uintbig.h"
#include "uintbig_namespace.h"

#define N UINTBIG_LIMBS

const uintbig uintbig_1 = {
  .c = { 1, 0, 0, 0, 0, 0, 0, 0 }
};

const uintbig uintbig_p = {
  .c = {
    0x1b81b90533c6c87bULL, 0xc2721bf457aca835ULL, 0x516730cc1f0b4f25ULL, 0xa7aac6c567f35507ULL,
    0x5afbfcc69322c9cdULL, 0xb42d083aedc88c42ULL, 0xfc8ab0d15e3e4c4aULL, 0x65b48e8f740f89bfULL
  }
};

const uintbig uintbig_four_sqrt_p = {
  .c = {
    0x17895e71e1a20b3fULL, 0x38d0cd95f8636a56ULL, 0x142b9541e59682cdULL, 0x856f1399d91d6592ULL,
    2, 0, 0, 0
  }
};

void uintbig_set(uintbig *x, uint64_t y)
{
  x->c[0] = y;
  for (long long i = 1; i < N; i++)
    x->c[i] = 0;
}

long long uintbig_bit(uintbig const *x, uint64_t k)
{
  unsigned idx = (unsigned)(k >> 6);
  unsigned shift = (unsigned)(k & 63);
  if (idx >= N) return 0;
  return (long long)((x->c[idx] >> shift) & 1);
}

long long uintbig_add(uintbig *x, uintbig const *y, uintbig const *z)
{
  unsigned long long carry = 0;
  for (long long i = 0; i < N; i++) {
    unsigned long long sum = y->c[i] + z->c[i] + carry;
    carry = (carry ? (sum <= y->c[i]) : (sum < y->c[i]));
    x->c[i] = (uint64_t)sum;
  }
  return (long long)carry;
}

long long uintbig_sub(uintbig *x, uintbig const *y, uintbig const *z)
{
  unsigned long long borrow = 0;
  for (long long i = 0; i < N; i++) {
    unsigned __int128 diff = (unsigned __int128)y->c[i] - (unsigned __int128)z->c[i] - (unsigned __int128)borrow;
    borrow = (diff >> 64) ? 1ULL : 0ULL;
    x->c[i] = (uint64_t)diff;
  }
  return (long long)borrow;
}

void uintbig_mul3_64(uintbig *x, uintbig const *y, uint64_t z)
{
  unsigned long long carry = 0;
  for (long long i = 0; i < N; i++) {
    unsigned __int128 prod = (unsigned __int128)y->c[i] * (unsigned __int128)z + carry;
    x->c[i] = (uint64_t)prod;
    carry = (uint64_t)(prod >> 64);
  }
}
