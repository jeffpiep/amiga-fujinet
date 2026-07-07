#ifndef JOYDECODE_H
#define JOYDECODE_H

#include <stdint.h>

/* Decode a raw JOYxDAT counter word (+ fire state) into the JOY_* bit
 * layout defined in amiga_vars.h. Pure logic — no AmigaOS — so it is T1
 * host-testable; the hardware register reads live in input.c. */
uint8_t joyDecode(uint16_t joydat, uint8_t fire_pressed);

#endif /* JOYDECODE_H */
