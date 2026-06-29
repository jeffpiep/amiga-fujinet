#include "misc.h"
#include <proto/exec.h>
#include <devices/timer.h>

unsigned long __stack = 32768;

static uint32_t timer_base = 0;

void resetTimer(void)
{
    timer_base = 0; /* Phase 1 stub — use tick counter in Phase 2 */
}

uint16_t getTime(void)
{
    return 0; /* Phase 1 stub */
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
