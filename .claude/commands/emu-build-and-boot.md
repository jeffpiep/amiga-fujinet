---
name: emu-build-and-boot
description: Build an Amiga app, run it in FS-UAE with fujinet-nio, and read the screenshot
argument-hint: [app-name]
allowed-tools:
  - Bash
  - Read
  - Write
  - Edit
---

# Amiga Emulator Test Workflow

Builds an Amiga app, runs it headless in FS-UAE with fujinet-nio wired via socat
PTY pair, captures a screenshot, and reports pass/fail. All orchestration is
handled by `emu/run.sh` and `emu/scripts/build-adf.sh` — invoked via `make emu-test`.

## Prerequisites

- `emu/config/paths.env` exists (copy from `paths.env.example`, set `KICKSTART_ROM` and `WB_ADF`)
- fujinet-nio built: `cd fujinet-nio && ./build.sh -p fujibus-rs232-debug`
- fujinet-nio-lib built: `cd fujinet-nio-lib && make TARGET=amiga`
- App `Makefile` includes `../../emu/template/emu.mk` and sets `APP_NAME`, `EMU_PASS_PATTERN`

## Step 1 — Build the Amiga binary

```bash
make -C apps/<appname>
file apps/<appname>/<appname>   # must say "AmigaOS loadseg()ble executable"
```

## Step 2 — Run the emulator test

`make emu-test` builds the ADF, launches Xvfb + FS-UAE + socat + fujinet-nio,
polls for the pass pattern, captures a screenshot, tears everything down, and
writes `result.json`.

```bash
make -C apps/<appname> emu-test
```

Expected output ends with:
```
screenshot: emu/logs/<appname>/<timestamp>/screenshot.png (800x600)
=== PASS (...) in Ns ===
```

## Step 3 — Find and read the screenshot

```bash
# Find the most recent run's screenshot
ls -t emu/logs/<appname>/*/screenshot.png | head -1
```

Use the `Read` tool on that path to load the image into context. The screenshot
captures the Xvfb virtual display at the moment just before teardown, showing the
final state of the Amiga CLI or Workbench screen.

## Step 4 — Interpret the screenshot and result

After reading the screenshot, describe what is visible:
- Boot output (AmigaDOS banner + Release 1.3)
- App stdout printed to the CLI
- Error dialogs ("Software error — task held", "Please insert Workbench", etc.)
- Whether the app completed successfully

Also check `result.json` for structured pass/fail, reason, and duration:

```bash
cat $(ls -t emu/logs/<appname>/*/result.json | head -1)
```

And the fujinet-nio log for FujiBus activity:

```bash
cat $(ls -t emu/logs/<appname>/*/fujinet.log | head -1)
```

---

## Adding emu-test to a new app

Run `emu/init-app.sh <appname>` to scaffold a new app with `emu.mk` wired in,
or add these lines to an existing app's `Makefile`:

```make
APP_NAME         = <appname>
APP_BINARY       = $(TARGET)
EMU_PASS_PATTERN = fujibus: receive:   # or "serial: setbaud: 19200" for init-only apps
EMU_TIMEOUT      = 60

include ../../emu/template/emu.mk
```

### Pass pattern guidance

| App behaviour | EMU_PASS_PATTERN |
|---------------|-----------------|
| Sends FujiBus traffic (fn_open, fn_read, etc.) | `fujibus: receive:` — matched in fujinet.log (line-buffered, real-time) |
| Opens serial only (fn_init + fn_is_ready, no data sent) | `serial: setbaud: 19200` — matched in FS-UAE's log after teardown |

---

## Log directory layout

Each run creates `emu/logs/<appname>/<timestamp>/`:

| File | Contents |
|------|----------|
| `screenshot.png` | Virtual display capture at end of run |
| `result.json` | Structured pass/fail, reason, duration |
| `fujinet.log` | fujinet-nio stdout (line-buffered, real-time) |
| `serial-trace.log` | socat hex dump of all serial bytes |
| `fsuae-sys.log` | FS-UAE's internal diagnostic log (flushed on exit) |
| `emulator.log` | FS-UAE stdout/stderr |
| `run.fs-uae` | Generated FS-UAE config for this run |

---

## Diagnosing failures

| Symptom | Cause | Fix |
|---------|-------|-----|
| "Please insert Workbench" screen | Missing/bad bootblock | `build-adf.sh` installs it automatically; check `WB_ADF` in paths.env |
| `serial.device` missing error on screen | `WB_ADF` path wrong or file absent | Set correct `WB_ADF` in `emu/config/paths.env` |
| FAIL reason `emulator_crash` | FS-UAE exited before timeout | Check `emulator.log` and `fsuae-sys.log` |
| FAIL reason `server_crash` | fujinet-nio exited early | Check `fujinet.log` for startup errors |
| FAIL reason `timeout`, screenshot shows CLI prompt | Pass pattern wrong for app | See pass pattern guidance above |
| FAIL reason `timeout`, screenshot shows boot screen | ADF didn't boot | Verify `bootable: True` in build-adf.sh output; check `WB_ADF` |
| FAIL reason `timeout`, screenshot is black | Xvfb or FS-UAE didn't start | Check `emulator.log`; ensure `xvfb` package is installed |
| FAIL reason `socat_pty_timeout` | socat didn't create PTYs within 10s | Check socat is installed; look for stale symlinks at /tmp/amiga-serial |
| FAIL reason `fujinet_binary_missing` | fujinet-nio not built | `cd fujinet-nio && ./build.sh -p fujibus-rs232-debug` |
| `KICKSTART_ROM` error at startup | ROM path wrong or file absent | Edit `emu/config/paths.env` |
| App crashes ("Software error — task held") | V36+ API called on KS 1.3 | Avoid `CreateMsgPort`/`CreateIORequest`; use `CreatePort`/`CreateExtIO` |
| `OpenDevice rc=-1` visible on screen | `serial.device` missing from ADF | `build-adf.sh` extracts it from `WB_ADF` automatically — check WB_ADF path |

---

## FS-UAE serial port notes

- **PTY** (`--serial_port=/dev/pts/X`) — FS-UAE emulates the UART directly.
  Guest software opens `serial.device` normally. `run.sh` always uses this mode.
- **TCP** (`serial_port = tcp://...`) — FS-UAE holds `serial.device` open inside
  the Amiga, so guest `OpenDevice` gets `IOERR_OPENFAIL (-1)`. Never use.
- `serial: ioctl TIOCMGET failed` in `fsuae-sys.log` is harmless — PTYs don't
  implement modem control lines; FS-UAE retries and continues normally.
- FS-UAE buffers its log file internally. Patterns in `fsuae-sys.log` only appear
  after FS-UAE exits. `run.sh` handles this with a post-teardown grep.

---

## Manual / Debug Fallback

Use these steps when you need to run a single stage in isolation or inspect
behaviour that the automated path obscures.

### Build ADF only

```bash
APP_NAME=<appname> APP_BINARY=apps/<appname>/<appname> \
  ADF_OUT=apps/<appname>/<appname>.adf \
  emu/scripts/build-adf.sh
```

### Run automated test with overrides

```bash
APP_NAME=<appname> \
  ADF_PATH=apps/<appname>/<appname>.adf \
  PASS_PATTERN="fujibus: receive:" \
  TIMEOUT_S=90 \
  emu/run.sh
```

### Run FS-UAE interactively on the real display

For visual debugging when you need to interact with the Amiga directly:

```bash
# Terminal 1 — socat PTY pair
socat -d -d pty,raw,echo=0,link=/tmp/amiga-serial \
           pty,raw,echo=0,link=/tmp/fn-pty \
  2>emu/logs/<appname>/serial-trace.log &

# Terminal 2 — fujinet-nio
FN_SERIAL_PORT=/tmp/fn-pty FN_SERIAL_BAUD=19200 \
  stdbuf -oL -eL fujinet-nio/build/fujibus-rs232-debug/fujinet-nio \
  >emu/logs/<appname>/fujinet.log 2>&1

# Terminal 3 — FS-UAE on real display
DISPLAY=:0 fs-uae emu/config/fsuae-test.fs-uae \
  --serial_port=$(readlink /tmp/amiga-serial)
```

Screenshot on real display (requires finding the window ID):

```bash
WIN_ID=$(DISPLAY=:0 xwininfo -root -children 2>/dev/null | grep '"FS-UAE' | awk '{print $1}')
DISPLAY=:0 xwd -id "$WIN_ID" -silent > /tmp/fsuae.xwd
python3 - <<'EOF'
import sys, struct
from PIL import Image
with open('/tmp/fsuae.xwd', 'rb') as f:
    data = f.read()
fields = struct.unpack('>20I', data[:80])
header_size, _, _, depth, w, h, _, byte_order, _, _, _, bpp, bpl = fields[:13]
ncolors = struct.unpack('>I', data[76:80])[0]
pixel_offset = header_size + ncolors * 12
pixels = data[pixel_offset:pixel_offset + bpl * h]
img = Image.frombytes('RGBA', (w, h), pixels, 'raw', 'BGRA', bpl, 1).convert('RGB')
img.save('emu/logs/<appname>/screenshot.png')
print(f"screenshot: {img.size}")
EOF
```

### Tear down manually

```bash
killall fs-uae socat fujinet-nio 2>/dev/null; rm -f /tmp/amiga-serial /tmp/fn-pty
```

### KS 1.3 compatibility

`CreateMsgPort()` and `CreateIORequest()` are exec.library V36+ (Kickstart 2.0).
On KS 1.3 they jump to a garbage LVO and crash. Use `CreatePort()` / `DeletePort()`
and `CreateExtIO()` / `DeleteExtIO()` from amiga.lib (`clib/alib_protos.h`).

Check: `grep -r "CreateMsgPort\|CreateIORequest" fujinet-nio-lib/src/`
