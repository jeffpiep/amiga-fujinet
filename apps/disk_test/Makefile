# apps/disk_test/Makefile
#
# Mounts FujiNet media as DN0: through fujinet-disk.device on KS 1.3 and
# reads it through DOS. Spec: contracts/amiga-adf-bootstrap.md,
# "Disk device (DN0:) on KS 1.3".
#
# Usage:
#   make              — build disk_test binary
#   make emu-test     — build ADF + test image, run in FS-UAE with fujinet-nio
#   make emu-clean    — remove ADF and emulator logs
#   make clean        — remove all build artifacts
#
# Prerequisites: as apps/fn_test, plus xdftool (amitools) for the test image.

include ../../make/amiga.mk

INCLUDES = -I$(NIO_INC) -I$(DRIVER)/amiga/include

TARGET  = disk_test
SRCS    = disk_test.c

# fujinet-nio serves host:/ from ./fujinet-data under its working directory,
# which for emu-test is this directory.
IMAGE      = fujinet-data/disk_test.adf
STATIC     = build/static
FILE_TEXT  = Hello from DN0 on KS 1.3\n

# ── Emulator test config ──────────────────────────────────────────────────
APP_NAME         = disk_test
APP_BINARY       = $(TARGET)
EMU_STARTUP_ARGS = check
ADF_STATIC_DIR   = $(abspath $(STATIC))
ADF_WB_FILES     = C/Mount
EMU_ADF_DEPS     = $(STATIC)/Devs/fujinet-disk.device $(STATIC)/Devs/MountList \
                   $(IMAGE)
define EMU_STARTUP_PREFIX
C:fujinet-load-resident DEVS:fujinet-disk.device fujinet-disk.device
disk_test mount 0 host:/disk_test.adf
Mount DN0:
endef
# disk_test check reports through a unit-1 mount request the server logs.
EMU_PASS_PATTERN = uri='host:/disk_test-PASS'
EMU_FAIL_PATTERN = uri='host:/disk_test-FAIL'
EMU_TIMEOUT      = 120

include ../../emu/template/emu.mk

# ── Build ─────────────────────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(STATIC)/Devs/fujinet-disk.device: $(DISK_DEVICE)
	@mkdir -p $(dir $@)
	cp $< $@

$(STATIC)/Devs/MountList: MountList
	@mkdir -p $(dir $@)
	cp $< $@

# An OFS image with the files disk_test check expects (see disk_test.c).
$(IMAGE): Makefile
	@mkdir -p $(dir $@) build
	printf '$(FILE_TEXT)' > build/hello.txt
	rm -f $@
	xdftool $@ create + format DISKTEST ofs \
	    + write build/hello.txt hello.txt \
	    + makedir sub + write build/hello.txt sub/again.txt

clean: emu-clean
	rm -rf $(TARGET) build $(IMAGE)

.PHONY: all clean
