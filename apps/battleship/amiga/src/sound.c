/* sound.c — battleship's effect table.
 *
 * The audio.device machinery this used to hold moved to the gamekit at
 * Track 1C Phase 3a (libs/amiga-gamekit/src/gksound.c) so the fujitzee port
 * could share it. What stays here is what is actually battleship's: which
 * waveforms each effect is made of, and the mute policy.
 *
 * Every effect is baked once at initSound() into the gamekit's single Chip
 * RAM block (waveform math in sndgen.c, T1-tested); playback is then
 * fire-and-forget.
 */
#include "misc.h" /* prefs.disableSound */
#include "gksound.h"
#include "sndgen.h"

/* Effect sample counts at 8 kHz (8 samples = 1 ms). */
#define N_CURSOR   240  /* 30 ms blip */
#define N_TICK     120  /* 15 ms click */
#define N_SELECT   640  /* two-note confirm */
#define N_PLACE    800  /* two-note thunk */
#define N_JOIN    1600  /* rising three-note fanfare */
#define N_MYTURN  1360  /* two beeps with a gap */
#define N_ATTACK   960  /* rising sweep */
#define N_INVALID 1200  /* falling buzz */
#define N_HIT     1600  /* noise burst */
#define N_MISS    1200  /* low splash sweep */
#define N_SINK    4000  /* long descending sweep */
#define N_DONE    3040  /* end-of-game fanfare */

#define N_TOTAL (N_CURSOR + N_TICK + N_SELECT + N_PLACE + N_JOIN + N_MYTURN + \
                 N_ATTACK + N_INVALID + N_HIT + N_MISS + N_SINK + N_DONE)

static GkSndFx fxCursor, fxTick, fxSelect, fxPlace, fxJoin, fxMyTurn,
    fxAttack, fxInvalid, fxHit, fxMiss, fxSink, fxDone;

static void bakeEffects(void)
{
    int8_t *p;

    sndgen_tone(gk_snd_carve(&fxCursor, N_CURSOR), N_CURSOR, 1000, 90, 0);
    sndgen_tone(gk_snd_carve(&fxTick, N_TICK), N_TICK, 3500, 70, 0);

    p = gk_snd_carve(&fxSelect, N_SELECT);
    sndgen_tone(p, 240, 900, 80, 60);
    sndgen_tone(p + 240, 400, 1200, 80, 0);

    p = gk_snd_carve(&fxPlace, N_PLACE);
    sndgen_tone(p, 320, 700, 90, 70);
    sndgen_tone(p + 320, 480, 500, 90, 0);

    p = gk_snd_carve(&fxJoin, N_JOIN);
    sndgen_tone(p, 480, 660, 90, 80);
    sndgen_tone(p + 480, 480, 880, 90, 80);
    sndgen_tone(p + 960, 640, 1100, 90, 0);

    p = gk_snd_carve(&fxMyTurn, N_MYTURN);
    sndgen_tone(p, 560, 880, 100, 0);
    sndgen_silence(p + 560, 240);
    sndgen_tone(p + 800, 560, 880, 100, 0);

    /* Both end at zero amplitude, not 60/30 — a sample that stops on a
     * non-zero value leaves Paula holding it as a DC offset on the channel
     * until the next effect (found by the Phase 3a audio capture). */
    sndgen_sweep(gk_snd_carve(&fxAttack, N_ATTACK), N_ATTACK,
                 400, 1400, 100, 0);
    sndgen_sweep(gk_snd_carve(&fxInvalid, N_INVALID), N_INVALID,
                 400, 150, 100, 0);
    sndgen_noise(gk_snd_carve(&fxHit, N_HIT), N_HIT, 110, 0, 0xACE1);
    sndgen_sweep(gk_snd_carve(&fxMiss, N_MISS), N_MISS, 300, 80, 80, 0);
    sndgen_sweep(gk_snd_carve(&fxSink, N_SINK), N_SINK, 900, 80, 110, 0);

    p = gk_snd_carve(&fxDone, N_DONE);
    sndgen_tone(p, 800, 523, 90, 80);
    sndgen_tone(p + 800, 800, 659, 90, 80);
    sndgen_tone(p + 1600, 1440, 784, 100, 0);
}

void initSound(void)
{
    if (gk_snd_open(N_TOTAL))
        bakeEffects();
}

static void play(const GkSndFx *fx)
{
    if (prefs.disableSound)
        return;
    gk_snd_play(fx);
}

void soundStop(void) { gk_snd_stop(); }

/* Key clicks are an Atari OS feature; nothing to toggle on Amiga. */
void enableKeySounds(void)  {}
void disableKeySounds(void) {}

void soundCursor(void)    { play(&fxCursor); }
void soundTick(void)      { play(&fxTick); }
void soundSelect(void)    { play(&fxSelect); }
void soundPlaceShip(void) { play(&fxPlace); }
void soundJoinGame(void)  { play(&fxJoin); }
void soundMyTurn(void)    { play(&fxMyTurn); }
void soundAttack(void)    { play(&fxAttack); }
void soundInvalid(void)   { play(&fxInvalid); }
void soundHit(void)       { play(&fxHit); }
void soundMiss(void)      { play(&fxMiss); }
void soundSink(void)      { play(&fxSink); }
void soundGameDone(void)  { play(&fxDone); }
