/*
 * gfxsetup.c - the one definition of fujitzee's screen configuration
 *
 * Sole translation unit that pulls in tiles.h, so the art arrays exist once
 * in the binary; graphics.c reaches them through fj_gfx_config.
 *
 * No hardware sprites: fujitzee's two cursors (score row, dice) sit on the
 * cell grid and are drawn as tiles, unlike battleship's free-moving attack
 * reticle. Leaving sprite_slots at 0 means the gamekit allocates none.
 */
#include "gfxsetup.h"
#include "fjlayout.h"
#include "tiles.h"

const struct gfx_config fj_gfx_config = {
    320, 200, 4,                 /* 320x200, 4 bitplanes = 16 colors */
    FJ_WIDTH, FJ_HEIGHT,         /* 40x25 cells of 8x8 px            */
    tile_palette,
    tile_table,
    TILE_COUNT,
    0,                           /* sprite_slots — none              */
    2, 0, 0, 0, 0, 0, 0          /* remaining sprite fields unused   */
};
