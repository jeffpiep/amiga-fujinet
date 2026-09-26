# Amiga ADF Bootstrap Contract

## Overview

Since the September 2026 sync, nio-lib's Amiga transport does not open
`serial.device` itself. It opens **`fujinet-nio.device`**, a resident Exec
broker from `fujinet-nio-driver/amiga/nio.device/`, and the broker opens
`serial.device` on the app's behalf. So a boot disk that reaches FujiNet on
Kickstart 1.3 needs three things the ROM does not supply:

1. **`Devs/serial.device`**, because KS 1.3 does not have it in ROM.
2. **`Devs/fujinet-nio.device`**, the broker.
3. **`C/fujinet-load-resident`**, plus startup-sequence lines that make
   *both* devices resident before the app runs.

`emu/scripts/build-adf.sh` does all of this by default. Offline harnesses that
never touch the network opt out with `ADF_NO_BROKER=1` (see below).

---

## Copyright notice

`serial.device` is copyrighted software (Commodore-Amiga, Inc.). **Do not
commit ADF files to git.** `*.adf` is gitignored. The file is extracted from a
legitimately owned Workbench 1.3.4 image (`WB_ADF` in `emu/config/paths.env`)
at build time.

`fujinet-nio.device` and `fujinet-load-resident` are built from the
`fujinet-nio-driver` submodule's sources and are not committed either.

---

## Required ADF contents

```
<LABEL>                         VOLUME  OFS
  Devs/                           DIR
    serial.device                 5292 bytes  ← from WB 1.3.4
    fujinet-nio.device            ~13 KB      ← fujinet-nio-driver
  c/                              DIR
    fujinet-load-resident         ~12 KB      ← built by make/amiga.mk
    Makedir, Assign, Echo                     ← from WB 1.3.4 (for prefixes)
  <appname>                       binary
  s/                              DIR
    startup-sequence              text
```

The bootblock must be installed; xdftool formats with a zero checksum, which
is not bootable.

---

## Startup-sequence

The broker lines always come first, in this order:

```
C:fujinet-load-resident DEVS:serial.device serial.device
C:fujinet-load-resident DEVS:fujinet-nio.device fujinet-nio.device
<EMU_STARTUP_PREFIX lines, if any>
<appname> <EMU_STARTUP_ARGS>
```

**Why `serial.device` is made resident first (required on KS 1.3).** The
broker's worker is a plain Exec Task created with `AddTask`, and it calls
`OpenDevice("serial.device")` lazily on the first exchange. On KS 1.3 that
device is disk-based, so opening it makes ramlib load it from `DEVS:` and
reply to the caller's `pr_MsgPort`. A Task has no `pr_MsgPort`, so the reply
goes through garbage and the machine gurus with **#00000003** on the first
exchange. Loading `serial.device` resident from a Process (the Shell running
the startup-sequence) means the worker's later `OpenDevice` finds it in the
device list and never touches ramlib. The durable fix is upstream (the worker
should be a Process); until it lands this line is mandatory.

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
| `fujinet-nio.device` | `make -C fujinet-nio-driver/amiga ../build/amiga/fujinet-nio.device` | Uses `-nostartfiles`; builds with our amiga-gcc. |
| `fujinet-load-resident` | `make/amiga.mk` rule `$(LOAD_RESIDENT)` → `build/amiga/` | Built from the driver's `tools/fujinet-load-resident.c` with `-mcrt=nix13`. The driver's own rule hardcodes `-mcrt=clib2`, which our toolchain does not ship. |

`make/amiga.mk` exposes both as `$(BROKER_DEVICE)` and `$(LOAD_RESIDENT)`,
and `emu/template/emu.mk`'s `emu-adf` target lists them as prerequisites and
passes them to `build-adf.sh`.

**Minimum driver revision.** Two driver bugs made the broker unloadable on
KS 1.3. Both are fixed on `jeffpiep/fujinet-nio-driver`
`feature/resident-endskip-in-code-hunk`, which is the pinned revision until
upstream merges them ([markjfisher/fujinet-nio-driver#1](https://github.com/markjfisher/fujinet-nio-driver/pull/1)):

- `rt_EndSkip` must point past the ROMTag, inside the first hunk
  (otherwise "No matching resident tag").
- `device_init` must set `ln_Name`, `ln_Type`, the version and the ID string,
  because KS 1.3's `InitResident` does not copy them from the ROMTag
  (otherwise the device is added unnamed and cannot be opened).

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
| Guru #00000003 on the first exchange | `serial.device` not made resident before the broker's first open | Add the `serial.device` load line, first |
| `fn_test` prints "FujiNet is ready." but nothing reaches the server | `fn_is_ready()` sends no traffic; the broker opens serial lazily | `fn_test` now also calls `fn_clock_get()`; judge by `fujibus: receive:` |
| "Software error — task held" on boot | V36+ exec API used, or binary built without `-mcrt=nix13` | See `contracts/amiga-coding-conventions.md` |
| CLI error on startup-sequence line | Redirect to `SER:` or unsupported shell syntax | Remove redirects |
| Error 121 with known-good binary | FS-UAE `.sdf` save-state caching old disk | `run.sh` deletes `~/Documents/FS-UAE/Save States/run/<name>.sdf` automatically |
| Guru Meditation after `m68k-amigaos-strip` | `strip` corrupts Amiga hunk binaries | Never use `m68k-amigaos-strip`; use `emu/scripts/strip-hunk-symbols.py` if needed |

---

## Local paths

Copy `emu/config/paths.env.example` to `emu/config/paths.env` (gitignored) and
fill in `KICKSTART_ROM` (KS 1.3 ROM) and `WB_ADF` (Workbench 1.3.4 ADF).
