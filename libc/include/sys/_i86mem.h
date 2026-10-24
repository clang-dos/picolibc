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
 * Helpers for the near and far functions of DOS compilers (Open Watcom,
 * Microsoft C) on the i8086: _fmemcpy, _fstrlen, _fmalloc, _nmalloc and
 * the like.
 *
 * Where a near or far pointer does not convert to the model's default
 * pointer, the standard functions get overloads for it, so that memcpy,
 * strlen, free and the rest call the right one for the pointers they are
 * given: far pointers in the tiny, small and medium models, and near
 * pointers to the near heap (free, realloc and _msize) in the others.
 * In C they use clang's overloadable attribute; in C++, overloads.
 */

#ifndef _SYS__I86MEM_H_
#define _SYS__I86MEM_H_

#if defined(__I86__) && !defined(__I86_NEAR_DATA__)
#if defined(__TINY__) || defined(__SMALL__) || defined(__MEDIUM__)
#define __I86_NEAR_DATA__ 1
#endif
#endif

/* __I86_OVERLOAD(ret, name, target, params, args, cond): declares name as
   an overload that calls target (name is in parentheses, as string.h may
   define it as a macro). cond, if not empty, says when the
   overload may be chosen; it keeps a null pointer constant, which converts
   to any pointer, on the plain function. */
#if defined(__I86__) && !defined(__I86_NO_OVERLOADS)
#ifdef __cplusplus
#define __I86_OVERLOAD(ret, name, target, params, args, cond)       \
    extern "C++" {                                                  \
    inline ret(name) params                                         \
        __attribute__((__always_inline__, cond))                    \
    { return target args; }                                         \
    }
#else
#define __I86_OVERLOAD(ret, name, target, params, args, cond)       \
    ret(name) params __attribute__((__overloadable__, cond))        \
        __asm__(__XSTRING(__USER_LABEL_PREFIX__) __XSTRING(target));
#endif
/* An attribute that does nothing, for overloads without a condition. */
#define __I86_ANY __unused__
/* For a pointer parameter p: not a null pointer constant. */
#define __I86_NOT_NULL(p) __enable_if__(!__builtin_constant_p(p) || (p), "")
#endif

#endif /* _SYS__I86MEM_H_ */
