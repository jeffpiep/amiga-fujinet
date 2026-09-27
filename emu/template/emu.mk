# emu/template/emu.mk — Emulator test targets for Amiga apps
#
# Include this from any app Makefile to get emu-adf, emu-test, emu-clean:
#
#   include ../../emu/template/emu.mk
#
# IMPORTANT: Define your default target (e.g. `all:`) BEFORE this include,
# otherwise emu-adf becomes the default target and `make` alone won't build.
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
#   EMU_STARTUP_PREFIX — AmigaDOS lines run after the broker, before the app
#   ADF_STATIC_DIR    — tree copied to the ADF root
#   ADF_WB_FILES      — extra files copied from the Workbench ADF (e.g. C/Mount)
#   EMU_ADF_DEPS      — extra prerequisites of emu-adf (staged files, images)
#
# The ADF always carries the resident broker (fujinet-nio.device) and the
# lines that load it — see contracts/amiga-adf-bootstrap.md. That needs
# $(BROKER_DEVICE) and $(LOAD_RESIDENT) from make/amiga.mk, included first.

_EMU_MK_DIR  := $(dir $(lastword $(MAKEFILE_LIST)))
_EMU_ROOT    := $(abspath $(_EMU_MK_DIR)../..)
_EMU_DIR     := $(_EMU_ROOT)/emu
_APP_ADF     := $(CURDIR)/$(APP_NAME).adf

APP_BINARY       ?= $(TARGET)
EMU_FAIL_PATTERN ?=
EMU_TIMEOUT      ?= 60
EMU_STARTUP_ARGS ?=
ADF_STATIC_DIR   ?=
EMU_STARTUP_PREFIX ?=
# Exported, not expanded into the recipe: it is usually a multi-line define,
# and a newline inside a recipe line splits it into separate shell commands.
export EMU_STARTUP_PREFIX
ADF_WB_FILES     ?=
EMU_ADF_DEPS     ?=

.PHONY: emu-adf emu-test emu-play emu-clean

emu-adf: $(APP_BINARY) $(BROKER_DEVICE) $(LOAD_RESIDENT) $(EMU_ADF_DEPS)
	APP_NAME=$(APP_NAME) \
	APP_BINARY=$(abspath $(APP_BINARY)) \
	BROKER_DEVICE=$(BROKER_DEVICE) \
	LOAD_RESIDENT=$(LOAD_RESIDENT) \
	EMU_STARTUP_ARGS="$(EMU_STARTUP_ARGS)" \
	ADF_STATIC_DIR="$(ADF_STATIC_DIR)" \
	ADF_WB_FILES="$(ADF_WB_FILES)" \
	ADF_OUT=$(_APP_ADF) \
	$(_EMU_DIR)/scripts/build-adf.sh

emu-test: emu-adf
	APP_NAME=$(APP_NAME) \
	ADF_PATH=$(_APP_ADF) \
	PASS_PATTERN="$(EMU_PASS_PATTERN)" \
	FAIL_PATTERN="$(EMU_FAIL_PATTERN)" \
	TIMEOUT_S=$(EMU_TIMEOUT) \
	$(_EMU_DIR)/run.sh

# Interactive: visible FS-UAE window, no pass/fail polling. For the manual
# checks the headless harness cannot do (anything that needs typing).
emu-play: emu-adf
	APP_NAME=$(APP_NAME) \
	ADF_PATH=$(_APP_ADF) \
	$(_EMU_DIR)/play.sh

emu-clean:
	rm -f $(_APP_ADF)
	rm -rf $(_EMU_DIR)/logs/$(APP_NAME)
