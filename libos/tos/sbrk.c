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

#include "local-tos.h"
#include <stdint.h>
#include <unistd.h>

/*
 * The heap is one block from Malloc(), taken the first time malloc()
 * needs memory: _heapsize bytes if the program defines it (long
 * _heapsize = 100000;), or else all of the largest free block but 64K,
 * which is left for GEMDOS and for programs this one starts. picolibc's
 * malloc needs its memory in one piece, and TOS before 1.4 can only track
 * a few dozen Malloc() blocks anyway.
 */
extern long  _heapsize __attribute__((weak));

static char *heap_base, *heap_brk, *heap_end;

#define RESERVE  65536L
#define MIN_HEAP 16384L

void *
sbrk(ptrdiff_t incr)
{
    if (!heap_base) {
        long size = (&_heapsize && _heapsize > 0) ? _heapsize : 0;
        long avail = __tos_Malloc(-1L);
        if (size <= 0) {
            size = avail - RESERVE;
            if (size < MIN_HEAP)
                size = avail;
        }
        if (size > avail)
            size = avail;
        size &= ~7L;
        long p = size > 0 ? __tos_Malloc(size) : 0;
        if (p <= 0) {
            errno = ENOMEM;
            return (void *)-1;
        }
        heap_base = heap_brk = (char *)p;
        heap_end = heap_base + size;
    }
    char *old = heap_brk;
    if (incr < 0 ? -incr > heap_brk - heap_base : incr > heap_end - heap_brk) {
        errno = ENOMEM;
        return (void *)-1;
    }
    heap_brk += incr;
    return old;
}
