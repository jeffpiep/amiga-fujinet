/* gksound.c — audio.device playback for baked effects (KS 1.3 safe).
 *
 * Extracted from apps/battleship/amiga/src/sound.c at Track 1C Phase 3a, on
 * the same seam as every other gamekit module: the pure half (waveform math)
 * was already sndgen.c, and this is the AmigaOS half both ports need. What
 * did NOT come along is the effect table — which tones, how long, and the
 * prefs check — because that is the genuinely game-specific part.
 *
 * Every effect is baked once into a signed 8-bit sample buffer living in
 * Chip RAM: Paula, like the blitter, can only fetch sample data from Chip.
 * One allocation is carved up rather than one AllocMem per effect. Playback
 * is a single fire-and-forget CMD_WRITE on one allocated channel — no
 * blocking, one code path for tones, sweeps and noise.
 *
 * KS 1.3: CreatePort/CreateExtIO from amiga.lib, never CreateMsgPort/
 * CreateIORequest (V36+, Guru on 1.3).
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <devices/audio.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>
#include <stdlib.h>

#include "gksound.h"

/* Paula clock (NTSC 3579545 Hz) / SNDGEN_RATE 8000 Hz ≈ 447. PAL plays ~1%
 * flat — inaudible for effects. */
#define SND_PERIOD 447

static struct MsgPort *sndPort;
static struct IOAudio *sndIO;
static int8_t *chipBlock;
static ULONG chipSize;
static ULONG chipUsed;
static BYTE sndOpen;
static BYTE sndPending;

static UBYTE chanMask[] = {1, 2, 4, 8}; /* any one free channel */

void gk_snd_close(void)
{
    if (sndOpen) {
        gk_snd_stop();
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
    chipSize = chipUsed = 0;
}

int gk_snd_open(uint32_t total_samples)
{
    if (sndOpen)
        return 1;
    if (total_samples == 0)
        return 0;

    chipSize = (ULONG)total_samples;
    chipUsed = 0;

    sndPort = CreatePort(NULL, 0);
    if (!sndPort)
        return 0;
    sndIO = (struct IOAudio *)CreateExtIO(sndPort, sizeof(struct IOAudio));
    chipBlock = AllocMem(chipSize, MEMF_CHIP);
    if (!sndIO || !chipBlock) {
        gk_snd_close();
        return 0;
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
        gk_snd_close();
        return 0;
    }
    sndOpen = 1;

    atexit(gk_snd_close);
    return 1;
}

int gk_snd_is_open(void)
{
    return sndOpen ? 1 : 0;
}

int8_t *gk_snd_carve(GkSndFx *fx, uint32_t len)
{
    fx->buf = NULL;
    fx->len = 0;

    len = (len + 1u) & ~1u; /* Paula DMA length is in words */
    if (!sndOpen || len == 0 || len > chipSize - chipUsed)
        return NULL;

    fx->buf = chipBlock + chipUsed;
    fx->len = len;
    chipUsed += len;
    return fx->buf;
}

/* FS-UAE (3.1.x) drops the AUDxVOL and AUDxPER writes audio.device performs
 * when a CMD_WRITE starts. Volume 0 makes the sample silent; a stale period
 * makes it play at whatever rate the channel was last left at — measured at
 * Phase 3a as ~4x too fast, so every effect came out two octaves high and a
 * quarter as long, identically whether ioa_Period said 447 or 1788.
 * Re-poking both registers with exactly the values the device is supposed to
 * set is a no-op on real hardware and makes the emulator correct. Volume was
 * diagnosed with direct-Paula vs audio.device probes (PR #17, strategic-plan
 * Lessons Learned 2026-07-08); the period by capturing emulator audio and
 * measuring the pitch (emu/checkaudio.py). */
static void pokePerVol(void)
{
    ULONG mask = (ULONG)sndIO->ioa_Request.io_Unit & 0xF;
    UWORD n;

    for (n = 0; n < 4; n++)
        if (mask & (1uL << n)) {
            *(volatile UWORD *)(0xDFF0A6 + n * 16) = SND_PERIOD;
            *(volatile UWORD *)(0xDFF0A8 + n * 16) = 64;
        }
}

void gk_snd_play(const GkSndFx *fx)
{
    if (!sndOpen || !fx || !fx->buf || !fx->len)
        return;
    gk_snd_stop();
    sndIO->ioa_Request.io_Command = CMD_WRITE;
    sndIO->ioa_Request.io_Flags = ADIOF_PERVOL;
    sndIO->ioa_Data = (UBYTE *)fx->buf;
    sndIO->ioa_Length = fx->len;
    sndIO->ioa_Period = SND_PERIOD;
    sndIO->ioa_Volume = 64; /* amplitude is baked into the samples */
    sndIO->ioa_Cycles = 1;
    SendIO((struct IORequest *)sndIO);
    pokePerVol();
    sndPending = 1;
}

void gk_snd_stop(void)
{
    if (!sndPending)
        return;
    AbortIO((struct IORequest *)sndIO); /* harmless if already complete */
    WaitIO((struct IORequest *)sndIO);
    sndPending = 0;
}
