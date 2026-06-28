CC      = m68k-amigaos-gcc
AR      = m68k-amigaos-ar
CFLAGS  = -Wall -Wextra -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13 -DNO_INLINE_MULDIV \
          -Wno-pointer-sign

NIOINC  = ../../fujinet-nio-lib/include
INCLUDES = -I$(NIOINC) -Iinclude

SRCS = src/fn_network.c src/fn_fuji.c
OBJS = $(SRCS:.c=.o)

TARGET = libfn_compat_amiga.a

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

test/compat_test: test/compat_test.c $(TARGET)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ -L. -lfn_compat_amiga -L../../fujinet-nio-lib -lfujinet_nio_amiga

test: test/compat_test

clean:
	rm -f $(OBJS) $(TARGET) test/compat_test
