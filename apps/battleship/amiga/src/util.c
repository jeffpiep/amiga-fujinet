#include "misc.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <devices/timer.h>

unsigned long __stack = 32768;

/*
 * Move timer, expressed in 1/50-second jiffies (matches getJiffiesPerSecond()).
 *
 * We use DateStamp() (dos.library) rather than timer.device: it is KS 1.3 (V33)
 * safe, needs no device to open/close, and dos.library is already linked. Its
 * ds_Tick field is in TICKS_PER_SECOND (== 50) units and ds_Minute is
 * minutes-since-midnight, so total jiffies = ds_Minute * 3000 + ds_Tick
 * (3000 = 60 * 50). This gives exactly the 50Hz base the game logic assumes.
 */
#define JIFFIES_PER_MINUTE  (60UL * TICKS_PER_SECOND) /* 3000 */
#define JIFFIES_PER_DAY     (1440UL * JIFFIES_PER_MINUTE)

/* Jiffies-since-midnight at the last resetTimer(). */
static uint32_t timer_base = 0;

static uint32_t nowJiffies(void)
{
    struct DateStamp ds;
    DateStamp(&ds);
    /* ds_Days is ignored on purpose: a move timer spans at most minutes, and
       the ds_Minute/ds_Tick pair already covers a full day; the midnight wrap
       is handled in getTime(). */
    return (uint32_t)ds.ds_Minute * JIFFIES_PER_MINUTE + (uint32_t)ds.ds_Tick;
}

void resetTimer(void)
{
    timer_base = nowJiffies();
}

uint16_t getTime(void)
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

uint8_t getJiffiesPerSecond(void)
{
    return 50; /* PAL default */
}

uint8_t getRandomNumber(uint8_t maxExclusive)
{
    static uint16_t lfsr = 0xACE1;
    lfsr ^= lfsr >> 7;
    lfsr ^= lfsr << 9;
    lfsr ^= lfsr >> 13;
    if (maxExclusive == 0) return 0;
    return (uint8_t)(lfsr % maxExclusive);
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
