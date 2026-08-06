/*
 * gkkeyq.c - non-blocking keyboard queue over the gamekit window's IDCMP
 *
 * Keyboard comes from the backdrop window's IDCMP port (gfxcore.c opens the
 * window with RAWKEY|VANILLAKEY|MOUSEBUTTONS). Draining is all-or-nothing —
 * one GetMsg loop takes whatever is there — so mouse-button events are
 * tracked here too rather than dropped on the floor by a key poll.
 *
 * Requires: intuition.library (window from gfxcore.c), exec.library
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * Extracted from apps/battleship/amiga in Track 1C Phase 2 —
 * see docs/plan-track1c-fujitzee.md.
 */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/exec.h>

#include "gfxcore.h"
#include "gkkeyq.h"
#include "keytrans.h"

/* Power-of-two ring buffer for decoded keys. */
#define KEYBUF_SIZE 8
#define KEYBUF_MASK (KEYBUF_SIZE - 1)
static char _keybuf[KEYBUF_SIZE];
static uint8_t _keybuf_head;
static uint8_t _keybuf_tail;

static uint8_t _cursor_mode = KT_CURSOR_WASD;

/* Left mouse button held (from IDCMP MOUSEBUTTONS). */
static uint8_t _mouse_button;

/*
 * Drain every pending IDCMP message, pushing decoded keys into the ring
 * buffer (oldest keys drop if the game falls far behind) and tracking the
 * left mouse button state.
 */
static void drain(void)
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
        key = kt_decode_ex(cls == RAWKEY, code, _cursor_mode);
        if (key == KT_NONE)
            continue;
        if ((uint8_t)(_keybuf_head - _keybuf_tail) >= KEYBUF_SIZE)
            _keybuf_tail++;
        _keybuf[_keybuf_head & KEYBUF_MASK] = (char)key;
        _keybuf_head++;
    }
}

void gk_key_cursor_mode(uint8_t mode)
{
    _cursor_mode = mode;
}

uint8_t gk_key_hit(void)
{
    drain();
    return _keybuf_head != _keybuf_tail;
}

char gk_key_get(void)
{
    char ch;

    drain();
    while (_keybuf_head == _keybuf_tail) {
        Wait(1UL << gfx_window->UserPort->mp_SigBit);
        drain();
    }
    ch = _keybuf[_keybuf_tail & KEYBUF_MASK];
    _keybuf_tail++;
    return ch;
}

uint8_t gk_mouse_button(void)
{
    drain();
    return _mouse_button;
}
