/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 */

/* asprintf with the integer printf variant. On i8086 the compiler calls
 * __i_asprintf instead of asprintf when the format string allows it, so
 * that programs which do not use floating point avoid linking it. */

#define vfprintf __i_vfprintf
#define asprintf __i_asprintf
#define __asprintfieee128 __i_asprintfieee128

#include "../../../libc/stdio/asprintf.c"
