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
#include <sys/stat.h>
#include <unistd.h>

static void
fill(struct stat *st, unsigned attr, unsigned long size, unsigned date, unsigned time)
{
    memset(st, 0, sizeof(*st));
    mode_t mode = (attr & __TOS_FA_DIR) ? S_IFDIR | 0555 : S_IFREG | 0444;
    if (!(attr & __TOS_FA_RDONLY))
        mode |= 0200;
    st->st_mode = mode;
    st->st_nlink = 1;
    st->st_size = size;
    st->st_blksize = 512;
    st->st_blocks = (size + 511) / 512;
    st->st_atime = st->st_mtime = st->st_ctime = (time_t)__tos_unix_time(date, time);
}

/* The root of a drive ("C:\", "\") has no directory entry of its own. */
static int
is_root(const char *name)
{
    if (name[0] && name[1] == ':')
        name += 2;
    return name[0] == '\\' && name[1] == '\0';
}

int
stat(const char *path, struct stat *st)
{
    char  buf[__TOS_PATH_MAX];
    char *name = __tos_path(path, buf);
    if (!name)
        return -1;
    if (is_root(name)) {
        fill(st, __TOS_FA_DIR, 0, 0, 0);
        return 0;
    }
    /* A trailing backslash names a directory; Fsfirst() wants it without. */
    size_t n = strlen(name);
    if (n > 1 && name[n - 1] == '\\' && name[n - 2] != ':')
        name[n - 1] = '\0';
    if (strpbrk(name, "*?")) {
        errno = ENOENT;
        return -1;
    }
    struct __tos_dta dta;
    void            *old = (void *)__tos_Fgetdta();
    __tos_Fsetdta(&dta);
    long r
        = __tos_Fsfirst(name, __TOS_FA_RDONLY | __TOS_FA_HIDDEN | __TOS_FA_SYSTEM | __TOS_FA_DIR);
    __tos_Fsetdta(old);
    if (r < 0)
        return __tos_error(r);
    fill(st, dta.d_attrib, dta.d_length, dta.d_date, dta.d_time);
    return 0;
}

int
lstat(const char *path, struct stat *st)
{
    return stat(path, st);
}

int
fstat(int fd, struct stat *st)
{
    struct __tos_fd *f = __tos_fd(fd);
    if (!f)
        return -1;
    if (__tos_is_tty(f)) {
        memset(st, 0, sizeof(*st));
        st->st_mode = S_IFCHR | 0666;
        st->st_nlink = 1;
        return 0;
    }
    long pos = __tos_Fseek(0L, f->handle, SEEK_CUR);
    long size = __tos_Fseek(0L, f->handle, SEEK_END);
    if (pos >= 0)
        __tos_Fseek(pos, f->handle, SEEK_SET);
    unsigned short dt[2] = { 0, 0 }; /* time, date */
    __tos_Fdatime(dt, f->handle, 0);
    fill(st, 0, size < 0 ? 0 : (unsigned long)size, dt[1], dt[0]);
    return 0;
}
