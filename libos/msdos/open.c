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
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>

static int
dos_open(const char *path, int access)
{
    struct __msdos_regs r = { .ax = 0x3d00 | access, .dx = __MSDOS_OFF(path),
                              .ds = __MSDOS_SEG(path) };
    if (__msdos_int21(&r))
        return -(int)r.ax;
    return r.ax;
}

static int
dos_create(const char *path, uint16_t fn, int readonly)
{
    struct __msdos_regs r = { .ax = fn << 8, .cx = readonly ? 1 : 0,
                              .dx = __MSDOS_OFF(path),
                              .ds = __MSDOS_SEG(path) };
    if (__msdos_int21(&r))
        return -(int)r.ax;
    if (fn == 0x3c) {
        /* DOS empties an existing file here, but some emulators (emu2)
         * don't; writing 0 bytes at the start truncates it anyway. */
        uint16_t fd = r.ax;
        struct __msdos_regs w = { .ax = 0x4000, .bx = fd };
        __msdos_int21(&w);
        return fd;
    }
    return r.ax;
}

int
open(const char *path, int flags, ...)
{
    int mode = 0666;
    int fd;

    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
    }
    int access = flags & O_ACCMODE;
    int readonly = !(mode & 0222);

    if ((flags & O_CREAT) && (flags & O_EXCL)) {
        /* Create a new file; fails if it exists. */
        fd = dos_create(path, 0x5b, readonly);
    } else if ((flags & O_CREAT) && (flags & O_TRUNC)) {
        fd = dos_create(path, 0x3c, readonly);
    } else {
        fd = dos_open(path, access);
        if (fd == -2 && (flags & O_CREAT))
            fd = dos_create(path, 0x3c, readonly);
        else if (fd >= 0 && (flags & O_TRUNC) && access != O_RDONLY) {
            /* Writing 0 bytes truncates the file. */
            struct __msdos_regs r = { .ax = 0x4000, .bx = (uint16_t)fd };
            __msdos_int21(&r);
        }
    }
    if (fd < 0)
        return __msdos_error(-fd);
    int text = (flags & O_TEXT) || (!(flags & O_BINARY) && _fmode != O_BINARY);
    __msdos_set_fdflags(fd, ((flags & O_APPEND) ? __MSDOS_FD_APPEND : 0) |
                                (text ? __MSDOS_FD_TEXT : 0));
    return fd;
}
