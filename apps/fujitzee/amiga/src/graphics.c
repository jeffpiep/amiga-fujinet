/*
 * graphics.c — Phase 1 stubs for upstream/src/platform-specific/graphics.h
 *
 * Phase 1's goal is that the upstream sources compile and every symbol
 * resolves; the real renderer arrives in Phase 2 on the gamekit's custom
 * 320x200x4 screen (gfxcore.h). Unlike the Battleship port there is no
 * throwaway ANSI-console renderer in between — these are stubs, not a
 * renderer we plan to delete (docs/plan-track1c-fujitzee.md, change 2).
 *
 * The one stub with real behaviour is waitvsync(): pause() is
 * `while (frames--) waitvsync()`, so a no-op would turn every upstream
 * animation delay into a busy spin.
 */
#include <stdbool.h>
#include <stdint.h>

#include <proto/dos.h>

#include "misc.h"

/* Declared extern by graphics.h; owned by the platform layer. Index into
 * whatever palette set Phase 2 gives the screen. */
unsigned char colorMode = 0;

void initGraphics(void) {}
void resetGraphics(void) {}

void resetScreen(bool forBorderScreen)
{
    (void)forBorderScreen;
}

/*
 * One dos.library tick is 1/50 s — the same unit the gamekit clock counts,
 * and close enough to a frame for the Phase 1 stub. Phase 2 replaces this
 * with a real WaitTOF() on the custom screen.
 */
void waitvsync(void)
{
    Delay(1);
}

void drawText(unsigned char x, unsigned char y, char *s)
{
    (void)x; (void)y; (void)s;
}

void drawTextAlt(unsigned char x, unsigned char y, char *s)
{
    (void)x; (void)y; (void)s;
}

void drawChar(unsigned char x, unsigned char y, char c, unsigned char alt)
{
    (void)x; (void)y; (void)c; (void)alt;
}

void drawIcon(unsigned char x, unsigned char y, unsigned char icon)
{
    (void)x; (void)y; (void)icon;
}

void drawFujitzee(unsigned char x, unsigned char y)
{
    (void)x; (void)y;
}

void drawDie(unsigned char x, unsigned char y, unsigned char s,
             bool isSelected, bool isHighlighted)
{
    (void)x; (void)y; (void)s; (void)isSelected; (void)isHighlighted;
}

void drawClock(unsigned char x, unsigned char y)
{
    (void)x; (void)y;
}

void drawConnectionIcon(unsigned char x, unsigned char y)
{
    (void)x; (void)y;
}

void drawBlank(unsigned char x, unsigned char y)
{
    (void)x; (void)y;
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w)
{
    (void)x; (void)y; (void)w;
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    (void)x; (void)y; (void)w;
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    (void)x; (void)y; (void)w; (void)h;
}

void drawBorder(void) {}
void drawBoard(void) {}
void clearBelowBoard(void) {}

void drawDiceCursor(unsigned char x)
{
    (void)x;
}

void hideDiceCursor(unsigned char x)
{
    (void)x;
}

/*
 * Returning false tells screens.c the platform has no screen buffer, so it
 * redraws instead of restoring. Phase 2 can back this with a second bitmap.
 */
bool saveScreenBuffer(void)
{
    return false;
}

void restoreScreenBuffer(void) {}

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash)
{
    (void)player; (void)isThisPlayer; (void)flash;
}

/* Cycles the player-colour preference; returns the newly selected mode. */
uint8_t cycleNextColor(void)
{
    return colorMode;
}

/*
 * Declared `void setColorMode()` upstream — unprototyped — but always called
 * with the stored preference. The parameter must be `int`, not
 * `unsigned char`: against an unprototyped declaration the definition's
 * types are compared after the default argument promotions, and a narrower
 * type is a hard "conflicting types" error under GCC.
 */
void setColorMode(int mode)
{
    colorMode = (unsigned char)mode;
}
