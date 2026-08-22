# Debug: Amiga serial port — no working link

**Status: In progress (2026-08-22)** — Link bring-up blocked on an Amiga-side
hardware fault. Linux side, USB adapter, and cable are all proven good;
the Amiga fails serial loopback on its own connector. Hardware troubleshooting
deferred.

## Symptom

No FujiBus link between the Linux box and the Amiga (CaffeineOS, KS 3.1, on
PiStorm). Bisected away from `fujinet-nio` entirely by testing the raw serial
link with terminal programs rather than the protocol stack.

Two distinct faults, in opposite directions:

- **Amiga → Linux: nothing at all.** `ECHO >SER: "text"` returns to the prompt
  immediately with no error, and zero bytes reach `/dev/ttyUSB0`.
- **Linux → Amiga: data arrives, but corrupted.** Bytes definitely land (the
  Amiga reacts to them), but render as high-ASCII/accented characters.
  Deterministic — the same input produces very nearly the same garbage.

## Proven good — do not retest

| Link | How it was proven |
|------|-------------------|
| FTDI FT232R adapter + dock USB path | Loopback jumper on DB-9 pins 2–3; miniterm echoes |
| Whole cable, end to end | Loopback jumper on the DB-25 pins 2–3; miniterm echoes |
| Linux port config | 19200 8N1, no flow control, r/w permission via `dialout`/ACL |

## Proven bad

**The Amiga fails loopback on its own DB-25**, with the cable, the adapter, and
Linux all disconnected. Jumper across pins 2–3, Retro32 with local echo off,
typed characters do not come back. The fault is inside the Amiga's serial path.

## Ruled out

- **`SER:` mount missing** — `ASSIGN` lists `SER` under Devices.
- **Flow control blocking transmit** — a 7-wire Amiga waiting on CTS would make
  the write *hang*; `ECHO >SER:` returns immediately.
- **`serial.device` unit 0 held by another program** — a failed `OpenDevice()`
  makes DOS print an error; the write returned silently, i.e. succeeded.
- **Baud mismatch** — simulated a UART decoding the exact transmitted waveform
  at every rate from 300 to 120000 baud, matching the observed bytes at any
  alignment. Best match was 1 byte in 4, i.e. chance. A real clock-ratio error
  is deterministic and would have reproduced the garbage exactly.

## Two red herrings that cost time

- **Two null-modem adapters were stacked in the cable run.** Two crossings
  cancel: pins 2/3 end up straight-through, so both machines drive pin 2 and
  both listen on a pin nobody talks on. Symptom is dead in *both* directions
  while cable loopback still passes perfectly — **a loopback test is blind to
  pin 2/3 orientation.** Removing one adapter restored the receive direction.
- **`COPY SER: TO CON:` always fails** with `not copied: packet request type
  unknown`. Not a link fault: `COPY` issues an Examine packet to size its
  source, and `L:Port-Handler` implements only read/write. Use `TYPE SER:`.

## Next step: hardware

The receive/transmit asymmetry (corrupt-but-present in, nothing out) points at
the RS-232 line drivers rather than Paula or the CIA. Transmit goes through a
1488-class driver needing the ±12V rails; receive through a 1489 on +5V. A dead
−12V rail explains silent transmit exactly.

Self-loopback failing means *both* halves are implicated, which makes a shared
supply rail a better first suspect than two independently failed chips.
**Meter the rails at the connector before pulling any chips.**

Also still unconfirmed: whether that DB-25 is wired to Paula at all on this
particular PiStorm machine, and that the port in use is the serial one — on
Amiga hardware serial is the **male** DB-25, parallel is **female**.

## Reproducing the Linux side

```bash
stty -F /dev/ttyUSB0 19200 cs8 -cstopb -parenb -crtscts raw -echo
python3 -m serial.tools.miniterm /dev/ttyUSB0 19200   # Ctrl-] to exit

# passive capture (hex is what distinguishes a baud error from a wiring fault:
# right byte count + wrong values = framing; nothing at all = wiring)
cat /dev/ttyUSB0 > /tmp/serial-rx.bin
hexdump -C /tmp/serial-rx.bin
```

Reading the modem status lines is worth doing early, but note the caveat:

```bash
python3 -c "import serial; s=serial.Serial('/dev/ttyUSB0',19200,timeout=0); \
print('CTS',s.cts,'DSR',s.dsr,'CD',s.cd)"
```

Many null-modem cables loop RTS→CTS *locally at each connector*, so an asserted
CTS may just be your own RTS coming back. It is not evidence the far end is
alive.

## Amiga-side commands used

```
ASSIGN                     ; SER should appear under Devices
TYPE SER:                  ; read from serial (NOT `COPY SER: TO CON:`)
ECHO >SER: "hello"         ; write to serial
TYPE S:Startup-Sequence TO SER:   ; bulk write, harder to lose than one line
```

Set rate in **Prefs/Serial** (19200, 8N1, Handshaking **None**) and click
**Save**, not just Use. `AUX:` gives a full serial Shell if wanted — it needs a
`DEVS:DOSDrivers/AUX` mount entry pointing at `L:Aux-Handler`, then
`MOUNT AUX:` and `NEWSHELL AUX:`.
