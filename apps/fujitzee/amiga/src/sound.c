/*
 * sound.c — Phase 1 stubs for upstream/src/platform-specific/sound.h
 *
 * Thirteen effects plus the key-sound enable pair. Phase 3a bakes the
 * waveforms with the gamekit's sndgen and plays them through audio.device —
 * including the FS-UAE AUDxVOL workaround (strategic-plan Lessons Learned
 * 2026-07-08). The audio.device machinery itself still lives in
 * apps/battleship/amiga/src/sound.c; see the Phase 3 extraction note in
 * docs/plan-track1c-fujitzee.md.
 *
 * pause() is NOT here — upstream's misc.c defines it as a waitvsync() loop.
 */
#include "misc.h"

void initSound(void) {}

void disableKeySounds(void) {}
void enableKeySounds(void) {}

void soundStop(void) {}
void soundJoinGame(void) {}
void soundMyTurn(void) {}
void soundFujitzee(void) {}
void soundGameDone(void) {}
void soundRollDice(void) {}
void soundRollButton(void) {}
void soundTick(void) {}
void soundCursor(void) {}
void soundScoreCursor(void) {}
void soundKeep(void) {}
void soundRelease(void) {}
void soundScore(void) {}
