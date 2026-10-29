# Syncing Upstream Submodules

> Evergreen procedure doc — edit in place.

Mark Fisher's `fujinet-nio` / `fujinet-nio-lib` move fast, upstream
squash-merges PRs (invalidating contributed branches), and this repo sometimes
has to pin a submodule to an in-flight PR branch. This procedure makes those
states explicit so no session has to re-derive them.

**As of 2026-08-08 every submodule is Tracking** — the first time that has been
true. `fujinet-nio-lib` rode `feature/amiga-rs232-pr` for the whole Amiga
transport bring-up; upstream merged it as
[markjfisher/fujinet-nio-lib#1](https://github.com/markjfisher/fujinet-nio-lib/pull/1)
and the pin moved to `upstream/master`. Keep it that way: the states below
exist to be left, not lived in.

---

## The three legal states of a submodule pointer

Every submodule in this repo is, at any moment, in one of these states. The
parent-repo bump commit message should say which.

1. **Tracking** — pinned to a commit on `upstream/master` (or the upstream
   default branch). The steady state, and where all five submodules are now.
2. **Riding a PR** — pinned to a feature branch on your fork (`origin`)
   because the parent repo needs work that upstream hasn't merged yet.
   Legitimate but *temporary*; every routine sync should try to exit this
   state. `apps/fujitzee/upstream` was here
   twice and left both times — worth reading as the two worked examples:
   the packed-struct opt-in
   [#9](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/9) merged on
   2026-08-06 and the branch was rebased away (the normal exit), and the
   big-endian opt-in
   [#10](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/10) merged and
   was then **reverted** by upstream a day later
   ([#11](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/11),
   [#12](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/12)) in favour of
   an existing server-side flag — see "If the PR was reverted" below.
   A game-port submodule can enter this state even though its normal role is
   a read-only pin; that also means its remotes may need the `origin`/
   `upstream` swap described in CLAUDE.md.
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

  **Check how it merged first — it is not always a squash.** nio-lib #1 landed
  as a real *merge commit* (`c69eadd`), so our `381b883` is a genuine ancestor
  of `upstream/master` rather than an invalidated duplicate. Confirm with:
  ```bash
  git -C <sub> merge-base --is-ancestor <our-sha> upstream/master && echo preserved
  ```
  When it is preserved, the fast-forward alone does the job and there is no
  rewrite to reconcile. The drift check still matters, but for a different
  reason: a merge resolves *upstream's* concurrent edits to the same files, so
  verify your platform hooks survived rather than diffing for review edits.
  For nio-lib #1 that meant grepping merged `master` for the `amiga` target,
  `COMPILER_FAMILY_amiga`, and `makefiles/compiler-amigagcc.mk` — upstream had
  been editing those same makefiles for BBC assembly work throughout.
- **Not merged →** keep the branch current with a rebase (upstream requires
  linear history; never merge master into the branch):
  ```bash
  git -C <sub> rebase upstream/master
  # resolve, build, test-host / emu-test
  git -C <sub> push --force-with-lease origin feature/<name>
  git add <sub> && git commit -m "bump <sub>: rebase feature/<name> onto upstream/master"
  ```

### If the PR was reverted

Merged is not the end of the story: upstream can merge a PR and then revert it,
usually because a better mechanism already existed. Treat this exactly like
*Merged* at the git level — pin to upstream's head, drop the branch — but the
parent repo has more to do, because the revert removes something the port was
depending on. Before bumping the pointer:

1. Read the revert PR's body and any doc it touched. It normally names the
   replacement.
2. Remove the port's side of the reverted feature — the `-D` flags in the app
   `Makefile`, the tests that pin it, the header comments that explain it.
3. Adopt the replacement, then rebuild and re-run T1 **and** T2. A revert that
   only breaks at runtime is the common case; the port compiled fine either
   way.

Worked example (2026-08-07): fujitzee's client-side byte swap
([#10](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/10)) was reverted by
[#12](https://github.com/FujiNetWIFI/fujinet-fujitzee/pull/12), whose body
pointed at `QUERY_SUFFIX "&be=1"` — a server-side flag the CoCo port already
used. The parent-repo change was: pin to upstream `main`, drop
`-DFUJITZEE_BIG_ENDIAN` and `test/host/test_endian.c`, set `QUERY_SUFFIX` in
`apps/fujitzee/amiga/include/amiga_vars.h`.

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
