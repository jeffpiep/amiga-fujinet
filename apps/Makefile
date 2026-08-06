# apps/Makefile — build all Amiga apps
#
# The game ports need their upstream submodules initialized, and battleship
# builds via an explicit target (bare `make` there builds the ADF) — see
# CLAUDE.md.

APPS = fn_test http_get compat_test

all: $(APPS) battleship fujitzee

$(APPS):
	$(MAKE) -C $@

battleship:
	$(MAKE) -C battleship/amiga battleship

fujitzee:
	$(MAKE) -C fujitzee/amiga

clean:
	@for app in $(APPS); do \
		$(MAKE) -C $$app clean; \
	done
	$(MAKE) -C battleship/amiga clean
	$(MAKE) -C fujitzee/amiga clean

.PHONY: all clean $(APPS) battleship fujitzee
