/* C implementation of fp (Montgomery) for 512-bit. Use for ARM/mobile when fp512.S is not available.
 * Compile with -DBITS=512 -D'NAMESPACEBITS(x)=highctidh_512_##x'. */
#include "fp.h"
#include "fp_namespace.h"
#include "uintbig.h"
#include "uintbig_namespace.h"

#define N 8
#define INV_MIN_P_MOD_R 0x66c1301f632e294dULL

const fp fp_0 = { 0 };
const fp fp_1 = {
  0xc8fc8df598726f0aULL, 0x7b1bc81750a6af95ULL, 0x5d319e67c1e961b4ULL, 0xb0aa7275301955f1ULL,
  0x4a080672d9ba6c64ULL, 0x97a5ef8a246ee77bULL, 0x06ea9e5d4383676aULL, 0x3496e2e117e0ec80ULL
};
const fp fp_2 = {
  0x767762e5fd1e1599ULL, 0x33c5743a49a0b6f6ULL, 0x68fc0c0364c77443ULL, 0xb9aa1e24f83f56dbULL,
  0x3914101f20520efbULL, 0x7b1ed6d95b1542b4ULL, 0x114a8be928c8828aULL, 0x03793732bbb24f40ULL
};
const fp fp_p = { 0 };

long long fp_mulsq_count = 0;
long long fp_sq_count = 0;
long long fp_addsub_count = 0;

void fp_copy(fp b, const fp a)
{
  for (long long i = 0; i < N; i++)
    b[i] = a[i];
}

void fp_cmov(fp *x, const fp *y, long long c)
{
  uint64_t mask = (uint64_t)(-(uint64_t)(c != 0));
  for (long long i = 0; i < N; i++)
    (*x)[i] ^= (mask & ((*x)[i] ^ (*y)[i]));
}

void fp_cswap(fp x, fp y, long long c)
{
  uint64_t mask = (uint64_t)(-(uint64_t)(c != 0));
  for (long long i = 0; i < N; i++) {
    uint64_t t = mask & (x[i] ^ y[i]);
    x[i] ^= t;
    y[i] ^= t;
  }
}

static void reduce_once(fp x)
{
  long long b = uintbig_sub((uintbig *)x, (const uintbig *)x, &uintbig_p);
  if (b)
    uintbig_add((uintbig *)x, (const uintbig *)x, &uintbig_p);
}

void fp_add2(fp *x, fp const *y)
{
  fp_add3(x, x, y);
}

void fp_add3(fp *x, fp const *y, fp const *z)
{
  fp_addsub_count++;
  uintbig_add((uintbig *)x, (const uintbig *)y, (const uintbig *)z);
  reduce_once(*x);
}

void fp_sub2(fp *x, fp const *y)
{
  fp tmp;
  fp_copy(tmp, *x);
  fp_sub3(x, &tmp, y);
}

void fp_sub3(fp *x, fp const *y, fp const *z)
{
  fp_addsub_count++;
  long long b = uintbig_sub((uintbig *)x, (const uintbig *)y, (const uintbig *)z);
  if (b)
    uintbig_add((uintbig *)x, (uintbig *)x, &uintbig_p);
}

static void montgomery_reduce(uint64_t *out, uint64_t *T)
{
  const uint64_t *p = uintbig_p.c;
  for (long long i = 0; i < N; i++) {
    uint64_t m = (uint64_t)((unsigned __int128)T[i] * (unsigned __int128)INV_MIN_P_MOD_R);
    unsigned __int128 carry = 0;
    for (long long j = 0; j < N; j++) {
      carry += (unsigned __int128)T[i + j] + (unsigned __int128)m * (unsigned __int128)p[j];
      T[i + j] = (uint64_t)carry;
      carry >>= 64;
    }
    /* propagate full carry into T[i+N], T[i+N+1], ... */
    for (long long k = 0; carry; k++) {
      carry += (unsigned __int128)T[i + N + k];
      T[i + N + k] = (uint64_t)carry;
      carry >>= 64;
    }
  }
  for (long long i = 0; i < N; i++)
    out[i] = T[N + i];
}

static void fp_mul3_impl(fp *x, fp const *y, fp const *z)
{
  uint64_t T[16];
  const uint64_t *a = *y;
  const uint64_t *b = *z;
  for (long long i = 0; i < 16; i++)
    T[i] = 0;
  for (long long i = 0; i < N; i++) {
    unsigned __int128 carry = 0;
    for (long long j = 0; j < N; j++) {
      carry += (unsigned __int128)T[i + j] + (unsigned __int128)a[i] * (unsigned __int128)b[j];
      T[i + j] = (uint64_t)carry;
      carry >>= 64;
    }
    /* propagate full carry into T[i+N..] */
    for (long long k = 0; carry && (i + N + k < 16); k++) {
      carry += (unsigned __int128)T[i + N + k];
      T[i + N + k] = (uint64_t)carry;
      carry >>= 64;
    }
  }
  montgomery_reduce(*x, T);
  reduce_once(*x);
}

void fp_mul2(fp *x, fp const *y)
{
  fp_mul3(x, x, y);
}

void fp_mul3(fp *x, fp const *y, fp const *z)
{
  fp_mulsq_count++;
  fp_mul3_impl(x, y, z);
}

void fp_sq1(fp *x)
{
  fp_sq2(x, x);
}

void fp_sq2(fp *x, fp const *y)
{
  fp_sq_count++;
  fp_mul3_impl(x, y, y);
}
