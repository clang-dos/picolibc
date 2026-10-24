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

/* Days from 1970-01-01 to y-m-d (proleptic Gregorian). */
static int32_t
days_from_civil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    int era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned long doe = yoe * 365UL + yoe / 4 - yoe / 100 + doy;
    return era * 146097L + (int32_t)doe - 719468L;
}

/* The DOS clock, which is local time, as seconds since 1970 and
 * hundredths. The clock ticks about 18.2 times a second. */
void
__msdos_time(int64_t *sec, uint16_t *centi)
{
    struct __msdos_regs d1, t, d2;
    do {
        /* Unused registers are zero: DS and ES of 0 are DGROUP. */
        d1 = (struct __msdos_regs){ .ax = 0x2a00 };
        __msdos_int21(&d1);
        t = (struct __msdos_regs){ .ax = 0x2c00 };
        __msdos_int21(&t);
        d2 = (struct __msdos_regs){ .ax = 0x2a00 };
        __msdos_int21(&d2);
        /* Read again if the date changed at midnight. */
    } while (d1.cx != d2.cx || d1.dx != d2.dx);
    int32_t days = days_from_civil(d1.cx, d1.dx >> 8, d1.dx & 0xff);
    *sec = (int64_t)days * 86400 + (uint32_t)(t.cx >> 8) * 3600
        + (t.cx & 0xff) * 60 + (t.dx >> 8);
    *centi = t.dx & 0xff;
}
