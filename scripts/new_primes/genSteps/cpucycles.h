#ifndef cpucycles_h
#define cpucycles_h

#if defined(__APPLE__) && defined(__MACH__) && (defined(__aarch64__) || defined(_M_ARM64))
#include <mach/mach_time.h>
#endif
#if !defined(__x86_64__) && !defined(_M_X64) && !defined(__i386__) && !defined(_M_IX86) \
    && !defined(__aarch64__) && !defined(_M_ARM64) && !(defined(__APPLE__) && defined(__MACH__))
#include <time.h>
#endif

static inline unsigned long long cpucycles(void) {
  unsigned long long result;

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
  asm volatile("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
    : "=a" (result) : : "%rdx");
#elif defined(__aarch64__) || defined(_M_ARM64)
  /* ARM64: use virtual count register (cntvct_el0) */
  asm volatile("mrs %0, cntvct_el0" : "=r" (result));
#elif defined(__APPLE__) && defined(__MACH__)
  result = (unsigned long long)mach_absolute_time();
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  result = (unsigned long long)ts.tv_sec * 1000000000ULL + (unsigned long long)ts.tv_nsec;
#endif

  return result;
}

#endif
