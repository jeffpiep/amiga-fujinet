# Implementation Plan: Track 1B — Battleship Amiga Port

**Depends on:** Track 1A (`libfn_compat_amiga.a`) must be complete and tested  
**Blocks:** nothing  
**Status:** Not started

---

## Goal

A playable two-player Battleship game running on Amiga (KS 1.3+), booting from
ADF, communicating with `https://battleship.carr-designs.com/` via FujiNet over
RS-232. No TCP stack required.

---

## Source Setup

The upstream repo is `https://github.com/FujiNetWIFI/fujinet-battleship`.

Add it as a git submodule:
```bash
git submodule add https://github.com/FujiNetWIFI/fujinet-battleship apps/battleship/upstream
```

The Amiga platform layer lives alongside it:
```
apps/battleship/
  upstream/          ← submodule (read-only; do not modify)
  amiga/             ← our Amiga-specific platform layer
    src/
      graphics.c
      input.c
      sound.c
      util.c
      vars.h
    Makefile
```

The `Makefile` compiles the upstream `src/*.c` files (common game logic) plus
our `amiga/src/*.c` files into a single Amiga executable, linking against
`libfn_compat_amiga.a` and `fujinet-nio-amiga.a`.

---

## Platform Interface Contracts

The upstream game defines these interfaces in
`upstream/src/platform-specific/*.h`. Each must be satisfied by our
`amiga/src/` implementation.

### `graphics.h`

| Function | Implementation |
|---|---|
| `initGraphics()` | No-op for console; clear screen via ANSI escape or `puts("\014")` |
| `resetScreen()` | Move cursor to home; clear |
| `drawText(x, y, str)` | ANSI cursor positioning: `\033[y;xH` + `puts(str)` |
| `drawIcon(x, y, icon_id)` | Print a single ASCII character for each icon type |
| `drawShip(x, y, dir, len, hit)` | Print ship characters at grid coords |
| `drawGamefield(field, x, y)` | Print 10×10 grid from `uint8_t[100]` state array |
| `clearGamefield(x, y)` | Print 10×10 spaces |

**KS 1.3 note:** ANSI escape codes work on the standard Amiga console (CON:
device). No Intuition required. This keeps the first pass simple and fast.

**Stretch (later):** Proper Intuition window with bitmap graphics. Keep
console working as a fallback.

### `input.h`

| Function | Implementation |
|---|---|
| `kbhit()` | Non-blocking: `WaitForChar(Input(), 0)` (dos.library, KS 1.3) |
| `cgetc()` | Blocking: `FGetC(Input())` |
| `readJoystick(port)` | Read `gameport.device` CIA registers; return direction + fire bitmask |
| `waitvsync()` | `WaitTOF()` (graphics.library, KS 1.3) |
| `enableKeySounds(on)` | No-op for now |

**Joystick:** Direct CIA register read is KS 1.3 safe and avoids `gameport.device`
complexity for the first pass. Port 1 = `0xDC000` joy register; port 2 = `0xDFF00C`.

### `sound.h`

| Function | Implementation |
|---|---|
| `soundCursor()` | Short 1kHz tone, 50ms via `audio.device` |
| `soundSelect()` | Short 2kHz tone, 80ms |
| `soundAttack()` | Descending sweep |
| `soundHit()` | Explosion-like noise burst |
| `soundMiss()` | Low "splash" tone |
| `soundSink()` | Longer descending chord |

**First pass:** All sound functions can be no-ops. Add audio in a later iteration
once the game loop is working end-to-end.

`audio.device` is available on KS 1.3. Use a small static sample buffer for
each effect.

### `util.h`

| Function | Implementation |
|---|---|
| `resetTimer()` | Record `ReadEClock()` or `timer.device` base value |
| `getTime()` | Return elapsed ticks since last `resetTimer()` |
| `getJiffiesPerSecond()` | Return 50 (PAL) or 60 (NTSC); detect via `GfxBase->DisplayFlags` |
| `getRandomNumber(max)` | Linear feedback shift register seeded from `ReadEClock()` |
| `quit()` | `exit(0)` |
| `housekeeping()` | No-op (Atari-specific) |

---

## Build System

`apps/battleship/amiga/Makefile`:

```makefile
CC      = m68k-amigaos-gcc
CFLAGS  = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13 -DNO_INLINE_MULDIV
INCLUDES = \
    -I../upstream/src \
    -I../upstream/src/platform-specific \
    -I../../../libs/fujinet-compat-amiga/include \
    -I../../../fujinet-nio-lib/include

COMPAT_LIB  = ../../../libs/fujinet-compat-amiga/libfn_compat_amiga.a
NIO_LIB     = ../../../fujinet-nio-lib/build/fujinet-nio-amiga.a

COMMON_SRCS = \
    ../upstream/src/main.c \
    ../upstream/src/gamelogic.c \
    ../upstream/src/screens.c \
    ../upstream/src/stateclient.c \
    ../upstream/src/misc.c

AMIGA_SRCS = src/graphics.c src/input.c src/sound.c src/util.c

SRCS = $(COMMON_SRCS) $(AMIGA_SRCS)
OBJS = $(notdir $(SRCS:.c=.o))

battleship: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(COMPAT_LIB) $(NIO_LIB)

%.o: ../upstream/src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) battleship
```

---

## Phases

### Phase 1 — Compile and link (no gameplay)

Get the binary to build without errors. All platform functions can be stubs
that print their name and return. Goal: confirm the upstream common code
compiles cleanly with amiga-gcc and the compat layer resolves all symbols.

Expected first-run issues:
- `int16_t`/`uint16_t` size assumptions (68000 is 16-bit int natively — check)
- Any POSIX-isms in the common code (no `malloc`, no `FILE*`, no `printf` format string issues)

### Phase 2 — Console game loop

Implement `graphics.c` (ANSI console) and `input.c` (keyboard only, no joystick).
Sound stubs. `util.c` with timer and random.

Goal: join a table, see the lobby, place ships, play a full game — text-only,
keyboard-controlled.

### Phase 3 — Full platform layer

- Add `audio.device` sound effects
- Add joystick input (`gameport.device` CIA read)
- Polish screen layout for 80×25 console

### Phase 4 — ADF boot test

- Build ADF using `/emu-build-and-boot`
- Boot in FS-UAE with `fujinet-nio` running
- Play a full game against a second player (or a bot table if one exists)
- Test on real Amiga 500 + PiStorm

---

## Verification Checklist

- [ ] Binary links without errors (Phase 1)
- [ ] Can join a table and see lobby (Phase 2)
- [ ] Can place ships and play a full game to completion (Phase 2)
- [ ] Sound effects play on hit/miss/sink (Phase 3)
- [ ] Joystick moves cursor (Phase 3)
- [ ] Boots from ADF in emulator, plays full game (Phase 4)
- [ ] Tested on real Amiga 500 + PiStorm (Phase 4)
- [ ] Player name and server URL persisted via AppKey/ENVARC: across reboots
