/*
 * graphics.c - upstream drawing contract on the graphical tile renderer
 *
 * Phase 3c: implements upstream/src/platform-specific/graphics.h on a
 * 320x200x4 custom screen as a 40x25 grid of 8x8 tiles. Layout math lives
 * in cellmap.c (host-tested), OS plumbing in gfxcore.c, art in tiles.h.
 * The layout mirrors the Atari renderer (upstream/src/atari/graphics.c).
 *
 * Requires: intuition.library, graphics.library (V33 / KS 1.3)
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * See: docs/plan-track1b-battleship.md (Phase 3c)
 */
#include "misc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/graphics.h>   /* WaitTOF */

#undef FN_ERR_UNKNOWN
#include "fujinet-nio.h"

#include "gfxcore.h"
#include "cellmap.h"

#if FIELD_ATTACK != CM_FIELD_ATTACK || FIELD_MISS != CM_FIELD_MISS
#error "cellmap.h field codes out of sync with upstream misc.h"
#endif

extern char playerName[12];

static uint8_t playerCount_g = 2;

void initGraphics(void)
{
    if (!gfx_open()) {
        /* nix13 binds stdout to the boot CLI, so this lands somewhere
         * visible even though our screen never opened. */
        printf("battleship: can't open screen (chip RAM?)\n");
        exit(1);
    }

    /* Open serial.device and set baud = 19200 before game logic starts. */
    fn_init();

    if (playerName[0] == '\0')
        strncpy(playerName, "amiga", 11);

    if (!prefs.seenHelp)
        prefs.seenHelp = 1;
}

void resetGraphics(void) {}   /* teardown runs via gfx_close atexit */

void resetScreen(void)
{
    gfx_cursor_hide();
    gfx_fill(0, 0, WIDTH, HEIGHT, PEN_BG);
}

void waitvsync(void)
{
    WaitTOF();
}

uint8_t cycleNextColor(void) { return 0; }

void drawText(uint8_t x, uint8_t y, const char *s)
{
    gfx_text(x, y, s, PEN_TEXT, PEN_BG);
}

void drawTextAlt(uint8_t x, uint8_t y, const char *s)
{
    gfx_text(x, y, s, PEN_TEXT_ALT, PEN_BG);
}

void drawIcon(uint8_t x, uint8_t y, uint8_t icon)
{
    char buf[2];

    buf[0] = (char)icon;
    buf[1] = '\0';
    gfx_text(x, y, buf, PEN_TEXT_ALT, PEN_BG);
}

void drawBlank(uint8_t x, uint8_t y)
{
    gfx_draw_tile(x, y, TILE_BLANK);
}

void drawSpace(uint8_t x, uint8_t y, uint8_t w)
{
    gfx_fill(x, y, w, 1, PEN_BG);
}

void drawLine(uint8_t x, uint8_t y, uint8_t w)
{
    uint8_t i;

    for (i = 0; i < w; i++)
        gfx_draw_tile(x + i, y, TILE_LINE);
}

void drawBox(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t i;

    gfx_draw_tile(x, y, TILE_BOX_TL);
    gfx_draw_tile(x + w + 1, y, TILE_BOX_TR);
    for (i = 1; i <= w; i++) {
        gfx_draw_tile(x + i, y, TILE_BORDER_H);
        gfx_draw_tile(x + i, y + h + 1, TILE_BORDER_H);
    }
    for (i = 1; i <= h; i++) {
        gfx_draw_tile(x, y + i, TILE_BORDER_V);
        gfx_draw_tile(x + w + 1, y + i, TILE_BORDER_V);
    }
    gfx_draw_tile(x, y + h + 1, TILE_BOX_BL);
    gfx_draw_tile(x + w + 1, y + h + 1, TILE_BOX_BR);
}

void drawBoard(uint8_t playerCount)
{
    uint8_t q, row, ox, oy;

    playerCount_g = playerCount;
    resetScreen();

    for (q = 0; q < playerCount && q < 4; q++) {
        /* 10x10 sea grid */
        cm_quadrant_origin(playerCount_g, q, &ox, &oy);
        for (row = 0; row < 10; row++) {
            uint8_t col;

            for (col = 0; col < 10; col++)
                gfx_draw_tile(ox + col, oy + row, TILE_SEA);
        }

        /* 3x8 sea drawer for the ship legend */
        cm_drawer_origin(playerCount_g, q, &ox, &oy);
        for (row = 0; row < 8; row++) {
            gfx_draw_tile(ox,     oy + row, TILE_SEA);
            gfx_draw_tile(ox + 1, oy + row, TILE_SEA);
            gfx_draw_tile(ox + 2, oy + row, TILE_SEA);
        }
    }
}

void drawClock(void)
{
    gfx_draw_tile(WIDTH - 1, HEIGHT - 1, TILE_CLOCK);
}

void drawConnectionIcon(bool show)
{
    gfx_draw_tile(0, HEIGHT - 1, show ? TILE_CONN_ON : TILE_CONN_OFF);
}

void drawEndgameMessage(const char *msg)
{
    uint8_t len = (uint8_t)strlen(msg);
    uint8_t x   = (uint8_t)((WIDTH - len) / 2);
    uint8_t i;

    for (i = 0; i < WIDTH; i++)
        gfx_draw_tile(i, HEIGHT - 2, TILE_ENDGAME_BAR);
    gfx_fill(0, HEIGHT - 1, WIDTH, 1, PEN_BG);
    gfx_text(x, HEIGHT - 1, msg, PEN_TEXT, PEN_BG);
}

/*
 * Name label: above the grid for top boards (quadrants 1, 2), below it for
 * bottom boards (0, 3) — the Atari arrangement. Active player gets bright
 * bracketed text, inactive players dim text.
 */
void drawPlayerName(uint8_t player, const char *name, bool active)
{
    char buf[16];
    uint8_t ox, oy, y;

    cm_quadrant_origin(playerCount_g, player, &ox, &oy);
    y = (player == 1 || player == 2) ? (uint8_t)(oy - 1) : (uint8_t)(oy + 10);

    snprintf(buf, sizeof(buf), active ? "[%s]" : " %s ", name);
    gfx_fill(ox, y, 12, 1, PEN_BG);
    gfx_text(ox, y, buf, active ? PEN_TEXT_ALT : PEN_DIM, PEN_BG);
}

void drawShip(uint8_t quadrant, uint8_t size, uint8_t pos, bool hide)
{
    uint8_t ox, oy, sx, sy, i, vertical = 0;

    if (pos > 99) {
        vertical = 1;
        pos -= 100;
    }
    cm_quadrant_origin(playerCount_g, quadrant, &ox, &oy);
    sx = pos % 10;
    sy = pos / 10;

    for (i = 0; i < size; i++) {
        uint8_t tile = hide ? TILE_SEA : cm_ship_tile(i, size, vertical);

        if (vertical)
            gfx_draw_tile(ox + sx, oy + sy + i, tile);
        else
            gfx_draw_tile(ox + sx + i, oy + sy, tile);
    }
}

void drawLegendShip(uint8_t player, uint8_t index, uint8_t size, uint8_t status)
{
    uint8_t ox, oy, dx, dy, i;

    cm_drawer_origin(playerCount_g, player, &ox, &oy);
    cm_legend_offset(index, &dx, &dy);
    ox += dx;
    oy += dy;

    for (i = 0; i < size; i++)
        gfx_draw_tile(ox, oy + i,
                      status ? cm_ship_tile(i, size, 1) : TILE_LEGEND_HIT);
}

/*
 * Overlay hits/misses only. Empty cells are left untouched so the sea grid
 * painted by drawBoard and any ships drawn by drawShip survive (the Atari
 * renderer does the same).
 */
void drawGamefield(uint8_t quadrant, uint8_t *field)
{
    uint8_t ox, oy, row, col;

    cm_quadrant_origin(playerCount_g, quadrant, &ox, &oy);
    for (row = 0; row < 10; row++) {
        for (col = 0; col < 10; col++) {
            uint8_t cell = field[row * 10 + col];

            if (cell != FIELD_ATTACK && cell != FIELD_MISS)
                continue;
            gfx_draw_tile(ox + col, oy + row, cm_field_tile(cell));
        }
    }
}

void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield,
                         uint8_t attackPos, uint8_t anim)
{
    uint8_t ox, oy;

    gfx_cursor_hide();
    cm_quadrant_origin(playerCount_g, quadrant, &ox, &oy);
    gfx_draw_tile(ox + attackPos % 10, oy + attackPos / 10,
                  cm_update_tile(gamefield[attackPos], anim));
}

void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y,
                         uint8_t *gamefield, uint8_t blink)
{
    uint8_t ox, oy;

    (void)gamefield;
    cm_quadrant_origin(playerCount_g, quadrant, &ox, &oy);
    /* blink is a 0..2 frame index — flip to the alternate sprite image on
     * the last phase for a soft two-tone pulse. */
    gfx_cursor_move(ox + x, oy + y, (uint8_t)(blink >= 2));
}

bool saveScreenBuffer(void)
{
    return gfx_save_screen() != 0;
}

void restoreScreenBuffer(void)
{
    gfx_restore_screen();
}
