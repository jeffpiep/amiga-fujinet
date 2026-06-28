#!/usr/bin/env bash
# Strips socat -v hex-dump headers and emits the raw bytes as printable text.
# Useful for reading Amiga program output captured via serial-trace.log.
#
# Usage:
#   ./extract-serial-text.sh <serial-trace.log>
#   cat <serial-trace.log> | ./extract-serial-text.sh

# When reading from stdin, buffer to a temp file so the python heredoc
# doesn't compete with the piped input for python's stdin.
FILE="${1:-}"
TMPFILE=""
if [ -z "$FILE" ]; then
    TMPFILE=$(mktemp)
    cat > "$TMPFILE"
    FILE="$TMPFILE"
fi

python3 - "$FILE" <<'EOF'
import sys, re

filename = sys.argv[1]
fh = open(filename) if filename != '-' else sys.stdin

raw = bytearray()
for line in fh:
    # Skip direction/timestamp lines: "> 2026/..." or "< 2026/..."
    if re.match(r'^[<>] \d{4}/', line):
        continue
    # Skip socat debug lines: "2026/... socat[N] ..."
    if re.match(r'^\d{4}/', line):
        continue
    # Data line: " XX XX XX XX ..." (hex octets, space-separated, leading space)
    m = re.match(r'^ ((?:[0-9a-f]{2} *)+)', line)
    if m:
        for b in re.findall(r'[0-9a-f]{2}', m.group(1)):
            raw.append(int(b, 16))

fh.close()

# Decode as latin-1, replace non-printable (except \n \r \t) with '.'
text = raw.decode('latin-1')
text = re.sub(r'[^\x09\x0a\x0d\x20-\x7e]', '.', text)

# Print non-empty lines
for line in text.split('\n'):
    stripped = line.rstrip('\r')
    if stripped.replace('.', '').strip():
        print(stripped)
EOF

[ -n "$TMPFILE" ] && rm -f "$TMPFILE"
