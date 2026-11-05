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
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

int64_t
__win32_filetime(const struct __win32_filetime *ft)
{
    return (int64_t)(((uint64_t)ft->high << 32) | ft->low);
}

void
__win32_to_filetime(int64_t t, struct __win32_filetime *ft)
{
    ft->low = (__win32_dword)(uint64_t)t;
    ft->high = (__win32_dword)((uint64_t)t >> 32);
}

static struct timespec
to_timespec(const struct __win32_filetime *ft)
{
    int64_t         t = __win32_filetime(ft) - __WIN32_EPOCH_DIFF;
    struct timespec ts;
    ts.tv_sec = t / 10000000;
    ts.tv_nsec = (long)(t % 10000000) * 100;
    if (ts.tv_nsec < 0) {
        ts.tv_sec--;
        ts.tv_nsec += 1000000000;
    }
    return ts;
}

/* Programs that Windows runs by their name. */
static int
is_program(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (!dot || strchr(dot, '\\') || strchr(dot, '/'))
        return 0;
    static const char *const ext[] = { ".exe", ".com", ".bat", ".cmd" };
    for (size_t i = 0; i < sizeof(ext) / sizeof(ext[0]); i++)
        if (!strcasecmp(dot, ext[i]))
            return 1;
    return 0;
}

static mode_t
file_mode(__win32_dword attributes, const char *name)
{
    if (attributes & __WIN32_FILE_ATTRIBUTE_DIR)
        return S_IFDIR | 0755;
    mode_t mode = S_IFREG | 0444;
    if (!(attributes & __WIN32_FILE_ATTRIBUTE_READONLY))
        mode |= 0200;
    if (name && is_program(name))
        mode |= 0111;
    return mode;
}

/* The drive number (A: is 0) of a full path, or 0 for a network path. */
static dev_t
drive(const char *full)
{
    if (isalpha((unsigned char)full[0]) && full[1] == ':')
        return (dev_t)(toupper((unsigned char)full[0]) - 'A');
    return 0;
}

/* "X:\" or "\\server\share\", which FindFirstFile does not find. */
static int
is_root(const char *full)
{
    if (isalpha((unsigned char)full[0]) && full[1] == ':')
        return (full[2] == '\\' || full[2] == '/') && full[3] == '\0';
    if ((full[0] == '\\' || full[0] == '/') && (full[1] == '\\' || full[1] == '/')) {
        int    parts = 0;
        size_t i = 2;
        while (full[i]) {
            while (full[i] && full[i] != '\\' && full[i] != '/')
                i++;
            parts++;
            if (full[i])
                i++;
            else
                break;
        }
        return parts <= 2;
    }
    return 0;
}

int
stat(const char * restrict path, struct stat * restrict st)
{
    char   full[1024];
    char  *file;
    if (!path[0] || strpbrk(path, "*?")) {
        errno = ENOENT;
        return -1;
    }
    /* The full path has no ".", ".." or trailing separators but those of a
     * root. */
    __win32_dword n = GetFullPathNameA(path, sizeof(full), full, &file);
    if (n == 0)
        return __win32_error(0);
    if (n >= sizeof(full)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memset(st, 0, sizeof(*st));
    st->st_nlink = 1;
    st->st_dev = st->st_rdev = drive(full);
    if (is_root(full)) {
        __win32_dword attr = GetFileAttributesA(full);
        if (attr == __WIN32_INVALID_FILE_ATTRIBUTES)
            return __win32_error(0);
        st->st_mode = S_IFDIR | 0755;
        return 0;
    }
    if (n > 1 && (full[n - 1] == '\\' || full[n - 1] == '/'))
        full[n - 1] = '\0';

    struct __win32_find_data data;
    __win32_handle           h = FindFirstFileA(full, &data);
    if (h == __WIN32_INVALID_HANDLE)
        return __win32_error(0);
    FindClose(h);
    st->st_mode = file_mode(data.attributes, full);
    st->st_size = (off_t)(((uint64_t)data.size_high << 32) | data.size_low);
    st->st_mtim = to_timespec(&data.write);
    st->st_atim = to_timespec(&data.access);
    st->st_ctim = to_timespec(&data.creation);
    if (!st->st_atim.tv_sec && !st->st_atim.tv_nsec)
        st->st_atim = st->st_mtim;
    if (!st->st_ctim.tv_sec && !st->st_ctim.tv_nsec)
        st->st_ctim = st->st_mtim;
    st->st_blksize = 4096;
    st->st_blocks = (st->st_size + 511) / 512;
    return 0;
}

int
lstat(const char * restrict path, struct stat * restrict st)
{
    return stat(path, st);
}

int
fstat(int fd, struct stat *st)
{
    __win32_handle h = __win32_handle_of(fd);
    if (!h)
        return -1;
    memset(st, 0, sizeof(*st));
    st->st_nlink = 1;
    int flags = __win32_fds[fd].flags;
    if (flags & __WIN32_FD_DEVICE) {
        st->st_mode = S_IFCHR | 0666;
        return 0;
    }
    if (flags & __WIN32_FD_PIPE) {
        st->st_mode = S_IFIFO | 0666;
        return 0;
    }
    struct __win32_file_info info;
    if (!GetFileInformationByHandle(h, &info))
        return __win32_error(0);
    st->st_mode = file_mode(info.attributes, NULL);
    st->st_nlink = info.links;
    st->st_dev = st->st_rdev = info.volume_serial;
    st->st_ino = (ino_t)(((uint64_t)info.index_high << 32) | info.index_low);
    st->st_size = (off_t)(((uint64_t)info.size_high << 32) | info.size_low);
    st->st_mtim = to_timespec(&info.write);
    st->st_atim = to_timespec(&info.access);
    st->st_ctim = to_timespec(&info.creation);
    if (!st->st_atim.tv_sec && !st->st_atim.tv_nsec)
        st->st_atim = st->st_mtim;
    if (!st->st_ctim.tv_sec && !st->st_ctim.tv_nsec)
        st->st_ctim = st->st_mtim;
    st->st_blksize = 4096;
    st->st_blocks = (st->st_size + 511) / 512;
    return 0;
}
