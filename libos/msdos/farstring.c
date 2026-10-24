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

/*
 * The far memory and string functions of DOS compilers (_fmemcpy,
 * _fstrlen and the rest, as in Open Watcom), in every model. Their
 * pointers are far pointers, so they work on any memory, and blocks are
 * up to 64 KiB, within one segment: offsets wrap around, as with any far
 * pointer. The ones that copy or fill use the string instructions; they
 * keep DS and ES.
 */

#include <ctype.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>

#define SEG(p) ((uint16_t)((uint32_t)(const void __far *)(p) >> 16))
#define OFF(p) ((uint16_t)(uint32_t)(const void __far *)(p))

/* The linear address of a far pointer. */
static uint32_t
linear(const void __far *p)
{
    return ((uint32_t)SEG(p) << 4) + OFF(p);
}

void __far *
_fmemcpy(void __far *d, const void __far *s, size_t n)
{
    uint16_t di = OFF(d), si = OFF(s), cx = (uint16_t)n;
    /* Words, then the odd byte. */
    __asm__ volatile("pushw %%es\n\t"
                     "pushw %%ds\n\t"
                     "movw %[es], %%es\n\t"
                     "movw %[ds], %%ds\n\t"
                     "shrw $1, %%cx\n\t"
                     "rep movsw\n\t"
                     "adcw %%cx, %%cx\n\t"
                     "rep movsb\n\t"
                     "popw %%ds\n\t"
                     "popw %%es"
                     : "+D"(di), "+S"(si), "+c"(cx)
                     : [ds] "r"(SEG(s)), [es] "r"(SEG(d))
                     : "memory");
    return d;
}

void __far *
_fmemmove(void __far *d, const void __far *s, size_t n)
{
    uint32_t ld = linear(d), ls = linear(s);
    if (ld <= ls || ld >= ls + (uint16_t)n)
        return _fmemcpy(d, s, n);
    /* d is inside the source: copy from the end down. */
    uint16_t cx = (uint16_t)n;
    uint16_t di = OFF(d) + cx - 1, si = OFF(s) + cx - 1;
    __asm__ volatile("pushw %%es\n\t"
                     "pushw %%ds\n\t"
                     "movw %[es], %%es\n\t"
                     "movw %[ds], %%ds\n\t"
                     "std\n\t"
                     "rep movsb\n\t"
                     "cld\n\t"
                     "popw %%ds\n\t"
                     "popw %%es"
                     : "+D"(di), "+S"(si), "+c"(cx)
                     : [ds] "r"(SEG(s)), [es] "r"(SEG(d))
                     : "memory");
    return d;
}

void __far *
_fmemset(void __far *d, int c, size_t n)
{
    uint16_t di = OFF(d), cx = (uint16_t)n;
    uint16_t ax = (uint8_t)c * 0x101u;
    __asm__ volatile("pushw %%es\n\t"
                     "movw %[es], %%es\n\t"
                     "shrw $1, %%cx\n\t"
                     "rep stosw\n\t"
                     "adcw %%cx, %%cx\n\t"
                     "rep stosb\n\t"
                     "popw %%es"
                     : "+D"(di), "+c"(cx)
                     : "a"(ax), [es] "r"(SEG(d))
                     : "memory");
    return d;
}

void __far *
_fmemccpy(void __far *d, const void __far *s, int c, size_t n)
{
    unsigned char __far       *dp = d;
    const unsigned char __far *sp = s;
    while (n--) {
        unsigned char ch = *sp++;
        *dp++ = ch;
        if (ch == (unsigned char)c)
            return dp;
    }
    return NULL;
}

void __far *
_fmemchr(const void __far *s, int c, size_t n)
{
    const unsigned char __far *p = s;
    for (; n; n--, p++)
        if (*p == (unsigned char)c)
            return (void __far *)p;
    return NULL;
}

int
_fmemcmp(const void __far *a, const void __far *b, size_t n)
{
    const unsigned char __far *p = a, *q = b;
    for (; n; n--, p++, q++)
        if (*p != *q)
            return *p - *q;
    return 0;
}

int
_fmemicmp(const void __far *a, const void __far *b, size_t n)
{
    const unsigned char __far *p = a, *q = b;
    for (; n; n--, p++, q++) {
        int x = tolower(*p), y = tolower(*q);
        if (x != y)
            return x - y;
    }
    return 0;
}

size_t
_fstrlen(const char __far *s)
{
    const char __far *p = s;
    while (*p)
        p++;
    return (uint16_t)(OFF(p) - OFF(s));
}

char __far *
_fstrcpy(char __far *d, const char __far *s)
{
    return _fmemcpy(d, s, _fstrlen(s) + 1);
}

char __far *
_fstrcat(char __far *d, const char __far *s)
{
    _fstrcpy(d + _fstrlen(d), s);
    return d;
}

char __far *
_fstrncpy(char __far *d, const char __far *s, size_t n)
{
    size_t i = 0;
    for (; i < n && s[i]; i++)
        d[i] = s[i];
    for (; i < n; i++)
        d[i] = '\0';
    return d;
}

char __far *
_fstrncat(char __far *d, const char __far *s, size_t n)
{
    char __far *p = d + _fstrlen(d);
    while (n-- && *s)
        *p++ = *s++;
    *p = '\0';
    return d;
}

int
_fstrcmp(const char __far *a, const char __far *b)
{
    for (; *a && *a == *b; a++, b++)
        ;
    return (unsigned char)*a - (unsigned char)*b;
}

int
_fstrncmp(const char __far *a, const char __far *b, size_t n)
{
    for (; n; n--, a++, b++)
        if (*a != *b || !*a)
            return (unsigned char)*a - (unsigned char)*b;
    return 0;
}

int
_fstricmp(const char __far *a, const char __far *b)
{
    for (;; a++, b++) {
        int x = tolower((unsigned char)*a), y = tolower((unsigned char)*b);
        if (x != y || !x)
            return x - y;
    }
}

int
_fstrnicmp(const char __far *a, const char __far *b, size_t n)
{
    for (; n; n--, a++, b++) {
        int x = tolower((unsigned char)*a), y = tolower((unsigned char)*b);
        if (x != y || !x)
            return x - y;
    }
    return 0;
}

char __far *
_fstrchr(const char __far *s, int c)
{
    for (;; s++) {
        if (*s == (char)c)
            return (char __far *)s;
        if (!*s)
            return NULL;
    }
}

char __far *
_fstrrchr(const char __far *s, int c)
{
    const char __far *last = NULL;
    for (;; s++) {
        if (*s == (char)c)
            last = s;
        if (!*s)
            return (char __far *)last;
    }
}

size_t
_fstrspn(const char __far *s, const char __far *set)
{
    size_t n = 0;
    for (; s[n] && _fstrchr(set, s[n]); n++)
        ;
    return n;
}

size_t
_fstrcspn(const char __far *s, const char __far *set)
{
    size_t n = 0;
    for (; s[n] && !_fstrchr(set, s[n]); n++)
        ;
    return n;
}

char __far *
_fstrpbrk(const char __far *s, const char __far *set)
{
    s += _fstrcspn(s, set);
    return *s ? (char __far *)s : NULL;
}

char __far *
_fstrstr(const char __far *s, const char __far *find)
{
    size_t n = _fstrlen(find);
    for (; *s; s++)
        if (*s == *find && !_fstrncmp(s, find, n))
            return (char __far *)s;
    return n ? NULL : (char __far *)s;
}

char __far *
_fstrtok(char __far *s, const char __far *delim)
{
    static char __far *next;
    if (!s)
        s = next;
    if (!s)
        return NULL;
    s += _fstrspn(s, delim);
    if (!*s) {
        next = NULL;
        return NULL;
    }
    char __far *end = s + _fstrcspn(s, delim);
    if (*end)
        *end++ = '\0';
    else
        end = NULL;
    next = end;
    return s;
}

char __far *
_fstrdup(const char __far *s)
{
    size_t      n = _fstrlen(s) + 1;
    char __far *d = _fmalloc(n);
    return d ? _fmemcpy(d, s, n) : NULL;
}

char __far *
_fstrlwr(char __far *s)
{
    for (char __far *p = s; *p; p++)
        *p = tolower((unsigned char)*p);
    return s;
}

char __far *
_fstrupr(char __far *s)
{
    for (char __far *p = s; *p; p++)
        *p = toupper((unsigned char)*p);
    return s;
}

char __far *
_fstrrev(char __far *s)
{
    size_t n = _fstrlen(s);
    for (size_t i = 0; i < n / 2; i++) {
        char t = s[i];
        s[i] = s[n - 1 - i];
        s[n - 1 - i] = t;
    }
    return s;
}

char __far *
_fstrset(char __far *s, int c)
{
    for (char __far *p = s; *p; p++)
        *p = (char)c;
    return s;
}

char __far *
_fstrnset(char __far *s, int c, size_t n)
{
    for (char __far *p = s; n && *p; p++, n--)
        *p = (char)c;
    return s;
}
