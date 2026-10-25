/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 */

/* sscanf with the integer scanf variant. On i8086 the compiler calls
 * __i_sscanf instead of sscanf when the format string allows it, so
 * that programs which do not use floating point avoid linking it. */

#define vfscanf __i_vfscanf
#define sscanf __i_sscanf
#define __sscanfieee128 __i_sscanfieee128

#include "../../../libc/stdio/sscanf.c"
