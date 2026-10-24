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

/* Frames in DGROUP for the locals of 16-bit Windows DLL code.
 *
 * A DLL runs on the stack of the task that calls it, so SS is not DS, and a
 * near pointer (which means DS) cannot point to a local. Code compiled with
 * -mwindows-dll in the small and medium models keeps the locals whose
 * address escapes in a frame in DGROUP instead, below a pointer that this
 * function finds. Each task that calls the DLL gets a region of its own,
 * found by its stack segment, as one task may yield inside the DLL while
 * another calls it. A region whose frames have all returned goes to the
 * next task that needs one. */
#include "local-msdos.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __WINDOWS__

#define NREGIONS 8

struct region {
    uint16_t ss;
    char *base; /* NULL if not allocated */
    char *top;
    char *ptr;  /* the frame pointer of the code, at top when idle */
};

static struct region regions[NREGIONS];
static struct region *last;

extern uint16_t __win16_dll_frame_size;

static void
fail(void)
{
    static const char msg[] = "DLL frame regions exhausted\r\n";
    write(2, msg, sizeof(msg) - 1);
    abort();
}

char **
__win16_frame_pointer_address(void)
{
    uint16_t ss;
    __asm__("movw %%ss, %0" : "=r"(ss));
    if (last && last->ss == ss)
        return &last->ptr;

    struct region *free_region = NULL;
    for (struct region *r = regions; r < regions + NREGIONS; r++) {
        if (r->base && r->ss == ss) {
            last = r;
            return &r->ptr;
        }
        if (!free_region && (!r->base || r->ptr == r->top))
            free_region = r;
    }
    if (!free_region)
        fail();
    if (!free_region->base) {
        free_region->base = malloc(__win16_dll_frame_size);
        if (!free_region->base)
            fail();
        /* Frames grow down from the top, which stays 2-byte aligned. */
        free_region->top = free_region->base + (__win16_dll_frame_size & ~1u);
        free_region->ptr = free_region->top;
    }
    free_region->ss = ss;
    last = free_region;
    return &free_region->ptr;
}

#endif
