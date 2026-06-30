#include "misc.h"
#include <dos/dos.h>
#include <proto/dos.h>

unsigned char kbhit(void)
{
    return (unsigned char)WaitForChar(Input(), 0);
}

char cgetc(void)
{
    char ch;
    Read(Input(), &ch, 1);
    return ch;
}

uint8_t readJoystick(void)
{
    return 0;
}
