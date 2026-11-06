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

#include "local-os2.h"
#include <stdint.h>
#include <unistd.h>

/*
 * picolibc's malloc grows the heap with sbrk and needs it in one piece.
 *
 * 2.x: the heap is a range of address space reserved with DosAllocMem the
 * first time it is needed, and committed as it grows.
 *
 * 1.x: in the near-data models (small, medium) the heap is the rest of
 * DGROUP, which the startup code grows to 64 KiB. In the far-data models
 * (compact, large) it is a 64 KiB segment of its own from DosAllocSeg.
 */

char *__os2_heap_base;
char *__os2_brk;
char *__os2_heap_end;

#ifdef __i386__
/* The reserved address space; 2.x programs have 512 MiB in all. */
static const unsigned long reserve[] = { 256ul << 20, 64ul << 20, 16ul << 20, 4ul << 20 };
static char               *committed;

#define PAG_READ    0x01
#define PAG_WRITE   0x02
#define PAG_COMMIT  0x10
#define PAG_DEFAULT 0x400
#define CHUNK       65536ul

static int
init_heap(void)
{
    void *base;
    for (unsigned i = 0; i < sizeof(reserve) / sizeof(reserve[0]); i++)
        if (DosAllocMem(&base, reserve[i], PAG_READ | PAG_WRITE) == 0) {
            __os2_heap_base = __os2_brk = committed = base;
            __os2_heap_end = (char *)base + reserve[i];
            return 0;
        }
    return -1;
}
#endif

void *
sbrk(ptrdiff_t incr)
{
#ifdef __i386__
    if (!__os2_heap_base && init_heap() < 0) {
        errno = ENOMEM;
        return (void *)-1;
    }
#endif
    char *old = __os2_brk;
    if (incr < 0 ? (size_t)-incr > (size_t)(old - __os2_heap_base)
                 : (size_t)incr > (size_t)(__os2_heap_end - old)) {
        errno = ENOMEM;
        return (void *)-1;
    }
#ifdef __i386__
    char *want = old + incr;
    if (want > committed) {
        unsigned long size = ((unsigned long)(want - committed) + CHUNK - 1) & ~(CHUNK - 1);
        if ((unsigned long)(__os2_heap_end - committed) < size)
            size = (unsigned long)(__os2_heap_end - committed);
        if (DosSetMem(committed, size, PAG_COMMIT | PAG_DEFAULT)) {
            errno = ENOMEM;
            return (void *)-1;
        }
        committed += size;
    }
#endif
    __os2_brk = old + incr;
    return old;
}
