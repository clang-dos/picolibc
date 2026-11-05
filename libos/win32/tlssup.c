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

/*
 * Thread-local storage. Windows sets aside a copy of the .tls section for
 * each thread, as the TLS directory, which the linker finds by the name
 * _tls_used, describes. clang reaches a thread's variables through
 * _tls_index, so a program with __thread variables (errno is one) brings
 * this in. The callbacks between __xl_a and __xl_z run when a thread
 * starts or ends; the linker sorts .CRT$XL* sections by name.
 */

typedef void(__attribute__((__stdcall__)) * __win32_tls_callback)(void *module,
                                                                  __win32_dword reason,
                                                                  void *reserved);

unsigned long _tls_index = 0;

char _tls_start __attribute__((__section__(".tls"), __used__)) = 0;
char _tls_end __attribute__((__section__(".tls$ZZZ"), __used__)) = 0;

__win32_tls_callback __xl_a __attribute__((__section__(".CRT$XLA"), __used__)) = 0;
__win32_tls_callback __xl_z __attribute__((__section__(".CRT$XLZ"), __used__)) = 0;

struct tls_directory {
    uintptr_t     start, end, index, callbacks;
    __win32_dword zero_fill, characteristics;
};

const struct tls_directory _tls_used __attribute__((__section__(".rdata$T"), __used__)) = {
    (uintptr_t)&_tls_start,
    (uintptr_t)&_tls_end,
    (uintptr_t)&_tls_index,
    (uintptr_t)(&__xl_a + 1),
    0,
    0,
};
