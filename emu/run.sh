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

# --- Kill stale processes ---
killall fs-uae    2>/dev/null || true
killall socat     2>/dev/null || true
killall fujinet-nio 2>/dev/null || true
rm -f /tmp/amiga-serial /tmp/fn-pty
sleep 1

# --- Copy ADF to FS-UAE Floppies directory ---
# floppy_drive_0 must be a short filename; absolute paths are silently ignored.
FLOPPIES_DIR="$HOME/Documents/FS-UAE/Floppies"
mkdir -p "$FLOPPIES_DIR"
ADF_BASENAME=$(basename "$ADF_PATH")
cp "$ADF_PATH" "$FLOPPIES_DIR/$ADF_BASENAME"

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
# -v enables hex dump of all bytes in both directions (goes to stderr / serial-trace.log)
socat -d -d -v \
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

# --- Launch FS-UAE ---
# Use DISPLAY=:0 (not xvfb-run): xvfb-run fails in background sessions
DISPLAY=:0 fs-uae "$FSUAE_CONFIG" --serial_port="$AMIGA_DEV" \
    >"$LOG_DIR/emulator.log" 2>&1 &
FSUAE_PID=$!
echo "FS-UAE PID: $FSUAE_PID"
sleep 5   # give FS-UAE time to open serial hardware before fujinet-nio connects

# --- Start fujinet-nio ---
FN_BIN="$PROJECT_ROOT/fujinet-nio/build/fujibus-rs232-debug/fujinet-nio"
if [ ! -x "$FN_BIN" ]; then
    echo "ERROR: fujinet-nio binary not found: $FN_BIN" >&2
    echo "Build with: cd fujinet-nio && ./build.sh -p fujibus-rs232-debug" >&2
    kill "$FSUAE_PID" "$SOCAT_PID" 2>/dev/null || true
    INCLUDE_TAILS=1 "$SCRIPTS/write-result.sh" "$LOG_DIR" "$APP_NAME" FAIL fujinet_binary_missing 0
    exit 1
fi

FN_SERIAL_PORT=/tmp/fn-pty FN_SERIAL_BAUD=19200 \
    "$FN_BIN" >"$LOG_DIR/fujinet.log" 2>&1 &
FN_PID=$!
echo "fujinet-nio PID: $FN_PID"

# --- Poll logs for PASS / FAIL ---
START_TIME=$SECONDS
RESULT=""
REASON=""

echo "Polling logs (timeout ${TIMEOUT_S}s)..."
while [ $((SECONDS - START_TIME)) -lt "$TIMEOUT_S" ]; do
    sleep 2

    # Crash detection
    if ! kill -0 "$FN_PID" 2>/dev/null; then
        RESULT=FAIL; REASON=server_crash; break
    fi
    if ! kill -0 "$FSUAE_PID" 2>/dev/null; then
        RESULT=FAIL; REASON=emulator_crash; break
    fi

    # FAIL pattern (fast exit on known-bad string)
    if [ -n "$FAIL_PATTERN" ]; then
        if grep -qsF "$FAIL_PATTERN" "$LOG_DIR/fujinet.log" "$LOG_DIR/serial-trace.log" 2>/dev/null; then
            RESULT=FAIL; REASON=fail_pattern; break
        fi
    fi

    # PASS pattern
    if grep -qsF "$PASS_PATTERN" "$LOG_DIR/fujinet.log" "$LOG_DIR/serial-trace.log" 2>/dev/null; then
        RESULT=PASS
        REASON="matched '${PASS_PATTERN}' in logs"
        break
    fi
done

DURATION=$((SECONDS - START_TIME))

if [ -z "$RESULT" ]; then
    RESULT=FAIL
    REASON=timeout
fi

# --- Tear down ---
kill "$FN_PID" "$SOCAT_PID" "$FSUAE_PID" 2>/dev/null || true
sleep 1

# --- Write result.json ---
if [ "$REASON" = "timeout" ] || [ "$REASON" = "emulator_crash" ] || [ "$REASON" = "server_crash" ]; then
    INCLUDE_TAILS=1 "$SCRIPTS/write-result.sh" "$LOG_DIR" "$APP_NAME" "$RESULT" "$REASON" "$DURATION"
else
    "$SCRIPTS/write-result.sh" "$LOG_DIR" "$APP_NAME" "$RESULT" "$REASON" "$DURATION"
fi

echo "=== $RESULT ($REASON) in ${DURATION}s ==="

[ "$RESULT" = "PASS" ] && exit 0 || exit 1
