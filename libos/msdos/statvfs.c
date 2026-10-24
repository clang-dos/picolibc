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
#include <sys/statvfs.h>

/* The size and free space of a drive (INT 21h function 36h). */
int
statvfs(const char * restrict path, struct statvfs * restrict buf)
{
    uint8_t drive = 0; /* The current drive. */
    if (path[0] && path[1] == ':') {
        char c = path[0] | 0x20;
        if (c < 'a' || c > 'z') {
            errno = ENOENT;
            return -1;
        }
        drive = c - 'a' + 1;
    }
    struct __msdos_regs r = { .ax = 0x3600, .dx = drive };
    __msdos_int21(&r);
    if (r.ax == 0xffff) {
        errno = ENOENT;
        return -1;
    }
    memset(buf, 0, sizeof(*buf));
    /* ax sectors per cluster, bx free clusters, cx bytes per sector, dx
     * clusters. */
    buf->f_bsize = buf->f_frsize = (unsigned long)r.ax * r.cx;
    buf->f_blocks = r.dx;
    buf->f_bfree = buf->f_bavail = r.bx;
    buf->f_namemax = 12;
    return 0;
}

int
fstatvfs(int fd, struct statvfs *buf)
{
    (void)fd;
    return statvfs("", buf);
}
