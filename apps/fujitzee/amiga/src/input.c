/*
 * input.c — Phase 1 stubs for the keyboard/joystick surface
 *
 * Upstream needs three entry points: readJoystick()
 * (platform-specific/input.h) plus kbhit()/cgetc(), which misc.c calls
 * through the cc65 <conio.h> our include/conio.h shims.
 *
 * Phase 2 wires kbhit()/cgetc() to the gamekit's kt_decode() over an IDCMP
 * pump, and Phase 3b wires readJoystick() to joyDecode(). Note that the
 * IDCMP pump and the raw JOYxDAT/CIAA reads currently live in
 * apps/battleship/amiga/src/input.c, not in the gamekit — see the extraction
 * note on Phase 3 in docs/plan-track1c-fujitzee.md before duplicating them.
 */
#include "misc.h"

/*
 * Fujitzee's readJoystick() takes no port argument: the Atari port polls
 * both sticks and returns whichever is active. The Amiga equivalent (port 1
 * mouse-port joystick + port 2) is Phase 3b.
 */
unsigned char readJoystick(void)
{
    return 0;
}

unsigned char kbhit(void)
{
    return 0;
}

char cgetc(void)
{
    return 0;
}
