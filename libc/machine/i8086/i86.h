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


/* Open Watcom's <i86.h>: the registers an interrupt function saves, and
 * interrupts off and on. */

#ifndef _I86_H_
#define _I86_H_

#ifdef __cplusplus
extern "C" {
#endif

/* The parameter of an interrupt function: the registers its prologue
 * saved, which it can change before they are restored.
 *
 *     void __interrupt __far handler(union INTPACK r) { r.w.ax = 1; }
 *
 * gs and fs hold nothing on the 8086. */
struct INTPACKW {
    unsigned short gs, fs, es, ds, di, si, bp, sp, bx, dx, cx, ax, ip, cs;
    unsigned       flags;
};
struct INTPACKB {
    unsigned short : 16, : 16, : 16, : 16, : 16, : 16, : 16, : 16;
    unsigned char  bl, bh, dl, dh, cl, ch, al, ah;
};
union INTPACK {
    struct INTPACKB h;
    struct INTPACKW w;
    struct INTPACKW x;
};

/* The bits of the flags. */
enum {
    INTR_CF = 0x0001, /* carry */
    INTR_PF = 0x0004, /* parity */
    INTR_AF = 0x0010, /* auxiliary carry */
    INTR_ZF = 0x0040, /* zero */
    INTR_SF = 0x0080, /* sign */
    INTR_TF = 0x0100, /* trace */
    INTR_IF = 0x0200, /* interrupt */
    INTR_DF = 0x0400, /* direction */
    INTR_OF = 0x0800  /* overflow */
};

/* Interrupts off (CLI) and on (STI). */
static __inline__ void
_disable(void)
{
    __asm__ __volatile__("cli" ::: "memory");
}

static __inline__ void
_enable(void)
{
    __asm__ __volatile__("sti" ::: "memory");
}

/* Far pointers from a segment and an offset, and back. */
#ifndef MK_FP
#define MK_FP(__s, __o)                                                   \
    ((void __far *)(((unsigned long)(unsigned short)(__s) << 16) |        \
                    (unsigned short)(__o)))
#define FP_SEG(__p) ((unsigned short)((unsigned long)(void __far *)(__p) >> 16))
#define FP_OFF(__p) ((unsigned short)(unsigned long)(void __far *)(__p))
#endif

#ifdef __cplusplus
}
#endif

#endif /* _I86_H_ */
