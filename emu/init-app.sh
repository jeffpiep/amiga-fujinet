#!/usr/bin/env bash
# emu/init-app.sh — Scaffold a new Amiga FujiNet app with emu-test support
#
# Usage: bash emu/init-app.sh <appname>
#
# Creates:
#   apps/<appname>/Makefile  — build + emu.mk include, with TODO stubs
#   apps/<appname>/main.c    — minimal fn_init() / fn_is_ready() skeleton

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

APP_NAME="${1:?Usage: $(basename "$0") <appname>}"
APP_DIR="$PROJECT_ROOT/apps/$APP_NAME"

if [ -d "$APP_DIR" ]; then
    echo "ERROR: $APP_DIR already exists" >&2
    exit 1
fi

echo "Scaffolding: $APP_DIR"
mkdir -p "$APP_DIR"

# ── Makefile ───────────────────────────────────────────────────────────────
cat > "$APP_DIR/Makefile" <<EOF
# apps/$APP_NAME/Makefile

CC      = m68k-amigaos-gcc
CFLAGS  = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13

NIO_LIB  = ../../fujinet-nio-lib
INCLUDES = -I\$(NIO_LIB)/include
LIBS     = \$(NIO_LIB)/build/fujinet-nio-amiga.a

TARGET  = $APP_NAME
SRCS    = main.c

# ── Emulator test config ──────────────────────────────────────────────────
APP_NAME         = $APP_NAME
APP_BINARY       = \$(TARGET)
EMU_PASS_PATTERN = fujibus: receive:
# EMU_FAIL_PATTERN =
# EMU_STARTUP_ARGS = arg1 arg2
EMU_TIMEOUT      = 60

include ../../emu/template/emu.mk

# ── Build ─────────────────────────────────────────────────────────────────
all: \$(TARGET)

\$(TARGET): \$(SRCS)
	\$(CC) \$(CFLAGS) \$(INCLUDES) -o \$@ \$^ \$(LIBS)

clean: emu-clean
	rm -f \$(TARGET)

.PHONY: all clean
EOF

# ── main.c ─────────────────────────────────────────────────────────────────
cat > "$APP_DIR/main.c" <<EOF
#include <stdio.h>
#include "fujinet-nio.h"

int main(int argc, char *argv[])
{
    uint8_t err;

    printf("$APP_NAME: starting\\n");

    err = fn_init();
    if (err != FN_OK) {
        printf("fn_init() failed: %s\\n", fn_error_string(err));
        return 1;
    }

    if (!fn_is_ready()) {
        printf("FujiNet not ready.\\n");
        return 1;
    }

    printf("FujiNet ready.\\n");
    /* TODO: add your app logic here */
    return 0;
}
EOF

echo ""
echo "Created:"
echo "  $APP_DIR/Makefile"
echo "  $APP_DIR/main.c"
echo ""
echo "Next steps:"
echo "  1. Edit $APP_DIR/main.c"
echo "  2. make -C apps/$APP_NAME           # compile"
echo "  3. make -C apps/$APP_NAME emu-test  # build ADF + run in emulator"
