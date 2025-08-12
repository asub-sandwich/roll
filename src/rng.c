#define _POSIX_C_SOURCE 199309L
#include "rng.h"
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#endif

static uint64_t seed_from_time(void) {
#if defined(_WIN32)
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  uint64_t t = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
  return t ^ (t << 13) ^ (t >> 7);
#else
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (uint64_t)ts.tv_sec ^ ((uint64_t)ts.tv_nsec << 32);
#endif
}

Rng rng_new(void) {
  Rng r;
  uint64_t seed = seed_from_time();
  r.state = (seed ? seed : 0x9E3779B97F4A7C15ULL);
  return r;
}

uint64_t rng_next_u64(Rng* r) {
  uint64_t x = r->state;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  r->state = x;
  return x * 0x2545F4914F6CDD1DULL;
}

unsigned rng_range(Rng* r, unsigned low, unsigned high) {
  return (unsigned)(rng_next_u64(r) % (high - low + 1)) + low;
}
