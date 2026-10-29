/*
 * util.c - upstream/src/platform-specific/util.h on the gamekit clock
 *
 * The clock and PRNG themselves live in libs/amiga-gamekit (gktimer.h,
 * gkrandom.h); this file is the thin adapter that gives them the names
 * upstream's platform API expects, plus the two things that are genuinely
 * battleship's: the stack size and the itoa() gamelogic.c wants.
 */
#include "misc.h"

#include "gktimer.h"
#include "gkrandom.h"

unsigned long __stack = 32768;

void resetTimer(void)
{
    gk_timer_reset();
}

uint16_t getTime(void)
{
    return gk_timer_elapsed();
}

/*
 * Must match the unit getTime() counts in — gamelogic.c only ever computes
 * (maxJifs - getTime()) / getJiffiesPerSecond(). The gamekit clock is
 * DateStamp-based, so that unit is 50/s of real time on PAL and NTSC alike;
 * see the note in libs/amiga-gamekit/src/gktimer.c about why display-mode
 * detection does NOT belong here.
 */
uint8_t getJiffiesPerSecond(void)
{
    return GK_JIFFIES_PER_SECOND;
}

uint8_t getRandomNumber(uint8_t maxExclusive)
{
    return gk_random(maxExclusive);
}

void quit(void)
{
    exit(0);
}

void housekeeping(void) {}

/* itoa — not in C99 standard; provided here for gamelogic.c */
char *itoa(int value, char *buf, int radix)
{
    (void)radix; /* only radix 10 is used; sprintf handles it */
    sprintf(buf, "%d", value);
    return buf;
}
