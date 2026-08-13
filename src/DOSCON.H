/*

       / / / / / / / / /
     / / doscon.h  / /
   / / / / / / / / /

         2026
        mpower

  small DOS console layer
   for Microsoft C 4.0

*/

#ifndef WORDELL_DOSCON_H
#define WORDELL_DOSCON_H

void dosVideoInit (void);
void clrscr (void);
int  cgetc (void);
void cputc (int);
void gotoxy (int, int);
void gotox (int);

#endif
