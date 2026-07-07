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

#include "joydecode.h"

/* Joystick port 2 — the customary Amiga game port (port 1 has the mouse,
 * whose movement would register as phantom directions). Read-only peeks at
 * the counter/CIA registers; conventional and safe alongside the OS. */
#define JOY1DAT (*(volatile uint16_t *)0xDFF00C)
#define CIAAPRA (*(volatile uint8_t *)0xBFE001) /* bit 7 = port-2 fire, active low */

uint8_t readJoystick(void)
{
    return joyDecode(JOY1DAT, (uint8_t)!(CIAAPRA & 0x80));
}
