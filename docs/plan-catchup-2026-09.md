# Catch-up plan — September 2026

**Status: In progress (2026-09-26).** Item 3 (sync) **done** — all five
submodules at upstream head, everything builds, results below. Item 1
**answered** in `docs/response-to-mark-disk-device-review.md`; its 1.3 beliefs
(`serial.device` opened from a Task, `GlobVec = -1`) await a boot. Item 2 not
started. The ADF broker rework that item 3 uncovered is outstanding, wants its
own PR, and is now the next step: it gates item 1's follow-up boots.

Six weeks passed with no work on this repo while Mark pushed hard on his side.
This snapshot records what changed upstream, what it breaks here, and the three
things to do about it, in order. It exists so the work survives a `/clear` or a
new session — nothing below should have to be re-derived from a diff.

Work order agreed 2026-09-26: **3 → 1 → 2.**

---

## Upstream drift at the time of writing

| Submodule | New commits | Range |
|---|---|---|
| `fujinet-nio` | 164 | `d676161..a4847de` |
| `fujinet-nio-driver` | 48 | `6f51335..99ff749` |
| `fujinet-nio-lib` | 13 | `e75c3c6..dac8bf6` |
| `apps/battleship/upstream` | 17 | `e44f4b8..9de5ef6` |
| `apps/fujitzee/upstream` | 13 | `e2a9699..be77201` |

Note `apps/battleship/upstream` has only an `origin` remote (pointing at
`FujiNetWIFI/fujinet-battleship`), not the `origin`/`upstream` pair the other
submodules use — `git -C apps/battleship/upstream fetch upstream` fails with a
misleading "repository does not exist". Fetch `origin` for that one.

### The three changes that matter to us

**1. The Amiga transport moved behind a resident broker device. This is
breaking.**

`fujinet-nio-lib/src/platform/amiga/fn_transport.c` was rewritten in `6e0b3d9`
("Cut Amiga fn_transport over to the fujinet-nio.device broker"). It no longer
opens `serial.device` or `timer.device`; it `OpenDevice`s
**`fujinet-nio.device`**, a resident Exec broker that lives in
`fujinet-nio-driver/amiga/nio.device/`. SLIP framing moved into the broker.
Upstream's README now reads "Amiga applications → `fujinet-nio.device` broker".

Two consequences:

- nio-lib's Amiga targets now compile with `-I../fujinet-nio-driver/amiga/include`
  (hardcoded as `AMIGA_NIO_DEVICE_INCLUDE` in `makefiles/targets.mk`), for
  `fujinet_nio_device.h` — the broker ABI. So `fujinet-nio-driver` **must** be a
  sibling of `fujinet-nio-lib`. Our layout already satisfies this; that was luck,
  not planning.
- Every ADF in `apps/` currently ships only `Devs/serial.device`. They now also
  need the broker device on disk plus `fujinet-load-resident` and a
  `S:Startup-Sequence` line. **Nothing in `apps/` will talk to FujiNet until
  that is done.** Affects `contracts/amiga-adf-bootstrap.md` and every app
  Makefile's ADF rule.

Mark's own disk builder already does this: "The workspace disk builder's
`--with-driver` path installs both files and adds this command to
`S:Startup-Sequence` automatically."

**2. He solved the high-baud dropped-byte problem by writing his own UART
driver.**

`fujinet-serial.device` — a Paula UART driver with `misc.resource` ownership,
8N1 only, exclusive open, an RX-buffer-full interrupt copying `SERDATR` into a
ring, and polled `TBE` on transmit. Selected at runtime:

```text
C:fujinet-load-resident DEVS:fujinet-serial.device fujinet-serial.device
C:fujinet-nio-serial fujinet-serial.device
C:fujinet-nio-baud 38400
```

Baud range 300–230400. The selection is held by the resident broker until
unload or reboot; put it in `S:Startup-Sequence` to persist. Stock
`serial.device` remains the default — do not rename it, select the FujiNet one
instead. Affects only the RS-232 byte-stream backend.

nio-lib carries a matching change: it will accept a SLIP frame **missing its
leading `C0`**, to tolerate a dropped byte at higher speeds (his commit message:
"hack or genius?").

New hardware reference docs in `fujinet-nio-driver/docs/amiga/`:
`rs232-38400-pacing-evidence.md`, `rs232-cold-warm-hardware-test.md`,
`Serial-IO-Interface.md`, `cia-chip-register-map.md`,
`cia-port-signal-assigments.md`, `serial-open-parameters-regression.md`, plus
AHRM v3 serial pages as PNGs.

Two warnings from `rs232-cold-warm-hardware-test.md` worth not rediscovering:

- **Do not run `C:fujinet-nio-exchange` with no arguments on PiStorm or real
  hardware.** That is the Amiberry isolation suite — it sends a malformed packet
  to force a timeout, then `CreateNewProc`s two extra processes for concurrent
  clock commands. On PiStorm that has completed with `PASS` and then rebooted the
  machine (power LED flash, no Guru).
- "PiStorm is the sole hardware-stability gate… Amiberry does not prove that."

**3. The disk device's blocker has an answer, and it is not the parallel port.**

`fujinet-nio` now contains `bridges/rp2350-zorro/` — an RP2350-based Zorro
bridge, with a reviewed L0–L10 SPI link feasibility ledger
(`docs/link-feasibility-evidence.md`) and a draft Story 2.4 packet ABI
(`docs/bridge-packet-abi.md`). Headline numbers: ~105 KB/s at the RP2350's
actual 6.818 MHz divider rate for 240-byte payloads; intermittent returned-frame
checksum failures at 7.5 MHz on a Dupont/breadboard fixture; polling and DMA
indistinguishable, so SPI FIFO servicing is not the bottleneck. Treat ≤6.818 MHz
as the demonstrated rate on that fixture — explicitly *not* a production limit.

Also new: `fujinet-nio/docs/amiga/zorro-autoboot.md` and
`fujinet-nio/docs/amiga/amiga-floppy-channel.md`. The latter overlaps the
unmerged planning work on our `feature/floppy-planning` branch and should be
read before that branch is revived. There is a separate public repo,
`markjfisher/fujinet-nio-hardware` ("Hardware projects for NIO"), that we do not
pin.

**`docs/strategic-plan.md`'s disk-device row is therefore stale** — it says
"blocked on parallel-port PHY". The PHY answer is now Zorro/RP2350.

---

## Item 3 — Sync and prove it builds

Bump all five submodules to current upstream, then build everything and report
precisely what the broker change breaks. **Diagnose, do not yet rewire the
ADFs** — that is its own piece of work and wants its own PR.

Sequence per `docs/syncing-upstream-submodules.md` (all five were *Tracking*
before this, so it is the fast-forward path):

```bash
git -C <sub> checkout master && git -C <sub> merge --ff-only upstream/master
```

Then, in order:

```bash
cd fujinet-nio && ./build.sh -cp fujibus-rs232-debug   # T3
make -C fujinet-nio-lib amiga                          # needs the driver sibling
make -C fujinet-nio-lib check                          # nio-lib's own host suite
export PATH=/opt/amiga/bin:$PATH
make -C fujinet-nio-driver amiga                       # fujinet-disk.device + broker
make test-host                                         # T1, repo-wide
make -C apps                                           # all Amiga apps
```

Expect `make -C apps` to compile (the transport is behind the nio-lib archive)
but every ADF to fail at runtime for want of the broker. Confirm that rather
than assuming it.

### Results (2026-09-26)

All five fast-forwarded cleanly. Verified green:

| Check | Result |
|---|---|
| `make test-host` (T1, repo-wide) | PASS — joydecode, keytrans, sndgen, gkclock, gkrandom, cellmap, mousemap, wireformat, fjlayout |
| `make -C fujinet-nio-lib amiga` | PASS — picks up `-I../fujinet-nio-driver/amiga/include` as designed |
| `make -C fujinet-nio-lib test-amiga-transport` | PASS — the *new* broker-client host test |
| nio-lib `test-library-link`, `test-session`, `test-disk`, `test-disk-context` | PASS |
| `fujinet-disk.device` / `fujinet-nio.device` / `fujinet-serial.device` | all link — 24 KB / 13 KB / 7 KB |
| `make -C apps` | PASS — every app builds |
| `fujinet-nio` T3 (`./build.sh -cp fujibus-rs232-debug`) | PASS — 383 doctest cases, 12,949 assertions, 3/3 ctest |

Two findings worth carrying forward:

**`make -C fujinet-nio-lib check` cannot run here** — it needs `CC65_HOME` for
the Atari/BBC targets and we have no cc65. Environment blocker, not a defect;
the Amiga-relevant subset above was run individually instead. (This is exactly
the case Mark's `docs/agent-test-policy.md` says to report rather than skip
silently or substitute a bigger suite.)

**The clib2 tool-link failure is now blocking, not cosmetic.** Installing the
broker requires `fujinet-load-resident`, which is one of the four tools that
hardcode `-mcrt=clib2` and so will not link against our libnix install.
Re-verified that dropping the flag builds `fujinet-load-resident`,
`fujinet-unload-resident` and `fujinet-td-probe` cleanly. **Filing the
`TOOL_CRT ?= -mcrt=clib2` fix upstream is now on the critical path** — fold it
into item 1's reply to Mark rather than sending it separately.

**Everything compiling is the trap here.** The broker cutover is behind the
nio-lib archive, so `apps/` builds and T1 passes while no ADF can actually reach
FujiNet. Only T2 would catch it. Recorded in the strategic plan's Lessons
Learned.

Known toolchain issue, unchanged and still upstream's: the four tools under
`fujinet-nio-driver/amiga/tools/` hardcode `-mcrt=clib2`, which our amiga-gcc
does not ship (we have libnix). `fujinet-disk.device` itself links fine because
it uses `-nostartfiles`. See CLAUDE.md; the fix belongs upstream as an
overridable `TOOL_CRT ?=`.

---

## Item 1 — Answer Mark's review request

**Source:** `fujinet-nio/docs/update-to-jeff-amiga-disk-device-2026-08-10.md`
(103 lines, "Draft for discussion", dated 2026-08-10). It is his reply to our
`docs/response-to-mark-driver-questions.md`, which shipped in PR #41 on
2026-08-09. **It has never been answered.** Background, now archived on his
side: `fujinet-nio/docs/archive/response-to-jeff-amiga-disk-device-plan.md`.

Most of the doc's body is overtaken by the six weeks since — it describes a
read-only path with writes "deliberately deferred", and writes are now live. Do
not respond to the body as though it were current. The part that still stands is
the closing section.

### The five things he asks for, verbatim

> Review from the Amiga side would be particularly useful for:
>
> - the native Exec and trackdisk command behavior;
> - the `DN0` MountList geometry;
> - synchronous semaphore serialization versus a future internal unit task;
> - running the driver in Jeff's emulator and physical RS-232 harnesses; and
> - the preferred larger raw-block geometry when the combined floppy/hard-disk
>   direction is ready to progress.

### What we know that bears on each

- **Exec / trackdisk behavior** — read `fujinet-nio-driver/amiga/README.md`
  first; it now documents idle vs. busy Expunge (`LIBF_DELEXP`), the
  `fujinet-unload-resident` tool and its three outcomes, and the private
  `FUJINET_DISK_CMD_MOUNT` / `..._MOUNT_WRITABLE` commands outside the trackdisk
  range. Our KS 1.3 floor is the angle he cannot check: his default guest is
  `wb32` on `a1200-030`, and his loader is validated on Workbench 3.1.
- **`DN0` MountList geometry** — his current profile is standard 880 KiB raw ADF,
  512-byte sectors, 1760 blocks, 80 cylinders / 2 heads / 11 sectors. Amiga unit
  0 ↔ DiskDevice slot 1 (and generally slot *n* ↔ unit *n−1*).
- **Semaphore vs. internal unit task** — he uses an Exec `SignalSemaphore` to
  serialize callers sharing one physical RS-232 session, and says an internal
  unit task plus a bounded async queue is deliberately not in the read-only
  implementation. Since then `7baa83e` made `CMD_WRITE` an async Exec request, so
  check whether this question has already moved.
- **Our harnesses** — this is the one he most wants and the one item 2 bears on.
  He is *not* asking us to adopt Amiberry; he wants a second, independent
  harness. Our honest answer today is that FS-UAE + `emu/drive.sh` cannot
  currently exercise the driver at all (no broker on our ADFs), and that we have
  no physical RS-232 harness result since the serial debug session recorded in
  `4085d3d`.
- **Larger raw-block geometry** — tied to the combined floppy/hard-disk
  direction. Read his `fujinet-nio/docs/amiga/amiga-floppy-channel.md` and
  `zorro-autoboot.md` (which plans `fujinet-hd.device`, host-backed HDF, RDB
  partition discovery) alongside our `feature/floppy-planning` branch before
  answering.

### How to deliver the answer

Follow the precedent: a doc in our `docs/`, like
`docs/response-to-mark-driver-questions.md`, referenced to him by path. Memory
also records a Discord reply to Mark drafted but never sent — check whether that
is still relevant or now superseded.

---

## Item 2 — Amiberry spike

Mark tests on **Amiberry**, not FS-UAE, and the harness is public:
**`markjfisher/fujinet-nio-workspace`** (pushed 2026-09-22). It is a
15-submodule superproject — the same shape as `amiga-fujinet`, except his holds
the emulator tooling. He drives it with BMAD (~40 `bmad-*` skills under
`.agents/skills/`) from ChatGPT/Codex; there is a `.claude/` directory too.

Scale: `tools/amiga_emulator/` is 4,011 lines of Python;
`integration-tests/amiberry/` is 2,765 lines across 22 pytest modules;
`tests.toml` declares **47 cases** with 53 startup sequences;
`docs/amiga/` is ~2,600 lines across 9 docs.

### Why it is more agent-friendly — the actual reason

**Amiberry has a Unix-domain-socket IPC control interface, and it is upstream,
not a fork:** <https://github.com/BlitterStudio/amiberry/wiki/IPC-Socket-support>.
Tab-separated commands on `$XDG_RUNTIME_DIR/amiberry.sock` (Amiberry appends an
instance suffix when that is occupied). Client:
`tools/amiga_emulator/ipc.py`.

Commands his harness actually uses, by frequency:

| Command | Uses | Buys |
|---|---|---|
| `SET_BREAKPOINT` | 20 | breakpoint at an m68k address |
| `GET_CPU_REGS` | 17 | register dump at the hit |
| `DEBUG_CONTINUE` | 15 | resume |
| `DEBUG_ACTIVATE` | 11 | enter Amiberry's built-in debugger |
| `READ_MEM` | 4 | read emulated memory |
| `SEND_KEY` | 5 | **raw Amiga keycode** + up/down state |
| `SCREENSHOT` | 3 | framebuffer to a path, on demand |
| `SET_SERIAL` / `GET_SERIAL` | 2 | swap the guest serial driver at runtime |
| `SET_BAUD` / `GET_BAUD` | 2 | change baud with no restart |
| `SET_CPU_SPEED`, `PAUSE`, `RESUME`, `QUIT`, `PING` | — | lifecycle |

The debugger group is the whole point. `tools/amiga_emulator/beginio_trace.py`
opens with a table of link-time offsets into `fujinet-disk.device`
(`DEVICE_BEGIN_IO_LINK = 0x110E`, `REPLY_MSG_LINK = 0x13F8`, …), resolves the
live device vector to anchor a static function, sets breakpoints, and reads
IORequest structs out of guest memory from Python. Siblings:
`io_request_compare_capture.py`, `task_snapshot.py`, `write_buffer_capture.py`,
`dn2_handler_trace.py`, `read_path_capture.py`, `debug_snapshot.py`.

**Our `emu/drive.sh` injects X11 XTEST keysyms into Xvfb and greps a
screenshot.** No control socket, no liveness check, no memory reads, no
registers, no breakpoints; a baud change means editing a config and rebooting.
Ours can assert *the right pixels appeared*; his can assert *`BeginIO` was
entered with this IORequest and replied in this order*. For broker and
disk-device work that gap is decisive.

Two further asymmetries:

- **TCP serial works for him.** His NIO listens on TCP 65504 and Amiberry
  connects to it — no socat in the serial path. Our memory records FS-UAE's TCP
  `serial_port` mode breaking `serial.device` with `IOERR_OPENFAIL`, which is why
  we carry socat + PTY pairs. `fujinet-nio/docs/posix_tcp_serial_channel.md`
  covers the TCP channel; `amiga-floppy-channel.md` names "TCP/Amiberry backend"
  as a first-class transport.
- **Guest-image discipline.** Three explicit roles: a pristine reproducible BASE
  HDF, a disposable TEST copy into which each case injects exactly the binaries
  it needs, and a persistent developer Workbench that launchers never write.
  Plus a read-only host-directory share (`NIO:`) exposing fresh build artifacts
  into a running guest. Our ADFs are rebuilt wholesale every time.
- **Evidence retention.** Every run writes
  `test-evidence/amiberry-YYYYMMDD-HHMMSS/` with the generated HDF, component
  logs, extracted result files and an IPC framebuffer capture. Nice trick for
  cases with continuous background traffic: `completion_mode = "nio_marker"` —
  the guest emits a unique final NIO operation and the harness quits on that
  marker instead of burning the safety timeout.

### Worth reading before deciding

- `docs/amiga/amiberry-testing.md` (860 lines) — the two public paths,
  `amiga-workbench` (interactive) and `amiga-tests` (automated)
- `docs/amiga/amiberry-control-examples.md` (238) — launch, IPC, rawkey tables,
  the modifier hold/tap/release rule
- `tools/amiga_emulator/README.md` (566) — harness internals
- `docs/amiga/nio-broker-architecture.md` (811) — the broker, i.e. item 3's
  breaking change, from his side
- `docs/amiga/cli-stack-and-iorequest.md` (39) — OpenDevice-only `WaitIO`
  pitfalls, referenced from his hardware-test doc
- `docs/agent-test-policy.md` (76) — **steal this outright.** Product-wide rule:
  "cheapest sufficient gate," with a per-repo *usual* / *deeper* / **do not
  default to** table. The full Amiberry suite is ~6 min and he treats that as too
  expensive for an inner agent loop; the normal gate is one pytest node. He also
  requires the exact command be recorded in the spec's Verification section *and
  executed* — "Incomplete if the command was not run." Stricter than our
  `docs/testing.md`; maps cleanly onto our T1/T2/T3 tiers.

His 47 cases double as a map of what he has been debugging:
`diskdevice-{loader,adf,hd-adf,hd-stage8,fmount,fumount-handler,unload-reload,mapping-failure,silent-timeout,fmount-restore,dynamic-dd,dynamic-fmount-hd,inspect-catalog,stalled-external-peer,adf-native-floppy}`,
`diskdevice-inhibit-{poc,exp-a,exp-b}`, `inspect-causal-{a…o}` (fifteen),
`nio-{broker-isolated,native-test,paula-serial,native-exchange,native-disk,native-fault-hold,native-fault-drop,tool-parity}`,
`amiga-fin-{slot-catalog,ffs-adf}`, `checksumbench`, `wifi-config`,
`cli-stateful`. Fifteen `inspect-causal-*` cases is what a hard bug looks like
when you have a scriptable debugger.

### Scope for the spike

Do not port the harness. Decide one question: **does `emu/` gain an Amiberry
mode alongside FS-UAE?** The cheapest probe is to install Amiberry, confirm the
IPC socket appears, and drive `GET_STATUS` / `SEND_KEY` / `SCREENSHOT` against
one existing ADF. If that works, the follow-on prize is `READ_MEM` +
`SET_BREAKPOINT` against `fujinet-nio.device`, which is what would make the
broker migration debuggable.

Keep FS-UAE either way: `make emu-test` is our CI gate, and Mark explicitly
wants our harness to stay independent of his.

---

## Repo housekeeping debt

- ~~**PR #42**~~ — squash-merged to `dev` 2026-09-26; `fujinet-nio-driver` is
  now a submodule on `dev`.
- `feature/floppy-planning` — 5 unmerged planning commits, no PR. Read Mark's
  `amiga-floppy-channel.md` and `zorro-autoboot.md` before reviving it.
- Dead local branches to reap: `feature/niotools` (0 ahead of `dev`),
  `feature/bump-nio-lib-merged` (its work is in `dev` via squash).
- `apps/battleship/upstream` has no `upstream` remote — see the note above.
- Upstream repos we do not pin but probably should:
  `markjfisher/fujinet-nio-hardware` (the Zorro/RP2350 board),
  `markjfisher/nio-core-apps` and `markjfisher/nio-apps` (where his Amiga test
  and product binaries live — not in the driver repo).
