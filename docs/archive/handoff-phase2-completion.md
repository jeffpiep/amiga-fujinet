# Handoff: Complete Battleship Phase 2

> **Status: ARCHIVED — completed 2026-07-01.** Phase 2 is done (console game
> loop: join a table, see the lobby, place ships, play a full game). Remaining
> work is tracked in `docs/plan-track1b-battleship.md` (Phases 3–4). Kept as a
> historical record.

Branch: `worktree-track1b`
Goal: Phase 2 = "Console game loop — join a table, see the lobby, place ships,
play a full game — text-only, keyboard-controlled."

---

## What's Done

- Binary builds, boots on KS 1.3, connects to the server via FujiNet serial
- `make emu-test` PASSES — `fujibus: receive:` appears in fujinet.log in ~15s
- ANSI console graphics (`graphics.c`) fully implemented — lobby, game boards,
  ship placement, cursor, legend, end-game messages all render
- AppKey files pre-populated on ADF via `ADF_STATIC_DIR` — player name ("AMIGA")
  and prefs (seenHelp=1) load without runtime Assign
- `fn_fuji.c` paths changed from `ENVARC:` to `SYS:` with fallback defaults
- Stale `.sdf` cache bug fixed in `emu/run.sh`
- `cgetc()` fixed: replaced V36-only `FGetC()` with V33-safe `Read()` (commit e9e098d)
- `platform_init.c` deleted (was never compiled; dead code) (commit e9e098d)
- Game reaches table selection screen interactively — FujiNet lobby is live,
  server tables (`cape fuji`, `ai – 1 on 1`, etc.) appear on screen

---

## Current Blocking Bug: CON: Raw Mode

### What We Know

The game boots and renders the table selection screen correctly. However,
keyboard navigation does not work the way you'd expect:

- **WASD keys echo to the screen** — the CON: device is in cooked mode, so each
  character typed is echoed immediately by the console handler
- **The game only sees the input after Enter is pressed** — `Read(Input(), &ch, 1)`
  in cooked mode blocks until the user presses Enter, at which point it returns
  the first character of the buffered line
- **Arrow keys do nothing** — they send ANSI escape sequences (multi-byte), but
  `cgetc()` reads one byte at a time; the game never sees a full escape sequence
  that maps to a direction key (the amiga_vars.h mapping is WASD, not arrow keys)

### Root Cause

AmigaOS CON: device starts in **cooked mode** (line-buffered, with echo). Raw mode
(immediate, no-echo, per-keypress) requires `SetMode(Input(), DOSTRUE)`. But
`SetMode()` is **dos.library V36 (KS 2.0+)**. Calling it on KS 1.3 jumps to a
garbage LVO and causes a Guru Meditation ("Software error — task held").

This was confirmed by trying it: adding `SetMode()` to `initGraphics()` crashed
the game on boot. Three approaches were attempted and all fail on KS 1.3:

1. `SetMode()` via `<proto/dos.h>` — implicit declaration, wrong calling convention, crash
2. `SetMode()` via `<inline/dos.h>` + `extern struct DosLibrary *DOSBase` — correct
   LVO expansion confirmed by preprocessor, but still crashes (V36 LVO slot invalid on KS 1.3)
3. `SetMode()` with version check: not yet tried (see below)

Current state: `SetMode()` code has been removed. Game boots correctly but input
requires Enter after each keypress (usable but not a real game).

### What to Try Next

**Option A — Version-gated SetMode (try this first)**

Check the dos.library version at runtime. On KS 2.0+ (V36+), use `SetMode()` for
proper raw mode. On KS 1.3 (V34), fall back to the `RAW:` approach below.

```c
#include <dos/dosextens.h>
#include <inline/dos.h>
extern struct DosLibrary *DOSBase;

void initGraphics(void)
{
    setbuf(stdout, NULL);
    if (((struct Library *)DOSBase)->lib_Version >= 36) {
        SetMode(Input(), DOSTRUE);
        atexit(restoreCookedMode);  // SetMode(Input(), DOSFALSE) on exit
    }
    fn_init();
    ...
}
```

This would let us test on KS 2.0+ today. The KS 1.3 fallback is still needed
for the Amiga 500 target but this unblocks interactive testing in FS-UAE.

**Option B — Open `RAW:` and redirect I/O (KS 1.3 compatible)**

`Open("RAW:0/0/640/256/Battleship/CLOSE", MODE_NEWFILE)` opens a raw console
window with immediate, no-echo key input. `SelectInput()` and `SelectOutput()`
are dos.library V33 (KS 1.2+) and redirect the process's I/O.

```c
static BPTR s_rawCon = 0;
static BPTR s_oldIn  = 0;
static BPTR s_oldOut = 0;

static void closeRawConsole(void)
{
    if (s_rawCon) {
        SelectInput(s_oldIn);
        SelectOutput(s_oldOut);
        Close(s_rawCon);
        s_rawCon = 0;
    }
}

void initGraphics(void)
{
    s_rawCon = Open("RAW:0/0/640/256/Battleship/CLOSE", MODE_NEWFILE);
    if (s_rawCon) {
        s_oldIn  = SelectInput(s_rawCon);
        s_oldOut = SelectOutput(s_rawCon);
        atexit(closeRawConsole);
    }
    setbuf(stdout, NULL);
    fn_init();
    ...
}
```

**Key unknown:** Does `printf()` follow `SelectOutput()`? With bebbo's nix13 CRT,
`stdout` is a FILE* bound to the DOS output handle at startup. If the CRT fetches
`Output()` dynamically per write, `SelectOutput()` will make `printf()` go to the
new window. If it's stored statically at startup, `printf()` will still go to the
old CLI window and the game will be invisible in the RAW: window.

Test by running and seeing if the game renders in the new window. If not, all
printf()-based output in `graphics.c` would need to be converted to `FPuts()`/
`FWrite()` with the raw handle — invasive but doable.

**Option C — Accept cooked mode with documentation**

For the Phase 2 milestone, accept that KS 1.3 requires Enter after each key. The
game IS functional this way (WASD then Enter moves cursor). Document it as a Phase 3
item to fix with the `RAW:` approach. This unblocks the gameplay verification tests
immediately.

### Recommended Path

1. Try **Option A** first — easiest, unblocks FS-UAE interactive testing now
2. If FS-UAE is running KS 1.3 (check `fsuae-test.fs-uae` — it is), go straight
   to **Option B** and test the `RAW:` redirect
3. If both fail, fall back to **Option C** and complete gameplay verification
   with Enter-per-key, then fix raw mode in Phase 3

---

## Interactive Testing Setup

When you're ready to test interactively, the infrastructure is already running.
Start fresh with:

```bash
# Kill any stale processes
killall fs-uae socat fujinet-nio 2>/dev/null
rm -f /tmp/amiga-serial /tmp/fn-pty

# Terminal 1: socat PTY pair
cd /path/to/amiga-fujinet
socat -d -d pty,raw,echo=0,link=/tmp/amiga-serial \
           pty,raw,echo=0,link=/tmp/fn-pty \
  2>emu/logs/battleship/interactive/serial-trace.log &
sleep 2

# Terminal 2: fujinet-nio
FN_SERIAL_PORT=/tmp/fn-pty FN_SERIAL_BAUD=19200 \
  stdbuf -oL -eL fujinet-nio/build/fujibus-rs232-debug/fujinet-nio \
  >emu/logs/battleship/interactive/fujinet.log 2>&1 &

# Rebuild ADF (always delete stale .sdf first!)
rm -f ~/Documents/FS-UAE/Save\ States/fsuae-test/*.sdf
rm -f ~/Documents/FS-UAE/Save\ States/run/*.sdf
APP_NAME=battleship APP_BINARY=apps/battleship/amiga/battleship \
  ADF_STATIC_DIR=apps/battleship/amiga/envarc \
  ADF_OUT=apps/battleship/amiga/battleship.adf \
  emu/scripts/build-adf.sh

# Launch FS-UAE on real display
DISPLAY=:0 fs-uae emu/config/fsuae-test.fs-uae \
  --floppy_drive_0=apps/battleship/amiga/battleship.adf \
  --serial_port=$(readlink /tmp/amiga-serial) \
  >emu/logs/battleship/interactive/emulator.log 2>&1 &
```

Take screenshots with:
```bash
WIN_ID=$(DISPLAY=:0 xwininfo -root -children 2>/dev/null | grep -i '"FS-UAE' | awk '{print $1}')
DISPLAY=:0 xwd -id "$WIN_ID" -silent > /tmp/fsuae.xwd
python3 - <<'EOF'
import struct
from PIL import Image
with open('/tmp/fsuae.xwd', 'rb') as f:
    data = f.read()
fields = struct.unpack('>20I', data[:80])
header_size, _, _, depth, w, h, _, byte_order, _, _, _, bpp, bpl = fields[:13]
ncolors = struct.unpack('>I', data[76:80])[0]
pixel_offset = header_size + ncolors * 12
pixels = data[pixel_offset:pixel_offset + bpl * h]
img = Image.frombytes('RGBA', (w, h), pixels, 'raw', 'BGRA', bpl, 1).convert('RGB')
img.save('emu/logs/battleship/interactive/screenshot.png')
print(f"screenshot: {img.size}")
EOF
```

---

## Key Files

| File | Role |
|------|------|
| `apps/battleship/amiga/src/input.c` | `kbhit()`, `cgetc()`, `readJoystick()` |
| `apps/battleship/amiga/src/graphics.c` | ANSI console rendering + `fn_init()` call — **raw mode fix goes here** |
| `apps/battleship/amiga/src/util.c` | `__stack`, timer stubs, `itoa()`, random |
| `apps/battleship/amiga/src/sound.c` | All no-ops (Phase 3) |
| `apps/battleship/amiga/include/amiga_vars.h` | Key mappings — WASD for nav, Return to select |
| `apps/battleship/amiga/Makefile` | Build + emu-test targets |
| `emu/config/fsuae-test.fs-uae` | FS-UAE config — KS 1.3, A500 model |
| `contracts/amiga-coding-conventions.md` | KS 1.3 API restrictions — **SetMode is V36, not V33** |

---

## KS 1.3 API Traps Found This Session

These dos.library functions are **V36 (KS 2.0+)**, not V33 as commonly assumed:

| Function | LVO | Reality | Safe alternative |
|----------|-----|---------|-----------------|
| `FGetC()` | 0x14A | V36 only | `Read(fh, &ch, 1)` |
| `SetMode()` | 0x1AA | V36 only | Open `RAW:` + `SelectInput()` |

Both were confirmed by Guru Meditation crashes on KS 1.3 in FS-UAE. Update
`contracts/amiga-coding-conventions.md` when the full list is known.

---

## Verification

Phase 2 is complete when:

- [x] `cgetc()` uses `Read()` instead of `FGetC()` (commit e9e098d)
- [x] `make emu-test` still PASSES (12s, fujibus: receive:)
- [x] `platform_init.c` deleted (was never compiled; dead code)
- [x] **Raw mode working** — keyboard navigates without requiring Enter
- [x] Interactive test: can navigate table selection with keyboard
- [x] Interactive test: can place ships and play at least one turn
- [x] **Full game to completion** — played end-to-end; playable and fun

## Phase 2 — COMPLETE

Console game loop done: lobby, ship placement, and full keyboard-controlled
games (2-player vs AI and 4-player) all verified on KS 1.3 / NTSC. Rendering
bugs found during playtesting were fixed (commits 170e047, 6fd87af, fa7b421):
empty-water dot grid, ship-erase, cursor trail, console scroll/drift, and the
3-4 player 4-column layout.

Known Phase 3 follow-ups (non-blocking): the per-turn forced redraw (the
console-scroll workaround) flickers more in multiplayer; the ASCII renderer is
slated to be replaced by a graphical Workbench app in Phase 3 (after sound and
joystick), which supersedes these text-console workarounds.

---

## RESOLVED — Raw mode via RAW: console (Option B)

The CON: cooked-mode blocker is fixed using **Option B** from the plan above.

`graphics.c` `initGraphics()` opens a `RAW:0/0/640/200/Battleship` window
(NTSC, V33-safe). All game output is written directly to that handle
(`conPrintf`, which `vsnprintf`s then `Write()`s) and `input.c` reads keys from
the same handle (`Read(handle, &ch, 1)` / `WaitForChar`). This sidesteps the
"does `printf` follow `SelectOutput`" unknown entirely — we never touch stdout.

`SelectInput`/`SelectOutput` were tried first but are **not** in the ndk13
headers (they link to a compat stub, not V33 dos.library) — same V36 trap class
as `SetMode`. They were removed; we read/write the handle directly instead.

### Verified interactively in FS-UAE (KS 1.3, A500, NTSC)

Drove the emulator with synthetic keystrokes (XTEST). Full happy path:

1. Lobby renders in the RAW: window; game list populates from the server.
2. `s` (down) walks the table cursor — **no `s` echoed, no Enter needed**.
3. `Return` joins "ai – 1 on 1"; AI opponent "clyd bot" auto-readies.
4. `space` readies up → "starting in 1" → ship placement screen.
5. WASD moves the placement cursor, `space` drops ships — all 5 placed.
6. Game enters play; fired shots register on the opponent board (`@`/`O`),
   bot fires back (`X` on our board). fujinet.log shows live send/receive
   for every move.

### Known follow-ups for Phase 3 (not blockers)

- **Screen-clear bleed-through**: the lobby → waiting-room sub-screens overlay
  without a full clear, so stale text shows during those transitions. The
  in-game board and ship-placement screens clear cleanly. Likely upstream
  redraw behavior on an 80x25 scrolling console; investigate `resetScreen()`
  vs the waiting-room draw path.
- **`s` key collision**: `amiga_vars.h` maps `s` to DOWN, but upstream
  `screens.c` also treats `s` as the sound-mute shortcut in the lobby. Both
  fire on one press. Harmless today (sound is a no-op) but should be
  de-conflicted (e.g. drop `s` as the sound key on Amiga).
- Synthetic key injection (XTEST → FS-UAE) is mildly lossy on rapid bursts;
  space presses ~0.45s apart when scripting tests.
