/*
 * gfxcore.h - screen/window/tile plumbing for the graphical renderer
 *
 * Owns the 320x200x4 custom screen, the borderless backdrop window (also
 * the IDCMP keyboard source for input.c), the chip-RAM tile bank, the
 * attack-cursor hardware sprite, and the save/restore screen buffer.
 * graphics.c implements the upstream drawing contract on top of these
 * primitives; nothing else touches Intuition directly.
 *
 * Requires: intuition.library, graphics.library (V33 / KS 1.3)
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * See: docs/plan-track1b-battleship.md (Phase 3c)
 */
#ifndef GFXCORE_H
#define GFXCORE_H

#include <stdint.h>

struct Window;   /* forward decl; input.c includes intuition headers itself */

/* Pen numbers — colors come from tile_palette[] in tiles.h (art surface). */
#define PEN_BG       0   /* black background            */
#define PEN_TEXT     1   /* white text                  */
#define PEN_SEA      2   /* sea blue                    */
#define PEN_HIT      3   /* hit red                     */
#define PEN_SHIP     4   /* ship green                  */
#define PEN_TEXT_ALT 5   /* amber alt/emphasis text     */
#define PEN_DIM      6   /* grey inactive/dim           */
#define PEN_SEA_DK   7   /* dark blue sea detail        */
#define PEN_EXPL     8   /* orange explosion            */
#define PEN_CONN     9   /* cyan connection icon        */

/* Open libraries, screen, window, palette, tile bank, sprite. Returns 1 on
 * success. On failure everything already opened is released and 0 returns.
 * Registers gfx_close() via atexit(). */
uint8_t gfx_open(void);
void gfx_close(void);

/* Backdrop window — input.c reads IDCMP keyboard events from its UserPort. */
extern struct Window *gfx_window;

/* Blit tile id at cell (cx, cy). Out-of-grid cells are ignored (upstream
 * pokes a few cells past the last row, harmless on the Atari too). */
void gfx_draw_tile(uint8_t cx, uint8_t cy, uint8_t tile);

/* Render s at cell (cx, cy) in the given pens (JAM2: bg pen paints the
 * cell background, so text self-erases like a char-mapped display). */
void gfx_text(uint8_t cx, uint8_t cy, const char *s, uint8_t pen, uint8_t bpen);

/* Fill a w x h cell rectangle with a pen. */
void gfx_fill(uint8_t cx, uint8_t cy, uint8_t w, uint8_t h, uint8_t pen);

/* Attack cursor sprite. alt selects the blink image. Move parks it over a
 * cell; hide parks it below the visible raster. */
void gfx_cursor_move(uint8_t cx, uint8_t cy, uint8_t alt);
void gfx_cursor_hide(void);

/* Whole-screen save/restore for the in-game menu (spare chip bitmap). */
uint8_t gfx_save_screen(void);
void gfx_restore_screen(void);

/* Mouse-aim handshake between the renderer and input.c. graphics.c arms
 * attack mode from drawGamefieldCursor and placement mode from the
 * placement blink cycle in drawShip; both clear on redraws/updates.
 * input.c polls this each frame to decide whether mouse chasing is live,
 * which board it targets, and where the cursor/ship currently sits. */
#define GFX_AIM_NONE   0
#define GFX_AIM_ATTACK 1   /* chase target: enemy-field cell (attack cursor) */
#define GFX_AIM_PLACE  2   /* chase target: own-field cell (ship origin)     */

void gfx_aim_set(uint8_t player_count, uint8_t cell_x, uint8_t cell_y);
void gfx_aim_set_place(uint8_t player_count, uint8_t cell_x, uint8_t cell_y,
                       uint8_t ship_size, uint8_t ship_vertical);
void gfx_aim_clear(void);
/* Returns the GFX_AIM_* mode; out-params valid when non-NONE. */
uint8_t gfx_aim_get(uint8_t *player_count, uint8_t *cell_x, uint8_t *cell_y);
/* Mover ship shape (valid in GFX_AIM_PLACE mode). */
void gfx_aim_get_ship(uint8_t *ship_size, uint8_t *ship_vertical);

/* Blank (1) or restore (0) the Intuition mouse pointer over the window.
 * State-guarded: repeated calls with the same value are no-ops. */
void gfx_pointer_blank(uint8_t blank);

#endif /* GFXCORE_H */
