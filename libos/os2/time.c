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
#include <sys/time.h>
#include <time.h>

/* The clock is local time; the time zone, when OS/2 2.x knows it, gives
 * UTC. */
int
gettimeofday(struct timeval *restrict tv, void *restrict tz)
{
    (void)tz;
    struct __os2_datetime dt;
    if (DosGetDateTime(&dt))
        return -1;
    unsigned short date
        = (unsigned short)(((dt.year - 1980) << 9) | (dt.month << 5) | dt.day);
    unsigned short time = (unsigned short)((dt.hours << 11) | (dt.minutes << 5));
    long           t = __os2_unix_time(date, time) + dt.seconds;
    if (dt.timezone != -1)
        t += (long)dt.timezone * 60;
    if (tv) {
        tv->tv_sec = t;
        tv->tv_usec = (long)dt.hundredths * 10000;
    }
    return 0;
}

int
nanosleep(const struct timespec *req, struct timespec *rem)
{
    unsigned long ms = (unsigned long)req->tv_sec * 1000 + (req->tv_nsec + 999999) / 1000000;
    __os2_sleep(ms);
    if (rem)
        rem->tv_sec = rem->tv_nsec = 0;
    return 0;
}
