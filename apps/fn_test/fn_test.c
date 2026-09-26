/*
 * fn_test.c - Minimal FujiNet init and readiness test
 *
 * Calls fn_init() and fn_is_ready(), then reads the clock. Neither of the
 * first two sends anything (the broker opens serial.device lazily), so the
 * clock read is what actually proves the link: a FujiBus round trip that
 * needs no internet.
 *
 * Requires: exec.library, fujinet-nio.device resident, serial.device
 * (see contracts/amiga-adf-bootstrap.md)
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * See: contracts/amiga-transport-api.md
 */

#include <stdio.h>
#include "fujinet-nio.h"

int main(int argc, char *argv[])
{
    uint8_t err;
    FN_TIME_T now;

    printf("fn_test: calling fn_init()...\n");
    err = fn_init();
    if (err != FN_OK) {
        printf("fn_init() failed: %s (0x%02x)\n", fn_error_string(err), (unsigned)err);
        return 1;
    }
    printf("fn_init() OK\n");

    printf("fn_test: calling fn_is_ready()...\n");
    if (fn_is_ready()) {
        printf("FujiNet is ready.\n");
    } else {
        printf("FujiNet not ready.\n");
        return 1;
    }

    printf("fn_test: calling fn_clock_get()...\n");
    err = fn_clock_get(&now);
    if (err != FN_OK) {
        printf("fn_clock_get() failed: %s (0x%02x)\n", fn_error_string(err), (unsigned)err);
        return 1;
    }
    /* libnix printf has no %llu; the low word is plenty for a smoke test */
    printf("FujiNet clock: %lu\n", (unsigned long)now);

    return 0;
}
