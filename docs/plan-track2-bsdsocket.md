# Implementation Plan: Track 2 — BSD Socket Compatibility Layer

**Depends on:** nothing (builds on top of `fujinet-nio-lib`)  
**Blocks:** nothing (Track 1 and Track 2 are independent)  
**Status:** Not started

---

## Goal

Produce `libfn_bsdsocket.a` — a static library for amiga-gcc that provides
standard BSD socket calls (`socket`, `connect`, `send`, `recv`, `closesocket`)
backed by `fujinet-nio-lib`. Amiga programs compiled with amiga-gcc can use
normal network I/O with no TCP stack (no Miami, no AmiTCP) installed.

Stretch: once the static lib is proven, promote to a proper AmigaOS `.library`
that any binary can open with `OpenLibrary("bsdsocket.library", 4)`.

---

## Location

```
libs/fujinet-bsdsocket/
  include/
    sys/socket.h       ← BSD socket types and function declarations
    netinet/in.h       ← sockaddr_in, IPPROTO_*, htons/ntohs, etc.
    netdb.h            ← gethostbyname, getaddrinfo (Phase 2)
    fn_bsd_ext.h       ← fn_bsd_set_tls() and other FujiNet-specific extensions
  src/
    fn_socket.c        ← socket/connect/send/recv/closesocket
    fn_dns.c           ← gethostbyname / getaddrinfo (Phase 2)
    fn_select.c        ← select / WaitSelect (Phase 4)
  Makefile
```

---

## Phase 1 — Core TCP Socket Operations

### Socket slot table

```c
typedef struct {
    fn_handle_t  handle;
    uint32_t     tx_cursor;
    uint32_t     rx_cursor;
    uint8_t      tls;       /* FN_OPEN_TLS requested */
    uint8_t      in_use;
} FnSocket;

static FnSocket sockets[FN_MAX_SESSIONS];  /* 4 slots */
```

Keep state per-slot (not global) so a future LibBase can scope this table
per opener. Access through a static inline `get_socket(fd)` helper that
validates the fd and returns a pointer, or NULL on bad fd.

### `socket(domain, type, protocol) → fd`

Only `AF_INET` + `SOCK_STREAM` is supported (TCP). Return a small integer
fd (0–3). Return `-1` with no errno if all slots are full.

`domain`, `type`, `protocol` params are accepted but `AF_INET6`, `SOCK_DGRAM`,
`SOCK_RAW` etc. return `-1` (unsupported — FujiNet doesn't expose raw sockets).

### `connect(fd, sockaddr_in*, addrlen) → 0 or -1`

1. Extract host and port from `sockaddr_in` (network byte order → host byte order).
2. Build URL: `tcp://a.b.c.d:port` or, if Phase 2 DNS is done, resolve first.
3. If `tls` flag set on the slot, use `tls://` scheme and `FN_OPEN_TLS`.
4. Call `fn_tcp_open(&handle, host_str, port)`.
5. Store `handle` in slot. Return 0 on success, -1 on error.

**Note on addresses:** callers that use `gethostbyname()` (Phase 2) will pass
a resolved IPv4 address. For Phase 1, build the URL from the raw `in_addr`
bytes and let the FujiNet server connect directly. This works because the server
resolves the connection, not the Amiga.

### `send(fd, buf, len, flags) → bytes_sent or -1`

1. Look up slot; validate `in_use`.
2. Call `fn_write(handle, tx_cursor, buf, len, &written)`.
3. Advance `tx_cursor += written`.
4. Return `written` (or -1 on error).

`flags` param accepted but ignored (MSG_OOB, MSG_DONTWAIT not supported).

### `recv(fd, buf, len, flags) → bytes_read, 0 (EOF), or -1`

1. Look up slot.
2. Poll `fn_read(handle, rx_cursor, buf, len, &bytes_read, &read_flags)`:
   - `FN_ERR_NOT_READY` / `FN_ERR_BUSY` → retry (blocking recv polls)
   - EOF flag → return 0
   - Error → return -1
3. Advance `rx_cursor += bytes_read`.
4. Return `bytes_read`.

### `closesocket(fd) → 0 or -1`

Call `fn_close(handle)`, zero out the slot. Return 0.

### `fn_bsd_set_tls(fd)` — FujiNet extension

Sets the `tls` flag on a socket slot before `connect()` is called. Allows TLS
on non-443 ports. Declared in `fn_bsd_ext.h` — not part of the standard BSD
header.

### Headers

`sys/socket.h` and `netinet/in.h` must define only what is needed; they must
not conflict with anything in `<exec/types.h>` or other AmigaOS headers.
Use `#ifndef` guards and avoid redefining types that amiga-gcc's libnix already
provides (`uint16_t`, etc.).

### Makefile

Mirror `apps/http_get/Makefile`. Produce `libfn_bsdsocket.a`.

### Smoke test

`libs/fujinet-bsdsocket/test/socket_test.c`:

```c
int fd = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in addr = { AF_INET, htons(80), { inet_addr("93.184.216.34") } };
connect(fd, (struct sockaddr*)&addr, sizeof(addr));
send(fd, "GET / HTTP/1.0\r\nHost: example.com\r\n\r\n", 38, 0);
char buf[512];
int n;
while ((n = recv(fd, buf, sizeof(buf)-1, 0)) > 0) {
    buf[n] = 0; puts(buf);
}
closesocket(fd);
```

Build, boot in emulator, confirm HTTP response received.

---

## Phase 2 — DNS: `gethostbyname` / `getaddrinfo`

**Option A (preferred, zero protocol changes):** The FujiNet server already
resolves hostnames when given `tcp://hostname:port`. `connect()` can accept a
hostname string in `sockaddr_in` by using a non-standard `sa_data` field — but
this is fragile. Better: implement `gethostbyname()` as a pass-through that
stores the hostname string and returns a fake `in_addr` that `connect()` then
decodes back into the URL.

Concrete approach for Option A:
- Maintain a small hostname cache: `{ uint32_t fake_ip, char hostname[64] }[4]`
- `gethostbyname(name)` assigns a fake IP (e.g., `0x7F000001` + slot index),
  stores `name → fake_ip` mapping, returns `hostent` pointing at fake IP
- `connect()` checks if `in_addr` matches a fake IP; if so, uses the stored
  hostname string in the `tcp://hostname:port` URL instead

**Option B** (only if a real app requires pre-resolution): Add a DNS query
command to FujiBus so the Amiga receives a real IP string back. Requires
changes to both `fujinet-nio` and `fujinet-nio-lib`.

Implement Option A first.

---

## Phase 3 — TLS Auto-Detection

- `connect()` to port 443 automatically sets `tls = 1` on the slot and uses
  `tls://` scheme
- `fn_bsd_set_tls(fd)` for explicit TLS on non-443 ports
- No Amiga-side TLS negotiation — the FujiNet server handles it entirely

---

## Phase 4 — Non-Blocking I/O / WaitSelect

### `select(nfds, readfds, writefds, exceptfds, timeout)`

- Poll each fd in `readfds` with a zero-timeout `fn_read()` call
- If `FN_ERR_NOT_READY` → fd not ready; if data returned → mark ready
- Loop until at least one fd is ready or `timeout` expires
- Uses `timer.device` for timeout tracking (KS 1.3 safe)

### `WaitSelect(nfds, readfds, writefds, exceptfds, timeout, sigmask)`

AmigaOS extension. Like `select()` but also waits on an Amiga signal mask
(e.g., window IDCMP signals). Implementation:
1. Check all fds for readiness (zero-timeout poll)
2. If none ready: `Wait(sigmask | timeoutSignal)` with a short timeout
3. Loop until a fd is ready, a signal fires, or timeout expires
4. Return ready fds and which signals fired

Uses `exec.library` `Wait()` — available on KS 1.3.

---

## Stretch — Proper AmigaOS `.library`

When the static lib is proven on real hardware, wrap it as a proper AmigaOS
shared library:

1. Add a `LibBase` struct extending `Library` with a pointer to per-opener
   socket table (allocated from `exec.library` `AllocMem()` in `LibOpen`)
2. Write a jump table: one XDEF entry per exported function using
   `__asm__` register stubs (amiga-gcc supports `__attribute__((regparm(...)))`)
3. Implement `LibOpen`, `LibClose`, `LibExpunge`, `LibNull` lifecycle functions
4. Link with amiga-gcc producing a relocatable `.library` binary
5. Place in `LIBS:bsdsocket.library` on ADF

At this point any AmigaOS binary that opens `bsdsocket.library` (Miami-style)
will use FujiNet for all networking.

**Design rule enforced from Phase 1:** The per-slot state table is already
scoped per-opener, so Phase 1–4 code promotes to the `.library` without
restructuring.

---

## Verification Checklist

- [ ] `libfn_bsdsocket.a` builds for `TARGET=amiga` (Phase 1)
- [ ] Smoke test: HTTP GET via raw IP address in emulator (Phase 1)
- [ ] `gethostbyname("example.com")` resolves and `connect()` succeeds (Phase 2)
- [ ] Port 443 auto-enables TLS; HTTPS response received (Phase 3)
- [ ] `select()` returns correct ready-fds with a 1-second timeout (Phase 4)
- [ ] Existing amiga-gcc app using BSD sockets compiles and runs without
      Miami/AmiTCP present (Phase 1–3 combined)
- [ ] Tested on real Amiga 500 + PiStorm (Phase 1)
