/*
 * gktimer.c - DateStamp-based jiffy clock (see gktimer.h)
 *
 * We use DateStamp() (dos.library) rather than timer.device: it is KS 1.3
 * (V33) safe, needs no device to open/close, and dos.library is already
 * linked. Its ds_Tick field is in TICKS_PER_SECOND (== 50) units and
 * ds_Minute is minutes-since-midnight, so total jiffies =
 * ds_Minute * 3000 + ds_Tick (3000 = 60 * 50).
 *
 * Why there is no PAL/NTSC detection here
 * ---------------------------------------
 * The Atari port derives getTime() from the OS frame counter, so its
 * getJiffiesPerSecond() must report 50 or 60 depending on the video
 * standard. Ours does not: TICKS_PER_SECOND is 50 by definition in
 * dos/dos.h, and AmigaOS keeps the DateStamp clock in real time on PAL and
 * NTSC machines alike. gamelogic.c divides getTime() by
 * getJiffiesPerSecond(), so returning 60 on an NTSC Amiga would make every
 * countdown run 20% fast — the opposite of a fix. 50 is correct on both.
 */
#include <stdint.h>

#include <proto/dos.h>
#include <dos/dos.h>

#include "gktimer.h"

#define JIFFIES_PER_MINUTE  (60UL * TICKS_PER_SECOND) /* 3000 */
#define JIFFIES_PER_DAY     (1440UL * JIFFIES_PER_MINUTE)

/* Jiffies-since-midnight at the last gk_timer_reset(). */
static uint32_t timer_base = 0;

static uint32_t nowJiffies(void)
{
    struct DateStamp ds;
    DateStamp(&ds);
    /* ds_Days is ignored on purpose: a move timer spans at most minutes, and
       the ds_Minute/ds_Tick pair already covers a full day; the midnight wrap
       is handled in gk_timer_elapsed(). */
    return (uint32_t)ds.ds_Minute * JIFFIES_PER_MINUTE + (uint32_t)ds.ds_Tick;
}

void gk_timer_reset(void)
{
    timer_base = nowJiffies();
}

uint16_t gk_timer_elapsed(void)
{
    uint32_t now = nowJiffies();
    uint32_t elapsed;

    if (now >= timer_base)
        elapsed = now - timer_base;
    else
        elapsed = now + JIFFIES_PER_DAY - timer_base; /* crossed midnight */

    /* A move timer is at most a few hundred seconds; saturate so a very long
       wait can't wrap the uint16_t and read back as a small value. */
    if (elapsed > 0xFFFFUL)
        elapsed = 0xFFFFUL;

    return (uint16_t)elapsed;
}
