include ../../../make/amiga.mk

UPSTREAM = ../upstream/src

# Fujitzee's platform-specific/vars.h has no PLATFORM_VARS hook (unlike
# battleship's) — it hard-codes per-toolchain includes that none of our
# macros trigger. Force-include ours instead.
CFLAGS += -include amiga_vars.h

# The server payload is read straight into upstream's Game struct, so its
# layout IS the wire format (cc65 packed). m68k alignment would pad it and
# shift every player record; this opts into the #pragma pack upstream's
# misc.h already applies for Open Watcom. Load-bearing — dropping it builds
# a binary that misparses every server response. See include/amiga_vars.h
# and test/host/test_wireformat.c.
CFLAGS += -DFUJITZEE_PACK_STRUCTS

INCLUDES = \
    -Iinclude \
    -I$(UPSTREAM) \
    -I$(UPSTREAM)/platform-specific \
    -I$(GAMEKIT_INC) \
    -I$(COMPAT_INC) \
    -I$(NIO_INC)

COMMON_SRCS = \
    $(UPSTREAM)/main.c \
    $(UPSTREAM)/gamelogic.c \
    $(UPSTREAM)/screens.c \
    $(UPSTREAM)/stateclient.c \
    $(UPSTREAM)/misc.c

# The reusable half of the platform layer lives in libs/amiga-gamekit
# (screen/tiles/sprites, key + joystick decode, waveform bakers, jiffy
# clock). What is left here is fujitzee-specific.
AMIGA_SRCS = src/graphics.c src/input.c src/sound.c src/util.c

SRCS = $(COMMON_SRCS) $(AMIGA_SRCS)

# Headers the sources #include. Listed as build prerequisites so editing one
# actually rebuilds the binary — without this, make sees no changed .c file
# and silently ships the stale binary.
AMIGA_HDRS = $(wildcard include/*.h)

TARGET = fujitzee

# Stack size is set via __stack in src/util.c

all: $(TARGET)

$(TARGET): $(SRCS) $(AMIGA_HDRS) $(GAMEKIT_LIB) $(COMPAT_LIB) $(NIO_ALIB)
	$(CC) $(CFLAGS) $(INCLUDES) -Wl,-x -o $@ $(SRCS) \
	    $(GAMEKIT_LIB) $(COMPAT_LIB) $(NIO_ALIB)

# ---- T1 host unit tests (docs/testing.md) ----
HOST_CC     = cc
# -DFUJITZEE_PACK_STRUCTS mirrors the Amiga build deliberately: the wire-format
# test is only meaningful if the host sees the same packing decision.
HOST_CFLAGS = -std=c99 -Wall -Wextra -g -O0 -DFUJITZEE_PACK_STRUCTS
# amiga_vars.h names the gamekit's GK_KEY_*/GK_JOY_* contract, and the tests
# include upstream's misc.h, so both dirs are on the host path too.
HOST_INC    = -Iinclude -I$(UPSTREAM) -I$(UPSTREAM)/platform-specific \
              -I$(GAMEKIT_INC) -I$(COMPAT_INC) \
              -I../../../libs/fujinet-compat-amiga/test/host
HOST_TESTS  = test/host/test_wireformat.test

# $(AMIGA_HDRS) as a prerequisite: these tests assert on what the headers say,
# so a header edit alone must rebuild them — otherwise make reruns a stale
# binary and reports a pass for code that no longer compiles.
test/host/%.test: test/host/%.c $(AMIGA_HDRS)
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_INC) -o $@ $<

test-host: $(HOST_TESTS)
	@fail=0; for t in $(HOST_TESTS); do echo "== $$t =="; ./$$t || fail=1; done; exit $$fail

clean:
	rm -f $(TARGET) $(HOST_TESTS)

.PHONY: all clean test-host
