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
 * halloc and hfree, as in Open Watcom and Microsoft C: blocks of memory
 * that can be larger than 64 KiB, reached through huge pointers, in every
 * DOS model but tiny. They come straight from DOS (INT 21h functions 48h
 * and 49h), outside the heap of malloc.
 *
 * DOS gives an .EXE program all of its memory, so the first halloc that
 * finds no free DOS memory shrinks the program's memory block: down to the
 * end of the heap in the models where the heap has a fixed size, or by the
 * size of the block in the huge models, whose heap can use the rest of
 * memory and then ends below it. Blocks are allocated last fit, at the top
 * of memory, away from the program.
 */

#if !defined(__WINDOWS__) && !defined(__TINY__)

#include "local-msdos.h"
#include <malloc.h>

/* The paragraph after the program's memory block; 0 until known. */
static uint16_t mem_top;

/* The linear address of a data pointer. */
static uint32_t
linear(const char *p)
{
#if defined(__HUGE__)
    return (uint32_t)p;
#elif defined(__COMPACT__) || defined(__LARGE__)
    return ((uint32_t)__MSDOS_SEG(p) << 4) + __MSDOS_OFF(p);
#else
    uint16_t ds;
    __asm__("movw %%ds, %0" : "=r"(ds));
    return ((uint32_t)ds << 4) + __MSDOS_OFF(p);
#endif
}

/* DOS function 48h, with the last fit strategy. Returns the segment, or 0. */
static uint16_t
dos_alloc(uint16_t paras)
{
    struct __msdos_regs r = { .ax = 0x5800 };
    __msdos_int21(&r);
    uint16_t old = r.ax;
    r = (struct __msdos_regs){ .ax = 0x5801, .bx = 2 };
    __msdos_int21(&r);
    r = (struct __msdos_regs){ .ax = 0x4800, .bx = paras };
    uint16_t seg = __msdos_int21(&r) ? 0 : r.ax;
    r = (struct __msdos_regs){ .ax = 0x5801, .bx = old };
    __msdos_int21(&r);
    return seg;
}

/* Gives memory at the end of the program's block back to DOS, keeping
 * what the heap uses, so that \p paras paragraphs may become free. */
static int
shrink_program(uint16_t paras)
{
    if (!mem_top) {
        uint16_t top;
        __msdos_fromfar(&top, __msdos_psp, 2, 2);
        mem_top = top;
    }
#ifdef __HUGE__
    /* The heap may grow up to its end: keep what it already has. */
    uint32_t used = linear(__msdos_brk);
#else
    uint32_t used = linear(__msdos_heap_end);
#endif
    uint16_t low = (uint16_t)((used + 15) >> 4);
    if (mem_top <= low)
        return 0;
    uint16_t top = low;
#ifdef __HUGE__
    /* The block and the paragraph of its DOS memory control block. */
    if (mem_top - low > paras + 1u)
        top = mem_top - paras - 1;
#endif
    struct __msdos_regs r = { .ax = 0x4a00, .bx = top - __msdos_psp,
                              .es = __msdos_psp };
    if (__msdos_int21(&r))
        return 0;
    mem_top = top;
#ifdef __HUGE__
    char *end = (char *)((uint32_t)top << 4);
    if (__msdos_heap_end > end)
        __msdos_heap_end = end;
#endif
    return 1;
}

void __huge *
halloc(long n, size_t size)
{
    if (n <= 0 || size == 0 || (unsigned long)n > 0xfffffUL / size) {
        errno = n < 0 ? EINVAL : ENOMEM;
        return NULL;
    }
    uint32_t bytes = (uint32_t)n * size;
    uint16_t paras = (uint16_t)((bytes + 15) >> 4);
    uint16_t seg = dos_alloc(paras);
    if (!seg && shrink_program(paras))
        seg = dos_alloc(paras);
    if (!seg) {
        errno = ENOMEM;
        return NULL;
    }
    /* The block starts zeroed, as with calloc. */
    for (uint16_t s = seg, left = paras; left;) {
        uint16_t chunk = left > 0x800 ? 0x800 : left;
        uint16_t __far *w = (uint16_t __far *)((uint32_t)s << 16);
        for (uint16_t i = 0; i < chunk * 8u; i++)
            w[i] = 0;
        s += chunk;
        left -= chunk;
    }
    return (void __huge *)((uint32_t)seg << 4);
}

void
hfree(void __huge *p)
{
    uint32_t addr = (uint32_t)p;
    if (!p || (addr & 15))
        return;
    struct __msdos_regs r = { .ax = 0x4900, .es = (uint16_t)(addr >> 4) };
    __msdos_int21(&r);
}

#endif
