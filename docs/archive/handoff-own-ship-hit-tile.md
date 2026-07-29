# Handoff: distinct tile for hits on your own ships

> **ARCHIVED 2026-07-20 — DONE.** Implemented on `feature/battleship-art-pass`
> (commit `c9822ef`): `TILE_HIT_SHIP` renders own-ship hits as a red X on the
> gray hull; per-quadrant `s_ship_map` footprint in `graphics.c`; host tests
> pass; verified in a live game. The `tile_hit_ship` *art* is a flat-gray MVP the
> user may refine. Kept for the design rationale; the durable outcome now lives in
> `docs/plan-track1b-battleship.md`.

**Original status:** implementation handoff (snapshot, 2026-07-20)
**Branch:** `feature/battleship-art-pass`

## Goal

Add a new tile so a hit **on one of your own ships** (shown on your board — the
bottom board in a 2-player game) renders as a **red X on the gray hull**, visually
distinct from a hit on open water / an enemy cell, which stays the current red X
on sea (`tile_hit`).

This is an **engine change**, not just a `tiles.h` edit — the renderer currently
uses one board-agnostic hit tile for every board.

## Blast radius — Amiga platform layer only (verified)

This change touches **only** `apps/battleship/amiga/` and does **not** modify any
common upstream code:

- Files edited: `include/tiles.h`, `include/cellmap.h`, `src/cellmap.c`,
  `src/graphics.c`, `test/host/test_cellmap.c` — all under `apps/battleship/amiga/`.
- `cm_field_tile` / `cm_update_tile` are platform-only (declared in the Amiga
  `cellmap.h`, called only from the Amiga `graphics.c`). They are **not**
  referenced by the common upstream sources compiled into the Amiga build
  (`COMMON_SRCS` = `main.c`, `gamelogic.c`, `screens.c`, `stateclient.c`,
  `misc.c`). The `TILE_HIT`/hit-select code seen under `upstream/src/atari/` and
  `upstream/src/c64/` belongs to *those* platforms' layers and is not compiled here.
- The platform **contract** signatures (`drawShip`, `drawGamefield`,
  `drawGamefieldUpdate` in `upstream/src/platform-specific/graphics.h`) stay
  byte-identical — only their internals change, so upstream is untouched and no
  submodule PR is involved.

## Why it isn't just a tile edit (current behavior)

Three "hit" tiles exist today (`apps/battleship/amiga/include/tiles.h`,
`tile_table[]`; ids in `apps/battleship/amiga/include/cellmap.h`):

| Tile | id | Art | Used for |
|------|----|----|----------|
| `tile_hit` (TILE_HIT) | 3 | red X on `PEN_SEA` | settled hit on **any** board cell |
| `tile_hit2` (TILE_HIT2) | 4 | white X on `PEN_SEA` | blink frame during a fresh hit's animation |
| `tile_legend_hit` (TILE_LEGEND_HIT) | 5 | red X on `PEN_BG` | sunk ship in the legend drawer (not on the grid) |

Tile selection (`apps/battleship/amiga/src/cellmap.c`):
- `cm_field_tile(cell)` → `TILE_HIT` when `cell == CM_FIELD_ATTACK` (cellmap.c:77-84).
  This is the board-repaint path used by `drawGamefield` (graphics.c:259-273).
- `cm_update_tile(cell, anim)` → explosion frames `TILE_ANIM_0..5` for `anim>9`,
  then `anim 1..9` → `TILE_HIT2` (blink), `anim 0` → `TILE_HIT` (settle)
  (cellmap.c:86-100). This is the live-attack path used by `drawGamefieldUpdate`
  (graphics.c:275-285).

`TILE_HIT` is **board-agnostic**: the tile is chosen from the cell *state*
(`CM_FIELD_ATTACK = 1`), not from which board it is. Its `PEN_SEA` background
means it *replaces* the whole 8×8 cell, so a hit painted over your own ship reads
as "red X on water," erasing the hull underneath.

## The core problem and the key finding

To pick a different tile for "hit on my own ship," the renderer must know that a
ship occupies the hit cell. **The `gamefield` array does not encode ship
presence** — it only holds `FIELD_ATTACK (1)` / `FIELD_MISS (2)` / `0`
(`upstream/src/misc.h:58-59`). So ship presence has to come from somewhere else.

**Key finding that makes this clean:** `drawShip(quadrant, size, pos, hide)`
(`upstream/src/platform-specific/graphics.h:28`) is only ever called for **your
own ships** — you never see enemy ships on the grid. So the Amiga platform layer
can record a per-quadrant "own-ship footprint" as ships are drawn, and consult it
when painting a hit. **No upstream/submodule changes needed** — everything lives
in `apps/battleship/amiga/`.

`pos` encodes orientation: `pos > 99` means vertical (`pos -= 100`); cells are
`sx = pos%10`, `sy = pos/10`, running horizontally or vertically for `size` cells
(see the existing loop in graphics.c:230-237).

## Recommended approach — platform-layer per-quadrant ship footprint

Keep it per-quadrant (do **not** hardcode "quadrant 0"): own ships are usually
drawn on quadrant 0, but at least one path draws them on `activePlayer`
(`upstream/src/gamelogic.c:445`; other paths use quadrant 0 at :312/:481). Keying
the footprint on `drawShip`'s `quadrant` arg handles both, and any quadrant that
received a ship draw is by definition a board whose ships you can see (yours).

### 1. New tile(s) — `apps/battleship/amiga/include/tiles.h`

Add a red-X-on-hull tile (2-color is fine — reuse the `tile_hit` X pattern with a
`PEN_SHIP` background):

```c
static const uint16_t tile_hit_ship[32] = TILE_PAT(PEN_HIT, PEN_SHIP,
    0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81);
```

Add `TILE_HIT_SHIP` to the enum in `cellmap.h` **at the end, before
`TILE_COUNT`** (the comment there says extend at the end; `TILE_COUNT` sizes the
chip-RAM bank and bounds-checks draws in gfxcore.c:65,181,254 — adding a tile is
auto-handled). Add `tile_hit_ship` to `tile_table[]` in the matching slot.

*Optional polish:* also add `TILE_HIT2_SHIP` (white X on `PEN_SHIP`) so the blink
frame over a hull doesn't flash sea. MVP can skip this — the white blink is
transient and reads acceptably.

### 2. Own-ship footprint — `apps/battleship/amiga/src/graphics.c`

- Add a static `uint8_t s_ship_map[4][100]` (per quadrant, 10×10).
- Clear it in `drawBoard()` (the full-redraw entry, graphics.c:126) — memset 0.
- In `drawShip()` (graphics.c:204): after computing `sx,sy,vertical`, set the
  ship's cells in `s_ship_map[quadrant]` when `!hide` (SHOW), and clear them when
  `hide`. Guard `quadrant < 4` and cell index `< 100`.
- Expose a tiny accessor or just read `s_ship_map` directly in the two draw
  functions below (same file).

**Ordering is safe:** upstream always draws ships, then overlays the gamefield —
`drawGamefield`'s own comment says "any ships drawn by drawShip survive"
(graphics.c:254-258), and the call sites confirm draw-ship-then-gamefield
(gamelogic.c:312→319, 445→448). So the footprint is populated before the hit
overlay reads it. Verify this ordering holds in every redraw path you touch.

### 3. Tile selection — `apps/battleship/amiga/src/cellmap.c` (+ `cellmap.h`)

Thread an `on_ship` flag into the two selectors (keeps the logic host-testable):

- `cm_field_tile(cell, on_ship)` → `on_ship && cell==CM_FIELD_ATTACK` ?
  `TILE_HIT_SHIP` : (existing result).
- `cm_update_tile(cell, anim, on_ship)` → when it would return `TILE_HIT`
  (settle), return `TILE_HIT_SHIP` if `on_ship`. (If you added `TILE_HIT2_SHIP`,
  swap the blink branch too.)

Update the two callers to pass the footprint bit:
- `drawGamefield` (graphics.c:270): `on_ship = s_ship_map[quadrant][row*10+col]`.
- `drawGamefieldUpdate` (graphics.c:283): `on_ship = s_ship_map[quadrant][attackPos]`.

`on_ship` only affects the `ATTACK` case (a miss is always water), so misses are
untouched.

### 4. Tests — `apps/battleship/amiga/test/host/test_cellmap.c`

`cm_field_tile` / `cm_update_tile` are pure and already T1-tested. Update existing
calls for the new signature and add cases: `on_ship=1` + `ATTACK` → `TILE_HIT_SHIP`;
`on_ship=1` + `MISS` → `TILE_MISS` (unchanged); `on_ship=0` → existing behavior.
Run `make -C apps/battleship/amiga test-host`.

## Verification

1. **Host tests:** `make -C apps/battleship/amiga test-host` (cellmap logic).
2. **Gallery:** add `TILE_HIT_SHIP` (id) to the grid automatically (it iterates
   `0..TILE_COUNT`), rebuild `make -C apps/battleship/amiga gallery-adf`, and eyeball
   the new tile. Interactive launch pattern is in
   `/tmp/tilegallery-interactive.fs-uae` / prior session; or
   `DISPLAY=:0 fs-uae <config>`.
3. **Live game (the real test):** build + run the game interactively so you get
   hit on your own ship on the bottom board. Prereqs: `fujinet-nio` built
   (`fujinet-nio/build/fujibus-rs232-debug/fujinet-nio`), battleship ADF
   (`make -C apps/battleship/amiga emu-adf`). Orchestration (socat PTY bridge +
   windowed FS-UAE on `:0` + fujinet-nio) is captured in
   `/tmp/run-battleship-interactive.sh` from the prior session — replicate it (it
   joins an AI table, so you can get hit quickly). Confirm: hit on your ship shows
   red X on gray hull; hit on water and hits you land on the enemy board still show
   red X on sea; sunk-ship legend marker unchanged.

## Scope / decisions to make

- **MVP:** just `TILE_HIT_SHIP` for the settled hit. **Polish:** add
  `TILE_HIT2_SHIP` for the blink frame too.
- **Art:** the tile above is a red X on flat gray. Could instead show the X over a
  shaded hull (`TILE_MC` with `PEN_SHIP`/`PEN_SHIP_SHD`) for consistency with the
  gray ship tiles — an art call for the user.

## Risks / gotchas

- **Draw order** (ships before gamefield) is the load-bearing assumption. If any
  redraw path overlays a hit before drawing ships, that cell won't know it's a
  ship. Grep every `drawGamefield`/`drawGamefieldUpdate` call site in
  `upstream/src/gamelogic.c` against the preceding `drawShip`.
- **Footprint staleness:** clear on `drawBoard` and maintain on every `drawShip`
  (set on SHOW, clear on HIDE). Placement-blink hide/show on quadrant 0 also hits
  `drawShip` — harmless (no attacks during placement) as long as `drawBoard`
  resets it.
- **`playerCount_g`/quadrant mapping:** the footprint is indexed by the same
  `quadrant` value `drawShip`/`drawGamefield` receive, so it stays consistent
  across 2- and 4-player layouts without extra mapping.
- Do **not** modify `apps/battleship/upstream/` — all changes are in the Amiga
  platform layer (`tiles.h`, `cellmap.c/.h`, `graphics.c`, host test).
```
