CC     = m68k-amigaos-gcc
CFLAGS = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13 \
         -DNO_INLINE_MULDIV -Wno-pointer-sign \
         -DPLATFORM_VARS='"amiga_vars.h"'

UPSTREAM = ../upstream/src
COMPAT   = ../../../libs/fujinet-compat-amiga
NIO_LIB  = ../../../fujinet-nio-lib

INCLUDES = \
    -Iinclude \
    -I$(UPSTREAM) \
    -I$(UPSTREAM)/platform-specific \
    -I$(COMPAT)/include \
    -I$(NIO_LIB)/include

COMPAT_LIB = $(COMPAT)/libfn_compat_amiga.a
NIO_ALIB   = $(NIO_LIB)/build/fujinet-nio-amiga.a

COMMON_SRCS = \
    $(UPSTREAM)/main.c \
    $(UPSTREAM)/gamelogic.c \
    $(UPSTREAM)/screens.c \
    $(UPSTREAM)/stateclient.c \
    $(UPSTREAM)/misc.c

AMIGA_SRCS = src/graphics.c src/input.c src/sound.c src/util.c

SRCS = $(COMMON_SRCS) $(AMIGA_SRCS)

TARGET = battleship

APP_NAME         = battleship
APP_BINARY       = $(TARGET)
EMU_PASS_PATTERN = fujibus: receive:
EMU_TIMEOUT      = 90

# Pre-populate ENVARC: on the ADF so fn_fuji.c's appkey reads succeed
# without runtime shell scripting (which is brittle under KS 1.3).
# Files are built into the ADF via build-adf.sh's ADF_STATIC_DIR feature.
ADF_STATIC_DIR = $(CURDIR)/envarc

# Stack size is set via __stack symbol in platform_init.c

# Appkey files for the ADF:
#   fujinet/0001/01/00 — player name (creator=1, app=1, key=0)
#   fujinet/E41C/05/00 — prefs: byte0=debugFlag, byte1=seenHelp (1=skip help screen)
ENVARC_PLAYER = $(ADF_STATIC_DIR)/fujinet/0001/01/00
ENVARC_PREFS  = $(ADF_STATIC_DIR)/fujinet/E41C/05/00

$(ENVARC_PLAYER):
	mkdir -p $(@D) && printf 'AMIGA' > $@

$(ENVARC_PREFS):
	mkdir -p $(@D) && printf '\000\001\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000' > $@

STRIP_SYMS = ../../../emu/scripts/strip-hunk-symbols.py

all: $(TARGET)

$(TARGET): $(SRCS) $(COMPAT_LIB) $(NIO_ALIB)
	$(CC) $(CFLAGS) $(INCLUDES) -Wl,-x -o $@ $(SRCS) $(COMPAT_LIB) $(NIO_ALIB)

strip: $(TARGET)
	python3 $(STRIP_SYMS) $<

include ../../../emu/template/emu.mk

# Ensure envarc appkey files exist before building the ADF
emu-adf: $(ENVARC_PLAYER) $(ENVARC_PREFS)

clean:
	rm -f $(TARGET)

.PHONY: all clean strip
