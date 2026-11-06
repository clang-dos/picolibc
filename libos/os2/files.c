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
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int
unlink(const char *path)
{
    __os2_ret r = __os2_delete(path);
    return r ? __os2_error(r) : 0;
}

int
rename(const char *from, const char *to)
{
    __os2_ret r = __os2_move(from, to);
    /* OS/2 won't move over an existing file; POSIX replaces it. */
    if (r == 5 /* ERROR_ACCESS_DENIED */ && strcmp(from, to) && __os2_delete(to) == 0)
        r = __os2_move(from, to);
    return r ? __os2_error(r) : 0;
}

int
mkdir(const char *path, mode_t mode)
{
    (void)mode;
    __os2_ret r = __os2_mkdir(path);
    return r ? __os2_error(r) : 0;
}

int
rmdir(const char *path)
{
    __os2_ret r = __os2_rmdir(path);
    return r ? __os2_error(r) : 0;
}

int
chdir(const char *path)
{
    __os2_ret r = __os2_chdir(path);
    return r ? __os2_error(r) : 0;
}

/* Seconds since 1970 for a DOS-format date and time, local time. */
long
__os2_unix_time(unsigned short date, unsigned short time)
{
    static const unsigned short days[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    unsigned                    y = 1980 + (date >> 9), m = (date >> 5) & 15, d = date & 31;
    if (m < 1 || m > 12 || d < 1)
        return 0;
    long n = (long)(y - 1970) * 365 + (y - 1969) / 4 + days[m - 1] + d - 1;
    if (m > 2 && y % 4 == 0)
        n++;
    return ((n * 24 + (time >> 11)) * 60 + ((time >> 5) & 63)) * 60 + (time & 31) * 2;
}

static void
fill(struct stat *st, const struct __os2_filestatus *fs)
{
    memset(st, 0, sizeof(*st));
    mode_t mode = (fs->attr & __OS2_FILE_DIRECTORY) ? S_IFDIR | 0555 : S_IFREG | 0444;
    if (!(fs->attr & __OS2_FILE_READONLY))
        mode |= 0200;
    st->st_mode = mode;
    st->st_nlink = 1;
    st->st_size = (off_t)fs->size;
    st->st_blksize = 512;
    st->st_blocks = (fs->alloc + 511) / 512;
    st->st_mtime = (time_t)__os2_unix_time(fs->wdate, fs->wtime);
    st->st_atime = fs->adate ? (time_t)__os2_unix_time(fs->adate, fs->atime) : st->st_mtime;
    st->st_ctime = fs->cdate ? (time_t)__os2_unix_time(fs->cdate, fs->ctime) : st->st_mtime;
}

int
stat(const char *path, struct stat *st)
{
    struct __os2_filestatus fs;
    __os2_ret               r = __os2_pathinfo(path, &fs);
    if (r)
        return __os2_error(r);
    fill(st, &fs);
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
    struct __os2_filestatus fs;
    if (__os2_isdev(fd)) {
        memset(st, 0, sizeof(*st));
        st->st_mode = S_IFCHR | 0666;
        st->st_nlink = 1;
        return 0;
    }
    __os2_ret r = __os2_fileinfo((__os2_handle)fd, &fs);
    if (r)
        return __os2_error(r);
    fill(st, &fs);
    return 0;
}

int
access(const char *path, int mode)
{
    struct stat st;
    if (stat(path, &st) < 0)
        return -1;
    if ((mode & W_OK) && !(st.st_mode & 0200)) {
        errno = EACCES;
        return -1;
    }
    return 0;
}
