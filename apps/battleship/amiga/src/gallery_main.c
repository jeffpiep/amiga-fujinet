/*
 * gallery_main.c - tile / palette / sprite preview harness (art pass)
 *
 * Standalone boot-to-grid preview: links gfxcore.c + tiles.h WITHOUT the
 * upstream game, opens the same 320x200x4 screen the renderer uses, and draws
 * every tile, the 16-entry palette, and the attack-cursor sprite at once so an
 * edit to tiles.h is a one-loop `rebuild -> look`. Any key exits.
 *
 * Build:  make tilegallery
 * Boot :  make gallery-adf   then boot tilegallery.adf in FS-UAE
 *
 * See: docs/plan-track1b-battleship.md (Phase 3c art pass)
 */
#include <exec/types.h>
#include <exec/ports.h>
#include <intuition/intuition.h>

#include <proto/exec.h>

#include "gfxcore.h"
#include "cellmap.h"      /* TILE_COUNT */
#include "amiga_vars.h"

/* Two-digit decimal (0..99) into a 3-byte buffer. */
static void num2(char *b, uint8_t n)
{
    b[0] = (char)('0' + (n / 10) % 10);
    b[1] = (char)('0' + n % 10);
    b[2] = '\0';
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

    if (!gfx_open())
        return 20;

    gfx_text(1, 0, "TILE GALLERY - press any key", PEN_TEXT, PEN_BG);

    /* Palette strip: pens 0..15 as 2-wide swatches with index labels. */
    for (id = 0; id < 16; id++) {
        gfx_fill((uint8_t)(1 + id * 2), 2, 2, 1, id);
        num2(lbl, id);
        gfx_text((uint8_t)(1 + id * 2), 3, lbl, PEN_TEXT, PEN_BG);
    }

    /* Tile grid: 8 per row, each in a 5x3 cell block (tile + id label). */
    for (id = 0; id < TILE_COUNT; id++) {
        uint8_t col = (uint8_t)(1 + (id % 8) * 5);
        uint8_t row = (uint8_t)(5 + (id / 8) * 3);
        gfx_draw_tile(col, row, id);
        num2(lbl, id);
        gfx_text(col, (uint8_t)(row + 1), lbl, PEN_TEXT_ALT, PEN_BG);
    }

    /* Attack-cursor sprite sample, bottom-left. */
    gfx_cursor_move(0, 2, 23, 0);

    wait_key();
    return 0;
}
