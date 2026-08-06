/*
 * preview_main.c - board layout preview harness (no server, no game)
 *
 * The scorecard only appears once a real multiplayer game is under way,
 * which makes the riskiest drawing in the port — a 13-row score grid with
 * six player columns on 40x25 cells — the one thing an automated boot test
 * never renders. This harness links the real graphics.c against a handful
 * of fake game values and draws a full board: lattice, score names, a
 * column of scores per player, the dice row with a kept die and the roll
 * button, both cursors, the active-column brackets, and the status line.
 *
 * Same idea as battleship's tilegallery, one level up: that one previews
 * tiles, this one previews the layout they compose into.
 *
 * Build:  make boardpreview
 * Boot :  make preview-adf   then boot boardpreview.adf in FS-UAE
 *
 * See: docs/plan-track1c-fujitzee.md (Phase 2, Phase 3c art pass)
 */
#include "misc.h"

#include <string.h>

#include <proto/graphics.h>   /* WaitTOF */

#include "fjlayout.h"

/*
 * Normally gamelogic.c's; the harness deliberately does not link the game,
 * so it owns these instead. Values copied from upstream — if they drift,
 * the preview stops matching what the game draws, which is exactly the
 * kind of thing this harness exists to show.
 */
uint8_t scoreY[] = { 3,4,5,6,7,8,10,11,13,14,15,16,17,18,19,21 };
char *scores[]   = { "one","two","three","four","five","six","total","bonus",
                     "set 3","set 4","house","s run","l run","count" };

static const char *const names[FJ_PLAYERS_SHOWN] = {
    "amy", "bob", "cyd", "dev", "eli", "fay"
};

/* A plausible mid-game column of scores, so the numbers land where the
 * game would put them (right-aligned in the 3-cell column). */
static const int8_t colScores[16] = {
    3, 8, 9, 12, 10, 18, 60, 0, 22, 25, 25, 30, 40, 0, 50, 99
};

static void drawScoreColumn(uint8_t player, uint8_t rows)
{
    char buf[4];
    uint8_t j, len;
    uint8_t x = (uint8_t)FJ_COL_X(player);

    for (j = 0; j < rows && j < 15; j++) {
        int v = colScores[(j + player) & 15];

        itoa(v, buf, 10);
        len = (uint8_t)strlen(buf);
        drawText((unsigned char)(x + 3 - len), scoreY[j], buf);
    }
}

int main(void)
{
    uint8_t i;

    initGraphics();

    resetScreen(false);
    drawBoard();

    /* Player names across the header row, the local player in alt. */
    for (i = 0; i < FJ_PLAYERS_SHOWN; i++) {
        uint8_t x = (uint8_t)FJ_COL_X(i);
        uint8_t c;

        for (c = 0; c < 3 && names[i][c]; c++)
            drawChar((unsigned char)(x + c), 1, names[i][c], i == 2);
        drawScoreColumn(i, (uint8_t)(15 - i));
    }

    /* Left panel: round counter and the player list. */
    drawTextAlt(1, 3, "round ");
    drawText(7, 3, "7");
    drawTextAlt(1, 4, "   of ");
    drawText(7, 4, "13");
    for (i = 0; i < FJ_PLAYERS_SHOWN; i++)
        drawText(1, (uint8_t)(6 + i), (char *)names[i]);
    drawIcon(0, 8, ICON_MARK);

    /* Bottom panel: prompt, timer, five dice with one held, the roll
     * button, the dice cursor, and the connection icon. */
    drawText(0, FJ_HEIGHT - 4, "your turn");
    /* Countdown reads right-to-left into the clock, as gamelogic.c draws
     * it: the number ends where the icon begins. */
    drawTextAlt(TIMER_X - 2, FJ_HEIGHT - 4, "12");
    drawClock(TIMER_X, FJ_HEIGHT - 4);

    for (i = 0; i < 5; i++)
        drawDie((unsigned char)(FJ_WIDTH - 20 + 4 * i), FJ_DICE_Y,
                (unsigned char)(i + 1), i == 1, i == 4);
    drawDie(ROLL_X, FJ_DICE_Y, 15, 0, 0);
    drawDiceCursor((unsigned char)(FJ_WIDTH - 20 + 4 * 2));

    setHighlight(2, true, 0);

    drawConnectionIcon(0, FJ_HEIGHT - 1);
    drawTextAlt(2, FJ_HEIGHT - 1, "board preview - press ctrl-c to quit");

    /* Idle so the emulator harness can screenshot the finished board. */
    for (;;)
        WaitTOF();
}
