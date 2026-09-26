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
#   - fujinet-nio-lib built: cd ../../fujinet-nio-lib && make amiga
#   - emu/config/paths.env filled in (copy from paths.env.example)

include ../../make/amiga.mk

INCLUDES = -I$(NIO_INC)
LIBS     = $(NIO_ALIB)

TARGET  = fn_test
SRCS    = fn_test.c

# ── Emulator test config ──────────────────────────────────────────────────
APP_NAME         = fn_test
APP_BINARY       = $(TARGET)
# fn_init() and fn_is_ready() send nothing: the broker opens serial.device
# lazily on the first exchange. So fn_test also reads the clock, a local
# round trip that needs no internet, and the server logging the request is
# the proof the link works.
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
