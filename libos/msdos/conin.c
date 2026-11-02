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

/* <conio.h> keyboard input. */

#include "conio-local.h"
#include <stdarg.h>
#include <stdio.h>

/* DOS function AH with AL, returning AL. */
static uint8_t
dos(uint16_t ax)
{
    __asm__ __volatile__("int $0x21" : "+a"(ax) : : "memory", "cc");
    return (uint8_t)ax;
}

int
getch(void)
{
    if (__conio.ungot >= 0) {
        int c = __conio.ungot;
        __conio.ungot = -1;
        return c;
    }
    /* Direct input without echo: a key with no character is 0, then its
     * scan code on the next call. */
    return dos(0x0700);
}

int
getche(void)
{
    int c = getch();
    putch(c);
    return c;
}

int
kbhit(void)
{
    if (__conio.ungot >= 0)
        return 1;
    return dos(0x0b00) != 0;
}

int
ungetch(int c)
{
    if (__conio.ungot >= 0 || c == EOF)
        return EOF;
    __conio.ungot = (unsigned char)c;
    return c;
}

/* Read a line of at most max characters into s, echoing with putch,
 * or not when echo is 0. Backspace erases. */
static unsigned
readline(char *s, unsigned max, int echo)
{
    unsigned n = 0;
    for (;;) {
        int c = getch();
        if (c == 0) {
            getch(); /* a key with no character */
            continue;
        }
        if (c == '\r')
            break;
        if (c == '\b') {
            if (n > 0) {
                n--;
                if (echo)
                    cputs("\b \b");
            }
            continue;
        }
        if (n < max) {
            s[n++] = (char)c;
            if (echo)
                putch(c);
        }
    }
    s[n] = '\0';
    return n;
}

char *
cgets(char *buf)
{
    unsigned char max = (unsigned char)buf[0];
    buf[1] = (char)readline(buf + 2, max ? max - 1 : 0, 1);
    return buf + 2;
}

int
cscanf(const char *format, ...)
{
    char line[128];
    readline(line, sizeof(line) - 1, 1);
    cputs("\r\n");
    va_list ap;
    va_start(ap, format);
    int n = vsscanf(line, format, ap);
    va_end(ap);
    return n;
}

char *
__conio_getpass(const char *prompt)
{
    static char pass[9];
    cputs(prompt);
    readline(pass, 8, 0);
    cputs("\r\n");
    return pass;
}
