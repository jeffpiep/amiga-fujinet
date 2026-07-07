# FujiNet AppKeys on Amiga — Legacy API via fujinet-nio-lib

> **Supersedes (2026-07-07):** this contract previously specified a mapping
> from the classic AppKey triple onto app-store namespaces
> (`appkey-<creator>-<app>` / `<key>`), implemented in our compat layer.
> The day after that shipped, upstream added the classic AppKey API
> **natively to fujinet-nio-lib** (`8eddb80`, `include/fn_legacy_appkey.h`
> + `src/legacy/`), storing keys in the classic firmware's own file layout
> via the new `persist://` filesystem alias (fujinet-nio `60ca4eb`). That
> answers the open question this contract carried ("confirm namespace
> convention with Mark") — the answer is *don't invent one*. The compat
> layer's implementation was removed; the app-store mapping text lives in
> git history. (Before that, this file specified a `0xDC`–`0xDB` wire
> protocol that was never implemented — also in git history.)

## Overview

AppKeys are small (≤64 byte) blobs that games persist on the FujiNet device,
addressed by a **creator / app / key** triple. Games written against the
standard `fujinet-fuji.h` API call:

```c
void fuji_set_appkey_details(uint16_t creator_id, uint8_t app_id, enum AppKeySize keysize);
bool fuji_read_appkey(uint8_t key_id, uint16_t *count, uint8_t *data);
bool fuji_write_appkey(uint8_t key_id, uint16_t count, uint8_t *data);
```

These three functions are now provided by **fujinet-nio-lib itself**
(`include/fn_legacy_appkey.h`, implemented in `src/legacy/`), built into
`fujinet-nio-amiga.a` as separate archive members — apps that never call
them don't link them.

```
game code (fujinet-fuji.h appkey API, numeric triple, ≤64 bytes)
        │  (declarations from the compat header; no compat-layer code)
fujinet-nio-lib src/legacy/fn_legacy_appkey_*.c
        │  FileDevice FileRead/FileWrite (0x03/0x04) over FujiBus
fujinet-nio  →  persist:///FujiNet/<creator%04x><app%02x><key%02x>.key
```

The `persist://` alias resolves to the platform's persistent store (host
filesystem on POSIX, SD/flash on ESP32) without the client knowing which.
The file layout is the **classic firmware's own** — an SD card written by
old firmware is readable by fujinet-nio apps with no migration.

Known keys used by Battleship / the lobby:

| Creator  | App    | Key    | File (under `persist:///FujiNet/`) | Contents |
|----------|--------|--------|------------------------------------|----------|
| `0x0001` | `0x01` | `0x00` | `00010100.key` | Lobby player name (string, max 8 bytes) |
| `0x0001` | `0x01` | `0x05` | `00010105.key` | Lobby server URL (string, ≤64 bytes) |
| `0xE41C` | `0x05` | `0x00` | `e41c0500.key` | Battleship prefs (byte 0 = debugFlag, byte 1 = seenHelp) |

## Semantics (upstream-defined, recorded here for Amiga consumers)

- `fuji_set_appkey_details(c, a, size)` sets static context; `DEFAULT` = 64
  bytes, `SIZE_256` = 256 bytes. A creator id of 0 disables subsequent ops.
- `fuji_read_appkey` returns `false` on **any** failure *including a missing
  key* (classic firmware behavior — unlike our former compat shim, there are
  no client-side defaults). Games already handle this: e.g. Battleship's
  `read_appkey()` helper treats `false` as "0 bytes read" and falls back to
  prompting for a name / showing help.
- `fuji_write_appkey` creates or replaces the whole key file.
- The compat layer's `fujinet-fuji.h` keeps its `FN_FUJI_H` include guard —
  `fn_legacy_appkey.h` checks that exact guard to avoid redefining
  `enum AppKeySize` (values match: `DEFAULT = 0`).

## Client-side obligations (fujinet-compat-amiga)

1. **Do not implement the appkey functions.** `fn_fuji.c` must not define
   `fuji_set_appkey_details` / `fuji_read_appkey` / `fuji_write_appkey`,
   or the linker would shadow nio-lib's legacy members. The compat header
   declares them; `fujinet-nio-amiga.a` (linked after the compat lib)
   defines them.
2. Keep the compat header's `enum AppKeySize` in sync with
   `fn_legacy_appkey.h` if upstream ever extends it.

## Server-side obligations (fujinet-nio)

None beyond running a build that includes the `persist://` alias
(upstream `60ca4eb` or later). The posix profile registers it by default;
keys land under the server's data root at `FujiNet/*.key`.

## Testing

- **Wire-level:** upstream owns it — `make test-legacy` in `fujinet-nio-lib`
  runs `tests/legacy_appkey_wire_test.c` against a live server. Not
  duplicated in this repo (submodule boundary, `docs/testing.md`).
- **T2:** `apps/compat_test` does an appkey write→read round-trip on the
  emulator; its pass pattern matches the final wire command
  `dev=0xFE cmd=0x03` (FileRead).
- **Seeding for emu tests:** drop a file at
  `<server data root>/FujiNet/<creator><app><key>.key` before boot, or write
  it through a prior client run — there is no app-store namespace involved.

## References

- `fujinet-nio-lib/include/fn_legacy_appkey.h` — the API (doc comments give the path scheme)
- `fujinet-nio-lib/src/legacy/` — implementation; `tests/legacy_appkey_wire_test.c`
- `fujinet-nio/docs/filesystem.md` — `persist://` alias and filesystem roots
- `fujinet-nio/docs/file_device_protocol.md` — FileDevice wire format
- `contracts/fujibus-protocol.md` — FujiBus framing and transport layer
