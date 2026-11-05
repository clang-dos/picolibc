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

/* Read from the handle. A pipe whose writer has closed reads as the end
 * of the file. */
static ssize_t
raw_read(__win32_handle h, void *buf, size_t count)
{
    __win32_dword got;
    if (count > 0x7fffffff)
        count = 0x7fffffff;
    if (!ReadFile(h, buf, (__win32_dword)count, &got, NULL)) {
        __win32_dword error = GetLastError();
        if (error == __WIN32_ERROR_BROKEN_PIPE || error == __WIN32_ERROR_HANDLE_EOF)
            return 0;
        return __win32_error(error);
    }
    return got;
}

static void
seek_back(__win32_handle h, size_t n)
{
    long high = -1;
    SetFilePointer(h, -(long)n, &high, __WIN32_FILE_CURRENT);
}

/*
 * In text mode, CR LF reads as LF and Ctrl-Z ends the data. A CR is kept
 * unless an LF follows it. From a file, a Ctrl-Z ends the file: it stays
 * at the Ctrl-Z, and further reads return 0 until the program seeks. From
 * the console or a pipe, a Ctrl-Z ends that read only.
 */
ssize_t
read(int fd, void *buf, size_t count)
{
    __win32_handle h = __win32_handle_of(fd);
    if (!h)
        return -1;
    int flags = __win32_fds[fd].flags;
    if (!(flags & __WIN32_FD_TEXT))
        return raw_read(h, buf, count);
    if (flags & __WIN32_FD_EOF)
        return 0;
    if (count == 0)
        return 0;

    char  *p = buf;
    size_t n = 0;
    if (flags & __WIN32_FD_PENDING) {
        p[n++] = __win32_fds[fd].pending;
        __win32_fds[fd].flags = flags &= ~__WIN32_FD_PENDING;
    }
    if (n < count) {
        ssize_t got = raw_read(h, p + n, count - n);
        if (got < 0)
            return n ? (ssize_t)n : -1;
        n += got;
    }
    if (n == 0)
        return 0;

    int    file = !(flags & (__WIN32_FD_DEVICE | __WIN32_FD_PIPE));
    size_t out = 0;
    for (size_t i = 0; i < n; i++) {
        char c = p[i];
        if (c == 0x1a) {
            if (file) {
                seek_back(h, n - i);
                __win32_fds[fd].flags |= __WIN32_FD_EOF;
            }
            break;
        }
        if (c == '\r') {
            if (i + 1 < n) {
                if (p[i + 1] == '\n')
                    continue;
            } else {
                /* Look at the next byte for the LF. */
                char next;
                if (raw_read(h, &next, 1) == 1) {
                    if (next == '\n')
                        c = '\n';
                    else if (file)
                        seek_back(h, 1);
                    else {
                        __win32_fds[fd].pending = next;
                        __win32_fds[fd].flags |= __WIN32_FD_PENDING;
                    }
                }
            }
        }
        p[out++] = c;
    }
    /* A read that held only CRs that went with LFs, such as an empty
     * console line, still returns something: the LF. */
    return out;
}

ssize_t _read(int fd, void *buf, size_t count) __attribute__((__alias__("read")));
