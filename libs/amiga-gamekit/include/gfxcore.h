/*
 * gfxcore.h - screen/window/tile plumbing for graphical Amiga game ports
 *
 * Owns a custom lores screen, a borderless backdrop window (also the IDCMP
 * keyboard source for the game's input layer), a chip-RAM tile bank, an
 * optional hardware-sprite overlay, and the save/restore screen buffer.
 * The game's graphics.c implements its upstream drawing contract on top of
 * these primitives; nothing else touches Intuition directly.
 *
 * Game-specific data — palette, tile art, sprite images, grid size — is
 * passed in as a struct gfx_config at open time. The gamekit owns no art
 * and no pen names; those live in the game (see apps/battleship/amiga).
 *
 * Requires: intuition.library, graphics.library (V33 / KS 1.3)
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * Extracted from apps/battleship/amiga in Track 1C Phase 0 —
 * see docs/plan-track1c-fujitzee.md.
 */
#ifndef GFXCORE_H
#define GFXCORE_H

#include <stdint.h>

struct Window;   /* forward decl; input.c includes intuition headers itself */

/* Cell grid unit. Fixed at the topaz-8 character box: every gfx_* call below
 * takes cell coordinates, and text must land on the same grid as the tiles. */
#define GFX_CELL_W 8
#define GFX_CELL_H 8

/* Most hardware sprites the overlay can drive (sprite 0 is the Intuition
 * pointer, so slots start at gfx_config.sprite_first). */
#define GFX_SPRITE_SLOTS_MAX 4

/*
 * Open-time configuration. The caller owns every pointer here and must keep
 * it alive until gfx_close(): image data is copied into chip RAM at open,
 * but the config itself is retained and read on every draw call.
 */
struct gfx_config {
    uint16_t screen_w;      /* 320 for lores                                */
    uint16_t screen_h;      /* 200 NTSC-safe                                */
    uint8_t  depth;         /* bitplanes; 4 = 16 colors                     */
    uint8_t  grid_w;        /* cells across = screen_w / GFX_CELL_W         */
    uint8_t  grid_h;        /* cells down   = screen_h / GFX_CELL_H         */

    const uint16_t *palette;             /* 1 << depth RGB4 words            */
    const uint16_t *const *tiles;        /* tile_count entries...            */
    uint8_t tile_count;                  /* ...each GFX_CELL_H * depth words */

    /* Optional hardware-sprite overlay (cursors, reticles). Set slots to 0
     * to skip it entirely — no sprites are allocated and gfx_sprite_* are
     * no-ops. Each slot gets its own chip-RAM copy of both images, because
     * MoveSprite() writes posctl words into the attached data. */
    uint8_t  sprite_slots;   /* 0..GFX_SPRITE_SLOTS_MAX                     */
    uint8_t  sprite_first;   /* hardware sprite number of slot 0 (>= 2)     */
    uint8_t  sprite_height;  /* scanlines                                   */
    uint8_t  sprite_words;   /* words per image: 2 + height * 2 + 2         */
    const uint16_t *sprite_img_a;   /* normal image                         */
    const uint16_t *sprite_img_b;   /* blink/alternate image                */
    const uint16_t *sprite_rgb;     /* 3 RGB4 words for the sprite bank     */
    /* Frames a slot may go unmoved before it is auto-parked off-screen.
     * 0 disables the sweep (slots stay put until moved or hidden). */
    uint8_t  sprite_park_age;
};

/* Open libraries, screen, window, palette, tile bank, sprites. Returns 1 on
 * success. On failure everything already opened is released and 0 returns.
 * Registers gfx_close() via atexit(). cfg must stay valid until close. */
uint8_t gfx_open(const struct gfx_config *cfg);
void gfx_close(void);

/* Backdrop window — the game's input.c reads IDCMP events from its UserPort. */
extern struct Window *gfx_window;

/* Blit tile id at cell (cx, cy). Out-of-grid cells are ignored (upstream
 * ports poke a few cells past the last row, harmless on the Atari too). */
void gfx_draw_tile(uint8_t cx, uint8_t cy, uint8_t tile);

/* Render s at cell (cx, cy) in the given pens (JAM2: bg pen paints the
 * cell background, so text self-erases like a char-mapped display). */
void gfx_text(uint8_t cx, uint8_t cy, const char *s, uint8_t pen, uint8_t bpen);

/*
 * Render s on row cy at an arbitrary pixel column, with a custom
 * per-character advance — for a string one glyph wider than the box it has
 * to go in. `span` pixels are cleared to bpen first and the glyphs are then
 * drawn transparently (JAM1) at px, px+advance, px+2*advance, …
 *
 * This is the one call that escapes the cell grid, and it exists because
 * the pixels either side of a box are usually not all spoken for: an 8 px
 * cell holding a 2 px rule has six free, and borrowing them is what lets a
 * number stay legible instead of being squeezed until its digits touch.
 * The caller picks px, span and advance to fit what is actually free;
 * nothing here clips to a cell boundary.
 */
void gfx_text_tight(uint16_t px, uint8_t cy, const char *s, uint8_t pen,
                    uint8_t bpen, uint8_t span, uint8_t advance);

/* Fill a w x h cell rectangle with a pen. */
void gfx_fill(uint8_t cx, uint8_t cy, uint8_t w, uint8_t h, uint8_t pen);

/* Sprite overlay. slot is 0..sprite_slots-1; alt selects the blink image.
 * Hide parks all slots below the visible raster. */
void gfx_sprite_move(uint8_t slot, uint8_t cx, uint8_t cy, uint8_t alt);
void gfx_sprite_hide(void);
/* Frame tick (call from the vsync wait): parks any slot that has not been
 * moved for sprite_park_age frames — a game that silently stops drawing a
 * cursor gets it removed instead of leaving it stuck on screen. */
void gfx_sprite_sweep(void);

/* Whole-screen save/restore for in-game menus (spare chip bitmap). */
uint8_t gfx_save_screen(void);
void gfx_restore_screen(void);

/* Blank (1) or restore (0) the Intuition mouse pointer over the window.
 * State-guarded: repeated calls with the same value are no-ops. */
void gfx_pointer_blank(uint8_t blank);

#endif /* GFXCORE_H */
