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

#endif /* GFXCORE_H */
