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
 * malloc and friends on AllocMem(). An Amiga program has no heap of its
 * own: every block comes from the system, and the blocks still allocated
 * when the program ends go back to it in _exit. Each block starts with a
 * header that links it into the list of blocks.
 */

#include "local-amigaos.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

struct block {
    struct block *next, *prev;
    unsigned long size; /* bytes from AllocMem */
    char         *base; /* the address AllocMem returned */
};

static struct block *blocks;

static void *
alloc(size_t size, size_t align, unsigned long flags)
{
    if (align < 8)
        align = 8;
    unsigned long total = size + sizeof(struct block) + align - 8;
    if (size > 0x7fffffff - sizeof(struct block) - align) {
        errno = ENOMEM;
        return NULL;
    }
    char *base = __amiga_AllocMem(total, flags);
    if (!base) {
        errno = ENOMEM;
        return NULL;
    }
    /* AllocMem returns 8-byte aligned memory, and the header is 16 bytes. */
    uintptr_t user = ((uintptr_t)base + sizeof(struct block) + align - 1) & ~(uintptr_t)(align - 1);
    struct block *b = (struct block *)user - 1;
    b->size = total;
    b->base = base;
    b->prev = NULL;
    b->next = blocks;
    if (blocks)
        blocks->prev = b;
    blocks = b;
    return (void *)user;
}

void *
malloc(size_t size)
{
    return alloc(size, 8, 0);
}

void *
calloc(size_t n, size_t size)
{
    if (size && n > SIZE_MAX / size) {
        errno = ENOMEM;
        return NULL;
    }
    return alloc(n * size, 8, __AMIGA_MEMF_CLEAR);
}

void
free(void *p)
{
    if (!p)
        return;
    struct block *b = (struct block *)p - 1;
    if (b->prev)
        b->prev->next = b->next;
    else
        blocks = b->next;
    if (b->next)
        b->next->prev = b->prev;
    __amiga_FreeMem(b->base, b->size);
}

size_t
malloc_usable_size(void *p)
{
    if (!p)
        return 0;
    struct block *b = (struct block *)p - 1;
    return b->size - ((char *)p - b->base);
}

void *
realloc(void *p, size_t size)
{
    if (!p)
        return malloc(size);
    if (size == 0) {
        free(p);
        return NULL;
    }
    size_t old = malloc_usable_size(p);
    if (size <= old && size >= old / 2)
        return p;
    void *n = malloc(size);
    if (!n)
        return NULL;
    memcpy(n, p, old < size ? old : size);
    free(p);
    return n;
}

void *
memalign(size_t align, size_t size)
{
    if (align & (align - 1)) {
        errno = EINVAL;
        return NULL;
    }
    return alloc(size, align, 0);
}

void *
aligned_alloc(size_t align, size_t size)
{
    return memalign(align, size);
}

int
posix_memalign(void **out, size_t align, size_t size)
{
    if (align < sizeof(void *) || (align & (align - 1)))
        return EINVAL;
    void *p = alloc(size, align, 0);
    if (!p)
        return ENOMEM;
    *out = p;
    return 0;
}

void *
valloc(size_t size)
{
    return memalign(4096, size);
}

void *
pvalloc(size_t size)
{
    return memalign(4096, (size + 4095) & ~(size_t)4095);
}

void
__amiga_free_all(void)
{
    while (blocks) {
        struct block *b = blocks;
        blocks = b->next;
        __amiga_FreeMem(b->base, b->size);
    }
}
