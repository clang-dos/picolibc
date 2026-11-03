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
 * Startup code for Atari TOS programs, linked first.
 *
 * GEMDOS starts a program at the first byte of its text with the address
 * of its basepage at 4(sp), and gives it all of the free memory (the TPA).
 * _start keeps what the program needs, its basepage, text, data, bss and
 * a stack of _stksize bytes, and gives the rest back with Mshrink(), so
 * that malloc() and other programs can have it.
 *
 * _stksize is 32K unless the program defines it, as with other Atari C
 * compilers: long _stksize = 65536; A value of 0 or less keeps the whole
 * TPA, with the stack at its top.
 */

	.section .text.startup,"ax",@progbits
	.globl	_start
	.type	_start,@function
_start:
	move.l	(4,%sp),%a0		/* the basepage */
	move.l	%a0,_base
	move.l	_stksize,%d0
	ble	.Lkeepall
	add.l	#0x100,%d0		/* basepage */
	add.l	(12,%a0),%d0		/* p_tlen */
	add.l	(20,%a0),%d0		/* p_dlen */
	add.l	(28,%a0),%d0		/* p_blen */
	and.l	#-4,%d0
	lea	(0,%a0,%d0.l),%sp
	move.l	%d0,-(%sp)		/* Mshrink(0, basepage, size) */
	move.l	%a0,-(%sp)
	clr.w	-(%sp)
	move.w	#0x4a,-(%sp)
	trap	#1
	lea	(12,%sp),%sp
	bra	.Lgo
.Lkeepall:
	move.l	(4,%a0),%sp		/* p_hitpa */
.Lgo:
	jsr	__tos_start		/* does not return */
	.size	_start,.-_start

	.section .data._stksize,"aw",@progbits
	.p2align 1
	.weak	_stksize
_stksize:
	.long	32768

	.section .bss._base,"aw",@nobits
	.p2align 1
	.globl	_base
_base:
	.zero	4

	.section .note.GNU-stack,"",@progbits
