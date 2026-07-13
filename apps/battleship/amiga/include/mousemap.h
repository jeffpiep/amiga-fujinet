/*
 * mousemap.h - mouse aiming math for the attack cursor (pure C99)
 *
 * Maps a screen-pixel mouse position onto an enemy gamefield cell and
 * computes the joystick-style direction bits that walk the game cursor
 * toward it. The game's input contract only accepts step deltas, so the
 * mouse "chases" the cursor to the hovered cell — at one step per frame
 * pair that reads as instant snapping (see input.c).
 *
 * Pure logic: T1 host-testable (test/host/test_mousemap.c).
 *
 * See: docs/plan-track1b-battleship.md (Phase 3c)
 */
#ifndef MOUSEMAP_H
#define MOUSEMAP_H

#include <stdint.h>

/* Direction/fire bits — same layout as readJoystick() / the JOY_* macros
 * in amiga_vars.h (asserted equivalent in test_mousemap.c). */
#define MM_UP    0x01
#define MM_DOWN  0x02
#define MM_LEFT  0x04
#define MM_RIGHT 0x08
#define MM_FIRE  0x10

/*
 * If screen pixel (px, py) lies inside an *enemy* gamefield (quadrants
 * 1..player_count-1; quadrant 0 is the player's own board), store the
 * 0..9 cell coordinates and return 1. Returns 0 outside every enemy field.
 */
uint8_t mm_aim_cell(uint8_t player_count, int16_t px, int16_t py,
                    uint8_t *cell_x, uint8_t *cell_y);

/*
 * One chase step: direction bits (diagonals allowed) moving cursor cell
 * (cur_x, cur_y) toward (tgt_x, tgt_y); 0 when already there.
 */
uint8_t mm_chase_bits(uint8_t cur_x, uint8_t cur_y,
                      uint8_t tgt_x, uint8_t tgt_y);

#endif /* MOUSEMAP_H */
