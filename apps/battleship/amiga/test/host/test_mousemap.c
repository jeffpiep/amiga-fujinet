/* T1 host unit test for mouse aiming math (see docs/testing.md). Includes
 * the module .c files directly per the repo T1 convention; amiga_vars.h
 * proves the MM_* bits match the game's JOY_* macros. */
#define FN_TEST_MAIN
#include "fn_test.h"

#include "amiga_vars.h"
#include "../../src/cellmap.c"
#include "../../src/mousemap.c"

int main(void)
{
    uint8_t cx, cy, ox, oy;
    int16_t left, top;

    /* MM_* bit layout must match the JOY_* macros the game tests with. */
    CHECK(JOY_UP(MM_UP));
    CHECK(JOY_DOWN(MM_DOWN));
    CHECK(JOY_LEFT(MM_LEFT));
    CHECK(JOY_RIGHT(MM_RIGHT));
    CHECK(JOY_BTN_1(MM_FIRE));
    CHECK_EQ(MM_FIRE, JOY_BTN_1_MASK);

    /* 2-player: only quadrant 1 (top board) is an enemy field. */
    cm_quadrant_origin(2, 1, &ox, &oy);
    left = CM_PX(ox);
    top  = CM_PY(oy);

    /* Top-left pixel of the field -> cell (0,0). */
    CHECK(mm_aim_cell(2, left, top, &cx, &cy));
    CHECK_EQ(cx, 0); CHECK_EQ(cy, 0);

    /* Bottom-right pixel of the field -> cell (9,9). */
    CHECK(mm_aim_cell(2, left + 79, top + 79, &cx, &cy));
    CHECK_EQ(cx, 9); CHECK_EQ(cy, 9);

    /* Mid-cell pixel -> correct cell. */
    CHECK(mm_aim_cell(2, left + 4 * 8 + 3, top + 7 * 8 + 5, &cx, &cy));
    CHECK_EQ(cx, 4); CHECK_EQ(cy, 7);

    /* One pixel outside each edge -> miss. */
    CHECK_EQ(mm_aim_cell(2, left - 1, top, &cx, &cy), 0);
    CHECK_EQ(mm_aim_cell(2, left, top - 1, &cx, &cy), 0);
    CHECK_EQ(mm_aim_cell(2, left + 80, top, &cx, &cy), 0);
    CHECK_EQ(mm_aim_cell(2, left, top + 80, &cx, &cy), 0);

    /* The player's own board (quadrant 0) is never a target. */
    cm_quadrant_origin(2, 0, &ox, &oy);
    CHECK_EQ(mm_aim_cell(2, CM_PX(ox) + 40, CM_PY(oy) + 40, &cx, &cy), 0);

    /* 4-player: quadrants 1, 2, 3 are all aimable. */
    cm_quadrant_origin(4, 2, &ox, &oy);
    CHECK(mm_aim_cell(4, CM_PX(ox) + 33, CM_PY(oy) + 65, &cx, &cy));
    CHECK_EQ(cx, 4); CHECK_EQ(cy, 8);
    cm_quadrant_origin(4, 3, &ox, &oy);
    CHECK(mm_aim_cell(4, CM_PX(ox), CM_PY(oy), &cx, &cy));
    CHECK_EQ(cx, 0); CHECK_EQ(cy, 0);
    cm_quadrant_origin(4, 0, &ox, &oy);
    CHECK_EQ(mm_aim_cell(4, CM_PX(ox) + 1, CM_PY(oy) + 1, &cx, &cy), 0);

    /* Placement: own board (quadrant 0) hit-testing. */
    cm_quadrant_origin(2, 0, &ox, &oy);
    left = CM_PX(ox);
    top  = CM_PY(oy);
    CHECK(mm_place_cell(2, left, top, &cx, &cy));
    CHECK_EQ(cx, 0); CHECK_EQ(cy, 0);
    CHECK(mm_place_cell(2, left + 6 * 8 + 2, top + 3 * 8 + 7, &cx, &cy));
    CHECK_EQ(cx, 6); CHECK_EQ(cy, 3);
    CHECK_EQ(mm_place_cell(2, left - 1, top, &cx, &cy), 0);
    CHECK_EQ(mm_place_cell(2, left + 80, top + 80, &cx, &cy), 0);

    /* Enemy board is not a placement target. */
    cm_quadrant_origin(2, 1, &ox, &oy);
    CHECK_EQ(mm_place_cell(2, CM_PX(ox) + 4, CM_PY(oy) + 4, &cx, &cy), 0);

    /* Origin clamping keeps the whole ship on the board. */
    cx = 9; cy = 4;
    mm_clamp_origin(5, 0, &cx, &cy);          /* horizontal size 5 */
    CHECK_EQ(cx, 5); CHECK_EQ(cy, 4);         /* x clamped, y untouched */
    cx = 4; cy = 9;
    mm_clamp_origin(3, 1, &cx, &cy);          /* vertical size 3 */
    CHECK_EQ(cx, 4); CHECK_EQ(cy, 7);
    cx = 2; cy = 2;
    mm_clamp_origin(4, 0, &cx, &cy);          /* already valid: unchanged */
    CHECK_EQ(cx, 2); CHECK_EQ(cy, 2);
    cx = 8; cy = 8;
    mm_clamp_origin(2, 1, &cx, &cy);          /* size 2 vertical: y max 8 */
    CHECK_EQ(cx, 8); CHECK_EQ(cy, 8);

    /* Chase steps: cardinal, diagonal, arrival. */
    CHECK_EQ(mm_chase_bits(5, 5, 5, 5), 0);
    CHECK_EQ(mm_chase_bits(5, 5, 8, 5), MM_RIGHT);
    CHECK_EQ(mm_chase_bits(5, 5, 2, 5), MM_LEFT);
    CHECK_EQ(mm_chase_bits(5, 5, 5, 9), MM_DOWN);
    CHECK_EQ(mm_chase_bits(5, 5, 5, 1), MM_UP);
    CHECK_EQ(mm_chase_bits(5, 5, 9, 9), (MM_RIGHT | MM_DOWN));
    CHECK_EQ(mm_chase_bits(5, 5, 0, 0), (MM_LEFT | MM_UP));

    return fn_test_report("test_mousemap");
}
