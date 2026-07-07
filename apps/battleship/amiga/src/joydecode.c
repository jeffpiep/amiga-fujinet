/* joydecode.c — pure JOYxDAT → JOY_* bitmask decode (T1 host-testable).
 *
 * JOYxDAT holds two quadrature counters (HRM "Joystick" decode):
 *   right = bit 1          left = bit 9
 *   down  = bit 0 ^ bit 1  up   = bit 8 ^ bit 9
 *
 * Output bit layout must match the JOY_* macros in amiga_vars.h:
 * up=0x01 down=0x02 left=0x04 right=0x08 btn1=0x10.
 */
#include "joydecode.h"

uint8_t joyDecode(uint16_t joydat, uint8_t fire_pressed)
{
    uint16_t xored = joydat ^ (uint16_t)(joydat >> 1);
    uint8_t v = 0;

    if (xored & 0x0100)
        v |= 0x01; /* up */
    if (xored & 0x0001)
        v |= 0x02; /* down */
    if (joydat & 0x0200)
        v |= 0x04; /* left */
    if (joydat & 0x0002)
        v |= 0x08; /* right */
    if (fire_pressed)
        v |= 0x10; /* JOY_BTN_1_MASK */

    return v;
}
