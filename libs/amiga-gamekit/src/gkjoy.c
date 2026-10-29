/*
 * gkjoy.c - joystick read for the Amiga game port (port 2)
 *
 * See gkjoy.h for the port-2-only rationale. The decode itself is pure
 * logic in joydecode.c (T1-tested); all that is left here is the two
 * register reads, which cannot be host-tested and so are kept to two lines.
 *
 * Requires: nothing (no library bases)
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * Extracted from apps/battleship/amiga in Track 1C Phase 3b —
 * see docs/plan-track1c-fujitzee.md.
 */
#include "gkjoy.h"
#include "joydecode.h"

#define JOY1DAT (*(volatile uint16_t *)0xDFF00C) /* port-2 counter word    */
#define CIAAPRA (*(volatile uint8_t *)0xBFE001)  /* bit 7 = fire, low = on */

uint8_t gk_joy_read_port2(void)
{
    return joyDecodePort2(JOY1DAT, CIAAPRA);
}
