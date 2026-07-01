# Handoff: Sync fujinet-nio with Mark's merged PR + fix serial config

> **Status: ARCHIVED — completed 2026-07-01.** This handoff was consumed; the
> nio sync landed (see commit "Sync fujinet-nio to merged RS-232 PR"). Kept as
> a historical record.

Kicks off a fresh session. Our `fujinet-nio` RS-232 PR was merged upstream by
Mark Fisher, who made alignment changes — most importantly, **serial settings now
come from the config system**, so our 19200 8N1 link needs explicit config or it
will break.

## The merged PR

- **PR:** https://github.com/markjfisher/fujinet-nio/pull/6 — "Add RS-232 serial
  channel for Amiga prototype" — **MERGED** 2026-06-30, merge commit
  **`80039ae`** on `markjfisher/fujinet-nio`.
- Read the PR conversation for Mark's exact notes.

### Mark's changes (from his PR comment)

1. **cmake/template drift** — the cmake file we edited is *generated*; the edit
   belonged in the **template** version. When syncing, do NOT re-apply our old
   edit to the generated file — take Mark's template-based version.
2. **Tests** — Mark added channel tests (we had none). Expect new test files.
3. **Config system** — Mark moved `FN_SERIAL_PORT` / `FN_SERIAL_BAUD` into the
   existing config system, **keeping the env vars as overrides**.

### Mark's note to Jeff (the important bit)

> "I've aligned setting the serial configuration from the existing `serial.uart`
> object in the configuration, so if you were relying on default 8N1, you need to
> specify it on the config now."

## Why this matters — the serial config

Serial framing/baud now read from the **uart config object**. In the code it's
`ChannelConfig.uart` (type `UartConfig`) in
`fujinet-nio/include/fujinet/config/fuji_config.h`:

```
struct UartConfig {
    std::uint32_t   baudRate{115200};          // <-- default is 115200, NOT ours
    int             dataBits{8};               // 8
    UartParity      parity{None};              // N
    UartStopBits    stopBits{One};             // 1
    UartFlowControl flowControl{None};
};
```

Our Amiga link is **19200 8N1** (see `contracts/rs232-hardware.md`). Previously
the `SerialChannel` hard-coded 8N1 via termios; now it honours this config object.
Two risks to close:
- **Baud:** struct default is 115200. Our `emu/run.sh` sets `FN_SERIAL_BAUD=19200`
  (env override), so confirm the override still wins after Mark's change.
- **Framing:** set 8N1 explicitly in the config (`dataBits 8, parity None,
  stopBits One`) rather than relying on defaults, per Mark's instruction. Verify
  the exact config key in the merged code (`serial.uart` vs `channel.uart`) and
  whether there's an env override for framing too.

## Current repo state

- Submodule `fujinet-nio/`: origin `jeffpiep/fujinet-nio`, upstream
  `markjfisher/fujinet-nio`.
- Local submodule is on our PR branch **`feature/amiga-rs232-pr`** at `bb8a552`
  (pre-merge, pre-Mark's-changes). Parent repo pins `ad8a20a8`.
- Client lib `fujinet-nio-lib/` is on `feature/amiga-rs232-transport` — check if
  it needs a matching config/baud change.

## Tasks

1. **Sync fork + submodule to Mark's merged upstream:**
   ```bash
   git -C fujinet-nio fetch upstream
   git -C fujinet-nio checkout master   # or main — confirm default branch
   git -C fujinet-nio merge --ff-only upstream/master
   git -C fujinet-nio push origin master   # update our fork
   ```
   Then set the submodule to the merged commit and drop our stale
   `feature/amiga-rs232-pr` branch (its work is now upstream, superseded by
   Mark's template-correct version).
2. **Read PR #6** — confirm Mark's tests + the config wiring; note the exact
   config key and any config-file format (the POSIX build had no runtime
   `.toml/.json` config checked in — find how it's loaded now).
3. **Set serial config to 19200 8N1 explicitly** wherever fujinet-nio reads it
   for the `fujibus-rs232-debug` profile. Reconcile with the `FN_SERIAL_BAUD`
   env override in `emu/run.sh`.
4. **Rebuild + test:**
   ```bash
   cd fujinet-nio && ./build.sh -cp fujibus-rs232-debug
   cd apps/battleship/amiga && make emu-test   # must still PASS (~12s)
   ```
   Confirm `[SerialChannel]` opens at 19200 8N1 and FujiBus framing works.
5. **Bump the `fujinet-nio` submodule pointer** in `amiga-fujinet` on a feature
   branch → squash PR to `main` (per CLAUDE.md).
6. **Check `fujinet-nio-lib`** (Amiga client) — no config change expected (the
   Amiga sets its own serial.device to 19200 8N1), but verify nothing regressed.

## Key files / references

| Path | Role |
|------|------|
| `fujinet-nio/include/fujinet/config/fuji_config.h` | `UartConfig` / `ChannelConfig` structs |
| `fujinet-nio/src/platform/posix/channel_factory.cpp` | `SerialChannel` (termios) |
| `fujinet-nio/build.sh`, `CMakePresets.json` | build; `fujibus-rs232-debug` preset |
| `emu/run.sh` (~line 171) | sets `FN_SERIAL_PORT=/tmp/fn-pty FN_SERIAL_BAUD=19200` |
| `contracts/rs232-hardware.md` | documents 19200 8N1 — update if config format changes |
| `contracts/fujibus-protocol.md` | FujiBus/SLIP framing |

## Verification (done when)

- `make -C apps/battleship/amiga emu-test` PASSES against the synced fujinet-nio.
- Serial link confirmed 19200 8N1 (log shows correct open); a battleship game
  still connects to the lobby.
- `fujinet-nio` submodule pointer bumped and merged to `amiga-fujinet` main.

## Project context

Battleship **Phase 2 is complete and merged** (parent PR #2, commit `301b350`).
This nio-sync is infrastructure before Phase 3 (graphical renderer + sound +
joystick). See `docs/handoff-phase2-completion.md` and
`docs/upstreaming-battleship.md`.
