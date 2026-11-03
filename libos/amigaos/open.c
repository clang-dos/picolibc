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

#include "local-amigaos.h"
#include <fcntl.h>
#include <stdarg.h>

static int
exists(const char *path)
{
    __amiga_bptr lock = __amiga_Lock(path, __AMIGA_SHARED_LOCK);
    if (lock)
        __amiga_UnLock(lock);
    return lock != 0;
}

int
open(const char *path, int flags, ...)
{
    int          acc = flags & O_ACCMODE;
    __amiga_bptr fh;

    if ((flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL) && exists(path)) {
        errno = EEXIST;
        return -1;
    }
    if ((flags & O_TRUNC) && acc != O_RDONLY) {
        /* MODE_NEWFILE truncates, or makes the file. */
        if (!(flags & O_CREAT) && !exists(path))
            return __amiga_error(0);
        fh = __amiga_Open(path, __AMIGA_MODE_NEWFILE);
    } else {
        /* MODE_OLDFILE opens an existing file for reading and writing. */
        fh = __amiga_Open(path, __AMIGA_MODE_OLDFILE);
        if (!fh && (flags & O_CREAT) && __amiga_IoErr() == 205 /* object not found */)
            fh = __amiga_Open(path, __AMIGA_MODE_NEWFILE);
    }
    if (!fh)
        return __amiga_error(0);

    int fd = __amiga_new_fd(fh, __AMIGA_FD_OWNED | ((flags & O_APPEND) ? __AMIGA_FD_APPEND : 0));
    if (fd < 0)
        __amiga_Close(fh);
    return fd;
}
