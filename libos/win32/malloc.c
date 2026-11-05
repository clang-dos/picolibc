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

#include "local-win32.h"
#include <malloc.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * malloc takes its blocks from the process heap (HeapAlloc), which is
 * thread-safe and gives memory back to Windows. Each block starts with a
 * header that holds the distance from the start of the HeapAlloc block, so
 * that free() takes blocks from memalign and aligned_alloc too.
 */
#define HDR       (alignof(max_align_t) > sizeof(size_t) ? alignof(max_align_t) : sizeof(size_t))
#define OFFSET(p) (((size_t *)(p))[-1])

__win32_handle __win32_heap;

static __win32_handle
heap(void)
{
    if (!__win32_heap)
        __win32_heap = GetProcessHeap();
    return __win32_heap;
}

static void *
alloc(size_t size, size_t align, __win32_dword flags)
{
    if (align < HDR)
        align = HDR;
    if (size > (size_t)PTRDIFF_MAX - align - HDR) {
        errno = ENOMEM;
        return NULL;
    }
    size_t extra = align == HDR ? HDR : align + HDR;
    char  *base = HeapAlloc(heap(), flags, size + extra);
    if (!base) {
        errno = ENOMEM;
        return NULL;
    }
    uintptr_t user = ((uintptr_t)base + HDR + align - 1) & ~(uintptr_t)(align - 1);
    OFFSET(user) = user - (uintptr_t)base;
    return (void *)user;
}

void *
malloc(size_t size)
{
    return alloc(size, HDR, 0);
}

void *
calloc(size_t n, size_t size)
{
    if (size && n > SIZE_MAX / size) {
        errno = ENOMEM;
        return NULL;
    }
    return alloc(n * size, HDR, __WIN32_HEAP_ZERO_MEMORY);
}

void
free(void *p)
{
    if (p)
        HeapFree(heap(), 0, (char *)p - OFFSET(p));
}

size_t
malloc_usable_size(void *p)
{
    if (!p)
        return 0;
    size_t off = OFFSET(p);
    return HeapSize(heap(), 0, (char *)p - off) - off;
}

void *
realloc(void *p, size_t size)
{
    if (!p)
        return malloc(size);
    size_t off = OFFSET(p);
    if (off == HDR) {
        if (size > (size_t)PTRDIFF_MAX - HDR) {
            errno = ENOMEM;
            return NULL;
        }
        char *base = HeapReAlloc(heap(), 0, (char *)p - HDR, size + HDR);
        if (!base) {
            errno = ENOMEM;
            return NULL;
        }
        return base + HDR;
    }
    /* An aligned block moves to a plain one. */
    void *q = malloc(size);
    if (!q)
        return NULL;
    size_t old = malloc_usable_size(p);
    memcpy(q, p, old < size ? old : size);
    free(p);
    return q;
}

void *
memalign(size_t align, size_t size)
{
    if (align == 0 || (align & (align - 1))) {
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

/* The Microsoft names. _aligned_malloc takes its arguments the other way
 * round. */
void *
_aligned_malloc(size_t size, size_t align)
{
    return memalign(align, size);
}

void
_aligned_free(void *p)
{
    free(p);
}

size_t
_msize(void *p)
{
    return malloc_usable_size(p);
}
