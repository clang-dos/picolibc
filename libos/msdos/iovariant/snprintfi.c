/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 */

/* snprintf with the integer printf variant. On i8086 the compiler calls
 * __i_snprintf instead of snprintf when the format string allows it, so
 * that programs which do not use floating point avoid linking it. */

#define vfprintf __i_vfprintf
#define snprintf __i_snprintf
#define __snprintfieee128 __i_snprintfieee128

#include "../../../libc/stdio/snprintf.c"
