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
 * The four cursor keys have no cooked (VANILLAKEY) form, so kt_decode maps
 * their raw codes onto the WASD letters the games already accept. The other
 * three are cooked ASCII and pass straight through; they are named here
 * because ports need to match them. */
#define GK_KEY_UP        'w'
#define GK_KEY_DOWN      's'
#define GK_KEY_LEFT      'a'
#define GK_KEY_RIGHT     'd'

#define GK_KEY_RETURN    '\r'   /* 0x0D */
#define GK_KEY_BACKSPACE 8      /* 0x08 */
#define GK_KEY_ESCAPE    27     /* 0x1B */
#define GK_KEY_SPACEBAR  ' '

#endif /* GKINPUT_H */
