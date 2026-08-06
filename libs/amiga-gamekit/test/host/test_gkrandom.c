/* T1: xorshift PRNG bounds and behavior (gkrandom.c). */
#define FN_TEST_MAIN
#include "fn_test.h"

#include "gkrandom.h"
#include "../../src/gkrandom.c"

int main(void)
{
    int i;
    int seen[6] = { 0, 0, 0, 0, 0, 0 };
    int distinct = 0;
    uint8_t first[8], again[8];

    /* Degenerate bound must not divide by zero. */
    CHECK_EQ(gk_random(0), 0);

    /* maxExclusive == 1 always yields 0. */
    for (i = 0; i < 32; i++)
        CHECK_EQ(gk_random(1), 0);

    /* A die: every value in range, and all six actually turn up. Fujitzee
     * rolls five dice a few thousand times a game — a generator that never
     * produces a face would be very visible. */
    for (i = 0; i < 2000; i++) {
        uint8_t v = gk_random(6);
        CHECK(v < 6);
        seen[v] = 1;
    }
    for (i = 0; i < 6; i++)
        distinct += seen[i];
    CHECK_EQ(distinct, 6);

    /* Reseeding is deterministic — same seed, same sequence. */
    gk_random_seed(0x1234);
    for (i = 0; i < 8; i++)
        first[i] = gk_random(100);
    gk_random_seed(0x1234);
    for (i = 0; i < 8; i++)
        again[i] = gk_random(100);
    for (i = 0; i < 8; i++)
        CHECK_EQ(again[i], first[i]);

    /* A zero seed is ignored rather than locking the xorshift at zero. */
    gk_random_seed(0);
    distinct = 0;
    for (i = 0; i < 64; i++)
        if (gk_random(64) != 0)
            distinct = 1;
    CHECK(distinct);

    return fn_test_report("test_gkrandom");
}
