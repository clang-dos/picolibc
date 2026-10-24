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
 * The far heap: _fmalloc, _fcalloc, _frealloc, _ffree and _fmsize, as in
 * Open Watcom and Microsoft C. With far data (compact, large and huge)
 * it is the heap of malloc. With near data (tiny, small and medium) it is
 * memory outside DGROUP: segments from DOS (__msdos_alloc_paras) or from
 * Windows (GlobalAlloc), of 8 KiB or more, each an arena (arena.h). A
 * block is at most 64 KiB less a few bytes; halloc gives larger ones.
 */

#include "local-msdos.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#ifdef __I86_NEAR_DATA__

#define ARENA_Q __far
#include "arena.h"

#define MK(seg, off) ((char __far *)(((uint32_t)(seg) << 16) | (off)))
#define SEG(p) ((uint16_t)((uint32_t)(const void __far *)(p) >> 16))

/* The size of a new segment for the heap. */
#define GROW 0x2000u

/* The first segment. Each one starts with the segment of the next, or 0;
 * its arena follows. */
static uint16_t first;

/* A new segment of size bytes (even), or 0. */
static uint16_t
new_segment(uint16_t size)
{
#ifdef __WINDOWS__
    uint16_t h = GlobalAlloc(0 /* GMEM_FIXED */, size);
    return h ? SEG(GlobalLock(h)) : 0;
#else
    return __msdos_alloc_paras((uint16_t)(((uint32_t)size + 15) >> 4));
#endif
}

void __far *
_fmalloc(size_t n)
{
    uint16_t need = arena_need(n);
    if (!need) {
        errno = ENOMEM;
        return NULL;
    }
    for (uint16_t seg = first; seg; seg = ARENA_WORD(MK(seg, 0))) {
        void __far *q = arena_alloc(MK(seg, 2), need);
        if (q)
            return q;
    }
    /* The link word, the block and the end of the arena. */
    uint16_t size = need + 4;
    uint16_t seg = 0;
    if (size < GROW)
        seg = new_segment(GROW);
    if (seg)
        size = GROW;
    else
        seg = new_segment(size);
    if (!seg) {
        errno = ENOMEM;
        return NULL;
    }
    char __far *base = MK(seg, 0);
    ARENA_WORD(base) = first;
    first = seg;
    arena_init(base + 2, size - 2);
    return arena_alloc(base + 2, need);
}

void
_ffree(void __far *p)
{
    if (p)
        arena_free(p);
}

size_t
_fmsize(void __far *p)
{
    return arena_size(p);
}

void __far *
_frealloc(void __far *p, size_t n)
{
    if (!p)
        return _fmalloc(n);
    if (!n) {
        _ffree(p);
        return NULL;
    }
    uint16_t need = arena_need(n);
    if (!need) {
        errno = ENOMEM;
        return NULL;
    }
    if (arena_resize(p, need))
        return p;
    void __far *q = _fmalloc(n);
    if (q) {
        _fmemcpy(q, p, arena_size(p));
        _ffree(p);
    }
    return q;
}

#else

/* With far data, the far heap is the heap of malloc. */

void __far *
_fmalloc(size_t n)
{
    return malloc(n);
}

void
_ffree(void __far *p)
{
    free((void *)p);
}

size_t
_fmsize(void __far *p)
{
    return malloc_usable_size((void *)p);
}

void __far *
_frealloc(void __far *p, size_t n)
{
    return realloc((void *)p, n);
}

#endif

void __far *
_fcalloc(size_t n, size_t size)
{
    if (size && n > (size_t)-1 / size) {
        errno = ENOMEM;
        return NULL;
    }
    void __far *p = _fmalloc(n * size);
    return p ? _fmemset(p, 0, n * size) : NULL;
}
