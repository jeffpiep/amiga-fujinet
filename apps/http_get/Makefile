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
#   - fujinet-nio-lib built: cd ../../fujinet-nio-lib && make amiga
#   - emu/config/paths.env filled in (copy from paths.env.example)

include ../../make/amiga.mk

INCLUDES = -I$(NIO_INC)
LIBS     = $(NIO_ALIB)

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
