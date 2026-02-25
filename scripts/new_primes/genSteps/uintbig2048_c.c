/* C implementation of uintbig for 2048-bit (32 limbs). Use for ARM / mobile when .S is not available.
 * Same API and constants as uintbig2048.S; compile with -DBITS=2048 -D'NAMESPACEBITS(x)=highctidh_2048_##x'. */
#include "uintbig.h"
#include "uintbig_namespace.h"

#define N UINTBIG_LIMBS

const uintbig uintbig_1 = {
  .c = {
    1, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
  }
};

const uintbig uintbig_p = {
  .c = {
    0xffffffffffffffffULL, 0xcb336e65dd9ab972ULL, 0x31da9904076e6a70ULL, 0x9f825d7bea428f54ULL,
    0x80b1d7895deabe60ULL, 0xd3b3cde44a99ebd8ULL, 0x2c433638fe69f531ULL, 0x46c02600030671e2ULL,
    0x3eb36ebf24eb264fULL, 0x5872fd3292cf3725ULL, 0x25261b882c99a891ULL, 0x5aa0c7a8e51c618fULL,
    0xdfe30228dc090a5eULL, 0x1235fff9c3496720ULL, 0x556d82dfb25a19e0ULL, 0xaf0d91a49ad1fcf6ULL,
    0x5cfe8bddcaaf7214ULL, 0x67d90f84820f3740ULL, 0x85dd61c29f9ea3faULL, 0x1b493e97510173baULL,
    0x57963f8a2ef281c0ULL, 0x215b3ca607652a95ULL, 0xbe13d52039838268ULL, 0xc1b8b0ac479fc5ebULL,
    0xaacddf254a1ceff1ULL, 0x5d89b511362ee752ULL, 0x5c935f8b2c251c39ULL, 0x3336ccfc4dd81041ULL,
    0x0fe4aea834b8f332ULL, 0x08aa77cdce5cca52ULL, 0x2dd1afc034e7a378ULL, 0x4b5882ea64ea57e7ULL
  }
};

const uintbig uintbig_four_sqrt_p = {
  .c = {
    0x828c5b71c63ded09ULL, 0x53568dfb25b9e64bULL, 0x88ed2a85fe7c83f8ULL, 0xd141c0753910d662ULL,
    0x132eeff80db164daULL, 0x8826735eb8597692ULL, 0x897d813f6f963472ULL, 0x221ca7b1f40d304aULL,
    0x38b3d211dff4f3f4ULL, 0x07463586b8f131fcULL, 0xaf328a440f017948ULL, 0x3a016ee0b4c48850ULL,
    0xe6087107526f888aULL, 0xa39c1d32230c1263ULL, 0x94b9d5d2b6451926ULL, 0x2b884784e60b8d0cULL,
    2, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
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

