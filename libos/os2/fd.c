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

#include "local-os2.h"
#include <fcntl.h>
#include <unistd.h>

unsigned char __os2_fd_flags[__OS2_MAX_FD];

/* Whether a handle is a character device (the console, NUL, a printer)
 * or a pipe: no seeking, and \n becomes \r\n on output. */
int
__os2_isdev(int fd)
{
    __os2_word type, attr;
    if (__os2_htype((__os2_handle)fd, &type, &attr))
        return 0;
    /* HANDTYPE_FILE 0, HANDTYPE_DEVICE 1, HANDTYPE_PIPE 2 */
    return (type & 0xff) != 0;
}

int
isatty(int fd)
{
    __os2_word type, attr;
    __os2_ret  r = __os2_htype((__os2_handle)fd, &type, &attr);
    if (r)
        return __os2_error(r), 0;
    if ((type & 0xff) == 1)
        return 1;
    errno = ENOTTY;
    return 0;
}

int
close(int fd)
{
    __os2_ret r = DosClose((__os2_handle)fd);
    if (r)
        return __os2_error(r);
    if (fd >= 0 && fd < __OS2_MAX_FD)
        __os2_fd_flags[fd] = 0;
    return 0;
}

int
open(const char *path, int flags, ...)
{
    __os2_handle h;
    __os2_word   action;
    __os2_word   how;
    __os2_word   mode = __OS2_OPEN_SHARE_DENYNONE | __OS2_OPEN_FAIL_ON_ERROR;

    switch (flags & O_ACCMODE) {
    case O_RDONLY:
        break;
    case O_WRONLY:
        mode |= 1;
        break;
    default:
        mode |= 2;
        break;
    }
    if (flags & O_CLOEXEC)
        mode |= __OS2_OPEN_NOINHERIT;
    if ((flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL))
        how = __OS2_OPEN_CREATE;
    else if (flags & O_TRUNC)
        how = __OS2_OPEN_REPLACE | ((flags & O_CREAT) ? __OS2_OPEN_CREATE : 0);
    else
        how = __OS2_OPEN_EXISTING | ((flags & O_CREAT) ? __OS2_OPEN_CREATE : 0);

    __os2_ret r = DosOpen(path, &h, &action, 0, 0, how, mode, 0);
    if (r) {
        /* An existing file with O_EXCL fails with ERROR_OPEN_FAILED. */
        if (r == __OS2_ERROR_OPEN_FAILED && (flags & O_EXCL)) {
            errno = EEXIST;
            return -1;
        }
        return __os2_error(r);
    }
    int fd = (int)h;
    if (fd >= 0 && fd < __OS2_MAX_FD)
        __os2_fd_flags[fd] = (flags & O_APPEND) ? __OS2_FD_APPEND : 0;
    return fd;
}

off_t
lseek(int fd, off_t offset, int whence)
{
    unsigned long pos;
    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
        errno = EINVAL;
        return -1;
    }
    if (offset != (long)offset) {
        errno = EOVERFLOW;
        return -1;
    }
    if (__os2_isdev(fd)) {
        errno = ESPIPE;
        return -1;
    }
    /* FILE_BEGIN, FILE_CURRENT and FILE_END are SEEK_SET, SEEK_CUR and
     * SEEK_END. */
    __os2_ret r = __os2_seek((__os2_handle)fd, (long)offset, (__os2_word)whence, &pos);
    if (r)
        return __os2_error(r);
    return (off_t)pos;
}

int
ftruncate(int fd, off_t length)
{
    __os2_ret r = __os2_truncate((__os2_handle)fd, (unsigned long)length);
    return r ? __os2_error(r) : 0;
}
