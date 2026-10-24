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
 * Startup code for 16-bit Windows DLLs (clang -shared), linked first in
 * place of crt0.obj. Windows calls the entry point once, when it loads the
 * DLL, with DS = the DLL's DGROUP, DI = its instance handle, CX = the local
 * heap size (heapsize=), ES:SI = the command line, and the caller's stack:
 * SS is not DS. It returns AX = nonzero for success with a far return.
 */

#if defined(__MEDIUM__) || defined(__LARGE__)
#define FAR_CODE 1
#endif

#ifdef __WINDOWS__
	/* The first 16 bytes of DGROUP belong to Windows, as in a program. */
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
	/* Clear BSS first: the values saved below are in it. */
	pushw	%es
	pushw	%si
	pushw	%cx
	pushw	%di
	pushw	%ds
	popw	%es
	cld
	movw	$___bss_start, %di
	movw	$___bss_end, %cx
	subw	%di, %cx
	xorw	%ax, %ax
	rep stosb
	popw	%di
	popw	%cx
	popw	%si
	popw	%es
	movw	%di, ___win16_hinstance
	movw	%cx, ___win16_heapsize
	movw	%si, ___win16_libcmdline
	movw	%es, ___win16_libcmdline+2
	/* LocalInit(DS, 0, heap size - 1) sets up the local heap. */
	jcxz	1f
	pushw	%ds
	xorw	%ax, %ax
	pushw	%ax
	decw	%cx
	pushw	%cx
	lcallw	$LOCALINIT@SEG, $LOCALINIT
	testw	%ax, %ax
	jz	9f
1:
#ifdef FAR_CODE
	lcallw	$___win16_libstart@SEG, $___win16_libstart
#else
	call	___win16_libstart
#endif
	lretw
9:	xorw	%ax, %ax
	lretw

	/* WEP, the exit procedure, comes from the C library. */
	.data
	.p2align 1
	.long	WEP

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

	.section .fardata,"aw",@progbits
	.section .farbss,"aw",@nobits

	.bss
___bss_start:
	.section .stack,"aw",@nobits
___bss_end:
#endif
