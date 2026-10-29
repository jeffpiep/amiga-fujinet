include ../../make/amiga.mk

INCLUDES = -Iinclude

SRCS = src/gfxcore.c src/keytrans.c src/gkkeyq.c src/joydecode.c src/gkjoy.c \
       src/sndgen.c src/gksound.c src/gktimer.c src/gkclock.c src/gkrandom.c
OBJS = $(SRCS:.c=.o)

TARGET = libamiga_gamekit.a

.PHONY: all clean test-host

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c $(wildcard include/*.h)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ── T1: host unit tests (native cc, fast, CI-safe, no AmigaOS) ───────────────
# Each test .c #includes the module .c to reach static helpers. The fn_test.h
# harness is shared with the compat layer — one copy, per docs/testing.md.
HOST_CC     = cc
HOST_CFLAGS = -std=c99 -Wall -Wextra -g -O0
HOST_INC    = -Iinclude -I../fujinet-compat-amiga/test/host
HOST_TESTS  = test/host/test_joydecode.test test/host/test_keytrans.test \
              test/host/test_sndgen.test test/host/test_gkclock.test \
              test/host/test_gkrandom.test

test/host/%.test: test/host/%.c
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_INC) -o $@ $<

test-host: $(HOST_TESTS)
	@fail=0; for t in $(HOST_TESTS); do echo "== $$t =="; ./$$t || fail=1; done; exit $$fail

clean:
	rm -f $(OBJS) $(TARGET) $(HOST_TESTS)
