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
 * The near heap: _nmalloc, _ncalloc, _nrealloc, _nfree and _nmsize, as in
 * Open Watcom and Microsoft C. With near data (tiny, small and
 * medium) it is the heap of malloc. With far data it is in DGROUP: on DOS
 * an arena (arena.h) in __msdos_near_heap, whose size the program can set
 * with _NEAR_HEAP_SIZE (nearheapsize.c has the default, 4 KiB); on 16-bit
 * Windows the local heap (LocalAlloc).
 */

#include "local-msdos.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#ifdef __I86_NEAR_DATA__

void __near *
_nmalloc(size_t n)
{
    return malloc(n);
}

void
_nfree(void __near *p)
{
    free(p);
}

size_t
_nmsize(void __near *p)
{
    return malloc_usable_size(p);
}

void __near *
_nrealloc(void __near *p, size_t n)
{
    return realloc(p, n);
}

#elif defined(__WINDOWS__)

/* LMEM_FIXED blocks: the handle is the near pointer. */

void __near *
_nmalloc(size_t n)
{
    if (n > 0xffe0u) {
        errno = ENOMEM;
        return NULL;
    }
    uint16_t h = LocalAlloc(0 /* LMEM_FIXED */, n);
    if (!h)
        errno = ENOMEM;
    return (void __near *)h;
}

void
_nfree(void __near *p)
{
    if (p)
        LocalFree((uint16_t)p);
}

size_t
_nmsize(void __near *p)
{
    return LocalSize((uint16_t)p);
}

void __near *
_nrealloc(void __near *p, size_t n)
{
    if (!p)
        return _nmalloc(n);
    if (!n) {
        _nfree(p);
        return NULL;
    }
    uint16_t h = n > 0xffe0u ? 0
                             : LocalReAlloc((uint16_t)p, n,
                                            2 /* LMEM_MOVEABLE */);
    if (!h)
        errno = ENOMEM;
    return (void __near *)h;
}

#else

#define ARENA_Q __near
#include "arena.h"

extern char __near __msdos_near_heap[];
extern const size_t __msdos_near_heap_size;

static char __near *
heap(void)
{
    static char __near *arena;
    if (!arena && __msdos_near_heap_size >= 4) {
        arena = __msdos_near_heap;
        arena_init(arena, __msdos_near_heap_size & ~1u);
    }
    return arena;
}

void __near *
_nmalloc(size_t n)
{
    uint16_t need = arena_need(n);
    char __near *a = heap();
    void __near *p = need && a ? arena_alloc(a, need) : NULL;
    if (!p)
        errno = ENOMEM;
    return p;
}

void
_nfree(void __near *p)
{
    if (p)
        arena_free(p);
}

size_t
_nmsize(void __near *p)
{
    return arena_size(p);
}

void __near *
_nrealloc(void __near *p, size_t n)
{
    if (!p)
        return _nmalloc(n);
    if (!n) {
        _nfree(p);
        return NULL;
    }
    uint16_t need = arena_need(n);
    if (need && arena_resize(p, need))
        return p;
    void __near *q = _nmalloc(n);
    if (q) {
        size_t old = arena_size(p);
        for (size_t i = 0; i < old; i++)
            ((char __near *)q)[i] = ((char __near *)p)[i];
        _nfree(p);
    }
    return q;
}

#endif

void __near *
_ncalloc(size_t n, size_t size)
{
    if (size && n > (size_t)-1 / size) {
        errno = ENOMEM;
        return NULL;
    }
    size_t       bytes = n * size;
    char __near *p = _nmalloc(bytes);
    if (p)
        for (size_t i = 0; i < bytes; i++)
            p[i] = 0;
    return p;
}
