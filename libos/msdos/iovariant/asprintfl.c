/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 */

/* asprintf with the long long printf variant. On i8086 the compiler calls
 * __l_asprintf instead of asprintf when the format string allows it, so
 * that programs which do not use floating point avoid linking it. */

#define vfprintf __l_vfprintf
#define asprintf __l_asprintf
#define __asprintfieee128 __l_asprintfieee128

#include "../../../libc/stdio/asprintf.c"
