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
 * The near and far heaps of DOS compilers on the i8086. _nmalloc and the
 * other _n functions use DGROUP; the _f functions use far memory. In the
 * tiny, small and medium models _nmalloc is malloc; in the others
 * _fmalloc is.
 *
 * free, realloc and _msize take a pointer of either kind and use its
 * heap.
 */

#ifndef _SYS__I86MALLOC_H_
#define _SYS__I86MALLOC_H_

#include <sys/cdefs.h>
#include <sys/_i86mem.h>
#define __need_size_t
#include <stddef.h>

_BEGIN_STD_C

void __near *_nmalloc(size_t) __picolibc_export;
void __near *_ncalloc(size_t, size_t) __picolibc_export;
void __near *_nrealloc(void __near *, size_t) __picolibc_export;
void         _nfree(void __near *) __picolibc_export;
size_t       _nmsize(void __near *) __picolibc_export;
void __far  *_fmalloc(size_t) __picolibc_export;
void __far  *_fcalloc(size_t, size_t) __picolibc_export;
void __far  *_frealloc(void __far *, size_t) __picolibc_export;
void         _ffree(void __far *) __picolibc_export;
size_t       _fmsize(void __far *) __picolibc_export;
/* The size of a block from malloc. */
size_t       _msize(void *) __picolibc_export;

#if !defined(__I86_NEAR_DATA__) && !defined(__WINDOWS__)
/* The near heap of the far data models on DOS is an array in DGROUP, of
   4 KiB unless one file of the program has _NEAR_HEAP_SIZE(bytes). */
#define _NEAR_HEAP_SIZE(__n)                    \
    char __near __msdos_near_heap[(__n)];       \
    const size_t __msdos_near_heap_size = (__n)
#endif

#ifdef __I86_OVERLOAD
/* The pointers that do not convert to the default ones. */
#ifdef __I86_NEAR_DATA__
#define __I86_HEAP __far
#define __I86_HEAP_FREE _ffree
#define __I86_HEAP_REALLOC _frealloc
#define __I86_HEAP_MSIZE _fmsize
#else
#define __I86_HEAP __near
#define __I86_HEAP_FREE _nfree
#define __I86_HEAP_REALLOC _nrealloc
#define __I86_HEAP_MSIZE _nmsize
#endif
__I86_OVERLOAD(void, free, __I86_HEAP_FREE, (void __I86_HEAP *__p), (__p),
               __I86_NOT_NULL(__p))
__I86_OVERLOAD(void __I86_HEAP *, realloc, __I86_HEAP_REALLOC,
               (void __I86_HEAP *__p, size_t __n), (__p, __n),
               __I86_NOT_NULL(__p))
__I86_OVERLOAD(size_t, _msize, __I86_HEAP_MSIZE, (void __I86_HEAP *__p),
               (__p), __I86_NOT_NULL(__p))
#undef __I86_HEAP
#undef __I86_HEAP_FREE
#undef __I86_HEAP_REALLOC
#undef __I86_HEAP_MSIZE
#endif

_END_STD_C

#endif /* _SYS__I86MALLOC_H_ */
