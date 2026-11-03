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
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

/* Days since 1970-01-01 for a date in the Gregorian calendar. */
static long
days_from_civil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    long     era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long)doe - 719468;
}

int64_t
__tos_unix_time(unsigned date, unsigned time)
{
    unsigned day = date & 31, month = (date >> 5) & 15;
    int      year = 1980 + (date >> 9);
    if (!day)
        day = 1;
    if (!month)
        month = 1;
    int64_t days = days_from_civil(year, month, day);
    return days * 86400 + (time >> 11) * 3600 + ((time >> 5) & 63) * 60 + (time & 31) * 2;
}

static long
read_hz200(void)
{
    return *(volatile long *)0x4ba;
}

unsigned long
__tos_hz200(void)
{
    return (unsigned long)__tos_Supexec(read_hz200);
}

/* The time of day: GEMDOS keeps local time to 2 seconds, which is taken
 * as UTC. The 200 Hz timer fills in the fraction, counted from the moment
 * the GEMDOS seconds last changed. */
static void
now(int64_t *sec, long *nsec)
{
    static int64_t       last_sec = -1;
    static unsigned long last_hz;
    unsigned             date = (unsigned)__tos_Tgetdate();
    unsigned             time = (unsigned)__tos_Tgettime();
    unsigned long        hz = __tos_hz200();
    int64_t              s = __tos_unix_time(date, time);
    if (s != last_sec) {
        last_sec = s;
        last_hz = hz;
    }
    unsigned long ticks = hz - last_hz;
    if (ticks >= 400)
        ticks = 399;
    *sec = s + ticks / 200;
    *nsec = (long)(ticks % 200) * 5000000L;
}

int
gettimeofday(struct timeval * restrict tv, void * restrict tz)
{
    (void)tz;
    int64_t s;
    long    ns;
    now(&s, &ns);
    tv->tv_sec = s;
    tv->tv_usec = ns / 1000;
    return 0;
}

int
clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    if (clock_id == CLOCK_MONOTONIC) {
        unsigned long hz = __tos_hz200();
        tp->tv_sec = hz / 200;
        tp->tv_nsec = (long)(hz % 200) * 5000000L;
        return 0;
    }
    if (clock_id != CLOCK_REALTIME) {
        errno = EINVAL;
        return -1;
    }
    int64_t s;
    long    ns;
    now(&s, &ns);
    tp->tv_sec = s;
    tp->tv_nsec = ns;
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
        res->tv_nsec = 5000000L;
    }
    return 0;
}

/* The 200 Hz timer since the program started, as its process time. */
clock_t
times(struct tms *buf)
{
    static unsigned long start;
    static int           started;
    unsigned long        hz = __tos_hz200();
    if (!started) {
        start = hz;
        started = 1;
    }
    clock_t t = (clock_t)((uint64_t)(hz - start) * CLOCKS_PER_SEC / 200);
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
    unsigned long ticks = (unsigned long)req->tv_sec * 200 + (req->tv_nsec + 4999999L) / 5000000L;
    unsigned long start = __tos_hz200();
    while (__tos_hz200() - start < ticks)
        ;
    if (rem)
        rem->tv_sec = rem->tv_nsec = 0;
    return 0;
}

pid_t
getpid(void)
{
    return (pid_t)(uintptr_t)_base;
}
