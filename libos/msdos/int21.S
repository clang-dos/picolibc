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
 * Real-mode helpers for the DOS system calls: near data, arguments on the
 * stack, SI, DI and BP preserved. In the medium memory model the helpers
 * are far functions: a 4-byte return address and RETF.
 */

#if defined(__MEDIUM__) || defined(__LARGE__) || defined(__HUGE__)
#define ARG(n)	(6 + (n))(%bp)
#define RETURN	lretw
#else
#define ARG(n)	(4 + (n))(%bp)
#define RETURN	retw
#endif

	.text

/* int __msdos_int21(struct __msdos_regs *r) */
	.globl	___msdos_int21
___msdos_int21:
	pushw	%bp
	movw	%sp, %bp
	pushw	%si
	pushw	%di
	movw	ARG(0), %bx
	pushw	%bx		/* r, at -6(%bp) */
	movw	(%bx), %ax
	movw	4(%bx), %cx
	movw	6(%bx), %dx
	movw	8(%bx), %si
	movw	10(%bx), %di
	movw	2(%bx), %bx
	pushw	%ds
	popw	%es
	int	$0x21
	/* Keep the flags until the carry is read. */
	xchgw	%bx, -6(%bp)	/* bx = r; the result bx is saved */
	movw	%ax, (%bx)
	movw	%cx, 4(%bx)
	movw	%dx, 6(%bx)
	movw	%si, 8(%bx)
	movw	%di, 10(%bx)
	popw	%ax
	movw	%ax, 2(%bx)
	sbbw	%ax, %ax
	negw	%ax
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
	popw	%es
	movw	ARG(0), %di
	movw	ARG(4), %si
	movw	ARG(6), %cx
	pushw	%ds
	movw	ARG(2), %ds
	cld
	rep movsb
	popw	%ds
	popw	%di
	popw	%si
	popw	%bp
	RETURN
