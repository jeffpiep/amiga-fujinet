#ifndef CONIO_H
#define CONIO_H

/* Amiga shim for cc65's <conio.h> — upstream's misc.c calls kbhit()/cgetc()
 * unconditionally. Only these two are used; the rest of cc65's conio is not.
 * Implemented in src/input.c. */

unsigned char kbhit(void);
char cgetc(void);

#endif /* CONIO_H */
