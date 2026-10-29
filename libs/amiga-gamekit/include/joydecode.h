#ifndef JOYDECODE_H
#define JOYDECODE_H

#include <stdint.h>

/* Decode a raw JOYxDAT counter word (+ fire state) into the GK_JOY_* bit
 * layout defined in gkinput.h. Pure logic — no AmigaOS — so it is T1
 * host-testable; the hardware register reads live in the game's input.c. */
uint8_t joyDecode(uint16_t joydat, uint8_t fire_pressed);

/* Same decode for the port-2 (game port) wiring specifically: takes the raw
 * CIAAPRA byte rather than a cooked fire flag, because the fire line is
 * active *low* there and getting that polarity backwards yields a stick
 * that fires continuously until pressed. Pure logic for the same reason —
 * gkjoy.c does the register reads. */
uint8_t joyDecodePort2(uint16_t joy1dat, uint8_t ciaapra);

#endif /* JOYDECODE_H */
