# apps/http_get/Makefile
#
# Usage:
#   make              — build http_get binary
#   make emu-test     — build ADF and run in FS-UAE with fujinet-nio
#   make emu-clean    — remove ADF and emulator logs
#   make clean        — remove all build artifacts
#
# Prerequisites:
#   - amiga-gcc toolchain on PATH (https://github.com/bebbo/amiga-gcc)
#   - fujinet-nio-lib built: cd ../../fujinet-nio-lib && make TARGET=amiga
#   - emu/config/paths.env filled in (copy from paths.env.example)

CC      = m68k-amigaos-gcc
CFLAGS  = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13

NIO_LIB  = ../../fujinet-nio-lib
INCLUDES = -I$(NIO_LIB)/include
LIBS     = $(NIO_LIB)/build/fujinet-nio-amiga.a

TARGET  = http_get
SRCS    = http_get.c

# ── Emulator test config ──────────────────────────────────────────────────
APP_NAME         = http_get
APP_BINARY       = $(TARGET)
EMU_PASS_PATTERN = fujibus: receive:
EMU_STARTUP_ARGS = https://wttr.in/Baltimore,MD?format=3
EMU_TIMEOUT      = 90

include ../../emu/template/emu.mk

# ── Build ─────────────────────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

clean: emu-clean
	rm -f $(TARGET)

.PHONY: all clean
