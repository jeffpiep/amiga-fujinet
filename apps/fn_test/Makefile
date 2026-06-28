# apps/fn_test/Makefile
#
# Builds the fn_test diagnostic for Amiga (m68k-amigaos-gcc).
#
# Usage:
#   make              - build fn_test executable
#   make clean        - remove build artifacts
#
# Prerequisites:
#   - amiga-gcc toolchain on PATH (https://codeberg.org/bebbo/amiga-gcc)
#   - fujinet-nio-lib built for Amiga:
#       cd ../../fujinet-nio-lib && make TARGET=amiga

CC        = m68k-amigaos-gcc
CFLAGS    = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13

NIO_LIB   = ../../fujinet-nio-lib
INCLUDES  = -I$(NIO_LIB)/include
LIBS      = $(NIO_LIB)/build/fujinet-nio-amiga.a

TARGET    = fn_test
SRCS      = fn_test.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
