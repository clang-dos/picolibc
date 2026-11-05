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
#include <string.h>
#include <unistd.h>

void
_exit(int status)
{
    ExitProcess((unsigned int)status);
}

pid_t
getpid(void)
{
    return (pid_t)GetCurrentProcessId();
}

int _getpid(void) __attribute__((__alias__("getpid")));

typedef unsigned char(__attribute__((__stdcall__)) * rtl_gen_random_t)(void *, unsigned long);

static uint64_t
mix(uint64_t x)
{
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

/*
 * getentropy, which seeds arc4random. Windows XP and later have
 * RtlGenRandom (SystemFunction036 in advapi32). Older Windows get timing
 * noise instead: the performance counter read around short sleeps, the
 * clock, the tick count and the process and thread ids.
 */
int
getentropy(void *buf, size_t len)
{
    if (len > 256) {
        errno = EIO;
        return -1;
    }
    static rtl_gen_random_t gen;
    static int              looked;
    if (!looked) {
        __win32_handle advapi = LoadLibraryA("advapi32.dll");
        if (advapi)
            gen = (rtl_gen_random_t)GetProcAddress(advapi, "SystemFunction036");
        looked = 1;
    }
    if (gen && gen(buf, (unsigned long)len))
        return 0;

    static uint64_t pool;
    uint64_t        state = pool ^ mix((uint64_t)__win32_now());
    state = mix(state ^ GetCurrentProcessId() ^ ((uint64_t)GetCurrentThreadId() << 32));
    state = mix(state ^ (uintptr_t)&state ^ GetTickCount());
    unsigned char *out = buf;
    for (size_t i = 0; i < len; i += 8) {
        for (int j = 0; j < 16; j++) {
            int64_t count = 0;
            QueryPerformanceCounter(&count);
            state = mix(state ^ (uint64_t)count);
            if ((j & 7) == 7)
                Sleep(0);
        }
        uint64_t v = mix(state + i);
        size_t   n = len - i < 8 ? len - i : 8;
        memcpy(out + i, &v, n);
    }
    pool = mix(state + 1);
    return 0;
}
