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
#include <sys/stat.h>
#include <unistd.h>

int
fstat(int fd, struct stat *st)
{
    int dev = __msdos_isdevice(fd);
    if (dev < 0)
        return -1;
    memset(st, 0, sizeof(*st));
    st->st_nlink = 1;
    st->st_blksize = 512;
    if (dev) {
        st->st_mode = S_IFCHR | 0666;
        return 0;
    }
    st->st_mode = S_IFREG | 0666;
    off_t cur = lseek(fd, 0, SEEK_CUR);
    if (cur >= 0) {
        st->st_size = lseek(fd, 0, SEEK_END);
        lseek(fd, cur, SEEK_SET);
    }
    st->st_blocks = (st->st_size + 511) / 512;
    /* File date and time. */
    struct __msdos_regs r = { .ax = 0x5700, .bx = (uint16_t)fd };
    if (!__msdos_int21(&r)) {
        struct tm tm = {
            .tm_year = (r.dx >> 9) + 80, .tm_mon = ((r.dx >> 5) & 15) - 1,
            .tm_mday = r.dx & 31, .tm_hour = r.cx >> 11,
            .tm_min = (r.cx >> 5) & 63, .tm_sec = (r.cx & 31) * 2,
        };
        st->st_mtime = st->st_atime = st->st_ctime = __msdos_mktime(&tm);
    }
    return 0;
}
