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
#include <stdlib.h>

/*
 * Constructors and destructors. For MinGW targets clang puts pointers to
 * them in .ctors and .dtors, and main() starts with a call to __main().
 * lld puts __CTOR_LIST__ and __DTOR_LIST__ around those sections: a -1,
 * the pointers, and a 0. Constructors run from the last to the first, as
 * with GCC, and destructors from the first to the last, at exit.
 */
extern void (*__CTOR_LIST__[])(void);
extern void (*__DTOR_LIST__[])(void);

void        __main(void);
void        __do_global_ctors(void);
void        __do_global_dtors(void);

void
__do_global_dtors(void)
{
    static void (**p)(void) = __DTOR_LIST__ + 1;
    while (*p) {
        void (*fn)(void) = *p++;
        fn();
    }
}

void
__do_global_ctors(void)
{
    size_t n = 0;
    while (__CTOR_LIST__[n + 1])
        n++;
    for (size_t i = n; i >= 1; i--)
        __CTOR_LIST__[i]();
    atexit(__do_global_dtors);
}

void
__main(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    __do_global_ctors();
}
