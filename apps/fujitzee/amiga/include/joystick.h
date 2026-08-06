#ifndef JOYSTICK_H
#define JOYSTICK_H

/* Amiga shim for cc65/CMOC's <joystick.h>. Upstream's misc.c and misc.h
 * include it for the JOY_* accessor macros only — never for a joystick
 * driver API. The macros themselves are defined in amiga_vars.h over the
 * gamekit's GK_JOY_* bit layout (libs/amiga-gamekit/include/gkinput.h),
 * which is where the port's key/joy contract lives; this header exists so
 * the #include resolves.
 *
 * Upstream ships stubs in upstream/src/include/, but that directory also
 * holds stdio.h/string.h/... stubs that would shadow the real toolchain
 * headers, so we cannot simply add it to the include path. */

#endif /* JOYSTICK_H */
