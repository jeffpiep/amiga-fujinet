# Handoff — mount `fujinet-disk.device` on KS 1.3 and settle `GlobVec`

> **Status: ARCHIVED — superseded 2026-09-26.** Done: `GlobVec = -1` confirmed fatal on 1.3; the FFS
> alternative was retracted. Results are in `docs/response-to-mark-disk-device-review.md` and
> markjfisher/fujinet-nio-driver#3. The 1.3 MountList rules are in `contracts/amiga-adf-bootstrap.md`,
> and the harness is `apps/disk_test`.

**Status: Not started (2026-09-26).** Point-in-time snapshot; archive per
`docs/README.md` §2 once its facts are rehomed.
**Branch:** `feature/globvec-disk-device` (parent, cut from `dev` at `077aed0`).
**Read first:** `docs/response-to-mark-disk-device-review.md` point 2 and the
update section at its top; `contracts/amiga-adf-bootstrap.md`;
`fujinet-nio-driver/amiga/README.md`.

## The question

Mark's `DN0` MountList (`fujinet-nio-driver/amiga/config/DN0`) sets
`GlobVec = -1` and has no `FileSystem` line. Our review said, as a *belief to
be confirmed on a 1.3 boot*:

> With no `FileSystem` line, 1.3's `Mount` gives `DN0:` the ROM file system,
> which on 1.3 is the BCPL OFS handler. `GlobVec = -1` means "no BCPL global
> vector", which is what FFS (C/asm) wants. The 1.3 BCPL handler needs the
> default instead. Fix: omit `GlobVec`, or add `FileSystem = L:FastFileSystem`
> and keep it.

The goal is a **confirmed or retracted** answer, backed by a KS 1.3 boot in
FS-UAE, reported to Mark as an update in
`docs/response-to-mark-disk-device-review.md` (and on his repo only with the
user's OK — posting there is public).

The three MountList variants to compare:

1. Mark's `DN0` as-is (`GlobVec = -1`, no `FileSystem`).
2. `GlobVec` omitted.
3. `GlobVec = -1` plus `FileSystem = L:FastFileSystem` (FFS from the WB 1.3.4
   disk; FFS reads OFS media).

For each: does `Mount DN0:` succeed, does `Dir DN0:` list the image, does
`Type` of a file work. Record exactly what happens, Guru or not.

While `DN0:` is up, `Buffers = 5` vs 20–30 is the other open point in the
same section of the review (a rough `Dir` timing is enough).

## What is already true

- **The broker works on 1.3.** PR #44 made every networked ADF install
  `fujinet-nio.device`; `make emu-test` passes for all five apps. The driver
  submodule is pinned at our fork's `feature/resident-endskip-in-code-hunk`
  (riding markjfisher/fujinet-nio-driver#1), which also fixes
  `fujinet-disk.device`'s ROMTag and unnamed-node bugs on 1.3.
- `make -C fujinet-nio-driver/amiga ../build/amiga/fujinet-disk.device`
  builds the disk device with our toolchain (`-nostartfiles`).
- `emu/scripts/build-adf.sh` already supports `ADF_STATIC_DIR` (extra files)
  and `EMU_STARTUP_PREFIX` (lines after the broker lines, before the app).
  That is how the broker proof of concept was staged, and it is the easy way
  to add `Devs/fujinet-disk.device`, a MountList, and load/mount lines.
- The broker's `serial.device` Guru (#2 upstream) is worked around by the
  resident-load line `build-adf.sh` already emits.
- Amiberry is available for post-Guru memory inspection; see
  `docs/testing.md` "Amiberry".

## Unknowns to settle before the GlobVec comparison means anything

These are **unverified** — check each rather than assuming:

- **1.3 `Mount` syntax.** `config/DN0` is a 2.0+ DOSDrivers-style file. KS
  1.3's `Mount` reads entries from `DEVS:MountList` terminated by `#`, and may
  not accept every keyword (check `DosType`, `StackSize`, `BufMemType`).
  `C/Mount` comes from the WB 1.3.4 ADF; `build-adf.sh` only copies
  `Makedir`, `Assign` and `Echo` today.
- **Getting media into slot 1.** End users use `FMOUNT` from
  `nio-core-apps`, which is not in this repo. `fujinet-mount` is the driver's
  diagnostic tool; its rule hardcodes `-mcrt=clib2`, so it would need the
  same `-mcrt=nix13` treatment `make/amiga.mk` gives `fujinet-load-resident`,
  and it has never been run on 1.3. Also check whether `fujinet-nio`'s
  config can pre-mount an image in slot 1 server-side, which would avoid the
  tool entirely.
- **A test image.** A plain OFS 880 KB ADF built with `xdftool` (a file or two
  in it) served by `fujinet-nio`. Don't use a copyrighted Workbench disk.
- **Does `fujinet-disk.device` open the broker safely?** Its worker is also a
  plain Task. The broker is already resident, so no ramlib load should happen,
  but confirm it rather than assume it.

## Suggested shape of the work

Proof of concept first, by hand, as with the broker: stage files through
`ADF_STATIC_DIR` + `EMU_STARTUP_PREFIX`, boot with `emu/run.sh` or
`emu/drive.sh` (screenshots), iterate. Only once the answer is known decide
whether a permanent `apps/` harness (a `disk_test` with an `emu-test` pass
pattern) is worth adding. If it is, write the contract change first.

Stay on topic: issue #2 (the broker's worker Task) is Mark's; don't fix it
here.
