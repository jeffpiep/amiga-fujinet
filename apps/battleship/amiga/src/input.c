#include "misc.h"
#include <dos/dos.h>
#include <proto/dos.h>

/* FGetC is not prototyped in all versions of the bebbo toolchain headers */
extern LONG FGetC(BPTR fh);

unsigned char kbhit(void)
{
    return (unsigned char)WaitForChar(Input(), 0);
}

char cgetc(void)
{
    return (char)FGetC(Input());
}

uint8_t readJoystick(void)
{
    return 0;
}
