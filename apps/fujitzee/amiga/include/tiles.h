/*
 * tiles.h - fujitzee tile art and palette (data only)
 *
 * THE art-pass surface: every non-text visual in the renderer is defined
 * here — swap the arrays, keep the names, and no engine code changes.
 * Include from gfxsetup.c ONLY (defines data, not just declarations).
 *
 * The art is written with the TILE_PAT composer from
 * libs/amiga-gamekit/include/tilepat.h, which documents the 32-word tile
 * format it expands to; any entry can equally be a hand- or tool-generated
 * array, and TILE_MC from the same header authors multicolor tiles.
 *
 * What is here is deliberately plain: Phase 2's goal is a playable board,
 * and Phase 3c is the art pass (real dice faces, a fujiTZEE logo, proper
 * clock/connection icons). The shapes below are geometric so the layout
 * can be judged before the art exists.
 *
 * See: docs/plan-track1c-fujitzee.md (Phase 2, Phase 3c)
 */
#ifndef TILES_H
#define TILES_H

#include <stdint.h>
#include "fjlayout.h"   /* TILE_* ids — tile_table[] is indexed by them */
#include "pens.h"       /* PEN_* numbers the palette below gives color to */
#include "tilepat.h"

/* ---- Palette (12-bit RGB4, LoadRGB4 order, indexed by PEN_*) ---- */

static const uint16_t tile_palette[16] = {
    /* 0 BG    1 TEXT  2 ALT   3 LINE  4 DIE   5 KEEP  6 DIE_HI 7 PIP   */
       0x000,  0xFFF,  0xFC3,  0x08A,  0xEEE,  0xFD6,  0x6CF,   0x001,
    /* 8 HI_SELF 9 HI_OTHER 10 HI_FLASH 11 CURSOR 12 CONN 13 DIM 14 SHADE 15 */
       0x3D5,    0x079,     0xFFF,      0xFC3,    0x0DE, 0x99A, 0x226,  0x000
};

/* ---- Board lattice ----------------------------------------------------
 *
 * Rules are drawn as bands inside the 8x8 cell: a "thick" horizontal fills
 * rows 3-4, a "thin" one just row 4, and a vertical fills columns 3-4
 * (0x18). Junction tiles are the union of the segments that meet there,
 * with half-length horizontals where a rule ends at the vertical, so the
 * lattice joins up without gaps. */

static const uint16_t tile_blank[32] = TILE_PAT(PEN_BG, PEN_BG,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

static const uint16_t tile_hrule[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00);

static const uint16_t tile_hthin[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00);

static const uint16_t tile_vrule[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18);

/* Box corners: half a thick rule meeting half a vertical. */
static const uint16_t tile_box_tl[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x00, 0x00, 0x00, 0x1F, 0x1F, 0x18, 0x18, 0x18);

static const uint16_t tile_box_tr[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x00, 0x00, 0x00, 0xF8, 0xF8, 0x18, 0x18, 0x18);

static const uint16_t tile_box_bl[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x18, 0x18, 0x18, 0x1F, 0x1F, 0x00, 0x00, 0x00);

static const uint16_t tile_box_br[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x18, 0x18, 0x18, 0xF8, 0xF8, 0x00, 0x00, 0x00);

/* Tees and crosses. */
static const uint16_t tile_tee_d[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0x18, 0x18, 0x18);

static const uint16_t tile_tee_u[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x18, 0x18, 0x18, 0xFF, 0xFF, 0x00, 0x00, 0x00);

static const uint16_t tile_tee_l[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x18, 0x18, 0x18, 0xF8, 0xF8, 0x18, 0x18, 0x18);

static const uint16_t tile_cross[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x18, 0x18, 0x18, 0xFF, 0xFF, 0x18, 0x18, 0x18);

static const uint16_t tile_cross_thin[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x18, 0x18, 0x18, 0x18, 0xFF, 0x18, 0x18, 0x18);

static const uint16_t tile_tee_r_thin[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x18, 0x18, 0x18, 0x18, 0x1F, 0x18, 0x18, 0x18);

static const uint16_t tile_tee_l_thin[32] = TILE_PAT(PEN_LINE, PEN_BG,
    0x18, 0x18, 0x18, 0x18, 0xF8, 0x18, 0x18, 0x18);

/* ---- Active-player column brackets ------------------------------------
 * setHighlight() swaps the two dividers bracketing a player's column for
 * these, so the column the turn belongs to reads at a glance without
 * recoloring the cells between them (which would mean re-rendering every
 * score). Wider than a plain divider — the point is to be seen. */

#define BRACKET_ROWS 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C

static const uint16_t tile_hi_self[32]  = TILE_PAT_V(PEN_HI_SELF, PEN_BG, BRACKET_ROWS);
static const uint16_t tile_hi_other[32] = TILE_PAT_V(PEN_HI_OTHER, PEN_BG, BRACKET_ROWS);
static const uint16_t tile_hi_flash[32] = TILE_PAT_V(PEN_HI_FLASH, PEN_BG, BRACKET_ROWS);

/* ---- Dice cursor ------------------------------------------------------
 * Half-height brackets either side of the die under the cursor. The Atari
 * and DOS ports draw a full frame around the die; we have a spare cell on
 * each side (dice are 4 cells apart, 3 wide) but none above or below, so
 * the cursor is the two sides only. */

static const uint16_t tile_cur_l[32] = TILE_PAT(PEN_CURSOR, PEN_BG,
    0x0F, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0F);

static const uint16_t tile_cur_r[32] = TILE_PAT(PEN_CURSOR, PEN_BG,
    0xF0, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0xF0);

/* ---- Status icons ----------------------------------------------------- */

/* Move timer: a clock face with hands at 12 and 4. */
static const uint16_t tile_clock[32] = TILE_PAT(PEN_ALT, PEN_BG,
    0x3C, 0x42, 0x99, 0x91, 0x8F, 0x42, 0x42, 0x3C);

/* Connection trouble: a two-cell plug-and-lead. */
static const uint16_t tile_conn_l[32] = TILE_PAT(PEN_CONN, PEN_BG,
    0x00, 0x0C, 0x1E, 0x3F, 0x3F, 0x1E, 0x0C, 0x00);

static const uint16_t tile_conn_r[32] = TILE_PAT(PEN_CONN, PEN_BG,
    0x00, 0x00, 0x80, 0xE0, 0xF8, 0x00, 0x00, 0x00);

/* ---- Dice -------------------------------------------------------------
 *
 * One 3x3-cell die per face. Each cell carries its share of the die's
 * outline plus, on seven of the nine positions, a pip; fjlayout.c decides
 * which cell gets which tile for a given face. The set is instantiated
 * three times, once per face color (plain / kept / highlighted), because a
 * tile carries its own colors — see fjlayout.h's FJ_STYLE_*.
 */

#define DIE_ROWS_TL      0xFF, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
#define DIE_ROWS_TL_PIP  0xFF, 0x80, 0xB8, 0xB8, 0xB8, 0x80, 0x80, 0x80
#define DIE_ROWS_T       0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define DIE_ROWS_TR      0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01
#define DIE_ROWS_TR_PIP  0xFF, 0x01, 0x39, 0x39, 0x39, 0x01, 0x01, 0x01
#define DIE_ROWS_L       0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
#define DIE_ROWS_L_PIP   0x80, 0x80, 0xB8, 0xB8, 0xB8, 0x80, 0x80, 0x80
#define DIE_ROWS_C       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define DIE_ROWS_C_PIP   0x00, 0x00, 0x38, 0x38, 0x38, 0x00, 0x00, 0x00
#define DIE_ROWS_R       0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01
#define DIE_ROWS_R_PIP   0x01, 0x01, 0x39, 0x39, 0x39, 0x01, 0x01, 0x01
#define DIE_ROWS_BL      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFF
#define DIE_ROWS_BL_PIP  0x80, 0x80, 0xB8, 0xB8, 0xB8, 0x80, 0x80, 0xFF
#define DIE_ROWS_B       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF
#define DIE_ROWS_BR      0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF
#define DIE_ROWS_BR_PIP  0x01, 0x01, 0x39, 0x39, 0x39, 0x01, 0x01, 0xFF

/* One whole face-color's worth of cells: pips and outline in PEN_PIP on a
 * `face`-colored ground. Order must match the FJD_* enum in fjlayout.h. */
#define DIE_SET(name, face) \
    static const uint16_t name##_tl[32]     = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_TL);     \
    static const uint16_t name##_tl_pip[32] = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_TL_PIP); \
    static const uint16_t name##_t[32]      = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_T);      \
    static const uint16_t name##_tr[32]     = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_TR);     \
    static const uint16_t name##_tr_pip[32] = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_TR_PIP); \
    static const uint16_t name##_l[32]      = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_L);      \
    static const uint16_t name##_l_pip[32]  = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_L_PIP);  \
    static const uint16_t name##_c[32]      = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_C);      \
    static const uint16_t name##_c_pip[32]  = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_C_PIP);  \
    static const uint16_t name##_r[32]      = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_R);      \
    static const uint16_t name##_r_pip[32]  = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_R_PIP);  \
    static const uint16_t name##_bl[32]     = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_BL);     \
    static const uint16_t name##_bl_pip[32] = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_BL_PIP); \
    static const uint16_t name##_b[32]      = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_B);      \
    static const uint16_t name##_br[32]     = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_BR);     \
    static const uint16_t name##_br_pip[32] = TILE_PAT_V(PEN_PIP, face, DIE_ROWS_BR_PIP)

/* The same 16 names as pointers, in FJD_* order, for tile_table[]. */
#define DIE_SET_ENTRIES(name) \
    name##_tl, name##_tl_pip, name##_t, name##_tr, name##_tr_pip,    \
    name##_l,  name##_l_pip,  name##_c, name##_c_pip,                \
    name##_r,  name##_r_pip,  name##_bl, name##_bl_pip, name##_b,    \
    name##_br, name##_br_pip

DIE_SET(die_plain, PEN_DIE);
DIE_SET(die_keep,  PEN_DIE_KEEP);
DIE_SET(die_hi,    PEN_DIE_HI);

/* ---- Tile table (indexed by the TILE_* enum in fjlayout.h) ---- */

static const uint16_t *const tile_table[TILE_COUNT] = {
    tile_blank,
    tile_hrule, tile_hthin, tile_vrule,
    tile_box_tl, tile_box_tr, tile_box_bl, tile_box_br,
    tile_tee_d, tile_tee_u, tile_tee_l, tile_cross, tile_cross_thin,
    tile_tee_r_thin, tile_tee_l_thin,
    tile_hi_self, tile_hi_other, tile_hi_flash,
    tile_cur_l, tile_cur_r,
    tile_clock, tile_conn_l, tile_conn_r,
    DIE_SET_ENTRIES(die_plain),
    DIE_SET_ENTRIES(die_keep),
    DIE_SET_ENTRIES(die_hi)
};

#endif /* TILES_H */
