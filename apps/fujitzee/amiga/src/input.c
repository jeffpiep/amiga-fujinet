/*
 * input.c - keyboard and joystick input
 *
 * Keyboard is the gamekit's shared IDCMP queue (gkkeyq.h), wrapped as the
 * cc65 <conio.h> pair upstream's misc.c calls: kbhit() polls without
 * blocking (readCommonInput polls the joystick in the same loop) and
 * cgetc() blocks only when nothing is buffered.
 *
 * Fujitzee needs KT_CURSOR_CTRL, not the gamekit's default WASD arrow
 * mapping: its lobby menu already answers to 's' (sound), 'r' (refresh),
 * 'c' (color), 'h' and 'q', and the name-entry screen accepts every letter
 * as text. An arrow key arriving as 'w' would type a W, and arrow-down
 * would toggle the sound. The control-code spelling keeps them apart —
 * see gkinput.h.
 *
 * Requires: intuition.library (window from gfxcore.c), exec.library
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * Joystick is game port 2 via the gamekit (gkjoy.h) — see readJoystick()
 * below for why "either port" collapses to one on this machine.
 *
 * See: docs/plan-track1c-fujitzee.md (Phase 2 keyboard, Phase 3b joystick)
 */
#include "misc.h"

#include "gkjoy.h"
#include "gkkeyq.h"
#include "keytrans.h"

/* The queue is a gamekit singleton with a WASD default, so the mode has to
 * be set before the first key is read. Done lazily rather than from
 * initGraphics() so the two platform files stay independent. */
static void ensureCursorMode(void)
{
    static uint8_t done;

    if (!done) {
        done = 1;
        gk_key_cursor_mode(KT_CURSOR_CTRL);
    }
}

unsigned char kbhit(void)
{
    ensureCursorMode();
    return gk_key_hit();
}

char cgetc(void)
{
    ensureCursorMode();
    return gk_key_get();
}

/*
 * Fujitzee's readJoystick() takes no port argument: the Atari port polls
 * both sticks and returns whichever is active. On the Amiga "both" is not
 * an option — port 1 shares its counter with the mouse, so polling it would
 * turn every mouse twitch into a phantom direction and make the game
 * unplayable for anyone who bumps the desk. So the answer to "either
 * joystick" here is game port 2, the port a stick is actually plugged into,
 * and the read itself is the gamekit's (gkjoy.h, extracted from battleship
 * in Phase 3b).
 *
 * With nothing plugged in this returns 0 every frame, which is what
 * readCommonInput() needs in order to fall through to the keyboard.
 */
unsigned char readJoystick(void)
{
    return gk_joy_read_port2();
}
