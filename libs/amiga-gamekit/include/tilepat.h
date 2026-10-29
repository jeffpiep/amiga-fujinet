/*
 * tilepat.h - authoring macros for 8x8 x 4-bitplane gamekit tiles
 *
 * A tile is 8x8 pixels x 4 bitplanes = 32 uint16_t words, plane 0 first,
 * one word per row, leftmost pixel in bit 15 (pixels live in the high byte
 * since tiles are 8 wide). That is exactly struct Image data for Width=8
 * Height=8 Depth=4, which is what gfxcore.c blits — see
 * apps/pacmantests/c99-pac-man/Ghosts/.
 *
 * Writing those 32 words by hand is unreadable, so tile art is authored
 * through the two composers below and expanded at compile time. Include
 * this from the game's tiles.h; the pen numbers you pass are the game's own
 * PEN_* names, and their colors come from the game's palette.
 *
 * Header-only, no AmigaOS, no code emitted.
 *
 * Extracted from apps/battleship/amiga in Track 1C Phase 2 —
 * see docs/plan-track1c-fujitzee.md.
 */
#ifndef TILEPAT_H
#define TILEPAT_H

#include <stdint.h>

/* ---- 2-color tile composer ---- */

/* One row of one plane: fg pens where the pattern bit is set, bg pens
 * elsewhere; shifted into the high byte (8-wide tile in a 16-bit word). */
#define TROW(pat, fg, bg, pl) \
    ((uint16_t)(((((fg) >> (pl)) & 1) ? (uint8_t)(pat) : 0) | \
                ((((bg) >> (pl)) & 1) ? (uint8_t)~(uint8_t)(pat) : 0)) << 8)

#define TPLANE(pl, fg, bg, r0, r1, r2, r3, r4, r5, r6, r7) \
    TROW(r0, fg, bg, pl), TROW(r1, fg, bg, pl), \
    TROW(r2, fg, bg, pl), TROW(r3, fg, bg, pl), \
    TROW(r4, fg, bg, pl), TROW(r5, fg, bg, pl), \
    TROW(r6, fg, bg, pl), TROW(r7, fg, bg, pl)

#define TILE_PAT(fg, bg, r0, r1, r2, r3, r4, r5, r6, r7)  { \
    TPLANE(0, fg, bg, r0, r1, r2, r3, r4, r5, r6, r7),      \
    TPLANE(1, fg, bg, r0, r1, r2, r3, r4, r5, r6, r7),      \
    TPLANE(2, fg, bg, r0, r1, r2, r3, r4, r5, r6, r7),      \
    TPLANE(3, fg, bg, r0, r1, r2, r3, r4, r5, r6, r7) }

/* Same thing, for when the eight rows arrive as one macro (a shape reused
 * across several tiles, e.g. the same die cell in three face colors). The
 * preprocessor counts a macro's arguments before expanding them, so
 * TILE_PAT(fg, bg, ROWS) would be a 3-argument call and fail; forwarding
 * through a variadic expands ROWS first. */
#define TILE_PAT_V(...) TILE_PAT(__VA_ARGS__)

/* ---- Multicolor tile composer (art pass) ----
 *
 * Author a full-color tile as an 8x8 grid of pen numbers (0-15), one pen per
 * pixel, left-to-right then top-to-bottom (64 values). TILE_MC expands them
 * into the same 32-word / 4-plane struct Image layout TILE_PAT produces, so a
 * multicolor tile drops into tile_table[] with no engine change. Use this for
 * hero tiles (sea, ships); keep TILE_PAT for flat 2-color art.
 *
 * Each value is a PEN_* number (see gfxcore.h) — its *color* comes from
 * tile_palette[] above. You write pens, not colors. Lay the 64 values out as
 * eight rows of eight so the source reads like the picture; whitespace and the
 * row grouping are cosmetic (the preprocessor just sees 64 comma-separated
 * values). Example — a sea tile with a diagonal foam streak and one bright
 * crest pixel (S = PEN_SEA water, D = PEN_SEA_DK trough, F = PEN_FOAM):
 *
 *     #define S PEN_SEA
 *     #define D PEN_SEA_DK
 *     #define F PEN_FOAM
 *     static const uint16_t tile_sea[32] = TILE_MC(
 *         F, S, S, S, S, S, S, D,     row 0: crest at top-left, trough at right
 *         S, F, S, S, S, S, D, S,     rows 1-6: foam streak runs down-right,
 *         S, S, F, S, S, D, S, S,                the darker trough mirrors it
 *         S, S, S, F, D, S, S, S,
 *         S, S, S, D, F, S, S, S,
 *         S, S, D, S, S, F, S, S,
 *         S, D, S, S, S, S, F, S,
 *         D, S, S, S, S, S, S, F);    row 7
 *     #undef S
 *     #undef D
 *     #undef F
 *
 * The single-letter #defines are optional but make the grid legible; undef
 * them after each tile (or reuse a shared set) so they do not leak. A pen you
 * use must have a real color in tile_palette[] — an all-zero slot draws black. */

/* One plane row: pack 8 pens' bit `pl` into a byte, MSB = leftmost pixel. */
#define MROW(pl, q0, q1, q2, q3, q4, q5, q6, q7) \
    ((uint16_t)( ((((q0) >> (pl)) & 1) << 7) | ((((q1) >> (pl)) & 1) << 6) | \
                 ((((q2) >> (pl)) & 1) << 5) | ((((q3) >> (pl)) & 1) << 4) | \
                 ((((q4) >> (pl)) & 1) << 3) | ((((q5) >> (pl)) & 1) << 2) | \
                 ((((q6) >> (pl)) & 1) << 1) |  (((q7) >> (pl)) & 1) ) << 8)

#define MPLANE(pl, \
    a0,a1,a2,a3,a4,a5,a6,a7, b0,b1,b2,b3,b4,b5,b6,b7, \
    c0,c1,c2,c3,c4,c5,c6,c7, d0,d1,d2,d3,d4,d5,d6,d7, \
    e0,e1,e2,e3,e4,e5,e6,e7, f0,f1,f2,f3,f4,f5,f6,f7, \
    g0,g1,g2,g3,g4,g5,g6,g7, h0,h1,h2,h3,h4,h5,h6,h7) \
    MROW(pl,a0,a1,a2,a3,a4,a5,a6,a7), MROW(pl,b0,b1,b2,b3,b4,b5,b6,b7), \
    MROW(pl,c0,c1,c2,c3,c4,c5,c6,c7), MROW(pl,d0,d1,d2,d3,d4,d5,d6,d7), \
    MROW(pl,e0,e1,e2,e3,e4,e5,e6,e7), MROW(pl,f0,f1,f2,f3,f4,f5,f6,f7), \
    MROW(pl,g0,g1,g2,g3,g4,g5,g6,g7), MROW(pl,h0,h1,h2,h3,h4,h5,h6,h7)

#define TILE_MC(...) { \
    MPLANE(0, __VA_ARGS__), MPLANE(1, __VA_ARGS__), \
    MPLANE(2, __VA_ARGS__), MPLANE(3, __VA_ARGS__) }
#endif /* TILEPAT_H */
