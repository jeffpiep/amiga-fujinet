/* T1 host unit test for the fujitzee layout math (see docs/testing.md).
 * Includes the module .c directly per the repo T1 convention.
 *
 * Two things are being pinned here. The dice half is ordinary: every face
 * gets the right pips in the right cells, and each style draws from its own
 * tile set. The board half is the one that would otherwise only be caught
 * by looking at the emulator — upstream computes the active player's score
 * column itself (gamelogic.c: validX = SCORES_X+6+player*4) while we draw
 * the dividers around it, so the two have to be checked against each other
 * rather than each against a literal. */
#define FN_TEST_MAIN
#include "fn_test.h"

#include "../../src/fjlayout.c"

/* Upstream's own formula, restated so a change on either side shows up. */
#define UPSTREAM_VALID_X(p) (FJ_SCORES_X + 6 + (p) * 4)

/* Count the pips fj_die_tile() reports across a whole face. */
static int pipsOn(uint8_t face)
{
    int cell, n = 0;

    for (cell = 0; cell < 9; cell++) {
        uint8_t t = fj_die_tile(face, (uint8_t)cell, FJ_STYLE_PLAIN);
        uint8_t within = (uint8_t)(t - TILE_DIE);

        if (within == FJD_TL_PIP || within == FJD_TR_PIP ||
            within == FJD_L_PIP  || within == FJD_C_PIP ||
            within == FJD_R_PIP  || within == FJD_BL_PIP ||
            within == FJD_BR_PIP)
            n++;
    }
    return n;
}

/* Is the tile for this cell a pipped one? */
static int hasPip(uint8_t face, uint8_t cell)
{
    return fj_die_tile(face, cell, FJ_STYLE_PLAIN) !=
           fj_frame_tile(cell, FJ_STYLE_PLAIN);
}

int main(void)
{
    uint8_t face, cell, p;

    /* ---- Board geometry ---- */

    /* The dividers we draw must bracket the column upstream writes into:
     * one immediately left of it, the next immediately right of its three
     * score cells. */
    for (p = 0; p < FJ_PLAYERS_SHOWN; p++) {
        CHECK_EQ(FJ_COL_X(p), UPSTREAM_VALID_X(p));
        CHECK_EQ(FJ_DIV_X(p), FJ_COL_X(p) - 1);
        CHECK_EQ(FJ_DIV_X(p + 1), FJ_COL_X(p) + 3);
    }

    /* The last divider is the last cell on the row: one more player column
     * would not fit, which is why FJ_PLAYERS_SHOWN is what it is. */
    CHECK_EQ(FJ_DIV_X(FJ_PLAYERS_SHOWN), FJ_WIDTH - 1);

    /* The score-name box sits left of the first divider and is wide enough
     * for the five-character labels ("count", "s run"). */
    CHECK_EQ(FJ_DIV_X(0) - FJ_SCORES_X, 5);

    /* Rules stay inside the grid, and the dice row clears the bottom one. */
    CHECK(FJ_THIN_ROW_A > FJ_BOARD_TOP && FJ_THIN_ROW_A < FJ_THIN_ROW_B);
    CHECK(FJ_THIN_ROW_B < FJ_BOARD_BOTTOM);
    CHECK(FJ_DICE_Y > FJ_BOARD_BOTTOM);
    CHECK(FJ_DICE_Y + 3 <= FJ_HEIGHT);

    /* ---- Dice ---- */

    /* Each face shows its own number of pips. */
    for (face = 1; face <= FJ_S_FACE_MAX; face++)
        CHECK_EQ(pipsOn(face), face);

    /* Face 1 is the centre alone; 2 is the leading diagonal's ends. */
    CHECK(hasPip(1, 4));
    CHECK(!hasPip(1, 0));
    CHECK(hasPip(2, 0) && hasPip(2, 8));
    CHECK(!hasPip(2, 4));

    /* Odd faces keep the centre pip, even faces never have one. */
    CHECK(hasPip(3, 4) && hasPip(5, 4));
    CHECK(!hasPip(4, 4) && !hasPip(6, 4));

    /* Four corners on every face from 4 up. */
    for (face = 4; face <= 6; face++)
        CHECK(hasPip(face, 0) && hasPip(face, 2) &&
              hasPip(face, 6) && hasPip(face, 8));

    /* Six is the only face using the mid-row sides. */
    CHECK(hasPip(6, 3) && hasPip(6, 5));
    CHECK(!hasPip(5, 3) && !hasPip(5, 5));

    /* Top-centre and bottom-centre never carry a pip on any face — the
     * reason those two cells have no pipped tile at all. */
    for (face = 1; face <= FJ_S_FACE_MAX; face++) {
        CHECK(!hasPip(face, 1));
        CHECK(!hasPip(face, 7));
    }

    /* Styles are three disjoint tile sets over the same cell layout. */
    for (cell = 0; cell < 9; cell++) {
        CHECK_EQ(fj_die_tile(3, cell, FJ_STYLE_KEEP),
                 fj_die_tile(3, cell, FJ_STYLE_PLAIN) + FJ_DIE_TILES);
        CHECK_EQ(fj_die_tile(3, cell, FJ_STYLE_HI),
                 fj_die_tile(3, cell, FJ_STYLE_PLAIN) + 2 * FJ_DIE_TILES);
    }

    /* Every tile a die can draw is inside the table gfxsetup.c publishes. */
    for (face = 1; face <= FJ_S_FACE_MAX; face++)
        for (cell = 0; cell < 9; cell++)
            CHECK(fj_die_tile(face, cell, FJ_STYLE_HI) < TILE_COUNT);

    /* Style selection: highlight wins over kept, as upstream's own dice
     * drawing assumes when it flags a fujitzee on held dice. */
    CHECK_EQ(fj_die_style(0, 0), FJ_STYLE_PLAIN);
    CHECK_EQ(fj_die_style(1, 0), FJ_STYLE_KEEP);
    CHECK_EQ(fj_die_style(0, 1), FJ_STYLE_HI);
    CHECK_EQ(fj_die_style(1, 1), FJ_STYLE_HI);

    /* Out-of-range arguments draw nothing rather than reading past a
     * table (upstream passes `s` values 13..16 through the same call). */
    CHECK_EQ(fj_die_tile(0, 0, FJ_STYLE_PLAIN), TILE_BLANK);
    CHECK_EQ(fj_die_tile(7, 0, FJ_STYLE_PLAIN), TILE_BLANK);
    CHECK_EQ(fj_die_tile(1, 9, FJ_STYLE_PLAIN), TILE_BLANK);
    CHECK_EQ(fj_die_tile(1, 0, FJ_DIE_STYLES), TILE_BLANK);
    CHECK_EQ(fj_frame_tile(9, FJ_STYLE_PLAIN), TILE_BLANK);

    /* ---- Roll button ---- */

    /* upstream draws it as `rollsLeft + 13`, so 13 means "no button". */
    CHECK_EQ(fj_roll_label(FJ_S_BLANK), 0);
    CHECK_EQ(fj_roll_label(14), '1');
    CHECK_EQ(fj_roll_label(15), '2');
    CHECK_EQ(fj_roll_label(16), 'x');
    CHECK_EQ(fj_roll_label(1), 0);

    /* ---- drawTextAlt's key-letter rule ---- */

    CHECK(fj_alt_is_key('Q'));
    CHECK(fj_alt_is_key('S'));
    CHECK(!fj_alt_is_key('q'));
    CHECK(!fj_alt_is_key(':'));
    CHECK(!fj_alt_is_key(' '));
    CHECK(!fj_alt_is_key('7'));

    return fn_test_report("test_fjlayout");
}
