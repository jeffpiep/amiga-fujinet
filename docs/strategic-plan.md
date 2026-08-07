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

### Track 1C — Fujitzee Amiga Port

Second port of a FujiNet game ([FujiNetWIFI/fujinet-fujitzee](https://github.com/FujiNetWIFI/fujinet-fujitzee),
a multiplayer Yahtzee-style dice game). Its purpose is different from 1B's:
Battleship proved the compat layer works at all; **Fujitzee proves a second port
is cheap** — that the Amiga platform work is a reusable asset rather than a
one-off.

The port-surface audit is already done: fujitzee uses exactly the same six
FujiNet functions Battleship does, all covered by Track 1A. No compat-layer work
is expected.

The headline change was **Phase 0: extract `libs/amiga-gamekit`** — pull the
reusable screen/tile engine, key translation, joystick decode, waveform
generation and timer out of `apps/battleship/amiga/src/` into a shared library,
proven by Battleship still passing its T1 and T2 suites. Done 2026-08-05.
Everything after that is fujitzee-specific renderer, sound, and art. Phase 1
(2026-08-05) scaffolded `apps/fujitzee/amiga/` and got the upstream sources
compiling and linking on stubs — the cheap-second-port thesis is holding so
far: no compat-layer work was needed, and the only real obstacle was the
server wire format's struct packing — which produced the port's one upstream
PR, a two-line opt-in. Phase 2 (2026-08-06) replaced the stubs with the real
renderer and keyboard, and made the thesis pay twice: the port needed two
things battleship already had (the IDCMP key pump, the tile-art composers),
and both were extracted into the gamekit rather than copied — battleship got
shorter, not forked. Phase 3b (2026-08-06) did the same for the joystick, and
the third extraction was the cheapest yet: two register reads and a pure
decode, leaving battleship's genuinely game-specific mouse aiming behind.
Phase 3a (2026-08-06) closed the port with sound, and the fourth extraction
was the one Phase 0 flagged as the genuinely hard call — battleship's
`sound.c` mixed effect definitions with `audio.device` playback. The seam
held anyway: playback became `libs/amiga-gamekit/src/gksound.c`, the effect
tables stayed per game, and battleship's `sound.c` halved. Four for four on
extract-rather-than-copy, and each one has left battleship smaller, which is
the cheap-second-port thesis paying out about as clearly as it can.

Full plan, including the five other process changes from 1B:
`docs/plan-track1c-fujitzee.md`.

**Game server:** `https://fujitzee.carr-designs.com/`

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
| Serial transport | ✅ Done — **upstreamed 2026-08-08** | 19200 baud, SLIP, FujiBus. Merged into `markjfisher/fujinet-nio-lib` as [#1](https://github.com/markjfisher/fujinet-nio-lib/pull/1) (`c69eadd`, a real merge commit — our `381b883` is preserved in upstream history). `fujinet-nio-lib` therefore left *Riding a PR* and now Tracks `master`, which makes every submodule Tracking for the first time |
| fn_* API (`fn_open`, `fn_read`, `fn_write`) | ✅ Done | HTTP + HTTPS GET/POST working |
| `http_get` demo app | ✅ Done | HTTPS auto-detect, NTSC mode, weather demo |
| `fn_test` smoke test | ✅ Done | Validates serial transport end-to-end |
| Track 1A — compat layer | ✅ Done (2026-07-01) | `libs/fujinet-compat-amiga`; header-sync procedure in `docs/updating-fujinet-compat-headers.md` |
| Track 1B — Battleship port | 🚧 Phase 4 complete (2026-07-28) — awaiting real hardware | 3a joystick ✅, 3b sound ✅, 3c graphical renderer ✅. Tile engine on a custom 320×200×4 screen, playable end-to-end in emulator (lobby → placement → gameplay → menu); mouse aiming (#21) and mouse ship placement (#23) merged; tile art pass done (`tiles.h` — multicolor sea/ships/markers/explosion, previewed via the `tilegallery` harness). Phase 4 ADF boot test ✅ (2026-07-28: full game in FS-UAE off the ADF, 6706 FujiBus frames, no errors). Remaining: real Amiga 500 + PiStorm. Also unblocks the upstream port PR |
| Track 1C — Fujitzee port | ✅ Complete (2026-08-06) | Upstream pinned at `apps/fujitzee/upstream`; port surface audited (same six FujiNet functions as Battleship — no compat-layer work needed); phases in `docs/plan-track1c-fujitzee.md`. Phase 0 ✅: `libs/amiga-gamekit` extracted (screen/tile/sprite core, key + joystick decode, waveform bakers, jiffy clock, PRNG); Battleship rebuilt against it and passes T1 + T2. Phase 1 ✅: scaffold links under m68k; server wire format needed packed structs, fixed by upstream PR [fujinet-fujitzee#9](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/9) (`FUJITZEE_PACK_STRUCTS`), merged 2026-08-06. Phase 2 ✅: real renderer on the gamekit screen — 40x25 cells, six player columns, the DOS layout (the Atari's is one row taller and does not fit); dice as 16 tiles x 3 face colours; keyboard via a new shared gamekit key queue, with arrows decoded as control codes because fujitzee's menus and name entry already use W/A/S/D. Boots and talks to `fujinet-nio` (T2 PASS); the scorecard is verified by a `boardpreview` ADF since it only renders in a live game. Phase 3b ✅: real joystick on game port 2 — the read extracted to the gamekit (`gkjoy.c` + a T1-tested `joyDecodePort2()` pinning the active-low fire line), and verified headlessly by `emu/drive.sh`'s new `JOYSTICK=1` mode, which puts FS-UAE's keyboard controller in the game port so a scripted run can move the stick. Phase 3c ✅: the art pass — bevelled multicolor dice, a five-cell mountain+TZEE wordmark on the game's own score row, and tiles for the turn marker and score cursor, all data in `tiles.h`. It also closed a real defect it uncovered: a four-digit score is right-aligned by upstream into a three-cell column and starts on the board divider, so the renderer now squeezes it into the 30 px that are actually free (`fj_score_spill()`, T1-tested, plus one new gamekit primitive `gfx_text_tight()`). Phase 3a ✅: sound — 13 effects at the pitches upstream's DOS port back-derived from the Atari POKEY, and the `audio.device` playback extracted to the gamekit (`gksound.c`) so battleship shares it, halving its `sound.c`. Verified by capturing the emulator's audio (`AUDIO_WAV=` in `drive.sh`, `emu/checkaudio.py`) rather than by ear, which found two defects nothing else would have: every effect that did not decay to zero left a DC offset on the channel, and FS-UAE drops the `AUDxPER` write the same way it drops `AUDxVOL`, so everything played ~4x too fast. Both fixed at the gamekit level, so battleship gets them too. Phase 3d ✅ (2026-08-07): opponent scores arrive big-endian — `QUERY_SUFFIX "&be=1"` in `amiga_vars.h` asks the server for it, after upstream merged and then reverted a client-side swap ([#10](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/10) → [#12](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/12)) in favour of the flag the CoCo port already used; `apps/fujitzee/upstream` tracks upstream `main` again. No stubs remain |
| Amiga disk device (`fujinet-disk.device`) | ⏸️ Deferred (2026-08-07) — blocked on parallel-port PHY | Remote block/ADF mounting: `DN0:` backed by blocks served over the wire, the Amiga's own ROM filesystem supplying all structure. Design and layering agreed in `docs/plan-amiga-disk-device.md`; **Q2 closed as disqualifying** — RS-232 at 19200 baud is ≈1.9 KB/s (~270 ms per 512-byte block, ~7.8 min for an 880K volume), so this waits on the faster PHY (`contracts/esp32-target.md`). Q1/Q3–Q6 still open. Also needs a block endpoint in `fujinet-nio`, which is not yet written |
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
- **2026-08-06** — FS-UAE drops the **`AUDxPER`** write audio.device performs
  at `CMD_WRITE`, exactly as it drops `AUDxVOL` (2026-07-08). Effects then
  play at whatever rate the channel was last left at — measured ~4x fast, two
  octaves high and a quarter as long. The tell that it is a dropped write and
  not a mis-set value: the output is byte-identical whether `ioa_Period` is
  447 or 1788. `gk_snd_play()` now re-pokes period and volume together
  (`pokePerVol()`), a no-op on real hardware. **The general lesson is the
  second one:** this shipped undetected in battleship because "is there
  sound?" was checked by ear on a screenshot-driven run, and wrong-pitch
  audio is still audio. `AUDIO_WAV=… bash emu/drive.sh` + `emu/checkaudio.py`
  now capture and measure it — FS-UAE plays through OpenAL, whose "wave"
  backend writes a .wav instead of opening a device.
- **2026-08-06** — A sample that ends on a non-zero amplitude leaves Paula
  holding that value as a **DC offset** on the channel until the next effect.
  Silent on its own, so it survives casual listening, but it thumps on the
  next transition. Bake every effect down to `vol_end 0`; the rule is stated
  in `libs/amiga-gamekit/include/sndgen.h`. Found by the same audio capture,
  as a burst whose zero-crossing rate was 0 while its mean amplitude was
  thousands — the signature of DC rather than sound.
- **2026-08-07** — A merged upstream PR can still be reverted. Fujitzee's
  client-side score byte-swap (fujinet-fujitzee#10) merged on 2026-08-06 and
  was reverted the next day (#11, #12): the server already accepted a `&be=1`
  query parameter that makes it emit big-endian, which the CoCo port had been
  using all along. Two lessons. **Before writing a client-side fix for a wire
  format, check whether the server can just send what you want** — the
  server-side flag has no call sites to get wrong, while the client swap had
  to be invoked only where the union was known to hold a `Game`. And
  **`Riding a PR` does not end at "merged"** — re-read the upstream repo on
  each routine sync, because a revert removes something the port depends on
  and it will not break the build, only the display. Procedure for the exit:
  `docs/syncing-upstream-submodules.md` ("If the PR was reverted").
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
- **2026-07-28** — `fn_transport_close()` is defined by every nio-lib platform
  backend but declared in no header, so nothing ever calls it. On Amiga that
  leaks `serial.device` past process exit (`AllocMem` isn't reclaimed at exit),
  and a second run of any FujiNet app silently behaves as if offline — no error
  is surfaced, it just stops talking. Cost a confusing appkey debug session:
  the persistence code was fine, the *second launch* was the broken thing.
  Lesson: when an Amiga app "loses" the network on a re-run, suspect leaked
  exec resources from the previous run before suspecting the feature under
  test. Details and the deferred fix: `contracts/amiga-transport-api.md`.
- **2026-07-12** — Three graphical-renderer gotchas surfaced during the Phase 3c
  scaffold, all rehomed to their owning docs: xwd screenshots scramble color
  channels on saturated colors while monochrome looks fine (capture artifact,
  not an Amiga bug — text-only screens hid it for weeks) and the emu-test pass
  pattern can fire before the first screen draw → both documented with the
  FS-UAE internal-screenshot (F12+S) recipe in
  `.claude/commands/emu-build-and-boot.md`; the system default font may be
  10 px-wide 60-column topaz rather than topaz 8 → documented in
  `contracts/amiga-coding-conventions.md` (Custom screens / rendering).
