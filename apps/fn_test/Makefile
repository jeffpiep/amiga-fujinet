# apps/fn_test/Makefile
#
# Usage:
#   make              — build fn_test binary
#   make emu-test     — build ADF and run in FS-UAE with fujinet-nio
#   make emu-clean    — remove ADF and emulator logs
#   make clean        — remove all build artifacts
#
# Prerequisites:
#   - amiga-gcc toolchain on PATH (https://codeberg.org/bebbo/amiga-gcc)
#   - fujinet-nio-lib built: cd ../../fujinet-nio-lib && make TARGET=amiga
#   - emu/config/paths.env filled in (copy from paths.env.example)

CC      = m68k-amigaos-gcc
CFLAGS  = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13

NIO_LIB  = ../../fujinet-nio-lib
INCLUDES = -I$(NIO_LIB)/include
LIBS     = $(NIO_LIB)/build/fujinet-nio-amiga.a

TARGET  = fn_test
SRCS    = fn_test.c

# ── Emulator test config ──────────────────────────────────────────────────
APP_NAME         = fn_test
APP_BINARY       = $(TARGET)
EMU_PASS_PATTERN = fujibus: receive:
EMU_TIMEOUT      = 60

include ../../emu/template/emu.mk

# ── Build ─────────────────────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

clean: emu-clean
	rm -f $(TARGET)

.PHONY: all clean
