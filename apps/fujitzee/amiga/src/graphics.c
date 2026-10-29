/*
 * graphics.c - upstream's drawing contract on the gamekit tile renderer
 *
 * Phase 2: implements upstream/src/platform-specific/graphics.h on a
 * 320x200x4 custom screen as a 40x25 grid of 8x8 cells. Text goes through
 * the screen's topaz-8 font, everything else is a tile. Layout math lives
 * in fjlayout.c (host-tested), OS plumbing in the shared gamekit
 * (libs/amiga-gamekit/src/gfxcore.c), screen config in gfxsetup.c, art in
 * tiles.h.
 *
 * The board layout follows upstream's DOS renderer (src/msdos/graphics.c)
 * rather than the Atari one: both it and we have 25 rows where the Atari
 * has 26, and it already re-derived the bottom-panel offsets and the
 * SCORES_X shift that the missing row forces.
 *
 * Requires: intuition.library, graphics.library (V33 / KS 1.3)
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * See: docs/plan-track1c-fujitzee.md (Phase 2)
 */
#include "misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/graphics.h>   /* WaitTOF */

#undef FN_ERR_UNKNOWN
#include "fujinet-nio.h"

#include "gfxcore.h"
#include "gfxsetup.h"
#include "fjlayout.h"
#include "pens.h"

/* Declared extern by graphics.h; owned by the platform layer. */
unsigned char colorMode = 0;

/* Face pen per FJ_STYLE_*, for text drawn on top of a die body. */
static const uint8_t _diePen[FJ_DIE_STYLES] = {
    PEN_DIE, PEN_DIE_KEEP, PEN_DIE_HI
};

/* Active-player column: which player is bracketed, and with which tile, so
 * the brackets can be lifted again when the turn moves on. 0xFF = none. */
#define HI_NONE 0xFF
static uint8_t _hiPlayer = HI_NONE;
static uint8_t _hiTile   = TILE_HI_OTHER;

/* Dice cursor: where it was last drawn. Upstream's hide calls can carry a
 * stale x (it advances its cursor variable before sending the hide), so we
 * always erase where we actually drew — the DOS port does the same. */
static uint8_t _curX;
static uint8_t _curShown;

/* Saved copies of both, for save/restoreScreenBuffer: the bitmap comes
 * back but the bookkeeping about what is drawn on it has to as well. */
static uint8_t _savedHiPlayer, _savedHiTile, _savedCurX, _savedCurShown;
static uint8_t _haveSaved;

/* ---- Setup ------------------------------------------------------------ */

void initGraphics(void)
{
    if (!gfx_open(&fj_gfx_config)) {
        /* nix13 binds stdout to the boot CLI, so this lands somewhere
         * visible even though our screen never opened. */
        printf("fujitzee: can't open screen (chip RAM?)\n");
        exit(1);
    }

    /* Nothing in fujitzee reads the mouse — every cursor is keyboard or
     * joystick driven — so the Intuition pointer is only ever in the way
     * of the dice it happens to sit on. */
    gfx_pointer_blank(1);

    /* Open serial.device and set baud = 19200 before game logic starts.
     * The compat layer would do it lazily on the first network_open(), but
     * failing here is far easier to diagnose than failing mid-lobby. */
    fn_init();
}

void resetGraphics(void) {}   /* teardown runs via gfx_close atexit */

void waitvsync(void)
{
    WaitTOF();
}

/* ---- Screen clearing -------------------------------------------------- */

void resetScreen(bool forBorderScreen)
{
    uint8_t r;

    _hiPlayer = HI_NONE;
    _curShown = 0;

    if (!forBorderScreen) {
        gfx_fill(0, 0, FJ_WIDTH, FJ_HEIGHT, PEN_BG);
        return;
    }

    /* Bordered screens keep the four corner dice screens.c drew, so clear
     * the middle band full width and the top and bottom bands only between
     * them. */
    gfx_fill(0, 3, FJ_WIDTH, FJ_HEIGHT - 6, PEN_BG);
    for (r = 0; r < 3; r++) {
        gfx_fill(3, r, FJ_WIDTH - 6, 1, PEN_BG);
        gfx_fill(3, (uint8_t)(FJ_HEIGHT - 1 - r), FJ_WIDTH - 6, 1, PEN_BG);
    }
}

void clearBelowBoard(void)
{
    _curShown = 0;
    gfx_fill(0, FJ_HEIGHT - 4, FJ_WIDTH, 4, PEN_BG);
}

/* ---- Text ------------------------------------------------------------- */

/*
 * Both text entry points first ask whether this draw is a score that has
 * grown wider than its column (see fj_score_spill). If it has, the digits
 * are squeezed into the column rather than allowed to start on the divider
 * and cut the board lattice in half. Returns 1 when it handled the draw.
 *
 * The squeeze reaches FJ_SPILL_BLEED pixels into each neighbouring divider
 * cell, and an ordinary draw over the same column later would not clear
 * them again — gfx_text only owns whole cells. Harmless here, because a
 * committed score never changes and resetScreen() clears the board between
 * games, but it is why this stays a leaf case instead of becoming the way
 * text is drawn generally.
 */
static uint8_t drawSpilledScore(unsigned char x, unsigned char y, char *s,
                                uint8_t pen)
{
    uint8_t col = fj_score_spill(x, y, s);
    uint8_t len;

    if (col == FJ_NO_SPILL)
        return 0;

    for (len = 0; s[len]; len++)
        ;
    gfx_text_tight((uint16_t)(col * GFX_CELL_W - FJ_SPILL_BLEED), y, s,
                   pen, PEN_BG, FJ_SPILL_SPAN, fj_spill_advance(len));
    return 1;
}

void drawText(unsigned char x, unsigned char y, char *s)
{
    if (drawSpilledScore(x, y, s, PEN_TEXT))
        return;
    gfx_text(x, y, s, PEN_TEXT, PEN_BG);
}

/*
 * drawTextAlt renders in the alternate pen, except for capital letters:
 * upstream capitalises the key that triggers a menu item ("  Q: quit
 * table", "S: sound ON") and expects the platform to make it stand out.
 * The Atari does that by folding case against its charset; with a real
 * font we keep the glyph and switch pens instead. Drawn in runs, so a
 * plain lowercase string is still a single Text() call.
 */
void drawTextAlt(unsigned char x, unsigned char y, char *s)
{
    char run[FJ_WIDTH + 1];
    uint8_t len = 0;
    uint8_t isKey;

    if (!s || !*s)
        return;
    if (drawSpilledScore(x, y, s, PEN_ALT))
        return;

    isKey = fj_alt_is_key(*s);
    while (*s) {
        if (fj_alt_is_key(*s) != isKey || len == FJ_WIDTH) {
            run[len] = '\0';
            gfx_text(x, y, run, isKey ? PEN_TEXT : PEN_ALT, PEN_BG);
            x = (unsigned char)(x + len);
            len = 0;
            isKey = fj_alt_is_key(*s);
        }
        run[len++] = *s++;
    }
    run[len] = '\0';
    gfx_text(x, y, run, isKey ? PEN_TEXT : PEN_ALT, PEN_BG);
}

void drawChar(unsigned char x, unsigned char y, char c, unsigned char alt)
{
    char buf[2];

    buf[0] = c;
    buf[1] = '\0';
    gfx_text(x, y, buf, alt ? PEN_ALT : PEN_TEXT, PEN_BG);
}

/*
 * Icons are single characters — amiga_vars.h defines every ICON_* as ASCII,
 * where the Atari and DOS ports point them at custom charset slots. The two
 * that carry meaning in play (whose turn it is, and where the score cursor
 * is) are tiles; the lobby's remaining markers are still glyphs, which is
 * what fj_icon_tile() decides.
 */
void drawIcon(unsigned char x, unsigned char y, unsigned char icon)
{
    uint8_t tile = fj_icon_tile(icon);
    char buf[2];

    if (tile != FJ_ICON_NO_TILE) {
        gfx_draw_tile(x, y, tile);
        return;
    }

    buf[0] = (char)icon;
    buf[1] = '\0';
    gfx_text(x, y, buf, PEN_ALT, PEN_BG);
}

/*
 * The fujitzee score row's label, and the sign-off on the help screen.
 * Upstream's other ports draw a six-glyph wordmark ("/|\TZEE" out of their
 * custom charset) starting one cell left of x; ours is five tiles on the
 * label column itself, which is all the score-name box is wide — the
 * mountain carries the /|\ and TZEE follows it.
 */
void drawFujitzee(unsigned char x, unsigned char y)
{
    gfx_draw_tile(x,                 y, TILE_LOGO_M);
    gfx_draw_tile((uint8_t)(x + 1),  y, TILE_LOGO_T);
    gfx_draw_tile((uint8_t)(x + 2),  y, TILE_LOGO_Z);
    gfx_draw_tile((uint8_t)(x + 3),  y, TILE_LOGO_E);
    gfx_draw_tile((uint8_t)(x + 4),  y, TILE_LOGO_E);
}

/* ---- Primitives ------------------------------------------------------- */

void drawBlank(unsigned char x, unsigned char y)
{
    gfx_draw_tile(x, y, TILE_BLANK);
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w)
{
    gfx_fill(x, y, w, 1, PEN_BG);
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    uint8_t i;

    for (i = 0; i < w; i++)
        gfx_draw_tile((uint8_t)(x + i), y, TILE_HRULE);
}

/* Upstream's box is w x h *interior* cells, with the border outside it. */
void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    uint8_t i;

    gfx_draw_tile(x, y, TILE_BOX_TL);
    gfx_draw_tile((uint8_t)(x + w + 1), y, TILE_BOX_TR);
    for (i = 1; i <= w; i++) {
        gfx_draw_tile((uint8_t)(x + i), y, TILE_HRULE);
        gfx_draw_tile((uint8_t)(x + i), (uint8_t)(y + h + 1), TILE_HRULE);
    }
    for (i = 1; i <= h; i++) {
        gfx_draw_tile(x, (uint8_t)(y + i), TILE_VRULE);
        gfx_draw_tile((uint8_t)(x + w + 1), (uint8_t)(y + i), TILE_VRULE);
    }
    gfx_draw_tile(x, (uint8_t)(y + h + 1), TILE_BOX_BL);
    gfx_draw_tile((uint8_t)(x + w + 1), (uint8_t)(y + h + 1), TILE_BOX_BR);
}

/* drawBorder() is part of graphics.h but no fujitzee source calls it —
 * screens.c draws its border by placing four dice itself. */
void drawBorder(void) {}

void drawClock(unsigned char x, unsigned char y)
{
    gfx_draw_tile(x, y, TILE_CLOCK);
}

void drawConnectionIcon(unsigned char x, unsigned char y)
{
    gfx_draw_tile(x, y, TILE_CONN_L);
    gfx_draw_tile((uint8_t)(x + 1), y, TILE_CONN_R);
}

/* ---- The board -------------------------------------------------------- */

/*
 * The score grid, cell for cell as the DOS port draws it: rules first,
 * then the score-name box, then the player-column dividers, then the
 * junction tiles that stitch them together. Order matters — later draws
 * overwrite earlier ones at the crossings, which is how each junction ends
 * up with the tile that has exactly the right arms.
 */
void drawBoard(void)
{
    uint8_t x, y, p;

    /* Thin rules: the two inside the score grid, and one under the left
     * panel's header. */
    for (x = FJ_SCORES_X - 1; x < FJ_WIDTH; x++)
        gfx_draw_tile(x, FJ_THIN_ROW_A, TILE_HTHIN);
    for (x = FJ_SCORES_X; x < FJ_WIDTH; x++)
        gfx_draw_tile(x, FJ_THIN_ROW_B, TILE_HTHIN);
    for (x = 0; x < FJ_SCORES_X - 1; x++)
        gfx_draw_tile(x, 5, TILE_HTHIN);

    /* Thick rules: the player-name header, and the top and bottom of the
     * score grid. */
    for (x = FJ_DIV_X(0); x < FJ_WIDTH; x++)
        gfx_draw_tile(x, 0, TILE_HRULE);
    for (x = FJ_SCORES_X; x < FJ_WIDTH; x++) {
        gfx_draw_tile(x, FJ_BOARD_TOP, TILE_HRULE);
        gfx_draw_tile(x, FJ_BOARD_BOTTOM, TILE_HRULE);
    }

    /* Score-name box: 5 interior cells wide (the labels), 17 tall. */
    drawBox(FJ_SCORES_X - 1, FJ_BOARD_TOP, 5, 17);

    /* Player-column dividers, from the header rule down to the last score
     * row. FJ_DIV_X(FJ_PLAYERS_SHOWN) is the grid's right edge. */
    for (p = 0; p <= FJ_PLAYERS_SHOWN; p++) {
        x = (uint8_t)FJ_DIV_X(p);
        gfx_draw_tile(x, 0, TILE_TEE_D);
        for (y = 1; y < FJ_BOARD_BOTTOM; y++)
            gfx_draw_tile(x, y, TILE_VRULE);
    }

    /* Junctions where the dividers cross the rules. */
    gfx_draw_tile(FJ_SCORES_X - 1, FJ_THIN_ROW_A, TILE_TEE_R_THIN);
    gfx_draw_tile(FJ_SCORES_X - 1, FJ_THIN_ROW_B, TILE_TEE_R_THIN);
    for (p = 0; p <= FJ_PLAYERS_SHOWN; p++) {
        x = (uint8_t)FJ_DIV_X(p);
        gfx_draw_tile(x, FJ_BOARD_TOP,    TILE_CROSS);
        gfx_draw_tile(x, FJ_THIN_ROW_A,   TILE_CROSS_THIN);
        gfx_draw_tile(x, FJ_THIN_ROW_B,   TILE_CROSS_THIN);
        gfx_draw_tile(x, FJ_BOARD_BOTTOM, TILE_TEE_U);
    }

    /* Corners: the header rule starts at the first divider and the grid's
     * right edge is the last one, so no arms may stick out past either. */
    gfx_draw_tile(FJ_DIV_X(0), 0, TILE_BOX_TL);
    x = (uint8_t)FJ_DIV_X(FJ_PLAYERS_SHOWN);
    gfx_draw_tile(x, 0,               TILE_BOX_TR);
    gfx_draw_tile(x, FJ_BOARD_TOP,    TILE_TEE_L);
    gfx_draw_tile(x, FJ_THIN_ROW_A,   TILE_TEE_L_THIN);
    gfx_draw_tile(x, FJ_THIN_ROW_B,   TILE_TEE_L_THIN);
    gfx_draw_tile(x, FJ_BOARD_BOTTOM, TILE_BOX_BR);

    /* Score names down the left column, fujitzee's own row last. */
    for (y = 0; y < 14; y++)
        drawTextAlt(FJ_SCORES_X, scoreY[y], scores[y]);
    drawFujitzee(FJ_SCORES_X, scoreY[14]);
}

/* ---- Active-player column --------------------------------------------- */

/*
 * Bracket the active player's score column by swapping its two dividers
 * for colored ones. The Atari uses a player-missile overlay and the DOS
 * port recolors the column's background in place; both cost more than they
 * buy here — the brackets say the same thing and survive any redraw inside
 * the column, since nothing else writes those two cells.
 *
 * The rule rows are skipped: their junction tiles carry the lattice, and
 * replacing them would break it for a highlight that moves every turn.
 */
static void paintColumn(uint8_t player, uint8_t tile)
{
    uint8_t y, lx, rx;

    lx = (uint8_t)FJ_DIV_X(player);
    rx = (uint8_t)FJ_DIV_X(player + 1);
    for (y = FJ_BOARD_TOP + 1; y < FJ_BOARD_BOTTOM; y++) {
        if (y == FJ_THIN_ROW_A || y == FJ_THIN_ROW_B)
            continue;
        gfx_draw_tile(lx, y, tile);
        gfx_draw_tile(rx, y, tile);
    }
}

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash)
{
    uint8_t tile = flash ? TILE_HI_FLASH
                         : (isThisPlayer ? TILE_HI_SELF : TILE_HI_OTHER);

    if (player >= FJ_PLAYERS_SHOWN)   /* seated, but off the visible grid */
        player = -1;

    if (_hiPlayer != HI_NONE && (player < 0 || (uint8_t)player != _hiPlayer))
        paintColumn(_hiPlayer, TILE_VRULE);

    if (player < 0) {
        _hiPlayer = HI_NONE;
        return;
    }

    if ((uint8_t)player != _hiPlayer || tile != _hiTile) {
        _hiPlayer = (uint8_t)player;
        _hiTile = tile;
        paintColumn(_hiPlayer, tile);
    }
}

/* ---- Dice ------------------------------------------------------------- */

/*
 * upstream's drawDie() is three drawings in one, selected by `s`: 1..6 a
 * die face, 13 nothing at all, 14..16 the Roll button. The Roll button is
 * a blank die body with the number of rolls left in the middle ('x' when
 * there are none) — placeholder for the Phase 3c "ROLL" art.
 */
void drawDie(unsigned char x, unsigned char y, unsigned char s,
             bool isSelected, bool isHighlighted)
{
    uint8_t style, cell;

    if (!s || s > FJ_S_ROLL_MAX)
        return;

    if (s == FJ_S_BLANK) {
        gfx_fill(x, y, 3, 3, PEN_BG);
        return;
    }

    if (s >= FJ_S_ROLL_MIN) {
        char buf[2];

        /* The pressed Roll button uses the highlight face, the way the
         * other ports switch to their alternate-color glyph set. */
        style = isHighlighted ? FJ_STYLE_HI : FJ_STYLE_PLAIN;
        for (cell = 0; cell < 9; cell++)
            gfx_draw_tile((uint8_t)(x + cell % 3), (uint8_t)(y + cell / 3),
                          fj_frame_tile(cell, style));
        buf[0] = fj_roll_label(s);
        buf[1] = '\0';
        if (buf[0])
            gfx_text((uint8_t)(x + 1), (uint8_t)(y + 1), buf,
                     PEN_PIP, _diePen[style]);
        return;
    }

    style = fj_die_style(isSelected, isHighlighted);
    for (cell = 0; cell < 9; cell++)
        gfx_draw_tile((uint8_t)(x + cell % 3), (uint8_t)(y + cell / 3),
                      fj_die_tile(s, cell, style));
}

/*
 * Dice cursor: brackets in the spare cell either side of the die. Only one
 * exists at a time, so drawing it anywhere first erases it wherever it was
 * — upstream does not always pair its hide with the same x.
 */
void drawDiceCursor(unsigned char x)
{
    uint8_t y;

    if (_curShown)
        hideDiceCursor(_curX);

    _curX = x;
    _curShown = 1;
    for (y = FJ_DICE_Y; y < FJ_DICE_Y + 3; y++) {
        gfx_draw_tile((uint8_t)(x - 1), y, TILE_CUR_L);
        gfx_draw_tile((uint8_t)(x + 3), y, TILE_CUR_R);
    }
}

void hideDiceCursor(unsigned char x)
{
    (void)x;   /* erase where we drew, not where the caller thinks it is */

    if (!_curShown)
        return;
    _curShown = 0;
    gfx_fill((uint8_t)(_curX - 1), FJ_DICE_Y, 1, 3, PEN_BG);
    gfx_fill((uint8_t)(_curX + 3), FJ_DICE_Y, 1, 3, PEN_BG);
}

/* ---- Screen buffer ---------------------------------------------------- */

bool saveScreenBuffer(void)
{
    if (!gfx_save_screen())
        return false;

    _savedHiPlayer = _hiPlayer;
    _savedHiTile   = _hiTile;
    _savedCurX     = _curX;
    _savedCurShown = _curShown;
    _haveSaved = 1;
    return true;
}

void restoreScreenBuffer(void)
{
    gfx_restore_screen();

    if (_haveSaved) {
        _hiPlayer = _savedHiPlayer;
        _hiTile   = _savedHiTile;
        _curX     = _savedCurX;
        _curShown = _savedCurShown;
        _haveSaved = 0;
    }
}

/* ---- Color mode ------------------------------------------------------- */

/* Upstream offers a per-player color preference; the Atari and DOS ports
 * both stub it out, and so do we until the Phase 3c art pass decides what
 * the palettes are. Returning 0 keeps prefs.color pinned at the default. */
uint8_t cycleNextColor(void)
{
    return 0;
}

/*
 * Declared `void setColorMode()` upstream — unprototyped — but always
 * called with the stored preference. The parameter must be `int`, not
 * `unsigned char`: against an unprototyped declaration the definition's
 * types are compared after the default argument promotions, and a narrower
 * type is a hard "conflicting types" error under GCC.
 */
void setColorMode(int mode)
{
    colorMode = (unsigned char)mode;
}
