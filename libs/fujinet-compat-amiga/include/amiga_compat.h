/*
 * amiga_compat.h — bridge types needed to compile fujinet-fuji.h on Amiga.
 *
 * fujinet-fuji.h defines NewDisk only inside platform-specific #ifdefs
 * (ATARI, MSDOS, APPLE2, CMOC, CBM, PMD85, ADAM). None match m68k-amigaos.
 * Provide a minimal definition so the function prototype compiles.
 */

#ifndef AMIGA_COMPAT_H
#define AMIGA_COMPAT_H

#include <stdint.h>

/* Minimal NewDisk for Amiga — fuji_create_new() is a stub anyway. */
typedef struct {
    uint8_t  hostSlot;
    uint8_t  deviceSlot;
    uint8_t  dummy;
    char     filename[256];
} NewDisk;

#endif /* AMIGA_COMPAT_H */
