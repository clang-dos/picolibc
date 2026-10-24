/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 */

/* fscanf with the integer scanf variant. On i8086 the compiler calls
 * __i_fscanf instead of fscanf when the format string allows it, so
 * that programs which do not use floating point avoid linking it. */

#define vfscanf __i_vfscanf
#define fscanf __i_fscanf
#define __fscanfieee128 __i_fscanfieee128

#include "../../../libc/stdio/fscanf.c"
