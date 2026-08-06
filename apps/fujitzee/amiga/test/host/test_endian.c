/* T1: the wire-format byte swap (upstream fixStateEndianness).
 *
 * Player.scores is the only multi-byte field the server sends, and it arrives
 * little-endian. m68k reads it big-endian, so before this fix every opponent
 * score displayed as value<<8 — a real 11 rendered as 2816 (0x0B00). Confirmed
 * against the live server on 2026-08-06: the trace showed `0b 00` on the wire
 * for a bot whose column read 2816.
 *
 * It hid for a whole phase because the two things anyone had looked at are both
 * byte-order invariant: 0xFFFF ("not yet scored"), and the candidate scores in
 * your own column, which are computed locally rather than parsed. That is also
 * why test_wireformat.c passing was not enough — packing and byte order are
 * independent, and it only pins the first.
 *
 * This asserts the swap as a *contract* (every score reversed, nothing else
 * touched), not "scores come out correct", so it means the same thing on a
 * little-endian host as on m68k. The Makefile forces -DFUJITZEE_BIG_ENDIAN for
 * this file so the real function is compiled even on x86.
 *
 * Rather than duplicate the loop, it includes upstream's stateclient.c and
 * stubs what that file needs to link. apiCall() is never called here.
 */
#define FN_TEST_MAIN
#include "fn_test.h"

#include <string.h>

#include "amiga_vars.h"
#include "misc.h"

/* Globals stateclient.c expects from upstream's main.c. */
ClientState clientState;
GameState state;
PrefsStruct prefs;
char tempBuffer[128];
char serverEndpoint[50];
unsigned char h, i, j, k, x, y;

/* Stubs for the network/platform calls in apiCall(), which this test never
 * reaches — they exist only to satisfy the linker. */
uint8_t network_open(const char *d, uint8_t m, uint8_t t) { (void)d;(void)m;(void)t; return 1; }
int16_t network_read(const char *d, uint8_t *b, uint16_t l) { (void)d;(void)b;(void)l; return 0; }
uint8_t network_close(const char *d) { (void)d; return 0; }
void waitvsync(void) { }

#include "stateclient.c"

int main(void)
{
    uint8_t p, s;

    memset(&clientState, 0, sizeof(clientState));

    /* A wire image: player 0 scored 11 in slot 0 and 3 in slot 15; player 11
     * (the last the struct holds) scored 1. Everything else stays 0. */
    clientState.game.playerCount = 5;
    strcpy(clientState.game.players[0].name, "ai meg");
    clientState.game.players[0].scores[0]  = 0x000B;
    clientState.game.players[0].scores[15] = 0x0003;
    clientState.game.players[PLAYER_MAX - 1].scores[0] = 0x0001;

    fixStateEndianness();

    /* Every score reversed — including the last slot of the last player, which
     * a loop bound that stopped at playerCount or 15 would miss. */
    CHECK_EQ((uint16_t)clientState.game.players[0].scores[0],  0x0B00);
    CHECK_EQ((uint16_t)clientState.game.players[0].scores[15], 0x0300);
    CHECK_EQ((uint16_t)clientState.game.players[PLAYER_MAX - 1].scores[0], 0x0100);

    /* Zero and 0xFFFF ("not yet scored") are their own mirror — the reason the
     * bug stayed invisible on an empty scorecard. */
    CHECK_EQ((uint16_t)clientState.game.players[1].scores[0], 0x0000);
    clientState.game.players[2].scores[0] = (int16_t)0xFFFF;
    fixStateEndianness();
    CHECK_EQ((uint16_t)clientState.game.players[2].scores[0], 0xFFFF);

    /* Single-byte fields must be left alone: the same union also carries the
     * table listing, which is all char/uint8_t. */
    CHECK_EQ(clientState.game.playerCount, 5);
    CHECK(strcmp(clientState.game.players[0].name, "ai meg") == 0);

    /* Swapping twice is the identity — proves it touches each score once. */
    memset(&clientState, 0, sizeof(clientState));
    for (p = 0; p < PLAYER_MAX; p++)
        for (s = 0; s < 16; s++)
            clientState.game.players[p].scores[s] = (int16_t)(p * 16 + s);
    fixStateEndianness();
    fixStateEndianness();
    for (p = 0; p < PLAYER_MAX; p++)
        for (s = 0; s < 16; s++)
            CHECK_EQ(clientState.game.players[p].scores[s], (int16_t)(p * 16 + s));

    return fn_test_report("endian");
}
