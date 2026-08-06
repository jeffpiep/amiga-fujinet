include ../../../make/amiga.mk

CFLAGS += -DPLATFORM_VARS='"amiga_vars.h"'

UPSTREAM = ../upstream/src

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
# clock). What is left here is battleship-specific.
AMIGA_SRCS = src/graphics.c src/gfxsetup.c src/cellmap.c src/aim.c \
             src/mousemap.c src/input.c src/sound.c src/util.c

SRCS = $(COMMON_SRCS) $(AMIGA_SRCS)

# Headers the sources #include (tiles.h is the art surface). Listed as build
# prerequisites so editing a tile/header actually rebuilds the binary — without
# this, make sees no changed .c file and silently ships the stale binary.
AMIGA_HDRS = $(wildcard include/*.h)

TARGET = battleship

APP_NAME         = battleship
APP_BINARY       = $(TARGET)
EMU_PASS_PATTERN = fujibus: receive:
EMU_TIMEOUT      = 90

# AppKeys live server-side as persist:///FujiNet/*.key files, served by
# fujinet-nio-lib's legacy appkey API (contracts/fujinet-appkey-protocol.md),
# so the ADF needs no ENVARC: seeding. A missing key reads as 0 bytes and
# the game prompts for name / shows help. To pre-seed a key for a test,
# drop <server data root>/FujiNet/<creator><app><key>.key before boot.

# Stack size is set via __stack in util.c

STRIP_SYMS = ../../../emu/scripts/strip-hunk-symbols.py

all: $(TARGET)

$(TARGET): $(SRCS) $(AMIGA_HDRS) $(GAMEKIT_LIB) $(COMPAT_LIB) $(NIO_ALIB)
	$(CC) $(CFLAGS) $(INCLUDES) -Wl,-x -o $@ $(SRCS) \
	    $(GAMEKIT_LIB) $(COMPAT_LIB) $(NIO_ALIB)

strip: $(TARGET)
	python3 $(STRIP_SYMS) $<

# ---- Art-pass tile gallery (preview harness, docs/plan-track1b-battleship.md) ----
# Standalone: gfxcore.c + tiles.h, no upstream game. `make tilegallery` builds
# the binary; `make gallery-adf` wraps it in a bootable ADF via the emu tooling.
GALLERY        = tilegallery
GALLERY_SRCS   = src/gallery_main.c src/gfxsetup.c
# tiles.h (the art surface) is #included, not compiled — list the headers as
# prerequisites so editing a tile actually rebuilds the binary (and thus the
# ADF). Without this, `make gallery-adf` silently packages the stale binary.
GALLERY_HDRS   = include/tiles.h include/gfxsetup.h include/pens.h \
                 include/cellmap.h include/amiga_vars.h
_GALLERY_ADF   = $(CURDIR)/$(GALLERY).adf

$(GALLERY): $(GALLERY_SRCS) $(GALLERY_HDRS) $(GAMEKIT_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -Wl,-x -o $@ $(GALLERY_SRCS) $(GAMEKIT_LIB)

gallery-adf: $(GALLERY)
	APP_NAME=$(GALLERY) \
	APP_BINARY=$(abspath $(GALLERY)) \
	ADF_OUT=$(_GALLERY_ADF) \
	../../../emu/scripts/build-adf.sh

.PHONY: gallery-adf

include ../../../emu/template/emu.mk

# ---- T1 host unit tests (docs/testing.md) ----
HOST_CC     = cc
HOST_CFLAGS = -std=c99 -Wall -Wextra -g -O0
# amiga_vars.h names the gamekit's GK_KEY_*/GK_JOY_* contract, so the
# gamekit include dir is on the host path too. joydecode/keytrans/sndgen are
# tested in libs/amiga-gamekit now — these are the battleship-only modules.
HOST_INC    = -Iinclude -I../../../libs/amiga-gamekit/include \
              -I../../../libs/fujinet-compat-amiga/test/host
HOST_TESTS  = test/host/test_cellmap.test test/host/test_mousemap.test

test/host/%.test: test/host/%.c
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_INC) -o $@ $<

test-host: $(HOST_TESTS)
	@fail=0; for t in $(HOST_TESTS); do echo "== $$t =="; ./$$t || fail=1; done; exit $$fail

clean:
	rm -f $(TARGET) $(GALLERY) $(_GALLERY_ADF) $(HOST_TESTS)

.PHONY: all clean strip test-host
