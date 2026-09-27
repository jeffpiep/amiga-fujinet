# release/Makefile — bootable ADFs for testers with real hardware
#
#   make -C release          build every disk into release/adf/
#   make -C release clean    remove them
#
# Each disk is a plain KS 1.3 boot floppy that carries the resident broker
# (contracts/amiga-adf-bootstrap.md), so it needs nothing from the tester but
# a serial cable to a machine running fujinet-nio at 19200 baud. README.txt
# in this directory is the tester-facing sheet; it is copied next to the
# ADFs so the folder can be zipped and sent as-is.
#
# release/adf/ is committed so testers can download it from GitHub. The disks
# carry serial.device and Workbench C: commands, which the project treats as
# redistributable (contracts/amiga-adf-bootstrap.md). Rebuild and commit
# them when a change should reach testers.

include ../make/amiga.mk

ROOT     = ..
BUILDADF = $(abspath $(ROOT)/emu/scripts/build-adf.sh)
OUT      = $(CURDIR)/adf

BATTLESHIP = $(abspath $(ROOT)/apps/battleship/amiga/battleship)
FUJITZEE   = $(abspath $(ROOT)/apps/fujitzee/amiga/fujitzee)
FN_TEST    = $(abspath $(ROOT)/apps/fn_test/fn_test)
HTTP_GET   = $(abspath $(ROOT)/apps/http_get/http_get)

DISKS = $(OUT)/fujinet-tools.adf $(OUT)/battleship.adf $(OUT)/fujitzee.adf

# Shell commands a tester may want once the tools disk drops to a prompt.
TOOLS_WB_FILES = C/Dir C/List C/Type C/Info C/Avail C/CD C/Copy C/Ed \
                 C/Execute C/Wait C/Version

BROKER = BROKER_DEVICE=$(BROKER_DEVICE) LOAD_RESIDENT=$(LOAD_RESIDENT)

all: $(DISKS) $(OUT)/README.txt

$(OUT):
	mkdir -p $@

$(OUT)/README.txt: README.txt | $(OUT)
	cp $< $@

# The apps are built by their own Makefiles; always ask them, like amiga.mk
# does for the broker, so a stale binary never gets packaged.
$(BATTLESHIP): FORCE
	$(MAKE) -C $(ROOT)/apps/battleship/amiga battleship
$(FUJITZEE): FORCE
	$(MAKE) -C $(ROOT)/apps/fujitzee/amiga fujitzee
$(FN_TEST): FORCE
	$(MAKE) -C $(ROOT)/apps/fn_test
$(HTTP_GET): FORCE
	$(MAKE) -C $(ROOT)/apps/http_get

$(OUT)/battleship.adf: $(BATTLESHIP) $(BROKER_DEVICE) $(LOAD_RESIDENT) | $(OUT)
	APP_NAME=battleship APP_BINARY=$(BATTLESHIP) $(BROKER) \
	ADF_OUT=$@ $(BUILDADF)

$(OUT)/fujitzee.adf: $(FUJITZEE) $(BROKER_DEVICE) $(LOAD_RESIDENT) | $(OUT)
	APP_NAME=fujitzee APP_BINARY=$(FUJITZEE) $(BROKER) \
	ADF_OUT=$@ $(BUILDADF)

# Boots, runs fn_test as a link check, then leaves a CLI with http_get and
# a few Workbench commands on the disk.
$(OUT)/fujinet-tools.adf: $(FN_TEST) $(HTTP_GET) $(BROKER_DEVICE) $(LOAD_RESIDENT) | $(OUT)
	rm -rf $(OUT)/.tools-static && mkdir -p $(OUT)/.tools-static
	cp $(HTTP_GET) $(OUT)/.tools-static/http_get
	APP_NAME=fn_test APP_BINARY=$(FN_TEST) $(BROKER) \
	ADF_LABEL=FUJINET-TOOLS \
	ADF_STATIC_DIR=$(OUT)/.tools-static \
	ADF_WB_FILES="$(TOOLS_WB_FILES)" \
	ADF_OUT=$@ $(BUILDADF)
	rm -rf $(OUT)/.tools-static

clean:
	rm -rf $(OUT)

.PHONY: all clean
