# Implementation Plan: Track 1D — FujiNet Lobby Amiga Port

**Depends on:** Track 1A (`libfn_compat_amiga.a`) ✅; `libs/amiga-gamekit` ✅
**Blocks:** nothing hard — but it is what turns two standalone game ADFs into a
front door a user can boot
**Status:** Planned, not started (2026-08-07). Port surface audited against
upstream on 2026-08-07; no code written.

---

## Goal

A FujiNet Lobby client on Amiga (KS 1.3+), booting from ADF, that lists live
game servers from `https://lobby.fujinet.online/` and hands off to the
already-ported Battleship and Fujitzee clients.

Upstream: [`FujiNetWIFI/fujinet-lobby`](https://github.com/FujiNetWIFI/fujinet-lobby)
(`clients/` subtree), to be pinned read-only at `apps/lobby/upstream` following
the `apps/<game>/upstream` + `apps/<game>/amiga` pattern from 1B/1C.

---

## Port surface audit (done 2026-08-07, from upstream sources)

**This is the smallest port yet.** The entire msdos platform layer is two
files — `conio.c` (3,971 bytes) and `platform.c` (1,531 bytes) — so the Amiga
platform layer is on the order of 5 KB of C.

`clients/src/platform.h` declares **four functions**:

```c
unsigned char readJoystick();
void initialize();
void waitvsync();
void reboot();
```

`clients/src/io.h` adds `readCommonInput()`, `inputField()`, and thin
`read_appkey()` / `write_appkey()` wrappers. Everything else is
`main.c` + `io.c`, platform-agnostic.

### FujiNet API usage — mostly covered, one genuine gap

| Used by the lobby | Status |
|---|---|
| `network_open` / `network_read` / `network_close` | ✅ `libs/fujinet-compat-amiga/src/fn_network.c` |
| `fuji_read_appkey` / `fuji_write_appkey` | ✅ served by nio-lib's legacy appkey API |
| `fuji_get_host_slots` / `fuji_mount_host_slot` / `fuji_mount_disk_image` | ❌ **not implemented, and blocked** — see below |

The mount trio is the canonical launch path and it depends on the Amiga disk
device, which is deferred until the parallel-port PHY
(`docs/plan-amiga-disk-device.md`). This is the plan's central constraint.

---

## The launch problem, and why it is smaller than it looks

Upstream's launch sequence, from `clients/src/main.c`:

1. split `client_path` into host + filename
2. `fuji_get_host_slots()`; `fuji_mount_host_slot(slot)`
3. `fuji_mount_disk_image(0, 1)`
4. `write_appkey(CREATOR_ID, APP_ID, game_type, url)`
5. `reboot()`

Steps 2–3 download and mount the platform's game client as a disk image, and
step 5 boots into it. **We cannot do 2, 3 or 5 as written** — no disk device,
and no autoboot.

But steps 4 and 5 are exactly the seam we need, and upstream already put
`reboot()` behind `platform.h`. So the Amiga variant is:

- **Steps 2–3 become a no-op.** Both games are already on the ADF; nothing
  needs downloading. Measured, a single OFS floppy holds `battleship`
  (60,880 B) + `fujitzee` (74,616 B) + `serial.device` + `C/` + a ~50 KB
  lobby ≈ **212 KB of ~880 KB**, so co-residency is comfortable.
- **Step 4 works today, unchanged.** This is the whole point: it is the
  mechanism both ported games already use to learn their server URL.
- **Step 5, `reboot()`, becomes "exit to the startup-sequence loop"** — the
  lobby writes which game was chosen, exits, and the startup-sequence runs
  it. One binary resident at a time, which also keeps a 512 KB A500 honest.

No upstream change is needed for the launch itself; only a decision about what
the mount calls compile to on Amiga (Q3).

### A detail worth recording: the appkey key id *is* `game_type`

Step 4 writes to key id `lobby.servers[selected].game_type`, not a constant.
That resolves a discrepancy already visible in our own docs — Battleship reads
lobby key `0x05` (`contracts/fujinet-appkey-protocol.md`) while Fujitzee reads
key `0x03` (`AK_LOBBY_KEY_SERVER`, `apps/fujitzee/upstream/src/misc.h`). They
are not inconsistent: each game registered its own `game_type` with the lobby
server, and the lobby writes the URL to whichever key that game answers to.
This belongs in `contracts/fujinet-appkey-protocol.md` (Phase 0).

---

## Two risks that have bitten this project before

**1. The wire format is a packed binary struct — again.** The client requests
`?bin=1` and memcpy's the response straight into:

```c
typedef struct {
  uint8_t server_count;
  ServerDetails servers[23];
} LobbyResponse;
```

This is the Fujitzee wire-format lesson verbatim (`FUJITZEE_PACK_STRUCTS`,
fujinet-fujitzee#9). **Pin the offsets with a T1 host test before writing any
renderer** — `apps/fujitzee/amiga/test/host/test_wireformat.c` is the model.
Expect the same `-fpack-struct`-style opt-in to be needed, and expect it to
need an upstream PR if upstream has no hook.

**2. Column count.** The gamekit screen is 320×200 → **40 columns**. Upstream
has both an 80-column msdos layout and a 40-column Atari one. Fujitzee went
the *other* way (the DOS layout fit, the Atari's was a row too tall), so do
not assume; check both against 40×25 in Phase 0. Server names and player
counts are the fields at risk.

---

## Phases

**Phase 0 — Pin, audit, decide.** Add `apps/lobby/upstream` as a submodule
(read-only pin, remotes per CLAUDE.md). Write the T1 wire-format test against
`ServerDetails` / `LobbyResponse` and get it passing on the host. Choose the
layout (40-col Atari vs 80-col msdos). Settle the platform id (Q1). Record the
`game_type`-as-key-id fact in `contracts/fujinet-appkey-protocol.md`.

**Phase 1 — Scaffold and link on stubs.** `apps/lobby/amiga/` with a Makefile
including `make/amiga.mk`, `amiga_vars.h` if upstream lacks a `PLATFORM_VARS`
hook (Fujitzee needed `-include`; check first), and the four `platform.h`
functions stubbed. Goal is only that it compiles and links under m68k.

**Phase 2 — Real text layer and input.** The lobby is a text UI, so this is
where the reusable asset appears: a **conio-style text API in the gamekit**
(`cputsxy`/`gotoxy`/`cclear` shape, matching what upstream's per-platform
`conio.c` files assume). Then `readCommonInput()`, `inputField()` for the
username, and a live server list rendered from a real HTTP response. This is
the first non-game consumer of the gamekit and the honest test of whether its
seams are a platform layer or battleship's engine in disguise.

**Phase 3 — The handoff.** `reboot()` → exit-and-relaunch; the mount calls
resolved per Q3; a single ADF carrying lobby + both games with a
startup-sequence dispatch loop. End to end: boot → pick a server → land in
Battleship or Fujitzee → quit → back at the lobby.

**Phase 4 — Boot test and T2 coverage.** `emu/drive.sh` script driving the
list and a launch; `make emu-test` pass pattern. Real hardware alongside the
outstanding Battleship hardware check.

---

## What we're doing differently

Carrying forward the 1C process changes that paid off:

- **Port surface audited up front** (this document, above) rather than
  discovered per phase.
- **Wire format pinned by a T1 test before the renderer**, because that is the
  exact defect class 1C hit and it is invisible until a live server disagrees
  with you.
- **Extract rather than copy**, four for four so far. The conio-ish text layer
  is the candidate here, and unlike the previous four it is being extracted
  for a consumer that is not a game.
- **Keep the launch mechanism behind one function.** When the disk device
  eventually lands, `launch_game()` swaps from exit-and-relaunch to
  mount-and-boot without touching the UI, the HTTP path, or the renderer.

---

## Open questions

**Q1 — Platform id.** The query is
`lobby.fujinet.online/view?bin=1&platform=<PLATFORM>&…`. Is there an `amiga`
platform value registered server-side, and are any servers advertising Amiga
clients? Without it the list returns clients we cannot run. Note this is a
*server-side registration* dependency, not just code. Interim fallback:
browse read-only with another platform's id during development.
*Closes with: Jeff + the Lobby maintainers (Thom / Norman).*

**Q2 — Does the lobby need to filter?** If the server list is
platform-filtered, an Amiga lobby shows only Amiga-capable servers — which
today means the two we ported. If it is not, the client should probably hide
or grey entries it cannot launch rather than offering a dead end.
*Closes with: Q1's answer.*

**Q3 — What do the mount calls compile to?** Three options: (a) stub them in
the Amiga platform layer and let upstream's flow run harmlessly, (b) `#ifdef`
them out, which means an upstream PR, or (c) implement them as a lookup from
`client_path` to a local ADF binary name. (c) is the most honest and is what
makes the dispatch loop work. *Closes with: Jeff, Phase 1.*

**Q4 — Does upstream want the port?** 1B and 1C both went upstream
(`docs/upstreaming-amiga-ports.md`). The lobby is the canonical client, so an
Amiga platform dir in `FujiNetWIFI/fujinet-lobby` is plausibly more welcome
than a game port — but it will carry a non-canonical launch path, which is a
reviewer conversation. *Closes with: Jeff + upstream, after Phase 3.*

**Q5 — Username sharing.** Lobby key `00010100` is the shared username both
games already read. Confirm the Amiga lobby writing it actually pre-fills
Fujitzee's name-entry screen — that is a visible, cheap win and a good Phase 3
acceptance check. *Closes with: Phase 3 testing.*

---

## References

- `docs/plan-track1c-fujitzee.md` — the pattern this follows; especially the
  wire-format and force-included-vars wrinkles
- `docs/plan-amiga-disk-device.md` — why the canonical mount-and-boot launch
  is unavailable (Q2 there closed as disqualifying, 2026-08-07)
- `contracts/fujinet-appkey-protocol.md` — appkey semantics and the known
  lobby keys
- `contracts/amiga-adf-bootstrap.md` — ADF recipe and startup-sequence rules
- `contracts/amiga-coding-conventions.md` — KS 1.3 floor, memory/stack rules
- `docs/testing.md` — where the T1 wire-format test and T2 boot test belong
