/* joydecode.c — pure JOYxDAT → JOY_* bitmask decode (T1 host-testable).
 *
 * JOYxDAT holds two quadrature counters (HRM "Joystick" decode):
 *   right = bit 1          left = bit 9
 *   down  = bit 0 ^ bit 1  up   = bit 8 ^ bit 9
 *
 * Output bit layout is the GK_JOY_* contract in gkinput.h:
 * up=0x01 down=0x02 left=0x04 right=0x08 btn1=0x10.
 */
#include "joydecode.h"
#include "gkinput.h"

uint8_t joyDecode(uint16_t joydat, uint8_t fire_pressed)
{
    uint16_t xored = joydat ^ (uint16_t)(joydat >> 1);
    uint8_t v = 0;

    if (xored & 0x0100)
        v |= GK_JOY_UP_MASK;
    if (xored & 0x0001)
        v |= GK_JOY_DOWN_MASK;
    if (joydat & 0x0200)
        v |= GK_JOY_LEFT_MASK;
    if (joydat & 0x0002)
        v |= GK_JOY_RIGHT_MASK;
    if (fire_pressed)
        v |= GK_JOY_BTN_1_MASK;

    return v;
}
