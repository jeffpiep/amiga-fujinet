# Audit: Repo Organization (Brief Output 1)

> **Status: ARCHIVED — superseded 2026-07-07.** R1–R4 executed in PR #8;
> R5 (compat_test single ownership) executed alongside
> contracts/esp32-target.md. The blessed game-port pattern lives in
> CLAUDE.md "Reference Apps".

Per the brief: audit only what this repo owns (`libs/`, `apps/`, `contracts/`,
`emu/`, `docs/`) for platform leakage and misplaced files; the HAL/OSAL
separation proper lives in Mark's submodules and is off-limits. Battleship's
structure is mid-negotiation with upstream and is left alone.

Honest answer, as invited by the brief: **the layout is fine.** The
directory-level separation is already clean and matches the two-track
strategy. Below are the only findings — all file-level tidying, no
restructuring.

---

## Structure verdict (keep)

- `libs/` = reusable Amiga-side layers, `apps/` = consumers, `contracts/` =
  cross-submodule specs, `emu/` = test harness, submodules = upstream code.
  That *is* the HAL split at the granularity this repo controls: platform
  adaptation lives in `libs/fujinet-compat-amiga` and
  `apps/battleship/amiga/src/{graphics,input,sound,util}.c`; game/core logic
  stays in upstream submodules. No platform leakage found in either direction.
- **Blessed pattern for future game ports** (make it official in CLAUDE.md):

  ```
  apps/<game>/
    upstream/     ← read-only pinned submodule of the game repo
    amiga/        ← Makefile + platform layer (graphics/input/sound/util + vars)
  ```
  linking `libs/fujinet-compat-amiga`. Battleship established it; the next
  lobby-game port should copy it verbatim.
- **ESP32 glue**, when it exists, is server-side and belongs in `fujinet-nio`
  (Mark's repo, via upstream PRs) — nothing to pre-create here beyond
  `contracts/esp32-target.md` (see build-workflow audit, B6).

## Findings (file-level tidying)

### R1 — Root-level strays
- `emuplan.md` — superseded point-in-time plan; archive per the docs-suite
  audit (F2).
- `tree.txt` — untracked scratch output; delete (audits explore the repo
  directly).

### R2 — Orphan build artifact
`libs/fujinet-compat-amiga/test/host/test_network.test` is an untracked
compiled x86-64 binary with no source in the tree. Delete it; the host-test
path should be built for real per build-workflow audit B1 (source checked in,
binaries gitignored).

### R3 — `apps/pacmantests/` is fine but undocumented
Exploration harnesses for the Phase 3 battleship renderer (bitplane graphics),
including the `amiga-pac-man` submodule. Keeping exploratory work in the repo
is good; the only gap is documentation — CLAUDE.md doesn't mention the
directory or its submodule (docs-suite audit F3 covers the fix). No move
needed. If exploration areas multiply, revisit a `labs/` convention; one
directory doesn't justify it.

### R4 — Battleship `envarc/` blobs are both tracked and generated
`apps/battleship/amiga/envarc/fujinet/**` files are checked in, but the
Makefile also has rules that create them. Pick one: recommend keeping the
Makefile rules as the source of truth and gitignoring the generated files
(they're opaque binary blobs named `00`; the Makefile printf rules are the
readable documentation of their contents).

### R5 — `apps/compat_test/` split across two directories
Its Makefile lives in `apps/compat_test/` but its only source is
`../../libs/fujinet-compat-amiga/test/compat_test.c`. Minor, but it means the
compat layer's tests are half app, half lib. When the host-test work (B1)
touches this area anyway, consolidate: test sources under
`libs/fujinet-compat-amiga/test/`, and `apps/compat_test/` keeps only the
thin emulator-packaging Makefile (its real job). No urgency.

## Explicitly not proposed

- No `src/`–`hal/`–`osal/` re-layering: the repo doesn't own enough core code
  to warrant it, and the submodule boundaries already enforce the separation.
- No changes to `apps/battleship/` structure until the upstream contribution
  process is negotiated (`upstreaming-battleship.md` owns that plan).
