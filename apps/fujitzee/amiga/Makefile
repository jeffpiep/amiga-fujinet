include ../../../make/amiga.mk

UPSTREAM = ../upstream/src

# Fujitzee's platform-specific/vars.h has no PLATFORM_VARS hook (unlike
# battleship's) — it hard-codes per-toolchain includes that none of our
# macros trigger. Force-include ours instead; zero upstream patches. It also
# carries the wire-format packing shim, which has to be in effect before any
# translation unit parses misc.h — see include/amiga_vars.h.
CFLAGS += -include amiga_vars.h

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
HOST_CFLAGS = -std=c99 -Wall -Wextra -g -O0
# amiga_vars.h names the gamekit's GK_KEY_*/GK_JOY_* contract and pulls in
# upstream's misc.h, so both dirs are on the host path too.
HOST_INC    = -Iinclude -I$(UPSTREAM) -I$(UPSTREAM)/platform-specific \
              -I../../../libs/amiga-gamekit/include \
              -I../../../libs/fujinet-compat-amiga/test/host
HOST_TESTS  = test/host/test_wireformat.test

test/host/%.test: test/host/%.c
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_INC) -o $@ $<

test-host: $(HOST_TESTS)
	@fail=0; for t in $(HOST_TESTS); do echo "== $$t =="; ./$$t || fail=1; done; exit $$fail

clean:
	rm -f $(TARGET) $(HOST_TESTS)

.PHONY: all clean test-host
