# Amiga Emulator Automation — Plan

> **Status: ARCHIVED — superseded 2026-07-05.** The plan shipped as the `emu/`
> harness and the `emu-build-and-boot` skill (`.claude/commands/emu-build-and-boot.md`),
> which owns the debug workflow and log inventory. Tool install prerequisites
> were rehomed to CLAUDE.md's Emulator Testing section. Phase 4 (CI) was never
> executed; the current position on CI is in `docs/audit-build-workflow.md` (B5).

End goal: a reusable workflow that builds any Amiga app, boots it in an emulator
with emulated serial connected to fujinet-nio, captures all I/O, and returns
structured pass/fail output that Claude Code (or CI) can inspect.

---

## 1. Tool Inventory

### Required — emulator & serial bridge

| Tool | Package | Purpose |
|------|---------|---------|
| `fs-uae` | `fs-uae` | Amiga emulator, Linux-native, TCP serial support |
| `socat` | `socat` | Bridge PTY ↔ TCP so fujinet-nio sees a normal serial port |
| `xvfb-run` | `xvfb` | Virtual framebuffer — lets FS-UAE run headless |

### Required — Amiga disk tools

| Tool | Package | Purpose |
|------|---------|---------|
| `xdftool` | `amitools` (pip) | Create/populate ADF floppy images |
| `amitools` | `amitools` (pip) | Broader Amiga toolchain (hunk inspection, etc.) |

### Optional — diagnostics & capture

| Tool | Package | Purpose |
|------|---------|---------|
| `tee` | coreutils | Split log streams to file + stdout simultaneously |
| `socat -v` flag | socat | Hex-dump raw serial bytes to stderr |
| `timeout` | coreutils | Kill hung emulator runs after N seconds |
| `jq` | `jq` | Format structured JSON result files |
| `file` | binutils | Verify m68k binary before packaging into ADF |
| `nm` / `objdump` | binutils-m68k-linux-gnu | Inspect Amiga binary symbols / disassembly |

### Installation checklist

- [X ] `sudo apt install fs-uae socat xvfb jq`
- [X ] `pip3 install amitools`
- [ X] Verify: `fs-uae --version`
- [X ] Verify: `socat -V`
- [X ] Verify: `xdftool --version`
- [ ] Kickstart 1.3 ROM available (confirm path, record in `emu/config/paths.env`)

---

## 2. Architecture

```
┌─────────────────────────────────┐
│  make emu-test  (per-app target) │
└──────────────┬──────────────────┘
               │
   ┌───────────▼───────────┐
   │  emu/run.sh           │  top-level runner (reusable)
   │  • builds ADF         │
   │  • starts socat       │
   │  • starts fujinet-nio │
   │  • starts FS-UAE      │
   │  • waits / captures   │
   │  • writes result.json │
   └───────────┬───────────┘
               │
   ┌───────────▼──────────────────────────────────┐
   │  Processes (all captured to emu/logs/)        │
   │                                               │
   │  FS-UAE ──serial TCP:1234──> socat            │
   │                                  │            │
   │                               PTY /tmp/fn-pty │
   │                                  │            │
   │                           fujinet-nio         │
   │                           (stdout → log)      │
   └───────────────────────────────────────────────┘
```

**Log files written per run** (under `emu/logs/<app>/<timestamp>/`):

| File | Contents |
|------|---------|
| `fujinet.log` | fujinet-nio stdout — FujiBus events, errors |
| `serial-trace.log` | socat `-v` hex dump — every byte on the wire |
| `emulator.log` | FS-UAE stdout/stderr — boot messages, crashes |
| `result.json` | Structured PASS/FAIL with exit reason and log paths |

---

## 3. Reusable Directory Layout

```
emu/
  run.sh              # main runner (app-agnostic)
  init-app.sh         # scaffold: copies template into apps/<name>/
  config/
    paths.env         # KICKSTART_ROM, FSUAE_BIN, etc. — gitignored
    paths.env.example # committed template
  template/
    emu.mk            # Makefile fragment — include in any app's Makefile
    fsuae.fs-uae.in   # FS-UAE config template (variables substituted by run.sh)
    startup-sequence.in # AmigaOS startup-sequence template
  logs/               # gitignored, written at runtime
```

`apps/<name>/Makefile` includes `../../emu/template/emu.mk` to get:
- `make emu-test` — full emulated run
- `make emu-adf`  — just build the ADF without running
- `make emu-clean` — remove ADF and logs

---

## 4. Phase 1 — Prototype with fn_test

Goal: manually wire up the full loop once to prove each piece works.

### 4.1 Install tools

- [ ] Complete tool inventory checklist (Section 1)
- [ ] Record `KICKSTART_ROM` path in `emu/config/paths.env`

### 4.2 Verify FS-UAE serial TCP

- [ ] Write a minimal `fsuae-test.fs-uae`:
  ```ini
  [fs-uae]
  amiga_model = A500
  kickstart_file = /path/to/kick13.rom
  floppy_drive_0 = fn_test.adf
  serial_port = tcp://localhost:1234
  ```
- [ ] Boot FS-UAE with a blank ADF — confirm it reaches the Workbench/CLI
  (proves Kickstart loads)
- [ ] Run `socat -v PTY,link=/tmp/fn-pty,rawer TCP-LISTEN:1234,reuseaddr`
  in one terminal, boot FS-UAE in another, type in Amiga CLI — verify
  characters appear in socat hex dump

### 4.3 Build fn_test ADF

- [ ] `make -C apps/fn_test`
- [ ] `file apps/fn_test/fn_test` → must say m68k
- [ ] Create ADF with auto-run startup-sequence:
  ```bash
  xdftool fn_test.adf create + format "FNTEST" ofs
  xdftool fn_test.adf mkdir s
  echo "fn_test >SER: 2>>SER:" | xdftool fn_test.adf write - s/startup-sequence
  xdftool fn_test.adf write apps/fn_test/fn_test
  xdftool fn_test.adf list
  ```
  > Note: redirecting stdout to `SER:` is the key I/O trick — Amiga CLI
  > output goes to the serial device, which socat captures on the Linux side.

### 4.4 Wire up fujinet-nio

- [ ] Start socat bridge:
  ```bash
  socat -v PTY,link=/tmp/fn-pty,rawer \
    TCP-LISTEN:1234,reuseaddr \
    2>emu/logs/fn_test/serial-trace.log &
  ```
- [ ] Start fujinet-nio on PTY:
  ```bash
  FN_SERIAL_PORT=/tmp/fn-pty FN_SERIAL_BAUD=19200 \
    fujinet-nio/build/fujibus-rs232-debug/fujinet-nio \
    > emu/logs/fn_test/fujinet.log 2>&1 &
  ```

### 4.5 Boot and capture

- [ ] Launch FS-UAE headless:
  ```bash
  xvfb-run fs-uae fsuae-test.fs-uae \
    > emu/logs/fn_test/emulator.log 2>&1 &
  ```
- [ ] Wait for `FujiNet is ready` or timeout (30 s):
  ```bash
  timeout 30 grep -m1 "FujiNet is ready" <(tail -f emu/logs/fn_test/fujinet.log)
  ```
- [ ] Inspect all three log files manually
- [ ] Kill processes, assess what worked / what didn't

### 4.6 I/O inspection by Claude Code

The logs written above can be read directly by Claude Code at any point:

- `cat emu/logs/fn_test/fujinet.log` — did the server see FujiBus traffic?
- `cat emu/logs/fn_test/serial-trace.log` — what bytes crossed the wire?
- `cat emu/logs/fn_test/emulator.log` — did FS-UAE crash? Did Kickstart boot?
- `cat emu/logs/fn_test/result.json` — structured summary (Phase 3 adds this)

Claude Code can be asked to read these files, identify the failure point, and
suggest the next debug step — without a human reading raw hex output.

---

## 5. Phase 2 — I/O Capture & Structured Results

Goal: make pass/fail machine-readable and give Claude Code enough signal to
diagnose failures without human interpretation.

### 5.1 Amiga → Linux output channel

The cleanest way to get Amiga `printf` output back to Linux over serial:

**Option A — redirect to SER: in startup-sequence** (simplest)
```
fn_test >SER: 2>>SER:
```
socat logs every byte; `fujinet.log` may not see app-level text (it's the
FujiBus server log). Need a second socat instance or log splitter.

**Option B — socat tee to file**
```bash
socat -v PTY,link=/tmp/fn-pty,rawer \
  "TCP-LISTEN:1234,reuseaddr,fork" \
  2>&1 | tee emu/logs/fn_test/serial-trace.log
```
The `-v` hex dump contains all bytes in both directions; extract ASCII with
`strings` or a small awk script.

**Option C — dedicated output port** (most robust, future use)
Run a second socat on TCP:1235 purely for Amiga `printf` output. App writes
to a second serial unit or to a named pipe. More plumbing but cleaner
separation of protocol traffic vs. diagnostic output.

**Recommended for prototype:** Option B (one socat, `-v` dump, parse ASCII later).
Revisit Option C when apps grow complex enough to need it.

- [ ] Decide on output channel option (recommend B to start)
- [ ] Write `emu/scripts/extract-serial-text.sh` — strips socat `-v` hex
  headers and prints printable ASCII lines

### 5.2 result.json schema

```json
{
  "app": "fn_test",
  "timestamp": "2026-06-15T02:00:00Z",
  "result": "PASS",
  "reason": "matched 'FujiNet is ready' in fujinet.log",
  "duration_s": 12,
  "logs": {
    "fujinet": "emu/logs/fn_test/20260615T020000/fujinet.log",
    "serial_trace": "emu/logs/fn_test/20260615T020000/serial-trace.log",
    "emulator": "emu/logs/fn_test/20260615T020000/emulator.log"
  }
}
```

- [ ] Write `emu/scripts/write-result.sh` — takes app name, PASS/FAIL,
  reason string; writes `result.json` to the timestamped log dir
- [ ] Each app's `emu.mk` sets `EMU_PASS_PATTERN` and `EMU_FAIL_PATTERN`
  (grep strings) used by `run.sh` to determine result

### 5.3 Timeout & crash detection

- [ ] `run.sh` uses `timeout 60 ...` around the FS-UAE launch
- [ ] Detect FS-UAE crash (non-zero exit before timeout) → FAIL with reason
  `"emulator_crash"`
- [ ] Detect fujinet-nio crash → FAIL with reason `"server_crash"`
- [ ] Detect timeout → FAIL with reason `"timeout"`, attach last 20 lines of
  each log to result.json

---

## 6. Phase 3 — Reusable Scaffold

Goal: `emu/init-app.sh <appname>` generates everything needed for a new app.

### 6.1 `emu/run.sh` (app-agnostic runner)

Accepts: `APP_NAME`, `APP_BINARY`, `ADF_PATH`, `PASS_PATTERN`, `FAIL_PATTERN`,
`TIMEOUT_S` as env vars or CLI args.

Steps (all in one script):
1. Source `emu/config/paths.env`
2. Create timestamped log dir
3. Generate FS-UAE config from `fsuae.fs-uae.in` template
4. Start socat; wait for PTY to appear
5. Start fujinet-nio on PTY
6. Boot FS-UAE under xvfb-run
7. Poll fujinet.log + serial-trace.log for PASS/FAIL patterns
8. On match or timeout: kill all background processes, write result.json
9. Exit 0 on PASS, 1 on FAIL (CI-friendly)

- [ ] Write `emu/run.sh` (prototype in bash, ~100 lines)
- [ ] Test with fn_test manually: `APP_NAME=fn_test APP_BINARY=fn_test \
  PASS_PATTERN="FujiNet is ready" bash emu/run.sh`

### 6.2 `emu/template/emu.mk` — Makefile fragment

```make
# Include from any app Makefile:
#   include ../../emu/template/emu.mk

EMU_DIR     := $(dir $(lastword $(MAKEFILE_LIST)))../..
EMU_PASS    ?= PASS
EMU_FAIL    ?= FAIL
EMU_TIMEOUT ?= 60

emu-adf: $(APP_BINARY)
	$(EMU_DIR)/emu/scripts/build-adf.sh $(APP_BINARY) $(APP_NAME)

emu-test: emu-adf
	APP_NAME=$(APP_NAME) APP_BINARY=$(APP_BINARY) \
	PASS_PATTERN="$(EMU_PASS_PATTERN)" \
	FAIL_PATTERN="$(EMU_FAIL_PATTERN)" \
	TIMEOUT_S=$(EMU_TIMEOUT) \
	$(EMU_DIR)/emu/run.sh

emu-clean:
	rm -f $(APP_NAME).adf
	rm -rf $(EMU_DIR)/emu/logs/$(APP_NAME)

.PHONY: emu-adf emu-test emu-clean
```

- [ ] Write `emu/template/emu.mk`
- [ ] Add `include ../../emu/template/emu.mk` to `apps/fn_test/Makefile`
- [ ] Add required variables (`APP_NAME`, `APP_BINARY`, `EMU_PASS_PATTERN`)
  to `apps/fn_test/Makefile`
- [ ] Verify `make emu-test` from `apps/fn_test/` works end-to-end

### 6.3 `emu/init-app.sh` — new app scaffold

```bash
emu/init-app.sh <appname>
```

Creates:
- `apps/<appname>/Makefile` — with emu.mk include and TODO stubs
- `apps/<appname>/main.c` — minimal Amiga C with fn_init() call
- No ADF or logs (generated at build time)

- [ ] Write `emu/init-app.sh`
- [ ] Test: `bash emu/init-app.sh hello_world` → `apps/hello_world/` created
- [ ] `make -C apps/hello_world emu-test` → runs through full pipeline

---

## 7. Phase 4 — CI Integration (Future)

Once Phase 3 is stable, wire into GitHub Actions or a local test runner.

- [ ] `.github/workflows/emu-test.yml` — runs `make emu-test` for each app
- [ ] Cache the Kickstart ROM path via GitHub secret (ROM is copyrighted;
  don't commit it)
- [ ] Use `ubuntu-latest` runner; `apt install fs-uae socat xvfb`
- [ ] Artifact upload: attach `result.json` and log files to each run
- [ ] Badge: parse `result.json` → update README pass/fail badge

---

## 8. Debug Workflow for Claude Code

When a test fails, Claude Code should be able to diagnose without human help:

1. Read `result.json` → identify reason (`timeout`, `emulator_crash`, etc.)
2. Based on reason, read the relevant log:
   - `emulator_crash` → tail `emulator.log` for FS-UAE error
   - `timeout` → read `fujinet.log` for last received event; read
     `serial-trace.log` for last bytes received
   - `server_crash` → tail `fujinet.log` for stack trace or error
3. Read `serial-trace.log` through `extract-serial-text.sh` to see what the
   Amiga actually printed before dying
4. If serial trace is empty → Amiga didn't send anything → likely startup-sequence
   error; inspect ADF contents with `xdftool <adf> list`
5. If serial trace has data but fujinet-nio didn't respond → protocol mismatch;
   compare bytes against `contracts/fujibus-protocol.md`

This flow means Claude Code can be asked: *"the last emu-test failed, figure out
why"* and can work through it from logs alone.

---

## 9. Execution Order

```
Phase 1  → prove the hardware emulation works (manual, ~1 session)
Phase 2  → add structured capture (run.sh first cut, ~1 session)
Phase 3a → emu.mk + fn_test integration (make emu-test works)
Phase 3b → init-app.sh scaffold
Phase 4  → CI (after at least two apps use the workflow)
```

Start with Phase 1 steps sequentially. Each has a checkbox. Do not advance to
Phase 2 until all Phase 1 checkboxes are done.
