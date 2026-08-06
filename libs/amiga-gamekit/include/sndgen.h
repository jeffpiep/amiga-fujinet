#ifndef SNDGEN_H
#define SNDGEN_H

#include <stdint.h>

/* Waveform bakers for game sound effects. Pure integer C — no AmigaOS, no
 * floats — so they are T1 host-testable; the audio.device playback lives in
 * each game's sound.c. All generators write signed 8-bit samples
 * meant for playback at SNDGEN_RATE (Paula period SND_PERIOD in sound.c).
 * Volumes are sample amplitudes 0..127; each generator ramps linearly
 * from vol_start to vol_end across the buffer (decay = ramp to 0).
 *
 * End every effect at vol_end 0. Paula holds the last sample value on the
 * channel after playback completes, so a buffer that stops on a non-zero
 * amplitude leaves a DC offset sitting there until the next effect plays —
 * silent in itself, but it thumps on the next transition. Only the last
 * segment of a multi-segment effect has to obey; the joins inside one are
 * exactly where you want a non-zero level.
 *
 * A NULL buf is a no-op in every generator, so the output of a failed
 * gk_snd_carve() can be baked into without a check at each call site — on
 * a 68000 that would otherwise write over the exec vectors at address 0. */
#define SNDGEN_RATE 8000u

/* Square tone at freq Hz. */
void sndgen_tone(int8_t *buf, uint16_t n, uint16_t freq,
                 uint8_t vol_start, uint8_t vol_end);

/* Square tone sweeping linearly f0 → f1 Hz. */
void sndgen_sweep(int8_t *buf, uint16_t n, uint16_t f0, uint16_t f1,
                  uint8_t vol_start, uint8_t vol_end);

/* White-ish noise from a 16-bit Galois LFSR (seed must be non-zero). */
void sndgen_noise(int8_t *buf, uint16_t n, uint8_t vol_start,
                  uint8_t vol_end, uint16_t seed);

/* Silence (gap between notes baked into one buffer). */
void sndgen_silence(int8_t *buf, uint16_t n);

#endif /* SNDGEN_H */
