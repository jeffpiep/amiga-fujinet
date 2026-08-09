# Answers to Mark's Driver/Disk Questions

**Date:** 2026-08-09
**Answering:**
[`fujinet-nio/docs/response-to-jeff-amiga-disk-device-plan.md`][mresp],
"Items needing explicit agreement".
**Also read:** [`driver_architecture.md`][march] and
[`amiga-floppy-channel.md`][mfloppy].

[mresp]: https://github.com/markjfisher/fujinet-nio/blob/master/docs/response-to-jeff-amiga-disk-device-plan.md
[march]: https://github.com/markjfisher/fujinet-nio/blob/master/docs/driver_architecture.md
[mfloppy]: https://github.com/markjfisher/fujinet-nio/blob/master/docs/amiga-floppy-channel.md

We are aligned on the architecture. NIO serves numbered fixed-size blocks and
a geometry; AmigaDOS owns OFS/FFS, directories, boot blocks and MountList; the
driver sits above an abstract channel so the PHY can change underneath it.
Nothing in your response needs arguing with.

## The seven points

**1. Is `fujinet-nio-driver` the home for the Amiga device, with MS-DOS moved
underneath?** Yes. The shared `common/{fujibus,disk_protocol,channel}` is
worth more to us than the faster local iteration we would get by prototyping
in our own `libs/`. It becomes a submodule on our side once it exists.

**2. Typed DiskDevice codecs in `fujinet-nio-lib` or the driver's `common/`?**
Your call — we consume either. Slight preference for `fujinet-nio-lib`, since
that is where our Amiga transport already went, and it keeps `fnctl`-style
tools from hand-rolling payloads.

**3. Is RS-232 acceptable as a correctness path?** Yes, and we want it kept.
We have the hardware and a full emulator harness wired for it. It only needs
to stay out of the driver API — no throughput assumption baked into the
interface.

**4. Is the first acceptance profile a standard 880 KiB / 1760-block ADF?**
Yes.

**5. Explicit unmount/remount for phase one, hot swap deferred?** Yes. Hot
swap needs joint design across NIO mount state, driver notification and
AmigaDOS cache invalidation, and it is not worth blocking on.

**6. Read-only first, or complete-block writes from the start?** Read-only
first. It sidesteps cache coherency, flush policy and media-change risk, and
gets `Mount DN0:` plus `Dir`/`Type` working sooner. When writes land they come
with a defined flush and failure policy rather than an assumption of
durability.

**7. Retain SLIP for the fast channel, or native packet framing?** Native
packet framing. Agreed with your stated direction — SLIP earns its place on a
byte stream; a floppy/Pico or Zorro link brings its own boundaries and
integrity, and wrapping FujiBus in SLIP there is pure overhead.

## What we are not ready to answer

We are still in the trade-study and requirements stage on the Amiga hardware
side, so please read anything below as direction rather than commitment. Our
own docs are pinned accordingly.

**We read PaulaNET's readme, and it is not what your note assumes.** It is a
floppy *emulator* first, with networking layered on top: the Pico presents as
a standard drive (DF1:), tracks 0–74 are a real AmigaDOS disk the ROM mounts
with no custom code, and networking rides tracks 75–77 through the stock
`trackdisk.device` via `ETD_RAWREAD`/`ETD_RAWWRITE`. A SanaII device sits
above that. They did not replace the Amiga's transport — they rode it.

Three things worth knowing before anyone plans around it:

- The ≈43 KB/s figure is the *network* number and depends on their RLE scheme,
  whose real purpose is producing a bitstream Paula can always lock onto. With
  it disabled the data is MFM-encoded and the rate halves. It is probably not
  the disk-serving number.
- The licence is `Copyright © 2026 RobSmithDev. All rights reserved.` — no
  open licence at all. Approach-level reading only; no code, no Gerbers.
- Track budget is real: they gave AmigaDOS only 75 tracks (~825 KB) to free
  tracks for the control channel. A design serving a full 1760-block ADF *and*
  wanting out-of-band control tracks has to give something up.

**Direction we are leaning, not a decision: FujiNet as both a floppy and a
hard disk.** An emulated drive supplies boot — the ROM reads track 0 with no
software on the machine — and `fujinet-disk.device` supplies capacity, with
the emulated floppy's Startup-Sequence mounting the larger volume. That is the
standard pre-RDB pattern, and it is also how PaulaNET gets its own driver onto
a machine. If it holds up, your driver work is unaffected: it is the hard-disk
half, and the floppy emulation does not displace it.

One constraint that falls out and is worth recording either way: autoconfig
autoboot is an expansion-bus mechanism (A500 side slot, Zorro), so a device on
the **floppy connector can never autoboot** however good the channel is. The
floppy-boots-then-mounts chain reaches the same result with no ROM.

## What we would like from you next

- **The channel/session interface** is the last thing needing joint agreement
  before implementation, more than anything else on this list. In particular:
  whether a legacy SLIP byte stream and a packet-native link present the same
  signature, and how capabilities — max transfer size, outstanding requests —
  get reported. Good candidate for a contract on our side.
- **A view on HD geometry**, when you get to it. Your response declines to
  infer `.hdf`/RDB semantics in phase one and we agree for phase one, but if
  the dual FD/HD direction holds we will want a larger profile. Our
  inclination is raw blocks with the MountList supplying geometry, and RDB
  only when autoboot makes it necessary.
- **A note on driver lifecycle.** You flagged that a resident driver should
  not lean on an application-oriented global transport, and you are right —
  ours assumes a process that exits. Whether that becomes an adaptation of the
  existing transport or a separate session layer is undecided on our side.

## References

- `docs/plan-amiga-disk-device.md` — our disk-device plan. The answers above
  close its Q1, Q4, Q5 and Q6; the updated version, plus a floppy-channel
  trade study carrying the PaulaNET findings and the FD/HD question, is on
  the unmerged `feature/floppy-planning` branch while we finish the
  requirements work. This doc is self-contained and does not depend on it.
