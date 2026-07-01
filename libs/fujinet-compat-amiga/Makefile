CC      = m68k-amigaos-gcc
AR      = m68k-amigaos-ar
CFLAGS  = -Wall -Wextra -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13 -DNO_INLINE_MULDIV \
          -Wno-pointer-sign

NIOINC  = ../../fujinet-nio-lib/include
INCLUDES = -I$(NIOINC) -Iinclude

SRCS = src/fn_network.c src/fn_fuji.c
OBJS = $(SRCS:.c=.o)

TARGET = libfn_compat_amiga.a

.PHONY: all clean test test-host

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ── T2: emulator/on-target smoke test (m68k, needs live fujinet-nio backend) ──
# Cross-compiles only; run it under FS-UAE via apps/compat_test (make emu-test).
test/compat_test: test/compat_test.c $(TARGET)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ -L. -lfn_compat_amiga -L../../fujinet-nio-lib -lfujinet_nio_amiga

test: test/compat_test

# ── T1: host unit tests (native cc, fast, CI-safe, no AmigaOS, no backend) ────
# Each test .c #includes the module .c to reach static helpers. See docs/testing.md.
HOST_CC     = cc
HOST_CFLAGS = -std=c99 -Wall -Wextra -g -O0
HOST_INC    = -Iinclude -I$(NIOINC) -Itest/host
HOST_TESTS  = test/host/test_network.test

test/host/%.test: test/host/%.c test/host/fn_test.h
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_INC) -o $@ $<

test-host: $(HOST_TESTS)
	@fail=0; for t in $(HOST_TESTS); do echo "== $$t =="; ./$$t || fail=1; done; exit $$fail

clean:
	rm -f $(OBJS) $(TARGET) test/compat_test $(HOST_TESTS)
