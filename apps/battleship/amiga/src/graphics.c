#include "misc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dos/dos.h>
#include <proto/dos.h>
#undef FN_ERR_UNKNOWN
#include "fujinet-nio.h"

extern char playerName[12];

static uint8_t playerCount_g = 0;
static uint8_t quadrant_offset[4][2];

/*
 * Raw console for immediate, no-echo, per-keypress input on KS 1.3.
 *
 * CON: opens in cooked mode (line-buffered + echo): Read() blocks until Enter
 * and every key is echoed. SetMode() would switch it to raw, but SetMode() is
 * dos.library V36 (KS 2.0+) and crashes on KS 1.3. RAW: gives raw, no-echo,
 * per-keypress input using only V33 (KS 1.2+) calls (Open/Close/Read/Write/
 * WaitForChar), so it works on the Amiga 500 / KS 1.3 target.
 *
 * All game output is written directly to this handle (conPrintf) and all
 * input read from it (input.c). We do not rely on stdout following
 * SelectOutput(): bebbo's nix13 CRT binds stdout to the boot CLI handle at
 * startup, so printf() would render in the wrong window.
 */
BPTR g_rawConsole = 0;

static void closeRawConsole(void)
{
    if (g_rawConsole) {
        Close(g_rawConsole);
        g_rawConsole = 0;
    }
}

static void conPrintf(const char *fmt, ...)
{
    char buf[96];
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;
    if (g_rawConsole)
        Write(g_rawConsole, buf, (LONG)n);
    else
        fwrite(buf, 1, (size_t)n, stdout);
}

static void goto_xy(uint8_t x, uint8_t y)
{
    conPrintf("\033[%d;%dH", (int)y + 1, (int)x + 1);
}

void initGraphics(void)
{
    /* Raw, no-echo console window (NTSC: 640x200). Falls back to the boot
     * CLI (cooked mode) if the window can't be opened. */
    g_rawConsole = Open((CONST_STRPTR)"RAW:0/0/640/200/Battleship", MODE_NEWFILE);
    if (g_rawConsole)
        atexit(closeRawConsole);
    setbuf(stdout, NULL);

    /* Open serial.device and set baud = 19200 before game logic starts.
     * If ENVARC isn't available, the upstream init path blocks waiting
     * for keyboard input; ensure fallbacks are in place first. */
    fn_init();

    if (playerName[0] == '\0')
        strncpy(playerName, "amiga", 11);

    if (!prefs.seenHelp)
        prefs.seenHelp = 1;
}

void resetGraphics(void) {}

void resetScreen(void)
{
    conPrintf("\033[2J\033[H");
}

void waitvsync(void) {}

uint8_t cycleNextColor(void) { return 0; }

void drawText(uint8_t x, uint8_t y, const char *s)
{
    goto_xy(x, y);
    conPrintf("%s", s);
}

void drawTextAlt(uint8_t x, uint8_t y, const char *s)
{
    goto_xy(x, y);
    conPrintf("%s", s);
}

void drawIcon(uint8_t x, uint8_t y, uint8_t icon)
{
    goto_xy(x, y);
    conPrintf("%c", (char)icon);
}

void drawBlank(uint8_t x, uint8_t y)
{
    goto_xy(x, y);
    conPrintf(" ");
}

void drawSpace(uint8_t x, uint8_t y, uint8_t w)
{
    goto_xy(x, y);
    while (w--) conPrintf(" ");
}

void drawLine(uint8_t x, uint8_t y, uint8_t w)
{
    goto_xy(x, y);
    while (w--) conPrintf("-");
}

void drawBox(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t i;
    goto_xy(x, y);
    conPrintf("+");
    for (i = 0; i < w; i++) conPrintf("-");
    conPrintf("+");
    for (i = 1; i <= h; i++) {
        goto_xy(x,     y + i); conPrintf("|");
        goto_xy(x+w+1, y + i); conPrintf("|");
    }
    goto_xy(x, y + h + 1);
    conPrintf("+");
    for (i = 0; i < w; i++) conPrintf("-");
    conPrintf("+");
}

/*
 * Quadrant layout (x,y = top-left of 10x10 grid):
 *   2-player: both grids at the same row, left and right halves of 80 cols
 *   4-player: 2x2 arrangement, player 0 bottom-left clockwise
 *
 *   Row budget (2-player, grid at y=7):
 *     y=6:      player name
 *     y=7..16:  10x10 gamefield
 *     y=18..22: ship legend (5 slots, index 0-4)
 *     y=23:     game-over message (HEIGHT-2)
 *     y=24:     status bar (HEIGHT-1)
 */
void drawBoard(uint8_t playerCount)
{
    playerCount_g = playerCount;
    if (playerCount <= 2) {
        quadrant_offset[0][0] = 4;  quadrant_offset[0][1] = 7;
        quadrant_offset[1][0] = 44; quadrant_offset[1][1] = 7;
        quadrant_offset[2][0] = 44; quadrant_offset[2][1] = 7;
        quadrant_offset[3][0] = 4;  quadrant_offset[3][1] = 7;
    } else {
        quadrant_offset[0][0] = 4;  quadrant_offset[0][1] = 13;
        quadrant_offset[1][0] = 4;  quadrant_offset[1][1] = 2;
        quadrant_offset[2][0] = 44; quadrant_offset[2][1] = 2;
        quadrant_offset[3][0] = 44; quadrant_offset[3][1] = 13;
    }
    resetScreen();
}

void drawClock(void)
{
    goto_xy(WIDTH - 3, HEIGHT - 1);
    conPrintf("[T]");
}

void drawConnectionIcon(bool show)
{
    goto_xy(0, HEIGHT - 1);
    conPrintf(show ? "[*]" : "[ ]");
}

void drawEndgameMessage(const char *msg)
{
    uint8_t len = (uint8_t)strlen(msg);
    uint8_t x   = (WIDTH - len) / 2;
    goto_xy(x, HEIGHT - 2);
    conPrintf("%s", msg);
}

void drawPlayerName(uint8_t player, const char *name, bool active)
{
    uint8_t x = quadrant_offset[player][0];
    uint8_t y = quadrant_offset[player][1] - 1;
    goto_xy(x, y);
    if (active)
        conPrintf("[%s]", name);
    else
        conPrintf(" %s ", name);
}

void drawShip(uint8_t quadrant, uint8_t size, uint8_t pos, bool hide)
{
    uint8_t ox = quadrant_offset[quadrant][0];
    uint8_t oy = quadrant_offset[quadrant][1];
    uint8_t delta = 0;
    uint8_t sx, sy, i;
    if (pos > 99) { delta = 1; pos -= 100; }
    sx = pos % 10;
    sy = pos / 10;
    for (i = 0; i < size; i++) {
        if (delta)
            goto_xy(ox + sx,     oy + sy + i);
        else
            goto_xy(ox + sx + i, oy + sy);
        conPrintf("%c", hide ? '.' : 'S');
    }
}

/*
 * Legend sits 11 rows below the grid top (one gap after the 10-row grid).
 * In the 4-player layout the bottom grids are at y=13, so the legend would
 * land at y>=24 and run off screen — skip silently in that case.
 */
void drawLegendShip(uint8_t player, uint8_t index, uint8_t size, uint8_t status)
{
    uint8_t ox = quadrant_offset[player][0];
    uint8_t oy = quadrant_offset[player][1] + 11 + index;
    uint8_t i;
    if (oy >= HEIGHT) return;
    goto_xy(ox, oy);
    for (i = 0; i < size; i++)
        conPrintf("%c", status ? 'S' : 'X');
}

void drawGamefield(uint8_t quadrant, uint8_t *field)
{
    uint8_t ox = quadrant_offset[quadrant][0];
    uint8_t oy = quadrant_offset[quadrant][1];
    uint8_t row, col;
    for (row = 0; row < 10; row++) {
        goto_xy(ox, oy + row);
        for (col = 0; col < 10; col++) {
            uint8_t cell = field[row * 10 + col];
            conPrintf("%c", cell == FIELD_ATTACK ? 'X' :
                         cell == FIELD_MISS   ? 'o' : '.');
        }
    }
}

void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t anim)
{
    uint8_t ox   = quadrant_offset[quadrant][0];
    uint8_t oy   = quadrant_offset[quadrant][1];
    uint8_t cell = gamefield[attackPos];
    (void)anim;
    goto_xy(ox + attackPos % 10, oy + attackPos / 10);
    conPrintf("%c", cell == FIELD_ATTACK ? 'X' :
                 cell == FIELD_MISS   ? 'o' : '.');
}

void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink)
{
    (void)gamefield;
    (void)blink;
    goto_xy(quadrant_offset[quadrant][0] + x,
            quadrant_offset[quadrant][1] + y);
    conPrintf("@");
}

bool saveScreenBuffer(void)  { return false; }
void restoreScreenBuffer(void) {}
