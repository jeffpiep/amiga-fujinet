#!/usr/bin/env python3
"""Render the fujitzee dice from tiles.h to a PNG, without building an ADF.

The dice only appear inside a live game, and even the board preview needs a
build-and-boot cycle to look at. This reads the art the same way the compiler
does — palette, pen names and the DIE_CELL_* macros straight out of the
headers — and composes the faces offline, which turns an art iteration from
about a minute into about a second.

Nothing here is a substitute for the emulator: pixel geometry it gets exactly
right, but the Amiga's non-square pixels and the real display's contrast are
things only a boot can settle. Use this to narrow candidates down, then check
the winner with:

    make -C apps/fujitzee/amiga preview-adf
    NO_SERVER=1 APP_NAME=boardpreview \\
        ADF_PATH=apps/fujitzee/amiga/boardpreview.adf \\
        KEYS="sleep80 shot:dice" bash emu/drive.sh

Two modes:

  Contact sheet (default) — all six faces in all three face colours, exactly
  as the header currently defines them. Use after editing tiles.h.

      tools/dicepreview.py -o /tmp/dice.png

  Stamp comparison (--stamps) — lays candidate pip designs over the real
  frame cells, so trying a design costs an 8x8 text block rather than an edit
  to seven macros. pipstamps.txt holds the candidates already evaluated,
  rejected ones included.

      tools/dicepreview.py --stamps pipstamps.txt -o /tmp/pips.png

Pip positions come from fjlayout.c's _pips[] table rather than a copy, so the
faces this draws cannot drift from the ones the game draws.

Requires Pillow (python3-pil).
"""
import argparse
import os
import re
import sys

try:
    from PIL import Image, ImageDraw
except ImportError:
    sys.exit("dicepreview: needs Pillow (apt install python3-pil)")

HERE = os.path.dirname(os.path.abspath(__file__))
CELL, DIE = 8, 24                    # pixels per cell, per die
FRAME = ["TL", "T", "TR", "L", "C", "R", "BL", "B", "BR"]


# ---- reading the art out of the sources --------------------------------

def read_palette(tiles_src):
    """tile_palette[]'s 16 RGB4 entries as 8-bit RGB tuples."""
    body = re.search(r"tile_palette\[16\]\s*=\s*\{(.*?)\};",
                     tiles_src, re.S).group(1)
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    rgb4 = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{3})\b", body)]
    if len(rgb4) != 16:
        sys.exit("dicepreview: tile_palette[] has %d entries, want 16"
                 % len(rgb4))
    return [(((c >> 8) & 0xF) * 17, ((c >> 4) & 0xF) * 17, (c & 0xF) * 17)
            for c in rgb4]


def read_pens(tiles_src, pens_src):
    """PEN_* numbers, plus the one-letter aliases tiles.h defines for them."""
    pens = {name: int(n) for name, n in
            re.findall(r"#define\s+(PEN_\w+)\s+(\d+)", pens_src)}
    for letter, pen in re.findall(r"^#define\s+([A-Z])\s+(PEN_\w+)\s*$",
                                  tiles_src, re.M):
        pens[letter] = pens[pen]
    return pens


def read_cells(tiles_src):
    """{name: [64 pen tokens]} for every DIE_CELL_*, 'F' left as itself."""
    cells = {}
    for m in re.finditer(r"#define\s+DIE_CELL_(\w+)\(F\)\s*TILE_MC\s*\(",
                         tiles_src):
        depth, i = 1, m.end()
        while depth:                          # scan to TILE_MC's closing ')'
            if tiles_src[i] == "(":
                depth += 1
            elif tiles_src[i] == ")":
                depth -= 1
            i += 1
        toks = [t.strip() for t in
                tiles_src[m.end():i - 1].replace("\\", " ").split(",")]
        toks = [t for t in toks if t]
        if len(toks) != CELL * CELL:
            sys.exit("dicepreview: DIE_CELL_%s has %d pixels, want %d"
                     % (m.group(1), len(toks), CELL * CELL))
        cells[m.group(1)] = toks
    if not cells:
        sys.exit("dicepreview: found no DIE_CELL_* macros")
    return cells


def read_pips(layout_src):
    """fjlayout.c's per-face pip masks over the 3x3 cell grid."""
    body = re.search(r"_pips\[FJ_S_FACE_MAX\]\s*=\s*\{(.*?)\};",
                     layout_src, re.S).group(1)
    masks = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]+)", body)]
    if len(masks) != 6:
        sys.exit("dicepreview: _pips[] has %d faces, want 6" % len(masks))
    return masks


# ---- drawing -----------------------------------------------------------

BLANK_STAMP = ["." * CELL] * CELL


class Art(object):
    def __init__(self, tiles_h, pens_h, layout_c):
        tiles_src = open(tiles_h).read()
        self.palette = read_palette(tiles_src)
        self.pens = read_pens(tiles_src, open(pens_h).read())
        self.cells = read_cells(tiles_src)
        self.pips = read_pips(open(layout_c).read())

    def face_pens(self):
        return [self.pens["PEN_DIE"], self.pens["PEN_DIE_KEEP"],
                self.pens["PEN_DIE_HI"]]

    def die(self, face, face_pen, stamp=None):
        """One 24x24 die. With `stamp`, pips come from it laid over the
        plain frame cells; without, from the header's own _PIP macros."""
        img = Image.new("RGB", (DIE, DIE))
        mask = self.pips[face - 1]
        for c in range(9):
            pipped = (mask >> c) & 1
            if stamp is None:
                name = FRAME[c] + "_PIP" if pipped else FRAME[c]
                toks = self.cells.get(name, self.cells[FRAME[c]])
                over = BLANK_STAMP
            else:
                toks = self.cells[FRAME[c]]
                over = stamp if pipped else BLANK_STAMP
            ox, oy = (c % 3) * CELL, (c // 3) * CELL
            for i, tok in enumerate(toks):
                ch = over[i // CELL][i % CELL]
                if ch != ".":
                    pen = self.pens[ch]
                else:
                    pen = face_pen if tok == "F" else self.pens[tok]
                img.putpixel((ox + i % CELL, oy + i // CELL),
                             self.palette[pen])
        return img


def grid(art, rows, faces, scale, label_w):
    """Lay rows of dice out into one PNG.

    rows: [(label, stamp-or-None, colour-or-None)]. A row that names one
    face colour spans just `faces` in it — that is the contact sheet, a row
    per colour. A row with colour None spans `faces` in all three, so
    candidate designs can be compared across the palette in one strip.
    """
    gap = 4
    cw = rh = (DIE + gap) * scale
    widest = max(len(faces) * (1 if c is not None else 3) for _, _, c in rows)
    out = Image.new("RGB", (label_w + widest * cw + gap,
                            len(rows) * rh + gap),
                    art.palette[art.pens["PEN_BG"]])
    draw = ImageDraw.Draw(out)
    for r, (label, stamp, colour) in enumerate(rows):
        y = r * rh + gap
        if label_w:
            draw.text((6, y + rh // 2 - 6), label, fill=(255, 255, 255))
        cols = ([(f, colour) for f in faces] if colour is not None
                else [(f, p) for p in range(3) for f in faces])
        for i, (face, fp) in enumerate(cols):
            im = art.die(face, art.face_pens()[fp], stamp)
            out.paste(im.resize((DIE * scale, DIE * scale), Image.NEAREST),
                      (label_w + i * cw, y))
    return out


# ---- candidate stamp files ---------------------------------------------

def read_stamps(path):
    """Parse a stamp file into ([(name, 8 rows)], {letter: PEN_ name}).

    '@ name' opens a stamp and 8 rows of 8 chars follow it; '= X PEN_FOO'
    names an extra pen letter beyond the aliases tiles.h already defines;
    '#' comments and blank lines are ignored.
    """
    rows, aliases, name, buf = [], {}, None, []
    for lineno, raw in enumerate(open(path), 1):
        line = raw.rstrip("\n")
        if line.startswith("#") or not line.strip():
            continue
        if line.startswith("="):
            parts = line[1:].split()
            if len(parts) != 2 or len(parts[0]) != 1:
                sys.exit("%s:%d: want '= <letter> <PEN_NAME>'" % (path, lineno))
            aliases[parts[0]] = parts[1]
            continue
        if line.startswith("@"):
            if name is not None:
                rows.append((name, _finish(name, buf, path)))
            name, buf = line[1:].strip(), []
            continue
        if name is None:
            sys.exit("%s:%d: pixels before any '@ name'" % (path, lineno))
        if len(line) != CELL:
            sys.exit("%s:%d: row is %d chars, want %d"
                     % (path, lineno, len(line), CELL))
        buf.append(line)
    if name is not None:
        rows.append((name, _finish(name, buf, path)))
    if not rows:
        sys.exit("dicepreview: %s defines no stamps" % path)
    return rows, aliases


def _finish(name, buf, path):
    if len(buf) != CELL:
        sys.exit("%s: stamp '%s' has %d rows, want %d"
                 % (path, name, len(buf), CELL))
    return buf


# ---- entry point -------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("-o", "--out", default="dice.png", help="PNG to write")
    ap.add_argument("-s", "--scale", type=int, default=8,
                    help="nearest-neighbour zoom (default 8)")
    ap.add_argument("--stamps", metavar="FILE",
                    help="compare candidate pip stamps from FILE")
    ap.add_argument("--faces", default="",
                    help="comma-separated faces (default all six for a "
                         "contact sheet, '1,6' for a stamp comparison)")
    ap.add_argument("--tiles", default=os.path.join(HERE, "..", "include",
                                                    "tiles.h"))
    ap.add_argument("--pens", default=os.path.join(HERE, "..", "include",
                                                   "pens.h"))
    ap.add_argument("--layout", default=os.path.join(HERE, "..", "src",
                                                     "fjlayout.c"))
    args = ap.parse_args()

    art = Art(args.tiles, args.pens, args.layout)
    if args.faces:
        faces = [int(f) for f in args.faces.split(",")]
        if any(f < 1 or f > 6 for f in faces):
            sys.exit("dicepreview: faces must be 1-6")
    else:
        faces = [1, 6] if args.stamps else [1, 2, 3, 4, 5, 6]

    if args.stamps:
        rows, aliases = read_stamps(args.stamps)
        label_w = 200
        for letter, pen in aliases.items():
            if pen not in art.pens:
                sys.exit("dicepreview: %s aliases '%s' to unknown %s"
                         % (args.stamps, letter, pen))
            art.pens[letter] = art.pens[pen]
        for name, stamp in rows:
            unknown = {c for row in stamp for c in row
                       if c != "." and c not in art.pens}
            if unknown:
                sys.exit("dicepreview: stamp '%s' uses undefined pen letter "
                         "%s — add '= <letter> PEN_NAME' to %s"
                         % (name, "/".join(sorted(unknown)), args.stamps))
        rows = [(name, stamp, None) for name, stamp in rows]
    else:
        rows = [(n, None, i) for i, n in
                enumerate(("plain", "kept", "highlighted"))]
        label_w = 100
    grid(art, rows, faces, args.scale, label_w).save(args.out)
    print(args.out)


if __name__ == "__main__":
    main()
