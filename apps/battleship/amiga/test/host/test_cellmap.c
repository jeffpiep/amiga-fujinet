/* T1 host unit test for the graphical renderer's layout math (see
 * docs/testing.md). Includes the module .c directly per the repo T1
 * convention. */
#define FN_TEST_MAIN
#include "fn_test.h"

#include "../../src/cellmap.c"

int main(void)
{
    uint8_t x, y;

    /* Centering shift: 2 boards shift right, 3-4 don't. */
    CHECK_EQ(cm_field_x(2), 7);
    CHECK_EQ(cm_field_x(4), 0);

    /* 2-player: quadrants 0 (bottom) and 1 (top) stack at x=15. */
    cm_quadrant_origin(2, 0, &x, &y);
    CHECK_EQ(x, 15); CHECK_EQ(y, 13);
    cm_quadrant_origin(2, 1, &x, &y);
    CHECK_EQ(x, 15); CHECK_EQ(y, 2);

    /* 4-player: 2x2 arrangement, columns 8 and 21. */
    cm_quadrant_origin(4, 0, &x, &y);
    CHECK_EQ(x, 8);  CHECK_EQ(y, 13);
    cm_quadrant_origin(4, 1, &x, &y);
    CHECK_EQ(x, 8);  CHECK_EQ(y, 2);
    cm_quadrant_origin(4, 2, &x, &y);
    CHECK_EQ(x, 21); CHECK_EQ(y, 2);
    cm_quadrant_origin(4, 3, &x, &y);
    CHECK_EQ(x, 21); CHECK_EQ(y, 13);

    /* Boards + labels must fit the 40x25 grid: bottom grid ends row 22,
     * name label row 23, status row 24. */
    cm_quadrant_origin(4, 3, &x, &y);
    CHECK(y + 10 <= 23);
    CHECK_EQ(CM_PX(40), 320);
    CHECK_EQ(CM_PY(25), 200);

    /* Drawer sides: 2-player p0 left / p1 right; 4-player 0,1 left 2,3 right. */
    CHECK_EQ(cm_drawer_is_right(2, 0), 0);
    CHECK_EQ(cm_drawer_is_right(2, 1), 1);
    CHECK_EQ(cm_drawer_is_right(4, 1), 0);
    CHECK_EQ(cm_drawer_is_right(4, 2), 1);

    /* Drawer origins: left = grid_x-4, right = grid_x+11, one row below
     * the grid top. */
    cm_drawer_origin(4, 1, &x, &y);
    CHECK_EQ(x, 4);  CHECK_EQ(y, 3);
    cm_drawer_origin(4, 2, &x, &y);
    CHECK_EQ(x, 32); CHECK_EQ(y, 3);
    cm_drawer_origin(2, 1, &x, &y);
    CHECK_EQ(x, 26); CHECK_EQ(y, 3);

    /* Drawer panels stay on the 40-cell grid (3 cells wide). */
    cm_drawer_origin(4, 3, &x, &y);
    CHECK(x + 3 <= 40);

    /* Legend slot offsets (Atari legendShipOffset). */
    cm_legend_offset(0, &x, &y); CHECK_EQ(x, 2); CHECK_EQ(y, 0);
    cm_legend_offset(2, &x, &y); CHECK_EQ(x, 0); CHECK_EQ(y, 0);
    cm_legend_offset(3, &x, &y); CHECK_EQ(x, 0); CHECK_EQ(y, 5);
    cm_legend_offset(4, &x, &y); CHECK_EQ(x, 1); CHECK_EQ(y, 6);

    /* Ship segment tiles, horizontal and vertical, sizes 2..5. */
    CHECK_EQ(cm_ship_tile(0, 5, 0), TILE_SHIP_BOW_H);
    CHECK_EQ(cm_ship_tile(2, 5, 0), TILE_SHIP_MID_H);
    CHECK_EQ(cm_ship_tile(4, 5, 0), TILE_SHIP_STERN_H);
    CHECK_EQ(cm_ship_tile(0, 2, 0), TILE_SHIP_BOW_H);
    CHECK_EQ(cm_ship_tile(1, 2, 0), TILE_SHIP_STERN_H);
    CHECK_EQ(cm_ship_tile(0, 3, 1), TILE_SHIP_BOW_V);
    CHECK_EQ(cm_ship_tile(1, 3, 1), TILE_SHIP_MID_V);
    CHECK_EQ(cm_ship_tile(2, 3, 1), TILE_SHIP_STERN_V);

    /* Field overlay tiles. */
    CHECK_EQ(cm_field_tile(0), TILE_SEA);
    CHECK_EQ(cm_field_tile(CM_FIELD_ATTACK), TILE_HIT);
    CHECK_EQ(cm_field_tile(CM_FIELD_MISS), TILE_MISS);

    /* Update tiles: anim>9 = explosion frames (clamped), anim 1..9 blinks
     * a hit, anim 0 = final state. */
    CHECK_EQ(cm_update_tile(CM_FIELD_ATTACK, 10), TILE_ANIM_0);
    CHECK_EQ(cm_update_tile(CM_FIELD_ATTACK, 15), TILE_ANIM_5);
    CHECK_EQ(cm_update_tile(CM_FIELD_ATTACK, 200), TILE_ANIM_5);
    CHECK_EQ(cm_update_tile(CM_FIELD_ATTACK, 1), TILE_HIT2);
    CHECK_EQ(cm_update_tile(CM_FIELD_ATTACK, 0), TILE_HIT);
    CHECK_EQ(cm_update_tile(CM_FIELD_MISS, 0), TILE_MISS);
    CHECK_EQ(cm_update_tile(0, 0), TILE_SEA);

    return fn_test_report("test_cellmap");
}
