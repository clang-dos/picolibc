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

#include "local-amigaos.h"
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

struct datestamp {
    long days, minute, tick;
} __attribute__((__aligned__(4)));

void
__amiga_time(int64_t *sec, long *tick)
{
    struct datestamp ds;
    __amiga_DateStamp(&ds);
    *sec = ((int64_t)ds.days * 1440 + ds.minute) * 60 + ds.tick / __AMIGA_TICKS
        + __AMIGA_EPOCH_OFFSET;
    *tick = ds.tick % __AMIGA_TICKS;
}

int
gettimeofday(struct timeval * restrict tv, void * restrict tz)
{
    (void)tz;
    int64_t sec;
    long    tick;
    __amiga_time(&sec, &tick);
    tv->tv_sec = sec;
    tv->tv_usec = tick * (1000000 / __AMIGA_TICKS);
    return 0;
}

int
clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    if (clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC) {
        errno = EINVAL;
        return -1;
    }
    int64_t sec;
    long    tick;
    __amiga_time(&sec, &tick);
    tp->tv_sec = sec;
    tp->tv_nsec = tick * (1000000000L / __AMIGA_TICKS);
    return 0;
}

int
clock_getres(clockid_t clock_id, struct timespec *res)
{
    if (clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC) {
        errno = EINVAL;
        return -1;
    }
    if (res) {
        res->tv_sec = 0;
        res->tv_nsec = 1000000000L / __AMIGA_TICKS;
    }
    return 0;
}

/* The DateStamp clock as the process time: AmigaOS doesn't count the time
 * each task runs. */
clock_t
times(struct tms *buf)
{
    static int64_t start = -1;
    int64_t        sec;
    long           tick;
    __amiga_time(&sec, &tick);
    int64_t now = sec * __AMIGA_TICKS + tick;
    if (start < 0)
        start = now;
    clock_t t = (clock_t)((now - start) * (CLOCKS_PER_SEC / __AMIGA_TICKS));
    if (buf) {
        buf->tms_utime = t;
        buf->tms_stime = buf->tms_cutime = buf->tms_cstime = 0;
    }
    return t;
}

int
nanosleep(const struct timespec *req, struct timespec *rem)
{
    if (req->tv_nsec < 0 || req->tv_nsec >= 1000000000L) {
        errno = EINVAL;
        return -1;
    }
    long ticks = (long)req->tv_sec * __AMIGA_TICKS
        + (req->tv_nsec + 1000000000L / __AMIGA_TICKS - 1) / (1000000000L / __AMIGA_TICKS);
    if (ticks > 0)
        __amiga_Delay(ticks);
    if (rem)
        rem->tv_sec = rem->tv_nsec = 0;
    return 0;
}

pid_t
getpid(void)
{
    return (pid_t)(uintptr_t)__amiga_FindTask(NULL);
}
