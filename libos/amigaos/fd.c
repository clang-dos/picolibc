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
#include <unistd.h>

struct __amiga_fd   __amiga_fds[__AMIGA_MAX_FD];
static __amiga_bptr console;

void                __amiga_init_fds(void);

/* fd 0 and 1 are the CLI's input and output, and fd 2 its error stream
 * (pr_CES, from version 36; the output before that). A program started
 * from Workbench has none of these: the first read or write of them opens
 * a console window. */
void
__amiga_init_fds(void)
{
    if (_WBenchMsg) {
        for (int fd = 0; fd < 3; fd++)
            __amiga_fds[fd].flags = __AMIGA_FD_OPEN | __AMIGA_FD_CON;
        return;
    }
    __amiga_bptr err = 0;
    if (__amiga_libversion(DOSBase) >= 36)
        err = __amiga_field(__amiga_FindTask(NULL), __AMIGA_PR_CES);
    __amiga_fds[0] = (struct __amiga_fd) { __amiga_Input(), __AMIGA_FD_OPEN };
    __amiga_fds[1] = (struct __amiga_fd) { __amiga_Output(), __AMIGA_FD_OPEN };
    __amiga_fds[2] = (struct __amiga_fd) { err ? err : __amiga_fds[1].fh, __AMIGA_FD_OPEN };
}

__amiga_bptr
__amiga_fh(int fd)
{
    if (fd < 0 || fd >= __AMIGA_MAX_FD || !(__amiga_fds[fd].flags & __AMIGA_FD_OPEN)) {
        errno = EBADF;
        return 0;
    }
    if (__amiga_fds[fd].flags & __AMIGA_FD_CON) {
        if (!console) {
            /* AUTO, CLOSE and WAIT are version 36 options. */
            if (__amiga_libversion(DOSBase) >= 36)
                console
                    = __amiga_Open("CON:0/20/640/150/Output/AUTO/CLOSE/WAIT", __AMIGA_MODE_OLDFILE);
            else
                console = __amiga_Open("CON:0/20/640/150/Output", __AMIGA_MODE_OLDFILE);
            if (!console) {
                errno = EIO;
                return 0;
            }
        }
        return console;
    }
    return __amiga_fds[fd].fh;
}

int
__amiga_new_fd(__amiga_bptr fh, int flags)
{
    for (int fd = 0; fd < __AMIGA_MAX_FD; fd++) {
        if (!(__amiga_fds[fd].flags & __AMIGA_FD_OPEN)) {
            __amiga_fds[fd].fh = fh;
            __amiga_fds[fd].flags = flags | __AMIGA_FD_OPEN;
            return fd;
        }
    }
    errno = EMFILE;
    return -1;
}

int
close(int fd)
{
    if (fd < 0 || fd >= __AMIGA_MAX_FD || !(__amiga_fds[fd].flags & __AMIGA_FD_OPEN)) {
        errno = EBADF;
        return -1;
    }
    int ok = 1;
    if (__amiga_fds[fd].flags & __AMIGA_FD_OWNED)
        ok = __amiga_Close(__amiga_fds[fd].fh) != 0;
    __amiga_fds[fd].flags = 0;
    __amiga_fds[fd].fh = 0;
    return ok ? 0 : __amiga_error(0);
}

void
__amiga_close_fds(void)
{
    for (int fd = 0; fd < __AMIGA_MAX_FD; fd++)
        if (__amiga_fds[fd].flags & __AMIGA_FD_OWNED)
            close(fd);
    if (console) {
        __amiga_Close(console);
        console = 0;
    }
}

int
dup(int fd)
{
    if (!__amiga_fh(fd))
        return -1;
    /* AmigaOS can't duplicate a file handle; the copy shares it and only
     * the original closes it. */
    return __amiga_new_fd(__amiga_fds[fd].fh, __amiga_fds[fd].flags & __AMIGA_FD_CON);
}

int
dup2(int fd, int fd2)
{
    if (!__amiga_fh(fd))
        return -1;
    if (fd2 < 0 || fd2 >= __AMIGA_MAX_FD) {
        errno = EBADF;
        return -1;
    }
    if (fd == fd2)
        return fd2;
    if (__amiga_fds[fd2].flags & __AMIGA_FD_OPEN)
        close(fd2);
    __amiga_fds[fd2].fh = __amiga_fds[fd].fh;
    __amiga_fds[fd2].flags = __AMIGA_FD_OPEN | (__amiga_fds[fd].flags & __AMIGA_FD_CON);
    return fd2;
}

int
isatty(int fd)
{
    __amiga_bptr fh = __amiga_fh(fd);
    if (!fh)
        return 0;
    if (__amiga_IsInteractive(fh))
        return 1;
    errno = ENOTTY;
    return 0;
}

int
fcntl(int fd, int cmd, ...)
{
    if (!__amiga_fh(fd))
        return -1;
    switch (cmd) {
    case F_GETFD:
    case F_SETFD:
        return 0;
    case F_GETFL:
        return O_RDWR | ((__amiga_fds[fd].flags & __AMIGA_FD_APPEND) ? O_APPEND : 0);
    default:
        errno = EINVAL;
        return -1;
    }
}
