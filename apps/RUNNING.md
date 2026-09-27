# Running a release ADF against fujinet-nio

How to start the FujiNet server (`fujinet-nio`) on Linux and boot one of the
tester disks in `release/adf/`:

| Disk | What it does |
|------|--------------|
| `fujinet-tools.adf` | **Start here.** Runs `fn_test` (link check), then leaves you at a CLI with `http_get`. |
| `battleship.adf` | FujiNet Battleship: lobby and online play. |
| `fujitzee.adf` | Fujitzee (dice game): lobby and online play. |

There are two ways to run them. **A** uses the FS-UAE emulator and needs no
Amiga hardware. **B** uses a real Amiga on a serial cable. Both need the server
built (step 1).

---

## Step 1 — One-time setup (both paths)

From the repo root:

```bash
# Fetch all submodules (--recursive is required)
git submodule update --init --recursive --force

# Build the Linux server
cd fujinet-nio
./build.sh -p fujibus-rs232-debug
cd ..
```

The server binary is written to
`fujinet-nio/build/fujibus-rs232-debug/fujinet-nio`.

The ADFs in `release/adf/` are committed, so you don't need the Amiga
toolchain to run them. Run `make -C release` only if you want to rebuild them
from source.

## Step 2 — Pick a path

Who starts the server depends on the path. **Don't start it yourself yet.**

| Path | You have | Who starts `fujinet-nio` |
|------|----------|--------------------------|
| **A — Emulator** | No Amiga, no serial adapter | `emu/play.sh` starts it for you, on a virtual serial port. Don't start it yourself. |
| **B — Real Amiga** | An Amiga, a null-modem cable and a USB-serial adapter | You do, in step B3, pointed at the adapter. |

To go the emulator way, continue with Path A. For a real Amiga, skip to
Path B. The server's settings and log output are explained in
[About the server](#about-the-server) at the end.

---

## Path A — FS-UAE emulator (no hardware)

### A1. One-time emulator setup

```bash
sudo apt install fs-uae socat xvfb jq x11-utils
cp emu/config/paths.env.example emu/config/paths.env
```

Edit `emu/config/paths.env` and set `KICKSTART_ROM` to your **Kickstart 1.3**
ROM file. You must supply the ROM yourself; it is not in the repo.

### A2. Run it (one command)

From a desktop session (it opens a visible window), in the repo root:

```bash
APP_NAME=fujitzee ADF_PATH=release/adf/fujitzee.adf bash emu/play.sh
```

Change `APP_NAME` and `ADF_PATH` to boot a different disk:

```bash
APP_NAME=tools      ADF_PATH=release/adf/fujinet-tools.adf bash emu/play.sh
APP_NAME=battleship ADF_PATH=release/adf/battleship.adf    bash emu/play.sh
```

`emu/play.sh` does everything below for you:

1. Kills any leftover `fs-uae`, `socat` or `fujinet-nio`.
2. Creates a virtual null-modem cable with `socat`: two linked PTYs,
   `/tmp/amiga-serial` (emulator side) and `/tmp/fn-pty` (server side).
3. Copies the ADF into `~/Documents/FS-UAE/Floppies/` and boots it as an
   A500 in DF0:.
4. Starts `fujinet-nio` with `FN_SERIAL_PORT=/tmp/fn-pty FN_SERIAL_BAUD=19200`.

It prints a `tail -f` command for the server log. Run that in a second
terminal to watch traffic. To stop, quit FS-UAE or press Ctrl-C in the
first terminal. Logs are kept under `emu/logs/<APP_NAME>/play-<timestamp>/`.

### A3. Doing the same by hand (optional)

This is useful when you want the server in its own terminal:

```bash
# Terminal 1: virtual null-modem cable
socat -d -d pty,raw,echo=0,link=/tmp/amiga-serial pty,raw,echo=0,link=/tmp/fn-pty

# Terminal 2: the emulator, attached to one end
#   Copy the ADF where FS-UAE looks for floppies (it ignores absolute paths
#   in the config file)
cp release/adf/fujitzee.adf ~/Documents/FS-UAE/Floppies/
fs-uae --amiga_model=A500 --kickstart_file=/path/to/kick13.rom \
       --floppy_drive_0=fujitzee.adf \
       --serial_port="$(readlink /tmp/amiga-serial)"

# Terminal 3: the server, attached to the other end
cd fujinet-nio/build/fujibus-rs232-debug
FN_SERIAL_PORT=/tmp/fn-pty FN_SERIAL_BAUD=19200 ./run-fujinet-nio
```

Two emulator settings break the link:

- **Don't use FS-UAE's TCP serial mode.** It makes `serial.device` fail to
  open. Always use a PTY, as above.
- **Delete stale `.sdf` files** in `~/Documents/FS-UAE/Save States/` if an
  old version of a disk keeps booting. `play.sh` does this for you.

---

## Path B — Real Amiga

### B1. Hardware

```
Amiga DB25 serial port  <-- null-modem cable -->  USB-serial adapter on Linux
```

Link settings: 19200 baud, 8N1, no flow control. For pinout and cable
wiring, see `contracts/rs232-hardware.md`.

Any stock A500 with Kickstart 1.3 or later and 512 KB of RAM will do.

### B2. Put the ADF on a floppy

Write the `.adf` to a real disk (Greaseweazle, ADF Opus, …) or copy it to a
Gotek's USB stick.

### B3. Start the server, then boot

```bash
ls /dev/ttyUSB* /dev/ttyACM*        # find your adapter
cd fujinet-nio/build/fujibus-rs232-debug
FN_SERIAL_PORT=/dev/ttyUSB0 FN_SERIAL_BAUD=19200 ./run-fujinet-nio
```

If you get "permission denied" on the port, add yourself to the `dialout`
group (`sudo usermod -aG dialout $USER`), then log out and back in.

Replace `/dev/ttyUSB0` with your adapter's name. Leave the server running
and power on the Amiga with the disk in DF0:. `fujibus: receive:` lines in
the server log mean the Amiga is getting through.

---

## What you should see

**`fujinet-tools.adf`** runs the link check by itself:

```
fn_init() OK
FujiNet is ready.
fn_test: calling fn_clock_get()...
FujiNet clock: <a number>
```

A number on the last line means a full round trip worked. At the CLI prompt,
try:

```
http_get https://wttr.in/London?format=3
```

Type `fn_test` to run the link check again.

**`battleship.adf` / `fujitzee.adf`** start the game straight away. Enter a
player name, then pick a table from the lobby. You can use a joystick in
port 2 or the keyboard. Battleship also takes the mouse. The server stores
your player name, so it survives reboots.

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| No `fujibus: receive:` lines in the server log | Cable, adapter, or wrong `FN_SERIAL_PORT`. In the emulator: the PTY wiring. |
| `Device not found` at boot | The resident broker (`fujinet-nio.device`) didn't load. Note what boot printed. |
| Guru Meditation `#00000003` | Known class of KS 1.3 bug. Record the full number. |
| `fn_clock_get()` fails or hangs | Serial link: null-modem cable? 19200 on both ends? Server running? |
| An old version of the disk boots (emulator) | Stale `.sdf` save file; see A3. |
| Game lobby is empty or fails | The Linux machine has no internet access. |

Tester-facing notes, including what to report back, are in
`release/README.txt`.

## About the server

You only need this for Path B, or for the by-hand version of Path A (A3).

Two environment variables configure the server:

| Variable | Meaning | Default |
|----------|---------|---------|
| `FN_SERIAL_PORT` | Serial device to talk to the Amiga on | `/dev/ttyUSB0` |
| `FN_SERIAL_BAUD` | Baud rate. **Must be 19200** for the release disks | `19200` |

Start it **from its build directory**,
`fujinet-nio/build/fujibus-rs232-debug`. It looks for its `fujinet-data/`
folder relative to the current directory.

`run-fujinet-nio` is a small wrapper that restarts the server when it asks
to be restarted. Running `./fujinet-nio` directly also works.

Every packet from the Amiga is logged as `fujibus: receive:`. If no such
lines appear after the Amiga boots, the problem is the link, not the
software.

The games (and `http_get`) reach the internet through this Linux machine,
so it needs network access.
