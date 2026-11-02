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

/* The state of the <conio.h> functions, shared by conio*.c. */

#ifndef _CONIO_LOCAL_H_
#define _CONIO_LOCAL_H_

#include <conio.h>
#include <i86.h>
#include <stdint.h>

struct __conio {
    uint8_t ready;
    /* The window, counted from 0, inclusive. */
    uint8_t left, top, right, bottom;
    uint8_t attr, normattr;
    uint8_t mode, lastmode;
    uint8_t cols, rows;
    /* The cursor, counted from 0 from the top left of the screen. */
    uint8_t x, y;
    uint16_t vseg; /* video memory of the displayed page */
    int16_t ungot; /* ungetch's character, or -1 */
};

extern struct __conio __conio;

/* The BIOS data area. */
#define __BDA8(o) (*(volatile uint8_t __far *)MK_FP(0x40, (o)))
#define __BDA16(o) (*(volatile uint16_t __far *)MK_FP(0x40, (o)))

/* INT 10h with AX, BX, CX and DX; returns AX, and CX and DX through
 * pointers when they are not null. BP is saved, as some BIOSes change it. */
static __inline__ uint16_t
__conio_int10(uint16_t ax, uint16_t bx, uint16_t *cx, uint16_t *dx)
{
    uint16_t c = cx ? *cx : 0, d = dx ? *dx : 0;
    __asm__ __volatile__("pushw %%bp\n\tint $0x10\n\tpopw %%bp"
                         : "+a"(ax), "+b"(bx), "+c"(c), "+d"(d)
                         :
                         : "si", "di", "memory", "cc");
    if (cx)
        *cx = c;
    if (dx)
        *dx = d;
    return ax;
}

/* Read the screen size, mode and cursor from the BIOS (the first time,
 * also the window and colors). */
void __conio_init(void);
/* Make sure the state is set up, and take the cursor from the BIOS, which
 * other output may have moved. */
void __conio_begin(void);
/* Move the BIOS cursor to the conio cursor. */
void __conio_end(void);
/* getpass, for getpass.c. */
char *__conio_getpass(const char *prompt);
/* Write c at the cursor, as putch does, without moving the BIOS cursor. */
void __conio_put(int c);
/* The video memory of cell (x, y), counted from 0. */
static __inline__ uint16_t __far *
__conio_cell(unsigned x, unsigned y)
{
    return (uint16_t __far *)MK_FP(__conio.vseg, (y * __conio.cols + x) * 2);
}
/* Fill columns l to r of rows t to b with blanks in attribute a. */
void __conio_fill(unsigned l, unsigned t, unsigned r, unsigned b, uint8_t a);
/* Scroll columns l to r of rows t to b up by n rows (down when n < 0),
 * clearing the rows left behind. */
void __conio_scroll(unsigned l, unsigned t, unsigned r, unsigned b, int n);

#endif /* _CONIO_LOCAL_H_ */
