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

/* <dos.h> of the DOS compilers: interrupt vectors and resident programs
 * (TSRs), as in Open Watcom and Microsoft C, and the timing, sound, port
 * and memory functions of Turbo C. int86 and the registers are in
 * <i86.h>, which this includes. */

#ifndef _DOS_H_
#define _DOS_H_

#include <i86.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The segment of the program segment prefix. */
extern unsigned short __msdos_psp;
#define _psp __msdos_psp

/* The handler of interrupt __intno (INT 21h function 35h). */
void(__interrupt __far *_dos_getvect(unsigned __intno))();

/* Point interrupt __intno at __handler (INT 21h function 25h). */
void _dos_setvect(unsigned __intno, void(__interrupt __far *__handler)());

/* End the program and stay resident (INT 21h function 31h), keeping
 * __memsize paragraphs of 16 bytes from the PSP, or, with 0, the program
 * up to the end of its heap. __retcode is the exit status. atexit
 * functions and destructors do not run, and open files stay open. */
__attribute__((__noreturn__)) void _dos_keep(unsigned __retcode, unsigned __memsize);

/* Called from an interrupt function: restore the registers it saved (with
 * any changes made to them) and jump to __handler, as if the interrupt
 * had gone to __handler, which returns to the interrupted program. */
__attribute__((__noreturn__)) void _chain_intr(void(__interrupt __far *__handler)());

/* Turbo C's names for the vector functions. They are functions, not
 * macros, so that the names stay free for members and parameters (such as
 * a "bool enable" parameter in C++ code that includes this header). */
static __inline__ void(__interrupt __far *getvect(unsigned __intno))()
{
    return _dos_getvect(__intno);
}

static __inline__ void
setvect(unsigned __intno, void(__interrupt __far *__handler)())
{
    _dos_setvect(__intno, __handler);
}

static __inline__ void
disable(void)
{
    _disable();
}

static __inline__ void
enable(void)
{
    _enable();
}

/* Wait __ms milliseconds, timed by the 8253 timer. */
void delay(unsigned __ms);
/* Sound the PC speaker at __hz hertz, until nosound(). */
void sound(unsigned __hz);
void nosound(void);

/* I/O ports, as in Turbo C. */
static __inline__ unsigned char
inportb(unsigned __port)
{
    unsigned char __v;
    __asm__ __volatile__("inb %w1, %b0" : "=a"(__v) : "Nd"((unsigned short)__port));
    return __v;
}

static __inline__ unsigned
inport(unsigned __port)
{
    unsigned short __v;
    __asm__ __volatile__("inw %w1, %w0" : "=a"(__v) : "Nd"((unsigned short)__port));
    return __v;
}

static __inline__ void
outportb(unsigned __port, unsigned char __value)
{
    __asm__ __volatile__("outb %b0, %w1" : : "a"(__value), "Nd"((unsigned short)__port));
}

static __inline__ void
outport(unsigned __port, unsigned __value)
{
    __asm__ __volatile__("outw %w0, %w1"
                         :
                         : "a"((unsigned short)__value), "Nd"((unsigned short)__port));
}

/* Read and write memory at segment:offset, as in Turbo C. These are
 * functions too, since a peek() macro would break C++'s istream::peek(). */
static __inline__ int
peek(unsigned __s, unsigned __o)
{
    return *(int __far *)MK_FP(__s, __o);
}

static __inline__ char
peekb(unsigned __s, unsigned __o)
{
    return *(char __far *)MK_FP(__s, __o);
}

static __inline__ void
poke(unsigned __s, unsigned __o, int __v)
{
    *(int __far *)MK_FP(__s, __o) = __v;
}

static __inline__ void
pokeb(unsigned __s, unsigned __o, char __v)
{
    *(char __far *)MK_FP(__s, __o) = __v;
}

#ifdef __cplusplus
}
#endif

#endif /* _DOS_H_ */
