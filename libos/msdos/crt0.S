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
 * Startup code for DOS programs, linked first.
 *
 * .COM (tiny model): DOS loads the program after the 256-byte PSP, with
 *   CS = DS = ES = SS = the PSP segment and SP at the top of the segment.
 * .EXE (other models): DS = ES = the PSP segment, and SS:SP is the end of
 *   the STACK segment. C code needs SS = DS = DGROUP, so SS:SP is moved
 *   onto DGROUP, whose segment comes from a base16 fixup.
 *
 * In the medium model code is far: each module has its own code segment
 * and C functions are called with CALL FAR.
 *
 * DGROUP is laid out as CONST, the constructor and destructor tables
 * (XIB XI XIE YIB YI YIE), _DATA, _BSS, the communal variables and STACK.
 * The heap starts after STACK.
 *
 * Far variables are in segments of classes FAR_DATA and FAR_BSS. FAR_DATA
 * is between the code and DGROUP. In the models with far data pointers,
 * FAR_BSS comes after STACK, so that it takes no room in the .EXE file, and
 * FAR_END ends the program; the heap starts after it. In the others FAR_BSS
 * is before DGROUP, whose end the heap uses.
 */

#ifndef __MSDOS_STACK_SIZE
#define __MSDOS_STACK_SIZE 4096
#endif

#if defined(__MEDIUM__) || defined(__LARGE__) || defined(__HUGE__)
#define FAR_CODE 1
#endif
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
#define FAR_DATA 1
#endif

	.text
#ifdef FAR_CODE
	/* Undefined weak symbols resolve here. Far pointers to them are
	 * 0000:0000, a null pointer. */
	.globl	__omf_weak_null
	.set	__omf_weak_null, 0
#else
	/* Undefined weak symbols resolve here, at offset 0, as in ELF. */
	.globl	__omf_weak_null
__omf_weak_null:
#endif
#ifdef __TINY__
	.org	0x100
#endif
	.globl	_start
_start:
#ifdef __TINY__
	movw	%cs, %bx		/* PSP */
#else
	movw	%ds, %bx		/* PSP */
	movw	$___bss_start@SEG, %dx	/* DGROUP */
	movw	%ss, %ax
	subw	%dx, %ax		/* paragraphs from DGROUP to STACK */
	movb	$4, %cl
	shlw	%cl, %ax
	cli
	movw	%dx, %ss
	addw	%ax, %sp
	sti
	movw	%dx, %ds
	movw	%dx, %es
#endif
	cld
	/* Clear _BSS and the communal variables. */
	movw	$___bss_start, %di
	movw	$___bss_end, %cx
	subw	%di, %cx
	xorw	%ax, %ax
	rep stosb
#ifdef FAR_DATA
	/* Clear far BSS, from ___farbss_start to the paragraph after
	 * ___far_end, 32 KiB at a time. */
	movw	$___farbss_start@SEG, %dx
	movw	$___far_end, %si
	addw	$15, %si
	movb	$4, %cl
	shrw	%cl, %si
	addw	$___far_end@SEG, %si
	subw	%dx, %si		/* paragraphs to clear */
1:	testw	%si, %si
	jz	3f
	movw	%si, %cx
	cmpw	$0x800, %cx
	jbe	2f
	movw	$0x800, %cx
2:	subw	%cx, %si
	movw	%dx, %es
	addw	%cx, %dx
	shlw	%cx
	shlw	%cx
	shlw	%cx			/* words */
	xorw	%di, %di
	rep stosw
	jmp	1b
3:	movw	%ds, %dx
	movw	%dx, %es
#endif
	movw	%bx, ___msdos_psp
	xorw	%bp, %bp
#ifdef FAR_CODE
	lcallw	$___msdos_start@SEG, $___msdos_start
#else
	call	___msdos_start
#endif
	/* Not reached: __msdos_start calls exit. */
	movw	$0x4cff, %ax
	int	$0x21

	/* Bounds of the constructor and destructor tables, for picolibc's
	 * __libc_init_array and __libc_fini_array. There is no separate
	 * preinit table. */
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

#ifndef __TINY__
	/* These empty segments place the far data classes: see above. */
	.section .fardata,"aw",@progbits
	.section .farbss,"aw",@nobits
#ifdef FAR_DATA
	.p2align 4
	.globl	___farbss_start
___farbss_start:
	.section .farend,"aw",@nobits
	.globl	___far_end, ___heap_start
___far_end:
___heap_start:
#endif
#endif

	.bss
___bss_start:

	.section .stack,"aw",@nobits
	.p2align 1
___bss_end:
#ifndef __TINY__
	/* The stack of a .EXE program. A .COM program uses the top of its
	 * segment. */
	.zero	__MSDOS_STACK_SIZE
#endif
#ifndef FAR_DATA
	.globl	___heap_start
___heap_start:
#endif
