# ESP32 Target — Decided Facts and Deferred Work

One page recording what is *settled* about the eventual ESP32 FujiNet for
Amiga, so future sessions neither re-derive it nor invent workflow for
hardware that doesn't exist yet. Everything not listed as decided is
**deferred until hardware exists**.

## Decided

- **The firmware is `fujinet-nio`.** The same codebase that runs on Linux
  today ships on the ESP32 (it already builds for both — see
  `main_esp32.cpp`/`main_posix.cpp` upstream). Its ESP-IDF build system is
  Mark's; we consume it via upstream PRs, never fork the build.
- **The Amiga side does not change.** Apps and the compat layer sit on
  `fujinet-nio-lib`, which is transport-agnostic; the Linux box and the ESP32
  are interchangeable behind the same FujiBus/SLIP serial framing
  (`contracts/fujibus-protocol.md`). No Amiga-side porting work is implied by
  the server moving to ESP32.
- **RS-232 is the bring-up path** and stays supported: 19200 baud through the
  Amiga's serial port per `contracts/rs232-hardware.md`. First ESP32
  bring-up runs over the same cable via a MAX3232-class level shifter.
- **Faster PHY candidates, in current order of likelihood:**
  1. **Parallel port** (most likely) — byte-wide, present on every Amiga
     model, no Zorro slot required (A500 compatible).
  2. **Zorro slot** — faster but excludes the A500 target without a
     sidecar/expansion path.
  The choice is *not yet made*; it determines a new transport in both
  `fujinet-nio` (channel) and `fujinet-nio-lib` (platform), each via
  upstream PR, plus a hardware contract like `rs232-hardware.md`.
- **Longer-term roles for the ESP32 unit** (goals, not designs): booting
  FujiNet CONFIG from an emulated floppy/hard drive served by the device,
  and using the nio console for monitoring/debugging.

## Deferred (do not write these docs yet)

- Flash/monitor workflow, partition sizing, ESP-IDF pinning — premature
  until a board is chosen; upstream's developer onboarding covers its own
  ESP32 build.
- Parallel-port electrical/timing spec — needs the PHY decision first.
- Any `emu/`-style automation for ESP32-in-the-loop testing.

When the PHY decision is made or hardware arrives, replace the relevant
"deferred" line with a pointer to the new contract rather than growing this
file.
