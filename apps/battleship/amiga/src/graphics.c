#include "misc.h"
#include <stdio.h>
#include <string.h>

static uint8_t playerCount_g = 0;
static uint8_t quadrant_offset[4][2];

static void goto_xy(uint8_t x, uint8_t y)
{
    printf("\033[%d;%dH", (int)y + 1, (int)x + 1);
}

void initGraphics(void)
{
    setbuf(stdout, NULL);
}

void resetGraphics(void) {}

void resetScreen(void)
{
    printf("\033[2J\033[H");
}

void waitvsync(void) {}

uint8_t cycleNextColor(void) { return 0; }

void drawText(uint8_t x, uint8_t y, const char *s)
{
    goto_xy(x, y);
    printf("%s", s);
}

void drawTextAlt(uint8_t x, uint8_t y, const char *s)
{
    goto_xy(x, y);
    printf("%s", s);
}

void drawIcon(uint8_t x, uint8_t y, uint8_t icon)
{
    goto_xy(x, y);
    printf("%c", (char)icon);
}

void drawBlank(uint8_t x, uint8_t y)
{
    goto_xy(x, y);
    printf(" ");
}

void drawSpace(uint8_t x, uint8_t y, uint8_t w)
{
    goto_xy(x, y);
    while (w--) printf(" ");
}

void drawLine(uint8_t x, uint8_t y, uint8_t w)
{
    goto_xy(x, y);
    while (w--) printf("-");
}

void drawBox(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t i;
    goto_xy(x, y);
    printf("+");
    for (i = 0; i < w; i++) printf("-");
    printf("+");
    for (i = 1; i <= h; i++) {
        goto_xy(x,     y + i); printf("|");
        goto_xy(x+w+1, y + i); printf("|");
    }
    goto_xy(x, y + h + 1);
    printf("+");
    for (i = 0; i < w; i++) printf("-");
    printf("+");
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
    printf("[T]");
}

void drawConnectionIcon(bool show)
{
    goto_xy(0, HEIGHT - 1);
    printf(show ? "[*]" : "[ ]");
}

void drawEndgameMessage(const char *msg)
{
    uint8_t len = (uint8_t)strlen(msg);
    uint8_t x   = (WIDTH - len) / 2;
    goto_xy(x, HEIGHT - 2);
    printf("%s", msg);
}

void drawPlayerName(uint8_t player, const char *name, bool active)
{
    uint8_t x = quadrant_offset[player][0];
    uint8_t y = quadrant_offset[player][1] - 1;
    goto_xy(x, y);
    if (active)
        printf("[%s]", name);
    else
        printf(" %s ", name);
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
        printf("%c", hide ? '.' : 'S');
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
        printf("%c", status ? 'S' : 'X');
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
            printf("%c", cell == FIELD_ATTACK ? 'X' :
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
    printf("%c", cell == FIELD_ATTACK ? 'X' :
                 cell == FIELD_MISS   ? 'o' : '.');
}

void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink)
{
    (void)gamefield;
    (void)blink;
    goto_xy(quadrant_offset[quadrant][0] + x,
            quadrant_offset[quadrant][1] + y);
    printf("@");
}

bool saveScreenBuffer(void)  { return false; }
void restoreScreenBuffer(void) {}
