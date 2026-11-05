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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <utime.h>

#ifndef UTIME_NOW
#define UTIME_NOW ((1L << 30) - 1L)
#endif
#ifndef UTIME_OMIT
#define UTIME_OMIT ((1L << 30) - 2L)
#endif

static int
to_filetime(const struct timespec *ts, struct __win32_filetime *ft)
{
    if (ts->tv_nsec == UTIME_OMIT)
        return 0;
    int64_t t;
    if (ts->tv_nsec == UTIME_NOW)
        t = __win32_now();
    else
        t = (int64_t)ts->tv_sec * 10000000 + ts->tv_nsec / 100 + __WIN32_EPOCH_DIFF;
    __win32_to_filetime(t, ft);
    return 1;
}

static int
set_times(__win32_handle h, const struct timespec times[2])
{
    struct __win32_filetime access, write;
    struct timespec         now[2] = { { 0, UTIME_NOW }, { 0, UTIME_NOW } };
    if (!times)
        times = now;
    int a = to_filetime(&times[0], &access);
    int w = to_filetime(&times[1], &write);
    if (!SetFileTime(h, NULL, a ? &access : NULL, w ? &write : NULL))
        return __win32_error(0);
    return 0;
}

int
futimens(int fd, const struct timespec times[2])
{
    __win32_handle h = __win32_handle_of(fd);
    if (!h)
        return -1;
    return set_times(h, times);
}

int
utimensat(int dirfd, const char *path, const struct timespec times[2], int flags)
{
    (void)flags;
    if (dirfd != AT_FDCWD && path[0] != '/' && path[0] != '\\' && !(path[0] && path[1] == ':')) {
        errno = ENOSYS;
        return -1;
    }
    /* Directories open only on Windows NT, with backup semantics. */
    __win32_handle h = CreateFileA(path, __WIN32_GENERIC_WRITE,
                                   __WIN32_FILE_SHARE_READ | __WIN32_FILE_SHARE_WRITE, NULL,
                                   __WIN32_OPEN_EXISTING, __WIN32_FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (h == __WIN32_INVALID_HANDLE)
        return __win32_error(0);
    int ret = set_times(h, times);
    CloseHandle(h);
    return ret;
}

int
utimes(const char *path, const struct timeval times[2])
{
    if (!times)
        return utimensat(AT_FDCWD, path, NULL, 0);
    struct timespec ts[2] = { { times[0].tv_sec, times[0].tv_usec * 1000 },
                              { times[1].tv_sec, times[1].tv_usec * 1000 } };
    return utimensat(AT_FDCWD, path, ts, 0);
}

int
utime(const char *path, const struct utimbuf *times)
{
    if (!times)
        return utimensat(AT_FDCWD, path, NULL, 0);
    struct timespec ts[2] = { { times->actime, 0 }, { times->modtime, 0 } };
    return utimensat(AT_FDCWD, path, ts, 0);
}
