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
 * A simple heap in one segment, for the far heap of the near data models
 * and the near heap of the far data models. Include it with ARENA_Q
 * defined as the qualifier of the arena's pointers (__far or __near).
 *
 * An arena is a list of blocks ended by a 0 word. Each block starts with
 * a word: its size in bytes, that word included, which is even, plus 1
 * while it is in use. Free blocks next to each other are merged when an
 * allocation passes over them; allocation is first fit.
 */

#include <stdint.h>
#include <stddef.h>

#define ARENA_WORD(p) (*(uint16_t ARENA_Q *)(p))

/* The block size for n bytes, or 0 if it does not fit in a segment. */
static uint16_t
arena_need(size_t n)
{
    if (n > 0xffe0u)
        return 0;
    uint16_t need = ((uint16_t)n + 3u) & ~1u;
    return need < 4 ? 4 : need;
}

/* Makes an arena of size bytes (even, at least 4) at p. */
static void
arena_init(char ARENA_Q *p, uint16_t size)
{
    ARENA_WORD(p) = size - 2;
    ARENA_WORD(p + size - 2) = 0;
}

/* Merges the free blocks after the block at p into it. Returns its size. */
static uint16_t
arena_merge(char ARENA_Q *p)
{
    uint16_t h = ARENA_WORD(p), size = h & ~1u, next;
    while ((next = ARENA_WORD(p + size)) != 0 && !(next & 1))
        size += next;
    ARENA_WORD(p) = size | (h & 1);
    return size;
}

/* Marks the block at p, of size bytes, in use with need bytes, and
 * returns what is left as a free block. */
static void
arena_take(char ARENA_Q *p, uint16_t size, uint16_t need)
{
    if (size - need >= 4) {
        ARENA_WORD(p + need) = size - need;
        size = need;
    }
    ARENA_WORD(p) = size | 1;
}

/* A block of need bytes from the arena at p, or NULL. */
static void ARENA_Q *
arena_alloc(char ARENA_Q *p, uint16_t need)
{
    for (uint16_t h; (h = ARENA_WORD(p)) != 0; p += h & ~1u) {
        if (h & 1)
            continue;
        h = arena_merge(p);
        if (h >= need) {
            arena_take(p, h, need);
            return p + 2;
        }
    }
    return NULL;
}

static void
arena_free(void ARENA_Q *q)
{
    ARENA_WORD((char ARENA_Q *)q - 2) &= ~1u;
}

/* The bytes a block can hold. */
static size_t
arena_size(void ARENA_Q *q)
{
    return (ARENA_WORD((char ARENA_Q *)q - 2) & ~1u) - 2;
}

/* Resizes the block q to need bytes where it is. Returns 0 if the free
 * space after it is too small. */
static int
arena_resize(void ARENA_Q *q, uint16_t need)
{
    char ARENA_Q *p = (char ARENA_Q *)q - 2;
    uint16_t size = arena_merge(p);
    if (size < need)
        return 0;
    arena_take(p, size, need);
    return 1;
}
