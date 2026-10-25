/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 */

/* scanf with the integer scanf variant. On i8086 the compiler calls
 * __i_scanf instead of scanf when the format string allows it, so
 * that programs which do not use floating point avoid linking it. */

#define vfscanf __i_vfscanf
#define scanf __i_scanf
#define __scanfieee128 __i_scanfieee128

#include "../../../libc/stdio/scanf.c"
