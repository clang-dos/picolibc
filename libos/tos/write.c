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

#include "local-tos.h"
#include <string.h>
#include <unistd.h>

/* On the console and the other character devices, \n is written as \r\n,
 * which the VT52 console needs to start a new line. */
static long
write_tty(short h, const char *s, long n)
{
    char buf[128];
    long done = 0;
    while (done < n) {
        int k = 0;
        while (done < n && k < (int)sizeof(buf) - 1) {
            char c = s[done];
            if (c == '\n')
                buf[k++] = '\r';
            buf[k++] = c;
            done++;
        }
        long r = __tos_Fwrite(h, k, buf);
        if (r < 0)
            return r;
    }
    return n;
}

ssize_t
write(int fd, const void *buf, size_t count)
{
    struct __tos_fd *f = __tos_fd(fd);
    if (!f)
        return -1;
    if (count > 0x7fffffff)
        count = 0x7fffffff;
    long n;
    if (__tos_is_tty(f)) {
        n = write_tty(f->handle, buf, (long)count);
    } else {
        if (f->flags & __TOS_FD_APPEND)
            __tos_Fseek(0L, f->handle, SEEK_END);
        n = __tos_Fwrite(f->handle, (long)count, buf);
    }
    return n < 0 ? __tos_error(n) : n;
}
