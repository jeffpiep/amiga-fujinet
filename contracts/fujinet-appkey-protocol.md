# FujiNet AppKey Protocol

## Overview

AppKeys are small (up to 64 bytes) named blobs that apps persist on the FujiNet
device. They are keyed by a three-part address — **creator**, **app**, **key** —
and stored as flat files on the FujiNet server's storage. Any platform that speaks
FujiBus can read and write its own keys without a network connection.

This contract specifies the four-command protocol session used between the Amiga
client library (`libs/fujinet-compat-amiga`) and the FujiNet server (`fujinet-nio`).

**Scope:** 64-byte (DEFAULT) key size. 256-byte keys are defined in the original
firmware but are out of scope for this implementation.

---

## Key Identifiers

| Field        | Type      | Description |
|--------------|-----------|-------------|
| `creator_id` | uint16_t  | Registered creator ID. Battleship: `0xE41C`. Lobby: `0x0001`. |
| `app_id`     | uint8_t   | Application within the creator. Battleship prefs: `0x05`. Lobby: `0x01`. |
| `key_id`     | uint8_t   | Slot within the app. Battleship prefs: `0x00`. |

Known keys used by Battleship:

| Creator  | App    | Key    | Contents |
|----------|--------|--------|----------|
| `0x0001` | `0x01` | `0x00` | Lobby player name (string, max 8 bytes) |
| `0x0001` | `0x01` | `0x05` | Lobby server URL (string, up to 64 bytes) |
| `0xE41C` | `0x05` | `0x00` | Battleship prefs struct (byte 0 = debugFlag, byte 1 = seenHelp) |

---

## Commands

| Command       | Byte   | Direction      |
|---------------|--------|----------------|
| OPEN_APPKEY   | `0xDC` | client → server |
| READ_APPKEY   | `0xDD` | client ← server |
| WRITE_APPKEY  | `0xDE` | client → server |
| CLOSE_APPKEY  | `0xDB` | client → server |

---

## Command Details

### OPEN_APPKEY (0xDC)

Sets the server-side session context for a subsequent read or write. Must be called
before READ_APPKEY or WRITE_APPKEY. Opening a new session while one is already open
implicitly closes the prior session.

**Request payload — 6 bytes:**

```
Offset  Size  Type     Field
------  ----  -------  -----
0       2     uint16_t creator_id  (little-endian)
2       1     uint8_t  app_id
3       1     uint8_t  key_id
4       1     int8_t   mode        (0 = READ, 1 = WRITE)
5       1     uint8_t  reserved    (0x00)
```

**Response:** no payload; success or error status only.

**Validation:**
- `creator_id` must be non-zero.
- `mode` must be `0` (READ) or `1` (WRITE).
- Storage must be available.

---

### WRITE_APPKEY (0xDE)

Writes key data to the server. Session must already be open in WRITE mode.

**Request payload:** raw key data, 1–64 bytes. Length is carried in the FujiBus
command-frame auxiliary bytes (`aux1` = low byte, `aux2` = high byte of length).

**Response:** no payload; success or error status only.

After a successful write the session is automatically closed (creator reset to 0).

---

### READ_APPKEY (0xDD)

Reads key data from the server. Session must already be open in READ mode.

**Request payload:** none.

**Response payload — 2 + 64 bytes (66 bytes total):**

```
Offset  Size  Type     Field
------  ----  -------  -----
0       2     uint16_t data_len  (little-endian; 0 if key does not exist)
2       N     uint8_t  key data  (N = data_len; remaining bytes zero-padded to 64)
```

The full 66-byte buffer is always returned. If the key file does not exist on the
server, `data_len` = 0 and all data bytes are zero — this is not an error.

After a successful read the session is automatically closed.

---

### CLOSE_APPKEY (0xDB)

Resets server-side session state. No-op if no session is open. Always succeeds.

**Request payload:** none.  
**Response:** no payload.

---

## Server-Side Storage

Keys are stored as flat files:

```
<storage_root>/FujiNet/<CREATOR><APP><KEY>.key
```

Where:
- `<CREATOR>` — 4 lowercase hex digits (uint16_t printed as `%04x`)
- `<APP>` — 2 lowercase hex digits
- `<KEY>` — 2 lowercase hex digits

Examples:
- creator=`0xE41C`, app=`0x05`, key=`0x00` → `FujiNet/e41c0500.key`
- creator=`0x0001`, app=`0x01`, key=`0x00` → `FujiNet/00010100.key`

The `FujiNet/` subdirectory is created automatically on first write if absent.

On POSIX builds (`fujinet-nio`) `<storage_root>` is the `"host"` filesystem
registered in `StorageManager` (default: `./fujinet-data`).

---

## Session State Machine

```
[IDLE]
  │
  ├─OPEN(mode=READ)──▶ [OPEN-READ]
  │                         │
  │                         ├─READ_APPKEY──▶ [IDLE]  (auto-close)
  │                         └─CLOSE────────▶ [IDLE]
  │
  └─OPEN(mode=WRITE)─▶ [OPEN-WRITE]
                            │
                            ├─WRITE_APPKEY─▶ [IDLE]  (auto-close)
                            └─CLOSE────────▶ [IDLE]
```

READ_APPKEY or WRITE_APPKEY while in the wrong mode (or with no session open)
returns an error response and leaves session state unchanged.

---

## Error Handling

| Condition | Response |
|-----------|----------|
| OPEN with `creator_id == 0` | IOError |
| OPEN with invalid mode | IOError |
| OPEN with storage unavailable | IOError |
| READ/WRITE with no session open | IOError |
| WRITE in READ mode (or vice-versa) | IOError |
| Key file not found on READ | Success; `data_len` = 0, data = zeros |
| File I/O failure during READ/WRITE | IOError |

---

## Client-Side Obligations (fujinet-compat-amiga)

`fuji_set_appkey_details()` / `fuji_read_appkey()` / `fuji_write_appkey()` in
`libs/fujinet-compat-amiga/src/fn_fuji.c` must, when a real FujiNet serial
connection is active, send the OPEN → READ/WRITE → CLOSE command sequence over
the transport rather than touching local Amiga files.

The current local-file implementation (`SYS:fujinet/<creator>/<app>/<key>`) remains
as the emulator/no-FujiNet fallback only.

---

## Server-Side Obligations (fujinet-nio)

`FujiDevice` in `fujinet-nio` must:

1. Add enum values to `include/fujinet/io/devices/fuji_commands.h`:
   ```cpp
   OpenAppKey  = 0xDC,
   ReadAppKey  = 0xDD,
   WriteAppKey = 0xDE,
   CloseAppKey = 0xDB,
   ```

2. Add a session-state struct member to `FujiDevice`:
   ```cpp
   struct AppKeyCtx {
       uint16_t creator{0};
       uint8_t  app{0};
       uint8_t  key{0};
       int8_t   mode{-1};  // -1 = closed, 0 = read, 1 = write
   } _appkey_ctx;
   ```

3. Implement `handle_open_appkey`, `handle_read_appkey`, `handle_write_appkey`,
   `handle_close_appkey` in `src/lib/fuji_device.cpp`, dispatching from
   `FujiDevice::handle()`.

4. File I/O via `_storage.get("host")` → `IFileSystem::open()` / `read()` / `write()`.

---

## Reference

- `fujinet-firmware/lib/device/sio/fuji.cpp` — canonical protocol implementation (lines 739–927)
- `fujinet-firmware/lib/device/sio/fuji.h` — `appkey` struct definition
- `libs/fujinet-compat-amiga/src/fn_fuji.c` — Amiga client (local-file fallback)
- `contracts/fujibus-protocol.md` — FujiBus framing and transport layer
