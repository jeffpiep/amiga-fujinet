# docs/ — Documentation Process

This directory holds two fundamentally different kinds of document. They are
maintained in opposite ways, so decide which kind you're writing before you
start.

## 1. Evergreen docs — edit in place

Procedures, references, strategy, architecture. These must always reflect the
**current** state of the project. When something changes, edit the doc so it
stays true. Never move an evergreen doc aside — git history is its archive.

Examples: `strategic-plan.md`, `updating-fujinet-compat-headers.md`,
`upstreaming-battleship.md`, `audit-track1a-gap-table.md`.

Canonical build/run steps live in the top-level `CLAUDE.md`, not here — this
directory is for deeper narrative and reference than the always-loaded index.

## 2. Point-in-time artifacts — archive, don't mutate

Session handoffs, debug session plans, and implementation plans are **snapshots
of a moment**. Their value comes from being a faithful record of what was true
then. Do **not** edit them to "keep them current" — that destroys their meaning.

When a snapshot's moment has passed:

1. **Rehome the durable facts first.** Migrate anything still useful (a build
   step, a gotcha, a decision) *up* into the evergreen doc that owns that topic
   — usually `CLAUDE.md` or one of the evergreen docs above. Don't spawn a new
   file for every fact; that just creates sprawl.
2. **Then retire the snapshot.** `git mv` it into `docs/archive/` and add a
   status banner under the title pointing to where its facts now live:

   ```markdown
   > **Status: ARCHIVED — superseded YYYY-MM-DD.** <one line: why, and where
   > the durable info moved.>
   ```

Rule of thumb: **rehome, then retire.** Never edit a snapshot in place.

## Status header convention

Implementation-plan docs that stay in `docs/` (because they still have open
work) carry a `**Status:**` line near the top:

- `Not started` → `In progress (DATE) — <what's done / what's left>`
- `Complete (DATE) — <where ongoing maintenance, if any, now lives>`

A plan whose work is fully done and has no residual reference value should be
archived per §2 instead of left with a `Complete` header.

## Archive

`docs/archive/` holds retired snapshots, kept browsable (rather than deleted)
because this project's history is narrated through handoffs worth re-reading.
Every file there should open with an `ARCHIVED` status banner.
