# Amiga Coding Conventions

Applies to: `fujinet-nio-lib/src/platform/amiga/`, `apps/`

---

## Hardware Context

- **CPU**: Motorola 68000 — 16-bit data bus, 32-bit registers, no FPU
- **OS**: AmigaOS 1.3+ (Kickstart ROM libraries, message-passing IPC)
- **RAM**: 512KB chip RAM minimum (A500 base); assume very limited stack
- **Compiler**: `m68k-amigaos-gcc` (amiga-gcc cross-compiler)
- **Standard**: C99 (`-std=c99`), `-mcpu=68000 -msoft-float`

---

## AmigaOS API Rules

### Library access
- Use `<proto/exec.h>` for exec.library functions (`CreateMsgPort`, `DoIO`, etc.)
- Include device headers from `<devices/>` (e.g. `<devices/serial.h>`)
- Include type definitions from `<exec/types.h>` — use AmigaOS types (`UBYTE`, `UWORD`, `ULONG`, `APTR`, `BYTE`) where interfacing with OS structures

### Resource lifecycle — always pair:
```c
CreateMsgPort()      / DeleteMsgPort()
CreateIORequest()    / DeleteIORequest()
OpenDevice()         / CloseDevice()
AllocMem()           / FreeMem()
```
On failure, clean up in reverse order before returning an error code.

### No dynamic allocation in library code
- `src/platform/amiga/` and `src/common/` must not call `malloc`/`free`
- Use static buffers or caller-provided buffers (see `_fn_transport_ctx`)
- `AllocMem()` is acceptable for one-time init that persists for the process lifetime

### Blocking I/O
- `DoIO()` blocks until completion — acceptable for prototype RS-232 use
- `SendIO()` / `WaitIO()` pattern for async — not needed yet but prefer `DoIO` for simplicity

### Custom screens / rendering
- **Never rely on the system default font for cell-aligned rendering.** The
  default can be the 60-column topaz (10 px wide), which silently overflows an
  8×8 cell grid and misaligns every `Text()` call against tile layouts. Request
  topaz 8 explicitly:
  ```c
  static struct TextAttr _topaz8 = {
      (STRPTR)"topaz.font", TOPAZ_EIGHTY, FS_NORMAL, FPF_ROMFONT
  };
  /* NewScreen.Font = &_topaz8; */
  ```
- Blitter/sprite source data (tiles, `struct Image` data, sprite images) must
  be staged into `AllocMem(MEMF_CHIP)` buffers at init — the `.datachip` linker
  section does **not** work on this toolchain. Full writeup:
  `apps/pacmantests/c99-pac-man/README.md`.

---

## C Style

### Types
- Use `uint8_t`, `uint16_t`, `uint32_t` (from `<stdint.h>` via `fujinet-nio.h`) for protocol/buffer data
- Use AmigaOS types (`UBYTE`, `UWORD`, `ULONG`) only when directly populating OS structures
- Do not mix them unnecessarily — cast at the boundary

### Naming
- Functions: `snake_case` — public functions prefixed `fn_`, internal helpers unprefixed
- Constants / macros: `UPPER_SNAKE_CASE`
- Static module-level variables: `_leading_underscore` (as in `_serial_port`, `_device_open`)
- No C++ name mangling — this is plain C

### Declarations
- Declare all variables at the top of a block (C89 compat for toolchain safety, even in C99 mode)
- No VLAs — fixed-size static buffers only

### Compile-time configuration
- Use `#ifndef` guards so callers can override via `-D` flags:
```c
#ifndef FN_AMIGA_BAUD
#define FN_AMIGA_BAUD 19200
#endif
```

---

## Memory and Stack

- Keep stack frames small — no large local arrays
- Large buffers must be `static` at file scope (e.g. `static UBYTE _wire_buf[...]`)
- Buffer sizes should be derived from protocol constants, not magic numbers:
```c
#define FN_TRANSPORT_WIRE_BUF_SIZE ((FN_MAX_PACKET_SIZE * 2) + 2)
```

---

## Error Handling

- All functions return `uint8_t` error codes from `fn_protocol.h` (`FN_OK`, `FN_ERR_IO`, etc.)
- Check every OS call return value; propagate errors upward immediately
- On init failure, undo all allocations before returning (no partial state)
- No `printf` / `fprintf` in library code — apps may have no console

---

## Apps (`apps/`)

- Apps are AmigaOS CLI executables — they have a `main(int argc, char *argv[])`
- `printf` is fine in app code (Amiga CLI provides a console)
- Use `static` for buffers larger than a few bytes:
```c
#define BUFFER_SIZE 512
static uint8_t buf[BUFFER_SIZE];
```
- URL / path arguments come from `argv[1]`; provide a compile-time default via `#ifndef FN_DEFAULT_URL`
- Use `%lu` with `(unsigned long)` cast for `uint32_t` values in `printf` (no `%PRIu32` on classic AmigaOS)

---

## Build Notes

- Amiga target in `fujinet-nio-lib`: `make TARGET=amiga`
- Apps built with `amiga-gcc` via `apps/<name>/Makefile`
- Include path: `fujinet-nio-lib/include/`
- Link against: `fujinet-nio-lib/build/fujinet-nio-amiga.a`
- No linking against `libc` dynamic — static link everything
- **Never use `m68k-amigaos-strip`** — it corrupts Amiga hunk binaries (Guru Meditation
  on load). The `-s` linker flag also produces bad output. If you need to strip symbols,
  use `emu/scripts/strip-hunk-symbols.py` instead

### CRT / Kickstart compatibility

The amiga-gcc toolchain defaults to **newlib**, which requires Kickstart 2.0+. Since we
target AmigaOS 1.3+ (A500 with Kickstart 1.3), always add `-mcrt=nix13` to `CFLAGS`
in app Makefiles:

```makefile
CFLAGS = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13
```

`-mcrt=nix13` selects libnix tuned for Kickstart 1.3. Omitting it produces binaries
that will crash or refuse to run on a stock A500.

---

## File Header Template

```c
/*
 * <filename> - <one-line description>
 *
 * <What it does and why.>
 *
 * Requires: exec.library, <other libs>
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * See: contracts/<relevant-contract>.md
 */
```

---

## What to Avoid

- No `malloc` / `free` in library code
- No floating point (no FPU on 68000; `-msoft-float` is slow — avoid entirely)
- No `long long` / 64-bit arithmetic (no native support on 68000)
- No POSIX headers (`<unistd.h>`, `<termios.h>`, `<fcntl.h>`) in Amiga platform code
- No KS 2.0+ dos.library calls — use V33 (KS 1.2+) equivalents. Confirmed
  V36-only traps that **link without error but Guru on KS 1.3**:

  | Function | Why it's unsafe on KS 1.3 | Safe V33 alternative |
  |----------|---------------------------|----------------------|
  | `FGetC` / `FPutC` | V36 LVO | `Read()` / `Write()` one byte |
  | `SetVBuf` | V36 LVO | `setbuf()` via libnix |
  | `SetMode` | V36 LVO (raw/cooked toggle) | Open `RAW:` window (see below) |
  | `SelectInput` / `SelectOutput` | Not in the ndk13 headers; resolve to a compat lib stub, not a V33 dos.library call | Write to / Read from the file handle directly |

  Rule of thumb: if a function is absent from `ndk13-include/clib/dos_protos.h`
  it is **not** safe on KS 1.3, even if the linker resolves it.

- **Raw, no-echo keyboard on KS 1.3**: CON: opens in cooked mode (line-buffered
  + echo); per-keypress input needs raw mode. Since `SetMode()` is V36-only,
  open a `RAW:` window instead — `Open("RAW:0/0/640/200/Title", MODE_NEWFILE)`
  uses only V33 calls. Read keys with `Read(handle, &ch, 1)` and write output
  with `Write(handle, ...)`. Do **not** rely on `SelectInput`/`SelectOutput` to
  redirect `printf`/`stdin`: the nix13 CRT binds stdio to the boot CLI handle at
  startup, so write to the RAW: handle directly. (See
  `apps/battleship/amiga/src/graphics.c` + `input.c`.)
- No C++ (`//` comments are fine in C99, but no classes, templates, references)
- No `#ifdef AMIGA` spaghetti — platform isolation is enforced by the build system; Amiga code lives in `src/platform/amiga/` only
