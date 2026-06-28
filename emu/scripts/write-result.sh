#!/usr/bin/env bash
# Writes a result.json file to the given log directory.
# Set INCLUDE_TAILS=1 to append the last 20 lines of each log.
#
# Usage:
#   write-result.sh <log_dir> <app_name> <PASS|FAIL> <reason> <duration_s>
#   INCLUDE_TAILS=1 write-result.sh <log_dir> <app_name> FAIL timeout <duration_s>

set -euo pipefail

LOG_DIR="$1"
APP_NAME="$2"
RESULT="$3"
REASON="$4"
DURATION_S="${5:-0}"
INCLUDE_TAILS="${INCLUDE_TAILS:-0}"
TIMESTAMP=$(date -u +%Y-%m-%dT%H:%M:%SZ)

# Build optional log_tail block
TAILS=""
if [ "$INCLUDE_TAILS" = "1" ]; then
    tail_json() {
        local file="$1"
        local key="$2"
        if [ -f "$file" ]; then
            local content
            content=$(tail -n 20 "$file" | python3 -c '
import sys, json
print(json.dumps(sys.stdin.read()))
')
            printf ',\n  "%s_tail": %s' "$key" "$content"
        fi
    }
    TAILS+=$(tail_json "$LOG_DIR/fujinet.log"      "fujinet")
    TAILS+=$(tail_json "$LOG_DIR/serial-trace.log" "serial_trace")
    TAILS+=$(tail_json "$LOG_DIR/emulator.log"     "emulator")
fi

cat > "$LOG_DIR/result.json" <<EOF
{
  "app": "$APP_NAME",
  "timestamp": "$TIMESTAMP",
  "result": "$RESULT",
  "reason": "$REASON",
  "duration_s": $DURATION_S,
  "logs": {
    "fujinet": "$LOG_DIR/fujinet.log",
    "serial_trace": "$LOG_DIR/serial-trace.log",
    "emulator": "$LOG_DIR/emulator.log"
  }$TAILS
}
EOF

echo "result.json: $RESULT ($REASON) in ${DURATION_S}s — $LOG_DIR/result.json"
