# Implementation Plan: Track 1A — fujinet-network / fujinet-fuji Compat Layer

> **Status: ARCHIVED — superseded 2026-07-05.** Work completed 2026-07-01;
> the durable procedure lives in `docs/updating-fujinet-compat-headers.md` and
> the symbol inventory in `docs/audit-track1a-gap-table.md`.

**Depends on:** nothing (builds on top of existing `fujinet-nio-lib`)  
**Blocks:** Track 1B (Battleship port)  
**Status:** Complete (2026-07-01) — `libs/fujinet-compat-amiga` builds and battleship links it. Ongoing header-sync procedure lives in `docs/updating-fujinet-compat-headers.md`.

---

## Goal

Produce `libfn_compat_amiga.a` — a static library implementing the standard
FujiNet multi-platform headers (`fujinet-network.h`, `fujinet-fuji.h`) on top
of `fujinet-nio-lib`. Any FujiNet game that targets those headers can be built
for Amiga by linking this library instead of the platform's native fujinet-lib.

---

## Step 0 — Audit (do this before writing any code)

Fetch the upstream headers from
`https://github.com/FujiNetWIFI/fujinet-lib` and compare every function in
`fujinet-network.h` and `fujinet-fuji.h` against `fujinet-nio-lib`'s capabilities.

Produce a gap table: for each function, note one of:
- **Direct map** — maps cleanly to an existing `fn_*` call
- **Needs work** — requires logic or a new nio protocol command
- **Stub / skip** — not needed for Battleship; safe to return `false`/`0` for now

The Battleship analysis gives a known-required subset (see below), but the full
audit catches anything a future port might need.

Known required subset from Battleship:

| Function | Status |
|---|---|
| `network_open(devicespec, mode, trans)` | Direct map |
| `network_read(devicespec, buf, len)` | Direct map |
| `network_close(devicespec)` | Direct map |
| `fuji_set_appkey_details(creator, app, size)` | Stub (context setter) |
| `fuji_read_appkey(key_id, &count, data)` | Needs work (ENVARC: file I/O) |
| `fuji_write_appkey(key_id, count, data)` | Needs work (ENVARC: file I/O) |

---

## Location

New directory: `libs/fujinet-compat-amiga/`

```
libs/fujinet-compat-amiga/
  include/
    fujinet-network.h      ← copy of upstream header (unmodified)
    fujinet-fuji.h         ← copy of upstream header (unmodified)
  src/
    fn_network.c           ← network_open/read/close implementation
    fn_fuji.c              ← fuji_set_appkey_details/read/write implementation
  Makefile
```

Headers are copied verbatim from upstream so games can include them without
modification. The implementation is entirely in `src/`.

---

## Implementation

### `fn_network.c`

**`network_open(devicespec, mode, trans)`**

1. Strip the `N:` prefix from `devicespec` to get a plain URL.
2. Detect scheme:
   - `https://` → `flags |= FN_OPEN_TLS`
3. Map `mode` to `fn_open()` method:
   - `OPEN_MODE_HTTP_GET` → `FN_METHOD_GET`
   - `OPEN_MODE_HTTP_POST` → `FN_METHOD_POST`
   - `OPEN_MODE_HTTP_PUT` → `FN_METHOD_PUT`
   - `OPEN_MODE_HTTP_DELETE` → `FN_METHOD_DELETE`
   - TCP modes → `fn_tcp_open()`
4. Store the returned `fn_handle_t` in a static slot keyed by `devicespec`
   (or a simple hash of it) so subsequent `network_read`/`network_close`
   calls can look up the handle.
5. Return `true` on success, `false` on error.

**Session slot table** — same per-opener discipline as Track 2:
```c
typedef struct {
    fn_handle_t handle;
    char        devicespec[256];  /* key for lookup */
    uint32_t    rx_cursor;
    uint8_t     in_use;
} NetSlot;
static NetSlot slots[FN_MAX_SESSIONS];  /* 4 slots */
```

**`network_read(devicespec, buf, len)`**

1. Look up handle by `devicespec`.
2. Poll `fn_read()` until data arrives or error:
   - `FN_ERR_NOT_READY` / `FN_ERR_BUSY` → retry
   - `FN_OK` with `bytes_read == 0` and EOF flag → return 0
   - error → return negative errno-style value
3. Advance `rx_cursor` by `bytes_read`.
4. Return `bytes_read` (positive), 0 (EOF), or negative (error).

**`network_close(devicespec)`**

1. Look up handle, call `fn_close()`, clear the slot.
2. Return `true`.

### `fn_fuji.c`

AppKey storage is mapped to flat binary files under `ENVARC:fujinet/`.

**Path construction:**
```
ENVARC:fujinet/<creator_id_hex>/<app_id_hex>/<key_id_hex>
```
Example: creator=`0x0001`, app=`0x01`, key=`0` →
`ENVARC:fujinet/0001/01/00`

`fuji_set_appkey_details(creator, app, size)` stores context in static vars.

`fuji_write_appkey(key_id, count, data)` opens (or creates) the file with
`Open()` (DOS) and writes `count` bytes.

`fuji_read_appkey(key_id, &count, data)` opens the file, reads into `data`,
sets `*count` to bytes read. Returns `false` if the file doesn't exist (first
run — caller must handle gracefully).

**KS 1.3 note:** `ENVARC:` requires at least one reboot after first write to
persist (it's on the RAM disk backed by the user's `startup-sequence`). This
is fine for preferences. Use `dos.library` `Open()`/`Read()`/`Write()`/`Close()`
— available on KS 1.3.

---

## Makefile

Mirror the pattern from `apps/http_get/Makefile`:

```makefile
CC      = m68k-amigaos-gcc
CFLAGS  = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13 -DNO_INLINE_MULDIV
INCLUDES = -I../../fujinet-nio-lib/include -Iinclude
AR      = m68k-amigaos-ar

SRCS = src/fn_network.c src/fn_fuji.c
OBJS = $(SRCS:.c=.o)

all: libfn_compat_amiga.a

libfn_compat_amiga.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) libfn_compat_amiga.a
```

---

## Smoke Test

Write `libs/fujinet-compat-amiga/test/compat_test.c`:

- Call `network_open("N:HTTP://httpbin.org/get", OPEN_MODE_HTTP_GET, 0)`
- Call `network_read(...)` in a loop, print response to stdout
- Call `network_close(...)`
- Also test `fuji_set_appkey_details` + `fuji_write_appkey` + `fuji_read_appkey`
  round-trip (write "testuser", read it back, assert equal)

Build as an Amiga executable, boot with `/emu-build-and-boot`.

---

## Verification Checklist

- [ ] Audit table produced and reviewed — no surprising gaps
- [ ] `libfn_compat_amiga.a` builds for `TARGET=amiga` without warnings
- [ ] `network_open` / `read` / `close` fetch a real HTTP URL in the emulator
- [ ] AppKey round-trip: write a value, reboot emulator, read it back unchanged
- [ ] Battleship can be linked against this library and reach `main()` without
      linker errors (full game test is Track 1B)
