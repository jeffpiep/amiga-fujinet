/*
 * input.c - keyboard (IDCMP) and joystick input
 *
 * Keyboard comes from the backdrop window's IDCMP port (gfxcore.c opens
 * the window with RAWKEY|VANILLAKEY). kbhit() drains pending messages into
 * a small ring buffer and never blocks — upstream's readCommonInput polls
 * the joystick in the same loop. cgetc() pops the buffer, Wait()ing on the
 * window signal only when it is empty (which is exactly when upstream
 * expects to block). Translation is pure logic in keytrans.c (T1-tested).
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
#include <proto/exec.h>

#include "gfxcore.h"
#include "keytrans.h"
#include "mousemap.h"

/* Power-of-two ring buffer for decoded keys. */
#define KEYBUF_SIZE 8
#define KEYBUF_MASK (KEYBUF_SIZE - 1)
static char _keybuf[KEYBUF_SIZE];
static uint8_t _keybuf_head;
static uint8_t _keybuf_tail;

/* Left mouse button held (from IDCMP MOUSEBUTTONS). */
static uint8_t _mouse_button;

/* Drain every pending IDCMP message, pushing decoded keys into the ring
 * buffer (oldest keys drop if the game falls far behind) and tracking the
 * left mouse button state. */
static void drainKeyMessages(void)
{
    struct IntuiMessage *msg;

    if (!gfx_window)
        return;
    while ((msg = (struct IntuiMessage *)GetMsg(gfx_window->UserPort))) {
        ULONG cls = msg->Class;
        UWORD code = msg->Code;
        int16_t key;

        ReplyMsg((struct Message *)msg);
        if (cls == MOUSEBUTTONS) {
            if (code == SELECTDOWN)
                _mouse_button = 1;
            else if (code == SELECTUP)
                _mouse_button = 0;
            continue;
        }
        if (cls != RAWKEY && cls != VANILLAKEY)
            continue;
        key = kt_decode(cls == RAWKEY, code);
        if (key == KT_NONE)
            continue;
        if ((uint8_t)(_keybuf_head - _keybuf_tail) >= KEYBUF_SIZE)
            _keybuf_tail++;
        _keybuf[_keybuf_head & KEYBUF_MASK] = (char)key;
        _keybuf_head++;
    }
}

unsigned char kbhit(void)
{
    drainKeyMessages();
    return _keybuf_head != _keybuf_tail;
}

char cgetc(void)
{
    char ch;

    drainKeyMessages();
    while (_keybuf_head == _keybuf_tail) {
        Wait(1UL << gfx_window->UserPort->mp_SigBit);
        drainKeyMessages();
    }
    ch = _keybuf[_keybuf_tail & KEYBUF_MASK];
    _keybuf_tail++;
    return ch;
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

    drainKeyMessages();   /* freshen button state */
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

    if (_mouse_button && in_field && cx == tx && cy == ty)
        bits |= MM_FIRE;
    return bits;
}

uint8_t readJoystick(void)
{
    uint8_t joy = joyDecode(JOY1DAT, (uint8_t)!(CIAAPRA & 0x80));

    return joy | mouseAimBits(joy);
}
