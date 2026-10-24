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

/* 16-bit Windows: the task's handles, the heap, and the start of a DLL. */
#include "local-msdos.h"
#include <stdint.h>
#include <sys/_initfini.h>

#ifdef __WINDOWS__
uint16_t __win16_hinstance;
uint16_t __win16_hprevinstance;
uint16_t __win16_cmdshow;
char __win16_cmdline[128];

/* The heap is one block from Windows that sbrk hands out: a fixed block of
 * the local heap in DGROUP with near data pointers, or a 64 KiB global
 * segment with far ones. Windows grows DGROUP for the local block as far as
 * it can; some of the local heap stays free for Windows itself (edit
 * controls keep their text there). */
void
__win16_init_heap(void)
{
    char *base = NULL;
    uint32_t size;
#if defined(__COMPACT__) || defined(__LARGE__)
    for (size = 0xfff0; size >= 0x1000 && !base; size >>= 1) {
        uint16_t h = GlobalAlloc(0 /* GMEM_FIXED */, size);
        if (h)
            base = GlobalLock(h);
    }
#else
    for (size = 0x8000; size >= 0x400 && !base; size >>= 1)
        base = (char *)(uintptr_t)LocalAlloc(0 /* LMEM_FIXED */, size);
#endif
    if (!base)
        size = 0;
    else
        size <<= 1;
    __msdos_heap_base = __msdos_brk = base;
    __msdos_heap_end = base + size;
}

uint16_t __win16_heapsize;
char __far *__win16_libcmdline;

int __WIN16_API LibMain(uint16_t instance, uint16_t data_seg,
                        uint16_t heap_size, char __far *cmdline);

/* Called by libentry.obj in a DLL, with BSS cleared and the local heap set
 * up. The stack is the caller's, so SS is not DS here. */
int
__win16_libstart(void)
{
    uint16_t ds;
    __asm__("movw %%ds, %0" : "=r"(ds));
    __win16_init_heap();
#ifdef __INIT_FINI_ARRAY
    __libc_init_array();
#endif
    return LibMain(__win16_hinstance, ds, __win16_heapsize,
                   __win16_libcmdline);
}
#endif
