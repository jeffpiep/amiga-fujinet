# Implementation Plan: Track 1C — Fujitzee Amiga Port

**Depends on:** Track 1A (`libfn_compat_amiga.a`) ✅; Track 1B (Battleship) for the
platform-layer code being extracted in Phase 0
**Blocks:** nothing
**Status:** All phases complete (2026-08-06) — Phase 3a landed the sound, which
was the last stub. The port is now real end to end: renderer, keyboard,
joystick, art and 13 sound effects, with the `audio.device` machinery
extracted to the gamekit so battleship shares it. **No stubs remain.**
What is left is not implementation but confirmation — the two manual
checkboxes at the end of this document (a full 13-round game, and hearing
the effects on real speakers).

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

### Three upstream-integration wrinkles

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

3. **Struct packing is load-bearing** (found in Phase 1 — the risk the phase
   flagged was real). `stateclient.c` `network_read()`s the server response
   straight into `clientState`, so upstream's `Game` struct *is* the wire
   format, in cc65's tightly-packed layout. `m68k-amigaos-gcc` aligns
   `int16_t` to 2 bytes and inserts one pad byte before `Game.players[]`:
   `offsetof` 96 instead of 95, `sizeof` 600 instead of 599 — every player
   record read one byte out of phase. Upstream already fixes this for Open
   Watcom (same fault, same cause) with `#pragma pack(push,1)` under
   `#ifdef __WATCOMC__`, and GCC honours that pragma.

   **This is the one place the port patches upstream.**
   [FujiNetWIFI/fujinet-fujitzee#9](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/9)
   widens that guard to
   `#if defined(__WATCOMC__) || defined(FUJITZEE_PACK_STRUCTS)` — two lines,
   no behaviour change for any existing platform — and the app Makefile
   passes `-DFUJITZEE_PACK_STRUCTS`. Until it merged (2026-08-06),
   `apps/fujitzee/upstream` was **Riding a PR**
   (`feature/pack-structs-opt-in` on our fork) rather than read-only tracking;
   see `docs/syncing-upstream-submodules.md` for exiting that state.

   The self-contained alternative — briefly defining `__WATCOMC__` around the
   `misc.h` include in `amiga_vars.h` — works and was what Phase 1 first
   shipped, but it silently inherits any future `__WATCOMC__` branch upstream
   adds to a header. The two-line opt-in says what we actually mean.

   Verified on both compilers (`offsetof(Game, players)` 95, `sizeof(Game)`
   599) and pinned by `apps/fujitzee/amiga/test/host/test_wireformat.c`, which
   builds with the same flag and fails if it goes missing.

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
| `cellmap.c`, `graphics.c`, `sound.c`, `input.c`, `tiles.h` | battleship | Stayed game-specific, as planned. Note that `sound.c` and `input.c` still hold reusable AmigaOS machinery (`audio.device` playback, the IDCMP pump, raw joystick register reads) that Phase 3a/3b will want — see the extraction note there |

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
      fjlayout.h     ← geometry, tile ids, dice math (host-tested)
      tiles.h        ← dice pips, lattice, icons (art surface)
      pens.h  gfxsetup.h
    src/
      graphics.c  gfxsetup.c  fjlayout.c
      input.c  sound.c  util.c
      preview_main.c ← board preview harness (make preview-adf)
    test/host/
    Makefile
```

```
libs/amiga-gamekit/          ← Phase 0, plus the Phase 2 additions
  include/  gfxcore.h  tilepat.h  gkinput.h  keytrans.h  gkkeyq.h
            joydecode.h  sndgen.h  gktimer.h  gkrandom.h
  src/      gfxcore.c  keytrans.c gkkeyq.c  joydecode.c sndgen.c
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

### Phase 1 — Compile and link ✅ (2026-08-05)

Scaffolded `apps/fujitzee/amiga/` with stub platform functions, the three
cc65-shim headers, `amiga_vars.h`, and the Makefile. The upstream `src/*.c`
compile under `m68k-amigaos-gcc` and every symbol resolves — a 55 KB
`fujitzee` binary. Wired into `apps/Makefile` and the repo-wide
`make test-host`.

The friction was mostly where expected, with one surprise:

- **Struct packing** — real, and the biggest finding of the phase. See
  wrinkle 3 above.
- **`KEYMAP_H` is a shared guard.** Every upstream platform `vars.h` uses the
  same `#ifndef KEYMAP_H`; whichever one the toolchain macros select claims
  it. `amiga_vars.h` claims it too. (Defensive now that packing is a
  `-D` flag; it was load-bearing under the `__WATCOMC__` shim, which let
  `msdos/vars.h` in to override our `WIDTH`/`HEIGHT`/`KEY_*`.)
- **`KEY_ESCAPE_ALT` cannot be `'q'`.** Battleship's key map is not
  transferable wholesale: fujitzee's `screens.c` already handles `'q'` as
  Quit, and every `KEY_*_ALT` macro is a `case` label in the same switch, so
  a collision is a compile error. Placeholders follow the Atari port (1/2/3).
- **`fujinet-fuji.h` needs `amiga_compat.h` first.** Fujitzee's `main.c` and
  `misc.c` include it directly (battleship's sources do not), and it declares
  `fuji_create_new(NewDisk *)` with `NewDisk` defined only under
  per-platform `#ifdef`s. `amiga_vars.h` pulls the compat bridge type in.
- **`itoa`** — declared in `amiga_vars.h`, defined in `src/util.c`, same as
  battleship. No `utoa`, and no `char`-signedness problems surfaced.

Eight warnings remain, all in upstream sources (`void main`, an unused
static, `-Wparentheses` on chained assignments); nothing in our platform
layer warns. Left alone rather than patched — the pin stays read-only.

### Phase 2 — Playable end to end ✅ (2026-08-06)

Real `graphics.c` on the gamekit screen (board lattice, dice, both cursors,
the active-player brackets, text), keyboard input, timer/random from Phase 1.
Sound stays stubbed for 3a, joystick for 3b — the game is keyboard-playable
without either.

**The scorecard question is settled: 40×25 cells, six player columns, the DOS
layout.** The natural reading was to port the Atari renderer, but the Atari is
40×**26** and spends that extra row: its bottom panel starts at `HEIGHT-5`,
which on 25 rows lands on the score box's own bottom border. Upstream's DOS
port is 40×25 for exactly the same reason we are (`msdos/graphics.c`), and had
already re-derived every offset the missing row shifts — including moving
`SCORES_X` from 11 to 10 so the seventh divider lands on column 39 instead of
falling off the grid. We follow that one, so `FJ_DIV_X(6) == FJ_WIDTH-1` and
six players fit exactly, which is what every other 40-column port shows. Tables
may seat up to `PLAYER_MAX`; upstream itself only draws the first six
(`gamelogic.c` skips `i>5`).

Decisions and findings worth keeping:

- **Arrow keys cannot be WASD here** — the phase's real surprise, and the
  reason the gamekit changed. Battleship takes cursor keys as `w/a/s/d`
  because nothing else wants those letters. Fujitzee's lobby menu answers to
  `s` (sound), `r`, `c`, `h`, `q`, and its name-entry screen accepts every
  letter as text: arrow-down would have toggled the sound, and typing a name
  would have moved the cursor. `kt_decode_ex()` now takes a `KT_CURSOR_*`
  mode, with `GK_KEY_CUR_*` (0x1C-0x1F) as the second spelling — unreachable
  from the cooked path, which passes only 32..126 plus `\r`, `\b`, ESC.
  `kt_decode()` is unchanged for battleship.
- **The IDCMP key pump moved to the gamekit** (`gkkeyq.c`), ahead of the
  Phase 3 extraction note below. It was 60 lines of battleship's `input.c`
  that fujitzee needed verbatim, it depends on nothing but `gfx_window` and
  the decoder, and duplicating it would have forked the first thing both
  ports touch. Mouse-button tracking came along because it arrives on the
  same port and a key drain would otherwise swallow it (`gk_mouse_button()`);
  battleship's mouse aiming reads it from there now.
- **The tile composers moved too** (`tilepat.h`): `TILE_PAT`/`TILE_MC` are
  how both ports author art, not battleship's private macros. Note
  `TILE_PAT_V`, added for fujitzee's dice: the preprocessor counts a macro's
  arguments *before* expanding them, so passing the eight rows as one macro
  needs a variadic forwarder.
- **Dice are 16 tiles × 3 face colors.** A die is 3×3 cells and the pip grid
  *is* the cell grid, so each cell is a frame position with or without a pip
  (top-centre and bottom-centre never have one — hence 16, not 18). Colour
  lives in the tile, so kept and highlighted dice are separate sets rather
  than a recolour.
- **`setHighlight` brackets the column** instead of tinting it. The Atari
  drives a player-missile overlay and the DOS port rewrites every pixel of
  the column's background; swapping the two dividers for coloured tiles says
  the same thing, survives score redraws inside the column, and costs three
  tiles.
- **The screenshots lie about colour.** The headless FS-UAE capture renders
  each Amiga pixel as an RGB triad, so anything that is not pure white or
  black comes out as fine vertical stripes — the dark-blue table background
  looked like corduroy until it was checked against a black one. It is the
  capture, not the bitmap. `PEN_BG` is black anyway now, which keeps T2
  screenshots reviewable.
- **Not verified headlessly:** an actual multi-player game. The board only
  renders once a game is under way, and the T2 harness cannot type. That is
  what `make -C apps/fujitzee/amiga preview-adf` exists for — it boots a full
  scorecard with fake values (`src/preview_main.c`), which is how the layout
  above was checked. Playing a real game to the end is a manual step, and it
  is the one open item on the Phase 2 checklist.

### Phase 3 — Full platform layer

> **Phase 0 extracted less than 3a/3b need — plan for a second extraction
> round here.** The gamekit got the *pure* halves of sound and input:
> `sndgen` (waveform bakers) and `joydecode` (counter word → direction bits).
> The AmigaOS halves stayed in `apps/battleship/amiga/` because they are
> genuinely entangled with the game — the `audio.device` open/allocate/play
> machinery and the FS-UAE `AUDxVOL` workaround live in its `sound.c`, and the
> IDCMP event pump plus the raw `JOYxDAT`/`CIAA` register reads live in its
> `input.c`. Neither is fujitzee-specific, so the choice at 3a/3b is: extract
> them into the gamekit first (paying the Battleship regression cost a second
> time, as in Phase 0), or duplicate ~150 lines and accept the fork. Decide
> deliberately rather than by default — the cheap-second-port thesis this
> whole track is testing is what is being measured.
>
> **Phase 2 answered half of this: extract.** The IDCMP keyboard pump is now
> `libs/amiga-gamekit/src/gkkeyq.c` and both ports call it; battleship's
> `input.c` lost 60 lines and kept only its joystick and mouse-aiming code.
> The regression cost was one rebuild plus T1 and T2 — cheap, because the
> pump depends on `gfx_window` and nothing else. What is left for 3a/3b is
> the `audio.device` machinery and the raw joystick register reads. Expect
> the same answer for the joystick (fujitzee's `readJoystick()` takes no port
> argument, so the *policy* differs but the register read does not) and
> weigh sound on its own: battleship's `sound.c` mixes effect definitions
> with playback, and only the playback half is shareable.

- **3a — Sound.** ✅ Done 2026-08-06. 13 effects via gamekit `sndgen` +
  `audio.device`.

  **Extraction, as the note above asked for.** The `audio.device` machinery
  is now `libs/amiga-gamekit/src/gksound.c` — device open, channel
  allocation, the single Chip RAM block, `gk_snd_carve()`, play and stop.
  What stayed in each game's `sound.c` is what the note predicted was the
  only genuinely game-specific part: the effect table and the mute policy.
  Battleship's `sound.c` went from 221 lines to 105 and got no worse.

  Pitches are not invented. Upstream's DOS port documents every effect as
  `Hz = 63920 / (2 * (n + 1))` back-derived from the Atari POKEY divisors,
  so `apps/fujitzee/amiga/src/sound.c` bakes that sequence. The two long
  fanfares are baked at half the DOS duration — at full length those two
  alone cost ~38 KB of Chip RAM, more than the screen.

  **Two defects the audio capture found, neither visible any other way:**

  1. **A DC offset after every effect that did not decay to zero.** Paula
     holds the last sample value on the channel once playback completes, so
     an effect ending at amplitude 20 leaves that sitting on the output
     until the next sound — silent itself, but it thumps on the next
     transition, and it made three fujitzee effects and two battleship ones
     run together in the capture. Every effect now ends at `vol_end 0`, and
     `sndgen.h` states the rule. This was pre-existing in battleship
     (`soundAttack`, `soundInvalid`), fixed here.
  2. **FS-UAE ignores `ioa_Period`, not just `AUDxVOL`.** The known
     workaround re-pokes the volume register audio.device is supposed to
     set; the period register turns out to be dropped the same way, so
     every effect played at whatever rate the channel was last left at —
     measured at ~4x too fast, two octaves high and a quarter as long,
     *identically* whether `ioa_Period` said 447 or 1788, which is what
     proves the device write is being dropped rather than mis-set.
     `pokeVolume()` is now `pokePerVol()` and writes both. Same no-op on
     real hardware, same reasoning as 2026-07-08.

  Both are gamekit-level, so battleship gets both fixes; its T1 and T2
  still pass.
- **3b — Joystick.** ✅ Done 2026-08-06. `readJoystick()` is real: game port 2
  via the new `libs/amiga-gamekit/src/gkjoy.c`.

  **Extraction, as the note above predicted** — and it cost almost nothing,
  because what was left to share turned out to be two register reads. The
  split follows the same seam as every other gamekit module: the *pure* half,
  `joyDecodePort2(joy1dat, ciaapra)`, joins `joydecode.c` and is T1-tested;
  the impure half is `gk_joy_read_port2()`, three lines that peek JOY1DAT and
  CIAAPRA. Battleship's `input.c` lost its two `#define`s and kept its mouse
  aiming, which is the genuinely game-specific part.

  The new pure function exists so the fire-line polarity is pinned by a test:
  CIAAPRA bit 7 is active *low*, and getting it backwards yields a stick that
  fires continuously until pressed — a bug that boots fine and only shows up
  in a game. `test_joydecode.c` now checks both polarities, and that bit 6
  (port 1's button, i.e. the mouse's) does not register.

  **"Either joystick" resolves to port 2 only.** Upstream's Atari
  `readJoystick()` polls both sticks and returns whichever is live, which is
  why it takes no port argument. That is not portable here: Amiga port 1
  shares its counter with the mouse, so polling it would turn every mouse
  twitch into a phantom direction. Port 2 is where a stick is actually
  plugged in, so the answer is one port, documented at the call site.

  **T2 verification is now possible headlessly** — `emu/drive.sh` gained
  `JOYSTICK=1`, which drops FS-UAE's built-in `keyboard` controller into the
  game port so scripted arrow keysyms move the *stick* (Right Ctrl fires)
  instead of reaching the emulated keyboard. Driving a real game against the
  bot table: `Up` moved the cursor from the dice row into the scorecard,
  `Left` brought it back to the roll button, and Right Ctrl rolled the dice.
  The control run matters as much as the result: the same key script against
  the previous stub build produced four **pixel-identical** screenshots,
  which is what proves the arrows travelled the joystick path and not the
  keyboard.
- **3c — Art pass.** ✅ Done 2026-08-06. All of it is data in
  `apps/fujitzee/amiga/include/tiles.h`; no engine change was needed.

  - **Dice.** The 16 cell shapes moved from 2-colour `TILE_PAT` row masks to
    multicolor `TILE_MC` grids: a rounded body with a two-pixel corner bite,
    lit along its top-left edge and shaded along its bottom-right. There is
    no dark outline — the background is black, so the bevel is what draws
    the silhouette. `PEN_TEXT` and `PEN_SHADE` serve as that light/dark pair
    for all three face colours, because the palette has no room for a pair
    per face; `PEN_DIE` dropped to 0xCCC so the white highlight has
    something to say against it.
  - **Wordmark.** `drawFujitzee()` draws five tiles — a snow-capped mountain
    in the board's own teal, then a heavier-than-topaz T Z E E — where the
    Atari spells `/|\TZEE` from its custom charset. Five cells is exactly
    what the score-name box is wide, which is why the mountain carries the
    `/|\` rather than getting a cell of its own.
  - **Icons.** `fj_icon_tile()` maps the ICON_* characters we have art for:
    a solid wedge for the turn marker and a hollow chevron for the score
    cursor (they point the same way because they mean the same thing — this
    row, this player), the chevron's tip alone for the blink phase, and a
    diamond for a committed score. The lobby's remaining markers are still
    font glyphs.
  - **Per-player colours** (`cycleNextColor`) stay stubbed, as on Atari and
    DOS: the 16 pens are spoken for, and two of them are now the dice bevel.

  Not reused from Battleship: its `tilegallery` previews tiles in isolation,
  where the thing worth looking at here is the composition, which
  `preview-adf` already draws. Candidate mountains were compared by
  rendering the 8x8 grids offline rather than by booting the emulator once
  per attempt.

- **3c.1 — Scores wider than their column.** Found during the art pass. A
  score column is three cells and upstream right-aligns into it with no
  clamp (`gamelogic.c`: `drawTextAlt(validX+3-strlen(...))`), so a
  four-digit number starts on the divider and eats the board lattice. The
  endianness bug (3d) made this visible with every opponent score; fixing it
  removed the symptom but not the exposure — the end-of-game grand total is
  a genuine fourth digit, and a desynced server can send anything.

  The renderer now catches that geometrically (`fj_score_spill()`, T1-tested)
  and squeezes the digits into the space that is actually free: 24 px of
  column plus three pixels either side, borrowed from the blank halves of
  the two 2-px dividers. Four digits at a 7 px advance fit that with topaz-8
  ink to spare, where squeezing the same four into 24 px makes them touch.
  The one new primitive, `gfx_text_tight()` in the gamekit, is the only call
  in the engine that escapes the cell grid.

  Worth knowing: `scoreY[15]`, the grand total, is row 21 — *below*
  `FJ_BOARD_BOTTOM`, where a wide number has no lattice to damage. That row
  is deliberately excluded, so it still draws exactly as upstream asks.

- **3d — Get big-endian scores.** ✅ Done 2026-08-07 — `QUERY_SUFFIX "&be=1"`
  in `include/amiga_vars.h`, which asks the *server* to emit big-endian 16-bit
  values. Found by playing a real game
  against the bot table (see below). `Player.scores` is `int16_t[16]` and the
  server sends it little-endian; m68k reads it big-endian, so every opponent
  score displays as `value << 8` (a real 11 renders as 2816, and wide values
  overflow their 4-column cell). Phase 1 fixed *packing* and pinned it with
  `test_wireformat.c`, but packing and endianness are separate bugs and only
  the first was caught — `-1` (0xffff, "unscored") and the locally computed
  candidate scores are both byte-order invariant, which is why the preview
  harness and every earlier screenshot looked correct.

  The affected surface is exactly `players[i].scores[]` — every other field in
  `Game`, `Table` and `Tables` is 8-bit.

  The first fix was client-side, shaped like the packing fix: an upstream
  opt-in macro (`-DFUJITZEE_BIG_ENDIAN`) that swapped the array after the read
  ([#10](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/10), merged
  2026-08-06, pinned by a `test_endian.c`). Eric Carr reverted it the next day
  ([#11](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/11),
  [#12](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/12)) in favour of
  the server-side flag the CoCo port was already using: append `&be=1` to the
  API query and the server emits big-endian. That is the better layer, and the
  reason is visible in the reverted patch — the payload is memcpy'd whole into
  a union, so a client-side swap has to be invoked only at the call sites where
  the union is known to hold a `Game`, never inside `apiCall()` itself. The
  server-side flag has no such condition to get wrong. The port now defines
  `QUERY_SUFFIX "&be=1"`, carries no swap code, and `apps/fujitzee/upstream`
  tracks upstream `main` again.

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
- [x] Phase 1 — fujitzee binary links; struct layout matches the server wire
      format (packed via upstream's Watcom pragma path; `offsetof(Game,
      players)` 95 / `sizeof(Game)` 599 confirmed on m68k and pinned by
      `test_wireformat.c`)
- [x] Phase 2 — renderer boots against a live `fujinet-nio`: welcome screen
      draws, the game reaches its first server call (`make -C
      apps/fujitzee/amiga emu-test`, PASS in 12 s), and battleship still
      passes T1 + T2 after the gamekit extraction
- [x] Phase 2 — scorecard readable with the maximum supported player count:
      six columns, verified on the `boardpreview` ADF with all six filled
- [x] Phase 2 — join lobby, sit at table, game starts and plays: verified
      2026-08-06 headlessly via `emu/drive.sh` against the live server. Both
      premises that made this "manual" were wrong — the harness *can* type
      (`emu/scripts/emukey.py`, XTEST), and the server hosts an **"ai room —
      4 bots"** table, so no second human is needed. The run reached round 2
      of 13 with bots taking turns and surfaced the 3d endianness bug
- [x] Phase 2 — arrow keys reach the game. The first attempt showed no
      movement, but the cause was FS-UAE mapping the host arrows to joystick
      port 1 when no stick is attached, not the `KT_CURSOR_CTRL` decode;
      `drive.sh` now sets `joystick_port_1 = nothing`. Confirmed indirectly:
      after `Up` + `Return` the score landed in a score row, which upstream
      only reaches at `cursorPos >= 10` — i.e. only if `Up` moved the cursor
      off the dice. A frame showing the cursor mid-move was not captured,
      because the persistent server table keeps rejoining a game in progress
- [ ] Phase 2 — finish a full 13-round game to the end screen
- [x] Phase 3a — all 13 sound effects audible in FS-UAE. Twelve of them are
      audible by definition (`soundStop()` is the silence, and every other
      effect exercises it — `gk_snd_play()` stops the channel first), and
      the twelve were captured and counted rather than taken on trust:
      `boardpreview`'s new SPACE sweep plays them in order 2.5 s apart, and
      `AUDIO_WAV=… bash emu/drive.sh` + `emu/checkaudio.py --expect 12`
      finds exactly twelve bursts at the right times, peaking around 0.27.
      That capture is also what found the two defects below
- [x] Phase 3b — joystick drives the dice/score cursors. Driven headlessly
      with `JOYSTICK=1 bash emu/drive.sh` against the bot table: up moved the
      cursor into the scorecard, left returned it to the roll button, Right
      Ctrl (fire) rolled. The same script on the pre-change stub build gave
      four pixel-identical shots, which is the control that rules out the
      arrows having reached the keyboard instead
- [x] Phase 3c — dice, logo, and icons rendered from tile art. Bevelled
      rounded dice (multicolor tiles, one shared light/dark pair across all
      three face colours), a five-cell `/|\ TZEE` wordmark on the game's own
      score row, and tiles for the turn marker and score cursor. Checked in
      the `boardpreview` harness and in a live game against the bot table
- [ ] Phase 4 — boots from ADF, full game against live `fujinet-nio`
- [ ] Phase 4 — prefs AppKey survives a soft reset
- [ ] Phase 4 — tested on real Amiga 500 + PiStorm
