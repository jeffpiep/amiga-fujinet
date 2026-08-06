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
