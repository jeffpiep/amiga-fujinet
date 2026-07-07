/* T1 host unit test for the waveform bakers (see docs/testing.md). */
#define FN_TEST_MAIN
#include "fn_test.h"

#include "../../src/sndgen.c"

/* Count sign changes — a square tone at f Hz over n samples at SNDGEN_RATE
 * has ~2*f*n/rate half-period transitions. */
static int crossings(const int8_t *buf, int n)
{
    int i, c = 0;
    for (i = 1; i < n; i++)
        if ((buf[i] < 0) != (buf[i - 1] < 0))
            c++;
    return c;
}

static int peak(const int8_t *buf, int n)
{
    int i, p = 0;
    for (i = 0; i < n; i++) {
        int a = buf[i] < 0 ? -buf[i] : buf[i];
        if (a > p)
            p = a;
    }
    return p;
}

int main(void)
{
    static int8_t buf[8000];
    int c;

    /* Tone: 1 kHz for 1 s at 8 kHz → ~2000 crossings; constant volume. */
    sndgen_tone(buf, 8000, 1000, 100, 100);
    c = crossings(buf, 8000);
    CHECK(c >= 1990 && c <= 2010);
    CHECK_EQ(peak(buf, 8000), 100);
    CHECK_EQ(peak(buf + 7900, 100), 100); /* no decay when vol constant */

    /* Tone decay: volume ramps 100 → 0, so the tail is quieter than the
     * head and nothing exceeds the start volume. */
    sndgen_tone(buf, 8000, 1000, 100, 0);
    CHECK(peak(buf, 8000) <= 100);
    CHECK(peak(buf, 800) > 80);
    CHECK(peak(buf + 7200, 800) < 20);

    /* Sweep: rising 500 → 2000 Hz averages ~1250 Hz → ~2500 crossings,
     * and the last quarter oscillates faster than the first. */
    sndgen_sweep(buf, 8000, 500, 2000, 100, 100);
    c = crossings(buf, 8000);
    CHECK(c >= 2400 && c <= 2600);
    CHECK(crossings(buf + 6000, 2000) > crossings(buf, 2000));

    /* Noise: bounded, non-silent, and much denser in sign changes than a
     * 1 kHz tone. */
    sndgen_noise(buf, 8000, 100, 100, 0xACE1);
    CHECK(peak(buf, 8000) == 100);
    CHECK(crossings(buf, 8000) > 3000);
    /* Zero seed must not lock the LFSR at silence. */
    sndgen_noise(buf, 8000, 100, 100, 0);
    CHECK(crossings(buf, 8000) > 3000);

    /* Silence. */
    sndgen_silence(buf, 8000);
    CHECK_EQ(peak(buf, 8000), 0);

    return fn_test_report("test_sndgen");
}
