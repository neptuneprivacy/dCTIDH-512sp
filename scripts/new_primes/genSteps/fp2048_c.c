/* C implementation of fp (Montgomery) for 2048-bit. Use for ARM/mobile when fp2048.S is not available.
 * Compile with -DBITS=2048 -D'NAMESPACEBITS(x)=highctidh_2048_##x'. */
#include "fp.h"
#include "fp_namespace.h"
#include "uintbig.h"
#include "uintbig_namespace.h"

#define N 32
#define INV_MIN_P_MOD_R 1ULL /* from fp2048.S: inv_min_p_mod_r = 1 */

const fp fp_0 = { 0 };

const fp fp_1 = {
  3ULL, 0x9e65b4ce672fd3a7ULL, 0x6a7034f3e9b4c0adULL, 0x2178e78c41385203ULL,
  0x7dea7963e63fc4deULL, 0x84e4965320323c76ULL, 0x7b365d5504c2206aULL, 0x2bbf8dfff6ecaa59ULL,
  0x43e5b3c2913e8d12ULL, 0xf6a7086847925a90ULL, 0x908dad677a33064bULL, 0xf01da90550aadb52ULL,
  0x6056f9856be4e0e4ULL, 0xc95e0012b623ca9dULL, 0xffb77760e8f1b25fULL, 0xf2d74b122f8a091cULL,
  0xe9045c669ff1a9c1ULL, 0xc874d17279d25a3eULL, 0x6e67dab821241410ULL, 0xae24443a0cfba4d0ULL,
  0xf93d416173287abfULL, 0x9bee4a0de9d0803fULL, 0xc5c4809f537578c7ULL, 0xbad5edfb2920ae3cULL,
  0xff96629021a9302aULL, 0xe762e0cc5d734a07ULL, 0xea45e15e7b90ab53ULL, 0x665b990b1677cf3bULL,
  0xd051f40761d52669ULL, 0xe600989694e9a109ULL, 0x768af0bf61491597ULL, 0x1df67740d140f84aULL
};

const fp fp_2 = {
  6ULL, 0x3ccb699cce5fa74eULL, 0xd4e069e7d369815bULL, 0x42f1cf188270a406ULL,
  0xfbd4f2c7cc7f89bcULL, 0x09c92ca6406478ecULL, 0xf66cbaaa098440d5ULL, 0x577f1bffedd954b2ULL,
  0x87cb6785227d1a24ULL, 0xed4e10d08f24b520ULL, 0x211b5acef4660c97ULL, 0xe03b520aa155b6a5ULL,
  0xc0adf30ad7c9c1c9ULL, 0x92bc00256c47953aULL, 0xff6eeec1d1e364bfULL, 0xe5ae96245f141239ULL,
  0xd208b8cd3fe35383ULL, 0x90e9a2e4f3a4b47dULL, 0xdccfb57042482821ULL, 0x5c48887419f749a0ULL,
  0xf27a82c2e650f57fULL, 0x37dc941bd3a1007fULL, 0x8b89013ea6eaf18fULL, 0x75abdbf652415c79ULL,
  0xff2cc52043526055ULL, 0xcec5c198bae6940fULL, 0xd48bc2bcf72156a7ULL, 0xccb732162cef9e77ULL,
  0xa0a3e80ec3aa4cd2ULL, 0xcc01312d29d34213ULL, 0xed15e17ec2922b2fULL, 0x3becee81a281f094ULL
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
    /* propagate full carry into T[i+N..] */
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
  uint64_t T[2 * N];
  const uint64_t *a = *y;
  const uint64_t *b = *z;
  for (long long i = 0; i < 2 * N; i++)
    T[i] = 0;
  for (long long i = 0; i < N; i++) {
    unsigned __int128 carry = 0;
    for (long long j = 0; j < N; j++) {
      carry += (unsigned __int128)T[i + j] + (unsigned __int128)a[i] * (unsigned __int128)b[j];
      T[i + j] = (uint64_t)carry;
      carry >>= 64;
    }
    /* propagate full carry into T[i+N..] */
    for (long long k = 0; carry && (i + N + k < 2 * N); k++) {
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

