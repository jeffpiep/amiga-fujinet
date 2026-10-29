/*
 * gfxsetup.h - battleship's gfx_config for the shared gamekit screen
 *
 * The gamekit's gfx_open() takes screen geometry, palette, tile art and the
 * sprite overlay as data (libs/amiga-gamekit/include/gfxcore.h). This is
 * battleship's instance of that, defined once in gfxsetup.c so both the game
 * and the tile-gallery harness open the identical screen.
 */
#ifndef GFXSETUP_H
#define GFXSETUP_H

#include "gfxcore.h"

/* Sprite slots: one attack cursor per possible enemy board, so the reticle
 * shows on every surviving opponent in 3-4 player games (upstream draws it
 * once per enemy quadrant each frame). Slot = quadrant - 1. */
#define BS_CURSOR_SLOTS 3

extern const struct gfx_config bs_gfx_config;

#endif /* GFXSETUP_H */
