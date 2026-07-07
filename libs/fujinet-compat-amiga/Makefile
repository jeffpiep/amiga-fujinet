include ../../make/amiga.mk

INCLUDES = -I$(NIO_INC) -Iinclude

SRCS = src/fn_network.c src/fn_fuji.c
OBJS = $(SRCS:.c=.o)

TARGET = libfn_compat_amiga.a

.PHONY: all clean test-host

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# The T2 smoke test source lives in test/compat_test.c but is built,
# packaged, and run on-target by apps/compat_test (make emu-test) — the
# single owner of the T2 binary. No m68k test rule here.

# ── T1: host unit tests (native cc, fast, CI-safe, no AmigaOS, no backend) ────
# Each test .c #includes the module .c to reach static helpers. See docs/testing.md.
HOST_CC     = cc
HOST_CFLAGS = -std=c99 -Wall -Wextra -g -O0
HOST_INC    = -Iinclude -I$(NIO_INC) -Itest/host
HOST_TESTS  = test/host/test_network.test

test/host/%.test: test/host/%.c test/host/fn_test.h
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_INC) -o $@ $<

test-host: $(HOST_TESTS)
	@fail=0; for t in $(HOST_TESTS); do echo "== $$t =="; ./$$t || fail=1; done; exit $$fail

clean:
	rm -f $(OBJS) $(TARGET) $(HOST_TESTS)
