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
 * Startup code for OS/2 programs, linked first.
 *
 * 2.x (i386, LX): OS/2 calls _start with the module handle, a zero, the
 * environment and the command line on the stack, after the return
 * address.
 *
 * 1.x (i8086, NE): OS/2 starts the program with AX = the environment's
 * selector, BX = the offset of the command line in it, CX = the size of
 * DGROUP, DS = SS = DGROUP and SP at the top of the stack. DGROUP is laid
 * out as for DOS (see libos/msdos/crt0.S): CONST, the constructor and
 * destructor tables, _DATA, _BSS and STACK; the heap follows.
 */
#ifdef __i386__
	/* Undefined weak symbols resolve to this absolute 0 (JWlink). */
	.globl	__elf_weak_null
	.set	__elf_weak_null, 0

	.text
	.globl	_start
_start:
	movl	12(%esp), %eax
	movl	%eax, __os2_env
	movl	16(%esp), %eax
	movl	%eax, __os2_cmdline
	xorl	%ebp, %ebp
	cld
	call	__os2_start
	/* Not reached: __os2_start calls exit. */
	pushl	$255
	pushl	$1
	call	DosExit

	/* The constructor and destructor tables. The linker puts a module's
	 * .init_array and .fini_array after the ones here, which mark their
	 * start, and before the end markers, which come later. */
	.section .init_array.a,"aw"
	.p2align 2
	.globl	__bothinit_array_start, __preinit_array_start
	.globl	__preinit_array_end, __init_array_start
__bothinit_array_start:
__preinit_array_start:
__preinit_array_end:
__init_array_start:
	.section .init_array,"aw"
	.section .init_array.z,"aw"
	.p2align 2
	.globl	__bothinit_array_end, __init_array_end
__bothinit_array_end:
__init_array_end:
	.section .fini_array.a,"aw"
	.p2align 2
	.globl	__fini_array_start
__fini_array_start:
	.section .fini_array,"aw"
	.section .fini_array.z,"aw"
	.p2align 2
	.globl	__fini_array_end
__fini_array_end:

	/* The stack of the first thread: the linker's option stack sets its
	 * size. */
	.section .stack,"aw",@nobits
	.p2align 4
	.zero	16
#else /* 16-bit OS/2 1.x */
#if defined(__MEDIUM__) || defined(__LARGE__)
#define FAR_CODE 1
#endif
#if defined(__COMPACT__) || defined(__LARGE__)
#define FAR_DATA 1
#endif
	/* No variable may be at offset 0 of DGROUP, where a near pointer to
	 * it would be a null pointer. */
	.section .rodata
	.zero	16

	.text
#ifdef FAR_CODE
	.globl	__omf_weak_null
	.set	__omf_weak_null, 0
#else
	.globl	__omf_weak_null
__omf_weak_null:
#endif
	.globl	_start
_start:
	cld
	movw	%ax, %dx		/* environment selector */
	/* Clear _BSS and the communal variables. */
	pushw	%ds
	popw	%es
	movw	$___bss_start, %di
	movw	$___bss_end, %cx
	subw	%di, %cx
	xorw	%ax, %ax
	rep stosb
	movw	%sp, ___os2_stack_top
	/* Far pointers to the environment and the command line. */
	movw	%ax, ___os2_env		/* offset 0 (AX is 0 here) */
	movw	%dx, ___os2_env+2
	movw	%bx, ___os2_cmdline
	movw	%dx, ___os2_cmdline+2
	xorw	%bp, %bp
#ifdef FAR_CODE
	lcallw	$___os2_start@SEG, $___os2_start
#else
	call	___os2_start
#endif
	/* Not reached: __os2_start calls exit. */
	movw	$1, %ax
	pushw	%ax
	movw	$255, %ax
	pushw	%ax
	lcallw	$DOSEXIT@SEG, $DOSEXIT

	.data
	.globl	___os2_stack_top
	.p2align 1
___os2_stack_top:
	.short	0

	.section XIB,"aw"
	.p2align 1
	.globl	___bothinit_array_start, ___preinit_array_start
	.globl	___preinit_array_end, ___init_array_start
___bothinit_array_start:
___preinit_array_start:
___preinit_array_end:
___init_array_start:
	.section XIE,"aw"
	.p2align 1
	.globl	___bothinit_array_end, ___init_array_end
___bothinit_array_end:
___init_array_end:
	.section YIB,"aw"
	.p2align 1
	.globl	___fini_array_start
___fini_array_start:
	.section YIE,"aw"
	.p2align 1
	.globl	___fini_array_end
___fini_array_end:
	/* These empty segments place the far data classes. */
	.section .fardata,"aw",@progbits
	.section .farbss,"aw",@nobits
	.bss
___bss_start:
	.section .stack,"aw",@nobits
	.p2align 1
___bss_end:
	.globl	___heap_start
___heap_start:
#endif
