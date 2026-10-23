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
 * Real-mode helpers for the DOS system calls: arguments on the stack, SI,
 * DI, BP and DS preserved. In the far-code models (medium, large, huge)
 * they are far functions: a 4-byte return address and RETF. In the
 * far-data models (compact, large, huge) pointer arguments are far.
 */

#if defined(__MEDIUM__) || defined(__LARGE__) || defined(__HUGE__)
#define ARG(n)	(6 + (n))(%bp)
#define RETURN	lretw
#else
#define ARG(n)	(4 + (n))(%bp)
#define RETURN	retw
#endif
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
#define FAR_DATA 1
#define PTR	4	/* bytes of a pointer argument */
#else
#define PTR	2
#endif

	.text

/* int __msdos_int21(struct __msdos_regs *r)
 *
 * struct __msdos_regs: ax 0, bx 2, cx 4, dx 6, si 8, di 10, ds 12, es 14.
 * Locals: -6(%bp) the caller's DS, -10(%bp) r (offset, then segment). */
	.globl	___msdos_int21
___msdos_int21:
	pushw	%bp
	movw	%sp, %bp
	pushw	%si
	pushw	%di
	pushw	%ds
#ifdef FAR_DATA
	lesw	ARG(0), %bx
#else
	movw	ARG(0), %bx
	pushw	%ds
	popw	%es
#endif
	pushw	%es		/* -8(%bp) */
	pushw	%bx		/* -10(%bp) */
	movw	%es:4(%bx), %cx
	movw	%es:6(%bx), %dx
	movw	%es:8(%bx), %si
	movw	%es:10(%bx), %di
	movw	%es:12(%bx), %ax	/* ds, or 0 for DGROUP */
	testw	%ax, %ax
	jnz	1f
	movw	-6(%bp), %ax
1:	pushw	%ax		/* -12(%bp) */
	movw	%es:14(%bx), %ax	/* es, or 0 for DGROUP */
	testw	%ax, %ax
	jnz	1f
	movw	-6(%bp), %ax
1:	pushw	%ax		/* -14(%bp) */
	movw	%es:(%bx), %ax
	movw	%es:2(%bx), %bx
	popw	%es
	popw	%ds
	int	$0x21
	/* Keep the flags until the carry is read: only MOV, PUSH, POP and
	 * LES until SBB. */
	movw	-6(%bp), %ds
	pushw	%es		/* -12(%bp): the result ES */
	pushw	%bx		/* -14(%bp): the result BX */
	lesw	-10(%bp), %bx
	movw	%ax, %es:(%bx)
	movw	%cx, %es:4(%bx)
	movw	%dx, %es:6(%bx)
	movw	%si, %es:8(%bx)
	movw	%di, %es:10(%bx)
	popw	%ax
	movw	%ax, %es:2(%bx)
	popw	%ax
	movw	%ax, %es:14(%bx)
	sbbw	%ax, %ax
	negw	%ax
	leaw	-6(%bp), %sp
	popw	%ds
	popw	%di
	popw	%si
	popw	%bp
	RETURN

/* void __msdos_fromfar(void *dst, uint16_t seg, uint16_t off, size_t n) */
	.globl	___msdos_fromfar
___msdos_fromfar:
	pushw	%bp
	movw	%sp, %bp
	pushw	%si
	pushw	%di
	pushw	%ds
#ifdef FAR_DATA
	lesw	ARG(0), %di
#else
	pushw	%ds
	popw	%es
	movw	ARG(0), %di
#endif
	movw	ARG(PTR + 2), %si
	movw	ARG(PTR + 4), %cx
	movw	ARG(PTR), %ds
	cld
	rep movsb
	popw	%ds
	popw	%di
	popw	%si
	popw	%bp
	RETURN
