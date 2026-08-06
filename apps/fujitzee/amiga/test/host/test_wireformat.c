/* T1: server wire format of the ClientState structs.
 *
 * stateclient.c network_read()s the server's response straight into
 * `clientState` as a raw byte image, so upstream's Game/Player/Table structs
 * ARE the wire format. The server emits the tightly-packed cc65 layout; any
 * padding a compiler inserts shifts every field after it and corrupts the
 * parse (upstream hit exactly this on Open Watcom — see the #pragma pack
 * comment in upstream/src/misc.h).
 *
 * m68k-amigaos-gcc aligns int16_t to 2 bytes, which without that pragma puts
 * one pad byte before Game.players[] — offsetof 96 instead of 95, sizeof 600
 * instead of 599. include/amiga_vars.h therefore enables upstream's Watcom
 * packing path for our build; this test pins the result so a toolchain or
 * upstream change cannot silently reintroduce the pad.
 *
 * Host-testable because it asserts on layout only: same rule applies to the
 * m68k build, and the host compiler inserts the same padding for the same
 * reason. Every expected number below is the cc65 packed layout, computed
 * from the field list by hand — not read back from the compiler.
 */
#define FN_TEST_MAIN
#include "fn_test.h"

#include <stddef.h>

/* amiga_vars.h defines the layout/key macros upstream's headers need and
 * pulls in misc.h with the packing shim applied — exactly as the -include
 * flag does for the Amiga build. */
#include "amiga_vars.h"

int main(void)
{
    /* Table: char[9] + char[21] + char[6], all byte-aligned. */
    CHECK_EQ(sizeof(Table), 36);
    CHECK_EQ(offsetof(Table, name), 9);
    CHECK_EQ(offsetof(Table, players), 30);

    /* Tables: uint8_t count + 10 tables. */
    CHECK_EQ(sizeof(Tables), 1 + 10 * 36);
    CHECK_EQ(offsetof(Tables, table), 1);

    /* Player: char[9] + uint8_t + int16_t[16]. The scores array starts at an
     * even offset anyway, so this one is safe either way — pinned so a field
     * reorder upstream cannot quietly break it. */
    CHECK_EQ(sizeof(Player), 42);
    CHECK_EQ(offsetof(Player, alias), 9);
    CHECK_EQ(offsetof(Player, scores), 10);

    /* Game — the struct that actually needs the packing. */
    CHECK_EQ(offsetof(Game, playerCount), 0);
    CHECK_EQ(offsetof(Game, serverName), 1);
    CHECK_EQ(offsetof(Game, prompt), 22);
    CHECK_EQ(offsetof(Game, round), 63);
    CHECK_EQ(offsetof(Game, rollsLeft), 64);
    CHECK_EQ(offsetof(Game, activePlayer), 65);
    CHECK_EQ(offsetof(Game, moveTime), 66);
    CHECK_EQ(offsetof(Game, viewing), 67);
    CHECK_EQ(offsetof(Game, dice), 68);
    CHECK_EQ(offsetof(Game, keepRoll), 74);
    CHECK_EQ(offsetof(Game, validScores), 80);

    /* The one the pad byte would move: 95, not 96. */
    CHECK_EQ(offsetof(Game, players), 95);
    CHECK_EQ(sizeof(Game), 95 + 12 * 42);   /* 599, not 600 */

    /* apiCall() reads sizeof(clientState.game) bytes into the union, so the
     * union must be at least as large as the largest member. */
    CHECK(sizeof(ClientState) >= sizeof(Game));
    CHECK_EQ(offsetof(ClientState, firstByte), 0);
    CHECK_EQ(offsetof(ClientState, game), 0);

    return fn_test_report("wireformat");
}
