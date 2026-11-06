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

#include "local-os2.h"
#include <unistd.h>

/* On a character device \n is written as \r\n. */
static ssize_t
write_dev(int fd, const char *s, size_t count)
{
    char   buf[128];
    size_t done = 0;
    while (done < count) {
        size_t     start = done;
        __os2_word k = 0, n;
        while (done < count && k < sizeof(buf) - 1) {
            char c = s[done++];
            if (c == '\n')
                buf[k++] = '\r';
            buf[k++] = c;
        }
        __os2_ret r = DosWrite((__os2_handle)fd, buf, k, &n);
        if (r)
            return start ? (ssize_t)start : __os2_error(r);
    }
    return (ssize_t)count;
}

ssize_t
write(int fd, const void *buf, size_t count)
{
    __os2_word n;
    if (count > __OS2_MAX_IO)
        count = __OS2_MAX_IO;
    if (__os2_isdev(fd))
        return write_dev(fd, buf, count);
    if (fd >= 0 && fd < __OS2_MAX_FD && (__os2_fd_flags[fd] & __OS2_FD_APPEND)) {
        unsigned long pos;
        __os2_seek((__os2_handle)fd, 0L, SEEK_END, &pos);
    }
    __os2_ret r = DosWrite((__os2_handle)fd, buf, (__os2_word)count, &n);
    if (r)
        return __os2_error(r);
    return (ssize_t)n;
}
