# FujiNet AppKey → App-Store Mapping

> **Supersedes (2026-07-06):** this contract previously specified a classic
> four-command AppKey wire protocol (`OPEN/READ/WRITE/CLOSE_APPKEY`,
> `0xDC`–`0xDB`) to be added to `fujinet-nio`. That protocol was never
> implemented. Upstream instead shipped a generalized **app-store** on the
> FileDevice (fujinet-nio `83627c0`, fujinet-nio-lib `445891c`), whose docs
> state it "deliberately avoids the legacy AppKey model of fixed
> creator/app/key triples and 64-byte blobs." We adopt the app-store; this
> contract now specifies how the classic AppKey API maps onto it. The old
> wire-protocol text lives in git history.

## Overview

AppKeys are small (≤64 byte) blobs that games persist on the FujiNet device,
addressed by a **creator / app / key** triple. Games written against the
standard `fujinet-fuji.h` API call:

```c
void fuji_set_appkey_details(uint16_t creator_id, uint8_t app_id, enum AppKeySize keysize);
bool fuji_read_appkey(uint8_t key_id, uint16_t *count, uint8_t *data);
bool fuji_write_appkey(uint8_t key_id, uint16_t count, uint8_t *data);
```

On Amiga, the compat layer (`libs/fujinet-compat-amiga/src/fn_fuji.c`)
implements these on top of the nio-lib **app-store API** — string-namespaced
key/value storage served by `fujinet-nio`'s FileDevice:

```
game code (fujinet-fuji.h appkey API, numeric triple, ≤64 bytes)
        │
libs/fujinet-compat-amiga/src/fn_fuji.c      ← this contract
        │
fn_appstore_stat/read/write()                (fujinet-nio-lib)
        │  FileDevice AppStore commands 0x20–0x24 over FujiBus
fujinet-nio  →  backing filesystem, /FujiNet/app-store/v1 (private root)
```

The server storage layout is an upstream implementation detail — the compat
layer must only use `fn_appstore_*` calls, never construct paths.

**Scope:** `keysize = DEFAULT` (64 bytes) only, matching what lobby games use.
The app-store supports arbitrary sizes; the classic API does not, so the compat
layer enforces the 64-byte ceiling.

## Naming convention (the heart of this contract)

The numeric triple maps to app-store strings as:

| App-store field | Format | Example (Battleship prefs) |
|---|---|---|
| `namespace` | `appkey-<creator%04x>-<app%02x>` | `appkey-e41c-05` |
| `key`       | `<key%02x>`                      | `00` |

- Lowercase hex, fixed width, matching the classic firmware's
  `%04x%02x%02x.key` filename style.
- One namespace per (creator, app) pair keeps `fn_appstore_list()` usable as
  "list this app's keys".
- **Open question (confirm with Mark before other platforms adopt it):**
  whether upstream wants a blessed convention for legacy-appkey consumers, so
  that e.g. lobby keys (creator `0x0001`) written by other future nio clients
  land in the same namespace.

Known keys used by Battleship / the lobby:

| Creator  | App    | Key    | Namespace / key | Contents |
|----------|--------|--------|-----------------|----------|
| `0x0001` | `0x01` | `0x00` | `appkey-0001-01` / `00` | Lobby player name (string, max 8 bytes) |
| `0x0001` | `0x01` | `0x05` | `appkey-0001-01` / `05` | Lobby server URL (string, ≤64 bytes) |
| `0xE41C` | `0x05` | `0x00` | `appkey-e41c-05` / `00` | Battleship prefs (byte 0 = debugFlag, byte 1 = seenHelp) |

## Semantics mapping

| Classic call | App-store realization |
|---|---|
| `fuji_set_appkey_details(c, a, size)` | Store `(c, a)` in static context; build the namespace string. `size != DEFAULT` → all subsequent ops fail (`false`). |
| `fuji_read_appkey(k, &count, data)` | One `fn_appstore_read(ns, key, 0, data, 64, &out)`. Missing key (`!EXISTS` flag) → **success** with `*count = 0` (classic semantics: absent key is not an error). Present → `*count = out.bytes_read`. A value >64 bytes (foreign writer) is truncated at 64; ignore the missing-EOF flag. |
| `fuji_write_appkey(k, count, data)` | Reject `count > 64`. One `fn_appstore_write(ns, key, 0, data, count, &out)` — offset 0 creates/replaces. |

Single-chunk transfers suffice: 64 bytes always fits in one FujiBus frame, so
the chunked-offset machinery is unused here.

**Missing-key defaults:** the compat layer keeps its current behavior of
returning built-in defaults for known keys when the read finds nothing (e.g.
Battleship prefs default to `seenHelp = 1`), so games boot sanely on a fresh
server. Defaults live in `fn_fuji.c`, not on the server.

## Client-side obligations (fujinet-compat-amiga)

1. Reimplement `fn_fuji.c` on `fn_appstore_*`. This **removes** the
   `SYS:fujinet/…` / `ENVARC:` local-file storage and with it the
   `proto/dos.h` dependency — the file becomes pure logic over `fn_*` calls
   and therefore **T1-eligible** (host unit tests per `docs/testing.md`,
   using the inert-stub technique from `test_network.c`).
2. Error mapping: any `fn_appstore_*` result other than `FN_OK` → the classic
   call returns `false` and sets the layer's last-error state.
3. Battleship's ADF `envarc/` seeding (`ADF_STATIC_DIR` in
   `apps/battleship/amiga/Makefile`) becomes obsolete once this lands —
   remove the seeding rules and instead seed the **server** in emu tests
   (see below).

## Server-side obligations (fujinet-nio)

None. The app-store already ships in upstream `master` and is registered in
the posix build (`main_posix.cpp`). The Amiga side consumes it as-is.

## Testing

- **T1:** unit-test the triple→string mapping and the read/write semantics in
  `libs/fujinet-compat-amiga/test/host/` with stubbed `fn_appstore_*`.
- **T2:** extend `test/compat_test.c` with an appkey write→read round-trip
  over the emulator against a live `fujinet-nio`.
- **Seeding for emu tests:** write keys server-side before boot with
  upstream's tool, e.g.
  `python -m fujinet_tools.file appstore-write appkey-0001-01 00 --data AMIGA`
  (see `fujinet-nio/py/fujinet_tools/file.py` for exact CLI), rather than
  touching `fujinet-data/` paths directly.

## References

- `fujinet-nio/docs/file_device_protocol.md` — AppStore commands `0x20`–`0x24`, wire format
- `fujinet-nio-lib/docs/api.md` — `fn_appstore_*` client API
- `libs/fujinet-compat-amiga/src/fn_fuji.c` — current (local-file) implementation to be replaced
- `contracts/fujibus-protocol.md` — FujiBus framing and transport layer
- `fujinet-firmware/lib/device/sio/fuji.cpp` — classic firmware AppKey implementation (historical reference)
