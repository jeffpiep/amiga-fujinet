# Handoff — ADF broker rework and the first KS 1.3 boot of Mark's driver

> **Status: ARCHIVED — superseded 2026-09-26.** The rework merged as PR #44.
> The ADF recipe and driver facts now live in `contracts/amiga-adf-bootstrap.md`
> and CLAUDE.md; bugs A–C in `docs/response-to-mark-disk-device-review.md` and
> markjfisher/fujinet-nio-driver#1/#2; Amiberry tooling, pitfalls and the
> Guru-tracing method in `docs/testing.md` ("Amiberry").

**Status: In progress (2026-09-26).** Point-in-time snapshot; archive per
`docs/README.md` §2 once its facts are rehomed.
**Branch:** `feature/adf-broker-rework` (parent, cut from `dev` at `8654cd2`).
**Read first:** `docs/plan-catchup-2026-09.md` (the overall catch-up),
`docs/response-to-mark-disk-device-review.md` (our review, which this work
partly corrects).

## Where things stand

PRs #42 (submodule sync) and #43 (review reply) are merged to `dev`. The ADF
rework started with a proof of concept. It has shown **`http_get` fetching a
real HTTPS page on KS 1.3 through `fujinet-nio.device`**, but only after three
upstream driver bugs were fixed or worked around. None of the rework is in the
build scripts yet. Every result below came from hand-staged ADFs.

## What was proven, in order

The proof of concept staged the broker files into an `ADF_STATIC_DIR` and
passed the load lines through `EMU_STARTUP_PREFIX`; `build-adf.sh` already
supports both.

1. **`fujinet-load-resident` builds with our toolchain from the parent repo.**
   No `-mcrt=clib2` is needed: `m68k-amigaos-gcc -std=c99 -Wall -Wextra -Werror
   -O2 -mcpu=68000 -msoft-float -mcrt=nix13
   -Ifujinet-nio-driver/amiga/include
   fujinet-nio-driver/amiga/tools/fujinet-load-resident.c -lamiga`. That gives
   a 12 KB binary, which runs on KS 1.3; it uses only `LoadSeg`,
   `InitResident` and stdio. **The upstream `TOOL_CRT` ask is therefore no
   longer blocking.**
2. **Bug A — resident tag rejected** ("No matching resident tag"). Every
   device set `rt_EndSkip = &device_end`. In `nio.device` and `serial.device`
   that symbol was uninitialised, so it landed in `.bss`. In `disk.device` it
   was initialised, but GCC 6.5 placed it *before* the tag. The loader rightly
   requires `EndSkip` to fall after the tag and inside the first hunk. **Fixed:**
   `rt_EndSkip = (APTR)(&device_resident + 1)` in all three devices.
3. **Bug B — device node never named.** None of the three sets `ln_Name`,
   `ln_Type`, the version or the ID string; each init table's data-table
   entry is `0`. KS 1.3's `InitResident` does not fill them from the ROMTag
   (later Kickstarts do, which is why Mark's 3.1 guest works). So the device
   was added unnamed, `OpenDevice` could not find it, and `fn_init()` returned
   "Device not found". The loader still printed "Resident loaded", because
   it checks only `InitResident`'s return value. **Fixed:** set them in
   `device_init`.
4. **Bug C — Guru #00000003 on the first exchange.** The broker's worker is a
   plain `AddTask` Task. It calls `OpenDevice("serial.device")`
   (`nio.device/fujinet_nio_serial_backend.c:597`). On KS 1.3 that device is
   disk-based, and loading it makes ramlib reply to the caller's
   `pr_MsgPort` at `task+92`. A Task has no such field, so `PutMsg` goes
   through garbage.

   How it was pinned down, with Amiberry (see below):
   - `READ_MEM` on the Guru's task address showed an unnamed type-0 Task
     whose `tc_UserData` was the broker base.
   - A return address on its stack resolved to `backend_open+0x9c`, directly
     after `jsr -444(a6)`, i.e. `OpenDevice`.
   - The FS-UAE log's `Exception 3` at ROM `fc1b9c` is `PutMsg`'s
     `move.l a1,(a0)`. A second fault, at `ff4656`, computes `task+92`.

   **Not fixed upstream.** Workaround, verified: make `serial.device`
   resident from a Process before the broker opens it:

   ```text
   C:fujinet-load-resident DEVS:serial.device serial.device
   C:fujinet-load-resident DEVS:fujinet-nio.device fujinet-nio.device
   ```

   The durable fix belongs to Mark: make the worker a Process, or do the first
   serial open in the caller's context.
5. **Result:** with A–C handled, FS-UAE's `make -C apps/http_get emu-test`
   **passes**, and the screen shows "Baltimore,MD: … +58°F / 30 bytes". This
   is the first app to reach FujiNet since the September sync.

Also found: **`fn_test`'s pass pattern is dead.** It waited for FS-UAE's
`serial: setbaud: 19200`, which only fired when nio-lib opened `serial.device`
itself. The broker opens it lazily on the first exchange, and `fn_is_ready()`
sends none, so `fn_test` now prints "FujiNet is ready." while proving nothing
about the link.

## Uncommitted or unpushed state — check this first

- **`fujinet-nio-driver`** is on local branch
  `feature/resident-endskip-in-code-hunk`, two commits ahead of `99ff749`,
  **not pushed**:
  - `35d0469` — fix(amiga): point rt_EndSkip past the ROMTag
  - `d11d503` — fix(amiga): name the device node in device_init for KS 1.3

  The driver's host tests pass (`make -C fujinet-nio-driver/amiga tests`).
  The parent sees the submodule as modified; **the pointer is deliberately not
  committed** yet.
- `fujinet-nio/build/fujibus-tcp-debug/` was built for the Amiberry probe
  (build output, untracked).
- The hand-staged ADFs in `apps/*/` are gitignored and disposable.

## Next steps

1. **The ADF rework proper** (this branch):
   - `emu/scripts/build-adf.sh` installs `Devs/fujinet-nio.device`,
     `C/fujinet-load-resident` and the two resident-load lines above, ahead of
     `EMU_STARTUP_PREFIX`. Give offline harnesses (`preview-adf`,
     `gallery-adf`, `pacmantests`) an opt-out.
   - A parent-repo make rule builds the loader with `-mcrt=nix13`, per item 1.
     Build the devices from the submodule
     (`make -C fujinet-nio-driver/amiga ../build/amiga/fujinet-nio.device`).
   - Rewrite `contracts/amiga-adf-bootstrap.md` first; it is the spec.
   - Give `fn_test` a pass condition that exercises the link, for example an
     exchange plus `fujibus: receive:`.
   - Commit the submodule pointer at the fix branch once it is pushed to
     `origin` (our fork). That is a PR-branch pin; see
     `docs/syncing-upstream-submodules.md`.
   - Run `make emu-test` repo-wide and fix what falls out.
2. **Correct `docs/response-to-mark-disk-device-review.md`** before Mark relies
   on it:
   - Belief #5 (Task opening `serial.device`) is now *confirmed* and worse
     than written: it crashes rather than failing.
   - Add bugs A and B.
   - Note that the loader reports success on an unopenable device.
   - Mark the `TOOL_CRT` ask as nice-to-have. `GlobVec` is still unverified.
3. **Ask the user** before opening upstream PRs (A, B) and an issue (C) on
   `markjfisher/fujinet-nio-driver`. That is public and not yet authorised.
4. Update `docs/plan-catchup-2026-09.md` and the strategic-plan disk-device row
   in the same PR as 1.

## Amiberry — installed, spike tooling in `emu/amiberry/`

Amiberry 8.3.0 is installed system-wide from the official
`amiberry_8.3.0+noble_amd64.deb`, not the Flatpak, whose sandbox would hide
the IPC socket. The user decided to keep it **alongside FS-UAE**; FS-UAE stays
the CI gate. `docs/plan-catchup-2026-09.md` item 2 has the full rationale.

`emu/amiberry/probe.sh [ADF|stop]` is a spike, not a harness. It launches
Xvfb `:98`, Amiberry, the TCP-profile `fujinet-nio` and a socat bridge, then
leaves them running. Logs go to `$AB_OUT` (default `/tmp/amiberry-probe`).
`ipc.py CMD …` sends one IPC command. `mem.py task|str|long|dump ADDR` reads
guest structures via `READ_MEM`.

Things that cost time and must not be rediscovered:

- **Force a real 68000.** `-C 68000` is not enough. The A500 model came up
  with JIT on and looped on host SIGSEGVs; one run wrote a 1.5 GB log. Pass
  `-s cpu_type=68000 -s cachesize=0 -s cpu_compatible=true
  -s cpu_24bit_addressing=true`. A JIT-looping instance ignores SIGTERM and
  holds the IPC socket, so use `pkill -9 -x amiberry`.
- **Serial must be TCP.** Amiberry rejects PTYs ("Error finding serial port"),
  so our socat PTY pair cannot be reused. Both Amiberry (`TCP://host:port`)
  and nio's `fujibus-tcp-debug` profile (`127.0.0.1:65504`) are TCP
  *servers*, and a socat `TCP:…` to `TCP:…` bridge dials both. Use a fresh
  port per run, because a stale listener in TIME_WAIT fails `bind()` with
  errno 98.
- **Never `pkill -f` with a pattern that appears in your own command line.**
  It kills the invoking shell (exit 144). `probe.sh` tracks PIDs in a file
  instead.
- **IPC facts:** the socket is `$XDG_RUNTIME_DIR/amiberry.sock`, with
  tab-separated commands; `HELP` lists them all. `READ_MEM <addr> <width>`
  returns one value per call, and `SCREENSHOT <path>` works. The guest can
  be inspected *after* a Guru: memory survives the alert.
- **Open problem:** under Amiberry, `http_get` gets `Transport error` even
  though the full 16-byte SLIP reply reached the emulator in one chunk. The
  same ADF passes under FS-UAE. Unconfirmed guess: the reply arrives unpaced
  and a 7 MHz 68000 on 1.3's `serial.device` overruns. Not blocking; FS-UAE
  is the gate.

The Guru trail and the debugging method (task struct → `tc_UserData` →
segment list at base+34 → code load address → stack return addresses → link
map) are worth rehoming into a durable `emu/` or `docs/testing.md` section
when the Amiberry mode is formalised.
