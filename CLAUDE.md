# Amiga FujiNet — Claude Code Instructions

## Project Overview

Prototype FujiNet for Amiga using a Linux box as the FujiNet server connected
via RS-232 serial to an Amiga computer.

- `fujinet-nio/` — FujiNet server (runs on Linux/ESP32) — git submodule
- `fujinet-nio-lib/` — Client library (runs on Amiga) — git submodule
- `fujinet-nio-driver/` — Native OS drivers (Amiga `fujinet-disk.device`,
  MS-DOS `FUJINET.SYS`) — git submodule, Mark's. Must be a **sibling** of
  `fujinet-nio-lib/`; its Amiga build consumes that checkout directly
- `contracts/` — Protocol & hardware specs — source of truth for cross-submodule work
- `apps/` — Amiga programs (amiga-gcc / m68k-amigaos)
- `libs/` — Amiga static libraries shared by the apps:
  - `fujinet-compat-amiga/` → `libfn_compat_amiga.a` — FujiNet API shim upstream games expect
  - `amiga-gamekit/` → `libamiga_gamekit.a` — game platform layer (custom screen +
    tile/sprite engine, tile-art composer macros, the IDCMP keyboard queue,
    keyboard and joystick decode, the game-port joystick read, waveform
    bakers, `audio.device` playback, jiffy clock, PRNG). Takes
    palette/art/geometry as data; owns no game-specific names.

## Git Workflow

### Repo roles

| Repo | Your fork (origin) | Upstream |
|------|--------------------|----------|
| `amiga-fujinet` (this repo) | jeffpiep/amiga-fujinet | — (not contributed upstream) |
| `fujinet-nio/` | jeffpiep/fujinet-nio | markjfisher/fujinet-nio |
| `fujinet-nio-lib/` | jeffpiep/fujinet-nio-lib | markjfisher/fujinet-nio-lib |
| `fujinet-nio-driver/` | — (read-only pin) | markjfisher/fujinet-nio-driver |
| `battleship/` | jeffpiep/battleship | FujiNetWIFI/battleship |
| `apps/fujitzee/upstream` | jeffpiep/fujinet-fujitzee | FujiNetWIFI/fujinet-fujitzee |
| `apps/pacmantests/amiga-pac-man` | — (read-only pin) | tschak909/amiga-pac-man |

Routine submodule syncing (fast-forwards, PR-branch pins, squash-merge
recovery, drift checks) follows **`docs/syncing-upstream-submodules.md`**.

Each submodule has two remotes:
- `origin` — your fork (push feature branches here)
- `upstream` — the authoritative repo (fetch/rebase from here, never push)

```bash
# One-time remote setup per submodule
git -C fujinet-nio remote add upstream https://github.com/markjfisher/fujinet-nio.git
git -C fujinet-nio-lib remote add upstream https://github.com/markjfisher/fujinet-nio-lib.git
# (battleship, when added)
# git -C battleship remote add upstream https://github.com/FujiNetWIFI/battleship.git

# apps/fujitzee/upstream started as a read-only pin, so its remotes were set
# up the other way round; it was renamed when the port needed an upstream PR:
#   git -C apps/fujitzee/upstream remote rename origin upstream
#   git -C apps/fujitzee/upstream remote add origin \
#       https://github.com/jeffpiep/fujinet-fujitzee.git
```

### Branch naming

```
feature/<short-description>
```

Feature branches are short-lived and focused. `main`/`master` is never committed
to directly — all changes arrive via squash-merge PR.

**Parent repo integration branch.** `amiga-fujinet` has a long-lived `dev`
branch. Feature branches are cut from `dev` and squash-merge back into `dev`;
`dev` merges into `main` at release points. `main` therefore only ever moves
via a `dev` → `main` PR. Submodules have no `dev` — their feature branches are
still cut from and contributed back to `master`/`main` upstream.

**One exception**, parent repo only. These may be committed straight to `dev`,
provided they touch no code, no build output, and no submodule pointer:

- **Tooling/editor config** — `.claude/settings.json`, `.gitignore` entries.
- **Simple docs-only changes** — typo and link fixes, status-table corrections,
  Lessons Learned entries, rehoming a fact into its owning evergreen doc.

Anything a reviewer could have an opinion about still gets a PR: new or
substantially rewritten docs, new contracts, and any change to this file's
rules. A status-table update that *belongs* to a track/phase PR still ships
inside that PR (see **Documentation** below) — this exception is for correcting
docs after the fact, not for splitting work out of the PR that earned it.

This exception never applies inside a submodule — those always go through the
upstream-PR flow.

### Commit message style

Subject in imperative present tense ("Add X", not "Added X"), ≤72 chars.
Body optional, separated by a blank line.

```
Add Amiga serial transport to fujinet-nio-lib
```

### Daily submodule development

```bash
# 1. Sync master to upstream before starting new work
git -C fujinet-nio-lib checkout master
git -C fujinet-nio-lib fetch upstream
git -C fujinet-nio-lib merge --ff-only upstream/master

# 2. Create a feature branch from the clean master
git -C fujinet-nio-lib checkout -b feature/my-change

# 3. Edit and commit (keep commits clean — they may become an upstream PR)
git -C fujinet-nio-lib add <files>
git -C fujinet-nio-lib commit -m "Add X to Y"

# 4. Keep branch current while working (rebase, never merge)
git -C fujinet-nio-lib fetch upstream
git -C fujinet-nio-lib rebase upstream/master

# 5. Update parent repo pointer
git add fujinet-nio-lib
git commit -m "bump nio-lib: my-change"
```

### Contributing a submodule feature upstream

When a submodule feature is ready to share:

```bash
# Push the feature branch to your fork
git -C fujinet-nio-lib push -u origin feature/my-change
# Open a PR on GitHub: jeffpiep/fujinet-nio-lib feature/my-change → markjfisher/fujinet-nio-lib master
# Upstream squash-merges it.

# After merge, clean up
git -C fujinet-nio-lib checkout master
git -C fujinet-nio-lib fetch upstream
git -C fujinet-nio-lib merge --ff-only upstream/master
git -C fujinet-nio-lib branch -D feature/my-change
```

### Parent repo (amiga-fujinet) workflow

Same feature-branch model; no upstream to contribute to. Integration branch is
`dev`, not `main`.

```bash
# Start work — always from an up-to-date dev
git switch dev && git pull
git checkout -b feature/my-topic

# Commit (bump submodule pointers as needed)
git add contracts/foo.md apps/bar/main.c fujinet-nio-lib
git commit -m "Add foo contract and bar app"

# Merge to dev via squash PR (gh pr create --base dev), then delete branch
git switch dev && git pull
git branch -D feature/my-topic
```

Releasing: open a `dev` → `main` PR and merge it (a real merge, not a squash —
`main` should keep the individual feature commits `dev` accumulated).

### Worktrees

Worktrees isolate parallel workstreams (e.g., two background Claude Code jobs
running at the same time). Each worktree lives under `.claude/worktrees/<name>/`
on a branch named `worktree-<name>`.

**Background Claude Code agents** call `EnterWorktree` before their first file
edit; edits in the shared checkout are rejected until that isolation step runs.

```bash
# See all active worktrees
git worktree list

# Create a worktree manually (rarely needed — the harness does this automatically)
git worktree add .claude/worktrees/my-track -b worktree-my-track

# Remove when done (harness prompts at session exit; or remove manually)
git worktree remove .claude/worktrees/my-track
git branch -D worktree-my-track
```

Worktree branches follow the same rules as feature branches: commits land there
first, then get merged/squash-merged to `dev` via PR.

### Never
- Edit submodule files without first creating a branch inside that submodule
- Commit the parent repo without first committing inside any modified submodule
- Commit directly to `main` or `master` — always use a feature branch + PR
  (narrow exception for trivial parent-repo tooling config, which goes to `dev`
  — see "Branch naming")
- Open a parent-repo feature PR against `main` — the base is `dev`; only a
  release PR targets `main`
- Merge `master`/`main` into a **submodule** feature branch — rebase instead (upstream requires linear history). For the parent repo, rebase is preferred but not required.
- Use `git add .` or `git add -A` — always stage specific files
- Edit files in the shared checkout from a background Claude Code job — use `EnterWorktree` first

## Reference Apps

| App | Path | What it tests |
|-----|------|---------------|
| `fn_test` | `apps/fn_test/` | `fn_init()` + `fn_is_ready()` — baseline serial transport check |
| `http_get` | `apps/http_get/` | HTTP and HTTPS GET — curl-like tool, auto-detects `https://` scheme |
| `battleship` | `apps/battleship/amiga/` | Full FujiNet game — lobby + gameplay over FujiBus. Sources in `apps/battleship/upstream/` submodule; links `libs/fujinet-compat-amiga`. |
| `compat_test` | `apps/compat_test/` | Compat-layer smoke test on the emulator — exercises `libs/fujinet-compat-amiga` end to end. |
| `fujitzee` | `apps/fujitzee/amiga/` | Second FujiNet game port (Track 1C). Sources in `apps/fujitzee/upstream/`; links `libs/amiga-gamekit` + `libs/fujinet-compat-amiga`. Complete — renderer and keyboard (Phase 2), joystick (Phase 3b), art (Phase 3c) and sound (Phase 3a) are all real. No stubs. |
| `pacmantests` | `apps/pacmantests/` | Exploratory (non-shipping) bitplane-graphics harnesses for the battleship Phase 3 renderer. Includes the `amiga-pac-man` submodule (tschak909). |

`apps/battleship/` establishes the pattern for future game ports:
`apps/<game>/upstream/` (read-only pinned submodule) + `apps/<game>/amiga/`
(Makefile + platform layer), linking `libs/fujinet-compat-amiga`.

`apps/fujitzee/` is the second port following that pattern (Track 1C) — start
from `docs/plan-track1c-fujitzee.md`. All phases are done: gamekit
extraction, scaffold + link, the real renderer + keyboard, the joystick, the
art, and the sound. No stubs remain.

All of the port's art is data in `apps/fujitzee/amiga/include/tiles.h`:
palette, board lattice, dice, wordmark, icons. Change the arrays, keep the
names, and no engine code moves.

Its scorecard only renders inside a live multiplayer game, so the layout has
its own preview harness: `make -C apps/fujitzee/amiga preview-adf` builds a
bootable ADF that draws a full board from fake values. Use it to check layout
or art changes without a server — the equivalent of battleship's
`make gallery-adf` one level up. Pressing SPACE there also plays all twelve
sound effects in order, which is the only way to reach the six that need game
states a script cannot produce (`soundFujitzee` wants five of a kind).

For the dice specifically there is a faster loop that skips the toolchain
entirely. `make -C apps/fujitzee/amiga dice-preview` renders every face in
every face colour straight out of `tiles.h` to a PNG, and `dice-stamps` lays
candidate pip designs (`tools/pipstamps.txt`, rejected ones kept) over the
real frame cells so trying one costs an 8x8 text block rather than an edit to
seven macros. Both need Pillow, not amiga-gcc. Narrow candidates down there,
then confirm the winner with `preview-adf` — pixel geometry the renderer gets
exactly right, but the Amiga's non-square pixels and the real display's
contrast are things only a boot settles.

Three things about fujitzee differ from battleship and are easy to trip over:

- Its upstream `platform-specific/vars.h` has **no `PLATFORM_VARS` hook**, so
  `apps/fujitzee/amiga/include/amiga_vars.h` is force-included with
  `-include` instead. That header also claims the shared `KEYMAP_H` guard and
  declares the cc65-isms upstream expects.
- The server payload is memcpy'd straight into upstream's `Game` struct, so
  **struct packing is load-bearing**: the Makefile passes
  `-DFUJITZEE_PACK_STRUCTS` (upstream opt-in, fujinet-fujitzee#9) and
  `test/host/test_wireformat.c` pins the resulting offsets.
- **Endianness is asked for, not fixed up.** `Player.scores` is the only
  multi-byte field on the wire, and the server defaults to little-endian, so
  m68k reads every opponent score byte-swapped. The fix is one line in
  `include/amiga_vars.h` — `#define QUERY_SUFFIX "&be=1"`, which makes the
  server emit big-endian. Don't reintroduce a client-side swap: upstream
  merged one (fujinet-fujitzee#10) and reverted it (#12) in favour of this.
- **Its cursor keys are not W/A/S/D.** Battleship maps them to those letters;
  fujitzee cannot, because its lobby menu is on `s`/`r`/`c`/`h`/`q` and its
  name-entry screen takes every letter as text. `src/input.c` selects
  `KT_CURSOR_CTRL`, so arrows arrive as `GK_KEY_CUR_*` control codes
  (`libs/amiga-gamekit/include/gkinput.h`).

Copy `fn_test` or `http_get`'s `Makefile` as a starting point for new apps.
All Amiga Makefiles get the toolchain (`CC`, `AR`, canonical `CFLAGS`) and the
nio-lib / compat-layer / gamekit paths from **`make/amiga.mk`** — include it
first, append per-app flags with `CFLAGS +=`, never redefine the toolchain
locally. Listing `$(COMPAT_LIB)` or `$(GAMEKIT_LIB)` as a prerequisite builds
that library automatically; `$(NIO_ALIB)` is not auto-built (submodule
boundary).

These apps are on-target (T2) smoke tests. For fast host-side unit tests of
pure-logic code, see the **Testing** section below and `docs/testing.md`.

## Build Commands

```bash
# One-time: initialize submodules (--recursive required for esp_littlefs sub-submodules)
git submodule update --init --recursive --force

# Build fujinet-nio for Linux RS-232 (use build.sh — handles venv, cmake, ctest)
cd fujinet-nio
./build.sh -p fujibus-rs232-debug      # first build
./build.sh -cp fujibus-rs232-debug     # clean + build

# Build fujinet-nio-lib for Amiga
cd fujinet-nio-lib && make amiga

# Build all Amiga apps (includes battleship; auto-builds the compat shim + gamekit)
make -C apps

# Or per app
make -C apps/http_get
make -C apps/battleship/amiga battleship   # note: explicit target; bare `make` builds the ADF

# Build the Amiga disk driver (fujinet-disk.device); needs amiga-gcc on PATH
export PATH=/opt/amiga/bin:$PATH
make -C fujinet-nio-driver amiga     # bare `make` also builds MS-DOS (needs Watcom)

# Game-port one-time extra deps (game sources; not initialized by default)
git submodule update --init apps/battleship/upstream
git submodule update --init apps/fujitzee/upstream
```

`fujinet-nio-driver` finds `fujinet-nio-lib` as a sibling by default, which is
why it is pinned at the repo root rather than under `apps/`; override with
`make amiga LIB_ROOT=/path/to/fujinet-nio-lib`. Pin it and `fujinet-nio-lib`
together — the driver README records the minimum compatible library revision.

**Known toolchain difference (upstream, not ours).** `make amiga` builds
`fujinet-disk.device` cleanly, then fails on the `fujinet-mount` diagnostic
tool with `-Werror=format` errors. Our amiga-gcc resolves `ULONG`/`LONG` to
`unsigned int`/`int` — anything that includes `<stdio.h>` gets NDK
`exec/types.h`'s `__use_amiga_stdc_c99` branch (`uint32_t`), under every
`-mcrt=`. Mark's environment gets the `unsigned long` branch, so his `%lu`
format strings are correct there and wrong here. It is a build-environment
mismatch, not a driver defect; the portable fix is casting the arguments to
`unsigned long` at each call site. Don't "fix" it locally — the submodule is a
read-only pin.

See `fujinet-nio/docs/developer_onboarding.md` for full build options, ESP32 setup,
available profiles (`./build.sh -p -S`), and CLI testing tools.

## Testing

Three tiers, named by target prefix so fast and slow tests stay separable. Full
rationale + how to add tests: **`docs/testing.md`**.

| Tier | Command | Runs on | Speed / prereqs |
|------|---------|---------|-----------------|
| **T1 host unit** | `make test-host` | native `cc` | ms; nothing but a host compiler (CI default) |
| **T2 emulator smoke** | `make emu-test` | FS-UAE | 60–90 s; needs a running `fujinet-nio` |
| **T3 nio host suite** | `./build.sh -p …` | native (submodule) | seconds; doctest/CTest inside `fujinet-nio` |

```bash
# T1 — fast host unit tests (pure-logic Amiga C, native cc, no backend)
make test-host                                 # repo-wide
make -C libs/amiga-gamekit test-host           # one module

# T2 — slow emulator smoke tests (needs FS-UAE + a running fujinet-nio)
make emu-test                                  # repo-wide
make -C apps/fn_test emu-test                  # one app

# T3 — fujinet-nio host suite (submodule; runs ctest as part of the build)
cd fujinet-nio && ./build.sh -p fujibus-rs232-debug
```

T1 tests live in `<module>/test/host/test_*.c` and use the tiny C99 harness
`libs/fujinet-compat-amiga/test/host/fn_test.h`. Only AmigaOS-free (`proto/*`,
`dos/*`, …) pure-logic code is T1-eligible; anything touching AmigaOS is tested
at T2. Do **not** modify the T3 suite from the parent repo — test changes inside
`fujinet-nio` / `fujinet-nio-lib` go through their own upstream-PR flow.

## Emulator Testing

Use the `/emu-build-and-boot` skill to build an ADF, boot it in FS-UAE, and
capture a screenshot automatically. See `.claude/commands/emu-build-and-boot.md`
for the full workflow.

There are three emulator modes, all sharing the same socat + `fujinet-nio`
wiring:

| Script | Display | Input | Use |
|--------|---------|-------|-----|
| `emu/run.sh` | Xvfb | none | `make emu-test` — CI pass/fail on a grep pattern |
| `emu/drive.sh` | Xvfb | scripted | keyboard-driven checks over SSH; captures pixel-exact shots |
| `emu/play.sh` | visible | you | `make emu-play` — hands-on, needs a desktop |

`drive.sh` is what makes the "manual" checks reachable from a remote CLI — it
types a token script through `emu/scripts/emukey.py` (XTEST) and dumps FS-UAE's
internal screenshots at labeled points:

```bash
APP_NAME=fujitzee ADF_PATH=apps/fujitzee/amiga/fujitzee.adf \
  KEYS="shot:lobby Return sleep50 space shot:table" bash emu/drive.sh
```

`shot:<label>` writes `<label>.png`; every other token is an emukey keysym or
`sleepN` (tenths of a second). `NO_SERVER=1` skips `fujinet-nio` for offline
harnesses — that is what `make -C apps/fujitzee/amiga preview-play` uses.

`AUDIO_WAV=<path>` captures what Paula actually played, so sound is checkable
without speakers: FS-UAE plays through OpenAL, whose "wave" backend writes a
.wav instead of opening a device, and `emu/checkaudio.py` reports the
non-silent bursts (`--expect N` to gate a run). Use it — two sound defects
shipped undetected precisely because "is there sound?" was answered by ear
(strategic-plan Lessons Learned 2026-08-06). For fujitzee the whole effect
table is reachable offline: `boardpreview` plays all twelve on SPACE.

`emu/clipaudio.py` turns a capture into something you can actually listen to
— trimmed to the effects, mono, normalised — which is how sound gets reviewed
when the only speakers are on the other end of an SSH session. `--ab` lays two
captures end to end under one shared gain, so a before/after is honest about
relative loudness; `--burst N` picks the same effect out of each. That loop is
what tuned the dice roll from a single noise burst to a three-pulse clatter.

```bash
AUDIO_WAV=/tmp/cap.wav NO_SERVER=1 APP_NAME=fujitzee-preview \
  ADF_PATH=apps/fujitzee/amiga/boardpreview.adf \
  KEYS="sleep80 space sleep330" bash emu/drive.sh
emu/checkaudio.py /tmp/cap.wav --expect 12     # measure
emu/clipaudio.py  /tmp/cap.wav -o /tmp/clip.wav # listen
```

`JOYSTICK=1` tests **stick** input instead: it puts FS-UAE's built-in
`keyboard` controller in the game port, so the arrow keysyms move the emulated
joystick and `Control_R` is its fire button. They then stop reaching the
emulated keyboard, which is the whole trick — and also the reason a joystick
run needs its own key script.

Two things that cost time to find:

- **`emukey.py` needs python-xlib**, which the system python3 lacks. The repo
  keeps a venv at `emu/.venv` (gitignored); `drive.sh` prefers it.
- **Free the joystick port before expecting arrow keys.** With no joystick
  attached, FS-UAE maps the host arrow keys to joystick port 1, so they never
  reach the emulated keyboard. `drive.sh` sets `joystick_port_1 = nothing`
  unless `JOYSTICK=1` asks for the opposite.

Quick setup:
```bash
sudo apt install fs-uae socat xvfb jq   # emulator, serial bridge, headless display
pip3 install amitools                    # xdftool — builds/populates ADF images
cp emu/config/paths.env.example emu/config/paths.env
# edit paths.env: set KICKSTART_ROM to your KS 1.3 ROM path
```

Every ADF requires `Devs/serial.device` extracted from a Workbench 1.3.4 disk image —
see `contracts/amiga-adf-bootstrap.md`. ADFs are gitignored (copyright).

Serial port is configured at runtime via environment variables:
- `FN_SERIAL_PORT` (default: `/dev/ttyUSB0`)
- `FN_SERIAL_BAUD` (default: `19200`)

## Orchestration Pattern

The top-level Claude Code session coordinates work across submodules.

**Pattern:**
1. Write or update the contract in `contracts/` first — before any code.
2. Implement against it in the current session by default, committing inside the
   submodule on its own feature branch.
3. Update the submodule pointer in the parent repo.

**Key rule:** `contracts/` is the handoff point. The spec lives in a file and is
referenced by path, so it survives `/clear`, a new session, or a reviewer who
wasn't there — never in conversation history alone.

### Delegation

Delegate to a subagent only for genuinely independent, sizeable tracks — a wide
multi-file investigation, or two tracks being worked in parallel worktrees. A
separate submodule session (`cd fujinet-nio-lib && claude --print "implement X
per ../contracts/amiga-transport-api.md"`) is one such case, and it still reads
the contract by path rather than inheriting context.

Don't delegate work you can finish in a handful of tool calls, and don't use a
subagent to check your own work. One agent beats several.

## Contracts

| File | Purpose |
|------|---------|
| `contracts/rs232-hardware.md` | Amiga RS-232 pinout, baud, cable wiring |
| `contracts/fujibus-protocol.md` | FujiBus + SLIP framing spec summary |
| `contracts/amiga-transport-api.md` | What to implement in `fujinet-nio-lib/src/platform/amiga/` |
| `contracts/amiga-coding-conventions.md` | C style, AmigaOS API rules, memory/stack constraints for Amiga code |
| `contracts/amiga-app-api.md` | Public fujinet-nio-lib API for writing Amiga apps (fn_open, fn_read, HTTPS, etc.) |
| `contracts/amiga-adf-bootstrap.md` | ADF build recipe, serial.device requirement, startup-sequence rules |
| `contracts/fujinet-appkey-protocol.md` | Classic AppKey API served natively by nio-lib's `fn_legacy_appkey.h` (`persist:///FujiNet/*.key`) |
| `contracts/esp32-target.md` | ESP32 target: decided facts (PHY candidates, what stays identical) + deferred work |

When adding new cross-submodule features, write the contract first.

## Documentation

`docs/` follows a defined process — see **`docs/README.md`** for the full rules.
In short:

- **Evergreen docs** (procedures, references, strategy) are edited in place;
  git history is their archive. Never move them aside.
- **Point-in-time artifacts** (handoffs, debug/impl plans) are snapshots. Don't
  mutate them to stay current — rehome their durable facts into the owning
  evergreen doc (usually this file), then `git mv` the snapshot to
  `docs/archive/` with an `ARCHIVED` status banner.

Canonical build/run steps live here in `CLAUDE.md`, not in `docs/`.

A PR that starts or completes a track/phase must update the Status table in
`docs/strategic-plan.md` in the same PR — that table is the first thing a new
session reads, and it goes stale the moment this rule is skipped.
