/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 */

/* snprintf with the long long printf variant. On i8086 the compiler calls
 * __l_snprintf instead of snprintf when the format string allows it, so
 * that programs which do not use floating point avoid linking it. */

#define vfprintf __l_vfprintf
#define snprintf __l_snprintf
#define __snprintfieee128 __l_snprintfieee128

#include "../../../libc/stdio/snprintf.c"
