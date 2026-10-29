#!/usr/bin/env bash
# emu/play.sh — Interactive counterpart to emu/run.sh.
#
# Same wiring (socat PTY pair → fujinet-nio → FS-UAE), but on a *visible*
# display with no pass/fail polling: FS-UAE runs until you quit it, then this
# script tears down the server and the PTY bridge and tells you where the logs
# are. Use it for the manual verification steps the headless harness cannot do
# — anything that needs typing, e.g. joining a fujitzee table and playing a
# hand to the end.
#
# Required env vars:
#   APP_NAME   - short app name, used for the log directory (e.g. fujitzee)
#   ADF_PATH   - path to the built .adf file
#
# Optional env vars:
#   DISPLAY    - X display to open the window on (default: :0)
#   FN_BIN     - fujinet-nio binary (default: the fujibus-rs232-debug build)
#   NO_SERVER  - set to 1 to skip fujinet-nio entirely (offline harnesses like
#                fujitzee's boardpreview, which never touch the serial port)
#
# Example:
#   make -C apps/fujitzee/amiga emu-adf
#   APP_NAME=fujitzee ADF_PATH=apps/fujitzee/amiga/fujitzee.adf bash emu/play.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# --- Validate required inputs ---
if [ -z "${APP_NAME:-}" ] || [ -z "${ADF_PATH:-}" ]; then
    echo "ERROR: APP_NAME and ADF_PATH must be set" >&2
    exit 1
fi
if [ ! -f "$ADF_PATH" ]; then
    echo "ERROR: ADF not found: $ADF_PATH" >&2
    exit 1
fi

NO_SERVER="${NO_SERVER:-0}"

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

# An interactive session needs a real X display. run.sh uses Xvfb precisely so
# no one can interact with it; here the opposite is the point.
PLAY_DISPLAY="${DISPLAY:-:0}"
if ! DISPLAY="$PLAY_DISPLAY" xdpyinfo >/dev/null 2>&1; then
    echo "ERROR: no usable X display at '$PLAY_DISPLAY'." >&2
    echo "       Run this from a desktop session, or set DISPLAY explicitly." >&2
    exit 1
fi

TIMESTAMP=$(date -u +%Y%m%dT%H%M%S)
LOG_DIR="$SCRIPT_DIR/logs/$APP_NAME/play-$TIMESTAMP"
mkdir -p "$LOG_DIR"

FSUAE_SYS_LOG="$HOME/Documents/FS-UAE/Cache/Logs/fs-uae.log.txt"

echo "=== emu/play.sh: $APP_NAME (interactive) ==="
echo "ADF:     $ADF_PATH"
echo "Display: $PLAY_DISPLAY"
echo "Logs:    $LOG_DIR"
echo "Server:  $([ "$NO_SERVER" = 1 ] && echo 'skipped (NO_SERVER=1)' || echo 'fujinet-nio')"

# --- Kill stale processes ---
killall fs-uae      2>/dev/null || true
killall socat       2>/dev/null || true
killall fujinet-nio 2>/dev/null || true
rm -f /tmp/amiga-serial /tmp/fn-pty
sleep 2

mkdir -p "$(dirname "$FSUAE_SYS_LOG")"
> "$FSUAE_SYS_LOG" 2>/dev/null || true

# --- Copy ADF to FS-UAE Floppies directory ---
# floppy_drive_0 must be a short filename; absolute paths are silently ignored.
FLOPPIES_DIR="$HOME/Documents/FS-UAE/Floppies"
mkdir -p "$FLOPPIES_DIR"
ADF_BASENAME=$(basename "$ADF_PATH")
cp "$ADF_PATH" "$FLOPPIES_DIR/$ADF_BASENAME"

# Stale save-disk files cache old ADF contents and boot an outdated image.
SAVE_DIR="$HOME/Documents/FS-UAE/Save States/run"
SDF_NAME="${ADF_BASENAME%.adf}.sdf"
rm -f "$SAVE_DIR/$SDF_NAME"

# --- Generate FS-UAE config for this run ---
FSUAE_CONFIG="$LOG_DIR/play.fs-uae"
cat > "$FSUAE_CONFIG" <<EOF
[fs-uae]
amiga_model = A500
kickstart_file = $KICKSTART_ROM
floppy_drive_0 = $ADF_BASENAME
console_debugger = 0
ntsc_mode = 1
fullscreen = 0
# serial_port is passed on the command line as --serial_port=<PTY>
# DO NOT add serial_port here: TCP mode causes IOERR_OPENFAIL in guest software
EOF

SOCAT_PID=""
FN_PID=""
FSUAE_PID=""

cleanup() {
    echo
    echo "Tearing down..."
    kill $FN_PID $SOCAT_PID $FSUAE_PID 2>/dev/null || true
    sleep 1
    cp "$FSUAE_SYS_LOG" "$LOG_DIR/fsuae-sys.log" 2>/dev/null || true
    echo "Logs: $LOG_DIR"
}
trap cleanup EXIT INT TERM

if [ "$NO_SERVER" != 1 ]; then
    # --- Start socat PTY pair ---
    socat -d -d -x \
        pty,raw,echo=0,link=/tmp/amiga-serial \
        pty,raw,echo=0,link=/tmp/fn-pty \
        2>"$LOG_DIR/serial-trace.log" &
    SOCAT_PID=$!

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
        exit 1
    fi

    AMIGA_DEV=$(readlink /tmp/amiga-serial)
    echo "Amiga serial PTY: $AMIGA_DEV"
    SERIAL_ARG=(--serial_port="$AMIGA_DEV")
else
    SERIAL_ARG=()
fi

# --- Launch FS-UAE (visible) ---
DISPLAY="$PLAY_DISPLAY" fs-uae "$FSUAE_CONFIG" "${SERIAL_ARG[@]}" \
    >"$LOG_DIR/emulator.log" 2>&1 &
FSUAE_PID=$!
echo "FS-UAE PID: $FSUAE_PID"

if [ "$NO_SERVER" != 1 ]; then
    sleep 5   # let FS-UAE open the serial hardware before the server connects

    FN_BIN="${FN_BIN:-$PROJECT_ROOT/fujinet-nio/build/fujibus-rs232-debug/fujinet-nio}"
    if [ ! -x "$FN_BIN" ]; then
        echo "ERROR: fujinet-nio binary not found: $FN_BIN" >&2
        echo "Build with: cd fujinet-nio && ./build.sh -p fujibus-rs232-debug" >&2
        exit 1
    fi

    # stdbuf -oL: without it fujinet-nio block-buffers stdout when it is not a
    # TTY, so `tail -f` on the log shows nothing until the buffer fills.
    FN_SERIAL_PORT=/tmp/fn-pty FN_SERIAL_BAUD=19200 \
        stdbuf -oL -eL "$FN_BIN" >"$LOG_DIR/fujinet.log" 2>&1 &
    FN_PID=$!
    echo "fujinet-nio PID: $FN_PID"
    echo
    echo "Follow the server live with:"
    echo "  tail -f $LOG_DIR/fujinet.log"
fi

echo
echo "FS-UAE window is open. Quit the emulator when done (or Ctrl-C here)."
wait "$FSUAE_PID" 2>/dev/null || true
