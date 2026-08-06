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

uint8_t fj_alt_is_key(char c)
{
    return (uint8_t)(c >= 'A' && c <= 'Z');
}
