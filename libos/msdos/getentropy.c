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

/*
 * getentropy for DOS and 16-bit Windows.
 *
 * Neither has a random source, so this collects timing noise and mixes it
 * into a 512-bit sponge built on the ChaCha permutation:
 *
 * - once per call: the DOS date and time, the BIOS tick count (DOS), the
 *   Windows tick count, task and instance (Windows), the PSP segment and
 *   the stack pointer, and the time stamp counter on CPUs that have one.
 * - then many readings of the 8253/8254 timer (channel 0, 1.19 MHz) and
 *   the time stamp counter, each after a short delay loop whose length
 *   depends on the previous reading. Interrupts, DMA, caches and the
 *   different clocks of the CPU and the timer make the readings jitter.
 *
 * On a fast machine with a time stamp counter this is a good seed. On an
 * 8088 PC, where the CPU and the timer run from the same crystal, most of
 * it comes from the moment the program started (to a microsecond); an
 * attacker who knows that time well can guess it. The state is kept
 * between calls, so each call also gets what the earlier ones collected.
 */

#include "local-msdos.h"
#include <unistd.h>
#include <string.h>

#ifdef __WINDOWS__
uint32_t __WIN16_API GetTickCount(void);
uint16_t __WIN16_API GetCurrentTask(void);
uint32_t __WIN16_API GetWinFlags(void);
#define WF_CPU386 0x0004
#define WF_CPU486 0x0008
#endif

#define RATE 8 /* 32-bit words absorbed and squeezed per permutation */

static uint32_t state[16] = {
    /* "expand 32-byte k", so that the state is never all zero. */
    0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
};
static uint8_t  absorbed; /* 16-bit halves absorbed since the permutation */
static int8_t   tsc = -1; /* the CPU has RDTSC: -1 not known yet */

static uint32_t
rotl(uint32_t x, int n)
{
    return (x << n) | (x >> (32 - n));
}

#define QR(a, b, c, d)                                                         \
    (s[a] += s[b], s[d] = rotl(s[d] ^ s[a], 16), s[c] += s[d],                 \
     s[b] = rotl(s[b] ^ s[c], 12), s[a] += s[b], s[d] = rotl(s[d] ^ s[a], 8),  \
     s[c] += s[d], s[b] = rotl(s[b] ^ s[c], 7))

/* The ChaCha8 permutation. */
static void
permute(void)
{
    uint32_t *s = state;
    for (int i = 0; i < 4; i++) {
        QR(0, 4, 8, 12);
        QR(1, 5, 9, 13);
        QR(2, 6, 10, 14);
        QR(3, 7, 11, 15);
        QR(0, 5, 10, 15);
        QR(1, 6, 11, 12);
        QR(2, 7, 8, 13);
        QR(3, 4, 9, 14);
    }
    absorbed = 0;
}

static void
absorb(uint16_t v)
{
    state[absorbed >> 1] ^= (uint32_t)v << ((absorbed & 1) * 16);
    if (++absorbed == RATE * 2)
        permute();
}

static void
absorb32(uint32_t v)
{
    absorb((uint16_t)v);
    absorb((uint16_t)(v >> 16));
}

/* Whether the CPU has RDTSC: a 386 or later with CPUID, and the TSC bit. */
static int
has_tsc(void)
{
#ifdef __WINDOWS__
    /* The FLAGS test below is for real mode; Windows knows the CPU. */
    if (!(GetWinFlags() & (WF_CPU386 | WF_CPU486)))
        return 0;
#else
    /* An 8086 or 80186 keeps FLAGS bits 12-15 set, an 80286 in real mode
     * keeps them clear. */
    uint16_t lo, hi;
    __asm__ volatile("pushfw\n\t"
                     "pushfw\n\t"
                     "popw %%ax\n\t"
                     "andw $0x0fff, %%ax\n\t"
                     "pushw %%ax\n\t"
                     "popfw\n\t"
                     "pushfw\n\t"
                     "popw %0\n\t"
                     "orw $0xf000, %%ax\n\t"
                     "pushw %%ax\n\t"
                     "popfw\n\t"
                     "pushfw\n\t"
                     "popw %1\n\t"
                     "popfw"
                     : "=r"(lo), "=r"(hi)
                     :
                     : "ax", "cc");
    if ((lo & 0xf000) == 0xf000 || !(hi & 0xf000))
        return 0;
#endif
    /* The rest uses 32-bit registers, which the assembler takes only for
     * an 80386, so they are bytes. CPUID exists if the ID flag (EFLAGS
     * bit 21) can be changed. */
    uint16_t id;
    __asm__ volatile(".byte 0x66, 0x9c\n\t"                   /* pushfl */
                     ".byte 0x66, 0x58\n\t"                   /* popl %eax */
                     ".byte 0x66, 0x89, 0xc1\n\t"             /* movl %eax, %ecx */
                     ".byte 0x66, 0x35, 0, 0, 0x20, 0\n\t"    /* xorl $0x200000, %eax */
                     ".byte 0x66, 0x50\n\t"                   /* pushl %eax */
                     ".byte 0x66, 0x9d\n\t"                   /* popfl */
                     ".byte 0x66, 0x9c\n\t"                   /* pushfl */
                     ".byte 0x66, 0x58\n\t"                   /* popl %eax */
                     ".byte 0x66, 0x51\n\t"                   /* pushl %ecx */
                     ".byte 0x66, 0x9d\n\t"                   /* popfl */
                     ".byte 0x66, 0x31, 0xc8\n\t"             /* xorl %ecx, %eax */
                     ".byte 0x66, 0xc1, 0xe8, 0x10"             /* shrl $16, %eax */
                     : "=a"(id)
                     :
                     : "cx", "cc");
    if (!(id & 0x20))
        return 0;
    /* CPUID leaf 1: EDX bit 4 is the TSC. EBX is kept whole. */
    uint16_t features;
    __asm__ volatile(".byte 0x66, 0x53\n\t"                   /* pushl %ebx */
                     ".byte 0x66, 0xb8, 1, 0, 0, 0\n\t"       /* movl $1, %eax */
                     ".byte 0x0f, 0xa2\n\t"                   /* cpuid */
                     ".byte 0x66, 0x5b"                         /* popl %ebx */
                     : "=d"(features)
                     :
                     : "ax", "cx", "cc");
    return (features & 0x10) != 0;
}

static void
absorb_tsc(void)
{
    uint16_t t0, t1, t2, t3;
    if (!tsc)
        return;
    __asm__ volatile(".byte 0x0f, 0x31\n\t"                   /* rdtsc */
                     "movw %%ax, %%cx\n\t"
                     ".byte 0x66, 0xc1, 0xe8, 0x10\n\t"       /* shrl $16, %eax */
                     "movw %%dx, %%bx\n\t"
                     ".byte 0x66, 0xc1, 0xea, 0x10"             /* shrl $16, %edx */
                     : "=c"(t0), "=a"(t1), "=b"(t2), "=d"(t3));
    absorb(t0);
    absorb(t1);
    absorb(t2);
    absorb(t3);
}

/* Timer channel 0, which counts down at 1.19 MHz. */
static uint16_t
pit(void)
{
    uint16_t v;
    __asm__ volatile("xorb %%al, %%al\n\t" /* latch channel 0 */
                     "outb %%al, $0x43\n\t"
                     "inb $0x40, %%al\n\t"
                     "movb %%al, %%ah\n\t"
                     "inb $0x40, %%al\n\t"
                     "xchgb %%al, %%ah"
                     : "=a"(v));
    return v;
}

static void
collect(size_t len)
{
    int64_t sec;
    uint16_t centi, ss, sp;

    if (tsc < 0)
        tsc = has_tsc();
    absorb_tsc();
    __msdos_time(&sec, &centi);
    absorb32((uint32_t)sec);
    absorb(centi);
    __asm__("movw %%ss, %0\n\tmovw %%sp, %1" : "=r"(ss), "=r"(sp));
    absorb(ss);
    absorb(sp);
#ifdef __WINDOWS__
    absorb32(GetTickCount());
    absorb(GetCurrentTask());
    absorb(__win16_hinstance);
#else
    uint32_t ticks;
    __msdos_fromfar(&ticks, 0x40, 0x6c, sizeof(ticks));
    absorb32(ticks);
    absorb(__msdos_psp);
#endif

    /* 64 readings, and 8 more for each byte asked for. */
    uint16_t v = pit();
    for (size_t n = 64 + len * 8; n > 0; n--) {
        for (volatile uint8_t i = (v & 15) + 1; i; i--)
            ;
        v = pit();
        absorb(v);
        absorb_tsc();
    }
    permute();
}

int
getentropy(void *buf, size_t len)
{
    if (len > 256) {
        errno = EIO;
        return -1;
    }
    collect(len);
    uint8_t *p = buf;
    while (len > 0) {
        size_t n = len < RATE * 4 ? len : RATE * 4;
        memcpy(p, state, n);
        p += n;
        len -= n;
        permute();
    }
    return 0;
}
