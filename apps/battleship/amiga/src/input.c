#include "misc.h"
#include <dos/dos.h>
#include <proto/dos.h>

/* Raw, no-echo console opened in graphics.c (initGraphics). When it is open
 * we read keys from it directly so each keypress arrives immediately without
 * echo and without waiting for Enter. Falls back to the cooked boot CLI
 * (Input()) if the RAW: window could not be opened. */
extern BPTR g_rawConsole;

static BPTR inputHandle(void)
{
    return g_rawConsole ? g_rawConsole : Input();
}

unsigned char kbhit(void)
{
    return (unsigned char)WaitForChar(inputHandle(), 0);
}

char cgetc(void)
{
    char ch;
    Read(inputHandle(), &ch, 1);
    return ch;
}

uint8_t readJoystick(void)
{
    return 0;
}
