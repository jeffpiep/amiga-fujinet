/* T1 host unit test for the IDCMP key translation (see docs/testing.md).
 * Includes the module .c directly per the repo T1 convention, and
 * amiga_vars.h to prove the arrow mappings match the game's KEY_* values. */
#define FN_TEST_MAIN
#include "fn_test.h"

#include "amiga_vars.h"
#include "../../src/keytrans.c"

int main(void)
{
    /* Raw cursor keys map to the game's arrow keys. */
    CHECK_EQ(kt_decode(1, KT_RAW_UP),    KEY_UP_ARROW);
    CHECK_EQ(kt_decode(1, KT_RAW_DOWN),  KEY_DOWN_ARROW);
    CHECK_EQ(kt_decode(1, KT_RAW_LEFT),  KEY_LEFT_ARROW);
    CHECK_EQ(kt_decode(1, KT_RAW_RIGHT), KEY_RIGHT_ARROW);

    /* Raw key releases (bit 7) are discarded, including arrow releases. */
    CHECK_EQ(kt_decode(1, KT_RAW_UP | 0x80), KT_NONE);
    CHECK_EQ(kt_decode(1, 0xFF), KT_NONE);

    /* Other raw keys are ignored (they arrive as VANILLAKEY instead). */
    CHECK_EQ(kt_decode(1, 0x20), KT_NONE);  /* raw 'a' position */

    /* Vanilla printable ASCII passes through. */
    CHECK_EQ(kt_decode(0, 'w'), 'w');
    CHECK_EQ(kt_decode(0, ' '), KEY_SPACEBAR);
    CHECK_EQ(kt_decode(0, 'Q'), 'Q');
    CHECK_EQ(kt_decode(0, '9'), '9');

    /* Vanilla control keys the game uses. */
    CHECK_EQ(kt_decode(0, 0x0D), KEY_RETURN);
    CHECK_EQ(kt_decode(0, 0x08), KEY_BACKSPACE);
    CHECK_EQ(kt_decode(0, 0x1B), KEY_ESCAPE);

    /* Everything else vanilla is dropped (other controls, 8-bit codes). */
    CHECK_EQ(kt_decode(0, 0x00), KT_NONE);
    CHECK_EQ(kt_decode(0, 0x0A), KT_NONE);
    CHECK_EQ(kt_decode(0, 0x7F), KT_NONE);
    CHECK_EQ(kt_decode(0, 0xE9), KT_NONE);

    return fn_test_report("test_keytrans");
}
