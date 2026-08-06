/*
 * input.c - keyboard (IDCMP) and joystick input
 *
 * Keyboard is the gamekit's shared queue (gkkeyq.h): it drains the backdrop
 * window's IDCMP port into a ring buffer, so kbhit() never blocks —
 * upstream's readCommonInput polls the joystick in the same loop — and
 * cgetc() blocks only when the buffer is empty, which is exactly when
 * upstream expects to block. Battleship keeps the default KT_CURSOR_WASD
 * arrow mapping; its menus don't compete for those letters.
 *
 * Joystick port 2 — the customary Amiga game port (port 1 has the mouse,
 * whose movement would register as phantom directions). Read-only peeks at
 * the counter/CIA registers; conventional and safe alongside the OS.
 *
 * Requires: intuition.library (window from gfxcore.c), exec.library
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * See: docs/plan-track1b-battleship.md (Phase 3c)
 */
#include "misc.h"

#include <exec/types.h>
#include <intuition/intuition.h>

#include "gfxcore.h"
#include "aim.h"
#include "gkkeyq.h"
#include "mousemap.h"

unsigned char kbhit(void)
{
    return gk_key_hit();
}

char cgetc(void)
{
    return gk_key_get();
}

#include "joydecode.h"

#define JOY1DAT (*(volatile uint16_t *)0xDFF00C)
#define CIAAPRA (*(volatile uint8_t *)0xBFE001) /* bit 7 = port-2 fire, active low */

/*
 * Mouse aiming (attack cursor only — armed by drawGamefieldCursor via
 * gfx_aim_set). Moving the mouse over an enemy gamefield blanks the
 * Intuition pointer and "chases" the game cursor to the hovered cell by
 * blending joystick-style direction bits into readJoystick()'s result;
 * the left button maps to the trigger.
 *
 * The chase pulses bits on alternating frames: upstream readCommonInput()
 * treats an unchanged joystick value as a held stick and imposes a
 * ~12-frame repeat delay, so a steady direction would crawl. Pulsing
 * looks like a fresh press every other frame — one cell per frame pair,
 * ~30 cells/s, which reads as snapping on a 10x10 board.
 *
 * Authority rules: the mouse only takes over when it moves (so joystick
 * and keyboard aiming don't fight a parked mouse), a real joystick input
 * cancels the chase, and the fire bit passes only while the cursor sits
 * on the hovered cell — the shot always lands where the player sees the
 * cursor (no fire-at-a-stale-target).
 */
static int16_t _mouse_last_mx = -1;
static int16_t _mouse_last_my = -1;
static uint8_t _mouse_chasing;
static uint8_t _mouse_tgt_x;
static uint8_t _mouse_tgt_y;
static uint8_t _mouse_pulse;

static uint8_t _mouse_last_mode;

static uint8_t mouseAimBits(uint8_t real_joy)
{
    uint8_t players, cx, cy, tx, ty, in_field, mode;
    uint8_t bits = 0;
    int16_t mx, my;

    if (!gfx_window)
        return 0;
    mode = gfx_aim_get(&players, &cx, &cy);
    if (mode == GFX_AIM_NONE) {
        _mouse_chasing = 0;
        _mouse_last_mode = mode;
        return 0;
    }
    if (mode != _mouse_last_mode) {
        _mouse_chasing = 0;         /* aim retargeted (attack <-> place) */
        _mouse_last_mode = mode;
    }

    mx = gfx_window->MouseX;
    my = gfx_window->MouseY;
    if (mode == GFX_AIM_PLACE) {
        /* Drag the moving ship's origin around the player's own board;
         * clamp so the whole ship stays on it. */
        in_field = mm_place_cell(players, mx, my, &tx, &ty);
        if (in_field) {
            uint8_t ship_size, ship_vert;

            gfx_aim_get_ship(&ship_size, &ship_vert);
            mm_clamp_origin(ship_size, ship_vert, &tx, &ty);
        }
    } else {
        in_field = mm_aim_cell(players, mx, my, &tx, &ty);
    }

    gfx_pointer_blank(in_field);

    if (real_joy) {
        _mouse_chasing = 0;         /* real stick wins */
    } else {
        if (in_field && (mx != _mouse_last_mx || my != _mouse_last_my)) {
            _mouse_chasing = 1;
            _mouse_tgt_x = tx;
            _mouse_tgt_y = ty;
        }
        if (_mouse_chasing) {
            if (cx == _mouse_tgt_x && cy == _mouse_tgt_y) {
                _mouse_chasing = 0;
            } else {
                _mouse_pulse ^= 1;
                if (_mouse_pulse)
                    bits |= mm_chase_bits(cx, cy,
                                          _mouse_tgt_x, _mouse_tgt_y);
            }
        }
    }
    _mouse_last_mx = mx;
    _mouse_last_my = my;

    /* gk_mouse_button() drains the port first, so the button state is as
     * fresh as the MouseX/MouseY read above. */
    if (gk_mouse_button() && in_field && cx == tx && cy == ty)
        bits |= MM_FIRE;
    return bits;
}

uint8_t readJoystick(void)
{
    uint8_t joy = joyDecode(JOY1DAT, (uint8_t)!(CIAAPRA & 0x80));

    return joy | mouseAimBits(joy);
}
