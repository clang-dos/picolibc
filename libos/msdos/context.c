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

/* <conio.h>: the text mode, the cursor shape, and screen copies. */

#include "conio-local.h"

void
gettextinfo(struct text_info *r)
{
    __conio_begin();
    r->winleft = __conio.left + 1;
    r->wintop = __conio.top + 1;
    r->winright = __conio.right + 1;
    r->winbottom = __conio.bottom + 1;
    r->attribute = __conio.attr;
    r->normattr = __conio.normattr;
    r->currmode = (__conio.rows > 25 && __conio.mode == C80) ? C4350 : __conio.mode;
    r->screenheight = __conio.rows;
    r->screenwidth = __conio.cols;
    r->curx = __conio.x - __conio.left + 1;
    r->cury = __conio.y - __conio.top + 1;
}

void
textmode(int mode)
{
    struct text_info ti;
    gettextinfo(&ti);
    if (mode == LASTMODE)
        mode = __conio.lastmode;
    __conio.lastmode = ti.currmode;
    if (mode == C4350) {
        __conio_int10(C80, 0, 0, 0);
        /* The 8x8 font: 43 rows on an EGA, 50 on a VGA. */
        __conio_int10(0x1112, 0, 0, 0);
    } else {
        __conio_int10((uint8_t)mode, 0, 0, 0);
    }
    __conio_init();
    __conio.x = 0;
    __conio.y = 0;
    __conio_end();
}

void
_setcursortype(int type)
{
    __conio_begin();
    unsigned h = __BDA8(0x85);
    if (h == 0)
        h = __conio.mode == 7 ? 14 : 8;
    uint16_t cx;
    switch (type) {
    case _NOCURSOR:
        cx = 0x2000;
        break;
    case _SOLIDCURSOR:
        cx = h - 1;
        break;
    default:
        cx = (h - 2) << 8 | (h - 1);
        break;
    }
    __conio_int10(0x0100, 0, &cx, 0);
}

static int
in_screen(int left, int top, int right, int bottom)
{
    __conio_begin();
    return left >= 1 && top >= 1 && left <= right && top <= bottom &&
           right <= __conio.cols && bottom <= __conio.rows;
}

int
gettext(int left, int top, int right, int bottom, void *dest)
{
    if (!in_screen(left, top, right, bottom))
        return 0;
    uint16_t *d = dest;
    for (int y = top - 1; y < bottom; y++) {
        uint16_t __far *s = __conio_cell(left - 1, y);
        for (int x = left; x <= right; x++)
            *d++ = *s++;
    }
    return 1;
}

int
puttext(int left, int top, int right, int bottom, const void *src)
{
    if (!in_screen(left, top, right, bottom))
        return 0;
    const uint16_t *s = src;
    for (int y = top - 1; y < bottom; y++) {
        uint16_t __far *d = __conio_cell(left - 1, y);
        for (int x = left; x <= right; x++)
            *d++ = *s++;
    }
    return 1;
}

int
movetext(int left, int top, int right, int bottom, int destleft, int desttop)
{
    int destright = destleft + right - left, destbottom = desttop + bottom - top;
    if (!in_screen(left, top, right, bottom) ||
        !in_screen(destleft, desttop, destright, destbottom))
        return 0;
    int w = right - left + 1;
    /* Copy in the order that never overwrites what is still to copy. */
    int down = desttop > top || (desttop == top && destleft > left);
    for (int i = 0; i <= bottom - top; i++) {
        int r = down ? bottom - top - i : i;
        uint16_t __far *s = __conio_cell(left - 1, top - 1 + r);
        uint16_t __far *d = __conio_cell(destleft - 1, desttop - 1 + r);
        if (down)
            for (int x = w - 1; x >= 0; x--)
                d[x] = s[x];
        else
            for (int x = 0; x < w; x++)
                d[x] = s[x];
    }
    return 1;
}
