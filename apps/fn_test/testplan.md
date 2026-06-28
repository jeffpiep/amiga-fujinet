# fn_test Bring-up Test Plan

End-to-end RS-232 bring-up: Amiga A500 ↔ Linux FujiNet.

---

## Phase 1 — Physical Setup & Linux Serial Sanity

### Step 1: Verify the cable

You need a **null-modem** cable, not straight-through. Minimum 3-wire
(per `contracts/rs232-hardware.md`):

| Amiga DB-25 pin | → | Linux DB-9 pin |
|---|---|---|
| 2 (TXD) | → | 2 (RXD) |
| 3 (RXD) | ← | 3 (TXD) |
| 7 (GND) | — | 5 (GND) |

If you have a DB-25 straight cable with a DB-25→DB-9 adapter, confirm the
adapter is a **null-modem** type (crosses TX↔RX), not a straight one.

### Step 2: Confirm serial port access

`/dev/ttyUSB0` should be present (USB→serial adapter plugged in). Verify:

```bash
picocom -b 19200 --omap crlf /dev/ttyUSB0
# Ctrl-A Ctrl-X to exit
```

If picocom opens without "permission denied" or "device busy", the adapter
is accessible. If you get a permission error, add yourself to the `dialout`
group:

```bash
sudo usermod -aG dialout $USER   # log out and back in
```

### Step 3: Loopback test (optional but recommended)

Before involving the Amiga, do a loopback on the Linux DB-9 end. Bridge
**pin 2 and pin 3** on the DB-9 with a jumper wire (or use a DB-9 loopback
plug). Open picocom — characters you type should echo back. This confirms
the USB→serial adapter sends and receives correctly.

Remove the jumper before connecting the Amiga.

---

## Phase 2 — Build fn_test for Amiga

### Step 4: Add amiga-gcc to PATH

The toolchain is installed at `/opt/amiga/bin` (not on PATH by default):

```bash
export PATH=/opt/amiga/bin:$PATH
m68k-amigaos-gcc --version   # confirm it prints a version
```

### Step 5: Confirm the Amiga library is built

```bash
ls -la ../../fujinet-nio-lib/build/fujinet-nio-amiga.a
```

If missing, build it first:

```bash
cd ../../fujinet-nio-lib
PATH=/opt/amiga/bin:$PATH make TARGET=amiga
```

### Step 6: Build fn_test

```bash
# from apps/fn_test/
make
```

Verify it's an m68k binary:

```bash
file fn_test
# expected: Amiga OS/M68k Hunk executable  (or similar m68k output)
```

---

## Phase 3 — Transfer fn_test to the Amiga A500

No USB floppy drive is assumed. Choose one option.

### Option A — Gotek / HxC floppy emulator (preferred)

Create an ADF disk image containing fn_test, load it on the Gotek.

Install amitools if not present:

```bash
pip3 install amitools
```

Create and populate the ADF:

```bash
xdftool fn_test.adf create + format "FNTEST" ofs
xdftool fn_test.adf write fn_test
xdftool fn_test.adf list   # verify the file is there
```

Copy `fn_test.adf` to the Gotek USB stick or SD card. Boot the Amiga from
it. From a CLI/Shell:

```
1> fn_test
```

### Option B — XMODEM over RS-232

Requires a terminal program on the Amiga (NComm, Term, VLT, etc.).

1. On Amiga: configure terminal for 19200 8N1, start **XMODEM receive**
   to `RAM:fn_test`.
2. On Linux:
   ```bash
   sx -b fn_test > /dev/ttyUSB0 < /dev/ttyUSB0
   ```
3. After transfer, from Amiga CLI:
   ```
   1> protect RAM:fn_test +e
   1> RAM:fn_test
   ```

### Option C — Physical floppy via another machine

Use Greaseweazle, Kryoflux, or similar to write the ADF created in
Option A to a real 880 KB DD floppy.

---

## Phase 4 — Run fn_test on the Amiga

### Step 7: Open a Shell/CLI on the Amiga

Workbench 1.3: double-click the Workbench disk icon → open CLI or Shell.

### Step 8: Run fn_test

```
1> fn_test
```

**Expected success:**

```
fn_test: calling fn_init()...
fn_init() OK
fn_test: calling fn_is_ready()...
FujiNet is ready.
```

**Expected failure** if `serial.device` cannot be opened:

```
fn_init() failed: not found (0x01)
```

> fn_test only verifies that `serial.device` opens and configures at
> 19200 8N1 (`SERF_XDISABLED`). It does **not** send data to the Linux
> side. The Linux server does not need to be running for this step to pass.

---

## Phase 5 — Build & Run fujinet-nio on Linux

### Step 9: Confirm or rebuild the server

The binary at `fujinet-nio/build/fujibus-rs232-debug/fujinet-nio` should
already be built. Verify or rebuild:

```bash
cd fujinet-nio
./build.sh -p fujibus-rs232-debug
```

### Step 10: Run the server

```bash
cd fujinet-nio/build/fujibus-rs232-debug
FN_SERIAL_PORT=/dev/ttyUSB0 FN_SERIAL_BAUD=19200 ./fujinet-nio
```

Expected startup output:

```
Starting fujinet-nio
[info] serial: /dev/ttyUSB0 @ 19200
[info] waiting for client...
```

---

## Phase 6 — End-to-End Wire Test

fn_test does not send FujiBus packets — it only opens the serial device.
Use the Python CLI to verify the full wire path.

### Step 11: Linux-side protocol test

With fujinet-nio running in another terminal, send a real FujiBus packet
from the Linux side:

```bash
cd fujinet-nio
./scripts/fujinet --port /dev/ttyUSB0 --baud 19200 clock
```

A response with the current time confirms the server speaks FujiBus
correctly over the serial port.

### Step 12: Amiga-initiated round trip (next milestone)

Once Steps 8 and 11 both pass you have confidence that:

- Amiga opens `serial.device` correctly
- Linux server opens the port and speaks FujiBus

The next step is running `http_get` from the Amiga, which is the first
true end-to-end round trip (Amiga sends packet → Linux server → internet →
response back to Amiga).

---

## Quick Reference

| Setting | Value |
|---|---|
| Linux device | `/dev/ttyUSB0` |
| Baud rate | 19200 |
| Format | 8N1, no parity, no flow control |
| Amiga `serial.device` unit | 0 |
| XON/XOFF | disabled (`SERF_XDISABLED`) |
| amiga-gcc path | `/opt/amiga/bin` |
| Library | `fujinet-nio-lib/build/fujinet-nio-amiga.a` |
| Server binary | `fujinet-nio/build/fujibus-rs232-debug/fujinet-nio` |
