/* T1 host unit test for the JOYxDAT quadrature decode (see docs/testing.md).
 * Includes the module .c directly per the repo T1 convention, and
 * amiga_vars.h so the expected bits come from the game's own JOY_* macros. */
#define FN_TEST_MAIN
#include "fn_test.h"

#include "amiga_vars.h"
#include "../../src/joydecode.c"

int main(void)
{
    /* Centered stick, no fire. */
    CHECK_EQ(joyDecode(0x0000, 0), 0);

    /* Single directions (up = bit8^bit9, down = bit0^bit1,
     * left = bit9, right = bit1). */
    CHECK(JOY_UP(joyDecode(0x0100, 0)));
    CHECK_EQ(joyDecode(0x0100, 0), 0x01); /* up only */
    CHECK(JOY_DOWN(joyDecode(0x0001, 0)));
    CHECK_EQ(joyDecode(0x0001, 0), 0x02); /* down only */
    CHECK(JOY_LEFT(joyDecode(0x0300, 0)));
    CHECK_EQ(joyDecode(0x0300, 0), 0x04); /* left only (bit8 set cancels up) */
    CHECK(JOY_RIGHT(joyDecode(0x0003, 0)));
    CHECK_EQ(joyDecode(0x0003, 0), 0x08); /* right only (bit0 set cancels down) */

    /* Diagonals. */
    CHECK_EQ(joyDecode(0x0200, 0), 0x05); /* up + left */
    CHECK_EQ(joyDecode(0x0002, 0), 0x0A); /* down + right */
    CHECK_EQ(joyDecode(0x0103, 0), 0x09); /* up + right */
    CHECK_EQ(joyDecode(0x0301, 0), 0x06); /* down + left */

    /* Fire button, alone and combined. */
    CHECK(JOY_BTN_1(joyDecode(0x0000, 1)));
    CHECK_EQ(joyDecode(0x0000, 1), JOY_BTN_1_MASK);
    CHECK_EQ(joyDecode(0x0100, 1), 0x11); /* up + fire */

    /* Unrelated counter bits (mouse-style high counts) must not leak in. */
    CHECK_EQ(joyDecode(0xFCFC, 0), 0);

    return fn_test_report("test_joydecode");
}
