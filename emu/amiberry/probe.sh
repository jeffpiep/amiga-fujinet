#!/usr/bin/env bash
# Spike: boot an ADF in Amiberry (A500, 68000, KS 1.3) wired to fujinet-nio.
# Amiberry and nio's TCP channel are both TCP servers, so a socat bridge dials
# both and traces every byte. Leaves everything running; `probe.sh stop` ends it.
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="${AB_OUT:-/tmp/amiberry-probe}"; mkdir -p "$HERE"
PIDS="$HERE/pids"
source "$ROOT/emu/config/paths.env"
DISP=:98
# A fresh port per run: the previous listener may still be in TIME_WAIT.
AB_PORT=$((20000 + RANDOM % 10000))

stop() {
    [ -f "$PIDS" ] && xargs -r kill <"$PIDS" 2>/dev/null
    rm -f "$PIDS"
}
if [ "${1:-}" = stop ]; then stop; exit 0; fi
ADF="${1:-$ROOT/apps/http_get/http_get.adf}"
stop; sleep 1

Xvfb $DISP -screen 0 800x600x24 >/dev/null 2>&1 &
echo $! >>"$PIDS"
sleep 1

DISPLAY=$DISP SDL_AUDIODRIVER=dummy amiberry --log --model A500 -G \
    -r "$KICKSTART_ROM" -c 1 -b 2 -s cpu_type=68000 -s cachesize=0 -s cpu_compatible=true -s cpu_24bit_addressing=true \
    -s serial_port=TCP://127.0.0.1:$AB_PORT -s ntsc=true \
    -0 "$ADF" >"$HERE/amiberry.log" 2>&1 &
echo $! >>"$PIDS"

"$ROOT/fujinet-nio/build/fujibus-tcp-debug/fujinet-nio" >"$HERE/fujinet.log" 2>&1 &
echo $! >>"$PIDS"
sleep 2

socat -d -d -x TCP:127.0.0.1:$AB_PORT,retry=20,interval=0.5 TCP:127.0.0.1:65504 \
    2>"$HERE/serial-trace.log" &
echo $! >>"$PIDS"
echo "started: $(tr '\n' ' ' <"$PIDS")"
