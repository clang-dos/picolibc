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

#include "local-msdos.h"
#include <string.h>
#include <unistd.h>

static int
dos_write(int fd, const char *p, size_t n, size_t *done)
{
    struct __msdos_regs r = { .ax = 0x4000, .bx = (uint16_t)fd,
                              .cx = (uint16_t)n, .dx = __MSDOS_OFF(p),
                              .ds = __MSDOS_SEG(p) };
    if (__msdos_int21(&r))
        return __msdos_error(r.ax);
    *done = r.ax;
    if (r.ax != n) {
        /* DOS reports a full disk as a short write. */
        errno = ENOSPC;
        return -1;
    }
    return 0;
}

/* In text mode each LF is written as CR LF. The count returned is of the
 * caller's bytes. */
ssize_t
write(int fd, const void *buf, size_t count)
{
    const char *p = buf;
    size_t n;
    int flags = __msdos_fdflags(fd);

    if (count > 0xfffe)
        count = 0xfffe;
    if (flags & __MSDOS_FD_APPEND) {
        struct __msdos_regs r = { .ax = 0x4202, .bx = (uint16_t)fd };
        if (__msdos_int21(&r))
            return __msdos_error(r.ax);
    }
    if (count == 0 || !(flags & __MSDOS_FD_TEXT) || !memchr(p, '\n', count)) {
        /* A write of 0 bytes truncates a file at the current position, as
         * in DOS. */
        if (dos_write(fd, p, count, &n) < 0)
            return n ? (ssize_t)n : -1;
        return n;
    }

    char tmp[128];
    size_t t = 0, pending = 0, done = 0;
    for (size_t i = 0; i <= count; i++) {
        if (i == count || t + 2 > sizeof(tmp)) {
            if (t && dos_write(fd, tmp, t, &n) < 0)
                return done ? (ssize_t)done : -1;
            done += pending;
            t = pending = 0;
            if (i == count)
                break;
        }
        if (p[i] == '\n')
            tmp[t++] = '\r';
        tmp[t++] = p[i];
        pending++;
    }
    return done;
}
