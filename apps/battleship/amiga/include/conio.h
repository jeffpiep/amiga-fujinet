#ifndef CONIO_H
#define CONIO_H

/* Amiga shim for conio.h — provides kbhit()/cgetc() expected by standard_lib.h */

unsigned char kbhit(void);
char cgetc(void);

#endif /* CONIO_H */
