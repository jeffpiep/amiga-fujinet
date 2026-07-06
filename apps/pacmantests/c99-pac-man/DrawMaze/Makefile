# apps/pacmantests/c99-pac-man/DrawMaze/Makefile
#
# C99 / amiga-gcc port of Thom Cherryhomes' "DrawMaze" test harness.
# Draws the Pac-Man maze using Intuition Border (vector) objects.
#
# Usage:
#   make              — build the DrawMaze binary
#   make emu-test     — build an ADF and run it in FS-UAE, capturing a screenshot
#   make emu-clean    — remove ADF and emulator logs
#   make clean        — remove all build artifacts
#
# Like the other pacman harnesses this is PURELY VISUAL: it opens a screen,
# draws the maze, and holds it (Delay). It sends no serial data, so there is no
# log pattern to match — the deliverable is the screenshot. emu/run.sh always
# grabs the framebuffer just before teardown, so we use an EMU_PASS_PATTERN that
# never appears and let the run reach EMU_TIMEOUT. "FAIL (timeout)" is EXPECTED;
# judge success from the PNG:
#   ls -t ../../../../emu/logs/drawmaze/*/screenshot.png | head -1
#
# Prerequisites:
#   - amiga-gcc toolchain on PATH (m68k-amigaos-gcc)
#   - emu/config/paths.env filled in (KICKSTART_ROM, WB_ADF)
#   - fujinet-nio built (run.sh launches it even though this app ignores it):
#       cd fujinet-nio && ./build.sh -p fujibus-rs232-debug

CC      = m68k-amigaos-gcc
CFLAGS  = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13 \
          -Wno-pointer-sign

TARGET  = DrawMaze
SRCS    = DrawMaze.c

# ── Build ─────────────────────────────────────────────────────────────────
# Default target MUST precede the emu.mk include, else emu-adf becomes default.
all: $(TARGET)

$(TARGET): $(SRCS) font.h
	$(CC) $(CFLAGS) -o $@ $(SRCS)

# ── Emulator test config ──────────────────────────────────────────────────
APP_NAME         = drawmaze
APP_BINARY       = $(TARGET)
# No serial traffic → intentionally unmatchable so the full timeout elapses and
# the maze is fully drawn before the screenshot is taken.
EMU_PASS_PATTERN = __drawmaze_visual_never_matches__
EMU_TIMEOUT      = 35

include ../../../../emu/template/emu.mk

clean: emu-clean
	rm -f $(TARGET)

.PHONY: all clean
