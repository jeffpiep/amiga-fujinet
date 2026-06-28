#include "misc.h"
#include <stdio.h>

/* Phase 1 stubs — keyboard and joystick input */

unsigned char kbhit(void)
{
    return 0; /* no key pending */
}

char cgetc(void)
{
    return (char)getchar(); /* blocking read; only called when kbhit() != 0 */
}

uint8_t readJoystick(void)
{
    return 0; /* no joystick input */
}
