# apps/pacmantests/c99-pac-man/Ghosts/Makefile
#
# C99 / amiga-gcc port of Thom Cherryhomes' "Ghosts" bitplane test harness.
#
# Usage:
#   make              — build the Ghosts binary
#   make emu-test     — build an ADF and run it in FS-UAE, capturing a screenshot
#   make emu-clean    — remove ADF and emulator logs
#   make clean        — remove all build artifacts
#
# This is a PURELY VISUAL harness: it opens a custom screen, draws the ghost
# bitplanes, and Wait()s on the window close gadget. It sends no serial data,
# so there is no log pattern to match — the deliverable is the screenshot.
#
# emu/run.sh always captures the framebuffer just before teardown, so we set an
# EMU_PASS_PATTERN that will never appear and let the run reach EMU_TIMEOUT.
# The "FAIL (timeout)" result is EXPECTED here; judge success from the PNG:
#   ls -t ../../../../emu/logs/ghosts/*/screenshot.png | head -1
#
# Prerequisites:
#   - amiga-gcc toolchain on PATH (m68k-amigaos-gcc)
#   - emu/config/paths.env filled in (KICKSTART_ROM, WB_ADF)
#   - fujinet-nio built (run.sh launches it even though this app ignores it):
#       cd fujinet-nio && ./build.sh -p fujibus-rs232-debug

CC      = m68k-amigaos-gcc
CFLAGS  = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13 \
          -Wno-pointer-sign

TARGET  = Ghosts
SRCS    = Ghosts.c

# ── Build ─────────────────────────────────────────────────────────────────
# Default target MUST precede the emu.mk include, else emu-adf becomes default.
all: $(TARGET)

$(TARGET): $(SRCS) Bobs.h font.h
	$(CC) $(CFLAGS) -o $@ $(SRCS)

# ── Emulator test config ──────────────────────────────────────────────────
APP_NAME         = ghosts
APP_BINARY       = $(TARGET)
# No serial traffic → intentionally unmatchable so the full timeout elapses and
# the screen is fully drawn before the screenshot is taken.
EMU_PASS_PATTERN = __ghosts_visual_never_matches__
EMU_TIMEOUT      = 35

include ../../../../emu/template/emu.mk

clean: emu-clean
	rm -f $(TARGET)

.PHONY: all clean
