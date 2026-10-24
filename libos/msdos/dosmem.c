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
 * Memory from DOS, for halloc and the far heap: INT 21h function 48h, last
 * fit, at the top of memory, away from the program.
 *
 * DOS gives a program all of its memory, so the first request that finds
 * no free DOS memory shrinks the program's memory block: down to the end
 * of the heap in the models where the heap has a fixed size, to its 64 KiB
 * segment in the tiny model, or by the size of the request in the huge
 * models, whose heap can use the rest of memory and then ends below it.
 */

#ifndef __WINDOWS__

#include "local-msdos.h"

/* The paragraph after the program's memory block; 0 until known. */
static uint16_t mem_top;

#ifndef __TINY__
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
#endif

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
#if defined(__TINY__)
    /* The stack is at the top of the program's segment. */
    uint16_t low = __msdos_psp + 0x1000;
#else
#ifdef __HUGE__
    /* The heap may grow up to its end: keep what it already has. */
    uint32_t used = linear(__msdos_brk);
#else
    uint32_t used = linear(__msdos_heap_end);
#endif
    uint16_t low = (uint16_t)((used + 15) >> 4);
#endif
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

uint16_t
__msdos_alloc_paras(uint16_t paras)
{
    uint16_t seg = dos_alloc(paras);
    if (!seg && shrink_program(paras))
        seg = dos_alloc(paras);
    return seg;
}

#endif
