# Amiga ADF Bootstrap Contract

## Overview

Since the September 2026 sync, nio-lib's Amiga transport does not open
`serial.device` itself. It opens **`fujinet-nio.device`**, a resident Exec
broker from `fujinet-nio-driver/amiga/nio.device/`, and the broker opens
`serial.device` on the app's behalf. So a boot disk that reaches FujiNet on
Kickstart 1.3 needs three things the ROM does not supply:

1. **`Devs/serial.device`**, because KS 1.3 does not have it in ROM.
2. **`Devs/fujinet-nio.device`**, the broker.
3. **`C/fujinet-load-resident`**, plus a startup-sequence line that makes
   the broker resident before the app runs.

`emu/scripts/build-adf.sh` does all of this by default. Offline harnesses that
never touch the network opt out with `ADF_NO_BROKER=1` (see below).

---

## Committed ADFs and Workbench files

`serial.device` and the Workbench `C:` commands these disks carry are
extracted at build time from a Workbench 1.3.4 image (`WB_ADF` in
`emu/config/paths.env`). **The project treats them as redistributable**
(decided 2026-09-27), so ADFs that contain them may be committed.

Only the tester disks in `release/adf/` are committed, rebuilt with
`make -C release`. Per-app ADFs under `apps/` are build output and stay
gitignored (`apps/**/*.adf`), like the other binaries.

`fujinet-nio.device` and `fujinet-load-resident` are built from the
`fujinet-nio-driver` submodule's sources. They are not committed as loose
files, but the release ADFs contain them.

---

## Required ADF contents

```
<LABEL>                         VOLUME  OFS
  Devs/                           DIR
    serial.device                 5292 bytes  ← from WB 1.3.4
    fujinet-nio.device            ~13 KB      ← fujinet-nio-driver
  c/                              DIR
    fujinet-load-resident         ~12 KB      ← fujinet-nio-driver
    Makedir, Assign, Echo                     ← from WB 1.3.4 (for prefixes)
  <appname>                       binary
  s/                              DIR
    startup-sequence              text
```

The bootblock must be installed; xdftool formats with a zero checksum, which
is not bootable.

---

## Startup-sequence

The broker line always comes first:

```
C:fujinet-load-resident DEVS:fujinet-nio.device fujinet-nio.device
<EMU_STARTUP_PREFIX lines, if any>
<appname> <EMU_STARTUP_ARGS>
```

**No `serial.device` line.** Until the October 2026 sync the broker's worker
was a plain Task, which gurus #00000003 opening the disk-based `serial.device`
on KS 1.3, so disks loaded `serial.device` resident first. Upstream now runs
the worker as a Process on 1.3
([markjfisher/fujinet-nio-driver#2](https://github.com/markjfisher/fujinet-nio-driver/issues/2),
closed), which opens it from `DEVS:` itself. The line is now harmful as well
as unneeded: the current `fujinet-load-resident` checks that the device
registered itself, stock `serial.device` does not, and the load fails with
return code 20.

**Why the broker is loaded explicitly.** `fujinet-nio.device` is not
auto-loaded from `DEVS:`; nio-lib's `OpenDevice` only finds it once it is in
the device list.

Rules that still hold:

- **Do NOT redirect stdout to `SER:`**. That opens `serial.device` a second
  time, conflicting with the broker.
- `2>>SER:` is not supported by KS 1.3's shell. Omit it.
- `fujinet-load-resident` prints "Resident loaded" whenever `InitResident`
  returns non-NULL. That does **not** prove the device can be opened: before
  the driver fix below, a device came up unnamed and `OpenDevice` failed with
  "Device not found" after a successful-looking load.

---

## Where the binaries come from

| File | Built by | Notes |
|------|----------|-------|
| `fujinet-nio.device`, `fujinet-load-resident`, `fujinet-disk.device` | `make -C fujinet-nio-driver amiga AMIGA_PROFILE=wb13` → `fujinet-nio-driver/build/amiga/wb13/` | The driver only builds into a named Workbench profile. `wb13` builds its tools with `-mcrt=nix13`, so they run on KS 1.3 and need no clib2. |

`make/amiga.mk` exposes them as `$(BROKER_DEVICE)`, `$(LOAD_RESIDENT)` and
`$(DISK_DEVICE)`, and `emu/template/emu.mk`'s `emu-adf` target lists the
first two as prerequisites and passes them to `build-adf.sh`.

**Minimum driver revision.** Three driver bugs made the broker unusable on
KS 1.3. The first two are upstream
([markjfisher/fujinet-nio-driver#1](https://github.com/markjfisher/fujinet-nio-driver/pull/1),
merged 2026-09-27):

- `rt_EndSkip` must point past the ROMTag, inside the first hunk
  (otherwise "No matching resident tag").
- `device_init` must set `ln_Name`, `ln_Type`, the version and the ID string,
  because KS 1.3's `InitResident` does not copy them from the ROMTag
  (otherwise the device is added unnamed and cannot be opened).

The third is on `jeffpiep/fujinet-nio-driver` `feature/wb13-own-dosbase`,
the pinned revision until upstream takes it:

- The broker must *define* `DOSBase`, not leave it a tentative (common)
  symbol. Our amiga-gcc satisfies a common `DOSBase` from libnix's
  `__dosbase.o`, whose initial value is the auto-open sentinel `-1`. Under
  `-nostartfiles` nothing replaces it, so the 1.3 worker-process path skips
  `OpenLibrary("dos.library")` and dereferences `-1`: Exception 3 inside
  `InitResident`, on the broker's load line. Check with
  `grep __dosbase fujinet-nio-driver/build/amiga/wb13/fujinet-nio.device.map`,
  which should print nothing.

---

## Disk device (`DN0:`) on KS 1.3

An ADF that mounts FujiNet media as a DOS volume adds `fujinet-disk.device`
on top of the broker. Settled on a KS 1.3 boot on 2026-09-26; see
`docs/response-to-mark-disk-device-review.md` and
[markjfisher/fujinet-nio-driver#3](https://github.com/markjfisher/fujinet-nio-driver/issues/3).

Extra ADF contents:

```
  Devs/fujinet-disk.device      ← fujinet-nio-driver
  Devs/MountList                ← DN0 entry, below
  c/Mount                       ← from WB 1.3.4 (ADF_WB_FILES="C/Mount")
  <a mount tool>                ← sends FUJINET_DISK_CMD_MOUNT
```

Startup-sequence order, after the broker lines:

```
C:fujinet-load-resident DEVS:fujinet-disk.device fujinet-disk.device
<mount tool> 0 host:/<image>.adf
Mount DN0:
```

Rules:

- **Load the disk device after the broker.** Its worker Task opens
  `fujinet-nio.device`. That device is already resident, so no ramlib load
  happens and the open is safe (booted).
- **Mount the media client-side before the first access to `DN0:`.** The
  driver keeps its own per-unit mounted flag. It never adopts the server's
  restored runtime mounts, so pre-mounting on the server does nothing.
  `apps/disk_test` has a 1.3-safe mount step. The `wb13` driver profile now
  builds `fujinet-mount` for 1.3 too, but it has not been booted here.
- **`DEVS:MountList` is 1.3 syntax:** one entry per name, terminated by `#`.
  Mark's `config/DN0` keywords are all accepted, including `DosType`,
  `StackSize` and `BufMemType`.
- **No `GlobVec` line with the ROM file system.** On 1.3 the ROM handler is
  BCPL. `GlobVec = -1` makes it guru **#00000003** on the first access to
  `DN0:`, before any sector is read, even though `Mount` itself succeeded.
- **`FileSystem = L:FastFileSystem` only for FFS media.** The 1.3 FFS
  accepts `DOS\1` only. It rejects OFS images with "Not a DOS disk". With FFS
  media, use `DosType = 0x444F5301` and keep `GlobVec = -1`.
- **`Buffers = 30`, not 5.** At 19200 baud each block costs about 0.35 s. A
  repeat `Dir` of a 20-file volume drops from 9 s to 2 s.

The working 1.3 entry, for OFS media:

```
DN0:
    Device = fujinet-disk.device
    Unit = 0
    Flags = 0
    Surfaces = 2
    BlocksPerTrack = 11
    Reserved = 2
    Interleave = 0
    LowCyl = 0
    HighCyl = 79
    Buffers = 30
    BufMemType = 1
    DosType = 0x444F5300
    StackSize = 32768
    Priority = 5
#
```

### `disk_test` verdict

`emu/run.sh` judges a run by grepping the **server** log. A DOS-level check
on the Amiga is invisible there. So `disk_test check` reports its verdict
through the device under test: it sends `FUJINET_DISK_CMD_MOUNT` for
`host:/disk_test-PASS` or `host:/disk_test-FAIL` on unit 1. `fujinet-nio`
logs the URI of every mount request (`uri='host:/disk_test-PASS'`), whether
or not the file exists. Neither file exists, so the mount fails harmlessly
and unit 1 stays empty. A Guru or a hang sends no verdict, and the run fails
on timeout.

`fujinet-nio`'s `host:` root is `./fujinet-data` relative to its working
directory. `make -C apps/disk_test emu-test` runs it from `apps/disk_test/`,
so the test image is generated at `apps/disk_test/fujinet-data/` (gitignored).

---

## `build-adf.sh` interface

Required: `APP_NAME`, `APP_BINARY`. From `paths.env`: `WB_ADF`.

| Variable | Meaning |
|----------|---------|
| `BROKER_DEVICE` | Path to `fujinet-nio.device`. Required unless `ADF_NO_BROKER=1`. |
| `LOAD_RESIDENT` | Path to `fujinet-load-resident`. Required unless `ADF_NO_BROKER=1`. |
| `ADF_NO_BROKER` | `1` omits the broker, the loader and both load lines. For offline harnesses only (`gallery-adf`, `preview-adf`). |
| `EMU_STARTUP_PREFIX` | AmigaDOS lines run after the broker lines, before the app. |
| `EMU_STARTUP_ARGS` | Arguments appended to the app's line. |
| `ADF_STATIC_DIR` | Tree copied to the ADF root. |
| `ADF_WB_FILES` | Extra files copied from `WB_ADF` to the same path, space-separated (e.g. `C/Mount L/FastFileSystem`). A missing file is a build error. |
| `ADF_OUT` | Output path. |

A missing `BROKER_DEVICE` or `LOAD_RESIDENT` is a build error, not a silent
omission: an ADF without the broker boots fine and then cannot reach FujiNet,
which is the failure this contract exists to prevent.

---

## FS-UAE config

The FS-UAE config (generated by `emu/run.sh`) references the ADF by short
filename only. FS-UAE resolves it against `~/Documents/FS-UAE/Floppies/`, and
silently ignores absolute paths.

```ini
floppy_drive_0 = <appname>.adf
```

---

## Diagnosing bootstrap failures

| Symptom | Cause | Fix |
|---------|-------|-----|
| "Please insert Workbench" | Missing or bad bootblock | `xdftool boot install` |
| `fn_init()` → "Device not found" | Broker not resident, or driver older than the minimum revision (unnamed device node) | Check the startup-sequence load line; rebuild the device from the pinned driver |
| `fujinet-load-resident`: "No matching resident tag" | Driver older than the `rt_EndSkip` fix | Rebuild the device from the pinned driver |
| "Resident initialized but not registered in DeviceList: serial.device", return code 20 | A pre-October-2026 `serial.device` load line | Remove it (see "Startup-sequence") |
| "Software error — task held" on the broker's load line; FS-UAE logs `Exception 3` | Broker linked libnix's `DOSBase` | Use a driver with the `DOSBase` fix (see "Minimum driver revision") |
| `fn_test` prints "FujiNet is ready." but nothing reaches the server | `fn_is_ready()` sends no traffic; the broker opens serial lazily | `fn_test` now also calls `fn_clock_get()`; judge by `fujibus: receive:` |
| "Software error — task held" on boot | V36+ exec API used, or binary built without `-mcrt=nix13` | See `contracts/amiga-coding-conventions.md` |
| CLI error on startup-sequence line | Redirect to `SER:` or unsupported shell syntax | Remove redirects |
| Error 121 with known-good binary | FS-UAE `.sdf` save-state caching old disk | `run.sh` deletes `~/Documents/FS-UAE/Save States/run/<name>.sdf` automatically |
| Guru Meditation after `m68k-amigaos-strip` | `strip` corrupts Amiga hunk binaries | Never use `m68k-amigaos-strip`; use `emu/scripts/strip-hunk-symbols.py` if needed |

---

## Local paths

Copy `emu/config/paths.env.example` to `emu/config/paths.env` (gitignored) and
fill in `KICKSTART_ROM` (KS 1.3 ROM) and `WB_ADF` (Workbench 1.3.4 ADF).
