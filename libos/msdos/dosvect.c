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


/* Interrupt vectors and resident programs: _dos_getvect, _dos_setvect and
 * _dos_keep, as in Open Watcom and Microsoft C. */

#include "local-msdos.h"
#include <dos.h>

#ifndef __WINDOWS__

typedef void (__interrupt __far *intr_handler)();

intr_handler
_dos_getvect(unsigned intno)
{
    struct __msdos_regs r = { .ax = 0x3500 | (intno & 0xff) };
    __msdos_int21(&r);
    return (intr_handler)(((uint32_t)r.es << 16) | r.bx);
}

void
_dos_setvect(unsigned intno, intr_handler handler)
{
    /* DS:DX is the handler, whose segment may be 0 (an unused vector):
     * __msdos_int21 would take a DS of 0 as DGROUP. */
    uint32_t addr = (uint32_t)handler;
    __asm__ volatile("pushw %%ds\n\t"
                     "movw %%bx, %%ds\n\t"
                     "int $0x21\n\t"
                     "popw %%ds"
                     :
                     : "a"(0x2500 | (intno & 0xff)), "d"((uint16_t)addr),
                       "b"((uint16_t)(addr >> 16))
                     : "memory");
}

/* The paragraph after the end of the heap. */
static uint16_t
heap_end_para(void)
{
#if defined(__HUGE__)
    /* A huge pointer as an integer is its linear address. */
    return (uint16_t)(((uint32_t)__msdos_brk + 15) >> 4);
#elif defined(__COMPACT__) || defined(__LARGE__)
    uint32_t brk = (uint32_t)__msdos_brk;
    return (uint16_t)((brk >> 16) + (((brk & 0xffff) + 15) >> 4));
#else
    uint16_t ds;
    __asm__("movw %%ds, %0" : "=r"(ds));
    return ds + (uint16_t)(((uint32_t)(uint16_t)__msdos_brk + 15) >> 4);
#endif
}

void
_dos_keep(unsigned retcode, unsigned memsize)
{
    if (memsize == 0)
        memsize = heap_end_para() - __msdos_psp;
    struct __msdos_regs r = { .ax = 0x3100 | (retcode & 0xff),
                              .dx = memsize };
    __msdos_int21(&r);
    for (;;)
        ;
}

#endif /* !__WINDOWS__ */
