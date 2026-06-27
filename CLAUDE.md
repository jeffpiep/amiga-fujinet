# Amiga FujiNet — Claude Code Instructions

## Project Overview

Prototype FujiNet for Amiga using a Linux box as the FujiNet server connected
via RS-232 serial to an Amiga computer.

- `fujinet-nio/` — FujiNet server (runs on Linux/ESP32) — git submodule
- `fujinet-nio-lib/` — Client library (runs on Amiga) — git submodule
- `contracts/` — Protocol & hardware specs — source of truth for cross-submodule work
- `apps/` — Amiga programs (amiga-gcc / m68k-amigaos)

## Git Workflow

**Always follow this order:**
1. Before making any changes, ensure the working tree is clean (`git status`)
2. Create a feature branch in each affected submodule before editing it
3. Make changes and commit inside the submodule(s)
4. Update the parent repo's submodule pointer(s) with `git add <submodule-dir>`
5. Commit the parent repo

### Feature branch naming
```
feature/<short-description>
```

### Submodule commit workflow
```bash
# 1. Branch inside submodule
git -C fujinet-nio-lib checkout -b feature/my-change

# 2. Edit, then commit inside submodule
git -C fujinet-nio-lib add <files>
git -C fujinet-nio-lib commit -m "description"

# 3. Update parent pointer
git add fujinet-nio-lib
git commit -m "bump nio-lib: my-change"
```

### Keeping feature branches current (fetch → rebase)

Before starting new work or before pushing, sync each affected repo:

```bash
# 1. Fetch upstream
git -C fujinet-nio-lib fetch origin

# 2. Fast-forward local master
git -C fujinet-nio-lib checkout master
git -C fujinet-nio-lib merge --ff-only origin/master

# 3. Rebase feature branch onto master (keeps linear history)
git -C fujinet-nio-lib checkout feature/my-branch
git -C fujinet-nio-lib rebase master
```

Repeat for `fujinet-nio` and the parent repo as needed. Always rebase (not merge)
to keep the feature branch history linear.

### Never
- Edit submodule files without first creating a branch inside that submodule
- Commit the parent repo without first committing changes inside any modified submodule
- Use `git add .` or `git add -A` — always stage specific files
- Merge master into a feature branch — rebase instead

## Build Commands

```bash
# One-time: initialize submodules (--recursive required for esp_littlefs sub-submodules)
git submodule update --init --recursive --force

# Build fujinet-nio for Linux RS-232 (use build.sh — handles venv, cmake, ctest)
cd fujinet-nio
./build.sh -p fujibus-rs232-debug      # first build
./build.sh -cp fujibus-rs232-debug     # clean + build

# Build fujinet-nio-lib for Amiga
cd fujinet-nio-lib && make TARGET=amiga

# Build Amiga apps
cd apps/http_get && make
```

See `fujinet-nio/docs/developer_onboarding.md` for full build options, ESP32 setup,
available profiles (`./build.sh -p -S`), and CLI testing tools.

Serial port is configured at runtime via environment variables:
- `FN_SERIAL_PORT` (default: `/dev/ttyUSB0`)
- `FN_SERIAL_BAUD` (default: `19200`)

## Orchestration Pattern

The top-level Claude Code session coordinates work across submodules.

**Pattern:**
1. Top-level session writes or updates a contract in `contracts/`
2. Top-level session invokes a submodule session:
   ```bash
   cd fujinet-nio-lib
   claude --print "implement X per ../contracts/amiga-transport-api.md"
   ```
3. Submodule session reads the contract, reads existing code, implements, commits
4. Top-level session updates the submodule pointer in the parent repo

**Key rule:** `contracts/` is the handoff point. Never rely on passing large context
between sessions — put the spec in a contract file and reference it by path.

## Contracts

| File | Purpose |
|------|---------|
| `contracts/rs232-hardware.md` | Amiga RS-232 pinout, baud, cable wiring |
| `contracts/fujibus-protocol.md` | FujiBus + SLIP framing spec summary |
| `contracts/amiga-transport-api.md` | What to implement in `fujinet-nio-lib/src/platform/amiga/` |
| `contracts/amiga-coding-conventions.md` | C style, AmigaOS API rules, memory/stack constraints for Amiga code |

When adding new cross-submodule features, write the contract first.
