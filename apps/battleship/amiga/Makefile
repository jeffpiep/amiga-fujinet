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

# Create ENVARC: assign and player name appkey before the app runs.
# fuji_read_appkey() reads ENVARC:fujinet/0001/01/00 (creator=1,app=1,key=0).
# Without a player name the app blocks on an interactive name-entry screen.
define EMU_STARTUP_PREFIX
MakDir RAM:fujinet
MakDir RAM:fujinet/0001
MakDir RAM:fujinet/0001/01
MakDir RAM:fujinet/E41C
MakDir RAM:fujinet/E41C/05
Assign ENVARC: RAM:
Echo AMIGA NOLINE > RAM:fujinet/0001/01/00
Echo AX NOLINE > RAM:fujinet/E41C/05/00
endef
export EMU_STARTUP_PREFIX

include ../../../emu/template/emu.mk

all: $(TARGET)

$(TARGET): $(SRCS) $(COMPAT_LIB) $(NIO_ALIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRCS) $(COMPAT_LIB) $(NIO_ALIB)

clean:
	rm -f $(TARGET)

.PHONY: all clean
