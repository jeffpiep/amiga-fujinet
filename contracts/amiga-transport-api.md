# Amiga Transport API Contract

## Overview

This contract defines what must be implemented in:
- `fujinet-nio-lib/src/platform/amiga/fn_transport.c`
- `fujinet-nio-lib/makefiles/targets.mk` (amiga target entry)
- `fujinet-nio-lib/makefiles/compiler-amigagcc.mk` (new compiler config)
- `fujinet-nio-lib/Makefile` (amiga target wiring)

The Amiga transport bridges the fujinet-nio-lib protocol stack to the
Amiga `serial.device` via the exec.library IORequest mechanism.

---

## Interface to Implement (`fn_platform.h`)

```c
uint8_t     fn_transport_init(void);
uint8_t     fn_transport_ready(void);
uint8_t     fn_transport_exchange(void);
const char *fn_platform_name(void);
```

All functions are defined in `include/fn_platform.h`. The exchange context is
passed via `_fn_transport_ctx` (see `fn_internal.h`).

---

## fn_transport_init()

**Purpose:** Open `serial.device` unit 0, configure for binary SLIP transport.

**Steps:**
1. `CreateMsgPort()` — create reply port
2. `CreateIORequest(port, sizeof(struct IOExtSer))` — create IO request block
3. `OpenDevice("serial.device", 0, (struct IORequest *)ioReq, 0)`
4. Set `IOExtSer` fields:
   - `io_Baud = 19200` (from `FN_AMIGA_BAUD` env or compile-time default)
   - `io_ReadLen = 8`, `io_WriteLen = 8`
   - `io_StopBits = 1`
   - `io_SerFlags = SERF_XDISABLED` (MUST disable XON/XOFF — SLIP uses 0xC0/0xDB)
   - Clear `SERF_7WIRE` for 3-wire no-flow-control
5. `CMD_SETPARAMS` IORequest to apply settings
6. Return `FN_OK` on success, `FN_ERR_NOT_FOUND` if device not found

**State:** Store `MsgPort *`, `IOExtSer *`, and an `isOpen` flag as static module state.

---

## fn_transport_ready()

**Purpose:** Return 1 if device is open and ready, 0 otherwise.

Simple: return `isOpen ? 1 : 0`.

---

## fn_transport_exchange()

**Purpose:** Send SLIP-framed request, receive SLIP-framed response.

Input/output via `_fn_transport_ctx` (from `fn_internal.h`):
```c
_fn_transport_ctx.request   // const uint8_t* — FujiBus packet to send
_fn_transport_ctx.req_len   // uint16_t — length of request
_fn_transport_ctx.response  // uint8_t* — buffer for decoded response
_fn_transport_ctx.resp_max  // uint16_t — max response buffer size
_fn_transport_ctx.resp_len  // uint16_t* — write actual response length here
```

**Steps:**
1. SLIP-encode request into local wire buffer (use `fn_slip_encode()` from common/)
2. Write encoded bytes via `serial.device` `CMD_WRITE` IORequest
3. Wait for write completion (`WaitIO()`)
4. Read bytes one at a time (or in chunks) via `CMD_READ` IORequest with timeout
5. Accumulate into wire buffer until second `0xC0` received
6. SLIP-decode via `fn_slip_decode()` into `_fn_transport_ctx.response`
7. Set `_fn_transport_ctx.resp_len`
8. Return `FN_OK` or appropriate error

**Timeout:** Use `timer.device` for read timeout (default `FN_TRANSPORT_TIMEOUT` = 5000ms).
Alternative for prototype: use a simple retry counter (less accurate but simpler).

**Wire buffer size:** `FN_TRANSPORT_WIRE_BUF_SIZE = (FN_MAX_PACKET_SIZE * 2) + 2`

---

## fn_platform_name()

```c
const char *fn_platform_name(void) { return "amiga"; }
```

---

## Build System Changes

### makefiles/targets.mk — add:
```makefile
PLATFORM_amiga          := amiga
COMPILER_FAMILY_amiga   := amigagcc
TARGET_CFLAGS_amiga     := -mcpu=68000 -msoft-float
TARGET_ASFLAGS_amiga    :=
```

### makefiles/compiler-amigagcc.mk — new file:
```makefile
# compiler-amigagcc.mk - amiga-gcc cross-compiler for m68k-amigaos
CC      = m68k-amigaos-gcc
AR      = m68k-amigaos-ar
AS      = m68k-amigaos-as
CFLAGS  = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -DNO_INLINE_MULDIV
ARFLAGS = rcs
LIBEXT  = .a
```

### makefiles/compiler.mk — add branch:
```makefile
else ifeq ($(COMPILER_FAMILY),amigagcc)
    -include makefiles/compiler-amigagcc.mk
```

### Makefile — add amiga to ALL_TARGETS:
```makefile
ALL_TARGETS = atari bbc linux amiga
```

### fn_platform.h — add Amiga detection:
```c
#ifdef __amigaos__
    #define FN_PLATFORM_AMIGA    1
    #define FN_PLATFORM_NAME     "amiga"
#endif
```

---

## Key AmigaOS Headers Required

```c
#include <exec/types.h>
#include <exec/memory.h>
#include <devices/serial.h>
#include <proto/exec.h>
```

Cross-compiler provides these via `m68k-amigaos-gcc` NDK headers.

---

## Reference

- Linux transport reference: `src/platform/linux/fn_transport.c`
- Platform interface: `include/fn_platform.h`
- SLIP functions: `src/common/fn_slip.c`
- Internal context: `include/fn_internal.h`
- Hardware details: `../contracts/rs232-hardware.md`
- Protocol details: `../contracts/fujibus-protocol.md`
