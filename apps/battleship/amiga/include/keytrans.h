/*
 * keytrans.h - IDCMP keyboard event -> game key (pure C99, no AmigaOS headers)
 *
 * The graphical renderer's window listens for IDCMP_VANILLAKEY (cooked
 * ASCII) and IDCMP_RAWKEY (needed only for the four cursor keys, which have
 * no vanilla form). input.c feeds each message's class/code here; pure logic
 * so it is T1 host-testable (test/host/test_keytrans.c).
 *
 * See: docs/plan-track1b-battleship.md (Phase 3c)
 */
#ifndef KEYTRANS_H
#define KEYTRANS_H

#include <stdint.h>

/* Amiga raw keycodes for the cursor keys. */
#define KT_RAW_UP    0x4C
#define KT_RAW_DOWN  0x4D
#define KT_RAW_RIGHT 0x4E
#define KT_RAW_LEFT  0x4F

/* Returned when an event carries no game key (key-up, unmapped raw key,
 * out-of-range vanilla code). */
#define KT_NONE (-1)

/*
 * Translate one IDCMP keyboard event. is_rawkey selects the event class
 * (1 = IDCMP_RAWKEY, 0 = IDCMP_VANILLAKEY); code is IntuiMessage->Code.
 * Returns the game key char (matching the KEY_* values in amiga_vars.h)
 * or KT_NONE.
 */
int16_t kt_decode(uint8_t is_rawkey, uint16_t code);

#endif /* KEYTRANS_H */
