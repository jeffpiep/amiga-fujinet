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

/* Power-of-two ring buffer for decoded keys. */
#define KEYBUF_SIZE 8
#define KEYBUF_MASK (KEYBUF_SIZE - 1)
static char _keybuf[KEYBUF_SIZE];
static uint8_t _keybuf_head;
static uint8_t _keybuf_tail;

/* Drain every pending IDCMP message, pushing decoded keys into the ring
 * buffer (oldest keys drop if the game falls far behind). */
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

uint8_t readJoystick(void)
{
    return joyDecode(JOY1DAT, (uint8_t)!(CIAAPRA & 0x80));
}
