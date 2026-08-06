/*
 * fjlayout.h - fujitzee screen geometry, tile ids, and die/dice layout math
 *
 * The AmigaOS-free half of the renderer: every decision that is arithmetic
 * rather than drawing lives here so it can be host-tested (T1,
 * test/host/test_fjlayout.c) instead of eyeballed in the emulator. src/
 * graphics.c is then only "call gfx_* with what this file says".
 *
 * The geometry constants are here rather than in amiga_vars.h because the
 * board lattice and the score columns have to agree exactly — upstream
 * computes the active player's column as SCORES_X+6+player*4 and we draw
 * the dividers, so a test that pins both against each other is worth more
 * than two matching literals in two headers. amiga_vars.h aliases these
 * to the names upstream expects.
 *
 * Pure C99, no AmigaOS headers — host-includable.
 *
 * See: docs/plan-track1c-fujitzee.md (Phase 2)
 */
#ifndef FJLAYOUT_H
#define FJLAYOUT_H

#include <stdint.h>

/* ---- Cell grid -------------------------------------------------------- *
 *
 * 40x25 cells of 8x8 px on a 320x200 custom screen. The Atari port is
 * 40x26 and its bottom panel offsets assume that extra row; the DOS port
 * is 40x25 like ours and already re-derived those offsets (its
 * clearBelowBoard comment spells out the one-row difference), so this port
 * follows the DOS layout, including SCORES_X = 10 rather than Atari's 11.
 */
#define FJ_WIDTH      40
#define FJ_HEIGHT     25

/* X of the score-name column (upstream's SCORES_X). The name box is drawn
 * one cell left of it and the player columns start 6 right of it. */
#define FJ_SCORES_X   10

/* Score-column geometry, both derived from FJ_SCORES_X. Upstream computes
 * the active player's score column as SCORES_X+6+player*4 (gamelogic.c's
 * validX); the vertical dividers we draw must bracket exactly that. */
#define FJ_COL_X(p)   (FJ_SCORES_X + 6 + (p) * 4)   /* score cells: x..x+2 */
#define FJ_DIV_X(p)   (FJ_SCORES_X + 5 + (p) * 4)   /* divider left of it  */

/* Players whose columns fit across 40 cells. Upstream draws at most 6
 * (gamelogic.c skips i>5) — the grid agrees: FJ_DIV_X(6) == 39, the last
 * usable column. A table may hold up to PLAYER_MAX; the rest are scored by
 * the server and simply not shown, same as the Atari and DOS ports. */
#define FJ_PLAYERS_SHOWN 6

/* Board rows. The score-name box spans rows 2..20 with thin rules inside. */
#define FJ_BOARD_TOP     2
#define FJ_BOARD_BOTTOM 20
#define FJ_THIN_ROW_A    9
#define FJ_THIN_ROW_B   12

/* Bottom panel: dice sit on rows FJ_DICE_Y..+2, status text on the last. */
#define FJ_DICE_Y     (FJ_HEIGHT - 4)

/* ---- Tile ids --------------------------------------------------------- *
 * tiles.h defines tile_table[] in exactly this order. */
enum {
    TILE_BLANK = 0,

    /* Board lattice. "Thick" rules are 2 px, "thin" 1 px, verticals 2 px. */
    TILE_HRULE,          /* thick horizontal                               */
    TILE_HTHIN,          /* thin horizontal                                */
    TILE_VRULE,          /* vertical                                       */
    TILE_BOX_TL,         /* box corners (thick)                            */
    TILE_BOX_TR,
    TILE_BOX_BL,
    TILE_BOX_BR,
    TILE_TEE_D,          /* thick rule with a vertical dropping down       */
    TILE_TEE_U,          /* thick rule with a vertical coming up           */
    TILE_TEE_L,          /* vertical with a thick rule leaving to the left */
    TILE_CROSS,          /* thick rule crossing a vertical                 */
    TILE_CROSS_THIN,     /* thin rule crossing a vertical                  */
    TILE_TEE_R_THIN,     /* vertical with a thin rule leaving to the right */
    TILE_TEE_L_THIN,     /* vertical with a thin rule leaving to the left  */

    /* Active-player column brackets (setHighlight). */
    TILE_HI_SELF,        /* it is the local player's turn                  */
    TILE_HI_OTHER,       /* someone else's turn                            */
    TILE_HI_FLASH,       /* end-of-turn flash                              */

    /* Dice selection cursor: brackets either side of the hovered die. */
    TILE_CUR_L,
    TILE_CUR_R,

    /* Status icons. */
    TILE_CLOCK,
    TILE_CONN_L,
    TILE_CONN_R,

    /* Die faces: FJ_DIE_STYLES sets of FJ_DIE_TILES cells each. Always
     * last — the sets are addressed as TILE_DIE + style*FJ_DIE_TILES. */
    TILE_DIE
};

/* ---- Dice ------------------------------------------------------------- *
 *
 * A die is 3x3 cells. Each cell is one of nine frame positions, and the
 * seven that can carry a pip have a second tile with the pip drawn in
 * (top-centre and bottom-centre never do, on any face). Cells are indexed
 * 0..8 left-to-right, top-to-bottom, which is also the pip grid: face 6 is
 * pips at 0,2,3,5,6,8, and so on.
 */
enum {
    FJD_TL = 0, FJD_TL_PIP,
    FJD_T,
    FJD_TR,     FJD_TR_PIP,
    FJD_L,      FJD_L_PIP,
    FJD_C,      FJD_C_PIP,
    FJD_R,      FJD_R_PIP,
    FJD_BL,     FJD_BL_PIP,
    FJD_B,
    FJD_BR,     FJD_BR_PIP,
    FJ_DIE_TILES
};

/* Die styles, in tile_table order. Which one applies is fj_die_style(). */
#define FJ_STYLE_PLAIN 0   /* rolled, not kept          */
#define FJ_STYLE_KEEP  1   /* held for the next roll    */
#define FJ_STYLE_HI    2   /* fujitzee! / roll pressed  */
#define FJ_DIE_STYLES  3

#define TILE_COUNT (TILE_DIE + FJ_DIE_STYLES * FJ_DIE_TILES)

/* upstream's drawDie() overloads its `s` argument: 1..6 are die faces,
 * 13 is "draw nothing", and 14..16 are the Roll button. */
#define FJ_S_FACE_MAX  6
#define FJ_S_BLANK    13
#define FJ_S_ROLL_MIN 14
#define FJ_S_ROLL_MAX 16

/* Which of the three tile sets a die drawn with these flags uses. */
uint8_t fj_die_style(uint8_t is_selected, uint8_t is_highlighted);

/* Tile for cell 0..8 of the given face (1..6) in the given style.
 * Out-of-range face or cell yields TILE_BLANK. */
uint8_t fj_die_tile(uint8_t face, uint8_t cell, uint8_t style);

/* Tile for cell 0..8 of a pipless die body — the Roll button, which is a
 * die-shaped box with a label in it. Out-of-range cell yields TILE_BLANK. */
uint8_t fj_frame_tile(uint8_t cell, uint8_t style);

/* Character shown in the middle of the Roll button for upstream's `s`
 * (FJ_S_ROLL_MIN..MAX): the rolls remaining, or 'x' when none are. Returns
 * 0 for any other `s`. Placeholder for the Phase 3c "ROLL" art. */
char fj_roll_label(uint8_t s);

/* drawTextAlt() renders a string in the alternate pen *except* for capital
 * letters, which upstream uses to call out the key that triggers a menu
 * item ("  Q: quit table"). Returns 1 when this character is such a hint.
 * The Atari does the same thing by folding case; we keep the glyph and
 * switch pens, which reads better with a real font. */
uint8_t fj_alt_is_key(char c);

#endif /* FJLAYOUT_H */
