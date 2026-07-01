# Battleship Emulator Test Debug Plan

> **Status: ARCHIVED — superseded 2026-07-01.** The Error 121 bug this plan
> tracks is fixed; `make emu-test` passes. Durable build steps have been
> rehomed to `CLAUDE.md` (Build Commands → battleship). Kept as a historical
> record of the debug session.

Branch: `worktree-track1b`  
Goal: `make emu-test` PASS — `fujibus: receive:` appears in fujinet.log

---

## Current State (as of 2026-06-29)

The battleship binary builds but `make emu-test` prints:
```
Unable to load battleship: Error code 121
1>
```

### What's committed

| File | Status |
|------|--------|
| `emu/scripts/build-adf.sh` | `ADF_STATIC_DIR` support — clean, working |
| `emu/template/emu.mk` | `ADF_STATIC_DIR` pass-through — clean |
| `apps/battleship/amiga/Makefile` | WIP — see issues below |
| `libs/fujinet-compat-amiga/src/fn_fuji.c` | WIP — see issues below |

### Known bugs in the WIP commits

**Bug 1 — Missing `Assign ENVARC: BATTLESHIP:` in startup-sequence**  
The Makefile comment says "No startup prefix needed: fn_fuji.c uses SYS:" but
`fn_fuji.c` still builds paths as `ENVARC:fujinet/...`.  Without the Assign,
AmigaDOS will pop a "Please insert volume ENVARC in any drive" requester.

Fix: restore `EMU_STARTUP_PREFIX` in the Makefile:
```makefile
define EMU_STARTUP_PREFIX
Assign ENVARC: BATTLESHIP:
endef
export EMU_STARTUP_PREFIX
```

**Bug 2 — Error 121 when loading the binary**  
Error 121 = `ERROR_NO_FREE_STORE` or a hunk-loading failure in AmigaDOS.
Appeared after adding `static const uint8_t def[24]` arrays to `fn_fuji.c`.
Before that change the binary loaded fine.

Two hypotheses (test in order):

1. **Stale ADF in `~/Documents/FS-UAE/Floppies/`** — FS-UAE copies ADF there and
   may use the cached copy.  `make emu-clean && make emu-test` forces a rebuild,
   but the Floppies/ copy is not cleaned.  Try deleting it manually:
   ```bash
   rm -f ~/Documents/FS-UAE/Floppies/battleship.adf
   make -C apps/battleship/amiga emu-clean emu-test
   ```

2. **BSS/DATA section too large for KS 1.3 loader** — The `static const` arrays
   landed in the DATA segment and grew it past some threshold.
   Check sizes:
   ```bash
   m68k-amigaos-size apps/battleship/amiga/battleship
   # text + data + bss should be well under 512 KB total
   m68k-amigaos-objdump -h apps/battleship/amiga/battleship
   ```
   Fix candidate: remove `static` from the fallback arrays so they stay on the
   stack (no DATA/BSS contribution):
   ```c
   /* in fn_fuji.c fuji_read_appkey(), change: */
   static const uint8_t def[] = "amiga";
   /* to: */
   const uint8_t def[] = "amiga";
   /* and: */
   static const uint8_t def[24] = { 0x00, 0x01, 0x00, 0x00 };
   /* to: */
   const uint8_t def[24] = { 0x00, 0x01, 0x00, 0x00 };
   ```
   Rebuild lib + binary, re-check sizes, re-run emu-test.

---

## Step-by-Step Debug Sequence

### Step 1 — Rule out stale ADF

```bash
rm -f ~/Documents/FS-UAE/Floppies/battleship.adf
make -C apps/battleship/amiga emu-clean emu-test
```

Check screenshot:
```bash
ls -t emu/logs/battleship/*/screenshot.png | head -1
```

If the binary now loads → problem was stale ADF.  Skip to Step 3.  
If still error 121 → go to Step 2.

### Step 2 — Fix BSS/DATA growth

In `libs/fujinet-compat-amiga/src/fn_fuji.c`, remove `static` from the two
fallback `def` arrays (lines ~146 and ~154).

Rebuild and verify section sizes decrease:
```bash
make -C libs/fujinet-compat-amiga
make -C apps/battleship/amiga
m68k-amigaos-size apps/battleship/amiga/battleship
```

Re-run:
```bash
rm -f ~/Documents/FS-UAE/Floppies/battleship.adf
make -C apps/battleship/amiga emu-clean emu-test
```

### Step 3 — Fix the missing ENVARC: Assign

If the binary loads but you see "ENTER YOUR NAME" or a system requester about
ENVARC:, add the Assign back to the Makefile:

```makefile
define EMU_STARTUP_PREFIX
Assign ENVARC: BATTLESHIP:
endef
export EMU_STARTUP_PREFIX
```

This ensures `ENVARC:fujinet/0001/01/00` resolves to `BATTLESHIP:fujinet/...`
where our ADF_STATIC_DIR files live.

Re-run emu-test and check the screenshot for:
- No system requester
- Game advancing past player name / help screens
- fujibus: receive: appearing in fujinet.log (PASS condition)

### Step 4 — Verify the fallback paths work without ADF files

Even without the static appkey files on the ADF, the fallback code in
`fn_fuji.c` should return "amiga" for player name and seenHelp=1 for prefs when
`Open()` fails.  If the game still blocks on "ENTER YOUR NAME", the fallback
code isn't being hit — add a printf to confirm:
```c
if (s_creator_id == 1 && s_app_id == 1 && key_id == 0) {
    fprintf(stderr, "fuji_read_appkey: using fallback player name\n");
    ...
```
Check `serial-trace.log` or screenshot for that output.

---

## Architecture Note

The ADF_STATIC_DIR approach and the fallback-defaults approach are redundant
on purpose: belt-and-suspenders.  Once emu-test passes, clean one up:

- **Keep ADF_STATIC_DIR** (preferred) — correct AmigaOS approach, persists
  across game sessions, no hardcoded C defaults
- **Remove fn_fuji.c fallbacks** — they're a crutch for environments without
  ENVARC:, but the ADF approach solves the root cause

---

## Success Criteria

```
=== PASS (...) === 
```

And in `fujinet.log`:
```
fujibus: receive: ...
```

---

## Resolution (2026-06-29)

**Root cause: Stale FS-UAE save-disk file (`battleship.sdf`)**

FS-UAE saves floppy disk state to `~/Documents/FS-UAE/Save States/run/<name>.sdf`.
On subsequent runs, it restores the .sdf instead of reading the fresh ADF. When
the binary was updated (bug fixes, recompilation), the emulator kept booting the
old cached disk image — which contained either the original unstripped binary or
an earlier broken version.

**Fix applied to `emu/run.sh`:** Delete any stale `.sdf` file matching the ADF
name before launching FS-UAE.

### Lessons Learned

1. **FS-UAE .sdf save-state files override ADF content.** This is the #1
   thing to check when "same binary, different results" appears. The fix is
   now automated in run.sh.

2. **HUNK_SYMBOL data is NOT the cause of error 121.** Large symbol tables
   (8KB+, 350 symbols) load fine on KS 1.3. The symbol-stripping script
   (`emu/scripts/strip-hunk-symbols.py`) was added to the Makefile as a
   nice-to-have (smaller binary) but is not required for correctness.

3. **`m68k-amigaos-strip` corrupts Amiga hunk binaries.** Stripped binaries
   crash with Guru Meditation. Do NOT use it. The `-s` linker flag also
   produces bad output. Use the Python strip script instead if needed.

4. **Makefile default target ordering matters.** `include emu.mk` before
   `all:` makes emu.mk's first target the default. Fixed by moving `all:`
   before the include.

5. **`fn_fuji.c` ENVARC: paths work via ADF_STATIC_DIR.** Pre-populating
   `SYS:fujinet/` directories on the ADF lets appkey reads succeed without
   runtime Assign commands. The fallback defaults in fn_fuji.c are belt-and-
   suspenders redundancy.
