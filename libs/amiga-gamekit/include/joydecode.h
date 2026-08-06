#ifndef JOYDECODE_H
#define JOYDECODE_H

#include <stdint.h>

/* Decode a raw JOYxDAT counter word (+ fire state) into the GK_JOY_* bit
 * layout defined in gkinput.h. Pure logic — no AmigaOS — so it is T1
 * host-testable; the hardware register reads live in the game's input.c. */
uint8_t joyDecode(uint16_t joydat, uint8_t fire_pressed);

#endif /* JOYDECODE_H */
