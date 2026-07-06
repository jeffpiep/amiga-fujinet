# apps/Makefile — build all Amiga apps
#
# battleship needs its upstream submodule initialized and builds via an
# explicit target (bare `make` there builds the ADF) — see CLAUDE.md.

APPS = fn_test http_get compat_test

all: $(APPS) battleship

$(APPS):
	$(MAKE) -C $@

battleship:
	$(MAKE) -C battleship/amiga battleship

clean:
	@for app in $(APPS); do \
		$(MAKE) -C $$app clean; \
	done
	$(MAKE) -C battleship/amiga clean

.PHONY: all clean $(APPS) battleship
