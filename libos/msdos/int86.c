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
#include <i86.h>

struct __msdos_iregs {
    uint16_t ax, bx, cx, dx, si, di, ds, es, flags, bp;
};

void __msdos_intr(unsigned intno, struct __msdos_iregs *r);

int
int86x(int intno, union REGS *in, union REGS *out, struct SREGS *sr)
{
    struct __msdos_iregs r;
    r.ax = in->x.ax;
    r.bx = in->x.bx;
    r.cx = in->x.cx;
    r.dx = in->x.dx;
    r.si = in->x.si;
    r.di = in->x.di;
    r.ds = sr->ds;
    r.es = sr->es;
    r.bp = 0;
    __msdos_intr((unsigned)intno & 0xff, &r);
    out->x.ax = r.ax;
    out->x.bx = r.bx;
    out->x.cx = r.cx;
    out->x.dx = r.dx;
    out->x.si = r.si;
    out->x.di = r.di;
    out->x.flags = r.flags;
    out->x.cflag = r.flags & 1;
    sr->ds = r.ds;
    sr->es = r.es;
    return r.ax;
}

int
int86(int intno, union REGS *in, union REGS *out)
{
    struct SREGS sr;
    segread(&sr);
    sr.es = sr.ds;
    return int86x(intno, in, out, &sr);
}

int
intdos(union REGS *in, union REGS *out)
{
    return int86(0x21, in, out);
}

int
intdosx(union REGS *in, union REGS *out, struct SREGS *sr)
{
    return int86x(0x21, in, out, sr);
}

void
segread(struct SREGS *sr)
{
    uint16_t cs, ss, ds, es;
    __asm__("movw %%cs, %0" : "=r"(cs));
    __asm__("movw %%ss, %0" : "=r"(ss));
    __asm__("movw %%ds, %0" : "=r"(ds));
    __asm__("movw %%es, %0" : "=r"(es));
    sr->cs = cs;
    sr->ss = ss;
    sr->ds = ds;
    sr->es = es;
}
