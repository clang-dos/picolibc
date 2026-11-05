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
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

/* GetSystemTimeAsFileTime is missing from Windows NT 3.1. */
int64_t
__win32_now(void)
{
    struct __win32_systemtime st;
    struct __win32_filetime   ft;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);
    return __win32_filetime(&ft);
}

/* The performance counter, or the tick count where there is none, in
 * nanoseconds since some point in the past. */
static int64_t
monotonic_ns(void)
{
    static int64_t freq;
    static int     checked;
    if (!checked) {
        if (!QueryPerformanceFrequency(&freq) || freq <= 0)
            freq = 0;
        checked = 1;
    }
    int64_t count;
    if (freq && QueryPerformanceCounter(&count))
        return (count / freq) * 1000000000LL + (count % freq) * 1000000000LL / freq;
    /* GetTickCount wraps after 49.7 days; count the wraps. */
    static __win32_dword last, wraps;
    __win32_dword        ticks = GetTickCount();
    if (ticks < last)
        wraps++;
    last = ticks;
    return (((int64_t)wraps << 32) + ticks) * 1000000LL;
}

int
clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    switch (clock_id) {
    case CLOCK_REALTIME: {
        int64_t t = __win32_now() - __WIN32_EPOCH_DIFF;
        tp->tv_sec = t / 10000000;
        tp->tv_nsec = (long)(t % 10000000) * 100;
        return 0;
    }
    case CLOCK_MONOTONIC: {
        int64_t t = monotonic_ns();
        tp->tv_sec = t / 1000000000;
        tp->tv_nsec = (long)(t % 1000000000);
        return 0;
    }
    default:
        errno = EINVAL;
        return -1;
    }
}

int
clock_getres(clockid_t clock_id, struct timespec *res)
{
    if (clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC) {
        errno = EINVAL;
        return -1;
    }
    if (res) {
        /* The system clock ticks every 10 to 55 ms. */
        res->tv_sec = 0;
        res->tv_nsec = clock_id == CLOCK_REALTIME ? 10000000L : 1000;
    }
    return 0;
}

int
gettimeofday(struct timeval * restrict tv, void * restrict tz)
{
    (void)tz;
    if (tv) {
        int64_t t = __win32_now() - __WIN32_EPOCH_DIFF;
        tv->tv_sec = t / 10000000;
        tv->tv_usec = (long)(t % 10000000) / 10;
    }
    return 0;
}

static int64_t start_ns;

static void __attribute__((__constructor__))
init_clock(void)
{
    start_ns = monotonic_ns();
}

/* Process time is the time since the program started, as with the
 * Microsoft C library's clock(). */
clock_t
times(struct tms *buf)
{
    clock_t t = (clock_t)((monotonic_ns() - start_ns) / (1000000000LL / CLOCKS_PER_SEC));
    if (buf) {
        buf->tms_utime = t;
        buf->tms_stime = 0;
        buf->tms_cutime = 0;
        buf->tms_cstime = 0;
    }
    return t;
}

int
nanosleep(const struct timespec *req, struct timespec *rem)
{
    if (req->tv_nsec < 0 || req->tv_nsec >= 1000000000L || req->tv_sec < 0) {
        errno = EINVAL;
        return -1;
    }
    int64_t ms = (int64_t)req->tv_sec * 1000 + (req->tv_nsec + 999999) / 1000000;
    while (ms > 0) {
        __win32_dword part = ms > 0x7fffffff ? 0x7fffffff : (__win32_dword)ms;
        Sleep(part);
        ms -= part;
    }
    if (rem) {
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
    }
    return 0;
}
