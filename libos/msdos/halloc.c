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
 * The memory comes from __msdos_alloc_paras (dosmem.c).
 */

#if !defined(__WINDOWS__) && !defined(__TINY__)

#include "local-msdos.h"
#include <malloc.h>

void __huge *
halloc(long n, size_t size)
{
    if (n <= 0 || size == 0 || (unsigned long)n > 0xfffffUL / size) {
        errno = n < 0 ? EINVAL : ENOMEM;
        return NULL;
    }
    uint32_t bytes = (uint32_t)n * size;
    uint16_t paras = (uint16_t)((bytes + 15) >> 4);
    uint16_t seg = __msdos_alloc_paras(paras);
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
