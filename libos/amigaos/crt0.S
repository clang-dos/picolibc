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
 * Startup code for AmigaOS programs, linked first.
 *
 * LoadSeg() starts a program at the first byte of its first hunk. From
 * the CLI, a0 points to the argument string and d0 holds its length.
 * From Workbench the registers mean nothing, and Workbench sends a
 * startup message to the process's message port, which the program
 * replies to when it ends.
 *
 * _start saves the registers it was given and the stack pointer, so that
 * __amiga_return can go back to the CLI or Workbench from anywhere.
 */

	.section .text.startup,"ax",@progbits
	.globl	_start
	.type	_start,@function
_start:
	lea	(-44,%sp),%sp
	movem.l	%d2-%d7/%a2-%a6,(%sp)
	move.l	%sp,__amiga_initial_sp
	move.l	%a0,%d2			/* argument string */
	move.l	%d0,%d3			/* its length */
	move.l	4,%a6
	move.l	%a6,SysBase
	move.l	#0,%a1
	jsr	(-294,%a6)		/* FindTask(NULL) */
	move.l	%d0,%a2
	move.l	(172,%a2),%d0		/* pr_CLI */
	bne	.Lcli
	lea	(92,%a2),%a0		/* pr_MsgPort */
	jsr	(-384,%a6)		/* WaitPort */
	lea	(92,%a2),%a0
	jsr	(-372,%a6)		/* GetMsg */
	move.l	%d0,_WBenchMsg
	moveq	#0,%d2
	moveq	#0,%d3
.Lcli:
	move.l	%d3,-(%sp)
	move.l	%d2,-(%sp)
	jsr	__amiga_start		/* does not return */
	.size	_start,.-_start

/* void __amiga_return(int status): end the program with status as its
 * return code, with the stack and registers that _start was given. */
	.section .text.__amiga_return,"ax",@progbits
	.globl	__amiga_return
	.type	__amiga_return,@function
__amiga_return:
	move.l	(4,%sp),%d0
	move.l	__amiga_initial_sp,%sp
	movem.l	(%sp),%d2-%d7/%a2-%a6
	lea	(44,%sp),%sp
	rts
	.size	__amiga_return,.-__amiga_return

	.section .bss.__amiga_initial_sp,"aw",@nobits
	.p2align 2
	.globl	__amiga_initial_sp
__amiga_initial_sp:
	.zero	4

	.section .note.GNU-stack,"",@progbits
