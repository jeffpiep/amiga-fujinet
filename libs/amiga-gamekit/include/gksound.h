#ifndef GKSOUND_H
#define GKSOUND_H

#include <stdint.h>

/* audio.device playback for baked sound effects (KS 1.3 safe).
 *
 * The split from sndgen.h is the usual gamekit seam: sndgen.c is the pure,
 * T1-testable waveform math, and this is the AmigaOS half — open the device,
 * own a channel, hold one Chip RAM block, fire samples at Paula. What stays
 * in each game's sound.c is the *effect table*: which waveforms, at what
 * pitch, for how long, and the mute policy.
 *
 * Usage:
 *     gk_snd_open(total);            once, with the summed sample count
 *     gk_snd_carve(&fx, n);          per effect, then bake into fx.buf
 *     gk_snd_play(&fx);              fire and forget
 *
 * All lengths are in samples, played at SNDGEN_RATE. Paula's DMA length is
 * in words, so every length must be even — gk_snd_carve rounds up if it
 * isn't. Sample data must live in Chip RAM; the single block this module
 * allocates is why carving beats a dozen AllocMem calls.
 */

typedef struct {
    int8_t  *buf;
    uint32_t len;
} GkSndFx;

/* Open audio.device, allocate one channel and a Chip RAM block big enough
 * for total_samples. Returns 1 on success, 0 if anything failed (in which
 * case playback is silently inert — never fatal to the game). Registers an
 * atexit() cleanup, so a game that exits via exit() needs no teardown call.
 * Calling it twice is a no-op. */
int gk_snd_open(uint32_t total_samples);

/* 1 once the device is open and a channel is owned. */
int gk_snd_is_open(void);

/* Carve the next len samples off the block and record them in *fx. Returns
 * fx->buf, or NULL if the device is closed or the block would overrun — a
 * caller that mis-sums its total gets a NULL to bake into rather than a
 * heap smash. A NULL-buffered effect is silent, not a crash, at play time. */
int8_t *gk_snd_carve(GkSndFx *fx, uint32_t len);

/* Play fx on the owned channel, cutting off whatever was playing. Returns
 * immediately — nothing about playback blocks. Silent (and harmless) if the
 * device is closed or fx was never carved. Mute policy belongs to the game:
 * check your own prefs before calling. */
void gk_snd_play(const GkSndFx *fx);

/* Stop playback now. Safe when nothing is playing. */
void gk_snd_stop(void);

/* Release channel, device and Chip RAM. Idempotent; also runs at exit. */
void gk_snd_close(void);

#endif /* GKSOUND_H */
