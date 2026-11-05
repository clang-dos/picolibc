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
#include <unistd.h>

off_t
lseek(int fd, off_t offset, int whence)
{
    __win32_handle h = __win32_handle_of(fd);
    if (!h)
        return -1;
    if (__win32_fds[fd].flags & (__WIN32_FD_DEVICE | __WIN32_FD_PIPE)) {
        errno = ESPIPE;
        return -1;
    }
    __win32_dword method;
    switch (whence) {
    case SEEK_SET:
        method = __WIN32_FILE_BEGIN;
        break;
    case SEEK_CUR:
        method = __WIN32_FILE_CURRENT;
        break;
    case SEEK_END:
        method = __WIN32_FILE_END;
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    int64_t off = offset;
    long    high = (long)(off >> 32);
    SetLastError(0);
    __win32_dword low = SetFilePointer(h, (long)(uint32_t)off, &high, method);
    if (low == __WIN32_INVALID_SET_FILE_POINTER) {
        __win32_dword error = GetLastError();
        if (error != 0)
            return __win32_error(error == 131 ? 87 : error);
    }
    __win32_fds[fd].flags &= ~(__WIN32_FD_EOF | __WIN32_FD_PENDING);
    return (off_t)(((int64_t)high << 32) | low);
}

off_t _lseek(int fd, off_t offset, int whence) __attribute__((__alias__("lseek")));
