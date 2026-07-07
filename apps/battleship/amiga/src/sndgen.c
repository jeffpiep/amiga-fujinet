/* sndgen.c — pure integer waveform bakers (T1 host-testable, see sndgen.h).
 *
 * Tones use a 16-bit phase accumulator: inc = freq * 2^16 / rate, and the
 * top phase bit selects the square-wave half. 68000-friendly: 32-bit
 * multiplies only at ramp interpolation, no division in the sample loop.
 */
#include "sndgen.h"

static int16_t ramp(uint8_t v0, uint8_t v1, uint16_t i, uint16_t n)
{
    return (int16_t)v0 + (int16_t)(((int32_t)((int16_t)v1 - (int16_t)v0) * i) / n);
}

void sndgen_tone(int8_t *buf, uint16_t n, uint16_t freq,
                 uint8_t vol_start, uint8_t vol_end)
{
    uint16_t inc = (uint16_t)(((uint32_t)freq << 16) / SNDGEN_RATE);
    uint16_t phase = 0;
    uint16_t i;

    for (i = 0; i < n; i++) {
        int16_t vol = ramp(vol_start, vol_end, i, n);
        buf[i] = (int8_t)((phase & 0x8000u) ? vol : -vol);
        phase += inc;
    }
}

void sndgen_sweep(int8_t *buf, uint16_t n, uint16_t f0, uint16_t f1,
                  uint8_t vol_start, uint8_t vol_end)
{
    uint16_t inc0 = (uint16_t)(((uint32_t)f0 << 16) / SNDGEN_RATE);
    uint16_t inc1 = (uint16_t)(((uint32_t)f1 << 16) / SNDGEN_RATE);
    uint16_t phase = 0;
    uint16_t i;

    for (i = 0; i < n; i++) {
        uint16_t inc = (uint16_t)((int32_t)inc0 +
                                  ((int32_t)inc1 - (int32_t)inc0) * i / n);
        int16_t vol = ramp(vol_start, vol_end, i, n);

        buf[i] = (int8_t)((phase & 0x8000u) ? vol : -vol);
        phase += inc;
    }
}

void sndgen_noise(int8_t *buf, uint16_t n, uint8_t vol_start,
                  uint8_t vol_end, uint16_t seed)
{
    uint16_t lfsr = seed ? seed : 0xACE1u;
    uint16_t i;

    for (i = 0; i < n; i++) {
        int16_t vol = ramp(vol_start, vol_end, i, n);
        buf[i] = (int8_t)((lfsr & 1u) ? vol : -vol);
        lfsr = (uint16_t)((lfsr >> 1) ^ ((0u - (lfsr & 1u)) & 0xB400u));
    }
}

void sndgen_silence(int8_t *buf, uint16_t n)
{
    uint16_t i;

    for (i = 0; i < n; i++)
        buf[i] = 0;
}
