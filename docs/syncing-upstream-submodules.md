# Syncing Upstream Submodules

> Evergreen procedure doc — edit in place.

Mark Fisher's `fujinet-nio` / `fujinet-nio-lib` move fast, upstream
squash-merges PRs (invalidating contributed branches), and this repo sometimes
has to pin a submodule to an in-flight PR branch. This procedure makes those
states explicit so no session has to re-derive them.

**Live example at time of writing:** `fujinet-nio-lib` is pinned to
`feature/amiga-rs232-pr` (1 commit), which has **diverged** from
`upstream/master` — upstream has since gained the app-store API (`445891c`)
that the on-hold appkey work is waiting for. The routine sync below is exactly
what resolves this.

---

## The three legal states of a submodule pointer

Every submodule in this repo is, at any moment, in one of these states. The
parent-repo bump commit message should say which.

1. **Tracking** — pinned to a commit on `upstream/master` (or the upstream
   default branch). The steady state. `fujinet-nio` and
   `apps/battleship/upstream` are normally here.
2. **Riding a PR** — pinned to a feature branch on your fork (`origin`)
   because the parent repo needs work that upstream hasn't merged yet.
   Legitimate but *temporary*; every routine sync should try to exit this
   state. `fujinet-nio-lib` is here now.
3. **Dirty** — local checkout differs from the recorded pointer. Never commit
   the parent in this state except as a deliberate bump.

## Routine sync (run when starting significant work, or ~weekly)

For each of `fujinet-nio` and `fujinet-nio-lib`:

```bash
git -C <sub> fetch upstream
```

**If Tracking:**
```bash
git -C <sub> checkout master
git -C <sub> merge --ff-only upstream/master
git add <sub> && git commit -m "bump <sub>: track upstream master (<short-sha>)"
# (on a parent feature branch, per CLAUDE.md — never directly on main)
```

**If Riding a PR** — first check whether the PR has been merged:
```bash
git -C <sub> log --oneline upstream/master | head   # look for the squash-merge
# or: gh pr view <n> --repo markjfisher/<sub>
```
- **Merged →** your branch's commits are now *invalidated duplicates* (squash
  rewrote them). Do NOT rebase or merge; drop the branch entirely:
  ```bash
  git -C <sub> checkout master
  git -C <sub> merge --ff-only upstream/master
  git -C <sub> branch -D feature/<name>
  git -C <sub> push origin --delete feature/<name>   # optional tidy-up
  git add <sub> && git commit -m "bump <sub>: PR #<n> merged; back to tracking upstream"
  ```
  Then run the **drift check** below — the squash may differ from what you
  contributed (review edits).
- **Not merged →** keep the branch current with a rebase (upstream requires
  linear history; never merge master into the branch):
  ```bash
  git -C <sub> rebase upstream/master
  # resolve, build, test-host / emu-test
  git -C <sub> push --force-with-lease origin feature/<name>
  git add <sub> && git commit -m "bump <sub>: rebase feature/<name> onto upstream/master"
  ```

## Drift check — does upstream's change break our layers?

Mark's changes land in two places that can silently affect this repo:

1. **`fujinet-nio-lib` API surface** → rebuild the compat layer immediately
   after any bump; the compiler is the completeness check:
   ```bash
   make -C fujinet-nio-lib TARGET=amiga
   make -C libs/fujinet-compat-amiga        # link errors = API drift
   make -C apps/battleship/amiga battleship
   ```
2. **`fujinet-nio` protocol behavior** → run one app's `emu-test`
   (`make -C apps/fn_test emu-test` is the cheapest) after bumping the server.

Separately, the compat layer's *headers* track `FujiNetWIFI/fujinet-lib` (not
Mark's repos) — that procedure already lives in
`updating-fujinet-compat-headers.md` and is triggered by fujinet-lib releases,
not by these bumps.

## Hygiene rules

- One bump per parent commit, message stating the new state:
  `bump fujinet-nio-lib: track upstream master (445891c)` — future sessions
  read state from `git log -- <sub>` instead of spelunking.
- Never point a submodule at a branch *name* mentally — the pointer is always
  a commit; record in the bump message which branch/PR it came from.
- After any bump, `git submodule status` in the parent must be clean before
  the parent commit.
- `apps/battleship/upstream` and `apps/pacmantests/amiga-pac-man` are
  read-only pins: fast-forward only when we deliberately want new upstream
  code, never as part of routine syncs (game-logic changes can break the
  Amiga platform layer mid-phase).
