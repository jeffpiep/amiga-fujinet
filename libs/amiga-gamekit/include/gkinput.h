/*
 * gkinput.h - the values the gamekit's input decoders emit
 *
 * kt_decode() (keytrans.h) and joyDecode() (joydecode.h) produce game-facing
 * values, but *which* values is a contract, not an implementation detail:
 * each game's platform-specific vars.h has to name the same numbers, or the
 * game silently ignores half its input. Both sides include this header so
 * there is one definition.
 *
 * A port's amiga_vars.h should define its upstream-facing macros from these
 * (e.g. `#define KEY_UP_ARROW GK_KEY_UP`), never restate the literals.
 *
 * Pure constants — no AmigaOS, host-includable.
 */
#ifndef GKINPUT_H
#define GKINPUT_H

/* ---- Joystick bit layout produced by joyDecode() ---- */
#define GK_JOY_UP_MASK    0x01
#define GK_JOY_DOWN_MASK  0x02
#define GK_JOY_LEFT_MASK  0x04
#define GK_JOY_RIGHT_MASK 0x08
#define GK_JOY_BTN_1_MASK 0x10
#define GK_JOY_BTN_2_MASK 0x20

#define GK_JOY_UP(v)    ((v) & GK_JOY_UP_MASK)
#define GK_JOY_DOWN(v)  ((v) & GK_JOY_DOWN_MASK)
#define GK_JOY_LEFT(v)  ((v) & GK_JOY_LEFT_MASK)
#define GK_JOY_RIGHT(v) ((v) & GK_JOY_RIGHT_MASK)
#define GK_JOY_BTN_1(v) ((v) & GK_JOY_BTN_1_MASK)
#define GK_JOY_BTN_2(v) ((v) & GK_JOY_BTN_2_MASK)

/* ---- Key values produced by kt_decode() ----
 * The four cursor keys have no cooked (VANILLAKEY) form, so the decoder has
 * to invent values for them. Two sets exist because ports differ in what
 * they can afford to spend (see KT_CURSOR_* in keytrans.h):
 *
 *   GK_KEY_UP/DOWN/LEFT/RIGHT      - the WASD letters (KT_CURSOR_WASD).
 *     Fine when the game has no letter menus or text entry competing for
 *     them; battleship is that game.
 *   GK_KEY_CUR_UP/DOWN/LEFT/RIGHT  - control codes (KT_CURSOR_CTRL).
 *     For games where the letters are already spoken for — fujitzee has a
 *     lobby menu on 's'/'r'/'c'/'h'/'q' and types player names — so an
 *     arrow key must not read as a letter. 0x1C-0x1F are unreachable any
 *     other way: the vanilla path passes 32..126 plus 0x0D/0x08/0x1B only.
 *
 * The other three keys are cooked ASCII and pass straight through; they are
 * named here because ports need to match them. */
#define GK_KEY_UP        'w'
#define GK_KEY_DOWN      's'
#define GK_KEY_LEFT      'a'
#define GK_KEY_RIGHT     'd'

#define GK_KEY_CUR_UP    0x1C
#define GK_KEY_CUR_DOWN  0x1D
#define GK_KEY_CUR_LEFT  0x1E
#define GK_KEY_CUR_RIGHT 0x1F

#define GK_KEY_RETURN    '\r'   /* 0x0D */
#define GK_KEY_BACKSPACE 8      /* 0x08 */
#define GK_KEY_ESCAPE    27     /* 0x1B */
#define GK_KEY_SPACEBAR  ' '

#endif /* GKINPUT_H */
