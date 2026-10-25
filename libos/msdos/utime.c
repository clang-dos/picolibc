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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <utime.h>

/* picolibc's headers may not define these. */
#ifndef UTIME_NOW
#define UTIME_NOW ((1L << 30) - 1L)
#endif
#ifndef UTIME_OMIT
#define UTIME_OMIT ((1L << 30) - 2L)
#endif

/* Set the date and time of an open file (INT 21h function 5701h). DOS
 * keeps one time, the last write, in local time, which is taken as UTC,
 * to 2 seconds, from 1980 to 2107. */
static int
set_time(int fd, time_t t)
{
    if (t < 315532800)
        t = 315532800; /* 1980-01-01 */
    int32_t days = t / 86400;
    int32_t secs = t % 86400;
    /* Days since 1970 to a date, after Howard Hinnant's civil_from_days. */
    int32_t z = days + 719468;
    int32_t era = z / 146097;
    int32_t doe = z - era * 146097;
    int32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    int32_t mp = (5 * doy + 2) / 153;
    int d = doy - (153 * mp + 2) / 5 + 1;
    int m = mp < 10 ? mp + 3 : mp - 9;
    int32_t y = yoe + era * 400 + (m <= 2);
    if (y > 2107) {
        errno = EINVAL;
        return -1;
    }
    uint16_t date = ((y - 1980) << 9) | (m << 5) | d;
    uint16_t time = ((secs / 3600) << 11) | (((secs / 60) % 60) << 5) | ((secs % 60) / 2);
    struct __msdos_regs r = { .ax = 0x5701, .bx = (uint16_t)fd, .cx = time,
                              .dx = date };
    if (__msdos_int21(&r))
        return __msdos_error(r.ax);
    return 0;
}

static time_t
now(void)
{
    int64_t sec;
    uint16_t centi;
    __msdos_time(&sec, &centi);
    return sec;
}

static int
set_path_time(const char *path, time_t t)
{
    int fd = open(path, O_RDONLY | O_BINARY);
    if (fd < 0)
        return -1;
    int ret = set_time(fd, t);
    close(fd);
    return ret;
}

int
futimens(int fd, const struct timespec times[2])
{
    if (times == NULL || times[1].tv_nsec == UTIME_NOW)
        return set_time(fd, now());
    if (times[1].tv_nsec == UTIME_OMIT)
        return 0;
    return set_time(fd, times[1].tv_sec);
}

int
utimensat(int dirfd, const char *path, const struct timespec times[2], int flags)
{
    (void)flags;
    if (dirfd != AT_FDCWD && path[0] != '/' && path[0] != '\\' &&
        !(path[0] && path[1] == ':')) {
        errno = ENOSYS;
        return -1;
    }
    if (times != NULL && times[1].tv_nsec == UTIME_OMIT)
        return access(path, F_OK);
    return set_path_time(path, times == NULL || times[1].tv_nsec == UTIME_NOW
                                   ? now() : times[1].tv_sec);
}

int
utimes(const char *path, const struct timeval times[2])
{
    return set_path_time(path, times ? times[1].tv_sec : now());
}

int
utime(const char *path, const struct utimbuf *times)
{
    return set_path_time(path, times ? times->modtime : now());
}
