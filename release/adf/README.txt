Amiga FujiNet -- test disks
===========================

Three bootable floppies for a stock Amiga 500 (Kickstart 1.3 or later,
512 KB is enough). Write each .adf to a real floppy (Greaseweazle, ADF
Opus, etc.) or load it on a Gotek.

  fujinet-tools.adf  Start here. Boots, checks the link to FujiNet, then
                     leaves you at a CLI prompt.
  battleship.adf     FujiNet Battleship -- lobby + online play.
  fujitzee.adf       Fujitzee (dice game) -- lobby + online play.

None of these have been run on a real Amiga before. They have only been
run in FS-UAE. Any result is useful, including a Guru code or a hang.


What the Amiga talks to
-----------------------

The Amiga has no FujiNet hardware of its own yet. Its 25-pin serial port
connects to a Linux machine running the FujiNet server, fujinet-nio. That
machine provides the network access.

  Amiga DB25 serial port  <-- null-modem cable -->  USB-serial adapter on Linux

Link settings: 19200 baud, 8N1, no flow control.

Build and run the server on the Linux machine:

  git clone --recursive https://github.com/markjfisher/fujinet-nio
  cd fujinet-nio
  ./build.sh -p fujibus-rs232-debug
  FN_SERIAL_PORT=/dev/ttyUSB0 FN_SERIAL_BAUD=19200 \
      ./build/fujibus-rs232-debug/fujinet-nio

Change /dev/ttyUSB0 if your adapter has a different name. Leave the server
running, then boot the Amiga. Every packet the Amiga sends shows up in the
server log as "fujibus: receive:". If nothing appears there, the cable is
the problem, not the software.


1. fujinet-tools.adf -- check the link first
--------------------------------------------

Boot it. The disk runs fn_test by itself. A working link prints:

  fn_init() OK
  FujiNet is ready.
  fn_test: calling fn_clock_get()...
  FujiNet clock: <a number>

A number on the last line means a full round trip over serial worked.
You then land at a CLI prompt. Try an HTTPS fetch:

  http_get https://wttr.in/London?format=3

To run the link check again, type fn_test. The disk also has Dir, List,
Type, Info, Avail, CD, Copy, Ed, Execute, Wait and Version.


2. battleship.adf / fujitzee.adf
--------------------------------

Boot the disk and the game starts. You are asked for a player name, then
you pick a table from the lobby. The games reach the public FujiNet game
servers through the Linux machine, so that machine needs internet access.
A joystick in port 2 works, and so does the keyboard. Battleship also
takes the mouse for aiming.

The server stores your player name, not the floppy, so it is kept
between boots even though the disk is never written to.


If something goes wrong
-----------------------

  "Device not found"             The resident driver didn't load. Note
                                 anything printed during boot.
  Guru Meditation #00000003      Tell us. It is a known class of bug on
                                 1.3, and we would like the full number.
  fn_clock_get() failed / hangs  Serial link. Check the cable is a
                                 null-modem cable, the baud rate is
                                 19200, and the server is running.
  Nothing in the server log      Cable, adapter, or the wrong /dev/tty*.

Please send back: Kickstart version, RAM, any accelerator, and the last
few lines of the server log. A photo of the screen is fine.
