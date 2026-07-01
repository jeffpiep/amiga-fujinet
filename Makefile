# Repo-wide test aggregation. See docs/testing.md and CLAUDE.md "## Testing".
#
#   make test        fast host unit tests (T1) — the CI default
#   make test-host   same as `make test`
#   make emu-test    slow emulator smoke tests (T2) — needs FS-UAE + live fujinet-nio
#
# The fujinet-nio host ctest suite (T3) lives in the submodule and is run via
# `cd fujinet-nio && ./build.sh -p fujibus-rs232-debug` — it is intentionally
# NOT driven from here (submodule boundary: referenced, never modified).

# Modules that expose a `test-host` target (T1). Add apps/* as they gain one.
HOST_TEST_DIRS = libs/fujinet-compat-amiga

# Apps with a T2 emulator smoke test.
EMU_TEST_DIRS = apps/fn_test apps/compat_test apps/http_get

.PHONY: test test-host emu-test

test: test-host

test-host:
	@set -e; for d in $(HOST_TEST_DIRS); do \
	  echo "### $$d ###"; $(MAKE) -C $$d test-host; done

emu-test:
	@set -e; for d in $(EMU_TEST_DIRS); do \
	  echo "### $$d ###"; $(MAKE) -C $$d emu-test; done
