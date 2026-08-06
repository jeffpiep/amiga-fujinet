/*
 * gallery_main.c - tile / palette / sprite preview harness (art pass)
 *
 * Standalone boot-to-grid preview: links the gamekit + tiles.h WITHOUT the
 * upstream game, opens the same 320x200x4 screen the renderer uses, and draws
 * every tile MAGNIFIED (each 8x8 tile blown up 3x via RectFill so pixels are
 * legible), the 16-entry palette, and the attack-cursor sprite. One edit to
 * tiles.h is a single `rebuild -> look` loop. Any key exits.
 *
 * Build:  make tilegallery
 * Boot :  make gallery-adf   then boot tilegallery.adf in FS-UAE
 *
 * See: docs/plan-track1b-battleship.md (Phase 3c art pass)
 */
#include <exec/types.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>

#include <proto/exec.h>
#include <proto/graphics.h>

#include "gfxcore.h"
#include "gfxsetup.h"     /* bs_gfx_config */
#include "pens.h"
#include "cellmap.h"      /* TILE_COUNT   */
#include "tiles.h"        /* tile_table[] */
#include "amiga_vars.h"

#define ZOOM 3            /* magnify each 8x8 tile to 24x24 px */

/* Two-digit decimal (0..99) into a 3-byte buffer. */
static void num2(char *b, uint8_t n)
{
    b[0] = (char)('0' + (n / 10) % 10);
    b[1] = (char)('0' + n % 10);
    b[2] = '\0';
}

/* Decode one pixel's pen (0..15) from a 32-word / 4-plane tile. Leftmost
 * pixel is bit 15 (tiles are 8 wide, data in the high byte). */
static uint8_t tile_pixel(const uint16_t *t, int col, int row)
{
    uint8_t pen = 0;
    int pl;
    for (pl = 0; pl < 4; pl++)
        if ((t[pl * 8 + row] >> (15 - col)) & 1)
            pen = (uint8_t)(pen | (1 << pl));
    return pen;
}

/* Blit a tile magnified `z`x at pixel (px, py) using solid pen rects. */
static void draw_tile_z(const uint16_t *t, int px, int py, int z)
{
    struct RastPort *rp = gfx_window->RPort;
    int row, col;
    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            int x = px + col * z;
            int y = py + row * z;
            SetAPen(rp, tile_pixel(t, col, row));
            RectFill(rp, x, y, x + z - 1, y + z - 1);
        }
    }
}

/* Block until a key press on the backdrop window's IDCMP port. */
static void wait_key(void)
{
    struct MsgPort *port = gfx_window->UserPort;
    struct IntuiMessage *msg;

    for (;;) {
        Wait(1UL << port->mp_SigBit);
        while ((msg = (struct IntuiMessage *)GetMsg(port)) != NULL) {
            ULONG cls = msg->Class;
            ReplyMsg((struct Message *)msg);
            if (cls == VANILLAKEY || cls == RAWKEY)
                return;
        }
    }
}

int main(void)
{
    uint8_t id;
    char lbl[3];

    if (!gfx_open(&bs_gfx_config))
        return 20;

    gfx_text(1, 0, "TILE GALLERY (3x) - press any key", PEN_TEXT, PEN_BG);

    /* Palette strip: pens 0..15 as 2-wide swatches with index labels. */
    for (id = 0; id < 16; id++) {
        gfx_fill((uint8_t)(1 + id * 2), 2, 2, 1, id);
        num2(lbl, id);
        gfx_text((uint8_t)(1 + id * 2), 3, lbl, PEN_TEXT, PEN_BG);
    }

    /* Tile grid: 7 per row in a 4-cell-wide block (3x tile + id label); the
     * right margin (cols 28-39) is reserved for the tiled sea preview. */
    for (id = 0; id < TILE_COUNT; id++) {
        uint8_t cell_x = (uint8_t)((id % 7) * 4);
        uint8_t cell_y = (uint8_t)(5 + (id / 7) * 4);
        draw_tile_z(tile_table[id], cell_x * 8, cell_y * 8, ZOOM);
        num2(lbl, id);
        gfx_text(cell_x, (uint8_t)(cell_y + 3), lbl, PEN_TEXT_ALT, PEN_BG);
    }

    /* Sea + ships preview: a 4x4 water matrix with an assembled horizontal
     * ship (bow-mid-stern) along the bottom row and a vertical ship down the
     * right column, so the tile joints and the ship-on-water look can be
     * judged in context. Drawn 3x in the right margin. */
    {
        static const uint8_t layout[4][4] = {
            { TILE_SEA,         TILE_SEA,         TILE_SEA,           TILE_SHIP_BOW_V   },
            { TILE_SEA,         TILE_SEA,         TILE_SEA,           TILE_SHIP_MID_V   },
            { TILE_SEA,         TILE_SEA,         TILE_SEA,           TILE_SHIP_STERN_V },
            { TILE_SHIP_BOW_H,  TILE_SHIP_MID_H,  TILE_SHIP_STERN_H,  TILE_SEA          },
        };
        int r, c;
        const int z = 3, ox = 28 * 8, oy = 6 * 8;
        gfx_text(28, 5, "SHIPS+SEA", PEN_TEXT, PEN_BG);
        for (r = 0; r < 4; r++)
            for (c = 0; c < 4; c++)
                draw_tile_z(tile_table[layout[r][c]], ox + c * 8 * z, oy + r * 8 * z, z);
    }

    /* Attack-cursor sprite sample (drawn at native size), bottom-left. */
    gfx_sprite_move(0, 2, 23, 0);

    wait_key();
    return 0;
}
