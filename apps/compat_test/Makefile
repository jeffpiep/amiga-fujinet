CC      = m68k-amigaos-gcc
CFLAGS  = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13 \
          -Wno-pointer-sign -DNO_INLINE_MULDIV

NIO_LIB   = ../../fujinet-nio-lib
COMPAT    = ../../libs/fujinet-compat-amiga
INCLUDES  = -I$(NIO_LIB)/include -I$(COMPAT)/include
LIBS      = $(COMPAT)/libfn_compat_amiga.a $(NIO_LIB)/build/fujinet-nio-amiga.a

TARGET = compat_test
SRCS   = ../../libs/fujinet-compat-amiga/test/compat_test.c

APP_NAME         = compat_test
APP_BINARY       = $(TARGET)
EMU_PASS_PATTERN = fujibus: receive:
EMU_TIMEOUT      = 90

all: $(TARGET)

include ../../emu/template/emu.mk

$(TARGET): $(SRCS) $(COMPAT)/libfn_compat_amiga.a
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRCS) $(LIBS)

clean: emu-clean
	rm -f $(TARGET)

.PHONY: all clean
