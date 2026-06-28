#ifndef AMIGA_VARS_H
#define AMIGA_VARS_H

/* Screen dimensions for 80x25 Amiga console */
#define WIDTH  80
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
#define KEY_LEFT_ARROW   'a'
#define KEY_LEFT_ARROW_2 '<'
#define KEY_LEFT_ARROW_3 ','

#define KEY_RIGHT_ARROW  'd'
#define KEY_RIGHT_ARROW_2 '>'
#define KEY_RIGHT_ARROW_3 '.'

#define KEY_UP_ARROW     'w'
#define KEY_UP_ARROW_2   '-'
#define KEY_UP_ARROW_3   1   /* Ctrl-A — distinct non-conflicting alternate */

#define KEY_DOWN_ARROW   's'
#define KEY_DOWN_ARROW_2 '='
#define KEY_DOWN_ARROW_3 2   /* Ctrl-B — distinct non-conflicting alternate */

#define KEY_ESCAPE       27
#define KEY_ESCAPE_ALT   'q'

#define KEY_SPACEBAR     ' '
#define KEY_BACKSPACE    8
#define KEY_RETURN       '\r'

/* Server query suffix appended to API URLs (empty — server handles defaults) */
#define QUERY_SUFFIX ""

/* Joystick bit-field macros (match readJoystick() bit layout) */
#define JOY_UP(v)    ((v) & 0x01)
#define JOY_DOWN(v)  ((v) & 0x02)
#define JOY_LEFT(v)  ((v) & 0x04)
#define JOY_RIGHT(v) ((v) & 0x08)
#define JOY_BTN_1(v) ((v) & 0x10)
#define JOY_BTN_2(v) ((v) & 0x20)
#define JOY_BTN_1_MASK 0x10

/* Layout constants (text console) */
#define BOTTOM_HEIGHT 3
#define SCORES_X      60
#define GAMEOVER_PROMPT_Y (HEIGHT - 2)
#define TIMER_X       70
#define TIMER_NUM_OFFSET_X 0
#define TIMER_NUM_OFFSET_Y 0
#define ROLL_SOUND_MOD 4
#define ROLL_FRAMES    10
#define ROLL_X         (WIDTH - 20)
#define SCORE_CURSOR_ALT 0

#endif /* AMIGA_VARS_H */
