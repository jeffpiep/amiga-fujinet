# Upstreaming the Amiga Game Ports

How and when to contribute our Amiga ports back to their upstream repos:

| Port | Upstream | Local plan |
|---|---|---|
| Battleship | [`FujiNetWIFI/fujinet-battleship`](https://github.com/FujiNetWIFI/fujinet-battleship) | `docs/plan-track1b-battleship.md` |
| Fujitzee | [`FujiNetWIFI/fujinet-fujitzee`](https://github.com/FujiNetWIFI/fujinet-fujitzee) | `docs/plan-track1c-fujitzee.md` |

Both repos share a maintainer, the MekkoGX build system, the
`src/<platform>/{graphics,input,sound,util}.c + vars.h` layout, and the
`fujinet-lib` dependency — so **the two integration blockers below are the same
for both ports, and one upstream conversation should settle both.**

## Decision (Battleship)

**Wait for Phase 3 to open the port PR, but coordinate with the maintainer early.**

Rationale:
1. **Phase 2's renderer is throwaway.** The current `graphics.c` is the ASCII
   text-console renderer, slated to be replaced by a graphical Workbench app in
   Phase 3. Upstreaming it now means contributing code we're about to delete.
2. **Not feature-complete.** No sound, no joystick (Phase 3); untested on real
   hardware (Phase 4). The existing upstream ports are complete.
3. **Architectural mismatch to resolve first** (see below) — best agreed with
   the maintainer *before* finalizing the port so we don't build it twice.

Until then, the port keeps living in **this** repo at `apps/battleship/amiga/`,
developed against the pinned `apps/battleship/upstream` submodule.

## How our port maps onto upstream

Upstream ports live at `src/<platform>/{graphics,input,sound,util}.c + vars.h`
(apple2, atari, c64, coco, msdos). Ours has the same files, so the Amiga port
would become:

| Ours (this repo) | Upstream target |
|------------------|-----------------|
| `apps/battleship/amiga/src/graphics.c` | `src/amiga/graphics.c` |
| `apps/battleship/amiga/src/input.c`    | `src/amiga/input.c` |
| `apps/battleship/amiga/src/sound.c`    | `src/amiga/sound.c` |
| `apps/battleship/amiga/src/util.c`     | `src/amiga/util.c` |
| `apps/battleship/amiga/include/amiga_vars.h` | `src/amiga/vars.h` (renamed) |

Fujitzee maps the same way (`apps/fujitzee/amiga/src/*` → `src/amiga/*`), with
two extra upstream asks specific to it:

- **A `PLATFORM_VARS` hook in `src/platform-specific/vars.h`.** Battleship has
  one; fujitzee hard-codes `#include "../atari/vars.h"` etc. Our port does not
  wait on this (we force-include our header with `-include`), but upstreaming
  the port cleanly wants the same hook Battleship already has.
- **The cc65/CMOC shim headers.** `misc.h` includes `<conio.h>`/`<joystick.h>`
  and `gamelogic.c` includes `<peekpoke.h>`; upstream's `src/include/` stubs
  don't fit amiga-gcc. Ours would land as `src/amiga/`-local headers or an
  `EXTRA_INCLUDE_AMIGA` dir, mirroring `EXTRA_INCLUDE_COCO`.

## The integration blockers to settle with the maintainer

Same for both ports — settle once.

1. **Toolchain.** Upstream builds through the **MekkoGX** Makefile system
   (cc65 for the retro targets) and auto-downloads **`fujinet-lib` 4.8.2**. The
   Amiga port uses **`m68k-amigaos-gcc`** (bebbo/amiga-gcc) plus ADF packaging —
   MekkoGX has no notion of that toolchain. Need an `amiga` PLATFORM target and
   agreement on how the toolchain/ADF steps fit their build.

2. **FujiNet library.** Upstream links `fujinet-lib`. The Amiga port does not —
   it goes through **`fujinet-nio-lib`** (markjfisher) + our **compat shim**
   (`libs/fujinet-compat-amiga`, which maps `fujinet-fuji.h` / `fujinet-network.h`
   onto the NIO library). Options to discuss:
   - keep the compat shim in the upstream repo under `src/amiga/`;
   - add Amiga support to `fujinet-lib` itself;
   - or a sanctioned per-platform library hook.

3. **Shared Amiga platform code across games.** Both ports link
   `libs/amiga-gamekit` (custom screen + tile bank, key translation, joystick
   decode, waveform generation, timer) — see `docs/plan-track1c-fujitzee.md`
   Phase 0. Upstream's per-platform dirs have no notion of code shared *between
   game repos*. Options: vendor a copy into each `src/amiga/`, publish the
   gamekit as its own repo the builds fetch (like `fujinet-lib`), or fold it
   into whatever answer blocker 2 gets.

## Fork workflow (per CLAUDE.md, when ready)

```bash
# origin = your fork, upstream = authoritative
git -C apps/battleship/upstream remote add upstream https://github.com/FujiNetWIFI/fujinet-battleship.git
git -C apps/fujitzee/upstream  remote add upstream https://github.com/FujiNetWIFI/fujinet-fujitzee.git
# push a feature branch to your fork, PR -> the FujiNetWIFI repo
```

## Later checklist

Battleship (Phase 3 → 4):

- [x] Graphical renderer replaces ASCII `graphics.c` (Phase 3)
- [x] Sound + joystick (Phase 3)
- [ ] Tested on real Amiga 500 + PiStorm (Phase 4)

Fujitzee: gated on Track 1C Phase 4 — see `docs/plan-track1c-fujitzee.md`.

Both, before either PR opens:

- [ ] Restructure into `src/amiga/`, rename `amiga_vars.h` → `vars.h`
- [ ] `amiga` PLATFORM target in the upstream Makefile / MekkoGX
- [ ] fujinet library path agreed and wired
- [ ] Shared-gamekit distribution agreed (blocker 3)
- [ ] Fork + feature-branch PR to the upstream repo

---

## DRAFT — coordination message (review before posting; do not auto-post)

> Intended as a GitHub Discussion (or Issue) on `FujiNetWIFI/fujinet-battleship`,
> cross-linked from `fujinet-fujitzee`.

**Title:** Adding Amiga (AmigaOS / Kickstart 1.3) ports — toolchain & fujinet-lib questions

**Body:**

Hi! I've built an Amiga port of Battleship: it boots on a stock Amiga 500 /
Kickstart 1.3, connects to the FujiNet game server over RS-232, and plays a full
game (lobby → ship placement → multiplayer rounds) on a custom 320×200 screen
with joystick, mouse, keyboard and sound. I'm now starting the same port of
Fujitzee, and the two share an Amiga platform library — so I'd like to align on
structure before finalizing either.

Three areas I'd love your guidance on:

1. **Toolchain.** The repo builds via MekkoGX with cc65 + auto-downloaded
   `fujinet-lib`. The Amiga build uses `m68k-amigaos-gcc` (bebbo's amiga-gcc)
   and packages an ADF. What's the preferred way to add a toolchain like this —
   a new `amiga` PLATFORM target, and how do you want non-cc65 toolchains and
   disk-image packaging to fit MekkoGX?

2. **FujiNet library.** Rather than `fujinet-lib`, the Amiga client currently
   talks to the server through `fujinet-nio-lib` (markjfisher) via a small compat
   shim that implements the `fujinet-fuji.h` / `fujinet-network.h` surface the
   game uses. Would you prefer that shim live under `src/amiga/`, that Amiga
   support be added to `fujinet-lib`, or some other per-platform hook?

3. **Code shared between the two game repos.** The Amiga screen/tile engine,
   key translation, joystick decode and sound-waveform code are identical for
   Battleship and Fujitzee, so I keep them in one small library rather than
   duplicating them. Would you rather each `src/amiga/` vendor its own copy, or
   have it fetched like `fujinet-lib` is?

Both ports map cleanly onto your `src/<platform>/{graphics,input,sound,util}.c +
vars.h` layout. (One small thing: fujitzee's `platform-specific/vars.h`
hard-codes its per-platform includes where battleship's uses a `PLATFORM_VARS`
define — I'm working around it locally, but would happily send a PR making
fujitzee match battleship.)

I'd like to open the PRs once each port is tested on real hardware, but wanted
to sort out the build/library integration with you first so they're structured
the way you'd want. Happy to hop on a discussion or follow whatever contribution
process you prefer. Thanks!
