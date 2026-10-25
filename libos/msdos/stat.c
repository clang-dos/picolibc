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

static int
find(const char *path)
{
    /* Find files, including hidden, system and directory entries. */
    struct __msdos_regs r = { .ax = 0x4e00, .cx = 0x16,
                              .dx = __MSDOS_OFF(path),
                              .ds = __MSDOS_SEG(path) };
    return __msdos_int21(&r) ? r.ax : 0;
}

/* "X:\", "X:/", "\" or "/", which have no directory entry of their own. */
static int
is_root(const char *p)
{
    if (p[0] && p[1] == ':')
        p += 2;
    return (p[0] == '\\' || p[0] == '/') && p[1] == '\0';
}

int
stat(const char * restrict path, struct stat * restrict st)
{
    static struct dta dta;
    /* Static, not on the stack, which is not DGROUP in a DLL. */
    static char full[128];
    struct __msdos_regs r = { .ax = 0x1a00, .dx = __MSDOS_OFF(&dta),
                              .ds = __MSDOS_SEG(&dta) };
    __msdos_int21(&r);
    int err = find(path);
    if (err) {
        int root = is_root(path);
        /* "dir\", ".", ".." and "dir\.." have no entry of that name
         * either: look for the canonical name (INT 21h function 60h). */
        r = (struct __msdos_regs){ .ax = 0x6000, .si = __MSDOS_OFF(path),
                                   .ds = __MSDOS_SEG(path),
                                   .di = __MSDOS_OFF(full),
                                   .es = __MSDOS_SEG(full) };
        if (!root && path[0] && !__msdos_int21(&r)) {
            size_t n = strlen(full);
            if (n > 3 && full[n - 1] == '\\')
                full[--n] = '\0';
            if (is_root(full))
                root = 1;
            else if (!find(full))
                err = 0;
        }
        if (root) {
            memset(st, 0, sizeof(*st));
            st->st_mode = S_IFDIR | 0777;
            st->st_nlink = 1;
            return 0;
        }
        if (err)
            return __msdos_error(err);
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
