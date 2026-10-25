/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 */

/* sprintf with the long long printf variant. On i8086 the compiler calls
 * __l_sprintf instead of sprintf when the format string allows it, so
 * that programs which do not use floating point avoid linking it. */

#define vfprintf __l_vfprintf
#define sprintf __l_sprintf
#define __sprintfieee128 __l_sprintfieee128

#include "../../../libc/stdio/sprintf.c"
