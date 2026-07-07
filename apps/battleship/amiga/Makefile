include ../../../make/amiga.mk

CFLAGS += -DPLATFORM_VARS='"amiga_vars.h"'

UPSTREAM = ../upstream/src

INCLUDES = \
    -Iinclude \
    -I$(UPSTREAM) \
    -I$(UPSTREAM)/platform-specific \
    -I$(COMPAT_INC) \
    -I$(NIO_INC)

COMMON_SRCS = \
    $(UPSTREAM)/main.c \
    $(UPSTREAM)/gamelogic.c \
    $(UPSTREAM)/screens.c \
    $(UPSTREAM)/stateclient.c \
    $(UPSTREAM)/misc.c

AMIGA_SRCS = src/graphics.c src/input.c src/joydecode.c src/sound.c src/util.c

SRCS = $(COMMON_SRCS) $(AMIGA_SRCS)

TARGET = battleship

APP_NAME         = battleship
APP_BINARY       = $(TARGET)
EMU_PASS_PATTERN = fujibus: receive:
EMU_TIMEOUT      = 90

# AppKeys now live server-side via the fujinet-nio app-store
# (contracts/fujinet-appkey-protocol.md); missing keys fall back to
# fn_fuji.c's built-in defaults, so the ADF needs no ENVARC: seeding.
# To pre-seed keys for a test, write them to the *server* with
# fujinet-nio's py/fujinet_tools before boot.

# Stack size is set via __stack in util.c

STRIP_SYMS = ../../../emu/scripts/strip-hunk-symbols.py

all: $(TARGET)

$(TARGET): $(SRCS) $(COMPAT_LIB) $(NIO_ALIB)
	$(CC) $(CFLAGS) $(INCLUDES) -Wl,-x -o $@ $(SRCS) $(COMPAT_LIB) $(NIO_ALIB)

strip: $(TARGET)
	python3 $(STRIP_SYMS) $<

include ../../../emu/template/emu.mk

# ---- T1 host unit tests (docs/testing.md) ----
HOST_CC     = cc
HOST_CFLAGS = -std=c99 -Wall -Wextra -g -O0
HOST_INC    = -Iinclude -I../../../libs/fujinet-compat-amiga/test/host
HOST_TESTS  = test/host/test_joydecode.test

test/host/%.test: test/host/%.c
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_INC) -o $@ $<

test-host: $(HOST_TESTS)
	@fail=0; for t in $(HOST_TESTS); do echo "== $$t =="; ./$$t || fail=1; done; exit $$fail

clean:
	rm -f $(TARGET) $(HOST_TESTS)

.PHONY: all clean strip test-host
