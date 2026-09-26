# make/amiga.mk — shared amiga-gcc toolchain definitions
#
# Include near the top of any Amiga Makefile in this repo:
#
#   include ../../make/amiga.mk        # (path relative to the app dir)
#
# Provides:
#   CC, AR              — the m68k-amigaos toolchain
#   CFLAGS              — canonical flags (append per-module with CFLAGS +=)
#   NIO_LIB / NIO_INC / NIO_ALIB       — fujinet-nio-lib paths + library
#   COMPAT / COMPAT_INC / COMPAT_LIB   — compat-layer paths + library
#   GAMEKIT / GAMEKIT_INC / GAMEKIT_LIB — shared Amiga game platform layer
#   BROKER_DEVICE / LOAD_RESIDENT      — what a networked boot disk needs
#                                        (contracts/amiga-adf-bootstrap.md)
#
# `$(COMPAT_LIB)` and `$(GAMEKIT_LIB)` rules are included so any app that
# lists them as prerequisites gets those libraries built automatically. The
# nio-lib library is NOT auto-built (submodule boundary — its build interface
# is upstream's); build it per CLAUDE.md if $(NIO_ALIB) is missing.

_AMIGA_MK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
_AMIGA_ROOT   := $(abspath $(_AMIGA_MK_DIR)..)

CC = m68k-amigaos-gcc
AR = m68k-amigaos-ar

CFLAGS = -Wall -Wextra -O2 -std=c99 -mcpu=68000 -msoft-float -mcrt=nix13 \
         -DNO_INLINE_MULDIV -Wno-pointer-sign

NIO_LIB  = $(_AMIGA_ROOT)/fujinet-nio-lib
NIO_INC  = $(NIO_LIB)/include
NIO_ALIB = $(NIO_LIB)/build/fujinet-nio-amiga.a

COMPAT     = $(_AMIGA_ROOT)/libs/fujinet-compat-amiga
COMPAT_INC = $(COMPAT)/include
COMPAT_LIB = $(COMPAT)/libfn_compat_amiga.a

# Shared game platform layer (screen/tiles/sprites, input decode, waveform
# bakers, jiffy clock). Used by apps/battleship and apps/fujitzee — see
# libs/amiga-gamekit and docs/plan-track1c-fujitzee.md.
GAMEKIT     = $(_AMIGA_ROOT)/libs/amiga-gamekit
GAMEKIT_INC = $(GAMEKIT)/include
GAMEKIT_LIB = $(GAMEKIT)/libamiga_gamekit.a

# The resident broker nio-lib's transport opens, and the tool that loads it.
# Every ADF that reaches FujiNet carries both; see
# contracts/amiga-adf-bootstrap.md.
DRIVER        = $(_AMIGA_ROOT)/fujinet-nio-driver
BROKER_DEVICE = $(DRIVER)/build/amiga/fujinet-nio.device
LOAD_RESIDENT = $(_AMIGA_ROOT)/build/amiga/fujinet-load-resident

# The device is the driver's own target; its Makefile tracks the sources, so
# always ask it rather than second-guessing staleness from here.
$(BROKER_DEVICE): FORCE
	$(MAKE) -C $(DRIVER)/amiga ../build/amiga/fujinet-nio.device

# The driver's rule for this tool hardcodes -mcrt=clib2, which our amiga-gcc
# does not ship. It uses only LoadSeg/InitResident and stdio, so libnix's
# 1.3 crt builds it and it runs on KS 1.3.
$(LOAD_RESIDENT): $(DRIVER)/amiga/tools/fujinet-load-resident.c
	@mkdir -p $(dir $@)
	$(CC) -std=c99 -Wall -Wextra -Werror -O2 -mcpu=68000 -msoft-float \
	    -mcrt=nix13 -I$(DRIVER)/amiga/include -o $@ $< -lamiga

.PHONY: FORCE
FORCE:

# Guard: skip when included from a library's own Makefile, which defines the
# real rule for that target.
ifneq ($(abspath $(CURDIR)),$(COMPAT))
$(COMPAT_LIB):
	$(MAKE) -C $(COMPAT)
endif

ifneq ($(abspath $(CURDIR)),$(GAMEKIT))
$(GAMEKIT_LIB):
	$(MAKE) -C $(GAMEKIT)
endif

# This include is usually the first thing in a Makefile, which would make the
# rule above the default goal. Every Makefile in this repo names its default
# target `all` — pin it so `make` alone always means `make all`.
.DEFAULT_GOAL := all
