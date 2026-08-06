/* gkrandom.c — 16-bit xorshift PRNG (see gkrandom.h). Pure C99. */
#include "gkrandom.h"

static uint16_t lfsr = 0xACE1;

void gk_random_seed(uint16_t seed)
{
    if (seed)
        lfsr = seed;
}

uint8_t gk_random(uint8_t maxExclusive)
{
    lfsr ^= (uint16_t)(lfsr >> 7);
    lfsr ^= (uint16_t)(lfsr << 9);
    lfsr ^= (uint16_t)(lfsr >> 13);
    if (maxExclusive == 0)
        return 0;
    return (uint8_t)(lfsr % maxExclusive);
}
