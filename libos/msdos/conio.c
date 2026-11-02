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

/* <conio.h> text output: the window, the cursor and colors. */

#include "conio-local.h"
#include <string.h>

struct __conio __conio = { .ungot = -1 };
int directvideo = 1;
int _wscroll = 1;

void
__conio_init(void)
{
    uint16_t cx = 0, dx = 0;
    uint8_t mode = __BDA8(0x49) & 0x7f;
    __conio.mode = mode;
    __conio.cols = (uint8_t)__BDA16(0x4a);
    if (__conio.cols == 0)
        __conio.cols = 80;
    /* The EGA and later keep the last row; the CGA and MDA have 25. */
    uint8_t lastrow = __BDA8(0x84);
    __conio.rows = (lastrow >= 24 && lastrow < 100) ? lastrow + 1 : 25;
    __conio.vseg = (mode == 7 ? 0xb000 : 0xb800) + (__BDA16(0x4e) >> 4);
    if (!__conio.ready) {
        __conio.ready = 1;
        __conio.lastmode = mode;
        /* The color under the cursor is the normal one. */
        uint16_t a = __conio_int10(0x0800, (uint16_t)__BDA8(0x62) << 8, &cx, &dx);
        __conio.normattr = a >> 8;
        if (__conio.normattr == 0)
            __conio.normattr = LIGHTGRAY;
        __conio.attr = __conio.normattr;
    }
    __conio.left = 0;
    __conio.top = 0;
    __conio.right = __conio.cols - 1;
    __conio.bottom = __conio.rows - 1;
}

void
__conio_begin(void)
{
    if (!__conio.ready)
        __conio_init();
    uint16_t pos = __BDA16(0x50 + 2 * __BDA8(0x62));
    __conio.x = (uint8_t)pos;
    __conio.y = pos >> 8;
}

void
__conio_end(void)
{
    uint16_t dx = (uint16_t)__conio.y << 8 | __conio.x;
    __conio_int10(0x0200, (uint16_t)__BDA8(0x62) << 8, 0, &dx);
}

void
__conio_fill(unsigned l, unsigned t, unsigned r, unsigned b, uint8_t a)
{
    if (!directvideo) {
        uint16_t cx = t << 8 | l, dx = b << 8 | r;
        __conio_int10(0x0600, (uint16_t)a << 8, &cx, &dx);
        return;
    }
    uint16_t blank = (uint16_t)a << 8 | ' ';
    for (unsigned y = t; y <= b; y++) {
        uint16_t __far *p = __conio_cell(l, y);
        for (unsigned x = l; x <= r; x++)
            *p++ = blank;
    }
}

void
__conio_scroll(unsigned l, unsigned t, unsigned r, unsigned b, int n)
{
    unsigned rows = b - t + 1;
    unsigned k = n < 0 ? -n : n;
    if (k >= rows) {
        __conio_fill(l, t, r, b, __conio.attr);
        return;
    }
    if (!directvideo) {
        uint16_t cx = t << 8 | l, dx = b << 8 | r;
        __conio_int10((n < 0 ? 0x0700 : 0x0600) | k, (uint16_t)__conio.attr << 8, &cx, &dx);
        return;
    }
    unsigned w = r - l + 1;
    if (n > 0) {
        for (unsigned y = t; y + k <= b; y++) {
            uint16_t __far *d = __conio_cell(l, y);
            uint16_t __far *s = __conio_cell(l, y + k);
            for (unsigned i = 0; i < w; i++)
                d[i] = s[i];
        }
        __conio_fill(l, b - k + 1, r, b, __conio.attr);
    } else {
        for (unsigned y = b; y >= t + k; y--) {
            uint16_t __far *d = __conio_cell(l, y);
            uint16_t __far *s = __conio_cell(l, y - k);
            for (unsigned i = 0; i < w; i++)
                d[i] = s[i];
        }
        __conio_fill(l, t, r, t + k - 1, __conio.attr);
    }
}

void
__conio_put(int c)
{
    switch (c) {
    case '\a':
        __conio_int10(0x0e07, 0, 0, 0);
        return;
    case '\b':
        if (__conio.x > __conio.left)
            __conio.x--;
        return;
    case '\r':
        __conio.x = __conio.left;
        return;
    case '\n':
        __conio.y++;
        break;
    default:
        if (directvideo) {
            *__conio_cell(__conio.x, __conio.y) = (uint16_t)__conio.attr << 8 | (uint8_t)c;
        } else {
            uint16_t page = (uint16_t)__BDA8(0x62) << 8;
            uint16_t dx = (uint16_t)__conio.y << 8 | __conio.x, cx = 1;
            __conio_int10(0x0200, page, 0, &dx);
            __conio_int10(0x0900 | (uint8_t)c, page | __conio.attr, &cx, 0);
        }
        if (++__conio.x > __conio.right) {
            __conio.x = __conio.left;
            __conio.y++;
        }
        break;
    }
    if (__conio.y > __conio.bottom) {
        if (_wscroll) {
            __conio_scroll(__conio.left, __conio.top, __conio.right, __conio.bottom, 1);
            __conio.y = __conio.bottom;
        } else {
            __conio.y = __conio.top;
        }
    }
}

int
putch(int c)
{
    __conio_begin();
    __conio_put(c);
    __conio_end();
    return (unsigned char)c;
}

int
cputs(const char *s)
{
    int c = 0;
    __conio_begin();
    while (*s)
        __conio_put(c = (unsigned char)*s++);
    __conio_end();
    return c;
}

void
window(int left, int top, int right, int bottom)
{
    __conio_begin();
    left--, top--, right--, bottom--;
    if (left < 0 || top < 0 || right >= __conio.cols || bottom >= __conio.rows ||
        left > right || top > bottom)
        return;
    __conio.left = left;
    __conio.top = top;
    __conio.right = right;
    __conio.bottom = bottom;
    __conio.x = left;
    __conio.y = top;
    __conio_end();
}

void
clrscr(void)
{
    __conio_begin();
    __conio_fill(__conio.left, __conio.top, __conio.right, __conio.bottom, __conio.attr);
    __conio.x = __conio.left;
    __conio.y = __conio.top;
    __conio_end();
}

void
clreol(void)
{
    __conio_begin();
    __conio_fill(__conio.x, __conio.y, __conio.right, __conio.y, __conio.attr);
}

void
insline(void)
{
    __conio_begin();
    __conio_scroll(__conio.left, __conio.y, __conio.right, __conio.bottom, -1);
}

void
delline(void)
{
    __conio_begin();
    __conio_scroll(__conio.left, __conio.y, __conio.right, __conio.bottom, 1);
}

void
gotoxy(int x, int y)
{
    __conio_begin();
    x += __conio.left - 1;
    y += __conio.top - 1;
    if (x < __conio.left || x > __conio.right || y < __conio.top || y > __conio.bottom)
        return;
    __conio.x = x;
    __conio.y = y;
    __conio_end();
}

int
wherex(void)
{
    __conio_begin();
    return __conio.x - __conio.left + 1;
}

int
wherey(void)
{
    __conio_begin();
    return __conio.y - __conio.top + 1;
}

void
textcolor(int color)
{
    __conio_begin();
    __conio.attr = (__conio.attr & 0x70) | (color & 0x8f);
}

void
textbackground(int color)
{
    __conio_begin();
    __conio.attr = (__conio.attr & 0x8f) | ((color & 7) << 4);
}

void
textattr(int attr)
{
    __conio_begin();
    __conio.attr = (uint8_t)attr;
}

void
highvideo(void)
{
    __conio_begin();
    __conio.attr |= 8;
}

void
lowvideo(void)
{
    __conio_begin();
    __conio.attr &= ~8;
}

void
normvideo(void)
{
    __conio_begin();
    __conio.attr = __conio.normattr;
}
