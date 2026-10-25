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
 * void _chain_intr(void (__interrupt __far *handler)())
 *
 * Called from an interrupt function, whose BP points at the registers its
 * prologue saved (union INTPACK in <i86.h>):
 *
 *   0 gs, 2 fs, 4 es, 6 ds, 8 di, 10 si, 12 bp, 14 sp, 16 bx, 18 dx,
 *   20 cx, 22 ax, 24 ip, 26 cs, 28 flags
 *
 * The handler's address goes in the cx and ax slots, just below the
 * interrupt's return address, and cx and ax in the gs and fs slots. With
 * SP at BP, the registers are popped and RETF jumps to the handler, with
 * the flags of the interrupt (IF and TF clear, as INT leaves them) and
 * the interrupt's IP, CS and flags on the stack for its IRET.
 */

#if defined(__MEDIUM__) || defined(__LARGE__) || defined(__HUGE__)
#define ARG	4	/* after a far return address */
#else
#define ARG	2
#endif

#ifndef __WINDOWS__
	.text
	.globl	__chain_intr
__chain_intr:
	movw	%sp, %bx
	movw	%ss:ARG(%bx), %ax	/* the handler's offset */
	movw	%ss:ARG+2(%bx), %dx	/* and segment */
	movw	%bp, %sp
	xchgw	%ax, 20(%bp)		/* ax = the saved cx */
	xchgw	%dx, 22(%bp)		/* dx = the saved ax */
	movw	%ax, 0(%bp)
	movw	%dx, 2(%bp)
	movw	28(%bp), %ax
	andw	$0xfcff, %ax		/* clear IF and TF */
	pushw	%ax
	popfw
	popw	%cx
	popw	%ax
	popw	%es
	popw	%ds
	popw	%di
	popw	%si
	popw	%bp
	popw	%bx			/* the saved SP */
	popw	%bx
	popw	%dx
	lretw
#endif
