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
 * Startup code for 32-bit OS/2 DLLs (LX), linked first. The linker marks
 * the DLL to be initialized and terminated for each process that uses it;
 * OS/2 calls _dll_start with the module handle and 0 (load) or 1 (free) on
 * the stack, and wants 1 back for success.
 */
	/* Undefined weak symbols resolve to this absolute 0 (JWlink). */
	.globl	__elf_weak_null
	.set	__elf_weak_null, 0

	.text
	.globl	_dll_start
_dll_start:
	cmpl	$0, 8(%esp)
	jne	1f
	call	__libc_init_array
	movl	$1, %eax
	ret
1:	call	__libc_fini_array
	movl	$1, %eax
	ret

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
