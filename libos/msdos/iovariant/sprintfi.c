/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 */

/* sprintf with the integer printf variant. On i8086 the compiler calls
 * __i_sprintf instead of sprintf when the format string allows it, so
 * that programs which do not use floating point avoid linking it. */

#define vfprintf __i_vfprintf
#define sprintf __i_sprintf
#define __sprintfieee128 __i_sprintfieee128

#include "../../../libc/stdio/sprintf.c"
