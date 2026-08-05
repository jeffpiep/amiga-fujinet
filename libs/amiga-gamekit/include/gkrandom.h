/*
 * gkrandom.h - small xorshift PRNG for Amiga game ports
 *
 * Both upstreams call getRandomNumber(maxExclusive) for cosmetic choices
 * (dice roll animation, taunt selection) — nothing security- or
 * fairness-critical, so a 16-bit xorshift is plenty and costs no division
 * beyond the final modulo.
 *
 * Pure: T1 host-testable (test/host/test_gkrandom.c).
 */
#ifndef GKRANDOM_H
#define GKRANDOM_H

#include <stdint.h>

/* Returns 0 <= n < maxExclusive, or 0 when maxExclusive is 0. */
uint8_t gk_random(uint8_t maxExclusive);

/* Reseed the generator. A zero seed is ignored (it would lock the xorshift
 * at zero forever); games that want per-run variation should mix in
 * something like the low word of a DateStamp. */
void gk_random_seed(uint16_t seed);

#endif /* GKRANDOM_H */
