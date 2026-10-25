/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 */

/* fprintf with the long long printf variant. On i8086 the compiler calls
 * __l_fprintf instead of fprintf when the format string allows it, so
 * that programs which do not use floating point avoid linking it. */

#define vfprintf __l_vfprintf
#define fprintf __l_fprintf
#define __fprintfieee128 __l_fprintfieee128

#include "../../../libc/stdio/fprintf.c"
