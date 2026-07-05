# c99-pac-man

C99 / amiga-gcc (`m68k-amigaos-gcc`) ports of Thom Cherryhomes' Pac-Man
graphics test harnesses, originally written in Lattice C. See the originals in
`../amiga-pac-man/test-harnesses/`.

Each harness is a self-contained learning exercise (bitplanes, sprites, BOBs,
maze drawing, …). Ports live in their own subdirectory and build/run through the
same emulator workflow as the other `apps/` (see the project `CLAUDE.md`).

| Harness | Status | Teaches |
|---------|--------|---------|
| `Ghosts/` | ✅ ported | Bitplanes — draws each plane of a ghost separately, then composited |

## Build & run a harness

```bash
cd Ghosts
make              # build the Amiga binary
make emu-test     # build an ADF, boot it in FS-UAE, capture a screenshot
```

`make emu-test` reports **FAIL (timeout)** on purpose: these are *visual*
harnesses that open a screen and `Wait()` on the close gadget — they send no
serial data, so there is no log pattern to match. Judge success from the
screenshot:

```bash
ls -t ../../../emu/logs/ghosts/*/screenshot.png | head -1   # Read this in Claude
```

## Porting notes (Lattice C → amiga-gcc C99)

The language surface is all that changes; behaviour is preserved:

- K&R function definitions → ANSI/C99 prototypes and definitions.
- Implicit-`int` `main()` → `int main(void)` with `return 0`.
- Add `proto/exec.h`, `proto/graphics.h`, `proto/intuition.h` so the compiler
  sees real library prototypes. Keep the manual `OpenLibrary` calls and global
  library bases (`GfxBase`, `IntuitionBase`) — the proto inline stubs resolve
  against those globals, exactly as the original expected.
- `CFLAGS` mirror the other apps: `-std=c99 -mcpu=68000 -msoft-float
  -mcrt=nix13`, plus `-Wno-pointer-sign` (the AmigaOS APIs take `UBYTE *`
  strings; the source passes plain `char *`).

### ⚠️ Blitter source data must live in Chip RAM

The single non-obvious gotcha. `DrawImage()` (and every blitter operation:
BOBs, sprites, `BltBitMap`, …) can only **read** its source from **Chip RAM**.

FS-UAE's default A500 has 512K chip + 512K *slow* RAM. AmigaDOS `LoadSeg()`
tends to put the program's data hunk in slow RAM, so blitting image data
straight out of a `static`/global array feeds the blitter garbage — you get
scattered pixels instead of the shape. A real 512K A500 is all-chip, which is
why the original Lattice builds "just worked."

**Fix used in `Ghosts/`:** allocate a `MEMF_CHIP` buffer at runtime, `CopyMem()`
the bitmap into it, and point `Image.ImageData` there (see `copyToChip()` in
`Ghosts/Ghosts.c`). This works on every memory configuration, including real
hardware. Free the buffers with `FreeMem()` on exit.

> The toolchain has a `.datachip` linker section, but on this amiga-gcc build it
> emits the data as an ordinary (non-chip) hunk — the hunk is *not* flagged
> `MEMF_CHIP` — so the section attribute alone does **not** work. Use the
> runtime `AllocMem(MEMF_CHIP)` + `CopyMem` approach above.

Diagnosis shortcut: if a blitted image is garbage but text/`RectFill` render
fine, suspect Chip RAM. Confirm by booting FS-UAE with `slow_memory = 0`
(forces all RAM to chip) — if the image renders, it was a Chip-RAM placement
issue.
