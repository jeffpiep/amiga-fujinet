/* T1: jiffy -> seconds conversion (gkclock.c).
 *
 * Guards the invariant the countdown clock depends on: the gamekit's jiffy
 * unit is the dos.library DateStamp tick (50/s of real time on PAL and NTSC
 * alike), so this conversion must NOT vary with display mode. A regression
 * here shows up as a visibly fast or slow move timer. */
#define FN_TEST_MAIN
#include "fn_test.h"

#include "gktimer.h"
#include "../../src/gkclock.c"

int main(void)
{
    /* The unit is fixed by the DateStamp tick, not the video standard. */
    CHECK_EQ(GK_JIFFIES_PER_SECOND, 50);

    CHECK_EQ(gk_timer_seconds(0), 0);
    CHECK_EQ(gk_timer_seconds(49), 0);      /* truncates, like upstream's / */
    CHECK_EQ(gk_timer_seconds(50), 1);
    CHECK_EQ(gk_timer_seconds(99), 1);
    CHECK_EQ(gk_timer_seconds(100), 2);

    /* A 60-second move timer, the value gamelogic.c uses most. */
    CHECK_EQ(gk_timer_seconds(60 * 50), 60);

    /* Saturated elapsed value must not wrap or overflow the return type. */
    CHECK_EQ(gk_timer_seconds(0xFFFF), 1310);

    return fn_test_report("test_gkclock");
}
