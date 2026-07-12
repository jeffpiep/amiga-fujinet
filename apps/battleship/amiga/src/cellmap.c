/*
 * cellmap.c - battleship board layout math (pure C99, no AmigaOS headers)
 *
 * Layout model is the Atari renderer (upstream/src/atari/graphics.c),
 * squeezed from its 40x26 grid to our 40x25:
 *
 *   row  0..1   title / status text
 *   row  2..11  top boards (quadrants 1, 2)
 *   row 12      shared border row between top and bottom boards
 *   row 13..22  bottom boards (quadrants 0, 3)
 *   row 23      bottom name labels
 *   row 24      status bar (HEIGHT-1)
 *
 * Columns: grids at cells 8 and 21, shifted +7 (cm_field_x) when only two
 * boards are shown so the pair sits centered. Legend drawers are 3-wide
 * panels at grid_x-4 (left) or grid_x+11 (right).
 */
#include "cellmap.h"

#define GRID_X_LEFT   8
#define GRID_X_RIGHT  21
#define GRID_Y_TOP    2
#define GRID_Y_BOTTOM 13

uint8_t cm_field_x(uint8_t player_count)
{
    return (player_count > 2) ? 0 : 7;
}

void cm_quadrant_origin(uint8_t player_count, uint8_t quadrant,
                        uint8_t *cx, uint8_t *cy)
{
    uint8_t fx = cm_field_x(player_count);

    *cx = ((quadrant == 0 || quadrant == 1) ? GRID_X_LEFT : GRID_X_RIGHT) + fx;
    *cy = (quadrant == 1 || quadrant == 2) ? GRID_Y_TOP : GRID_Y_BOTTOM;
}

uint8_t cm_drawer_is_right(uint8_t player_count, uint8_t player)
{
    return (player > 1 || (player_count == 2 && player > 0)) ? 1 : 0;
}

void cm_drawer_origin(uint8_t player_count, uint8_t player,
                      uint8_t *cx, uint8_t *cy)
{
    uint8_t gx, gy;

    cm_quadrant_origin(player_count, player, &gx, &gy);
    *cx = cm_drawer_is_right(player_count, player) ? (uint8_t)(gx + 11)
                                                   : (uint8_t)(gx - 4);
    *cy = (uint8_t)(gy + 1);
}

void cm_legend_offset(uint8_t index, uint8_t *dx, uint8_t *dy)
{
    /* Atari legendShipOffset[] = {2, 1, 0, 40*5, 40*6+1} as (dx, dy). */
    static const uint8_t offs[CM_LEGEND_SLOTS][2] = {
        {2, 0}, {1, 0}, {0, 0}, {0, 5}, {1, 6}
    };

    if (index >= CM_LEGEND_SLOTS)
        index = CM_LEGEND_SLOTS - 1;
    *dx = offs[index][0];
    *dy = offs[index][1];
}

uint8_t cm_ship_tile(uint8_t i, uint8_t size, uint8_t vertical)
{
    if (i == 0)
        return vertical ? TILE_SHIP_BOW_V : TILE_SHIP_BOW_H;
    if (i + 1 >= size)
        return vertical ? TILE_SHIP_STERN_V : TILE_SHIP_STERN_H;
    return vertical ? TILE_SHIP_MID_V : TILE_SHIP_MID_H;
}

uint8_t cm_field_tile(uint8_t cell)
{
    if (cell == CM_FIELD_ATTACK)
        return TILE_HIT;
    if (cell == CM_FIELD_MISS)
        return TILE_MISS;
    return TILE_SEA;
}

uint8_t cm_update_tile(uint8_t cell, uint8_t anim)
{
    if (anim > 9) {
        uint8_t frame = (uint8_t)(anim - 10);

        if (frame > 5)
            frame = 5;
        return (uint8_t)(TILE_ANIM_0 + frame);
    }
    if (cell == CM_FIELD_ATTACK)
        return anim ? TILE_HIT2 : TILE_HIT;
    if (cell == CM_FIELD_MISS)
        return TILE_MISS;
    return TILE_SEA;
}
