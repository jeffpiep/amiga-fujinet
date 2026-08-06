/* T1 host unit test for the IDCMP key translation (see docs/testing.md).
 * Includes the module .c directly per the repo T1 convention, and
 * gkinput.h to prove the arrow mappings match the published GK_KEY_* contract. */
#define FN_TEST_MAIN
#include "fn_test.h"

#include "gkinput.h"
#include "../../src/keytrans.c"

int main(void)
{
    /* Raw cursor keys map to the game's arrow keys. */
    CHECK_EQ(kt_decode(1, KT_RAW_UP),    GK_KEY_UP);
    CHECK_EQ(kt_decode(1, KT_RAW_DOWN),  GK_KEY_DOWN);
    CHECK_EQ(kt_decode(1, KT_RAW_LEFT),  GK_KEY_LEFT);
    CHECK_EQ(kt_decode(1, KT_RAW_RIGHT), GK_KEY_RIGHT);

    /* Raw key releases (bit 7) are discarded, including arrow releases. */
    CHECK_EQ(kt_decode(1, KT_RAW_UP | 0x80), KT_NONE);
    CHECK_EQ(kt_decode(1, 0xFF), KT_NONE);

    /* Other raw keys are ignored (they arrive as VANILLAKEY instead). */
    CHECK_EQ(kt_decode(1, 0x20), KT_NONE);  /* raw 'a' position */

    /* Vanilla printable ASCII passes through. */
    CHECK_EQ(kt_decode(0, 'w'), 'w');
    CHECK_EQ(kt_decode(0, ' '), GK_KEY_SPACEBAR);
    CHECK_EQ(kt_decode(0, 'Q'), 'Q');
    CHECK_EQ(kt_decode(0, '9'), '9');

    /* Vanilla control keys the game uses. */
    CHECK_EQ(kt_decode(0, 0x0D), GK_KEY_RETURN);
    CHECK_EQ(kt_decode(0, 0x08), GK_KEY_BACKSPACE);
    CHECK_EQ(kt_decode(0, 0x1B), GK_KEY_ESCAPE);

    /* Everything else vanilla is dropped (other controls, 8-bit codes). */
    CHECK_EQ(kt_decode(0, 0x00), KT_NONE);
    CHECK_EQ(kt_decode(0, 0x0A), KT_NONE);
    CHECK_EQ(kt_decode(0, 0x7F), KT_NONE);
    CHECK_EQ(kt_decode(0, 0xE9), KT_NONE);

    /* ---- KT_CURSOR_CTRL: arrows as control codes, for ports whose game
     * reads plain letters (fujitzee's lobby menu and name entry). ---- */
    CHECK_EQ(kt_decode_ex(1, KT_RAW_UP,    KT_CURSOR_CTRL), GK_KEY_CUR_UP);
    CHECK_EQ(kt_decode_ex(1, KT_RAW_DOWN,  KT_CURSOR_CTRL), GK_KEY_CUR_DOWN);
    CHECK_EQ(kt_decode_ex(1, KT_RAW_LEFT,  KT_CURSOR_CTRL), GK_KEY_CUR_LEFT);
    CHECK_EQ(kt_decode_ex(1, KT_RAW_RIGHT, KT_CURSOR_CTRL), GK_KEY_CUR_RIGHT);

    /* Releases and unmapped raw codes are still dropped in this mode. */
    CHECK_EQ(kt_decode_ex(1, KT_RAW_UP | 0x80, KT_CURSOR_CTRL), KT_NONE);
    CHECK_EQ(kt_decode_ex(1, 0x20, KT_CURSOR_CTRL), KT_NONE);

    /* The mode changes the raw path only — cooked keys are untouched, so a
     * typed 'w' stays a 'w' and cannot be confused with an arrow. */
    CHECK_EQ(kt_decode_ex(0, 'w', KT_CURSOR_CTRL), 'w');
    CHECK_EQ(kt_decode_ex(0, 's', KT_CURSOR_CTRL), 's');
    CHECK_EQ(kt_decode_ex(0, 0x1B, KT_CURSOR_CTRL), GK_KEY_ESCAPE);

    /* The control codes must stay unreachable from the cooked path, or the
     * two spellings would collide. */
    CHECK_EQ(kt_decode(0, GK_KEY_CUR_UP), KT_NONE);
    CHECK_EQ(kt_decode(0, GK_KEY_CUR_RIGHT), KT_NONE);

    /* kt_decode() is exactly the WASD spelling. */
    CHECK_EQ(kt_decode(1, KT_RAW_UP), kt_decode_ex(1, KT_RAW_UP, KT_CURSOR_WASD));

    return fn_test_report("test_keytrans");
}
