# Review of the Amiga Disk Device, from the Amiga Side

**Date:** 2026-09-26
**Answering:**
[`fujinet-nio/docs/update-to-jeff-amiga-disk-device-2026-08-10.md`][mupd],
"Requested review".
**Reviewed at:** `fujinet-nio-driver` `99ff749`, `fujinet-nio-lib` `dac8bf6`.
Line references below are to those revisions.

[mupd]: https://github.com/markjfisher/fujinet-nio/blob/master/docs/update-to-jeff-amiga-disk-device-2026-08-10.md

Sorry this took six weeks. It also means we are reviewing the driver as it
stands now, not as your 2026-08-10 note describes it. Writes, `CMD_UPDATE`,
catalogue mounts and the broker have all landed since. Where the two differ, we
reviewed the code.

Our angle is the one your harness does not cover: **Kickstart 1.3 on a 68000.**
Your default guest is `wb32` on an A1200/030; our floor is KS 1.3 on an A500
(+ PiStorm). Several findings below are 1.3-specific. We label each one either
*read from the code* or *belief, to be confirmed on a 1.3 boot*. We have not yet
booted your driver (see point 4), so none of them is confirmed on a live 1.3
machine.

## Update: first KS 1.3 boot of the broker (2026-09-26, later the same day)

Since writing the review below we have booted `fujinet-nio.device` on KS 1.3
in FS-UAE, and `http_get` has fetched a real HTTPS page through it. Getting
there took three fixes, and one of our beliefs turned out worse than we
wrote. The review text below is left as written; this section supersedes it
where they differ.

- **A — "No matching resident tag" (fixed, read and booted).** All three
  devices set `rt_EndSkip = &device_end`. In `nio.device` and
  `serial.device` that symbol is uninitialised, so it lands in `.bss`; in
  `disk.device` it is initialised, but GCC 6.5 placed it *before* the tag.
  The loader rightly requires `EndSkip` after the tag and inside the first
  hunk. Fix: `rt_EndSkip = (APTR)(&device_resident + 1)`.
- **B — device node never named (fixed, booted).** None of the three
  `device_init`s sets `ln_Name`, `ln_Type`, the version or the ID string,
  and each init table's data-table entry is `0`. KS 1.3's `InitResident`
  does not copy them from the ROMTag (later Kickstarts do, which is why your
  3.1 guest never saw it), so the device was added unnamed and `OpenDevice`
  returned "Device not found". Fix: set them in `device_init`.
- **The loader reports success on an unopenable device.**
  `fujinet-load-resident` printed "Resident loaded" throughout B, because it
  checks only `InitResident`'s return. A `FindName` on `DeviceList` after
  loading would have caught it.
- **C — the `serial.device` open from the worker Task: confirmed, and it
  crashes rather than fails.** Point 1's last belief predicted
  `IOERR_OPENFAIL`. What actually happens is Guru **#00000003** on the first
  exchange: ramlib loads the device and replies to the caller's
  `pr_MsgPort` at `task+92`, which a plain Task does not have, so `PutMsg`
  writes through garbage. We pinned it with Amiberry's IPC `READ_MEM`: the
  faulting task's `tc_UserData` was the broker base, and a return address
  on its stack sat directly after `backend_open`'s `OpenDevice` call
  (`fujinet_nio_serial_backend.c:597`). The workaround we predicted works:
  `C:fujinet-load-resident DEVS:serial.device serial.device` from the
  Startup-Sequence, before the broker is loaded. The durable fix is still
  yours: make the worker a Process, or do the first open in the caller's
  context in `OpenDevice`.
- **`TOOL_CRT` is no longer blocking** (point 4). `fujinet-load-resident`
  builds from our side with `-mcrt=nix13`, and runs on 1.3. We would still
  take `TOOL_CRT ?= -mcrt=clib2`, but it is a nice-to-have now.
- **The `GlobVec` belief (point 2)** was still unverified at this point.
  It is now settled; see the next update.

A and B are on `jeffpiep/fujinet-nio-driver`
`feature/resident-endskip-in-code-hunk` (`35d0469`, `d11d503`), with the
driver's host tests passing, and are sent as
[markjfisher/fujinet-nio-driver#1](https://github.com/markjfisher/fujinet-nio-driver/pull/1).
C is filed as [markjfisher/fujinet-nio-driver#2](https://github.com/markjfisher/fujinet-nio-driver/issues/2).

## Update: `DN0:` mounted on KS 1.3 — `GlobVec` and `Buffers` (2026-09-26)

We have now mounted `fujinet-disk.device` as `DN0:` on KS 1.3 (FS-UAE, A500,
WB 1.3.4 `C:Mount`), at the driver revision pinned above. The `GlobVec` belief
is **confirmed**. Half of the fix we suggested is **retracted**: 1.3's
FastFileSystem does not read OFS media.

**Setup.** Your `config/DN0` was reshaped into a single `#`-terminated entry
in `DEVS:MountList`. All of its keywords are accepted by 1.3 `Mount`,
including `DosType`, `StackSize` and `BufMemType`. The WB 1.3.4 MountList has
a `FAST:` example that uses all three. The boot loads the device with
`fujinet-load-resident` after the broker, mounts `host:/<image>` into unit 0,
then runs `Mount DN0:`, `Dir DN0: ALL` twice, `Type`, and `Info`. Test images
were 880 KB ADFs built with `xdftool`.

| MountList | Image | Result |
|-----------|-------|--------|
| 1. Yours as-is (`GlobVec = -1`, no `FileSystem`) | OFS | `Mount DN0:` returns cleanly. The first access (`Dir`) starts the ROM handler, which gurus **#00000003** before issuing a single sector read. The server log ends at the mount's Info and ClearChanged. |
| 2. `GlobVec` omitted | OFS | Works: `Dir`, `Type`, `Info` (`DN0: 880K Read Only`). |
| 3. `GlobVec = -1` + `FileSystem = L:FastFileSystem` | OFS | The handler starts and reads LBA 0. It finds `DOS\0` and puts up *"Not a DOS disk in unit 0"*. |
| 3b. As 3, with `DosType = 0x444F5301` | FFS | Works: `Dir`, `Type`, `Info`. |

So, for 1.3:

- **With no `FileSystem` line, `GlobVec = -1` is fatal.** Omitting it is
  the fix. We have not tried omitting it on 2.0+. We expect the ROM handler
  there to be fine either way, but your 3.1 guest is the place to check. If
  it isn't fine, 1.3 needs its own entry.
- **`FileSystem = L:FastFileSystem` is not the general answer we said it
  was.** The 1.3 FFS (L:FastFileSystem from WB 1.3.4) mounts `DOS\1` media
  only. It is correct only for images formatted FFS, with a matching
  `DosType`. The review's "FFS reads OFS media" holds for 2.0+ only.
- `GlobVec = -1` with FFS is correct on 1.3 (row 3b), as the stock 1.3
  MountList's `FAST:` example also has it.

**`Buffers`.** Measured with the working entry (row 2) at 19200 baud, on a
20-file OFS image. The timing is from `Date` before and after each `Dir`, so
it is rounded to whole seconds.

| `Buffers` | First `Dir DN0: ALL` | Second `Dir DN0: ALL` | Sector reads (both `Dir`s + `Type`) |
|-----------|----------------------|-----------------------|-----------------------------|
| 5 | 10 s | 9 s | 47 |
| 30 | 11 s | **2 s** | 26 |

That is about 0.35 s per block, as estimated. With 5 buffers the second
listing re-reads every header. With 30 it comes from cache. A working set
larger than the cache gains nothing: on a 40-file image, both 5 and 30
buffers re-read all 42 blocks on every `Dir`, at 14–16 s each. We would
still suggest 20–30 for the floppy-sized profile.

**Two more things this boot showed:**

- **`fujinet-disk.device` opens the broker safely from its worker Task.**
  Sector I/O ran through the resident broker with no Guru. The broker is
  already resident, so no ramlib load happens, unlike `serial.device` in #2.
- **We could not use `fujinet-mount` to mount media on 1.3.** It calls
  `CreateNewProcTags` (2.0+) for its boundary worker, so it will not build
  against a 1.3 C runtime. `FMOUNT` is not something we have. The driver also
  keeps its own per-unit mounted flag and never reads the server's restored
  runtime mounts. The server logged "Restored 1 runtime mount(s) as pending",
  yet the unit stays empty until a client sends `FUJINET_DISK_CMD_MOUNT`.
  (That is read from the code: `mounted` is set only in
  `fujinet_disk_mount`.) We used a 60-line Exec-only stand-in that sends
  `FUJINET_DISK_CMD_MOUNT` and `TD_CHANGESTATE`. A 1.3 user needs some
  equivalent. It could be `FMOUNT` if that avoids 2.0 calls, or a
  `fujinet-mount` with the boundary test compiled out.

## The five points

### 1. Native Exec and trackdisk command behaviour

The command set is right for what AmigaDOS actually sends, and the
`WRITE_MEDIA_POLICY.md` semantics are what we would have written. A few specific
things are done correctly and are easy to get wrong:

- `CMD_FLUSH` has its Exec queue meaning and never touches media.
- `TD_REMOVE` is synchronous and its request is never retained;
  `TD_ADDCHANGEINT` is retained and never replied until `TD_REMCHANGEINT`.
- ETD requests are checked against `iotd_Count`, and a sector-label buffer
  gets `IOERR_NOCMD`.
- `dg_BufMemType = MEMF_PUBLIC`. There is no DMA, so chip RAM would only waste
  a 512 KB machine's scarcest memory.

Findings, most important first. All are read from the code. The same patterns
appear in both `fujinet-disk.device` and the serial broker
(`nio.device/fujinet_nio_device.c`), so each fix applies twice.

1. **The worker `struct Task` is only partly initialised.** `device_init`
   (`disk.device/fujinet_disk_device.c:226-230`) fills in the stack and
   `tc_UserData`, then calls `AddTask`. It never sets `ln_Type = NT_TASK`,
   `ln_Name` or `ln_Pri`, and never calls `NewList(&tc_MemEntry)`. The device
   base is `MEMF_CLEAR`, so on stock hardware this works by accident: at
   expunge time, `RemTask` walks a list whose head is NULL, and it only
   survives because address 0 reads back as zero. Under Enforcer or an MMU
   that walk is reported as a hit. Tools that list tasks, and any
   `FindTask(name)`, will trip over the NULL name. This is the
   `amiga.lib CreateTask()` preamble; copying it verbatim fixes it. Give the
   task a name too: `fujinet-disk worker` in a task list is worth having.
2. **The worker's signal is allocated on the wrong task.** `AllocSignal(-1)` in
   `device_init` (`:219`) runs on whichever task called `InitResident`, i.e.
   `fujinet-load-resident`. It reserves a bit in the loader's `tc_SigAlloc`,
   not the worker's, and `FreeSignal` at expunge (`:404`) frees it on whatever
   task happens to be expunging. It works because the worker's own signal mask
   is empty and `Wait()` doesn't check. The clean fix is to use a fixed bit on
   the private task (for example `SIGBREAKF_CTRL_F`), or to have the worker
   allocate its own signal and hand the number back.
3. **`CloseDevice` edits two shared lists without locking.** `device_close`
   (`:266-277`) removes change registrations and takes requests off
   `io_queue`. Every other path that touches those lists does so under
   `Disable()`: `BeginIO`, `AbortIO` and the worker's `next_runnable_request`.
   Two tasks closing and issuing I/O on different units can therefore corrupt
   the queue. The worker's `TD_ADDCHANGEINT` path (`:736`), which does an
   `AddTail`, and `signal_media_change` (`:179`), which walks the list, are
   also unprotected against a concurrent `AbortIO` from the handler. Both
   windows are narrow, but the cost of closing them is one `Disable()` pair
   each.
4. **`CMD_RESET` is treated as `CMD_UPDATE`** (`:633`). In Exec, reset means
   "return the unit to its initial state, aborting pending I/O". Nothing we know
   of in AmigaDOS sends it to a trackdisk-style device, so this is low stakes,
   but it should at least abort the unit's queued requests the way `CMD_FLUSH`
   does. Whether it should also flush is your call; if it does, document it in
   `WRITE_MEDIA_POLICY.md`.
5. **`TD_GETDRIVETYPE` returns `DRIVE3_5` for HD media too** (`:693`).
   trackdisk returns `DRIVE3_5_150RPM` for an HD drive. Some format tools
   branch on it; FFS doesn't. Cheap to get right while `TD_GETGEOMETRY` is
   already branching on block count.
6. **Housekeeping.** `TD_REMOVE`, `TD_REMCHANGEINT` and `CMD_FLUSH` are fully
   handled in `BeginIO` (`:863-900`) and never reach the FIFO, so their cases in
   `device_process_request` are dead code. That code also has an implicit
   `CMD_FLUSH` fall-through into `TD_MOTOR` (`:657`). Harmless, but the next
   reader will wonder whether it is intended.

**The KS 1.3 one: which task opens `serial.device`.** *Belief, to be confirmed.*
On KS 1.3, `serial.device` is not in ROM; it is loaded from `DEVS:` on first
open. We carry the WB 1.3.4 copy on every ADF for exactly this reason. The
broker opens it from its worker, which is a plain `AddTask` Task and not a
Process (`nio.device/fujinet_nio_serial_backend.c:597`). As we understand
1.x, loading a device from disk needs a Process context, so the first
`OpenDevice` would fail with `IOERR_OPENFAIL` rather than load. *(Confirmed
on 1.3, and worse: it gurus. See the update at the top.)* The workaround needs no code
change: have something that is a Process open `serial.device` first, such as a
`Startup-Sequence` line. It stays resident until a memory flush expunges it,
and the failure would come back after one. If we confirm this, the durable fix
is for the broker to do its first open from the caller's Process context in
`OpenDevice`. We will report back once we have booted it.

(The `dos.library` v37 and `CreateNewProcTags` use in the broker is confined to
`FUJINET_NIO_DIRECTORY_BACKEND`, the native-test build. The serial broker
itself stays within the 1.3 API, which we checked because it would have been a
hard stop for us.)

### 2. The `DN0` MountList geometry

`Surfaces 2`, `BlocksPerTrack 11`, `LowCyl 0`, `HighCyl 79`, `Reserved 2` are
exactly the DF0 geometry and agree with `TD_GETGEOMETRY`. `DN0HD` at 22
sectors per track likewise. No change needed to the geometry itself.

Two changes to the surrounding fields:

- **`GlobVec = -1` on KS 1.3.** *Belief, to be confirmed.* With no `FileSystem`
  line, 1.3's `Mount` gives `DN0:` the ROM file system, which on 1.3 is the
  **BCPL** OFS handler. `GlobVec = -1` means "no BCPL global vector", which is
  what FastFileSystem (C/asm) wants. As far as we know, the 1.3 BCPL handler
  needs the default instead. On 2.0+ the ROM handler is FFS for both dostypes,
  which is why `-1` is correct in your guest. If we are right, the 1.3 fix is
  either to omit `GlobVec`, or to add `FileSystem = L:FastFileSystem` and keep
  it. The second is the better answer for 1.3 users anyway, since FFS reads OFS
  media.
- **`Buffers = 5` is the floppy default and too low for this link.** At
  19200 baud a 512-byte block costs roughly a third of a second, so every
  re-read of a directory or bitmap block that fell out of a five-buffer cache is
  paid for in full. 20–30 buffers is about 10–16 KB and should cut `Dir`
  times noticeably. The same reasoning applies more weakly at 38400. We will
  measure it rather than guess once we have it booted.

`StackSize = 32768` is generous, but harmless on anything with more than
512 KB.

### 3. Semaphore serialisation versus an internal unit task

This question has moved on since your note. The code now has **a worker task
per device**: `fujinet-disk.device` adds one in `device_init`, and the broker
has its own. Requests queue into a device-owned FIFO, and `BeginIO` returns
immediately except for locally answerable status commands. We think the move
was the right one. A filesystem handler calls `BeginIO` on its own task and
stack, so a design that did serial I/O inside `BeginIO` would put a potentially
multi-second transfer on the handler's stack. It would also block every other
DOS packet to that volume for the duration.

Two follow-ups:

- **`amiga/README.md` still says "No permanent worker task is required."** It
  is now wrong, and it is the first doc a reviewer reads.
- **Worker priority is 0 (the zeroed `ln_Pri`).** The handler it serves runs at
  `Priority = 5` from the MountList. A CPU-bound priority-0 task then
  time-slices with the I/O worker. Matching the handler's priority, or a fixed
  small positive value, would be more predictable. (It also goes away with
  finding 1 above, once `ln_Pri` is set on purpose.)

### 4. Running the driver in our emulator and physical harnesses

This is the one you most wanted, and the honest answer is: **not yet, on
either.**

- **Emulator.** Our FS-UAE harness (`emu/run.sh`, `emu/drive.sh`) boots KS 1.3
  from an ADF, bridges serial to `fujinet-nio` through a socat PTY pair, types
  keys via XTEST, and asserts on screenshots. Its ADF recipe predates the broker
  and ships only `Devs/serial.device`. So since the September sync, *none* of
  our apps can reach FujiNet from it, and neither could your driver. Reworking
  that recipe to install `fujinet-nio.device`, `fujinet-disk.device`,
  `fujinet-load-resident` and the `Startup-Sequence` lines is our next piece of
  work. Driver runs on KS 1.3 follow directly from it, and so do the 1.3 checks
  above.
- **The clib2 tool link blocks that recipe.** *(No longer: see the update
  at the top.)* The four tools under
  `amiga/tools/` hardcode `-mcrt=clib2`, and the stock amiga-gcc install we
  use ships libnix only. That includes `fujinet-load-resident`, which the
  broker install now requires. Dropping the flag builds
  `fujinet-load-resident`, `fujinet-unload-resident` and `fujinet-td-probe`
  cleanly against libnix. **Would you take `TOOL_CRT ?= -mcrt=clib2`** in
  `amiga/Makefile`? It keeps your default and lets us pass
  `TOOL_CRT=`. We can send the PR.
- **Physical.** Our A500's serial link is down at the Amiga end: it fails
  loopback on its own DB-25 with cable, adapter and host all disconnected. We
  have proven everything else good. No physical result until that is repaired.
- **What we will not do** is adopt Amiberry as a replacement. You asked for an
  independent harness, and FS-UAE on 1.3 is more useful to you as a second
  opinion than as a copy of yours. We are looking at an Amiberry mode
  *alongside* it, specifically for the IPC debugger (`READ_MEM`,
  `SET_BREAKPOINT`), because that is what makes broker bugs debuggable.

### 5. Larger raw-block geometry for the combined floppy/hard-disk direction

Having read `zorro-autoboot.md`, we agree with its split. `DNx` stays a
floppy-image device, and hard disks become a separate `fujinet-hd.device` with
RDB-partitioned HDFs. That reverses our earlier "raw blocks, MountList supplies
geometry, RDB only when autoboot needs it", and for good reason: autoboot does
need it. Once it does, a second RDB-less large-disk convention is only a
second thing to maintain.

What we would still want:

- **`TD_GETGEOMETRY` derived from the medium, not hardcoded.** Today it is
  `dg_Cylinders = 80` with an 11/22 branch (`:709-718`). For `fujinet-hd.device`,
  take geometry from Info (or from the RDB's `rdb_Cylinders` / `rdb_Heads` /
  `rdb_Sectors` when present). Check that it agrees with the partition's
  `DosEnvec`, because HDToolBox and FFS both compare the two.
- **For an RDB-less HDF, the UAE convention: 32 sectors per track, 1 surface,
  cylinders equal to blocks ÷ 32.** It is what every emulator user's existing
  HDFs assume. It also keeps `DosEnvec` arithmetic simple, and it covers the
  "just mount this image" case without an RDB tool.
- **KS 1.3 limits worth writing down now:** there is no 64-bit TD64/NSD in 1.3,
  so a unit tops out at 4 GB of byte offsets. FFS has to be loaded from the
  RDB's `FileSysHeaderBlock` chain or from `L:`, because the 1.3 ROM has only
  OFS. And `zorro-autoboot.md` is right that pre-V36 BootNode registration
  (`Enqueue` onto `eb_MountList` under `Forbid`) is its own code path. We
  would put it in the first milestone rather than a later one: most of the
  A500/A2000 machines a Zorro II card targets still run 1.3.

## Summary of asks

1. Initialise the worker `struct Task` fully (finding 1.1), in both devices.
2. Move or fix the worker signal allocation (1.2).
3. Put `device_close`'s list edits and the worker's change-list edits under
   `Disable()` (1.3).
4. Take `TOOL_CRT ?= -mcrt=clib2`. We can send the PR.
5. Correct `amiga/README.md`'s "no permanent worker task".

From us: the ADF rework, then KS 1.3 boots of both devices to confirm or
retract the `serial.device` open and `GlobVec` beliefs. We will report the
results in this doc's successor.
