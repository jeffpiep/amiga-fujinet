#ifndef PEEKPOKE_H
#define PEEKPOKE_H

/* Amiga shim for cc65's <peekpoke.h>. gamelogic.c includes it under
 * #ifndef __WATCOMC__ but never calls PEEK/POKE — only the Atari platform
 * layer does, and that file is not part of this build. Deliberately empty:
 * defining PEEK/POKE here would invite their use on a machine where a bare
 * absolute address means nothing. */

#endif /* PEEKPOKE_H */
