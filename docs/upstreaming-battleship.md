# Upstreaming the Amiga Battleship Port

How and when to contribute the Amiga port to
[`FujiNetWIFI/fujinet-battleship`](https://github.com/FujiNetWIFI/fujinet-battleship).

## Decision

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

## The two integration blockers to settle with the maintainer

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

## Fork workflow (per CLAUDE.md, when ready)

```bash
# origin = your fork, upstream = authoritative
git -C apps/battleship/upstream remote add upstream https://github.com/FujiNetWIFI/fujinet-battleship.git
# push a feature branch to jeffpiep/battleship, PR -> FujiNetWIFI/fujinet-battleship
```

## Later checklist (Phase 3 → 4)

- [ ] Graphical renderer replaces ASCII `graphics.c` (Phase 3)
- [ ] Sound + joystick (Phase 3)
- [ ] Tested on real Amiga 500 + PiStorm (Phase 4)
- [ ] Restructure into `src/amiga/`, rename `amiga_vars.h` → `vars.h`
- [ ] `amiga` PLATFORM target in the upstream Makefile / MekkoGX
- [ ] fujinet library path agreed and wired
- [ ] Fork + feature-branch PR to `FujiNetWIFI/fujinet-battleship`

---

## DRAFT — coordination message (review before posting; do not auto-post)

> Intended as a GitHub Discussion (or Issue) on `FujiNetWIFI/fujinet-battleship`.

**Title:** Adding an Amiga (AmigaOS / Kickstart 1.3) port — toolchain & fujinet-lib questions

**Body:**

Hi! I've been building an Amiga port of Battleship and have a working prototype:
it boots on a stock Amiga 500 / Kickstart 1.3, connects to the FujiNet game
server over RS-232, and plays a full game (lobby → ship placement → multiplayer
rounds), keyboard-controlled. I'd like to eventually contribute it upstream and
want to align on structure before I finalize it.

Two areas I'd love your guidance on:

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

The port maps cleanly onto your `src/<platform>/{graphics,input,sound,util}.c +
vars.h` layout. I'm planning to open the PR once the renderer is finalized
(moving from the current text console to a graphical AmigaOS window) and it's
tested on real hardware — but wanted to sort out the build/library integration
with you first so it's structured the way you'd want. Happy to hop on a
discussion or follow whatever contribution process you prefer. Thanks!
