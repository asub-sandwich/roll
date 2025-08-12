#ifndef RNG_H
#define RNG_H

#include <stdint.h>

typedef struct {
  uint64_t state;
} Rng;

Rng rng_new(void);
uint64_t rng_next_u64(Rng* r);
unsigned rng_range(Rng* r, unsigned low, unsigned high);

#endif
