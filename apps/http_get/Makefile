# apps/http_get/Makefile
#
# Builds the http_get example for Amiga (m68k-amigaos-gcc).
#
# Usage:
#   make              - build http_get executable
#   make clean        - remove build artifacts
#
# Prerequisites:
#   - amiga-gcc toolchain on PATH (https://github.com/bebbo/amiga-gcc)
#   - fujinet-nio-lib built for Amiga:
#       cd ../../fujinet-nio-lib && make TARGET=amiga
#
# Output: http_get  (AmigaOS executable — copy to Amiga or run in WinUAE)

CC        = m68k-amigaos-gcc
CFLAGS    = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13

NIO_LIB   = ../../fujinet-nio-lib
INCLUDES  = -I$(NIO_LIB)/include
LIBS      = $(NIO_LIB)/build/fujinet-nio-amiga.a

TARGET    = http_get
SRCS      = http_get.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
