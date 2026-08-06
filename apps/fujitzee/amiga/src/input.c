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
 * See: docs/plan-track1c-fujitzee.md (Phase 2 keyboard, Phase 3b joystick)
 */
#include "misc.h"

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
 * Phase 3b. Fujitzee's readJoystick() takes no port argument — the Atari
 * port polls both sticks and returns whichever is active — so it differs
 * from battleship's port-2-only read, and the raw JOYxDAT/CIAA reads it
 * needs still live in apps/battleship/amiga/src/input.c. See the Phase 3
 * extraction note in docs/plan-track1c-fujitzee.md before duplicating
 * them. Until then the game is keyboard-driven, which is enough to play.
 */
unsigned char readJoystick(void)
{
    return 0;
}
