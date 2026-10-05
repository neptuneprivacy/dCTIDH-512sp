#include "randombytes.h"

#include <stdlib.h>

// #include "crypto_classify.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>

void randombytes(void *x, size_t l)
{
  unsigned char *p = (unsigned char *) x;
  while (l > 0) {
    ULONG n = l > 0x7fffffff ? 0x7fffffff : (ULONG) l;
    if (!BCRYPT_SUCCESS(BCryptGenRandom(NULL, p, n, BCRYPT_USE_SYSTEM_PREFERRED_RNG)))
      exit(2);
    p += n;
    l -= n;
  }
}

#elif defined(GETRANDOM) && !defined(__APPLE__)
#include <unistd.h>
#include <sys/random.h>

void randombytes(void *x, size_t l)
{
  ssize_t n;
  for (size_t i = 0; i < l; i += n)
    if (0 >= (n = getrandom((char *) x + i, l - i, 0)))
      exit(2);
//   crypto_classify(x,l);
}

#else
#include <unistd.h>
#include <fcntl.h>

void randombytes(void *x, size_t l)
{
    static int fd = -1;
    ssize_t n;
    if (fd < 0 && 0 > (fd = open("/dev/urandom", O_RDONLY)))
        exit(1);
    for (size_t i = 0; i < l; i += n)
        if (0 >= (n = read(fd, (char *) x + i, l - i)))
            exit(2);
    // crypto_classify(x,l);
}
#endif
