#!/usr/bin/env python3
"""Strip HUNK_SYMBOL sections from an Amiga hunk executable.

KS 1.3 LoadSeg chokes on large HUNK_SYMBOL data (ERROR_FILE_NOT_OBJECT / 121).
This script removes all HUNK_SYMBOL sections while preserving everything else.
"""
import struct, sys

HUNK_CODE    = 0x3E9
HUNK_DATA    = 0x3EA
HUNK_BSS     = 0x3EB
HUNK_RELOC32 = 0x3EC
HUNK_SYMBOL  = 0x3F0
HUNK_END     = 0x3F2
HUNK_HEADER  = 0x3F3

def read32(data, pos):
    return struct.unpack('>I', data[pos:pos+4])[0], pos+4

def main():
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print(f"Usage: {sys.argv[0]} <input> [output]", file=sys.stderr)
        print("  If output is omitted, input is modified in place.", file=sys.stderr)
        sys.exit(1)

    src = sys.argv[1]
    dst = sys.argv[2] if len(sys.argv) == 3 else src

    with open(src, 'rb') as f:
        data = f.read()

    out = bytearray()
    pos = 0

    magic, pos = read32(data, pos)
    if magic != HUNK_HEADER:
        print(f"Not a hunk file: 0x{magic:08x}", file=sys.stderr)
        sys.exit(1)
    out += data[0:4]

    while True:
        n, pos = read32(data, pos)
        out += struct.pack('>I', n)
        if n == 0:
            break
        out += data[pos:pos+n*4]
        pos += n*4

    table_size, pos = read32(data, pos)
    first_hunk, pos = read32(data, pos)
    last_hunk, pos = read32(data, pos)
    out += struct.pack('>III', table_size, first_hunk, last_hunk)
    num_hunks = last_hunk - first_hunk + 1
    for _ in range(num_hunks):
        sz, pos = read32(data, pos)
        out += struct.pack('>I', sz)

    stripped = 0
    while pos < len(data):
        htype, newpos = read32(data, pos)

        if htype == HUNK_SYMBOL:
            p = newpos
            while True:
                namelen, p = read32(data, p)
                if namelen == 0:
                    break
                p += namelen * 4 + 4
            stripped += p - pos
            pos = p
        elif htype in (HUNK_CODE, HUNK_DATA):
            sz, _ = read32(data, newpos)
            end = newpos + 4 + sz * 4
            out += data[pos:end]
            pos = end
        elif htype == HUNK_BSS:
            out += data[pos:newpos+4]
            pos = newpos + 4
        elif htype == HUNK_RELOC32:
            p = newpos
            while True:
                count, p = read32(data, p)
                if count == 0:
                    break
                p += 4 + count * 4
            out += data[pos:p]
            pos = p
        elif htype == HUNK_END:
            out += data[pos:newpos]
            pos = newpos
        else:
            out += data[pos:]
            break

    with open(dst, 'wb') as f:
        f.write(out)

    if stripped > 0:
        print(f"Stripped {stripped} bytes of HUNK_SYMBOL data ({len(data)} -> {len(out)})")

if __name__ == '__main__':
    main()
