# Amiga ADF Bootstrap Contract

## Overview

Every Amiga app ADF in this project must include `Devs/serial.device` because
`serial.device` is **not ROM-resident in Kickstart 1.3**. Without it,
`OpenDevice("serial.device", ...)` returns `IOERR_OPENFAIL (-1)` and
`fn_init()` fails with `FN_ERR_NOT_FOUND`.

---

## Copyright notice

`serial.device` is copyrighted software (Commodore-Amiga, Inc.). **Do not
commit ADF files to git.** Add `*.adf` to `.gitignore`. The file must be
extracted from a legitimately-owned Workbench disk image at build time.

---

## Required ADF contents

```
<LABEL>                    VOLUME  OFS
  Devs/                      DIR
    serial.device            5292 bytes  ← from WB 1.3.4
  <appname>                  binary
  s/                         DIR
    startup-sequence         text
```

Bootblock must be installed (xdftool formats with a zero checksum by default,
which is non-bootable).

---

## Build recipe

```bash
# 1. Extract serial.device from Workbench 1.3.4 ADF (one-time per machine)
xdftool /path/to/amiga-os-134-workbench.adf read DEVS/serial.device /tmp/serial.device
# Verify: should be exactly 5292 bytes
ls -la /tmp/serial.device

# 2. Build the app binary first
make -C apps/<appname>

# 3. Create ADF
ADF=apps/<appname>/<appname>.adf
rm -f "$ADF"
xdftool "$ADF" create + format "<LABEL>" ofs

# 4. Add startup-sequence
xdftool "$ADF" makedir s
printf '<appname>\n' > /tmp/startup-sequence
xdftool "$ADF" write /tmp/startup-sequence s/startup-sequence

# 5. Add app binary
xdftool "$ADF" write apps/<appname>/<appname> <appname>

# 6. Add Devs/serial.device
xdftool "$ADF" makedir Devs
xdftool "$ADF" write /tmp/serial.device Devs/serial.device

# 7. Install bootblock (critical — without this the Amiga shows "Please insert Workbench")
xdftool "$ADF" boot install
xdftool "$ADF" boot show   # verify: bootable: True

# 8. Copy to FS-UAE Floppies directory
cp "$ADF" ~/Documents/FS-UAE/Floppies/
```

---

## FS-UAE config

The FS-UAE config file (`emu/config/fsuae-test.fs-uae`) references the ADF by
short filename only. FS-UAE resolves it against `~/Documents/FS-UAE/Floppies/`.
Absolute paths are silently ignored.

```ini
floppy_drive_0 = <appname>.adf
```

---

## Startup-sequence rules

- **Do NOT redirect stdout to `SER:`** — this opens `serial.device` a second time,
  conflicting with `fn_init()`. The program will crash.
- `2>>SER:` is not supported by KS 1.3's shell — omit it.
- Keep the startup-sequence to a single line: the app name and any arguments.

```
http_get "https://example.com/"
```

---

## Diagnosing bootstrap failures

| Symptom | Cause | Fix |
|---------|-------|-----|
| "Please insert Workbench" | Missing/bad bootblock | `xdftool boot install` |
| `fn_init()` → `FN_ERR_NOT_FOUND` | `Devs/serial.device` missing from ADF | Extract from WB 1.3.4 and add to ADF |
| "Software error — task held" on boot | V36+ exec API used, or binary built without `-mcrt=nix13` | See `contracts/amiga-coding-conventions.md` |
| CLI error on startup-sequence line | Redirect to `SER:` or unsupported shell syntax | Remove redirects |
| Error 121 with known-good binary | FS-UAE `.sdf` save-state caching old disk | Delete `~/Documents/FS-UAE/Save States/run/<name>.sdf`; `run.sh` now does this automatically |
| Guru Meditation after `m68k-amigaos-strip` | `strip` corrupts Amiga hunk binaries | Never use `m68k-amigaos-strip`; use `emu/scripts/strip-hunk-symbols.py` if needed |

---

## Local paths

Copy `emu/config/paths.env.example` to `emu/config/paths.env` (gitignored) and
fill in `KICKSTART_ROM` with the path to your KS 1.3 ROM image.

The WB 1.3.4 ADF path used in this project: `/home/jeffp/Downloads/amiga-os-134-workbench.adf`
(not committed; must be present on the build machine).
