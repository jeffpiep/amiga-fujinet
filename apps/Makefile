# apps/Makefile — build all Amiga apps

APPS = http_get

all:
	@for app in $(APPS); do \
		echo "Building $$app..."; \
		$(MAKE) -C $$app || exit 1; \
	done

clean:
	@for app in $(APPS); do \
		$(MAKE) -C $$app clean; \
	done

.PHONY: all clean $(APPS)
