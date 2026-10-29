/*
 * util.c — upstream/src/platform-specific/util.h on the gamekit clock
 *
 * The clock itself lives in libs/amiga-gamekit (gktimer.h); this file is the
 * thin adapter giving it the names upstream's platform API expects, plus the
 * two things that are genuinely this app's: the stack size and the itoa()
 * gamelogic.c and screens.c want.
 *
 * Same shape as apps/battleship/amiga/src/util.c — deliberately, since the
 * two upstreams define the same platform util surface.
 */
#include <stdio.h>
#include <stdlib.h>

#include "misc.h"

#include "gktimer.h"

/* nix13 crt reads this at startup. Upstream recurses through the screen
 * stack (welcome -> table select -> game) and keeps a 600-byte ClientState
 * plus a 128-byte temp buffer live; 32K matches battleship. */
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
 * see libs/amiga-gamekit/src/gktimer.c for why display-mode detection does
 * NOT belong here (and docs/plan-track1c-fujitzee.md, change 5).
 */
uint8_t getJiffiesPerSecond(void)
{
    return GK_JIFFIES_PER_SECOND;
}

void quit(void)
{
    resetScreen(false);
    resetGraphics();
    exit(0);
}

/* Atari clears its attract-mode countdown here; the Amiga has no equivalent
 * (screen blanking is a Workbench preference we must not fight). */
void housekeeping(void) {}

/* itoa — not in C99; provided here for gamelogic.c and screens.c, which only
 * ever call it with radix 10. */
char *itoa(int value, char *buf, int radix)
{
    (void)radix;
    sprintf(buf, "%d", value);
    return buf;
}
