# Audit: Build System & Workflow (Brief Output 2)

> **Status: ARCHIVED — superseded 2026-07-07.** B1 (host tests, PR #10),
> B2/B3 (make/amiga.mk + apps/Makefile, PR #11), and B6
> (contracts/esp32-target.md) executed. B4 (cppcheck) and B5 (CI)
> deliberately deferred — see docs/testing.md "Future work".

Audit of Makefiles, emu harness, and testing/CI gaps, per the repo-audit
brief. The goal is a workflow where a Claude Opus session can build, test, and
verify any target with one documented command and never re-derive toolchain
setup.

Verdict up front: **the per-app `make` / `make emu-test` pattern is already
the right shape.** The gaps are (1) no cheap host-side inner loop, (2) flag
drift across duplicated Makefile preambles, (3) a stale aggregate Makefile,
and (4) no CI safety net. All fixable with small changes.

---

## Findings

### B1 — No host-side test loop for the compat layer (highest value fix)

> **Satisfied (2026-07-05)** by the tiered testing practice — see
> `docs/testing.md` and CLAUDE.md `## Testing` (`make test-host`).

The only way to exercise `libs/fujinet-compat-amiga` today is a full FS-UAE
boot (~90 s timeout, needs serial bridge + fujinet-nio + Xvfb). That's the
*outer* verification loop; there is no *inner* one. Evidence it's wanted:
`libs/fujinet-compat-amiga/test/host/` contains a compiled x86-64 test binary
(`test_network.test`) with **no source and no Makefile rule** — an orphan
artifact from a hand-run experiment.

**Fix:** make the host loop real.
- `fujinet-nio-lib` already builds `TARGET=linux`; add a `test-host` target to
  the compat-layer Makefile that compiles `src/*.c` + host test sources with
  the native `gcc` against the linux nio-lib, guarded so Amiga-only code
  (`ENVARC:` appkey file I/O in `fn_fuji.c`) is stubbed or conditionally
  compiled.
- Check in the test *source*; delete the orphan binary; gitignore `test/host/`
  build products.
- Convention: `make test-host` = seconds, no hardware; `make -C
  apps/<app> emu-test` = the closed-loop integration check. Document both in
  CLAUDE.md's Build Commands.

### B2 — Toolchain preamble is copy-pasted with drift

> **Satisfied (2026-07-05)** by `make/amiga.mk` — all Amiga Makefiles now
> include it; `-Wextra` is canonical.

`CC`/`CFLAGS` are repeated in every Amiga Makefile and have already diverged:
the compat lib uses `-Wall -Wextra`, the apps only `-Wall`; `-Wno-pointer-sign`
and `-DNO_INLINE_MULDIV` appear in some but not others. Drift here becomes
"works in one target, warns/breaks in another," which burns agent time.

**Fix:** add `make/amiga.mk` (sibling in spirit to `emu/template/emu.mk`)
defining `CC`, `AR`, and a single canonical `CFLAGS` (recommend promoting
`-Wextra` everywhere — it's the cheapest static analysis available), plus the
standard `NIO_LIB`/`COMPAT` paths. App Makefiles shrink to: sources, includes,
emu variables, link rule. Keep per-app additions possible via `CFLAGS +=`.

### B3 — `apps/Makefile` is stale

> **Satisfied (2026-07-05)** — `make -C apps` now builds all four apps,
> including battleship via its explicit target.

It builds only `http_get`; `fn_test`, `compat_test`, and battleship are
missing. A stale aggregate is worse than none — it silently under-verifies.

**Fix:** either list all apps (battleship with its explicit `battleship`
target) or delete the file and document per-app builds as the norm. Recommend
fixing it: a working `make -C apps` is the natural CI entry point (B5).

### B4 — Warning/analysis posture
No static analysis beyond compiler warnings. Hobby-appropriate additions, in
order of value:
1. `-Wextra` everywhere via B2 (free).
2. `cppcheck --std=c99` over `libs/` and `apps/*/src` as an optional
   `make lint` target (one apt package, no config).
3. Stop there. clang-tidy/sanitizers don't fit m68k cross-builds, and the
   host-side loop (B1) is where sanitizers could later run natively if ever
   wanted.

### B5 — No CI
Nothing catches a broken build until a session trips over it. Minimal,
hobby-scale recipe (one GitHub Actions workflow):
- Job 1: native — build `fujinet-nio-lib TARGET=linux`, run `make test-host`
  (from B1).
- Job 2: cross — use a prebuilt amiga-gcc container image to build the compat
  lib and `make -C apps`.
- **Explicitly out of scope:** FS-UAE in CI. Kickstart ROM and Workbench
  `serial.device` are copyrighted and can't live in the repo or CI secrets
  comfortably; the emulator loop stays a local/agent tool.

This is optional. If it feels heavy, do B1–B3 and skip CI until a second
contributor appears.

### B6 — ESP32: defer, but write down what's decided
Per the brief: no flash/monitor/partition workflow design now. Create
`contracts/esp32-target.md` (one page) recording only settled facts: fujinet-nio
is the firmware and its build system is Mark's (ESP-IDF, upstream-owned); the
Amiga-side code is transport-agnostic through fujinet-nio-lib; PHY candidates
are parallel port (likely) vs Zorro; RS-232 remains the bring-up path.
Everything else is marked "deferred until hardware exists." This gives future
sessions a place to look and a signal not to invent workflow.

## What this buys a Claude Opus session

After B1–B3 + doc updates, every workflow is one memorized command:

| Intent | Command | Cost |
|---|---|---|
| Unit-check compat logic | `make -C libs/fujinet-compat-amiga test-host` | seconds |
| Cross-build everything | `make -C apps` (+ lib prereqs per CLAUDE.md) | seconds |
| Integration check an app | `make -C apps/<app> emu-test` | ~1–2 min |
| Full game verification | `/emu-build-and-boot` skill | minutes |

No discovery, no workflow re-planning — which is the concrete meaning of
"optimized for Claude Opus."
