/*
 * mousemap.c - mouse aiming math for the attack cursor (pure C99)
 *
 * Board geometry comes from cellmap.c so mouse hit-testing can never
 * drift from where the renderer actually draws the gamefields.
 */
#include "mousemap.h"
#include "cellmap.h"

uint8_t mm_aim_cell(uint8_t player_count, int16_t px, int16_t py,
                    uint8_t *cell_x, uint8_t *cell_y)
{
    uint8_t q, ox, oy;
    int16_t left, top, dx, dy;

    for (q = 1; q < player_count && q < 4; q++) {
        cm_quadrant_origin(player_count, q, &ox, &oy);
        left = CM_PX(ox);
        top  = CM_PY(oy);
        dx = px - left;
        dy = py - top;
        if (dx < 0 || dy < 0 ||
            dx >= 10 * CM_CELL_W || dy >= 10 * CM_CELL_H)
            continue;
        *cell_x = (uint8_t)(dx / CM_CELL_W);
        *cell_y = (uint8_t)(dy / CM_CELL_H);
        return 1;
    }
    return 0;
}

uint8_t mm_chase_bits(uint8_t cur_x, uint8_t cur_y,
                      uint8_t tgt_x, uint8_t tgt_y)
{
    uint8_t bits = 0;

    if (tgt_x < cur_x)
        bits |= MM_LEFT;
    else if (tgt_x > cur_x)
        bits |= MM_RIGHT;
    if (tgt_y < cur_y)
        bits |= MM_UP;
    else if (tgt_y > cur_y)
        bits |= MM_DOWN;
    return bits;
}
