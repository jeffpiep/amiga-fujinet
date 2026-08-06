#ifndef AMIGA_VARS_H
#define AMIGA_VARS_H

/*
 * amiga_vars.h — the Amiga answer to upstream's src/<platform>/vars.h.
 *
 * Force-included ahead of every translation unit via `-include amiga_vars.h`
 * (see the Makefile). Fujitzee's upstream platform-specific/vars.h hard-codes
 * `#include "../atari/vars.h"` and friends, each self-guarded by a toolchain
 * macro (__ATARI__, __WATCOMC__, …); under m68k-amigaos-gcc none of them fire,
 * so it contributes nothing but ESCAPE and we supply the rest from here
 * without patching the pinned upstream submodule.
 */

/*
 * Every upstream platform vars.h (atari, apple2, coco, msdos) shares the one
 * include guard KEYMAP_H — whichever platform's file the toolchain macros
 * select is the file that claims it. We are that file, so claim it here.
 * Defensive rather than load-bearing today: none of those files' toolchain
 * guards fire under m68k-amigaos-gcc, so none would define the constants
 * below anyway. It costs one line to not depend on that.
 */
#define KEYMAP_H

/* The key values kt_decode() emits and the bit layout joyDecode() returns are
 * the gamekit's published contract — name them here, never restate the
 * literals (libs/amiga-gamekit/include/gkinput.h). */
#include "gkinput.h"

/* Screen geometry and the board's column arithmetic live with the layout
 * code that is host-tested against them, not as literals here. */
#include "fjlayout.h"

/* cc65/CMOC headers upstream includes unconditionally. Pulled in here rather
 * than left to misc.h because the packing shim at the bottom of this file
 * suppresses misc.h's own copies of these two includes. */
#include "conio.h"
#include "joystick.h"

/* main.c and misc.c include <fujinet-fuji.h> directly, which declares
 * fuji_create_new(NewDisk *) — and defines NewDisk only inside per-platform
 * #ifdefs, none of which match m68k-amigaos. The compat layer's bridge type
 * has to be in scope first. */
#include "amiga_compat.h"

/* itoa() is not C99; src/util.c provides it for gamelogic.c and screens.c,
 * which reach for it the way cc65's <stdlib.h> offers it. Declare it here so
 * both callers see a prototype rather than an implicit int-returning one. */
char *itoa(int value, char *buf, int radix);

/* ---- Screen geometry ------------------------------------------------- */

/* Cell grid of the graphical renderer: 40x25 cells of 8x8 pixels on a
 * 320x200 custom screen. The Atari port is 40x26; we lose one row to the
 * 200-line PAL/NTSC-common display, which is exactly the DOS port's
 * situation, so the layout follows that one. See fjlayout.h. */
#define WIDTH  FJ_WIDTH
#define HEIGHT FJ_HEIGHT

/* ---- Icons ------------------------------------------------------------ */
/* The Atari values are internal-charset codes; ours are ASCII placeholders
 * until the Phase 3c art pass gives them real tiles. */
#define ICON_TEXT_CURSOR '>'
#define ICON_MARK        '+'
#define ICON_MARK_ALT    '.'
#define ICON_PLAYER      '@'
#define ICON_SPEC        '#'
#define ICON_CURSOR      '*'
#define ICON_CURSOR_ALT  'o'
#define ICON_CURSOR_BLIP '.'

/* ---- Keyboard map ----------------------------------------------------- */

/* The cursor keys arrive as the gamekit's control-code spelling
 * (KT_CURSOR_CTRL, selected in src/input.c), NOT as WASD the way
 * battleship takes them. Fujitzee reads plain letters everywhere — the
 * lobby menu is on s/r/c/h/q and the name-entry screen accepts any letter
 * as text — so an arrow key that decoded to 'w' would type a W and an
 * arrow-down would toggle the sound. See gkinput.h. */
#define KEY_LEFT_ARROW    GK_KEY_CUR_LEFT
#define KEY_LEFT_ARROW_2  '<'
#define KEY_LEFT_ARROW_3  ','

#define KEY_RIGHT_ARROW   GK_KEY_CUR_RIGHT
#define KEY_RIGHT_ARROW_2 '>'
#define KEY_RIGHT_ARROW_3 '.'

/* The _3 alternates and KEY_ESCAPE_ALT are unreachable placeholders, as on
 * Atari: every macro in the group is a case label in misc.c's switch, so
 * they must be distinct from each other AND from the letters screens.c
 * handles itself — 'q' in particular is Quit. Control codes are safe because
 * kt_decode() never emits them. */
#define KEY_UP_ARROW      GK_KEY_CUR_UP
#define KEY_UP_ARROW_2    '-'
#define KEY_UP_ARROW_3    2

#define KEY_DOWN_ARROW    GK_KEY_CUR_DOWN
#define KEY_DOWN_ARROW_2  '='
#define KEY_DOWN_ARROW_3  3

#define KEY_ESCAPE        GK_KEY_ESCAPE
#define KEY_ESCAPE_ALT    1

#define KEY_SPACEBAR      GK_KEY_SPACEBAR
#define KEY_BACKSPACE     GK_KEY_BACKSPACE
#define KEY_RETURN        GK_KEY_RETURN

/* ---- Joystick --------------------------------------------------------- */
/* The bit layout joyDecode() produces; <joystick.h> is only a resolve-stub. */
#define JOY_UP(v)    GK_JOY_UP(v)
#define JOY_DOWN(v)  GK_JOY_DOWN(v)
#define JOY_LEFT(v)  GK_JOY_LEFT(v)
#define JOY_RIGHT(v) GK_JOY_RIGHT(v)
#define JOY_BTN_1(v) GK_JOY_BTN_1(v)
#define JOY_BTN_2(v) GK_JOY_BTN_2(v)
#define JOY_BTN_1_MASK GK_JOY_BTN_1_MASK

/* ---- Layout constants ------------------------------------------------- */
/* Values follow upstream's DOS port, which shares our 40x25 grid — the
 * Atari's are one row taller and one column further right. Phase 2 settled
 * these against the real scorecard: six player columns fit
 * (FJ_PLAYERS_SHOWN), which is what every other 40-column port shows. */
#define BOTTOM_HEIGHT      4   /* height of the bottom panel                */
#define SCORES_X           FJ_SCORES_X  /* X start of the scoreboard        */
#define GAMEOVER_PROMPT_Y  (HEIGHT - 3)
#define TIMER_X            12
#define TIMER_NUM_OFFSET_X 0
#define TIMER_NUM_OFFSET_Y 0
#define ROLL_SOUND_MOD     4   /* how often to play the roll sound          */
#define ROLL_FRAMES        31  /* how many roll frames to play              */
#define ROLL_X             (WIDTH - 25)
#define SCORE_CURSOR_ALT   0   /* alternate score-cursor colour (Atari 0x80)*/

/* Extra query params appended to every API URL — none for Amiga. */
#define QUERY_SUFFIX ""

/* ---- Wire format ------------------------------------------------------- *
 *
 * Nothing to do here, but know that it matters: the server sends the
 * Game/Player/Table structs as a raw byte image in the tightly-packed cc65
 * layout, and stateclient.c network_read()s it straight into `clientState`.
 * m68k-amigaos-gcc aligns int16_t to 2 bytes, which without packing inserts
 * one pad byte before Game.players[] (offsetof 96 instead of 95, sizeof 600
 * instead of 599) and reads every player record one byte out of phase.
 *
 * The Makefile passes -DFUJITZEE_PACK_STRUCTS, which turns on the
 * `#pragma pack(push,1)` upstream's misc.h already applies for Open Watcom
 * (same fault, same cause). test/host/test_wireformat.c pins the resulting
 * offsets so a toolchain or upstream change cannot silently reintroduce the
 * pad — including, notably, the flag going missing from the build.
 */

#endif /* AMIGA_VARS_H */
