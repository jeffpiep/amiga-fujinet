/*
 * gfxsetup.c - the one definition of battleship's screen configuration
 *
 * Sole translation unit that pulls in tiles.h, so the art arrays exist once
 * in the binary. graphics.c and gallery_main.c reach them through
 * bs_gfx_config (gallery_main.c also walks tile_table directly — it is the
 * art preview harness).
 */
#include "gfxsetup.h"
#include "cellmap.h"   /* TILE_COUNT */
#include "tiles.h"
#include "amiga_vars.h"

const struct gfx_config bs_gfx_config = {
    320, 200, 4,               /* 320x200, 4 bitplanes = 16 colors */
    WIDTH, HEIGHT,             /* 40x25 cells of 8x8 px            */
    tile_palette,
    tile_table,
    TILE_COUNT,
    BS_CURSOR_SLOTS,
    2,                         /* first hardware sprite (0 = pointer) */
    CURSOR_SPR_HEIGHT,
    CURSOR_SPR_WORDS,
    cursor_spr_a,
    cursor_spr_b,
    cursor_spr_rgb,
    /* Park a cursor that has not moved for ~a blink period: the attack loop
     * repositions live cursors every <= 10 frames, and upstream silently
     * stops drawing the cursor on eliminated opponents' boards. */
    13
};
