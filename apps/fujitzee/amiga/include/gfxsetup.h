/*
 * gfxsetup.h - fujitzee's gamekit screen configuration
 *
 * One definition, in gfxsetup.c, so tiles.h (which defines art data, not
 * declarations) is included by exactly one translation unit.
 */
#ifndef GFXSETUP_H
#define GFXSETUP_H

#include "gfxcore.h"

extern const struct gfx_config fj_gfx_config;

#endif /* GFXSETUP_H */
