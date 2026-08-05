# Implementation Plan: Track 1C — Fujitzee Amiga Port

**Depends on:** Track 1A (`libfn_compat_amiga.a`) ✅; Track 1B (Battleship) for the
platform-layer code being extracted in Phase 0
**Blocks:** nothing
**Status:** Phase 0 complete (2026-08-05) — `libs/amiga-gamekit` extracted and
Battleship rebuilt against it. Upstream submodule pinned, port surface audited,
phases below agreed. Next: Phase 1.

---

## Goal

A playable multiplayer Fujitzee (Yahtzee-style dice game) on Amiga (KS 1.3+),
booting from ADF, talking to `https://fujitzee.carr-designs.com/` via FujiNet
over RS-232. Same shape as Battleship: upstream game logic untouched, Amiga
platform layer in this repo.

Upstream: [`FujiNetWIFI/fujinet-fujitzee`](https://github.com/FujiNetWIFI/fujinet-fujitzee),
pinned read-only at `apps/fujitzee/upstream`.

---

## Port surface audit (done 2026-08-05)

Done up front rather than discovered phase by phase — this is the first process
change from Track 1B (see "What we're doing differently" below).

**FujiNet API usage — fully covered by the existing compat layer.** The entire
game touches six functions, all already implemented and proven by Battleship:

| Used by fujitzee | Status |
|---|---|
| `network_open` / `network_read` / `network_close` | ✅ `libs/fujinet-compat-amiga/src/fn_network.c` |
| `fuji_set_appkey_details` / `fuji_read_appkey` / `fuji_write_appkey` | ✅ `src/fn_fuji.c`, served by nio-lib's legacy appkey API |

No compat-layer work is expected. AppKeys used: `e41c0300` (prefs) and the
shared lobby keys `00010100` (username) / `00010103` (server URL) — the same
lobby creator/app IDs Battleship already exercises.

**Platform interfaces to implement** (`upstream/src/platform-specific/`):

| Header | Surface | Amiga approach |
|---|---|---|
| `graphics.h` | ~25 functions: `drawText`/`drawTextAlt`/`drawChar`, `drawBox`/`drawLine`/`drawBorder`/`drawBoard`, `drawDie`, `drawFujitzee`, `drawClock`, `drawConnectionIcon`, `drawDiceCursor`, `setHighlight`, `save`/`restoreScreenBuffer`, `cycleNextColor`/`setColorMode` | Custom 320×200×4 screen via the extracted gfx core; dice/icons as tiles, everything else as text cells |
| `input.h` | `readJoystick()` only | Extracted `joydecode` + IDCMP keyboard, same as Battleship |
| `sound.h` | 13 effects + `pause(frames)` + key-sound enable/disable | Extracted `sndgen` waveform bakers + `audio.device` playback |
| `util.h` | `resetTimer`, `getTime`, `quit`, `housekeeping`, `getJiffiesPerSecond` | Extracted `util` (DateStamp jiffies) — but see the PAL/NTSC note below |
| `vars.h` | screen dims, icon codes, key map, layout constants | Our `amiga_vars.h`, force-included (see below) |

Fujitzee's surface is **wider but shallower** than Battleship's: more drawing
entry points, but no blitter-heavy animation, no ship-placement mode, and no
mouse requirement. The renderer is a character-grid + a handful of tiles.

### Two upstream-integration wrinkles

1. **No `PLATFORM_VARS` hook.** Battleship's `platform-specific/vars.h` includes
   `PLATFORM_VARS`, which let us point it at our own header. Fujitzee's version
   hard-codes `#include "../atari/vars.h"` and friends, each self-guarded by a
   compiler macro (`__ATARI__`, `__WATCOMC__`, …). Under `m68k-amigaos-gcc`
   none of those fire, so upstream's `vars.h` expands to nothing but `ESCAPE`.
   **We supply ours with `-include amiga_vars.h` on the command line** — zero
   upstream patches, no submodule fork. Propose the `PLATFORM_VARS` hook
   upstream separately for consistency; the port does not wait on it.

2. **`misc.h` includes `<conio.h>` and `<joystick.h>`; `gamelogic.c` includes
   `<peekpoke.h>`.** These are cc65/CMOC headers. Upstream ships stubs in
   `src/include/` for non-cc65 toolchains, but they are wrong for us
   (`joystick.h` is CMOC-only, and adding that dir would shadow real headers).
   We ship our own three-header shim in `apps/fujitzee/amiga/include/` —
   `conio.h` (kbhit/cgetc, same as Battleship's), `joystick.h` (the `JOY_*`
   accessor macros over our bit layout), and an empty `peekpoke.h` (fujitzee
   includes it but never calls `PEEK`/`POKE`).

---

## What we're doing differently from Track 1B

Six deliberate changes. The first is the big one.

### 1. Extract the reusable platform layer into a shared library (Phase 0)

Track 1B built genuinely reusable Amiga machinery, but it lives inside
`apps/battleship/amiga/src/`. Copy-pasting it into a second port would fork it
permanently. Extract first, then port:

**New `libs/amiga-gamekit/` → `libamiga_gamekit.a`**

| Module | From | Outcome |
|---|---|---|
| `gfxcore.c` | battleship | ✅ Extracted. Screen geometry, palette, tile bank and sprite overlay now come in as a `struct gfx_config` at `gfx_open()`. The Battleship-specific bits came out: pens → `apps/battleship/amiga/include/pens.h`, the aim handshake → `src/aim.c`, the 3 fixed attack-cursor slots → a generic N-slot sprite overlay (`gfx_sprite_move/hide/sweep`) |
| `keytrans.c` | battleship | ✅ Extracted unchanged, retargeted at `GK_KEY_*` |
| `joydecode.c` | battleship | ✅ Extracted unchanged, retargeted at `GK_JOY_*` |
| `sndgen.c` | battleship | ✅ Extracted unchanged |
| `util.c` (timer/random) | battleship | ✅ Split: clock → `gktimer.c`/`gkclock.c`, PRNG → `gkrandom.c`. Battleship's `util.c` is now a ~10-line adapter holding only `__stack`, `itoa`, and the upstream-facing names |
| *(new)* `gkinput.h` | — | ✅ Added. The key/joy values the decoders emit were previously duplicated between `keytrans.c` and `amiga_vars.h`; they are now one published contract each port names |
| `mousemap.c` | battleship | Left in place — generalize if fujitzee ever wants mouse; not required |
| `cellmap.c`, `graphics.c`, `sound.c`, `input.c`, `tiles.h` | battleship | Stayed game-specific, as planned |

The palette/pen names moved into each game's own header; the gamekit takes a
palette array and tile bank as data. Battleship had to build and pass its T1 and
T2 suites against the extracted library before Phase 0 was done — that was the
regression proof, and it is why extraction came first rather than "someday".

### 2. No throwaway ASCII renderer

Battleship built an ANSI-console `graphics.c` in Phase 2 and deleted it in
Phase 3c. That cost real time and was called out in `upstreaming-battleship.md`
as the reason to delay the upstream PR. With the gamekit's custom screen
available on day one, Fujitzee goes straight to the real renderer. Phase 1
still gets stubs — but stubs, not a renderer we plan to delete.

### 3. Audit the port surface before writing code

Done above. Battleship discovered its compat-layer needs incrementally.

### 4. T1 host tests from the start, not retrofitted

Fujitzee has more pure-logic surface than Battleship did (score-card cell
geometry, dice layout, clock formatting, key mapping). Every such module lands
with `test/host/test_*.c` in the same PR, per `docs/testing.md`.

### 5. ~~Fix the PAL/NTSC jiffy assumption~~ — withdrawn, the assumption was right

**This item was wrong and was not implemented.** Recorded here rather than
deleted, because the reasoning is the kind that looks correct twice.

The claim was that `getJiffiesPerSecond()` returning a hard-coded 50 is a bug on
NTSC, and the gamekit should detect the mode via `GfxBase->DisplayFlags` and
return 50 or 60. Checking the actual call site in Phase 0 showed the opposite.
`gamelogic.c` uses the two calls only as a matched pair:

```c
i = (maxJifs - getTime()) / jifsPerSecond;
```

so `getJiffiesPerSecond()` must report **the unit `getTime()` counts in**, not
the display refresh rate. The Atari port needs the PAL/NTSC test because its
`getTime()` reads the OS frame counter; the DOS port hard-codes 60 because it
normalizes `getTime()` to 60. Ours is neither: it is built on dos.library
`DateStamp()`, whose `ds_Tick` is `TICKS_PER_SECOND` = 50 units of *real* time
on every Amiga regardless of video standard. Returning 60 on an NTSC machine
would have made every countdown run 20% fast — introducing exactly the visible
clock error the item set out to prevent.

What Phase 0 did instead: kept 50, published it as `GK_JIFFIES_PER_SECOND`,
wrote the reasoning into `libs/amiga-gamekit/src/gktimer.c`, and added a T1 test
(`test_gkclock.c`) that pins the constant and the conversion so a future reader
cannot re-introduce the "fix".

### 6. Generalize the upstreaming doc

`docs/upstreaming-battleship.md` → `docs/upstreaming-amiga-ports.md`. Its two
blockers (MekkoGX has no Amiga toolchain target; upstream links `fujinet-lib`
where we link `fujinet-nio-lib` + compat shim) are **identical** for Fujitzee —
same maintainer, same build system, same library question. One conversation
upstream should settle both ports.

---

## Layout

```
apps/fujitzee/
  upstream/          ← submodule (read-only pin; never modify)
  amiga/
    include/
      amiga_vars.h   ← force-included via -include
      conio.h  joystick.h  peekpoke.h    ← cc65/CMOC shims
      tiles.h        ← dice pips, icons (art surface)
    src/
      graphics.c  input.c  sound.c  util.c
    test/host/
    Makefile
```

```
libs/amiga-gamekit/          ← shipped in Phase 0
  include/  gfxcore.h  gkinput.h  keytrans.h  joydecode.h
            sndgen.h   gktimer.h  gkrandom.h
  src/      gfxcore.c  keytrans.c joydecode.c sndgen.c
            gktimer.c  gkclock.c  gkrandom.c
  test/host/
  Makefile                   → libamiga_gamekit.a
```

Build wiring follows `make/amiga.mk` (toolchain + nio-lib/compat paths) plus the
`GAMEKIT`/`GAMEKIT_INC`/`GAMEKIT_LIB` variables added in Phase 0. Add
`fujitzee` to `apps/Makefile` and to the Reference Apps table in `CLAUDE.md`.

---

## Phases

Each phase is one PR. Phase 0 lands before any fujitzee code.

### Phase 0 — Extract `libs/amiga-gamekit` ✅ (2026-08-05)

Moved the reusable modules out of `apps/battleship/amiga/`, split the
Battleship-specific bits out of `gfxcore`, added `GAMEKIT_*` to `make/amiga.mk`.
The PAL/NTSC jiffy item was withdrawn — see change 5 above.

Verified: Battleship builds against `libamiga_gamekit.a` and passes
`make test-host` (repo-wide, 8 T1 binaries) and
`make -C apps/battleship/amiga emu-test` (PASS in 10 s), with the lobby screen
rendering identically to before the extraction. The `tilegallery` harness builds
too. No fujitzee files in this PR.

### Phase 1 — Compile and link

Scaffold `apps/fujitzee/amiga/` with stub platform functions, the three
cc65-shim headers, `amiga_vars.h`, and the Makefile. Goal: the upstream
`src/*.c` compile clean under `m68k-amigaos-gcc` and every symbol resolves.

Expected friction: `int` width assumptions in the packed `Game`/`Player`
structs (the server sends a cc65 tightly-packed layout — see upstream's Watcom
`#pragma pack` comment; verify m68k struct layout matches, this is a real
wire-format risk), `char` signedness, and `itoa`/`utoa` style non-C99 helpers.

### Phase 2 — Playable end to end

Real `graphics.c` on the gamekit screen, keyboard input, timer/random, sound
stubs. Goal: join the lobby, sit at a table, roll dice, score a category, and
finish a game against other players.

The scorecard is the layout problem: 13+ categories × up to 12 players against a
40-column-equivalent grid. Take a position early on how many players the Amiga
build displays and whether it uses a wider grid than the 40×26 Atari layout —
we have 320×200 and an 8×8 font, so 40×25 is the natural cell grid, but nothing
forces it.

### Phase 3 — Full platform layer

- **3a — Sound.** 13 effects via gamekit `sndgen` + `audio.device`. Remember the
  FS-UAE `AUDxVOL` workaround (`pokeVolume()`, strategic-plan Lessons Learned
  2026-07-08).
- **3b — Joystick.** Gamekit `joydecode`; smaller than Battleship's since
  fujitzee's `readJoystick()` takes no port argument.
- **3c — Art pass.** Dice faces, the Fujitzee logo, connection/clock icons,
  player highlight colors. Reuse Battleship's `tilegallery` harness pattern for
  previewing.

### Phase 4 — ADF boot test

Build the ADF via `/emu-build-and-boot`, boot in FS-UAE against a live
`fujinet-nio`, play a full multi-player game, then test on real Amiga 500 +
PiStorm. Verify prefs AppKey round-trips across a **soft reset** (not a
relaunch — see the `fn_transport_close()` leak note in
`contracts/amiga-transport-api.md`).

---

## Verification checklist

- [x] Phase 0 — Battleship builds and passes T1 + T2 against `libamiga_gamekit.a`
- [x] Phase 0 — ~~`getJiffiesPerSecond()` returns 60 on NTSC, 50 on PAL~~ withdrawn: 50 is correct for a DateStamp-based clock (see change 5), pinned by `test_gkclock.c`
- [ ] Phase 1 — fujitzee binary links; struct layout matches the server wire format
- [ ] Phase 2 — join lobby, sit at table, roll, score, finish a game
- [ ] Phase 2 — scorecard readable with the maximum supported player count
- [ ] Phase 3a — all 13 sound effects audible in FS-UAE
- [ ] Phase 3b — joystick drives the dice/score cursors
- [ ] Phase 3c — dice, logo, and icons rendered from tile art
- [ ] Phase 4 — boots from ADF, full game against live `fujinet-nio`
- [ ] Phase 4 — prefs AppKey survives a soft reset
- [ ] Phase 4 — tested on real Amiga 500 + PiStorm
