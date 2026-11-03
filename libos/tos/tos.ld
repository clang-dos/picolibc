/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 *
 * Linker script for Atari TOS programs. GEMDOS loads the text, data and
 * bss one after another and starts the program at the first byte of the
 * text, so: the startup code first, then the rest of the code and the
 * read-only data (the PRG text), then the constructor tables and the data
 * (the PRG data), then the bss. llvm-elf2tos turns the result into a PRG
 * file, with a fixup for each 32-bit address.
 */

ENTRY(_start)

SECTIONS
{
	. = 0;

	.text : {
		KEEP (*(.text.startup))
		*(.text.unlikely .text.unlikely.*)
		*(.text .text.*)
	}

	.rodata : ALIGN(2) {
		*(.rodata .rodata.*)
		*(.srodata .srodata.*)
	}

	.init_array : ALIGN(2) {
		PROVIDE_HIDDEN (__bothinit_array_start = .);
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP (*(.init_array .ctors))
		PROVIDE_HIDDEN (__init_array_end = .);
		PROVIDE_HIDDEN (__bothinit_array_end = .);
	}

	.fini_array : ALIGN(2) {
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
		KEEP (*(.fini_array .dtors))
		PROVIDE_HIDDEN (__fini_array_end = .);
	}

	.data : ALIGN(2) {
		*(.data .data.*)
		*(.sdata .sdata.*)
		. = ALIGN(2);
	}

	.bss (NOLOAD) : ALIGN(2) {
		PROVIDE_HIDDEN (__bss_start = .);
		*(.bss .bss.*)
		*(.sbss .sbss.*)
		*(COMMON)
		. = ALIGN(2);
		PROVIDE_HIDDEN (__bss_end = .);
	}
	PROVIDE (_end = .);
}
