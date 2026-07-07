# Audit: Documentation Suite (Brief Output 4)

> **Status: ARCHIVED — superseded 2026-07-07.** All findings (F1–F5)
> executed across PRs #8/#10. The status-update rule and doc inventory
> live on in CLAUDE.md and docs/README.md.

Non-advocate review of the existing doc suite (CLAUDE.md, docs/, contracts/),
per the repo-audit brief. Verdict up front: **the suite is small, load-bearing,
and well-processed — its disease is staleness in status-bearing sections, not
sprawl.** No new doc suite is needed; fix the items below and add one habit.

---

## What's working (keep as is)

- `docs/README.md` evergreen-vs-archive process is genuinely good and being
  followed for handoffs (`docs/archive/` has proper banners).
- `contracts/` is the right shape: seven focused specs, each with a single
  owner topic, referenced from CLAUDE.md by table.
- `updating-fujinet-compat-headers.md` and `upstreaming-battleship.md` are
  model evergreen docs — procedure + rationale, short, current.
- CLAUDE.md as entry point with canonical build commands is exactly the
  "short entry point that links out" an agent needs.

## Findings

### F1 — `strategic-plan.md` status table is stale (highest value fix)
The Status table says Track 1A "Not started" and Track 1B "Not started", but
`plan-track1a-compat-layer.md` is **Complete (2026-07-01)** and Track 1B
Phases 1–2 shipped (battleship plays a full game). An agent reading the
strategy doc first — which is what it's for — gets the project state wrong.
The Lessons Learned section has one entry while at least one more lesson
(stale `.sdf` save-disk cache overriding fresh ADFs) lives only in `emu/run.sh`
and session memory.

**Fix:** update the Status table and add the `.sdf` lesson. Then adopt the
habit in F5.

### F2 — `emuplan.md` is a root-level orphan
It's a point-in-time implementation plan whose work shipped as `emu/` + the
`emu-build-and-boot` skill. Per the docs process it should have been rehomed
and archived. Sitting at repo root, it's the first "plan" a file listing
surfaces.

**Fix:** rehome any still-true facts (tool inventory / install checklist →
CLAUDE.md or a contracts/emu doc), then `git mv emuplan.md
docs/archive/plan-emu-automation.md` with an ARCHIVED banner.

### F3 — CLAUDE.md Reference Apps table is incomplete
`apps/compat_test/` and `apps/pacmantests/` (which includes the undocumented
`amiga-pac-man` submodule, tschak909) exist but aren't in the table or the
submodule setup docs. An agent discovering them has no stated purpose or
build/ownership info.

**Fix:** add rows for `compat_test` (compat-layer smoke test on emulator) and
`pacmantests` (graphics exploration harnesses for the Phase 3 battleship
renderer — mark as exploratory/non-shipping), and add `amiga-pac-man` to the
submodule table with upstream noted.

### F4 — `plan-track1a-compat-layer.md` should be archived
Its own Status line says Complete, and the docs process says a fully-done plan
with no residual reference value gets archived. Its residual value (the
header-sync procedure) already lives in `updating-fujinet-compat-headers.md`,
and the gap table lives separately in `audit-track1a-gap-table.md` (which
stays — it's referenced by the sync procedure).

**Fix:** rehome-check, then `git mv` to `docs/archive/` with banner.

### F5 — No doc inventory, and no trigger to update status
Docs are only discoverable by `ls docs/`. More importantly, nothing prompts a
status update when a phase completes — that's how F1 happened.

**Fix (two small additions):**
1. Add a one-line-per-doc inventory table to `docs/README.md` (evergreen docs
   only; archive is browsable by banner).
2. Add one rule to CLAUDE.md's Documentation section: *"A PR that completes or
   starts a track/phase must update the `strategic-plan.md` Status table in
   the same PR."* That closes the staleness loop at the moment the
   information exists.

## What NOT to do

- Don't add per-directory READMEs, architecture decision records, or a wiki.
  The suite's strength is that everything is reachable in two hops from
  CLAUDE.md; more files would dilute that and rot.
- Don't merge contracts/ into docs/ — the split (specs vs. narrative) is
  pulling its weight in the orchestration pattern.
