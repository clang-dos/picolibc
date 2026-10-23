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
 * setjmp and longjmp for 16-bit real mode. Arguments are on the stack, the
 * caller pops them, and SI, DI and BP are preserved across calls. In the
 * far-code models (medium, large, huge) they are far functions, and in the
 * far-data models (compact, large, huge) env is a far pointer.
 *
 * jmp_buf: SI, DI, BP, SP after return, IP, and CS for far code.
 */

#if defined(__MEDIUM__) || defined(__LARGE__) || defined(__HUGE__)
#define FAR_CODE 1
#define RA	4	/* bytes of the return address */
#else
#define RA	2
#endif
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
#define FAR_DATA 1
#define PTR	4
#else
#define PTR	2
#endif

/* ES:BX = env, which is at off(%bx). */
#ifdef FAR_DATA
#define LOAD_ENV(off)	lesw	off(%bx), %bx
#else
#define LOAD_ENV(off)	movw	off(%bx), %bx; pushw %ds; popw %es
#endif

	.text
	.globl	_setjmp
_setjmp:
	popw	%cx		/* return address */
#ifdef FAR_CODE
	popw	%dx		/* return segment */
#endif
	movw	%sp, %bx
	LOAD_ENV(0)
	movw	%si, %es:(%bx)
	movw	%di, %es:2(%bx)
	movw	%bp, %es:4(%bx)
	movw	%sp, %es:6(%bx)
	movw	%cx, %es:8(%bx)
	xorw	%ax, %ax
#ifdef FAR_CODE
	movw	%dx, %es:10(%bx)
	pushw	%dx
	pushw	%cx
	lretw
#else
	jmpw	*%cx
#endif

	.globl	_longjmp
_longjmp:
	movw	%sp, %bx
	movw	(RA + PTR)(%bx), %ax	/* val */
	LOAD_ENV(RA)
	testw	%ax, %ax
	jnz	1f
	incw	%ax		/* longjmp(env, 0) returns 1 */
1:	movw	%es:(%bx), %si
	movw	%es:2(%bx), %di
	movw	%es:4(%bx), %bp
	movw	%es:6(%bx), %sp
#ifdef FAR_CODE
	ljmpw	*%es:8(%bx)
#else
	jmpw	*%es:8(%bx)
#endif
