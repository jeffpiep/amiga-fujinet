/*
 * tiles.h - battleship tile art, palette and cursor sprite (data only)
 *
 * THE art-pass surface: every visual in the graphical renderer is defined
 * here — swap the arrays, keep the names, and no engine code changes.
 * Include from gfxcore.c ONLY (defines data, not just declarations).
 *
 * The art below is written with the TILE_PAT (2-color) and TILE_MC
 * (multicolor) composers from libs/amiga-gamekit/include/tilepat.h, which
 * also documents the 32-word tile format they expand to. Any entry can
 * equally be a hand- or tool-generated 32-word array.
 *
 * See: contracts/amiga-adf-bootstrap.md, docs/plan-track1b-battleship.md
 */
#ifndef TILES_H
#define TILES_H

#include <stdint.h>
#include "cellmap.h"   /* TILE_* ids — tile_table[] is indexed by them */
#include "pens.h"      /* PEN_* numbers the palette below gives color to */

/* ---- Palette (12-bit RGB4, LoadRGB4 order, indexed by PEN_*) ---- */

static const uint16_t tile_palette[16] = {
    /*0 BG  1 TEXT 2 SEA  3 HIT  4 SHIP 5 ALT  6 DIM  7 SEA_DK */
     0x000, 0xFFF, 0x05A, 0xD22, 0x888, 0xFC3, 0x888, 0x038,
    /* 8 EXPL 9 CONN 10 SHIP_HI 11 SHIP_SHD 12 SEA_LT 13 FOAM 14 WOOD 15 - */
       0xF80, 0x0DE, 0xBBB,     0x444,      0x4BE,    0xABF,  0x963,  0x000
};

/* ---- Tile composers ----
 * TILE_PAT (2-color from a bit pattern) and TILE_MC (multicolor from a
 * grid of pen numbers) live in the gamekit so both ports author art the
 * same way — see libs/amiga-gamekit/include/tilepat.h for the format and
 * a worked example. */
#include "tilepat.h"


/* ---- Placeholder tiles ---- */

static const uint16_t tile_blank[32] = TILE_PAT(PEN_BG, PEN_BG,
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00); /* 00000000 */

#define S PEN_SEA
#define D PEN_SEA_DK
#define F PEN_FOAM
static const uint16_t tile_sea[32] = TILE_MC(
    D, D, D, D, D, D, D, D,     //row 0: crest at top-left, trough at right
    D, S, S, S, S, S, S, S,     //rows 1-6: foam streak runs down-right,
    D, S, S, S, S, S, S, S,    //            the darker trough mirrors it
    D, S, F, S, S, S, S, S,
    D, S, S, S, S, S, S, S,
    D, S, S, S, S, F, S, S,
    D, S, S, S, S, S, S, S,
    D, S, S, S, S, S, S, S);    //row 7
#undef F

#define T PEN_TEXT
static const uint16_t tile_miss[32] = TILE_MC(
    D, D, D, D, D, D, D, D,     
    D, S, S, S, S, S, S, S,     
    D, S, S, T, T, S, S, S,    
    D, S, T, T, T, T, S, S,
    D, S, T, T, T, T, S, S,
    D, S, S, T, T, S, S, S,
    D, S, S, S, S, S, S, S,
    D, S, S, S, S, S, S, S); 
#undef T
#undef S
#undef D


/* Enemy-board hit (red) and its blink partner (white): a clean X in the 7x7
 * interior, under the PEN_SEA_DK top/left border, on sea. Same silhouette. */
#define D PEN_SEA_DK
#define S PEN_SEA
#define B PEN_HIT
#define T PEN_TEXT
static const uint16_t tile_hit[32] = TILE_MC(
    D, D, D, D, D, D, D, D,
    D, B, S, S, S, S, S, B,
    D, S, B, S, S, S, B, S,
    D, S, S, B, S, B, S, S,
    D, S, S, S, B, S, S, S,
    D, S, S, B, S, B, S, S,
    D, S, B, S, S, S, B, S,
    D, B, S, S, S, S, S, B);

static const uint16_t tile_hit2[32] = TILE_MC(
    D, D, D, D, D, D, D, D,
    D, T, S, S, S, S, S, T,
    D, S, T, S, S, S, T, S,
    D, S, S, T, S, T, S, S,
    D, S, S, S, T, S, S, S,
    D, S, S, T, S, T, S, S,
    D, S, T, S, S, S, T, S,
    D, T, S, S, S, S, S, T);
#undef D
#undef S
#undef B
#undef T

/* Hit on one of your own ships: red X on the gray hull (distinct from
 * tile_hit's red X on sea). See docs/archive/handoff-own-ship-hit-tile.md. */
#define W PEN_SEA
#define D PEN_SEA_DK
#define S PEN_SHIP
// #define H PEN_SHIP_HI
// #define L PEN_SHIP_SHD
#define B PEN_HIT
#define F PEN_EXPL
static const uint16_t tile_hit_ship[32] = TILE_MC(
    D, D, D, D, D, D, D, D,
    D, B, S, B, F, S, B, W,
    D, S, B, B, F, B, S, W,
    D, F, F, B, B, B, B, W,
    D, B, B, B, B, F, F, W,
    D, S, B, F, B, B, S, W,
    D, B, S, F, B, S, B, W,
    D, W, W, W, W, W, W, W);

static const uint16_t tile_legend_hit[32] = TILE_MC(
    B, D, D, D, D, D, D, B,
    D, B, W, W, W, W, B, W,
    D, W, B, W, F, B, W, W,
    D, W, F, B, B, W, W, W,
    D, W, W, B, B, F, W, W,
    D, W, B, F, W, B, W, W,
    D, B, W, W, W, W, B, W,
    B, W, W, W, W, W, W, B);
#undef F

// #define W PEN_SEA
// #define D PEN_SEA_DK
// #define S PEN_SHIP
#define H PEN_SHIP_HI
#define L PEN_SHIP_SHD

static const uint16_t tile_ship_mid_h[32] = TILE_MC(
    D, D, D, D, D, D, D, D,
    L, L, L, L, L, L, L, L,
    S, S, S, S, S, S, S, S,
    S, S, S, L, L, S, S, S,
    S, S, S, L, L, S, S, S,
    S, S, S, S, S, S, S, S,
    L, L, L, L, L, L, L, L,
    D, W, W, W, W, W, W, W);

static const uint16_t tile_ship_stern_h[32] = TILE_MC(
    D, D, D, D, D, D, D, D,
    L, L, L, L, L, L, L, W,
    S, S, S, S, S, S, S, L,
    S, S, S, L, L, S, S, L,
    S, S, S, L, L, S, S, L,
    S, S, S, S, S, S, S, L,
    L, L, L, L, L, L, L, W,
    D, W, W, W, W, W, W, W);

static const uint16_t tile_ship_bow_h[32] = TILE_MC(
    D, D, D, D, D, D, D, D,
    D, W, L, L, L, L, L, L,
    D, L, S, S, S, S, S, S,
    L, S, S, L, L, S, S, S,
    L, S, S, L, L, S, S, S,
    D, L, S, S, S, S, S, S,
    D, W, L, L, L, L, L, L,
    D, W, W, W, W, W, W, W);

static const uint16_t tile_ship_bow_v[32] = TILE_MC(
    D, D, D, L, L, D, D, D,
    D, W, L, S, S, L, W, W,
    D, L, S, S, S, S, L, W,
    D, L, S, L, L, S, L, W,
    D, L, S, L, L, S, L, W,
    D, L, S, S, S, S, L, W,
    D, L, S, S, S, S, L, W,
    D, L, S, S, S, S, L, W);

static const uint16_t tile_ship_mid_v[32] = TILE_MC(
    D, L, S, S, S, S, L, D,
    D, L, S, S, S, S, L, W,
    D, L, S, S, S, S, L, W,
    D, L, S, L, L, S, L, W,
    D, L, S, L, L, S, L, W,
    D, L, S, S, S, S, L, W,
    D, L, S, S, S, S, L, W,
    D, L, S, S, S, S, L, W);

static const uint16_t tile_ship_stern_v[32] = TILE_MC(
    D, L, S, S, S, S, L, D,
    D, L, S, S, S, S, L, W,
    D, L, S, S, S, S, L, W,
    D, L, S, L, L, S, L, W,
    D, L, S, L, L, S, L, W,
    D, L, S, S, S, S, L, W,
    D, L, S, S, S, S, L, W,
    D, W, L, L, L, L, W, W);

#undef W
#undef D
#undef S
#undef H
#undef L
#undef B

/* Attack animation: expanding blast, then dissipating ring. Re-fit into the
 * 7x7 interior under the PEN_SEA_DK top/left border, matching the marker tiles.
 * E = PEN_EXPL blast on S = PEN_SEA water. */
#define D PEN_SEA_DK
#define S PEN_SEA
#define E PEN_EXPL
static const uint16_t tile_anim_0[32] = TILE_MC(   /* 2x2 spark */
    D, D, D, D, D, D, D, D,
    D, S, S, S, S, S, S, S,
    D, S, S, S, S, S, S, S,
    D, S, S, E, E, S, S, S,
    D, S, S, E, E, S, S, S,
    D, S, S, S, S, S, S, S,
    D, S, S, S, S, S, S, S,
    D, S, S, S, S, S, S, S);

static const uint16_t tile_anim_1[32] = TILE_MC(   /* 4x4 block */
    D, D, D, D, D, D, D, D,
    D, S, S, S, S, S, S, S,
    D, S, S, E, E, S, S, S,
    D, S, E, E, E, E, S, S,
    D, S, E, E, E, E, S, S,
    D, S, S, E, E, S, S, S,
    D, S, S, S, S, S, S, S,
    D, S, S, S, S, S, S, S);

static const uint16_t tile_anim_2[32] = TILE_MC(   /* rounded blob */
    D, D, D, D, D, D, D, D,
    D, S, E, E, E, E, S, S,
    D, E, E, E, E, E, E, S,
    D, E, E, E, E, E, E, S,
    D, E, E, E, E, E, E, S,
    D, E, E, E, E, E, E, S,
    D, S, E, E, E, E, S, S,
    D, S, S, S, S, S, S, S);

static const uint16_t tile_anim_3[32] = TILE_MC(   /* near-full blast */
    D, E, E, E, E, E, E, D,
    E, E, E, E, E, E, E, E,
    E, E, E, E, E, E, E, E,
    E, E, E, E, E, E, E, E,
    E, E, E, E, E, E, E, E,
    E, E, E, E, E, E, E, E,
    E, E, E, E, E, E, E, E,
    D, E, E, E, E, E, E, S);

static const uint16_t tile_anim_4[32] = TILE_MC(   /* dissipating ring */
    D, E, E, E, E, E, E, D,
    E, E, E, E, E, E, E, E,
    E, E, S, S, S, S, E, E,
    E, E, S, S, S, S, E, E,
    E, E, S, S, S, S, E, E,
    E, E, S, S, S, S, E, E,
    E, E, E, E, E, E, E, E,
    D, E, E, E, E, E, E, S);

static const uint16_t tile_anim_5[32] = TILE_MC(   /* corner remnants */
    E, E, S, S, S, S, E, E,
    E, E, S, S, S, S, E, E,
    S, S, S, S, S, S, S, S,
    S, S, S, S, S, S, S, S,
    S, S, S, S, S, S, S, S,
    S, S, S, S, S, S, S, S,
    E, E, S, S, S, S, E, E,
    E, E, S, S, S, S, E, E);
#undef D
#undef S
#undef E

static const uint16_t tile_border_h[32] = TILE_PAT(PEN_TEXT, PEN_BG,
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0xFF,  /* 11111111 */
    0xFF,  /* 11111111 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00); /* 00000000 */

static const uint16_t tile_border_v[32] = TILE_PAT(PEN_TEXT, PEN_BG,
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x18); /* 00011000 */

static const uint16_t tile_box_tl[32] = TILE_PAT(PEN_TEXT, PEN_BG,
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x1F,  /* 00011111 */
    0x1F,  /* 00011111 */
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x18); /* 00011000 */

static const uint16_t tile_box_tr[32] = TILE_PAT(PEN_TEXT, PEN_BG,
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0xF8,  /* 11111000 */
    0xF8,  /* 11111000 */
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x18); /* 00011000 */

static const uint16_t tile_box_bl[32] = TILE_PAT(PEN_TEXT, PEN_BG,
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x1F,  /* 00011111 */
    0x1F,  /* 00011111 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00); /* 00000000 */

static const uint16_t tile_box_br[32] = TILE_PAT(PEN_TEXT, PEN_BG,
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0xF8,  /* 11111000 */
    0xF8,  /* 11111000 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00); /* 00000000 */

static const uint16_t tile_line[32] = TILE_PAT(PEN_DIM, PEN_BG,
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0xFF,  /* 11111111 */
    0xFF,  /* 11111111 */
    0x00,  /* 00000000 */
    0x00,  /* 00000000 */
    0x00); /* 00000000 */

static const uint16_t tile_endgame_bar[32] = TILE_PAT(PEN_DIM, PEN_BG,
    0x00,  /* 00000000 */
    0xFF,  /* 11111111 */
    0xFF,  /* 11111111 */
    0xFF,  /* 11111111 */
    0xFF,  /* 11111111 */
    0xFF,  /* 11111111 */
    0xFF,  /* 11111111 */
    0x00); /* 00000000 */

static const uint16_t tile_clock[32] = TILE_PAT(PEN_TEXT, PEN_BG,
    0x3C,  /* 00111100 */
    0x42,  /* 01000010 */
    0x99,  /* 10011001 */
    0xBD,  /* 10111101 */
    0x85,  /* 10000101 */
    0x81,  /* 10000001 */
    0x42,  /* 01000010 */
    0x3C); /* 00111100 */

static const uint16_t tile_conn_on[32] = TILE_PAT(PEN_CONN, PEN_BG,
    0x18,  /* 00011000 */
    0x3C,  /* 00111100 */
    0x7E,  /* 01111110 */
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x7E,  /* 01111110 */
    0x3C,  /* 00111100 */
    0x18); /* 00011000 */

static const uint16_t tile_conn_off[32] = TILE_PAT(PEN_DIM, PEN_BG,
    0x18,  /* 00011000 */
    0x3C,  /* 00111100 */
    0x7E,  /* 01111110 */
    0x18,  /* 00011000 */
    0x18,  /* 00011000 */
    0x7E,  /* 01111110 */
    0x3C,  /* 00111100 */
    0x18); /* 00011000 */

/* ---- Tile table (indexed by the TILE_* enum in cellmap.h) ---- */

static const uint16_t *const tile_table[TILE_COUNT] = {
    tile_blank,        /* TILE_BLANK        */
    tile_sea,          /* TILE_SEA          */
    tile_miss,         /* TILE_MISS         */
    tile_hit,          /* TILE_HIT          */
    tile_hit2,         /* TILE_HIT2         */
    tile_legend_hit,   /* TILE_LEGEND_HIT   */
    tile_ship_bow_h,   /* TILE_SHIP_BOW_H   */
    tile_ship_mid_h,   /* TILE_SHIP_MID_H   */
    tile_ship_stern_h, /* TILE_SHIP_STERN_H */
    tile_ship_bow_v,   /* TILE_SHIP_BOW_V   */
    tile_ship_mid_v,   /* TILE_SHIP_MID_V   */
    tile_ship_stern_v, /* TILE_SHIP_STERN_V */
    tile_anim_0,       /* TILE_ANIM_0       */
    tile_anim_1,       /* TILE_ANIM_1       */
    tile_anim_2,       /* TILE_ANIM_2       */
    tile_anim_3,       /* TILE_ANIM_3       */
    tile_anim_4,       /* TILE_ANIM_4       */
    tile_anim_5,       /* TILE_ANIM_5       */
    tile_border_h,     /* TILE_BORDER_H     */
    tile_border_v,     /* TILE_BORDER_V     */
    tile_box_tl,       /* TILE_BOX_TL       */
    tile_box_tr,       /* TILE_BOX_TR       */
    tile_box_bl,       /* TILE_BOX_BL       */
    tile_box_br,       /* TILE_BOX_BR       */
    tile_line,         /* TILE_LINE         */
    tile_endgame_bar,  /* TILE_ENDGAME_BAR  */
    tile_clock,        /* TILE_CLOCK        */
    tile_conn_on,      /* TILE_CONN_ON      */
    tile_conn_off,     /* TILE_CONN_OFF     */
    tile_hit_ship,     /* TILE_HIT_SHIP     */
};

/* ---- Attack cursor (hardware sprite 2, colors from registers 21-23) ----
 *
 * 16-wide sprite, corner brackets framing the full 8px cell (the left 8
 * columns). Two images for the blink: image 0 uses sprite plane A only
 * (color 21), image 1 plane B only (color 22). Layout is SimpleSprite data:
 * posctl pair, height x (planeA, planeB), terminator.
 *
 * Bracket art per row (high byte = the cell's 8px, bit 15 = leftmost):
 *   0xC300 = ##....##   0x8100 = #......#   — corners top (rows 0-1) and
 *   bottom (rows 6-7), open sides. */

#define CURSOR_SPR_HEIGHT 8
#define CURSOR_SPR_WORDS  (2 + CURSOR_SPR_HEIGHT * 2 + 2)

static const uint16_t cursor_spr_a[CURSOR_SPR_WORDS] = {
    0x0000, 0x0000,
    0xC300, 0x0000,
    0x8100, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x8100, 0x0000,
    0xC300, 0x0000,
    0x0000, 0x0000
};

static const uint16_t cursor_spr_b[CURSOR_SPR_WORDS] = {
    0x0000, 0x0000,
    0x0000, 0xC300,
    0x0000, 0x8100,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x8100,
    0x0000, 0xC300,
    0x0000, 0x0000
};

/* Sprite color registers 21-23 (sprites 2/3 bank): bright / blink / mix. */
static const uint16_t cursor_spr_rgb[3] = { 0xFFF, 0xFC3, 0xF80 };

#endif /* TILES_H */
