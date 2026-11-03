/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 *
 * setjmp and longjmp in the Motorola syntax that LLVM's assembler reads.
 * The jmp_buf layout is that of setjmp.S: the stack pointer at 8, the
 * return address at 12, and d2-d7/a2-a6 from 20.
 */
#include <picolibc.h>

	.section .text.setjmp,"ax",@progbits
	.globl	setjmp
	.type	setjmp,@function
setjmp:
	move.l	(4,%sp),%a0
	move.l	(%sp),(12,%a0)
	move.l	%sp,(8,%a0)
	movem.l	%d2-%d7/%a2-%a6,(20,%a0)
	moveq	#0,%d0
	rts
	.size	setjmp,.-setjmp

	.section .text.longjmp,"ax",@progbits
	.globl	longjmp
	.type	longjmp,@function
longjmp:
	move.l	(4,%sp),%a0
	move.l	(8,%sp),%d0
	bne	.Lnonzero
	moveq	#1,%d0
.Lnonzero:
	movem.l	(20,%a0),%d2-%d7/%a2-%a6
	move.l	(8,%a0),%sp
	move.l	(12,%a0),(%sp)
	rts
	.size	longjmp,.-longjmp

	.section .note.GNU-stack,"",@progbits
