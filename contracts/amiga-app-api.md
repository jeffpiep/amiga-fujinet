# Amiga App Developer API Contract

## Overview

This contract documents the `fujinet-nio-lib` public API for writing Amiga apps.
The library runs on the Amiga and communicates with the `fujinet-nio` server over
RS-232 serial using the FujiBus/SLIP protocol.

**Header:** `#include "fujinet-nio.h"`  
**Library:** `fujinet-nio-lib/build/fujinet-nio-amiga.a`  
**Reference apps:** `apps/fn_test/`, `apps/http_get/`

---

## Initialization

```c
uint8_t fn_init(void);
uint8_t fn_is_ready(void);
```

Always call these first. `fn_init()` opens `serial.device`; `fn_is_ready()` does
a ping to confirm the server is alive.

```c
uint8_t err = fn_init();
if (err != FN_OK) { printf("Init failed: %s\n", fn_error_string(err)); return 1; }
if (!fn_is_ready()) { printf("FujiNet not ready\n"); return 1; }
```

---

## HTTP GET (most common)

```c
uint8_t fn_open(fn_handle_t *handle, uint8_t method, const char *url, uint8_t flags);
uint8_t fn_info(fn_handle_t handle, uint16_t *http_status, uint32_t *content_length, uint8_t *flags);
uint8_t fn_read(fn_handle_t handle, uint32_t offset, uint8_t *buf, uint16_t max_len,
                uint16_t *bytes_read, uint8_t *flags);
uint8_t fn_close(fn_handle_t handle);
```

### HTTPS auto-detect pattern (from `apps/http_get/`)

```c
fn_handle_t handle;
uint8_t open_flags = 0;
if (strncmp(url, "https://", 8) == 0 || strncmp(url, "tls://", 6) == 0)
    open_flags |= FN_OPEN_TLS;

uint8_t result = fn_open(&handle, FN_METHOD_GET, url, open_flags);
if (result != FN_OK) { /* handle error */ }
```

### Read loop

The server fetches asynchronously. Keep retrying `FN_ERR_NOT_READY` / `FN_ERR_BUSY`
until data arrives or a real error occurs.

```c
static uint8_t buf[512];
uint32_t offset = 0;

for (;;) {
    uint16_t bytes_read;
    uint8_t  read_flags;
    uint8_t  result = fn_read(handle, offset, buf, sizeof(buf) - 1,
                              &bytes_read, &read_flags);

    if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY)
        continue;                   /* server still fetching — poll */
    if (result != FN_OK)
        break;                      /* real error */
    if (bytes_read == 0)
        break;                      /* done */

    buf[bytes_read] = '\0';
    /* process buf here */
    offset += bytes_read;

    if (read_flags & FN_READ_EOF)
        break;
}
fn_close(handle);
```

### HTTP status and content-length

Call `fn_info()` after `fn_open()` and before the read loop. It may also return
`FN_ERR_NOT_READY` if the server hasn't connected yet — retry or skip if not needed.

```c
uint16_t http_status;
uint32_t content_length;
uint8_t  info_flags;
if (fn_info(handle, &http_status, &content_length, &info_flags) == FN_OK) {
    if (info_flags & FN_INFO_HAS_STATUS)  printf("HTTP %u\n", http_status);
    if (info_flags & FN_INFO_HAS_LENGTH)  printf("Length: %lu\n", content_length);
}
```

---

## HTTP POST

```c
fn_open(&handle, FN_METHOD_POST, url, open_flags);

const char *body = "{\"key\":\"value\"}";
uint16_t written;
fn_write(handle, 0, (const uint8_t *)body, strlen(body), &written);
fn_write(handle, written, NULL, 0, &written);  /* signal end of body */

/* then read response as normal */
```

---

## TCP / Raw socket

```c
fn_handle_t handle;
fn_open(&handle, 0, "tcp://hostname:port", 0);        /* method=0 for TCP */
/* or: fn_tcp_open(&handle, "hostname", port);        convenience wrapper  */

/* send */
uint16_t written;
fn_write(handle, 0, (const uint8_t *)"hello\r\n", 7, &written);
fn_write(handle, written, NULL, 0, &written);  /* half-close write side */

/* receive — same polling loop as HTTP read */
```

For TLS sockets: use `"tls://hostname:port"` and `FN_OPEN_TLS` flag.

---

## Open flags

| Flag | Value | Meaning |
|------|-------|---------|
| `FN_OPEN_TLS` | `0x01` | Enable TLS (required for `https://` or `tls://`) |
| `FN_OPEN_FOLLOW_REDIR` | `0x02` | Auto-follow HTTP redirects |
| `FN_OPEN_ALLOW_EVICT` | `0x08` | Evict oldest handle if all slots full |

**HTTPS requires `FN_OPEN_TLS`.** The server does NOT auto-detect the URL scheme —
set the flag explicitly (or use the auto-detect pattern from http_get above).

---

## Error codes

| Code | Value | Meaning |
|------|-------|---------|
| `FN_OK` | `0x00` | Success |
| `FN_ERR_NOT_FOUND` | `0x01` | serial.device not found / FujiNet not present |
| `FN_ERR_INVALID` | `0x02` | Bad parameter |
| `FN_ERR_BUSY` | `0x03` | Retry |
| `FN_ERR_NOT_READY` | `0x04` | Data not ready yet — poll again |
| `FN_ERR_IO` | `0x05` | I/O error (often: transport timeout after server never responded) |
| `FN_ERR_TIMEOUT` | `0x06` | Device timeout |
| `FN_ERR_TRANSPORT` | `0x10` | Serial transport error |
| `FN_ERR_URL_TOO_LONG` | `0x11` | URL exceeds 256 bytes |
| `FN_ERR_NO_HANDLES` | `0x12` | All 4 session slots in use |

`FN_ERR_IO` during a read loop usually means the Amiga transport timed out waiting
for data (default: 50 retries × 100ms = 5 seconds). If the remote server is slow
(e.g. connection refused, TLS timeout), the read will fail with `FN_ERR_IO` after
~5 seconds even though `fn_open()` returned `FN_OK`.

Use `fn_error_string(err)` to print a human-readable message.

---

## Constraints

| Limit | Value | Notes |
|-------|-------|-------|
| Max URL length | 256 bytes | |
| Max concurrent sessions | 4 | Use `fn_close()` when done |
| Max read/write chunk | 512 bytes | Read in a loop for larger responses |
| Serial baud rate | 19200 | ~2.4 KB/s throughput |
| No dynamic allocation | — | All buffers must be caller-provided (static or stack) |
| No large stack allocations | — | Amiga 68000 stack is limited; use `static` for large buffers |

---

## Clock API

The FujiNet server provides network time — useful for timestamping or display.

```c
uint8_t time_data[7];
fn_clock_get_format(time_data, FN_TIME_FORMAT_SIMPLE);
/* time_data = [century, year, month, day, hour, min, sec] */
printf("20%02u-%02u-%02u %02u:%02u:%02u\n",
       time_data[1], time_data[2], time_data[3],
       time_data[4], time_data[5], time_data[6]);
```

Available formats: `FN_TIME_FORMAT_SIMPLE`, `FN_TIME_FORMAT_PRODOS`,
`FN_TIME_FORMAT_APETIME`, `FN_TIME_FORMAT_TZ_ISO`, `FN_TIME_FORMAT_UTC_ISO`,
`FN_TIME_FORMAT_APPLE3_SOS`.

---

## Makefile pattern

See `apps/http_get/Makefile` or `apps/fn_test/Makefile` for the canonical template.
Key points:

```makefile
CC     = m68k-amigaos-gcc
CFLAGS = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13

NIO_LIB  = ../../fujinet-nio-lib
INCLUDES = -I$(NIO_LIB)/include
LIBS     = $(NIO_LIB)/build/fujinet-nio-amiga.a
```

`-mcrt=nix13` is **required** for KS 1.3 compatibility (see `contracts/amiga-coding-conventions.md`).
Link the `.a` archive directly — do not use `-L / -l` with it.
