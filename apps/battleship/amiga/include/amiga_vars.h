#ifndef AMIGA_VARS_H
#define AMIGA_VARS_H

/* The key values kt_decode() emits and the bit layout joyDecode() returns
 * are the gamekit's published contract — name them here, never restate the
 * literals (libs/amiga-gamekit/include/gkinput.h). */
#include "gkinput.h"

/* Cell grid of the graphical renderer: 40x25 cells of 8x8 pixels on a
 * 320x200 custom screen (Phase 3c). Mirrors the Atari port's 40-column
 * layout squeezed by one row (Atari is 40x26) — see src/cellmap.c. */
#define WIDTH  40
#define HEIGHT 25

/* Icon characters (ASCII) */
#define ICON_TEXT_CURSOR '>'
#define ICON_MARK        '+'
#define ICON_MARK_ALT    '.'
#define ICON_PLAYER      '@'
#define ICON_CURSOR      '*'
#define ICON_CURSOR_ALT  'o'
#define ICON_CURSOR_BLIP '.'
#define ICON_SPEC        '#'

/* Keyboard mappings — WASD + standard keys for Phase 1 */
#define KEY_LEFT_ARROW   GK_KEY_LEFT
#define KEY_LEFT_ARROW_2 '<'
#define KEY_LEFT_ARROW_3 ','

#define KEY_RIGHT_ARROW  GK_KEY_RIGHT
#define KEY_RIGHT_ARROW_2 '>'
#define KEY_RIGHT_ARROW_3 '.'

#define KEY_UP_ARROW     GK_KEY_UP
#define KEY_UP_ARROW_2   '-'
#define KEY_UP_ARROW_3   1   /* Ctrl-A — distinct non-conflicting alternate */

#define KEY_DOWN_ARROW   GK_KEY_DOWN
#define KEY_DOWN_ARROW_2 '='
#define KEY_DOWN_ARROW_3 2   /* Ctrl-B — distinct non-conflicting alternate */

#define KEY_ESCAPE       GK_KEY_ESCAPE
#define KEY_ESCAPE_ALT   'q'

#define KEY_SPACEBAR     GK_KEY_SPACEBAR
#define KEY_BACKSPACE    GK_KEY_BACKSPACE
#define KEY_RETURN       GK_KEY_RETURN

/* Server query suffix appended to API URLs (empty — server handles defaults) */
#define QUERY_SUFFIX ""

/* Joystick bit-field macros (the layout joyDecode() produces) */
#define JOY_UP(v)    GK_JOY_UP(v)
#define JOY_DOWN(v)  GK_JOY_DOWN(v)
#define JOY_LEFT(v)  GK_JOY_LEFT(v)
#define JOY_RIGHT(v) GK_JOY_RIGHT(v)
#define JOY_BTN_1(v) GK_JOY_BTN_1(v)
#define JOY_BTN_2(v) GK_JOY_BTN_2(v)
#define JOY_BTN_1_MASK GK_JOY_BTN_1_MASK

/* Layout constants (40-column grid — values follow the Atari port) */
#define BOTTOM_HEIGHT 4
#define SCORES_X      11
#define GAMEOVER_PROMPT_Y (HEIGHT - 3)
#define TIMER_X       12
#define TIMER_NUM_OFFSET_X 0
#define TIMER_NUM_OFFSET_Y 0
#define ROLL_SOUND_MOD 4
#define ROLL_FRAMES    31
#define ROLL_X         (WIDTH - 25)
#define SCORE_CURSOR_ALT 0

#endif /* AMIGA_VARS_H */
