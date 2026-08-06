/* sound.c — fujitzee's effect table (Track 1C Phase 3a).
 *
 * The audio.device machinery is shared: libs/amiga-gamekit/src/gksound.c,
 * extracted from battleship for this phase. What lives here is the effect
 * table — thirteen effects baked once into the gamekit's Chip RAM block by
 * the T1-tested waveform bakers in sndgen.c, then fired and forgotten.
 *
 * Pitches come from upstream's DOS port (src/msdos/sound.c), which documents
 * each one as Hz = 63920 / (2 * (n + 1)) back-derived from the Atari POKEY
 * divisors in src/atari/sound.c. Paula is a better instrument than a PC
 * speaker, so where the DOS port had to drop the Atari's chord notes we keep
 * its single-voice sequence but bake the decay the POKEY version had.
 *
 * Durations follow the DOS port's vsync-frame counts (1 frame ≈ 16.7 ms ≈
 * 133 samples at 8 kHz), with one deliberate exception: soundFujitzee and
 * soundGameDone are baked at half their DOS length. Those two are a 1.9 s
 * and a 2.9 s fanfare, and at full length the pair alone would cost ~38 KB
 * of Chip RAM — more than the 32 KB screen — on a machine that may only have
 * 512 KB of it. The pitch sequence is what makes them recognisable, not the
 * tail. Everything else is at its documented length.
 *
 * pause() is NOT here — upstream's misc.c defines it as a waitvsync() loop.
 */
#include "misc.h" /* prefs.disableSound */
#include "gksound.h"
#include "sndgen.h"

/* Effect sample counts at 8 kHz (8 samples = 1 ms). Paula's DMA length is in
 * words, so keep every length even. */
#define N_CURSOR    160  /* 20 ms blip, 310 Hz */
#define N_SCURSOR   160  /* 20 ms blip, 347 Hz — a tone above the board one */
#define N_TICK      160  /* 20 ms low click, 159 Hz */
#define N_ROLLBTN   528  /* two-note button press */
#define N_ROLLDICE  240  /* 30 ms dice clatter */
#define N_KEEP      664  /* rising sweep, die held */
#define N_RELEASE   936  /* falling sweep, die let go */
#define N_SCORE     536  /* rising sweep, score taken */
#define N_JOIN     1056  /* three-note join fanfare */
#define N_MYTURN   1440  /* single 390 Hz call with decay */
#define N_FUJITZEE 6992  /* six-note fanfare (half the DOS length) */
#define N_DONE    11600  /* four-chord resolve (half the DOS length) */

#define N_TOTAL (N_CURSOR + N_SCURSOR + N_TICK + N_ROLLBTN + N_ROLLDICE + \
                 N_KEEP + N_RELEASE + N_SCORE + N_JOIN + N_MYTURN + \
                 N_FUJITZEE + N_DONE)

static GkSndFx fxCursor, fxScoreCursor, fxTick, fxRollButton, fxRollDice,
    fxKeep, fxRelease, fxScore, fxJoin, fxMyTurn, fxFujitzee, fxDone;

static void bakeEffects(void)
{
    int8_t *p;

    /* Single blips. The cursor pair is deliberately a tone apart so moving
     * across the board and moving down the scorecard do not sound alike. */
    sndgen_tone(gk_snd_carve(&fxCursor, N_CURSOR), N_CURSOR, 310, 80, 0);
    sndgen_tone(gk_snd_carve(&fxScoreCursor, N_SCURSOR), N_SCURSOR,
                347, 80, 0);
    sndgen_tone(gk_snd_carve(&fxTick, N_TICK), N_TICK, 159, 90, 0);

    /* atari/msdos: note 96 then 81 — down, then up. */
    p = gk_snd_carve(&fxRollButton, N_ROLLBTN);
    sndgen_tone(p, 264, 329, 90, 90);
    sndgen_tone(p + 264, 264, 390, 90, 0);

    /* atari: sound(0, 150 + (rand()%20)*5, 8, 8) — a random-pitch clatter.
     * Noise is the closer Paula equivalent of "different every roll" than
     * re-baking a tone at run time would be. */
    sndgen_noise(gk_snd_carve(&fxRollDice, N_ROLLDICE), N_ROLLDICE,
                 100, 0, 0x1D57);

    /* Held / released / scored: the three sweeps, matching the Atari's
     * stepped loops (159→198 up, 141→125 down, 395→627 up). */
    sndgen_sweep(gk_snd_carve(&fxKeep, N_KEEP), N_KEEP, 159, 198, 90, 0);
    sndgen_sweep(gk_snd_carve(&fxRelease, N_RELEASE), N_RELEASE,
                 141, 125, 90, 0);
    sndgen_sweep(gk_snd_carve(&fxScore, N_SCORE), N_SCORE, 395, 627, 95, 0);

    /* atari: note(81); note(96); note(81); */
    p = gk_snd_carve(&fxJoin, N_JOIN);
    sndgen_tone(p, 352, 390, 90, 90);
    sndgen_tone(p + 352, 352, 329, 90, 90);
    sndgen_tone(p + 704, 352, 390, 90, 0);

    sndgen_tone(gk_snd_carve(&fxMyTurn, N_MYTURN), N_MYTURN, 390, 100, 0);

    /* atari: ascending, then resolving. 415 551 695 841 695 841. */
    p = gk_snd_carve(&fxFujitzee, N_FUJITZEE);
    sndgen_tone(p, 864, 415, 90, 90);
    sndgen_tone(p + 864, 864, 551, 90, 90);
    sndgen_tone(p + 1728, 864, 695, 95, 95);
    sndgen_tone(p + 2592, 1536, 841, 100, 90);
    sndgen_tone(p + 4128, 864, 695, 95, 95);
    sndgen_tone(p + 4992, 2000, 841, 100, 0);

    /* atari: four-chord triumphant resolve. 248 329 372 415. */
    p = gk_snd_carve(&fxDone, N_DONE);
    sndgen_tone(p, 2000, 248, 90, 90);
    sndgen_tone(p + 2000, 3464, 329, 95, 95);
    sndgen_tone(p + 5464, 2000, 372, 95, 95);
    sndgen_tone(p + 7464, 4136, 415, 100, 0);
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
void disableKeySounds(void) {}
void enableKeySounds(void)  {}

void soundJoinGame(void)    { play(&fxJoin); }
void soundMyTurn(void)      { play(&fxMyTurn); }
void soundFujitzee(void)    { play(&fxFujitzee); }
void soundGameDone(void)    { play(&fxDone); }
void soundRollDice(void)    { play(&fxRollDice); }
void soundRollButton(void)  { play(&fxRollButton); }
void soundTick(void)        { play(&fxTick); }
void soundCursor(void)      { play(&fxCursor); }
void soundScoreCursor(void) { play(&fxScoreCursor); }
void soundKeep(void)        { play(&fxKeep); }
void soundRelease(void)     { play(&fxRelease); }
void soundScore(void)       { play(&fxScore); }
