# emu/template/emu.mk — Emulator test targets for Amiga apps
#
# Include this from any app Makefile to get emu-adf, emu-test, emu-clean:
#
#   include ../../emu/template/emu.mk
#
# Required variables (set before the include):
#   APP_NAME          — e.g. fn_test
#   APP_BINARY        — compiled binary path relative to app dir (default: $(TARGET))
#   EMU_PASS_PATTERN  — grep string; match in fujinet.log or serial-trace.log → PASS
#
# Optional variables:
#   EMU_FAIL_PATTERN  — grep string; match → immediate FAIL
#   EMU_TIMEOUT       — seconds before timeout FAIL (default: 60)
#   EMU_STARTUP_ARGS  — extra args appended to app name in startup-sequence

_EMU_MK_DIR  := $(dir $(lastword $(MAKEFILE_LIST)))
_EMU_ROOT    := $(abspath $(_EMU_MK_DIR)../..)
_EMU_DIR     := $(_EMU_ROOT)/emu
_APP_ADF     := $(CURDIR)/$(APP_NAME).adf

APP_BINARY       ?= $(TARGET)
EMU_FAIL_PATTERN ?=
EMU_TIMEOUT      ?= 60
EMU_STARTUP_ARGS ?=

.PHONY: emu-adf emu-test emu-clean

emu-adf: $(APP_BINARY)
	APP_NAME=$(APP_NAME) \
	APP_BINARY=$(abspath $(APP_BINARY)) \
	EMU_STARTUP_ARGS="$(EMU_STARTUP_ARGS)" \
	ADF_OUT=$(_APP_ADF) \
	$(_EMU_DIR)/scripts/build-adf.sh

emu-test: emu-adf
	APP_NAME=$(APP_NAME) \
	ADF_PATH=$(_APP_ADF) \
	PASS_PATTERN="$(EMU_PASS_PATTERN)" \
	FAIL_PATTERN="$(EMU_FAIL_PATTERN)" \
	TIMEOUT_S=$(EMU_TIMEOUT) \
	$(_EMU_DIR)/run.sh

emu-clean:
	rm -f $(_APP_ADF)
	rm -rf $(_EMU_DIR)/logs/$(APP_NAME)
