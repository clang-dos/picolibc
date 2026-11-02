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
 * Call any software interrupt with every general register, DS, ES and BP
 * set, for int86x. The interrupt is entered as INT would enter it: the
 * flags, CS and the return offset are pushed and the handler is reached
 * through its vector with interrupts off. BP may come back changed (INT
 * 10h function 1130h returns a pointer in ES:BP), so the frame is found
 * again from SP.
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
#endif

	.text

/* void __msdos_intr(unsigned intno, struct __msdos_iregs *r)
 *
 * struct __msdos_iregs: ax 0, bx 2, cx 4, dx 6, si 8, di 10, ds 12,
 * es 14, flags 16, bp 18. Locals: -6(%bp) the caller's DS, -10(%bp) r
 * (offset, then segment). */
	.globl	___msdos_intr
___msdos_intr:
	pushw	%bp
	movw	%sp, %bp
	pushw	%si
	pushw	%di
	pushw	%ds
#ifdef FAR_DATA
	lesw	ARG(2), %bx
#else
	movw	ARG(2), %bx
	pushw	%ds
	popw	%es
#endif
	pushw	%es		/* -8(%bp) */
	pushw	%bx		/* -10(%bp) */
	/* The return frame of the interrupt. */
	pushfw
	pushw	%cs
	movw	$1f, %ax
	pushw	%ax
	/* The vector, for the far return that enters the handler. */
	movw	ARG(0), %si
	shlw	%si
	shlw	%si
	xorw	%ax, %ax
	movw	%ax, %ds
	pushw	2(%si)
	pushw	(%si)
	/* The registers. */
	movw	%es:4(%bx), %cx
	movw	%es:6(%bx), %dx
	movw	%es:8(%bx), %si
	movw	%es:10(%bx), %di
	pushw	%es:12(%bx)
	pushw	%es:14(%bx)
	pushw	%es:18(%bx)
	pushw	%es:(%bx)
	movw	%es:2(%bx), %bx
	popw	%ax
	popw	%bp
	popw	%es
	popw	%ds
	cli
	lretw
1:	pushfw			/* -12: the flags */
	pushw	%bp		/* -14 */
	pushw	%ds		/* -16 */
	pushw	%es		/* -18 */
	pushw	%bx		/* -20 */
	movw	%sp, %bp
	addw	$20, %bp
	movw	-6(%bp), %ds
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
	popw	%ax
	movw	%ax, %es:12(%bx)
	popw	%ax
	movw	%ax, %es:18(%bx)
	popw	%ax
	movw	%ax, %es:16(%bx)
	leaw	-6(%bp), %sp
	popw	%ds
	popw	%di
	popw	%si
	popw	%bp
	RETURN
