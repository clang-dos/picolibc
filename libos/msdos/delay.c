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

/* Turbo C's delay, sound and nosound: the 8253 timer and the PC speaker. */

#include <dos.h>
#include <stdint.h>

/* The 8253's input clock, in hertz. */
#define PIT_HZ 1193182UL

static uint16_t
pit_count(void)
{
    uint16_t flags;
    __asm__ __volatile__("pushfw; popw %0; cli" : "=r"(flags));
    outportb(0x43, 0x00); /* latch counter 0 */
    uint16_t lo = inportb(0x40);
    uint16_t hi = inportb(0x40);
    if (flags & 0x200)
        _enable();
    return (uint16_t)(lo | hi << 8);
}

/* Counter 0 runs in mode 3 (square wave), set by the BIOS: it counts down
 * by 2 at each input clock, from 65536 to 0, twice per timer tick. So the
 * time between two close readings is half their difference in clocks.
 * The readings must be less than about 27 ms apart, which a loop that
 * does nothing else keeps. */
void
delay(unsigned ms)
{
    uint32_t want = (uint32_t)ms * (PIT_HZ / 1000) + ((uint32_t)ms * (PIT_HZ % 1000)) / 1000;
    uint32_t done = 0;
    uint16_t prev = pit_count();
    while (done < want) {
        uint16_t now = pit_count();
        done += (uint16_t)(prev - now) >> 1;
        prev = now;
    }
}

void
sound(unsigned hz)
{
    if (hz < 19)
        hz = 19;
    uint16_t div = (uint16_t)(PIT_HZ / hz);
    outportb(0x43, 0xb6); /* counter 2, low then high byte, mode 3 */
    outportb(0x42, (uint8_t)div);
    outportb(0x42, (uint8_t)(div >> 8));
    outportb(0x61, inportb(0x61) | 3);
}

void
nosound(void)
{
    outportb(0x61, inportb(0x61) & ~3);
}
