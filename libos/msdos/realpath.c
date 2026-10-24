/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "local-msdos.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* The canonical name of a path, from INT 21h function 60h (DOS 3 and
 * later): the full path with the drive, in upper case, with "." and ".."
 * resolved. As on DOS, the separators are backslashes. The path must
 * exist, as POSIX requires. */
char *
realpath(const char * restrict path, char * restrict resolved)
{
    /* Static, not on the stack, which is not DGROUP in a DLL. */
    static char full[128];
    struct stat st;
    if (stat(path, &st) < 0)
        return NULL;
    struct __msdos_regs r = { .ax = 0x6000, .si = __MSDOS_OFF(path),
                              .ds = __MSDOS_SEG(path),
                              .di = __MSDOS_OFF(full),
                              .es = __MSDOS_SEG(full) };
    if (__msdos_int21(&r)) {
        __msdos_error(r.ax);
        return NULL;
    }
    size_t len = strlen(full) + 1;
    if (resolved == NULL) {
        resolved = malloc(len);
        if (resolved == NULL)
            return NULL;
    } else if (len > PATH_MAX) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    memcpy(resolved, full, len);
    return resolved;
}
