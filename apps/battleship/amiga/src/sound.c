/* sound.c — battleship sound effects via audio.device (KS 1.3 safe).
 *
 * Every effect is baked once at initSound() into a signed 8-bit sample
 * buffer (waveform math in sndgen.c, T1-tested) living in Chip RAM —
 * Paula, like the blitter, can only fetch sample data from Chip RAM.
 * Playback is then a single fire-and-forget CMD_WRITE on one allocated
 * audio channel: no blocking, one code path for tones/sweeps/noise.
 *
 * KS 1.3: CreatePort/CreateExtIO from amiga.lib, never
 * CreateMsgPort/CreateIORequest (V36+, Guru on 1.3).
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <devices/audio.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>
#include <stdlib.h>

#include "misc.h" /* prefs.disableSound */
#include "sndgen.h"

/* Paula clock (NTSC 3579545 Hz) / 8000 Hz sample rate ≈ 447. PAL plays
 * ~1% flat — inaudible for effects. */
#define SND_PERIOD 447

struct SndFx {
    int8_t *buf;
    ULONG len;
};

static struct MsgPort *sndPort;
static struct IOAudio *sndIO;
static int8_t *chipBlock;
static ULONG chipSize;
static BYTE sndOpen;
static BYTE sndPending;

static UBYTE chanMask[] = {1, 2, 4, 8}; /* any one free channel */

/* Effect sample counts at 8 kHz (8 samples = 1 ms). Paula DMA length is in
 * words, so keep every length even. */
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

static struct SndFx fxCursor, fxTick, fxSelect, fxPlace, fxJoin, fxMyTurn,
    fxAttack, fxInvalid, fxHit, fxMiss, fxSink, fxDone;

/* Carve the next len samples out of the single chip allocation. */
static int8_t *carve(ULONG *off, struct SndFx *fx, ULONG len)
{
    fx->buf = chipBlock + *off;
    fx->len = len;
    *off += len;
    return fx->buf;
}

static void bakeEffects(void)
{
    ULONG off = 0;
    int8_t *p;

    sndgen_tone(carve(&off, &fxCursor, N_CURSOR), N_CURSOR, 1000, 90, 0);
    sndgen_tone(carve(&off, &fxTick, N_TICK), N_TICK, 3500, 70, 0);

    p = carve(&off, &fxSelect, N_SELECT);
    sndgen_tone(p, 240, 900, 80, 60);
    sndgen_tone(p + 240, 400, 1200, 80, 0);

    p = carve(&off, &fxPlace, N_PLACE);
    sndgen_tone(p, 320, 700, 90, 70);
    sndgen_tone(p + 320, 480, 500, 90, 0);

    p = carve(&off, &fxJoin, N_JOIN);
    sndgen_tone(p, 480, 660, 90, 80);
    sndgen_tone(p + 480, 480, 880, 90, 80);
    sndgen_tone(p + 960, 640, 1100, 90, 0);

    p = carve(&off, &fxMyTurn, N_MYTURN);
    sndgen_tone(p, 560, 880, 100, 0);
    sndgen_silence(p + 560, 240);
    sndgen_tone(p + 800, 560, 880, 100, 0);

    sndgen_sweep(carve(&off, &fxAttack, N_ATTACK), N_ATTACK,
                 400, 1400, 100, 60);
    sndgen_sweep(carve(&off, &fxInvalid, N_INVALID), N_INVALID,
                 400, 150, 100, 30);
    sndgen_noise(carve(&off, &fxHit, N_HIT), N_HIT, 110, 0, 0xACE1);
    sndgen_sweep(carve(&off, &fxMiss, N_MISS), N_MISS, 300, 80, 80, 0);
    sndgen_sweep(carve(&off, &fxSink, N_SINK), N_SINK, 900, 80, 110, 0);

    p = carve(&off, &fxDone, N_DONE);
    sndgen_tone(p, 800, 523, 90, 80);
    sndgen_tone(p + 800, 800, 659, 90, 80);
    sndgen_tone(p + 1600, 1440, 784, 100, 0);
}

static void soundCleanup(void)
{
    if (sndOpen) {
        soundStop();
        CloseDevice((struct IORequest *)sndIO); /* frees the channel */
        sndOpen = 0;
    }
    if (sndIO) {
        DeleteExtIO((struct IORequest *)sndIO);
        sndIO = NULL;
    }
    if (sndPort) {
        DeletePort(sndPort);
        sndPort = NULL;
    }
    if (chipBlock) {
        FreeMem(chipBlock, chipSize);
        chipBlock = NULL;
    }
}

void initSound(void)
{
    if (sndOpen)
        return;

    chipSize = N_CURSOR + N_TICK + N_SELECT + N_PLACE + N_JOIN + N_MYTURN +
               N_ATTACK + N_INVALID + N_HIT + N_MISS + N_SINK + N_DONE;

    sndPort = CreatePort(NULL, 0);
    if (!sndPort)
        return;
    sndIO = (struct IOAudio *)CreateExtIO(sndPort, sizeof(struct IOAudio));
    chipBlock = AllocMem(chipSize, MEMF_CHIP);
    if (!sndIO || !chipBlock) {
        soundCleanup();
        return;
    }

    /* OpenDevice performs the ADCMD_ALLOCATE set up here; ln_Pri is the
     * allocation precedence (sound effects: modest, steal from nobody). */
    sndIO->ioa_Request.io_Message.mn_Node.ln_Pri = 0;
    sndIO->ioa_Request.io_Command = ADCMD_ALLOCATE;
    sndIO->ioa_Request.io_Flags = ADIOF_NOWAIT;
    sndIO->ioa_AllocKey = 0;
    sndIO->ioa_Data = chanMask;
    sndIO->ioa_Length = sizeof(chanMask);
    if (OpenDevice((STRPTR)"audio.device", 0,
                   (struct IORequest *)sndIO, 0) != 0) {
        soundCleanup();
        return;
    }
    sndOpen = 1;

    bakeEffects();
    atexit(soundCleanup); /* quit() exits via exit(0) */
}

/* FS-UAE (3.1.x) drops the AUDxVOL write audio.device performs when a
 * CMD_WRITE starts, so the sample plays at volume 0 — silent. Re-poking
 * the owned channel's volume register with the same value the device
 * sets is a no-op on real hardware and makes the emulator audible.
 * Diagnosed with direct-Paula vs audio.device probes; see PR #17. */
static void pokeVolume(void)
{
    ULONG mask = (ULONG)sndIO->ioa_Request.io_Unit & 0xF;
    UWORD n;

    for (n = 0; n < 4; n++)
        if (mask & (1uL << n))
            *(volatile UWORD *)(0xDFF0A8 + n * 16) = 64;
}

static void play(const struct SndFx *fx)
{
    if (!sndOpen || prefs.disableSound)
        return;
    soundStop();
    sndIO->ioa_Request.io_Command = CMD_WRITE;
    sndIO->ioa_Request.io_Flags = ADIOF_PERVOL;
    sndIO->ioa_Data = (UBYTE *)fx->buf;
    sndIO->ioa_Length = fx->len;
    sndIO->ioa_Period = SND_PERIOD;
    sndIO->ioa_Volume = 64; /* amplitude is baked into the samples */
    sndIO->ioa_Cycles = 1;
    SendIO((struct IORequest *)sndIO);
    pokeVolume();
    sndPending = 1;
}

void soundStop(void)
{
    if (!sndPending)
        return;
    AbortIO((struct IORequest *)sndIO); /* harmless if already complete */
    WaitIO((struct IORequest *)sndIO);
    sndPending = 0;
}

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
