/*
 * aim.h - mouse-aim handshake between the renderer and input.c
 *
 * graphics.c arms attack mode from drawGamefieldCursor and placement mode
 * from the placement blink cycle in drawShip; both clear on redraws and
 * updates. input.c polls this each frame to decide whether mouse chasing is
 * live, which board it targets, and where the cursor/ship currently sits.
 *
 * Battleship-specific (fujitzee has no mouse aiming), so it stays here
 * rather than in libs/amiga-gamekit — see docs/plan-track1c-fujitzee.md
 * Phase 0.
 */
#ifndef AIM_H
#define AIM_H

#include <stdint.h>

#define GFX_AIM_NONE   0
#define GFX_AIM_ATTACK 1   /* chase target: enemy-field cell (attack cursor) */
#define GFX_AIM_PLACE  2   /* chase target: own-field cell (ship origin)     */

void gfx_aim_set(uint8_t player_count, uint8_t cell_x, uint8_t cell_y);
void gfx_aim_set_place(uint8_t player_count, uint8_t cell_x, uint8_t cell_y,
                       uint8_t ship_size, uint8_t ship_vertical);
void gfx_aim_clear(void);
/* Returns the GFX_AIM_* mode; out-params valid when non-NONE. */
uint8_t gfx_aim_get(uint8_t *player_count, uint8_t *cell_x, uint8_t *cell_y);
/* Mover ship shape (valid in GFX_AIM_PLACE mode). */
void gfx_aim_get_ship(uint8_t *ship_size, uint8_t *ship_vertical);

#endif /* AIM_H */
