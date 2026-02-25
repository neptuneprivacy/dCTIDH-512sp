/* Portable C implementation of int32_sort (no AVX2/x86). Use for ARM/mobile.
 * Constant-time odd-even sort. Same API as int32_sort.c. */
#include "int32_sort.h"
#include <stdint.h>

#define int32 int32_t

static inline void int32_MINMAX(int32 *a, int32 *b)
{
  int32 x = *a, y = *b;
  int32 mask = (int32)((uint32_t)((y - x) >> 31));
  *a = x ^ ((x ^ y) & mask);
  *b = y ^ ((x ^ y) & mask);
}

void crypto_sort_int32(int32 *x, long long n)
{
  if (n <= 1) return;
  for (long long p = 0; p < n; p++) {
    for (long long i = (p & 1); i + 1 < n; i += 2)
      int32_MINMAX(&x[i], &x[i + 1]);
  }
}

const char crypto_sort_int32_implementation[] = "portable";
const char crypto_sort_int32_version[] = "1.0";
const char crypto_sort_int32_compiler[] = "gcc";
