/*
 * keytrans.c - IDCMP keyboard event -> game key (pure C99, no AmigaOS headers)
 *
 * VANILLAKEY already delivers cooked ASCII (WASD, space, return 0x0D,
 * backspace 0x08, ESC 0x1B), so it passes through with a range filter.
 * RAWKEY matters only for the cursor keys, which Intuition never cooks;
 * they map onto the GK_KEY_* values in gkinput.h, which each port's
 * amiga_vars.h names for its upstream. Raw key-releases (code bit 7 set)
 * and every other raw code are discarded.
 */
#include "keytrans.h"
#include "gkinput.h"

int16_t kt_decode(uint8_t is_rawkey, uint16_t code)
{
    if (is_rawkey) {
        if (code & 0x80)            /* key release */
            return KT_NONE;
        switch (code) {
        case KT_RAW_UP:    return GK_KEY_UP;
        case KT_RAW_DOWN:  return GK_KEY_DOWN;
        case KT_RAW_LEFT:  return GK_KEY_LEFT;
        case KT_RAW_RIGHT: return GK_KEY_RIGHT;
        default:           return KT_NONE;
        }
    }

    /* Vanilla: printable ASCII plus the control keys the game uses. */
    if (code >= 32 && code < 127)
        return (int16_t)code;
    if (code == 0x0D || code == 0x08 || code == 0x1B)
        return (int16_t)code;
    return KT_NONE;
}
