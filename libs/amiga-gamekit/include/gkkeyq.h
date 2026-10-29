/*
 * gkkeyq.h - non-blocking keyboard queue over the gamekit window's IDCMP
 *
 * The upstream games all expect cc65's <conio.h> pair: kbhit() polls and
 * never blocks, cgetc() blocks until a key arrives. Intuition gives us
 * neither directly — it gives a message port that must be drained or it
 * fills up — so this module drains every pending IDCMP message into a small
 * ring buffer and answers both questions from there. Left mouse button
 * state rides along, because it arrives on the same port and would
 * otherwise be lost when a key drain consumed it.
 *
 * The port's input.c wraps these as kbhit()/cgetc(); the decode itself is
 * pure logic in keytrans.c (T1-tested).
 *
 * Requires: gfx_open() to have opened the window (gfxcore.h), exec.library
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * Extracted from apps/battleship/amiga in Track 1C Phase 2 —
 * see docs/plan-track1c-fujitzee.md.
 */
#ifndef GKKEYQ_H
#define GKKEYQ_H

#include <stdint.h>

/*
 * Select what the four cursor keys decode to (KT_CURSOR_* in keytrans.h).
 * Defaults to KT_CURSOR_WASD; call before reading any key if the game needs
 * the control-code spelling instead.
 */
void gk_key_cursor_mode(uint8_t mode);

/* Non-blocking: drain the port, then report whether a key is buffered. */
uint8_t gk_key_hit(void);

/* Pop one key, Wait()ing on the window signal while the buffer is empty. */
char gk_key_get(void);

/* Left mouse button held? Drains the port first, so the answer is current. */
uint8_t gk_mouse_button(void);

#endif /* GKKEYQ_H */
