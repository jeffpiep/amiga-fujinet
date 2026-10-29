#!/usr/bin/env bash
# emu/drive.sh — Headless *scripted* emulator session.
#
# The third emulator mode, between run.sh and play.sh:
#
#   run.sh    headless, boots and greps a pass pattern. No input. CI.
#   play.sh   visible window, you drive it by hand. Needs a desktop.
#   drive.sh  headless, but types a scripted key sequence and captures
#             pixel-exact screenshots along the way. Works over SSH.
#
# This is what makes the "manual" T2 checks reachable from a remote CLI:
# menu walks, screen-by-screen art review, and any flow that needs a keyboard.
#
# Required env vars:
#   APP_NAME   - short app name, used for the log directory
#   ADF_PATH   - path to the built .adf file
#   KEYS       - space-separated token script (see below)
#
# Optional env vars:
#   BOOT_WAIT  - seconds to wait after launch before the first token (default 25)
#   NO_SERVER  - 1 to skip fujinet-nio (offline harnesses)
#   FN_BIN     - fujinet-nio binary override
#   JOYSTICK   - 1 to put a keyboard-driven joystick in the game port, so a
#                scripted run can test stick input: the arrow keysyms then
#                move the *stick* (and Control_R fires) instead of reaching
#                the emulated keyboard. Default 0.
#   AUDIO_WAV  - path to capture emulated audio to as a .wav, so a headless
#                run can check that a sound effect was audible and not just
#                that the code path ran. Mutes FS-UAE's floppy-drive samples
#                for the duration. See emu/checkaudio.py.
#
# KEYS tokens are emukey.py keysyms ('space', 'Escape', 's', 'Up', combos
# with '+'), plus:
#   sleepN       pause N tenths of a second (emukey.py builtin)
#   shot:<label> capture FS-UAE's internal screenshot as <label>.png
#
# Screenshots use FS-UAE's own F12+S dump, not xwd — xwd under llvmpipe
# scrambles color channels on saturated colors (see
# .claude/commands/emu-build-and-boot.md). These are the real Amiga frame.
#
# Example:
#   APP_NAME=fujitzee ADF_PATH=apps/fujitzee/amiga/fujitzee.adf \
#     KEYS="shot:welcome space sleep20 shot:lobby" bash emu/drive.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ -z "${APP_NAME:-}" ] || [ -z "${ADF_PATH:-}" ] || [ -z "${KEYS:-}" ]; then
    echo "ERROR: APP_NAME, ADF_PATH and KEYS must be set" >&2
    exit 1
fi
if [ ! -f "$ADF_PATH" ]; then
    echo "ERROR: ADF not found: $ADF_PATH" >&2
    exit 1
fi

BOOT_WAIT="${BOOT_WAIT:-25}"
NO_SERVER="${NO_SERVER:-0}"
JOYSTICK="${JOYSTICK:-0}"

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

# emukey.py needs python-xlib. The repo keeps a venv for it because the
# system python3 has no Xlib and pip3 may point at an unrelated venv.
EMU_PY="$SCRIPT_DIR/.venv/bin/python"
if [ ! -x "$EMU_PY" ]; then
    EMU_PY=python3
fi
if ! "$EMU_PY" -c "import Xlib" 2>/dev/null; then
    echo "ERROR: python-xlib not available to $EMU_PY" >&2
    echo "       python3 -m venv emu/.venv && emu/.venv/bin/pip install python-xlib" >&2
    exit 1
fi

TIMESTAMP=$(date -u +%Y%m%dT%H%M%S)
LOG_DIR="$SCRIPT_DIR/logs/$APP_NAME/drive-$TIMESTAMP"
SHOT_DIR="$LOG_DIR/shots"
mkdir -p "$SHOT_DIR"

FSUAE_SYS_LOG="$HOME/Documents/FS-UAE/Cache/Logs/fs-uae.log.txt"

echo "=== emu/drive.sh: $APP_NAME (headless, scripted) ==="
echo "ADF:   $ADF_PATH"
echo "Logs:  $LOG_DIR"
echo "Keys:  $KEYS"

killall fs-uae      2>/dev/null || true
killall socat       2>/dev/null || true
killall fujinet-nio 2>/dev/null || true
rm -f /tmp/amiga-serial /tmp/fn-pty
sleep 2

mkdir -p "$(dirname "$FSUAE_SYS_LOG")"
> "$FSUAE_SYS_LOG" 2>/dev/null || true

# floppy_drive_0 must be a short filename; absolute paths are silently ignored.
FLOPPIES_DIR="$HOME/Documents/FS-UAE/Floppies"
mkdir -p "$FLOPPIES_DIR"
ADF_BASENAME=$(basename "$ADF_PATH")
cp "$ADF_PATH" "$FLOPPIES_DIR/$ADF_BASENAME"

# Stale save-disk files cache old ADF contents and boot an outdated image.
rm -f "$HOME/Documents/FS-UAE/Save States/run/${ADF_BASENAME%.adf}.sdf"

# Port 1 (the Amiga's port 2 / game port — FS-UAE numbers from 0, where port
# 0 is the mouse) is either freed for the keyboard or driven *by* it:
#
#   JOYSTICK=0  no stick. FS-UAE would otherwise steal the host arrow keys
#               for a fallback joystick and they'd never reach the emulated
#               keyboard, so the port is explicitly emptied.
#   JOYSTICK=1  FS-UAE's built-in "keyboard" controller sits in the port and
#               the host arrows become stick directions, Right Ctrl/Right Alt
#               the fire button (share/fs-uae/input/default_keyboard.conf).
#               Arrows then do NOT reach the keyboard — that is the point:
#               it is how a scripted run exercises a joystick it cannot plug
#               in. Every other key still types normally.
if [ "$JOYSTICK" = 1 ]; then
    JOYSTICK_PORT_1=keyboard
else
    JOYSTICK_PORT_1=nothing
fi
echo "Joy:   port 1 = $JOYSTICK_PORT_1"

# AUDIO_WAV=<path> — capture what Paula actually played, so a headless run can
#               check sound instead of only pixels. FS-UAE plays through
#               OpenAL (not SDL), and OpenAL Soft has a "wave" backend that
#               writes a .wav instead of opening a device, selected by an
#               ALSOFT_CONF we generate here. Floppy-drive sounds are muted
#               for the capture: they are FS-UAE's own samples, not the
#               guest's, and they would otherwise swamp a 20 ms blip.
FLOPPY_VOLUME=""
if [ -n "$AUDIO_WAV" ]; then
    AUDIO_WAV=$(realpath -m "$AUDIO_WAV")
    ALSOFT_CONF="$LOG_DIR/alsoft.conf"
    cat > "$ALSOFT_CONF" <<EOF
[general]
drivers = wave
# Default here is 32-bit float in a WAVE_FORMAT_EXTENSIBLE header, which
# python's wave module refuses to open. Plain 16-bit PCM keeps checkaudio.py
# on the standard library.
sample-type = int16
[wave]
file = $AUDIO_WAV
EOF
    export ALSOFT_CONF
    FLOPPY_VOLUME="floppy_drive_volume = 0"
    rm -f "$AUDIO_WAV"
    echo "Audio: capturing to $AUDIO_WAV (floppy sounds muted)"
fi

FSUAE_CONFIG="$LOG_DIR/drive.fs-uae"
cat > "$FSUAE_CONFIG" <<EOF
[fs-uae]
amiga_model = A500
kickstart_file = $KICKSTART_ROM
floppy_drive_0 = $ADF_BASENAME
console_debugger = 0
ntsc_mode = 1
screenshots_output_dir = $SHOT_DIR
joystick_port_1 = $JOYSTICK_PORT_1
$FLOPPY_VOLUME
# serial_port is passed on the command line as --serial_port=<PTY>
# DO NOT add serial_port here: TCP mode causes IOERR_OPENFAIL in guest software
EOF

SOCAT_PID=""; FN_PID=""; FSUAE_PID=""; XVFB_PID=""
EMU_DISPLAY=:99

cleanup() {
    kill $FN_PID $SOCAT_PID $FSUAE_PID $XVFB_PID 2>/dev/null || true
    sleep 1
    cp "$FSUAE_SYS_LOG" "$LOG_DIR/fsuae-sys.log" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

if [ "$NO_SERVER" != 1 ]; then
    socat -d -d -x \
        pty,raw,echo=0,link=/tmp/amiga-serial \
        pty,raw,echo=0,link=/tmp/fn-pty \
        2>"$LOG_DIR/serial-trace.log" &
    SOCAT_PID=$!
    for i in $(seq 1 10); do
        sleep 1
        [ -L /tmp/amiga-serial ] && [ -L /tmp/fn-pty ] && break
    done
    if [ ! -L /tmp/amiga-serial ]; then
        echo "ERROR: socat PTYs did not appear after 10s" >&2
        exit 1
    fi
    AMIGA_DEV=$(readlink /tmp/amiga-serial)
    echo "Amiga serial PTY: $AMIGA_DEV"
    SERIAL_ARG=(--serial_port="$AMIGA_DEV")
else
    SERIAL_ARG=()
fi

killall -q Xvfb 2>/dev/null || true
sleep 1
Xvfb "$EMU_DISPLAY" -screen 0 800x600x24 &
XVFB_PID=$!
sleep 1

DISPLAY="$EMU_DISPLAY" fs-uae "$FSUAE_CONFIG" "${SERIAL_ARG[@]}" \
    >"$LOG_DIR/emulator.log" 2>&1 &
FSUAE_PID=$!
echo "FS-UAE PID: $FSUAE_PID"

if [ "$NO_SERVER" != 1 ]; then
    sleep 5
    FN_BIN="${FN_BIN:-$PROJECT_ROOT/fujinet-nio/build/fujibus-rs232-debug/fujinet-nio}"
    if [ ! -x "$FN_BIN" ]; then
        echo "ERROR: fujinet-nio binary not found: $FN_BIN" >&2
        exit 1
    fi
    FN_SERIAL_PORT=/tmp/fn-pty FN_SERIAL_BAUD=19200 \
        stdbuf -oL -eL "$FN_BIN" >"$LOG_DIR/fujinet.log" 2>&1 &
    FN_PID=$!
    echo "fujinet-nio PID: $FN_PID"
fi

echo "Booting (${BOOT_WAIT}s)..."
sleep "$BOOT_WAIT"

# --- Walk the key script ---
# FS-UAE names its dumps fs-uae-crop-<n>.png (plus a full-frame variant we
# discard); rename the newest crop to the requested label.
shot() {
    local label="$1"
    DISPLAY="$EMU_DISPLAY" "$EMU_PY" "$SCRIPT_DIR/scripts/emukey.py" "$EMU_DISPLAY" F12+s >/dev/null 2>&1
    sleep 2
    local newest
    newest=$(ls -t "$SHOT_DIR"/*crop*.png 2>/dev/null | head -1)
    if [ -n "$newest" ]; then
        mv "$newest" "$SHOT_DIR/$label.png"
        echo "  shot: $label.png"
    else
        echo "  shot: $label FAILED (no png written)" >&2
    fi
}

for token in $KEYS; do
    case "$token" in
        shot:*) shot "${token#shot:}" ;;
        *)
            echo "  key: $token"
            DISPLAY="$EMU_DISPLAY" "$EMU_PY" "$SCRIPT_DIR/scripts/emukey.py" \
                "$EMU_DISPLAY" "$token" >/dev/null 2>&1
            ;;
    esac
done

# Discard the full-frame dumps; the crops are the raw Amiga display.
rm -f "$SHOT_DIR"/fs-uae-*.png

echo "=== done ==="
echo "Shots: $SHOT_DIR"
ls "$SHOT_DIR" 2>/dev/null
