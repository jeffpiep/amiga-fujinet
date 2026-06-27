---
name: emu-build-and-boot
description: Build an Amiga app, create a bootable ADF, and boot it in FS-UAE with serial connected to fujinet-nio
argument-hint: [app-name]
allowed-tools:
  - Bash
  - Read
  - Write
  - Edit
---

# Amiga Build → ADF → FS-UAE Boot Workflow

Complete workflow for building an Amiga program, packaging it as a bootable ADF floppy, and running it in FS-UAE with the serial port bridged to fujinet-nio.

## Step 1 — Build the Amiga binary

```bash
make -C apps/<appname>
file apps/<appname>/<appname>        # must say "AmigaOS loadseg()ble executable"
```

## Step 2 — Create and populate the ADF

```bash
# Create OFS floppy image
xdftool apps/<appname>/<appname>.adf create + format "<LABEL>" ofs

# Add startup-sequence (s/ directory)
xdftool apps/<appname>/<appname>.adf makedir s
printf '<appname>\n' > /tmp/startup-sequence
xdftool apps/<appname>/<appname>.adf write /tmp/startup-sequence s/startup-sequence

# Add the binary
xdftool apps/<appname>/<appname>.adf write apps/<appname>/<appname> <appname>

# Verify contents
xdftool apps/<appname>/<appname>.adf list
```

**If your app uses `serial.device` (e.g. via `fn_init()`), you must include it on the boot disk.**
`serial.device` is NOT ROM-resident in KS 1.3 — it must be loaded from `DEVS:serial.device`.
Without it, `OpenDevice("serial.device", ...)` returns `IOERR_OPENFAIL (-1)`.

```bash
# Extract serial.device from Workbench 1.3.4 ADF (one-time)
xdftool /home/jeffp/Downloads/amiga-os-134-workbench.adf read DEVS/serial.device /tmp/serial.device

# Add to your boot ADF
xdftool apps/<appname>/<appname>.adf makedir Devs
xdftool apps/<appname>/<appname>.adf write /tmp/serial.device Devs/serial.device
```

**Do NOT redirect stdout to SER: in the startup-sequence.** If the program uses
serial.device directly (e.g. via fn_init()), `>SER:` causes a device conflict —
two openers of serial.device, causing a crash. Also, `2>>SER:` is not supported
by KS 1.3's shell.

## Step 3 — Install the bootblock

This step is critical. xdftool formats disks with a zero checksum (non-bootable).
Without this step, the Amiga shows the "Please insert Workbench" screen instead
of running the startup-sequence.

```bash
xdftool apps/<appname>/<appname>.adf boot install
xdftool apps/<appname>/<appname>.adf boot show   # verify: bootable: True
```

## Step 4 — Place ADF in FS-UAE's Floppies directory

FS-UAE's `floppy_drive_0` option resolves short filenames against its Floppies
directory. Absolute paths are silently dropped. Always copy the ADF here.

```bash
mkdir -p ~/Documents/FS-UAE/Floppies
cp apps/<appname>/<appname>.adf ~/Documents/FS-UAE/Floppies/
```

## Step 5 — FS-UAE config

`emu/config/fsuae-test.fs-uae` does NOT include `serial_port` — that is passed
on the command line using the PTY device path (see Step 7).

```ini
[fs-uae]
amiga_model = A500
kickstart_file = /home/jeffp/Downloads/amiga-os-130.rom
floppy_drive_0 = <appname>.adf
console_debugger = 0
```

**Do NOT use `serial_port = tcp://localhost:1234` in the config.** TCP serial
mode causes FS-UAE to hold `serial.device` open inside the Amiga, so guest
software gets `IOERR_OPENFAIL (-1)` when it tries to open the device. Use a
PTY pair instead (Step 7).

## Step 6 — Kill any stale processes

**Use `killall`, NOT `pkill -f`.** `pkill -f fs-uae` matches the bash eval
string in our shell and kills the running bash process (exit 144).

```bash
killall fs-uae 2>/dev/null
killall socat 2>/dev/null
killall fujinet-nio 2>/dev/null
rm -f /tmp/amiga-serial /tmp/fn-pty
sleep 1
```

## Step 7 — Create PTY pair and launch FS-UAE

Create a socat PTY pair: one end is the Amiga's virtual serial port, the other
connects to fujinet-nio. Pass the Amiga-side PTY to FS-UAE via `--serial_port`.

**CRITICAL: Do NOT use `serial_port = tcp://...`** — it prevents guest software
from opening `serial.device` (IOERR_OPENFAIL). The PTY approach emulates the
serial hardware directly with no OS-level interference.

```bash
mkdir -p emu/logs/<appname>

# Create PTY pair; socat creates symlinks at the stable paths below
socat -d -d pty,raw,echo=0,link=/tmp/amiga-serial \
           pty,raw,echo=0,link=/tmp/fn-pty \
  2>emu/logs/<appname>/serial-trace.log &
SOCAT_PID=$!

# Wait for both PTY symlinks
for i in $(seq 1 10); do
  sleep 1
  [ -L /tmp/amiga-serial ] && [ -L /tmp/fn-pty ] && { echo "PTYs ready after ${i}s"; break; }
done

AMIGA_DEV=$(readlink /tmp/amiga-serial)
echo "Amiga serial PTY: $AMIGA_DEV"

# Launch FS-UAE with the Amiga-side PTY
DISPLAY=:0 fs-uae emu/config/fsuae-test.fs-uae --serial_port="$AMIGA_DEV" \
  >/dev/null 2>&1 &
FSUAE_PID=$!
echo "FS-UAE PID: $FSUAE_PID"
sleep 5   # give FS-UAE time to initialize
```

## Step 8 — Start fujinet-nio

```bash
# Run fujinet-nio on the fn-pty side
FN_SERIAL_PORT=/tmp/fn-pty FN_SERIAL_BAUD=19200 \
  fujinet-nio/build/fujibus-rs232-debug/fujinet-nio \
  >emu/logs/<appname>/fujinet.log 2>&1 &
FN_PID=$!
echo "fujinet-nio PID: $FN_PID"
```

## Step 9 — Wait and collect results

```bash
sleep 60   # A500 at 7MHz is slow to boot from floppy; allow plenty of time

echo "=== fujinet log ===" && cat emu/logs/<appname>/fujinet.log
echo "=== serial trace ===" && cat emu/logs/<appname>/serial-trace.log
```

## Step 10 — Capture emulator screenshot

Capture the FS-UAE window by its X11 window ID using `xwd`, then decode the
XWD format with PIL. This captures the window regardless of what is on top of it.

The session runs under X11 (not Wayland), so `xwd -id` works reliably.

```bash
# Find the FS-UAE window ID and capture it
WIN_ID=$(DISPLAY=:0 xwininfo -root -children 2>/dev/null | grep '"FS-UAE' | awk '{print $1}')
echo "FS-UAE window: $WIN_ID"
DISPLAY=:0 xwd -id "$WIN_ID" -silent > /tmp/fsuae.xwd

# Decode XWD to PNG using PIL
python3 - <<'EOF'
import struct
from PIL import Image

with open('/tmp/fsuae.xwd', 'rb') as f:
    data = f.read()

fields = struct.unpack('>20I', data[:80])
header_size, _, _, depth, w, h, _, byte_order, _, _, _, bpp, bpl = fields[:13]
ncolors = struct.unpack('>I', data[76:80])[0]
cmap_size = ncolors * 12
pixel_offset = header_size + cmap_size
pixels = data[pixel_offset:pixel_offset + bpl * h]

img = Image.frombytes('RGBA', (w, h), pixels, 'raw', 'BGRA', bpl, 1)
img = img.convert('RGB')
img.save('emu/logs/<appname>/screenshot.png')
print(f"screenshot saved: {img.size}")
EOF
```

## Step 11 — Read screenshot into Claude Code

Use the `Read` tool on `emu/logs/<appname>/screenshot.png`.

Claude Code's `Read` tool supports PNG images — this feeds the screenshot into
Claude's vision context. After reading, describe what is visible on the Amiga
screen: boot output, program results, error dialogs, or the "Please insert
Workbench" screen. State whether the app appears to have run successfully.

## Step 12 — Tear down processes

```bash
kill $FN_PID $SOCAT_PID $FSUAE_PID 2>/dev/null
```

## Diagnosing failures

| Symptom | Cause | Fix |
|---------|-------|-----|
| "Please insert Workbench" screen | Missing/bad bootblock | `xdftool boot install` |
| CLI redirection error on boot | `2>>SER:` in startup-sequence | Remove stderr redirect |
| "Software error — task held" | App crashes (often V36+ API on KS 1.3) | See OS version notes below |
| `OpenDevice rc=-1 io_Error=-1` | `serial.device` missing from boot disk (not ROM-resident in KS 1.3) | Extract from WB 1.3.4 ADF → add as `Devs/serial.device` on boot ADF |
| Empty serial trace | fujinet-nio not started before fn_test ran | Check PTY exists; increase boot wait |
| `pkill -f fs-uae` kills bash | Pattern matches bash eval string | Use `killall fs-uae` |
| `xvfb-run` fails silently | Background session can't init Xvfb | Use `DISPLAY=:0` |
| `floppy_drive_0` shows null in log | Absolute path silently dropped | Use short filename + copy to Floppies dir |
| `xwd -id` returns empty/black | FS-UAE window not found or ID stale | Re-query `xwininfo -root -children` for current ID |

## KS 1.3 compatibility notes

`CreateMsgPort()` and `CreateIORequest()` are **exec.library V36+ functions**
(Kickstart 2.0). Calling them on KS 1.3 jumps to a garbage LVO and crashes.

KS 1.3-compatible alternatives: use `CreatePort()` / `DeletePort()` and
`CreateExtIO()` / `DeleteExtIO()` from amiga.lib (clib/alib_protos.h).

Check with: `grep -r "CreateMsgPort\|CreateIORequest" fujinet-nio-lib/src/`

## FS-UAE serial port mode notes

- `serial_port = tcp://host:port` — FS-UAE holds `serial.device` open inside
  the Amiga. Guest software calling `OpenDevice("serial.device", ...)` gets
  `IOERR_OPENFAIL (-1)`. **Do not use with software that opens serial.device.**
- `serial_port = /dev/pts/X` (PTY) — FS-UAE emulates the serial UART hardware
  and routes it to the PTY. Guest software can open `serial.device` normally.
  Use a socat PTY pair to bridge to fujinet-nio.
- FS-UAE log caps at 40KB (810 lines). SERIAL events may be absent even on
  success — check `io_Error` in diagnostic output on the Amiga screen instead.
