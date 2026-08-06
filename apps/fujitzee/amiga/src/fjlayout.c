/*
 * fjlayout.c - fujitzee die/dice layout math (pure C99, no AmigaOS)
 *
 * See fjlayout.h. Host-tested by test/host/test_fjlayout.c.
 */
#include "fjlayout.h"

/*
 * Pip positions per face, as a bitmask over the 3x3 cell grid (bit 0 =
 * top-left, bit 8 = bottom-right). Faces are the familiar arrangement:
 * odd faces put a pip in the centre, 4 and 6 fill the corners and — for 6
 * — the two mid-row cells.
 */
static const uint16_t _pips[FJ_S_FACE_MAX] = {
    0x010,  /* 1:       centre               */
    0x101,  /* 2: two opposite corners       */
    0x111,  /* 3: diagonal through centre    */
    0x145,  /* 4: four corners               */
    0x155,  /* 5: four corners + centre      */
    0x16D   /* 6: four corners + mid row     */
};

/* Frame tile per cell, and the pipped variant (same value when the cell
 * can never carry a pip). */
static const uint8_t _frame[9] = {
    FJD_TL, FJD_T, FJD_TR,
    FJD_L,  FJD_C, FJD_R,
    FJD_BL, FJD_B, FJD_BR
};
static const uint8_t _framePip[9] = {
    FJD_TL_PIP, FJD_T, FJD_TR_PIP,
    FJD_L_PIP,  FJD_C_PIP, FJD_R_PIP,
    FJD_BL_PIP, FJD_B, FJD_BR_PIP
};

uint8_t fj_die_style(uint8_t is_selected, uint8_t is_highlighted)
{
    if (is_highlighted)
        return FJ_STYLE_HI;
    if (is_selected)
        return FJ_STYLE_KEEP;
    return FJ_STYLE_PLAIN;
}

uint8_t fj_die_tile(uint8_t face, uint8_t cell, uint8_t style)
{
    uint16_t pips;

    if (face < 1 || face > FJ_S_FACE_MAX || cell > 8 || style >= FJ_DIE_STYLES)
        return TILE_BLANK;

    pips = _pips[face - 1];
    return (uint8_t)(TILE_DIE + style * FJ_DIE_TILES +
                     ((pips >> cell) & 1 ? _framePip[cell] : _frame[cell]));
}

uint8_t fj_frame_tile(uint8_t cell, uint8_t style)
{
    if (cell > 8 || style >= FJ_DIE_STYLES)
        return TILE_BLANK;
    return (uint8_t)(TILE_DIE + style * FJ_DIE_TILES + _frame[cell]);
}

char fj_roll_label(uint8_t s)
{
    switch (s) {
    case 14: return '1';
    case 15: return '2';
    case 16: return 'x';   /* rolls exhausted */
    default: return 0;
    }
}

/*
 * The icon characters are spelled out rather than pulled from amiga_vars.h:
 * that header is force-included into the game build only, and this file is
 * also compiled natively for the host tests. The literals below are the
 * ICON_* values — test_fjlayout.c pins them against the header so the two
 * cannot drift apart silently.
 */
uint8_t fj_icon_tile(uint8_t icon)
{
    switch (icon) {
    case '+': return TILE_ICON_MARK;         /* ICON_MARK                  */
    case '*': return TILE_ICON_CURSOR;       /* ICON_CURSOR                */
    case 'o': return TILE_ICON_CURSOR_ALT;   /* ICON_CURSOR_ALT            */
    /* ICON_CURSOR_BLIP and ICON_MARK_ALT are both '.', and both want the
     * same dim dot — one is the cursor's blink phase, the other the lobby's
     * unselected table mark. */
    case '.': return TILE_ICON_BLIP;
    default:  return FJ_ICON_NO_TILE;
    }
}

uint8_t fj_score_spill(uint8_t x, uint8_t y, const char *s)
{
    uint8_t len = 0;
    uint8_t p;

    if (!s)
        return FJ_NO_SPILL;
    for (len = 0; s[len]; len++) {
        if (s[len] < '0' || s[len] > '9')
            return FJ_NO_SPILL;
        if (len > 5)
            return FJ_NO_SPILL;    /* not a score; leave it alone */
    }
    if (len < 4)
        return FJ_NO_SPILL;        /* fits the three cells as drawn */

    /* Inside the lattice only — the grand-total row hangs below it. */
    if (y <= FJ_BOARD_TOP || y >= FJ_BOARD_BOTTOM)
        return FJ_NO_SPILL;

    /* x must be a divider cell, i.e. one left of some player's column. */
    for (p = 0; p < FJ_PLAYERS_SHOWN; p++) {
        if (x == (uint8_t)FJ_DIV_X(p))
            return (uint8_t)FJ_COL_X(p);
    }
    return FJ_NO_SPILL;
}

uint8_t fj_spill_advance(uint8_t len)
{
    uint8_t adv;

    if (!len)
        return 8;
    adv = (uint8_t)(FJ_SPILL_SPAN / len);   /* 4 digits -> 7px, 5 -> 6 */
    return adv > 8 ? 8 : adv;
}

uint8_t fj_alt_is_key(char c)
{
    return (uint8_t)(c >= 'A' && c <= 'Z');
}
