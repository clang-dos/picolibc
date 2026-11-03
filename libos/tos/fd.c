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
#include <unistd.h>

/* fd 0 and 1 are GEMDOS's standard input and output (handles 0 and 1).
 * GEMDOS handle 2 is the serial port, so fd 2, the standard error, is
 * the console device (handle -1). */
struct __tos_fd __tos_fds[__TOS_MAX_FD] = {
    { 0, __TOS_FD_OPEN },
    { 1, __TOS_FD_OPEN },
    { -1, __TOS_FD_OPEN | __TOS_FD_TTY },
};

struct __tos_fd *
__tos_fd(int fd)
{
    if (fd < 0 || fd >= __TOS_MAX_FD || !(__tos_fds[fd].flags & __TOS_FD_OPEN)) {
        errno = EBADF;
        return NULL;
    }
    return &__tos_fds[fd];
}

int
__tos_new_fd(short handle, int flags)
{
    for (int fd = 0; fd < __TOS_MAX_FD; fd++) {
        if (!(__tos_fds[fd].flags & __TOS_FD_OPEN)) {
            __tos_fds[fd].handle = handle;
            __tos_fds[fd].flags = flags | __TOS_FD_OPEN;
            return fd;
        }
    }
    errno = EMFILE;
    return -1;
}

/* Character devices (the console, the serial port, the printer) have
 * negative handles, or don't move when sought: Fseek() on them returns
 * 0. A file returns its new position, or an error past its end. */
int
__tos_is_tty(struct __tos_fd *f)
{
    if (f->flags & __TOS_FD_TTY)
        return 1;
    if (f->flags & __TOS_FD_NOTTY)
        return 0;
    int  tty = f->handle < 0;
    if (!tty) {
        long pos = __tos_Fseek(0L, f->handle, SEEK_CUR);
        if (pos == 0 && __tos_Fseek(1L, f->handle, SEEK_CUR) == 0)
            tty = 1;
        else if (pos >= 0)
            __tos_Fseek(pos, f->handle, SEEK_SET);
    }
    f->flags |= tty ? __TOS_FD_TTY : __TOS_FD_NOTTY;
    return tty;
}

int
close(int fd)
{
    struct __tos_fd *f = __tos_fd(fd);
    if (!f)
        return -1;
    long r = 0;
    if (f->flags & __TOS_FD_OWNED)
        r = __tos_Fclose(f->handle);
    f->flags = 0;
    f->handle = 0;
    return r < 0 ? __tos_error(r) : 0;
}

void
__tos_close_fds(void)
{
    for (int fd = 0; fd < __TOS_MAX_FD; fd++)
        if (__tos_fds[fd].flags & __TOS_FD_OWNED)
            close(fd);
}

int
dup(int fd)
{
    struct __tos_fd *f = __tos_fd(fd);
    if (!f)
        return -1;
    /* The copy shares the GEMDOS handle; only the original closes it. */
    return __tos_new_fd(f->handle, f->flags & (__TOS_FD_TTY | __TOS_FD_NOTTY));
}

int
dup2(int fd, int fd2)
{
    struct __tos_fd *f = __tos_fd(fd);
    if (!f)
        return -1;
    if (fd2 < 0 || fd2 >= __TOS_MAX_FD) {
        errno = EBADF;
        return -1;
    }
    if (fd == fd2)
        return fd2;
    if (__tos_fds[fd2].flags & __TOS_FD_OPEN)
        close(fd2);
    __tos_fds[fd2].handle = f->handle;
    __tos_fds[fd2].flags = __TOS_FD_OPEN | (f->flags & (__TOS_FD_TTY | __TOS_FD_NOTTY));
    return fd2;
}

int
isatty(int fd)
{
    struct __tos_fd *f = __tos_fd(fd);
    if (!f)
        return 0;
    if (__tos_is_tty(f))
        return 1;
    errno = ENOTTY;
    return 0;
}

int
fcntl(int fd, int cmd, ...)
{
    struct __tos_fd *f = __tos_fd(fd);
    if (!f)
        return -1;
    switch (cmd) {
    case F_GETFD:
    case F_SETFD:
        return 0;
    case F_GETFL:
        return O_RDWR | ((f->flags & __TOS_FD_APPEND) ? O_APPEND : 0);
    default:
        errno = EINVAL;
        return -1;
    }
}
