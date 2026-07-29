# Implementation Plan: Amiga Disk Device (`fujinet-disk.device`)

**Depends on:** `fujinet-nio-lib` Amiga transport (jeffpiep PR #1, open); a
block-level endpoint in `fujinet-nio` (not yet written)
**Blocks:** FujiNet "boot disk" / config-disk parity with Atari, Apple, MSDOS
and BBC
**Status:** Design in discussion (2026-07-28) — layering agreed, geometry and
media-change semantics open. Not a contract yet; promote to
`contracts/amiga-disk-device.md` once the open questions below are closed.

---

## Goal

Present FujiNet storage to the Amiga as a **normal AmigaDOS volume**, so that
existing tools, the `Mount` command, and the ROM filesystem do the heavy
lifting. On a stock Amiga 500 running Kickstart 1.3 the user should be able to
boot a floppy and have `DN0:` appear, backed by blocks served over the wire.

Longer term the same volume becomes bootable directly from FujiNet hardware,
with no floppy involved.

---

## Why this shape

From design discussion with Thom Cherryhomes (2026-07-28). The key decision is
to split the problem at the **block boundary** rather than the file boundary:

| Layer | Runs on | Responsibility |
|-------|---------|----------------|
| OFS / FFS | Amiga (ROM) | directories, files, bootblock, validation |
| AmigaDOS mount (`DN0:`) | Amiga | binds a filesystem to a device + unit |
| `fujinet-disk.device` | Amiga | exec device: `CMD_READ` / `CMD_WRITE` against `io_Offset` / `io_Length` |
| FujiBus transport | Amiga ↔ server | RS-232 today; parallel or Zorro later |
| `fujinet-nio` | server | serve and accept fixed-size blocks by number |

Two consequences worth stating plainly, because both cut work:

1. **The server does not need to understand AmigaDOS.** It serves 512-byte
   blocks; the Amiga's own ROM filesystem supplies all structure. There is no
   need to implement OFS/FFS, bootblock checksums, or hash chains server-side.
   A sector is a sector, which is likely reusable against the disk-image work
   already done upstream for MSDOS and BBC.
2. **The transport is swappable underneath a stable interface.** The device's
   block API is the boundary; moving from RS-232 to a parallel-port or Zorro
   PHY replaces what sits below it and leaves AmigaDOS, the filesystem, and
   every application above it untouched.

This supersedes an earlier assumption (recorded here so it is not re-derived)
that the server would need to generate whole AmigaDOS disk images. It does not.

---

## Boot story

Two eras, and the difference matters for what is achievable *now*.

### Now — RS-232, no FujiNet hardware: boot floppy

Nothing exists at reset for the machine to autoboot from, so the driver has to
arrive from a floppy. The ADF carries:

```
Devs/fujinet-disk.device     the exec device
Devs/MountList               DN0: entry — device, unit, geometry, DosType
Devs/serial.device           already required, see contracts/amiga-adf-bootstrap.md
S/Startup-Sequence           ... Mount DN0:
```

This is the standard pre-RDB third-party-hard-drive pattern: the MountList
describes the device and geometry, and AmigaDOS registers the volume. Kickstart
1.3 uses `Devs/MountList` and the `Mount` command — the `DEVS:DOSDrivers/`
directory is a 2.0+ mechanism and is not available to us.

### Later — FujiNet hardware: autoboot ROM

The Amiga has supported booting from an arbitrary block device since Kickstart
1.3, via an autoboot ROM on the card that registers a BootNode with
`expansion.library` at reset. (It was intended for 1.2; a register bug in the
expansion.library base pointer, found after tapeout, pushed it to 1.3. The
A2090A SCSI/ST506 card was the first to use it, and the Rigid Disk Block
standardised autoboot metadata and partition tables on hard disks.)

When FujiNet hardware with an autoboot ROM exists, the *same* `.device` is
injected from ROM instead of loaded from floppy, and the machine boots straight
into a FujiNet-served volume. No Amiga-side rework above the device layer.

---

## Phases

**Phase 1 — block device over RS-232.** `fujinet-disk.device` supporting
`CMD_READ` / `CMD_WRITE` (and the trackdisk subset decided in Q3), backed by
the existing FujiBus transport. Exec device, KS 1.3 API floor, same constraints
as `contracts/amiga-coding-conventions.md`. Validate standalone — read and
write known blocks — before involving AmigaDOS.

**Phase 2 — AmigaDOS mount.** MountList entry, `Mount DN0:`, `Format`, then
ordinary `Dir` / `Copy` / `Type` against the volume. This is where geometry
(Q1) has to be committed to.

**Phase 3 — boot ADF.** Package device + MountList + startup-sequence into a
bootable ADF per `contracts/amiga-adf-bootstrap.md`, with FujiNet CLI utilities
on the volume. T2 emulator coverage per `docs/testing.md`.

**Phase 4 — autoboot ROM.** Deferred until hardware exists; see
`contracts/esp32-target.md`. Adds a BootNode/DiagArea ROM image and RDB-style
metadata; no change to the layers above.

---

## What `fujinet-nio` needs to provide

For upstream (Mark) — the Amiga side needs, at minimum:

- Read block N of a mounted image into a buffer.
- Write a buffer to block N.
- Report image geometry, or at least total block count, so the MountList and
  the device agree on size.

Block size is expected to be 512 bytes to match Amiga sector size and the
existing FujiBus read/write shape. Anything beyond this (formatting, image
creation, host/slot management) is reusable from the existing disk work and is
not Amiga-specific.

---

## Open questions

Nothing below is decided. Each names who or what closes it.

**Q1 — Volume geometry.** Does `DN0:` present as an 880K DD floppy (1760
blocks, familiar and interchangeable with real ADFs), or as a larger
hard-drive-style volume? Larger is more useful; floppy geometry is easier to
round-trip with existing tooling and images. *Closes with: Jeff + Thom.*

**Q2 — Throughput at 19200 baud.** ≈1.9 KB/s, so a single 512-byte block is
roughly 270 ms before protocol overhead, and filesystem access touches many
blocks. Is the RS-232 era targeting a small, mostly-read config volume — with
caching, or a RAM-backed shadow — rather than a general-purpose disk? This
should be measured, not guessed, once Phase 1 can move blocks. *Closes with:
measurement in Phase 1.*

**Q3 — How much of the trackdisk command set.** `CMD_READ` / `CMD_WRITE` are
required. Whether to implement `TD_CHANGESTATE`, `TD_CHANGENUM`,
`TD_PROTSTATUS`, `CMD_UPDATE` and friends depends on Q4 and on what the ROM
filesystem actually calls. *Closes with: Phase 1 experimentation.*

**Q4 — Media change.** Is `DN0:` a fixed volume for the session, or can the
server swap the backing image underneath a running system (FujiNet's "mount a
different disk to slot N" model)? Supporting swap means real media-change
signalling and invalidating the filesystem's cached blocks. *Closes with: Jeff
+ Thom + Mark, since it shapes the NIO-side API too.*

**Q5 — Write support scope.** Read-only would sidestep most cache-coherency
and media-change risk for a first cut. Is write needed in Phase 1, or deferred
to Phase 2? *Closes with: Jeff.*

**Q6 — Where the device source lives.** `libs/fujinet-disk-device/` in this
repo alongside the compat layer, or contributed into `fujinet-nio-lib` as an
Amiga platform component? The former is easier to iterate on; the latter is
where transport-adjacent code has gone so far. *Closes with: Jeff + Mark.*

---

## References

- `contracts/amiga-adf-bootstrap.md` — ADF build recipe, `serial.device`
  requirement, startup-sequence rules
- `contracts/amiga-coding-conventions.md` — KS 1.3 API floor, memory and stack
  constraints
- `contracts/fujibus-protocol.md` — FujiBus + SLIP framing
- `contracts/esp32-target.md` — PHY candidates and what a faster transport
  changes
- `docs/testing.md` — where device and mount tests belong across T1/T2
