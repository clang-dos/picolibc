/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 */

/* scanf with the long long scanf variant. On i8086 the compiler calls
 * __l_scanf instead of scanf when the format string allows it, so
 * that programs which do not use floating point avoid linking it. */

#define vfscanf __l_vfscanf
#define scanf __l_scanf
#define __scanfieee128 __l_scanfieee128

#include "../../../libc/stdio/scanf.c"
