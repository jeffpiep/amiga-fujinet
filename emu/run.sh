#!/usr/bin/env bash
# emu/run.sh — App-agnostic Amiga emulator test runner (Phase 2 first cut)
#
# Wires socat PTY pair → fujinet-nio → FS-UAE, polls logs for pass/fail,
# writes result.json, and exits 0 on PASS / 1 on FAIL.
#
# Required env vars:
#   APP_NAME      - short app name, used for log directory (e.g. fn_test)
#   ADF_PATH      - absolute or relative path to the built .adf file
#   PASS_PATTERN  - grep pattern; match in fujinet.log or serial-trace.log → PASS
#
# Optional env vars:
#   FAIL_PATTERN  - grep pattern; match → immediate FAIL (default: none)
#   TIMEOUT_S     - seconds before declaring timeout FAIL (default: 60)
#
# Example:
#   APP_NAME=fn_test ADF_PATH=apps/fn_test/fn_test.adf \
#     PASS_PATTERN="FujiNet is ready" \
#     bash emu/run.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SCRIPTS="$SCRIPT_DIR/scripts"

# --- Validate required inputs ---
if [ -z "${APP_NAME:-}" ] || [ -z "${ADF_PATH:-}" ] || [ -z "${PASS_PATTERN:-}" ]; then
    echo "ERROR: APP_NAME, ADF_PATH, and PASS_PATTERN must be set" >&2
    exit 1
fi
if [ ! -f "$ADF_PATH" ]; then
    echo "ERROR: ADF not found: $ADF_PATH" >&2
    exit 1
fi

FAIL_PATTERN="${FAIL_PATTERN:-}"
TIMEOUT_S="${TIMEOUT_S:-60}"

# --- Source paths config ---
PATHS_ENV="$SCRIPT_DIR/config/paths.env"
if [ ! -f "$PATHS_ENV" ]; then
    echo "ERROR: $PATHS_ENV not found — copy paths.env.example and fill in paths" >&2
    exit 1
fi
# shellcheck source=/dev/null
source "$PATHS_ENV"
if [ -z "${KICKSTART_ROM:-}" ] || [ ! -f "${KICKSTART_ROM:-}" ]; then
    echo "ERROR: KICKSTART_ROM not set or file not found: ${KICKSTART_ROM:-}" >&2
    exit 1
fi

# --- Create timestamped log dir ---
TIMESTAMP=$(date -u +%Y%m%dT%H%M%S)
LOG_DIR="$SCRIPT_DIR/logs/$APP_NAME/$TIMESTAMP"
mkdir -p "$LOG_DIR"

echo "=== emu/run.sh: $APP_NAME ==="
echo "ADF:     $ADF_PATH"
echo "Logs:    $LOG_DIR"
echo "Pass:    $PASS_PATTERN"
echo "Timeout: ${TIMEOUT_S}s"

# FS-UAE writes its own diagnostic log here (independent of our log dir)
FSUAE_SYS_LOG="$HOME/Documents/FS-UAE/Cache/Logs/fs-uae.log.txt"

# --- Kill stale processes ---
killall fs-uae    2>/dev/null || true
killall socat     2>/dev/null || true
killall fujinet-nio 2>/dev/null || true
rm -f /tmp/amiga-serial /tmp/fn-pty
sleep 2   # let previous FS-UAE die fully before starting a new one

# Truncate the FS-UAE system log so grep only sees output from THIS run.
# (FS-UAE appends across runs; without truncation the pass pattern could
#  match leftover lines from a previous test.)
mkdir -p "$(dirname "$FSUAE_SYS_LOG")"
> "$FSUAE_SYS_LOG" 2>/dev/null || true

# --- Copy ADF to FS-UAE Floppies directory ---
# floppy_drive_0 must be a short filename; absolute paths are silently ignored.
FLOPPIES_DIR="$HOME/Documents/FS-UAE/Floppies"
mkdir -p "$FLOPPIES_DIR"
ADF_BASENAME=$(basename "$ADF_PATH")
cp "$ADF_PATH" "$FLOPPIES_DIR/$ADF_BASENAME"

# Remove stale FS-UAE save-disk files — they cache old ADF contents and
# cause the emulator to boot an outdated floppy image.
SAVE_DIR="$HOME/Documents/FS-UAE/Save States/run"
SDF_NAME="${ADF_BASENAME%.adf}.sdf"
if [ -f "$SAVE_DIR/$SDF_NAME" ]; then
    rm -f "$SAVE_DIR/$SDF_NAME"
fi

# --- Generate FS-UAE config for this run ---
FSUAE_CONFIG="$LOG_DIR/run.fs-uae"
cat > "$FSUAE_CONFIG" <<EOF
[fs-uae]
amiga_model = A500
kickstart_file = $KICKSTART_ROM
floppy_drive_0 = $ADF_BASENAME
console_debugger = 0
ntsc_mode = 1
# serial_port is passed on the command line as --serial_port=<PTY>
# DO NOT add serial_port here: TCP mode causes IOERR_OPENFAIL in guest software
EOF

# --- Start socat PTY pair ---
# -x enables hex dump of all bytes in both directions (goes to stderr / serial-trace.log)
# Use -x (hex), not -v (printable): extract-serial-text.sh expects hex format
socat -d -d -x \
    pty,raw,echo=0,link=/tmp/amiga-serial \
    pty,raw,echo=0,link=/tmp/fn-pty \
    2>"$LOG_DIR/serial-trace.log" &
SOCAT_PID=$!

# Wait for both PTY symlinks (socat creates them asynchronously)
echo "Waiting for socat PTYs..."
PTY_READY=0
for i in $(seq 1 10); do
    sleep 1
    if [ -L /tmp/amiga-serial ] && [ -L /tmp/fn-pty ]; then
        echo "PTYs ready after ${i}s"
        PTY_READY=1
        break
    fi
done

if [ "$PTY_READY" -eq 0 ]; then
    echo "ERROR: socat PTYs did not appear after 10s" >&2
    kill "$SOCAT_PID" 2>/dev/null || true
    INCLUDE_TAILS=1 "$SCRIPTS/write-result.sh" "$LOG_DIR" "$APP_NAME" FAIL socat_pty_timeout 10
    exit 1
fi

AMIGA_DEV=$(readlink /tmp/amiga-serial)
echo "Amiga serial PTY: $AMIGA_DEV"

# --- Launch Xvfb on a dedicated display ---
# Use a virtual framebuffer so FS-UAE has no visible window that can receive
# SDL_QUIT from user interaction or the window manager.
# (xvfb-run wraps a single command; here we need a persistent display for
#  the full test duration, so we start Xvfb directly.)
EMU_DISPLAY=:99
killall -q Xvfb 2>/dev/null || true
sleep 1
Xvfb "$EMU_DISPLAY" -screen 0 800x600x24 &
XVFB_PID=$!
sleep 1   # give Xvfb time to start

# --- Launch FS-UAE ---
DISPLAY="$EMU_DISPLAY" fs-uae "$FSUAE_CONFIG" --serial_port="$AMIGA_DEV" \
    >"$LOG_DIR/emulator.log" 2>&1 &
FSUAE_PID=$!
echo "FS-UAE PID: $FSUAE_PID"
sleep 5   # give FS-UAE time to open serial hardware before fujinet-nio connects

# --- Start fujinet-nio ---
# FN_BIN can be overridden by the caller; defaults to the build in this repo
FN_BIN="${FN_BIN:-$PROJECT_ROOT/fujinet-nio/build/fujibus-rs232-debug/fujinet-nio}"
if [ ! -x "$FN_BIN" ]; then
    echo "ERROR: fujinet-nio binary not found: $FN_BIN" >&2
    echo "Build with: cd fujinet-nio && ./build.sh -p fujibus-rs232-debug" >&2
    kill "$FSUAE_PID" "$SOCAT_PID" 2>/dev/null || true
    INCLUDE_TAILS=1 "$SCRIPTS/write-result.sh" "$LOG_DIR" "$APP_NAME" FAIL fujinet_binary_missing 0
    exit 1
fi

# stdbuf -oL forces line-buffered stdout so each log line is flushed immediately.
# Without this, fujinet-nio uses full buffering (block-buffered) when stdout is
# not a TTY — small apps like fn_test never fill the buffer, so grep never sees
# the pattern during the poll window.
FN_SERIAL_PORT=/tmp/fn-pty FN_SERIAL_BAUD=19200 \
    stdbuf -oL -eL "$FN_BIN" >"$LOG_DIR/fujinet.log" 2>&1 &
FN_PID=$!
echo "fujinet-nio PID: $FN_PID"

# --- Poll logs for PASS / FAIL ---
START_TIME=$SECONDS
RESULT=""
REASON=""

echo "Polling logs (timeout ${TIMEOUT_S}s)..."
while [ $((SECONDS - START_TIME)) -lt "$TIMEOUT_S" ]; do
    sleep 2

    # Crash detection — check /proc to distinguish live vs zombie processes
    # (kill -0 returns 0 for zombies, so we must check the state explicitly)
    _fn_state=$(grep "^State:" /proc/$FN_PID/status 2>/dev/null | awk '{print $2}')
    if [ -z "$_fn_state" ] || [ "$_fn_state" = "Z" ]; then
        RESULT=FAIL; REASON=server_crash; break
    fi
    _fsuae_state=$(grep "^State:" /proc/$FSUAE_PID/status 2>/dev/null | awk '{print $2}')
    if [ -z "$_fsuae_state" ] || [ "$_fsuae_state" = "Z" ]; then
        RESULT=FAIL; REASON=emulator_crash; break
    fi

    # FAIL pattern (fast exit on known-bad string)
    if [ -n "$FAIL_PATTERN" ]; then
        if grep -qsF "$FAIL_PATTERN" "$LOG_DIR/fujinet.log" "$LOG_DIR/serial-trace.log" 2>/dev/null; then
            RESULT=FAIL; REASON=fail_pattern; break
        fi
    fi

    # PASS pattern (check our flushed log files; FS-UAE's own log is checked
    # post-teardown because it uses buffered I/O and won't flush mid-run)
    if grep -qsF "$PASS_PATTERN" "$LOG_DIR/fujinet.log" "$LOG_DIR/serial-trace.log" 2>/dev/null; then
        RESULT=PASS
        REASON="matched '${PASS_PATTERN}' in logs"
        break
    fi
done

DURATION=$((SECONDS - START_TIME))

# --- Screenshot ---
# Capture the virtual framebuffer before killing FS-UAE. Uses xwd -root on the
# Xvfb display (no window ID needed — it's the only app on the virtual screen).
XWD_TMP="$LOG_DIR/screenshot.xwd"
SCREENSHOT="$LOG_DIR/screenshot.png"
DISPLAY="$EMU_DISPLAY" xwd -root -silent > "$XWD_TMP" 2>/dev/null && \
python3 - "$XWD_TMP" "$SCREENSHOT" <<'PYEOF'
import sys, struct
from PIL import Image

xwd_path, png_path = sys.argv[1], sys.argv[2]
with open(xwd_path, 'rb') as f:
    data = f.read()

fields = struct.unpack('>20I', data[:80])
header_size, _, _, depth, w, h, _, byte_order, _, _, _, bpp, bpl = fields[:13]
ncolors = struct.unpack('>I', data[76:80])[0]
pixel_offset = header_size + ncolors * 12
pixels = data[pixel_offset:pixel_offset + bpl * h]
img = Image.frombytes('RGBA', (w, h), pixels, 'raw', 'BGRA', bpl, 1).convert('RGB')
img.save(png_path)
print(f"screenshot: {png_path} ({w}x{h})")
PYEOF
rm -f "$XWD_TMP"

# --- Tear down ---
kill "$FN_PID" "$SOCAT_PID" "$FSUAE_PID" "$XVFB_PID" 2>/dev/null || true
# Wait for FS-UAE to fully exit and flush its internal log file before copying.
# FS-UAE buffers its log with stdio; the data only reaches disk on flush/exit.
# Patterns that only appear in the FS-UAE log (e.g. "serial: setbaud: 19200"
# for apps that don't send FujiBus traffic) require this post-teardown check.
sleep 2
cp "$FSUAE_SYS_LOG" "$LOG_DIR/fsuae-sys.log" 2>/dev/null || true

# Post-teardown pass check: re-run the grep against the now-flushed logs.
# This catches patterns that FS-UAE buffered internally during the test.
if [ -z "$RESULT" ] || [ "$REASON" = "timeout" ]; then
    if grep -qsF "$PASS_PATTERN" \
            "$LOG_DIR/fujinet.log" \
            "$LOG_DIR/serial-trace.log" \
            "$LOG_DIR/fsuae-sys.log" 2>/dev/null; then
        RESULT=PASS
        REASON="matched '${PASS_PATTERN}' in post-teardown logs"
    fi
fi

if [ -z "$RESULT" ]; then
    RESULT=FAIL
    REASON=timeout
fi

# --- Write result.json ---
if [ "$REASON" = "timeout" ] || [ "$REASON" = "emulator_crash" ] || [ "$REASON" = "server_crash" ]; then
    INCLUDE_TAILS=1 "$SCRIPTS/write-result.sh" "$LOG_DIR" "$APP_NAME" "$RESULT" "$REASON" "$DURATION"
else
    "$SCRIPTS/write-result.sh" "$LOG_DIR" "$APP_NAME" "$RESULT" "$REASON" "$DURATION"
fi

echo "=== $RESULT ($REASON) in ${DURATION}s ==="

[ "$RESULT" = "PASS" ] && exit 0 || exit 1
