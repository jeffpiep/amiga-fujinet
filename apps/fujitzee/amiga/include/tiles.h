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
 * Phase 2 drew flat geometric shapes here so the layout could be judged
 * before the art existed; Phase 3c is that art. The dice are bevelled and
 * rounded, the game's own score row is a wordmark, and the two icons a
 * player watches in play — whose turn it is, and where the score cursor
 * sits — are tiles rather than ASCII stand-ins.
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
       0x000,  0xFFF,  0xFC3,  0x08A,  0xCCC,  0xFD6,  0x6CF,   0x001,
    /* 8 HI_SELF 9 HI_OTHER 10 HI_FLASH 11 CURSOR 12 CONN 13 DIM 14 SHADE 15 */
       0x3D5,    0x079,     0xFFF,      0xFC3,    0x0DE, 0x99A, 0x557,  0x000
};

/*
 * Two of those are load-bearing for the dice and worth stating outright.
 * PEN_TEXT doubles as the lit edge of a die and PEN_SHADE as its shaded
 * edge, shared across all three face colors — the palette has no room for a
 * light and a dark per face, and one neutral pair reads correctly against
 * white, gold and blue alike. That is also why PEN_DIE is 0xCCC rather than
 * near-white: a white face leaves the white highlight nothing to say.
 */

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
 * bevelled body plus, on seven of the nine positions, a pip; fjlayout.c
 * decides which cell gets which tile for a given face. The set is instantiated
 * three times, once per face color (plain / kept / highlighted), because a
 * tile carries its own colors — see fjlayout.h's FJ_STYLE_*.
 */

/*
 * The body is a rounded rectangle with a two-pixel bite out of each corner,
 * lit along its top and left edge (H) and shaded along its bottom and right
 * (S), which is what turns a flat square into something that reads as a
 * thrown object. There is deliberately no dark outline: the background is
 * black, so an outline in PEN_PIP would be invisible and the bevel is what
 * draws the silhouette.
 *
 * Pips are 3x3 at cell columns/rows 3-5, which centres them on the die's
 * 4 / 12 / 20 pixel thirds.
 */
#define B PEN_BG
#define K PEN_PIP
#define H PEN_TEXT
#define S PEN_SHADE

#define DIE_CELL_TL(F) TILE_MC( \
    B,B,H,H,H,H,H,H,  B,H,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F, \
    H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F)

#define DIE_CELL_TL_PIP(F) TILE_MC( \
    B,B,H,H,H,H,H,H,  B,H,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,K,K,K,F,F, \
    H,F,F,K,K,K,F,F,  H,F,F,K,K,K,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F)

#define DIE_CELL_T(F) TILE_MC( \
    H,H,H,H,H,H,H,H,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F, \
    F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F)

#define DIE_CELL_TR(F) TILE_MC( \
    H,H,H,H,H,H,B,B,  F,F,F,F,F,F,S,B,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S, \
    F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S)

#define DIE_CELL_TR_PIP(F) TILE_MC( \
    H,H,H,H,H,H,B,B,  F,F,F,F,F,F,S,B,  F,F,F,F,F,F,F,S,  F,F,F,K,K,K,F,S, \
    F,F,F,K,K,K,F,S,  F,F,F,K,K,K,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S)

#define DIE_CELL_L(F) TILE_MC( \
    H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F, \
    H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F)

#define DIE_CELL_L_PIP(F) TILE_MC( \
    H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,K,K,K,F,F, \
    H,F,F,K,K,K,F,F,  H,F,F,K,K,K,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F)

#define DIE_CELL_C(F) TILE_MC( \
    F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F, \
    F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F)

#define DIE_CELL_C_PIP(F) TILE_MC( \
    F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  F,F,F,K,K,K,F,F, \
    F,F,F,K,K,K,F,F,  F,F,F,K,K,K,F,F,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F)

#define DIE_CELL_R(F) TILE_MC( \
    F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S, \
    F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S)

#define DIE_CELL_R_PIP(F) TILE_MC( \
    F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,K,K,K,F,S, \
    F,F,F,K,K,K,F,S,  F,F,F,K,K,K,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S)

#define DIE_CELL_BL(F) TILE_MC( \
    H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F, \
    H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  B,H,F,F,F,F,F,F,  B,B,S,S,S,S,S,S)

#define DIE_CELL_BL_PIP(F) TILE_MC( \
    H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,F,F,F,F,F,  H,F,F,K,K,K,F,F, \
    H,F,F,K,K,K,F,F,  H,F,F,K,K,K,F,F,  B,H,F,F,F,F,F,F,  B,B,S,S,S,S,S,S)

#define DIE_CELL_B(F) TILE_MC( \
    F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F, \
    F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  F,F,F,F,F,F,F,F,  S,S,S,S,S,S,S,S)

#define DIE_CELL_BR(F) TILE_MC( \
    F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S, \
    F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,S,B,  S,S,S,S,S,S,B,B)

#define DIE_CELL_BR_PIP(F) TILE_MC( \
    F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,F,F,F,F,S,  F,F,F,K,K,K,F,S, \
    F,F,F,K,K,K,F,S,  F,F,F,K,K,K,F,S,  F,F,F,F,F,F,S,B,  S,S,S,S,S,S,B,B)

/* One whole face-color's worth of cells, bevelled against `face` as the lit
 * ground. Order must match the FJD_* enum in fjlayout.h. */
#define DIE_SET(name, face) \
    static const uint16_t name##_tl[32]     = DIE_CELL_TL(face);     \
    static const uint16_t name##_tl_pip[32] = DIE_CELL_TL_PIP(face); \
    static const uint16_t name##_t[32]      = DIE_CELL_T(face);      \
    static const uint16_t name##_tr[32]     = DIE_CELL_TR(face);     \
    static const uint16_t name##_tr_pip[32] = DIE_CELL_TR_PIP(face); \
    static const uint16_t name##_l[32]      = DIE_CELL_L(face);      \
    static const uint16_t name##_l_pip[32]  = DIE_CELL_L_PIP(face);  \
    static const uint16_t name##_c[32]      = DIE_CELL_C(face);      \
    static const uint16_t name##_c_pip[32]  = DIE_CELL_C_PIP(face);  \
    static const uint16_t name##_r[32]      = DIE_CELL_R(face);      \
    static const uint16_t name##_r_pip[32]  = DIE_CELL_R_PIP(face);  \
    static const uint16_t name##_bl[32]     = DIE_CELL_BL(face);     \
    static const uint16_t name##_bl_pip[32] = DIE_CELL_BL_PIP(face); \
    static const uint16_t name##_b[32]      = DIE_CELL_B(face);      \
    static const uint16_t name##_br[32]     = DIE_CELL_BR(face);     \
    static const uint16_t name##_br_pip[32] = DIE_CELL_BR_PIP(face)

/* The same 16 names as pointers, in FJD_* order, for tile_table[]. */
#define DIE_SET_ENTRIES(name) \
    name##_tl, name##_tl_pip, name##_t, name##_tr, name##_tr_pip,    \
    name##_l,  name##_l_pip,  name##_c, name##_c_pip,                \
    name##_r,  name##_r_pip,  name##_bl, name##_bl_pip, name##_b,    \
    name##_br, name##_br_pip

DIE_SET(die_plain, PEN_DIE);
DIE_SET(die_keep,  PEN_DIE_KEEP);
DIE_SET(die_hi,    PEN_DIE_HI);

/* ---- Wordmark ---------------------------------------------------------
 *
 * drawFujitzee() draws these five cells: a snow-capped mountain, then TZEE
 * in a heavier letterform than topaz so the game's own row reads as a logo
 * and not as one more score name. The mountain is the /|\ the Atari's
 * custom charset spells the same idea with, and it borrows the board's own
 * teal so the wordmark belongs to the lattice it sits in.
 */
#define W PEN_TEXT
#define L PEN_LINE

static const uint16_t tile_logo_m[32] = TILE_MC(
    B,B,B,B,B,B,B,B,  B,B,B,W,B,B,B,B,  B,B,W,W,W,B,B,B,  B,W,W,L,W,W,B,B,
    B,L,L,L,L,L,L,B,  L,L,L,L,L,L,L,L,  L,L,L,L,L,L,L,L,  L,L,L,L,L,L,L,L);

static const uint16_t tile_logo_t[32] = TILE_PAT(PEN_ALT, PEN_BG,
    0x00, 0x7E, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x00);

static const uint16_t tile_logo_z[32] = TILE_PAT(PEN_ALT, PEN_BG,
    0x00, 0x7E, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00);

static const uint16_t tile_logo_e[32] = TILE_PAT(PEN_ALT, PEN_BG,
    0x00, 0x7E, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00);

/* ---- Play icons -------------------------------------------------------
 *
 * The turn marker and the score cursor are the two icons a player actually
 * watches, and both were ASCII placeholders ('+' and '*'). They point the
 * same way for the same reason — the marker points at a name in the player
 * list, the cursor at the row a score would land on — so the marker is a
 * solid wedge and the cursor a hollow chevron, distinct at a glance without
 * being two unrelated shapes. The blip is the cursor's blink phase (and the
 * lobby's unselected-table mark), so it is the chevron's tip alone.
 */
static const uint16_t tile_icon_mark[32] = TILE_PAT(PEN_ALT, PEN_BG,
    0x00, 0xC0, 0xF0, 0xFC, 0xFC, 0xF0, 0xC0, 0x00);

static const uint16_t tile_icon_cursor[32] = TILE_PAT(PEN_CURSOR, PEN_BG,
    0xC0, 0x60, 0x30, 0x18, 0x18, 0x30, 0x60, 0xC0);

static const uint16_t tile_icon_cursor_alt[32] = TILE_PAT(PEN_HI_SELF, PEN_BG,
    0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x3C, 0x18);

static const uint16_t tile_icon_blip[32] = TILE_PAT(PEN_CURSOR, PEN_BG,
    0x00, 0x00, 0x18, 0x3C, 0x3C, 0x18, 0x00, 0x00);

#undef W
#undef L
#undef B
#undef K
#undef H
#undef S

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
    tile_logo_m, tile_logo_t, tile_logo_z, tile_logo_e,
    tile_icon_mark, tile_icon_cursor, tile_icon_cursor_alt, tile_icon_blip,
    DIE_SET_ENTRIES(die_plain),
    DIE_SET_ENTRIES(die_keep),
    DIE_SET_ENTRIES(die_hi)
};

#endif /* TILES_H */
