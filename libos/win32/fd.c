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

#include "local-win32.h"
#include <fcntl.h>
#include "include/io.h"
#include <unistd.h>

struct __win32_fd __win32_fds[__WIN32_MAX_FD];

/* The mode for open() calls that give neither O_TEXT nor O_BINARY. */
int               _fmode = O_TEXT;

int
__win32_type_flags(__win32_handle h)
{
    switch (GetFileType(h)) {
    case __WIN32_FILE_TYPE_CHAR:
        return __WIN32_FD_DEVICE;
    case __WIN32_FILE_TYPE_PIPE:
        return __WIN32_FD_PIPE;
    default:
        return 0;
    }
}

/* stdin, stdout and stderr are the standard handles, in text mode. A GUI
 * program may have none; its descriptors 0 to 2 then stay closed. */
static const __win32_dword std_handles[3]
    = { __WIN32_STD_INPUT_HANDLE, __WIN32_STD_OUTPUT_HANDLE, __WIN32_STD_ERROR_HANDLE };

void
__win32_init_fds(void)
{
    for (int fd = 0; fd < 3; fd++) {
        __win32_handle h = GetStdHandle(std_handles[fd]);
        if (h == NULL || h == __WIN32_INVALID_HANDLE)
            continue;
        __win32_fds[fd].handle = h;
        __win32_fds[fd].flags
            = __WIN32_FD_OPEN | __WIN32_FD_TEXT | __WIN32_FD_NOCLOSE | __win32_type_flags(h);
    }
}

__win32_handle
__win32_handle_of(int fd)
{
    if (fd < 0 || fd >= __WIN32_MAX_FD || !(__win32_fds[fd].flags & __WIN32_FD_OPEN)) {
        errno = EBADF;
        return NULL;
    }
    return __win32_fds[fd].handle;
}

int
__win32_new_fd(__win32_handle h, int flags)
{
    for (int fd = 0; fd < __WIN32_MAX_FD; fd++) {
        if (!(__win32_fds[fd].flags & __WIN32_FD_OPEN)) {
            __win32_fds[fd].handle = h;
            __win32_fds[fd].flags = flags | __WIN32_FD_OPEN;
            return fd;
        }
    }
    errno = EMFILE;
    return -1;
}

int
close(int fd)
{
    __win32_handle h = __win32_handle_of(fd);
    if (!h)
        return -1;
    int flags = __win32_fds[fd].flags;
    __win32_fds[fd].flags = 0;
    __win32_fds[fd].handle = NULL;
    /* The standard handles belong to the process; closing descriptor 0, 1
     * or 2 leaves them open for anything else that uses them. */
    if (!(flags & __WIN32_FD_NOCLOSE) && !CloseHandle(h))
        return __win32_error(0);
    return 0;
}

static __win32_handle
duplicate(__win32_handle h)
{
    __win32_handle out;
    if (!DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &out, 0, 1,
                         __WIN32_DUPLICATE_SAME_ACCESS))
        return NULL;
    return out;
}

int
dup(int fd)
{
    __win32_handle h = __win32_handle_of(fd);
    if (!h)
        return -1;
    __win32_handle copy = duplicate(h);
    if (!copy)
        return __win32_error(0);
    int nfd = __win32_new_fd(copy, __win32_fds[fd].flags & ~(__WIN32_FD_NOCLOSE | __WIN32_FD_OPEN));
    if (nfd < 0)
        CloseHandle(copy);
    return nfd;
}

int
dup2(int fd, int fd2)
{
    __win32_handle h = __win32_handle_of(fd);
    if (!h)
        return -1;
    if (fd2 < 0 || fd2 >= __WIN32_MAX_FD) {
        errno = EBADF;
        return -1;
    }
    if (fd == fd2)
        return fd2;
    __win32_handle copy = duplicate(h);
    if (!copy)
        return __win32_error(0);
    if (__win32_fds[fd2].flags & __WIN32_FD_OPEN)
        close(fd2);
    __win32_fds[fd2].handle = copy;
    __win32_fds[fd2].flags = __win32_fds[fd].flags & ~__WIN32_FD_NOCLOSE;
    /* Programs this one starts get descriptors 0 to 2 as their standard
     * handles. */
    if (fd2 < 3)
        SetStdHandle(std_handles[fd2], copy);
    return fd2;
}

int
isatty(int fd)
{
    if (!__win32_handle_of(fd))
        return 0;
    if (__win32_fds[fd].flags & __WIN32_FD_DEVICE)
        return 1;
    errno = ENOTTY;
    return 0;
}

int
fcntl(int fd, int cmd, ...)
{
    if (!__win32_handle_of(fd))
        return -1;
    int flags = __win32_fds[fd].flags;
    switch (cmd) {
    case F_GETFD:
    case F_SETFD:
        return 0;
    case F_GETFL:
        return O_RDWR | ((flags & __WIN32_FD_APPEND) ? O_APPEND : 0)
            | ((flags & __WIN32_FD_TEXT) ? O_TEXT : O_BINARY);
    default:
        errno = EINVAL;
        return -1;
    }
}

int
setmode(int fd, int mode)
{
    if (!__win32_handle_of(fd))
        return -1;
    if (mode != O_TEXT && mode != O_BINARY) {
        errno = EINVAL;
        return -1;
    }
    int flags = __win32_fds[fd].flags;
    int old = (flags & __WIN32_FD_TEXT) ? O_TEXT : O_BINARY;
    flags &= ~(__WIN32_FD_TEXT | __WIN32_FD_EOF);
    if (mode == O_TEXT)
        flags |= __WIN32_FD_TEXT;
    __win32_fds[fd].flags = flags;
    return old;
}

int _setmode(int fd, int mode) __attribute__((__alias__("setmode")));

/* For stdio, which tracks the file position itself. */
int
__win32_is_text(int fd)
{
    return (__win32_fdflags(fd) & __WIN32_FD_TEXT) != 0;
}

intptr_t
_get_osfhandle(int fd)
{
    __win32_handle h = __win32_handle_of(fd);
    return h ? (intptr_t)h : -1;
}

int
_open_osfhandle(intptr_t handle, int flags)
{
    __win32_handle h = (__win32_handle)handle;
    int            text = (flags & O_TEXT) || (!(flags & O_BINARY) && _fmode != O_BINARY);
    return __win32_new_fd(h,
                          __win32_type_flags(h) | ((flags & O_APPEND) ? __WIN32_FD_APPEND : 0)
                              | (text ? __WIN32_FD_TEXT : 0));
}

int
pipe(int fds[2])
{
    struct __win32_security_attributes sa = { sizeof(sa), NULL, 1 };
    __win32_handle                     r, w;
    if (!CreatePipe(&r, &w, &sa, 0))
        return __win32_error(0);
    int text = _fmode != O_BINARY ? __WIN32_FD_TEXT : 0;
    fds[0] = __win32_new_fd(r, __WIN32_FD_PIPE | text);
    if (fds[0] < 0) {
        CloseHandle(r);
        CloseHandle(w);
        return -1;
    }
    fds[1] = __win32_new_fd(w, __WIN32_FD_PIPE | text);
    if (fds[1] < 0) {
        close(fds[0]);
        CloseHandle(w);
        return -1;
    }
    return 0;
}

int _close(int fd) __attribute__((__alias__("close")));
int _dup(int fd) __attribute__((__alias__("dup")));
int _dup2(int fd, int fd2) __attribute__((__alias__("dup2")));
int _isatty(int fd) __attribute__((__alias__("isatty")));
