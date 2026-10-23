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

/* Find-first data, as INT 21h/4Eh fills it in. */
struct dta {
    uint8_t reserved[21];
    uint8_t attr;
    uint16_t time, date;
    uint32_t size;
    char name[13];
} __attribute__((packed));

static time_t
dos_time(uint16_t date, uint16_t time)
{
    struct tm tm = {
        .tm_year = (date >> 9) + 80, .tm_mon = ((date >> 5) & 15) - 1,
        .tm_mday = date & 31, .tm_hour = time >> 11,
        .tm_min = (time >> 5) & 63, .tm_sec = (time & 31) * 2,
    };
    return __msdos_mktime(&tm);
}

int
stat(const char * restrict path, struct stat * restrict st)
{
    static struct dta dta;
    struct __msdos_regs r = { .ax = 0x1a00, .dx = (uint16_t)&dta };
    __msdos_int21(&r);
    /* Find files, including hidden, system and directory entries. */
    r = (struct __msdos_regs){ .ax = 0x4e00, .cx = 0x16, .dx = (uint16_t)path };
    if (__msdos_int21(&r)) {
        /* The root directory of a drive has no entry. */
        size_t n = strlen(path);
        if ((n == 3 && path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
            || (n == 1 && (path[0] == '\\' || path[0] == '/'))) {
            memset(st, 0, sizeof(*st));
            st->st_mode = S_IFDIR | 0777;
            st->st_nlink = 1;
            return 0;
        }
        return __msdos_error(r.ax);
    }
    memset(st, 0, sizeof(*st));
    st->st_mode = (dta.attr & 0x10) ? S_IFDIR | 0777 : S_IFREG | 0666;
    if (dta.attr & 0x01)
        st->st_mode &= ~0222;
    st->st_nlink = 1;
    st->st_size = dta.size;
    st->st_mtime = st->st_atime = st->st_ctime = dos_time(dta.date, dta.time);
    st->st_blksize = 512;
    st->st_blocks = (dta.size + 511) / 512;
    return 0;
}
