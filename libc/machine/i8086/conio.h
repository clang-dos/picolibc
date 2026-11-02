/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* <conio.h>: the text screen and keyboard of the DOS compilers. The
 * console functions of Turbo C and Borland C++ (windows, colors, cursor
 * movement and screen copies) and of Microsoft C and Open Watcom (getch,
 * kbhit, cprintf, port I/O) in one header.
 *
 * Output goes straight to the video memory of the text screen (or through
 * the BIOS when directvideo is 0), in the current text window and color.
 * As in Turbo C, '\n' moves the cursor down a line and '\r' back to the
 * start of it: write "\r\n" to start a new line. */

#ifndef _CONIO_H_
#define _CONIO_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __COLORS
#define __COLORS
/* The sixteen text colors. */
enum COLORS {
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY,
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
};
#endif

/* Added to a foreground color: the character blinks. */
#define BLINK 128

/* The text modes of textmode(). C4350 is 43 lines on an EGA or 50 lines
 * on a VGA, and LASTMODE the mode before the last textmode() call. */
enum text_modes {
    LASTMODE = -1,
    BW40 = 0,
    C40 = 1,
    BW80 = 2,
    C80 = 3,
    MONO = 7,
    C4350 = 64
};

/* The text window and screen, from gettextinfo(). Coordinates count from
 * 1 at the top left of the screen. */
struct text_info {
    unsigned char winleft, wintop, winright, winbottom;
    unsigned char attribute, normattr;
    unsigned char currmode;
    unsigned char screenheight, screenwidth;
    unsigned char curx, cury;
};

/* The cursor shapes of _setcursortype(). */
#define _NOCURSOR 0
#define _SOLIDCURSOR 1
#define _NORMALCURSOR 2

/* Nonzero (the default): write to video memory. 0: go through the BIOS,
 * which is slower but works with any display adapter. */
extern int directvideo;
/* Nonzero (the default): scroll the window when output passes its last
 * line. 0: wrap to the top line instead. */
extern int _wscroll;

/* The text window: the screen area that output, clrscr and gotoxy use. */
void window(int __left, int __top, int __right, int __bottom);
void clrscr(void);
void clreol(void);
void insline(void);
void delline(void);
void gotoxy(int __x, int __y);
int wherex(void);
int wherey(void);

/* Colors: the color of new text, of its background, or both as a
 * BIOS attribute (background * 16 + foreground). */
void textcolor(int __color);
void textbackground(int __color);
void textattr(int __attr);
void highvideo(void);
void lowvideo(void);
void normvideo(void);

/* The screen: the text mode, its state, and the cursor. */
void textmode(int __mode);
void gettextinfo(struct text_info *__r);
void _setcursortype(int __type);

/* Copy screen rectangles (2 bytes a cell: character, attribute) to and
 * from memory, or within the screen. Return 1, or 0 for a rectangle not
 * on the screen. */
int gettext(int __left, int __top, int __right, int __bottom, void *__dest);
int puttext(int __left, int __top, int __right, int __bottom,
            const void *__src);
int movetext(int __left, int __top, int __right, int __bottom,
             int __destleft, int __desttop);

/* Output in the text window. */
int putch(int __c);
int cputs(const char *__s);
int cprintf(const char *__format, ...)
    __attribute__((__format__(__printf__, 1, 2)));
int vcprintf(const char *__format, va_list __ap);

/* Keyboard. getch waits for a key and returns it without echo; a key
 * with no character (a function or arrow key) gives 0, then its scan
 * code on the next call. getche echoes it. kbhit is nonzero when a key
 * is waiting. ungetch pushes one character back. */
int getch(void);
int getche(void);
int kbhit(void);
int ungetch(int __c);

/* Read a line: __buf[0] is the most characters to read; the line is
 * stored from __buf + 2, its length in __buf[1]. Returns __buf + 2. */
char *cgets(char *__buf);
int cscanf(const char *__format, ...)
    __attribute__((__format__(__scanf__, 1, 2)));
/* Prompt and read a password of up to 8 characters, without echo. */
char *getpass(const char *__prompt);

/* I/O ports, as in Microsoft C and Open Watcom. */
static __inline__ unsigned
inp(unsigned __port)
{
    unsigned char __v;
    __asm__ __volatile__("inb %w1, %b0" : "=a"(__v) : "Nd"((unsigned short)__port));
    return __v;
}

static __inline__ unsigned
inpw(unsigned __port)
{
    unsigned short __v;
    __asm__ __volatile__("inw %w1, %w0" : "=a"(__v) : "Nd"((unsigned short)__port));
    return __v;
}

static __inline__ unsigned
outp(unsigned __port, unsigned __value)
{
    __asm__ __volatile__("outb %b0, %w1" : : "a"((unsigned char)__value),
                         "Nd"((unsigned short)__port));
    return __value;
}

static __inline__ unsigned
outpw(unsigned __port, unsigned __value)
{
    __asm__ __volatile__("outw %w0, %w1" : : "a"((unsigned short)__value),
                         "Nd"((unsigned short)__port));
    return __value;
}

#ifdef __cplusplus
}
#endif

#endif /* _CONIO_H_ */
