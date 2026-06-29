#!/usr/bin/env bash
# emu/scripts/build-adf.sh — Build a bootable ADF for an Amiga app
#
# Required env vars:
#   APP_NAME      — app name; used as the ADF volume label and binary name on disk
#   APP_BINARY    — path to the compiled AmigaOS binary
#
# Optional env vars:
#   EMU_STARTUP_ARGS   — extra args appended to the app in startup-sequence
#   EMU_STARTUP_PREFIX — AmigaDOS commands to run before the app (multi-line OK)
#   ADF_OUT            — output path (default: <dir of APP_BINARY>/<APP_NAME>.adf)
#
# Reads from emu/config/paths.env:
#   WB_ADF        — path to a Workbench 1.3.x ADF (source of serial.device)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EMU_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# shellcheck source=/dev/null
source "$EMU_DIR/config/paths.env"

: "${APP_NAME:?APP_NAME required}"
: "${APP_BINARY:?APP_BINARY required}"
: "${WB_ADF:?WB_ADF must be set in emu/config/paths.env}"

if [ ! -f "$APP_BINARY" ]; then
    echo "ERROR: binary not found: $APP_BINARY" >&2
    exit 1
fi
if [ ! -f "$WB_ADF" ]; then
    echo "ERROR: Workbench ADF not found: $WB_ADF" >&2
    exit 1
fi

EMU_STARTUP_ARGS="${EMU_STARTUP_ARGS:-}"
EMU_STARTUP_PREFIX="${EMU_STARTUP_PREFIX:-}"
ADF_OUT="${ADF_OUT:-$(dirname "$APP_BINARY")/$APP_NAME.adf}"
# Volume label: uppercase, max 30 chars
ADF_LABEL="${APP_NAME^^}"

echo "Building ADF: $ADF_OUT"
echo "  binary:   $APP_BINARY"
echo "  startup:  $APP_NAME ${EMU_STARTUP_ARGS:-(no args)}"

# Temp dir for intermediate files; cleaned up on exit
TMPWORK=$(mktemp -d)
trap 'rm -rf "$TMPWORK"' EXIT

# Extract serial.device from Workbench ADF
# serial.device is not ROM-resident in KS 1.3 — must be on boot disk
xdftool "$WB_ADF" read DEVS/serial.device "$TMPWORK/serial.device"

# Extract external commands needed by EMU_STARTUP_PREFIX scripts.
# The ROM assigns C: to the boot disk's C/ dir; without these, MakDir/Assign/Echo fail.
mkdir -p "$TMPWORK/c"
for CMD in Makedir Assign Echo; do
    xdftool "$WB_ADF" read "C/$CMD" "$TMPWORK/c/$CMD" 2>/dev/null || \
        echo "  note: $CMD not found in WB_ADF (skipping)"
done

# Write startup-sequence
{
    [ -n "$EMU_STARTUP_PREFIX" ] && printf '%s\n' "$EMU_STARTUP_PREFIX"
    if [ -n "$EMU_STARTUP_ARGS" ]; then
        printf '%s %s\n' "$APP_NAME" "$EMU_STARTUP_ARGS"
    else
        printf '%s\n' "$APP_NAME"
    fi
} > "$TMPWORK/startup-sequence"

# Build the ADF
rm -f "$ADF_OUT"
xdftool "$ADF_OUT" create + format "$ADF_LABEL" ofs
xdftool "$ADF_OUT" makedir s
xdftool "$ADF_OUT" write "$TMPWORK/startup-sequence" s/startup-sequence
xdftool "$ADF_OUT" makedir Devs
xdftool "$ADF_OUT" write "$TMPWORK/serial.device" Devs/serial.device
xdftool "$ADF_OUT" write "$APP_BINARY" "$APP_NAME"
# Include external commands so EMU_STARTUP_PREFIX scripts can use MakDir/Assign/Echo
if [ -d "$TMPWORK/c" ] && [ "$(ls -A "$TMPWORK/c" 2>/dev/null)" ]; then
    xdftool "$ADF_OUT" makedir c
    for f in "$TMPWORK/c"/*; do
        xdftool "$ADF_OUT" write "$f" "c/$(basename "$f")"
    done
fi

# Install bootblock (xdftool formats with zero checksum — not bootable without this)
xdftool "$ADF_OUT" boot install

# Verify
BOOTABLE=$(xdftool "$ADF_OUT" boot show 2>&1 | grep "bootable:")
echo "$BOOTABLE"
if ! echo "$BOOTABLE" | grep -q "True"; then
    echo "ERROR: bootblock install failed" >&2
    exit 1
fi

echo "ADF ready: $ADF_OUT"
xdftool "$ADF_OUT" list
