# Amiga FujiNet — Strategic Plan

> This document records the project's strategic direction. Update it as lessons
> are learned and priorities shift — it is the long-lived accountability anchor,
> not a one-time snapshot.

---

## Vision

Bring networking to vintage Amiga hardware — specifically the Amiga 500 running
Workbench 1.3 through 3.x (including CaffeineOS on PiStorm) — using FujiNet as
the network coprocessor over RS-232 serial. FujiNet offloads TCP/IP, TLS/SSL,
DNS, and protocol adapters so the Amiga needs no TCP stack (no Miami, no AmiTCP).

**Out of scope:** AmigaOS 4 / "fancy" systems. The intended user is running a
classic Amiga, not a modern tower.

---

## Hard Constraints

These apply to all Amiga code in the project:

- **OS floor:** Kickstart / Workbench 1.3 — every API call must work on KS 1.3+
- **Compiler:** `m68k-amigaos-gcc`, C99, `-mcrt=nix13`
- **CPU:** 68000-safe — no FPU, no 68020+ instructions
- **Memory:** caller-provided buffers; no dynamic allocation in library code
- **Stack:** keep stack frames small; use `static` for buffers >128 bytes

---

## Architecture

```
Track 1: Cross-platform app ecosystem        Track 2: BSD socket compat layer
──────────────────────────────────────       ──────────────────────────────────
  Game/App (fujinet-network + fuji API)        App (socket/connect/send/recv)
                  │                                        │
  ┌───────────────┴─────────────────┐          libfn_bsdsocket.a
  │  fujinet-network/fuji compat    │                     │
  │  layer for Amiga/nio            │             fujinet-nio-lib
  └───────────────┬─────────────────┘                     │
                  │                            ────────────┘
          fujinet-nio-lib
                  │
          FujiBus / SLIP / RS-232
                  │
           fujinet-nio server
         (Linux RS-232 or ESP32)
```

The `fujinet-nio` server runs on either a Linux box (RS-232 dev/prototype) or
the ESP32 FujiNet hardware (eventual shipping target). Both are transparent to
Amiga-side code through the `fujinet-nio-lib` abstraction.

---

## Track 1: Cross-Platform App Ecosystem

Goal: Allow FujiNet apps and games written for the standard multi-platform
`fujinet-network` / `fujinet-fuji` API to run on Amiga with minimal changes.

### Track 1A — Compatibility Layer (`libfn_compat_amiga.a`)

A thin Amiga-specific implementation of the standard FujiNet library headers
(`fujinet-network.h`, `fujinet-fuji.h`) that delegates to `fujinet-nio-lib`.
Any game targeting those headers compiles for Amiga by linking this library.
**This is a reusable asset — every future Amiga port of a FujiNet game links
against it.**

**Known mapping (from Battleship analysis):**

`fujinet-network` — 3 functions cover all known game usage:

| fujinet-network | fujinet-nio-lib |
|---|---|
| `network_open(devicespec, mode, trans)` | strip `N:` prefix; detect `https://` → `FN_OPEN_TLS`; map mode → `fn_open()` |
| `network_read(devicespec, buf, len)` | `fn_read()` with `FN_ERR_NOT_READY` polling |
| `network_close(devicespec)` | `fn_close()` |

`fujinet-fuji` — AppKey storage → `ENVARC:` flat binary files:

| fujinet-fuji | Amiga replacement |
|---|---|
| `fuji_set_appkey_details(creator, app, size)` | set context for subsequent calls |
| `fuji_read_appkey(key_id, &count, data)` | read from `ENVARC:fujinet/<creator>/<app>/<key>` |
| `fuji_write_appkey(key_id, count, data)` | write to same path |

**Phases:**
1. Audit full upstream `fujinet-network.h` / `fujinet-fuji.h` against
   `fujinet-nio-lib`; document any gaps beyond the Battleship subset
2. Implement shim on `TARGET=linux` first; cross-compile for Amiga
3. Validate against Battleship before declaring done

**Location:** `libs/fujinet-compat-amiga/`

### Track 1B — Battleship Amiga Port

Proof-of-concept port of [FujiNet Battleship](https://github.com/FujiNetWIFI/fujinet-battleship)
to Amiga. Validates the Track 1A compat layer with a real multiplayer game.

The Battleship codebase has clean separation: all game logic in `src/*.c`
(platform-agnostic), platform specifics behind `src/platform-specific/*.h`
interfaces. Amiga work is purely in `src/amiga/`.

**Platform layer to implement:**

| Interface | Amiga approach |
|---|---|
| `graphics.h` — drawText, drawIcon, drawShip, drawGamefield, … | Console (KS 1.3 DOS) first; Intuition/graphics.library as stretch |
| `input.h` — cgetc, kbhit, readJoystick, waitvsync | `console.device` (keyboard); `gameport.device` (joystick); vblank via `graphics.library` |
| `sound.h` — soundCursor, soundHit, soundMiss, soundSink, … | `audio.device` simple tones |
| `util.h` — getTime, resetTimer, getRandomNumber, quit | `timer.device`; LFSR or `mathffp.library`; `exit()` |
| `vars.h` — screen dimensions, key constants | 80×25 terminal-safe values |

**Dependency:** Track 1A compat layer must be solid before integrating 1B.

**Phases:**
1. Console-only platform layer — text output, keyboard, no audio/joystick
2. End-to-end game loop: join table, play a game over FujiNet
3. Full platform layer: audio, joystick, proper screen layout
4. ADF boot test on emulator and real hardware

**Game server:** `https://battleship.carr-designs.com/` — existing public server,
no infrastructure to maintain.

---

## Track 2: BSD Socket Compatibility Layer

Goal: Make FujiNet a drop-in network adapter for any amiga-gcc program using
standard BSD socket calls. No TCP stack required.

### Phases

| Phase | Deliverable |
|---|---|
| 1 — Core TCP | `libfn_bsdsocket.a`: socket/connect/send/recv/closesocket |
| 2 — DNS | gethostbyname via server-side URL resolution (Option A first; FujiBus DNS query as Option B if needed) |
| 3 — TLS | Port 443 → `FN_OPEN_TLS` auto; `fn_bsd_set_tls(fd)` for explicit |
| 4 — Non-blocking | select() + WaitSelect() via polling loop |
| Stretch | Proper AmigaOS `.library` — drop-in bsdsocket.library for any binary |

**Key design rule (Phase 1):** Socket state must be per-opener (not global) from
day one. This is what allows the stretch `.library` promotion to be a wrapper,
not a refactor.

---

## Status

| Track | Status | Notes |
|---|---|---|
| Serial transport | ✅ Done | 19200 baud, SLIP, FujiBus |
| fn_* API (`fn_open`, `fn_read`, `fn_write`) | ✅ Done | HTTP + HTTPS GET/POST working |
| `http_get` demo app | ✅ Done | HTTPS auto-detect, NTSC mode, weather demo |
| `fn_test` smoke test | ✅ Done | Validates serial transport end-to-end |
| Track 1A — compat layer | ✅ Done (2026-07-01) | `libs/fujinet-compat-amiga`; header-sync procedure in `docs/updating-fujinet-compat-headers.md` |
| Track 1B — Battleship port | 🚧 Phase 3c scaffolding done (2026-07-12) | 3a joystick ✅, 3b sound ✅. 3c graphical renderer: tile engine on custom 320×200×4 screen playable end-to-end in emulator with placeholder art (lobby → placement → gameplay → menu verified); art pass (Atari-modeled tiles/font/palette in `tiles.h`) remains. Then Phase 4 (real hardware) |
| Track 2 Phase 1 — BSD sockets | 🔲 Not started | |
| Track 2 Phase 2 — DNS | 🔲 Not started | |
| Track 2 Phase 3 — TLS | 🔲 Not started | |
| Track 2 Phase 4 — WaitSelect | 🔲 Not started | |
| Stretch — AmigaOS .library | 🔲 Not started | |

---

## Lessons Learned

> Add entries here as the project progresses. Date each entry.

- **2026-06-28** — FS-UAE `serial_port = tcp:…` mode causes `IOERR_OPENFAIL` in
  `serial.device`. Always use a PTY pair (`socat` + `ser2net` or similar) for
  emulator testing. See `contracts/` and emulator config docs.
- **2026-07-02** — FS-UAE `.sdf` save-disk files silently override a freshly
  built ADF, causing stale boots that look like build failures. `emu/run.sh`
  now deletes them before every run; if a boot ever shows old behavior, check
  for stray `.sdf` files first.
- **2026-07-05** — Upstream squash-merges invalidate contributed submodule
  branches; never rebase them post-merge — drop the branch and re-track
  upstream master. Procedure: `docs/syncing-upstream-submodules.md`.
- **2026-07-07** — Our invented appkey→app-store namespace convention was
  superseded within a day by upstream adding the classic appkey API natively
  (`fn_legacy_appkey.h`, `persist://` alias). Before inventing a cross-client
  convention in a compat layer, ask upstream first — the flagged open question
  in the contract was the right instinct; implementing ahead of the answer
  cost a (small) rewrite.
- **2026-07-08** — FS-UAE 3.1.x drops the `AUDxVOL` write `audio.device`
  performs at CMD_WRITE start: OS-polite audio plays at volume 0 (silent,
  `io_Error=0`, correct DMA timing) while direct Paula register pokes sound
  fine — games bang the hardware, so the bug goes unnoticed. Workaround:
  re-poke the owned channel's volume register after `SendIO` (no-op on real
  hardware) — see `pokeVolume()` in `apps/battleship/amiga/src/sound.c`.
  Diagnosis technique worth remembering: record the PipeWire sink monitor
  (`pw-record --target <sink> --properties '{stream.capture.sink=true}'`)
  and RMS-analyze to distinguish "silent guest" from "muted host".
- **2026-07-12** — Three graphical-renderer gotchas surfaced during the Phase 3c
  scaffold, all rehomed to their owning docs: xwd screenshots scramble color
  channels on saturated colors while monochrome looks fine (capture artifact,
  not an Amiga bug — text-only screens hid it for weeks) and the emu-test pass
  pattern can fire before the first screen draw → both documented with the
  FS-UAE internal-screenshot (F12+S) recipe in
  `.claude/commands/emu-build-and-boot.md`; the system default font may be
  10 px-wide 60-column topaz rather than topaz 8 → documented in
  `contracts/amiga-coding-conventions.md` (Custom screens / rendering).
