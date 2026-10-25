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
 * (TSRs), as in Open Watcom and Microsoft C. */

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
void (__interrupt __far *_dos_getvect(unsigned __intno))();

/* Point interrupt __intno at __handler (INT 21h function 25h). */
void _dos_setvect(unsigned __intno, void (__interrupt __far *__handler)());

/* End the program and stay resident (INT 21h function 31h), keeping
 * __memsize paragraphs of 16 bytes from the PSP, or, with 0, the program
 * up to the end of its heap. __retcode is the exit status. atexit
 * functions and destructors do not run, and open files stay open. */
__attribute__((__noreturn__)) void _dos_keep(unsigned __retcode,
                                             unsigned __memsize);

/* Called from an interrupt function: restore the registers it saved (with
 * any changes made to them) and jump to __handler, as if the interrupt
 * had gone to __handler, which returns to the interrupted program. */
__attribute__((__noreturn__)) void
_chain_intr(void (__interrupt __far *__handler)());

#ifdef __cplusplus
}
#endif

#endif /* _DOS_H_ */
