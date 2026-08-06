/* T1 host unit test for the JOYxDAT quadrature decode (see docs/testing.md).
 * Includes the module .c directly per the repo T1 convention, and
 * gkinput.h so the expected bits come from the published GK_JOY_* contract. */
#define FN_TEST_MAIN
#include "fn_test.h"

#include "gkinput.h"
#include "../../src/joydecode.c"

int main(void)
{
    /* Centered stick, no fire. */
    CHECK_EQ(joyDecode(0x0000, 0), 0);

    /* Single directions (up = bit8^bit9, down = bit0^bit1,
     * left = bit9, right = bit1). */
    CHECK(GK_JOY_UP(joyDecode(0x0100, 0)));
    CHECK_EQ(joyDecode(0x0100, 0), 0x01); /* up only */
    CHECK(GK_JOY_DOWN(joyDecode(0x0001, 0)));
    CHECK_EQ(joyDecode(0x0001, 0), 0x02); /* down only */
    CHECK(GK_JOY_LEFT(joyDecode(0x0300, 0)));
    CHECK_EQ(joyDecode(0x0300, 0), 0x04); /* left only (bit8 set cancels up) */
    CHECK(GK_JOY_RIGHT(joyDecode(0x0003, 0)));
    CHECK_EQ(joyDecode(0x0003, 0), 0x08); /* right only (bit0 set cancels down) */

    /* Diagonals. */
    CHECK_EQ(joyDecode(0x0200, 0), 0x05); /* up + left */
    CHECK_EQ(joyDecode(0x0002, 0), 0x0A); /* down + right */
    CHECK_EQ(joyDecode(0x0103, 0), 0x09); /* up + right */
    CHECK_EQ(joyDecode(0x0301, 0), 0x06); /* down + left */

    /* Fire button, alone and combined. */
    CHECK(GK_JOY_BTN_1(joyDecode(0x0000, 1)));
    CHECK_EQ(joyDecode(0x0000, 1), GK_JOY_BTN_1_MASK);
    CHECK_EQ(joyDecode(0x0100, 1), 0x11); /* up + fire */

    /* Unrelated counter bits (mouse-style high counts) must not leak in. */
    CHECK_EQ(joyDecode(0xFCFC, 0), 0);

    /* ---- port-2 wrapper: CIAAPRA fire is active LOW ---- */
    /* Idle port: bit 7 high means the button is NOT pressed. Getting this
     * backwards would report a permanently held trigger. */
    CHECK_EQ(joyDecodePort2(0x0000, 0xFF), 0);
    CHECK_EQ(joyDecodePort2(0x0000, 0x80), 0);
    /* Button held pulls bit 7 low. */
    CHECK_EQ(joyDecodePort2(0x0000, 0x7F), GK_JOY_BTN_1_MASK);
    CHECK_EQ(joyDecodePort2(0x0000, 0x00), GK_JOY_BTN_1_MASK);
    /* Direction and fire together. */
    CHECK_EQ(joyDecodePort2(0x0100, 0x7F), 0x11); /* up + fire */
    CHECK_EQ(joyDecodePort2(0x0003, 0xFF), 0x08); /* right, no fire */
    /* Bit 6 is port 1's (the mouse's) button and must not register. */
    CHECK_EQ(joyDecodePort2(0x0000, 0xBF), 0);

    return fn_test_report("test_joydecode");
}
