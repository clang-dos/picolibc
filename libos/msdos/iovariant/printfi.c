/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 */

/* printf with the integer printf variant. On i8086 the compiler calls
 * __i_printf instead of printf when the format string allows it, so
 * that programs which do not use floating point avoid linking it. */

#define vfprintf __i_vfprintf
#define printf __i_printf
#define __printfieee128 __i_printfieee128

#include "../../../libc/stdio/printf.c"
