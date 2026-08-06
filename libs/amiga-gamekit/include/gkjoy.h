/*
 * gkjoy.h - joystick read for the Amiga game port (port 2)
 *
 * The pure counter→direction decode is joydecode.h; this is the half that
 * has to touch hardware: the JOY1DAT quadrature counter and the CIAAPRA
 * fire line. Read-only peeks at two registers — conventional and safe
 * alongside the OS, which is why no library base or Forbid() is involved
 * and why this needs no open/close pair.
 *
 * Port 2 only, deliberately. Port 1 shares its counter with the mouse, so
 * polling it reports every mouse twitch as a stick direction; port 2 is the
 * customary Amiga game port and the one a player plugs a stick into. Ports
 * whose upstream reads "either joystick" (fujitzee polls both Atari sticks)
 * collapse to this single read — see apps/fujitzee/amiga/src/input.c.
 *
 * Only fire button 1 is reported. The port-2 second button lives in POTGOR
 * and no current port uses it; add it here rather than in a game if one does.
 *
 * Requires: nothing (no library bases). Compiler: m68k-amigaos-gcc.
 *
 * Extracted from apps/battleship/amiga in Track 1C Phase 3b —
 * see docs/plan-track1c-fujitzee.md.
 */
#ifndef GKJOY_H
#define GKJOY_H

#include <stdint.h>

/* Poll game port 2. Returns the GK_JOY_* bit layout from gkinput.h; 0 when
 * nothing is plugged in (an unconnected port reads a centered, unpressed
 * stick), so a game can call this unconditionally. */
uint8_t gk_joy_read_port2(void);

#endif /* GKJOY_H */
