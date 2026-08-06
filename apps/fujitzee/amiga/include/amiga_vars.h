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
 * Without this, the packing shim at the bottom (which briefly defines
 * __WATCOMC__) would let msdos/vars.h through and its WIDTH/HEIGHT/KEY_*
 * would silently override every constant below.
 */
#define KEYMAP_H

/* The key values kt_decode() emits and the bit layout joyDecode() returns are
 * the gamekit's published contract — name them here, never restate the
 * literals (libs/amiga-gamekit/include/gkinput.h). */
#include "gkinput.h"

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
 * 200-line PAL/NTSC-common display. Phase 2 decides whether the scorecard
 * needs a wider grid than this — nothing forces 40 columns on a 320-pixel
 * screen but the 8x8 font. */
#define WIDTH  40
#define HEIGHT 25

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
#define KEY_LEFT_ARROW    GK_KEY_LEFT
#define KEY_LEFT_ARROW_2  '<'
#define KEY_LEFT_ARROW_3  ','

#define KEY_RIGHT_ARROW   GK_KEY_RIGHT
#define KEY_RIGHT_ARROW_2 '>'
#define KEY_RIGHT_ARROW_3 '.'

/* The _3 alternates and KEY_ESCAPE_ALT are unreachable placeholders, as on
 * Atari: every macro in the group is a case label in misc.c's switch, so
 * they must be distinct from each other AND from the letters screens.c
 * handles itself — 'q' in particular is Quit. Control codes are safe because
 * kt_decode() never emits them. */
#define KEY_UP_ARROW      GK_KEY_UP
#define KEY_UP_ARROW_2    '-'
#define KEY_UP_ARROW_3    2

#define KEY_DOWN_ARROW    GK_KEY_DOWN
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
/* Values follow the Atari 40-column port; Phase 2 revisits them against the
 * real scorecard. */
#define BOTTOM_HEIGHT      4   /* height of the bottom panel                */
#define SCORES_X           11  /* X start of the scoreboard                 */
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

/* ---- Wire-format packing shim ----------------------------------------- *
 *
 * The server sends the Game/Player/Table structs as a raw byte image in the
 * tightly-packed cc65 layout, and stateclient.c network_read()s it straight
 * into `clientState`. m68k-amigaos-gcc aligns int16_t to 2 bytes, which
 * inserts one pad byte before Game.players[] (offsetof 96 instead of 95,
 * sizeof 600 instead of 599) — every player record would be read one byte
 * out of phase. Upstream already solved this for Open Watcom, whose 16-bit
 * default alignment causes the same fault: misc.h wraps those structs in
 * `#pragma pack(push,1)` under `#ifdef __WATCOMC__`, and GCC honours that
 * pragma too.
 *
 * So we take upstream's fix by pulling misc.h in here with __WATCOMC__
 * momentarily defined, then undefining it before any .c file is parsed. The
 * include guard makes each TU's own `#include "misc.h"` a no-op, so the
 * structs are packed everywhere while misc.c and gamelogic.c still compile
 * their non-Watcom paths (real joystick macros, <peekpoke.h>). The only
 * __WATCOMC__ uses in upstream *headers* are the two shim includes handled
 * above and this pragma pair — everything else is inside .c files.
 *
 * The alternative was a two-line upstream patch widening that guard to a
 * FUJITZEE_PACK_STRUCTS opt-in, which is the better long-term shape and is
 * worth proposing alongside the PLATFORM_VARS hook (docs/upstreaming-
 * amiga-ports.md). This keeps the pin read-only until then.
 *
 * test/host/test_wireformat.c pins the resulting offsets so a toolchain or
 * upstream change cannot silently reintroduce the pad.
 */
#define __WATCOMC__ 1
#include "misc.h"
#undef __WATCOMC__

#endif /* AMIGA_VARS_H */
