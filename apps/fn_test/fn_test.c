/*
 * fn_test.c - Minimal FujiNet init and readiness test
 *
 * Calls fn_init() then fn_is_ready() and prints the result.
 * Useful for verifying the serial transport comes up correctly
 * before exercising network operations.
 *
 * Requires: exec.library, serial.device (via fujinet-nio-lib)
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * See: contracts/amiga-transport-api.md
 */

#include <stdio.h>
#include "fujinet-nio.h"

int main(int argc, char *argv[])
{
    uint8_t err;

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

    return 0;
}
