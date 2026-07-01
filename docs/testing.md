# Testing Practice

How we test across this repo. This is an **evergreen** doc — the *why* and the
conventions live here; the canonical run commands live in the top-level
`CLAUDE.md` (`## Testing`). Keep the two in sync when either changes.

## The problem this solves

Amiga apps and libraries compile with Bebbo `m68k-amigaos-gcc` to 68000 Hunk
executables that **cannot run on the x86-64 host** — only under FS-UAE emulation.
Historically that made the *only* way to test any Amiga logic a slow emulator
boot against a live `fujinet-nio` backend. But much of the code is pure logic
(URL prefix parsing, error/mode mapping) that touches no AmigaOS API and can be
unit-tested natively in milliseconds. This practice adds that fast tier and names
the tiers so they stay separable.

## The three tiers

| Tier | Target | Runs on | Speed | Location |
|------|--------|---------|-------|----------|
| **T1 — host unit** | `make test-host` | native `cc` | ms, CI-safe, no backend | `<module>/test/host/test_*.c` |
| **T2 — emulator smoke** | `make emu-test` | FS-UAE + live `fujinet-nio` | 60–90 s | `<app>/` via `emu/template/emu.mk` |
| **T3 — nio host ctest** | `./build.sh -p …` | native, in submodule | seconds | `fujinet-nio/tests/` |

The target-name prefix is the load-bearing convention: **`test-host` = fast/CI,
`emu-test` = slow/hardware-in-the-loop.** Never overload one target to do both.

- **T1** is what CI runs and what you run constantly while developing pure logic.
- **T2** already existed (the `make emu-test` → `emu/run.sh` FS-UAE harness that
  greps `fujinet.log` for a pass pattern). It stays as-is; T1 does not replace it.
- **T3** is the `fujinet-nio` submodule's own doctest/CTest suite. We *reference*
  it, never modify it from the parent repo (see "Submodule boundary").

## T1 — host unit tests

### The pure-logic firewall

A `.c` file is T1-eligible **iff it includes no AmigaOS headers** — nothing from
`proto/*`, `exec/*`, `clib/*`, `dos/*`, or `devices/*`. Those pull in the m68k
ABI and can't compile or link natively.

- `libs/fujinet-compat-amiga/src/fn_network.c` passes — pure C plus the FujiNet
  headers. Its statics (`strip_prefix`, `map_mode`, `map_err`, `slot_find`) are
  exactly the kind of thing T1 targets.
- `libs/fujinet-compat-amiga/src/fn_fuji.c` does **not** — it includes
  `proto/dos.h`. Code like that is tested at T2.

When you want new logic to be host-testable, keep the AmigaOS calls in a separate
translation unit from the pure logic.

### Layout and the harness

```
libs/fujinet-compat-amiga/test/host/
  fn_test.h        # tiny single-header C99 assert harness (CHECK / CHECK_EQ)
  test_network.c   # a T1 test; #includes ../../src/fn_network.c
```

The harness (`fn_test.h`) increments a global `fn_failures` on every failed check
and `main` returns that count — non-zero means the test failed. This deliberately
formalizes the printf/failure-counter pattern already used by the T2 smoke test
in `test/compat_test.c`. Output is one `PASS <name> (N checks)` line, or a `FAIL
file:line: …` line per failure.

### How a T1 test reaches `static` helpers

The test `.c` **`#include`s the module `.c` directly** (single translation unit).
That makes file-local `static` functions reachable with **zero production-code
change** — the statics stay static in the real m68k build. The tradeoff: the
module's other functions come along, so the test provides small **inert linker
stubs** for the transport symbols they reference (`fn_open`, `fn_read`, …). Those
stubs exist only to satisfy the linker — we do **not** mock transport behavior in
T1. Anything whose behavior depends on the backend belongs in T2.

### Adding a T1 test

1. Create `test/host/test_<thing>.c`, `#include "fn_test.h"` (with
   `#define FN_TEST_MAIN` in exactly one file), and `#include` the module under
   test.
2. Add inert stubs for any transport symbols the included module references.
3. Write `CHECK` / `CHECK_EQ` assertions; `return fn_test_report("test_<thing>")`.
4. Add the `.test` binary to `HOST_TESTS` in the module's Makefile.

## T2 — emulator smoke tests

Unchanged by this practice. Each app's Makefile includes `emu/template/emu.mk`,
sets an `EMU_PASS_PATTERN`, and `make emu-test` boots the app in FS-UAE via
`emu/run.sh`, polling the FujiNet/serial logs for that pattern. Slow, and needs
`fujinet-nio` built and running. See `CLAUDE.md` "Emulator Testing" and the
`emu-build-and-boot` skill. `libs/fujinet-compat-amiga`'s m68k `make test`
cross-compiles `compat_test.c`; it is run on-target via `apps/compat_test`.

## T3 — the fujinet-nio host suite (submodule)

`fujinet-nio/tests/` holds ~33 `test_*.cpp` on **doctest 2.4.12**, wired into a
`fujinet-nio-tests` CTest target gated by `FN_BUILD_TESTS`. `./build.sh -p
fujibus-rs232-debug` builds and runs them. This is a fast native suite, but it
lives in the submodule.

## Why a tiny C harness (not doctest or Unity)

- **doctest** is what T3 uses, but it's C++ — adopting it for T1 would force a C++
  toolchain and `extern "C"` wrapping around every C unit under test.
- **Unity** would add vendored third-party files and runner-generation ceremony.
- The **tiny single-header C99 harness** is zero-dependency, matches the C99
  code-under-test and the KS 1.3 tiny-footprint ethos, needs nothing beyond `cc`
  for CI, and formalizes a pattern the repo already used by hand.

We keep language-consistency (C tests for C code, C++ doctest for the C++ server)
rather than forcing a single framework across the C/C++ boundary.

## Repo-wide entry points

```
make test        # == make test-host (the fast CI default)
make test-host   # all T1 host unit tests, every module in HOST_TEST_DIRS
make emu-test    # all T2 emulator smoke tests (slow; needs FS-UAE + fujinet-nio)
```

New T1-bearing modules get added to `HOST_TEST_DIRS` in the top-level `Makefile`.
T3 is intentionally **not** aggregated here — it's driven from the submodule.

## Submodule boundary

All parent-repo testing work is confined to `libs/fujinet-compat-amiga/`,
`apps/*`, `emu/`, and the top-level aggregation files. Test changes *inside* a
submodule follow that submodule's own feature-branch → upstream-PR flow:

| Change | Lands in | Flow |
|--------|----------|------|
| T1 harness/tests, `test-host` targets, top-level `Makefile`, docs | parent repo | feature branch → squash PR |
| New `test_*.cpp` / CTest / `FN_BUILD_TESTS` changes | `fujinet-nio` | upstream PR |
| Host-build/test changes to the Amiga lib itself | `fujinet-nio-lib` | upstream PR |

T3 is referenced from the parent repo, never modified there.

## Future work

CI is designed to be trivial to add: a workflow that runs `make test-host` needs
nothing beyond a host `cc`. The slow T2/T3 tiers stay opt-in / local until the
emulator harness is worth wiring into CI.
