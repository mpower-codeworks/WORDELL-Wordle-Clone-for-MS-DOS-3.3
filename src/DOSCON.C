/*

      / / / / / / / / /
    / / doscon.c  / /
  / / / / / / / / /

         2026
        mpower

  small DOS console layer
   for Microsoft C 4.0

  screen positioning uses BIOS
  keyboard/character I/O uses conio

*/

#include <dos.h>
#include <conio.h>

#include "DOSCON.H"

/* current BIOS display page
*/
static unsigned char videoPage = 0;

/* ============
** dosVideoInit
** ============
** keeps standard 80-column text modes
** otherwise switches to color text mode 3
*/
void dosVideoInit () {
    union REGS inregs;
    union REGS outregs;
    int mode;

    inregs.h.ah = 0x0F;
    int86(0x10, &inregs, &outregs);

    mode = outregs.h.al;
    videoPage = outregs.h.bh;

    if (mode != 2 && mode != 3 && mode != 7) {
        inregs.h.ah = 0x00;
        inregs.h.al = 0x03;
        int86(0x10, &inregs, &outregs);
        videoPage = 0;
    }
}

/* ======
** clrscr
** ======
** clears the full 80x25 text screen
** and returns the cursor to 0,0
*/
void clrscr () {
    union REGS inregs;
    union REGS outregs;

    inregs.h.ah = 0x06;
    inregs.h.al = 0;
    inregs.h.bh = 0x07;
    inregs.h.ch = 0;
    inregs.h.cl = 0;
    inregs.h.dh = 24;
    inregs.h.dl = 79;
    int86(0x10, &inregs, &outregs);

    gotoxy(0, 0);
}

/* =====
** cgetc
** =====
** returns one DOS console key
** extended keys are consumed as one keypress
*/
int cgetc () {
    int ch;

    ch = getch();

    if (ch == 0) {
        getch();
        return 0;
    }

    return ch;
}

/* =====
** cputc
** =====
*/
void cputc (ch)
    int ch;
{
    putch(ch);
}

/* ======
** gotoxy
** ======
** Wordell coordinates are zero based
*/
void gotoxy (x, y)
    int x;
    int y;
{
    union REGS inregs;
    union REGS outregs;

    inregs.h.ah = 0x02;
    inregs.h.bh = videoPage;
    inregs.h.dh = y;
    inregs.h.dl = x;
    int86(0x10, &inregs, &outregs);
}

/* =====
** gotox
** =====
** changes column but keeps current row
*/
void gotox (x)
    int x;
{
    union REGS inregs;
    union REGS outregs;
    unsigned char row;

    inregs.h.ah = 0x03;
    inregs.h.bh = videoPage;
    int86(0x10, &inregs, &outregs);
    row = outregs.h.dh;

    gotoxy(x, row);
}
