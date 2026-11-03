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

#include "local-tos.h"
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

int
open(const char *path, int flags, ...)
{
    char buf[__TOS_PATH_MAX];
    char *name = __tos_path(path, buf);
    if (!name)
        return -1;
    int  acc = flags & O_ACCMODE;
    long attr = __tos_Fattrib(name, 0, 0);
    int  exists = attr >= 0;
    long h;

    if (exists && (attr & __TOS_FA_DIR)) {
        errno = EISDIR;
        return -1;
    }
    if (exists && (flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL)) {
        errno = EEXIST;
        return -1;
    }
    if (!exists && !(flags & O_CREAT))
        return __tos_error(attr);
    if (!exists || ((flags & O_TRUNC) && acc != O_RDONLY)) {
        /* Fcreate() makes the file, or truncates it, open for writing. */
        h = __tos_Fcreate(name, 0);
        if (h >= 0 && acc != O_WRONLY) {
            /* Reopen it to read as well. */
            __tos_Fclose((short)h);
            h = __tos_Fopen(name, acc == O_RDONLY ? 0 : 2);
        }
    } else {
        h = __tos_Fopen(name, acc == O_RDONLY ? 0 : acc == O_WRONLY ? 1 : 2);
    }
    if (h < 0)
        return __tos_error(h);

    int fd = __tos_new_fd((short)h, __TOS_FD_OWNED | ((flags & O_APPEND) ? __TOS_FD_APPEND : 0));
    if (fd < 0)
        __tos_Fclose((short)h);
    return fd;
}
