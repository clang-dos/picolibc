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
#include <unistd.h>


static ssize_t
dos_read(int fd, void *buf, size_t count)
{
    struct __msdos_regs r = { .ax = 0x3f00, .bx = (uint16_t)fd,
                              .cx = (uint16_t)count, .dx = (uint16_t)buf };
    if (count > 0xfffe)
        r.cx = 0xfffe;
    if (__msdos_int21(&r))
        return __msdos_error(r.ax);
    return r.ax;
}

static void
seek_back(int fd, uint16_t n)
{
    uint32_t o = -(int32_t)n;
    struct __msdos_regs r = { .ax = 0x4201, .bx = (uint16_t)fd,
                              .cx = (uint16_t)(o >> 16), .dx = (uint16_t)o };
    __msdos_int21(&r);
}

/* In text mode, CR LF reads as LF and Ctrl-Z ends the file. A CR is kept
 * unless an LF follows it; from a device, such as the console, a CR at the
 * end of the data is dropped too. */
ssize_t
read(int fd, void *buf, size_t count)
{
    int flags = __msdos_fdflags(fd);
    if (!(flags & __MSDOS_FD_TEXT))
        return dos_read(fd, buf, count);
    if (flags & __MSDOS_FD_EOF)
        return 0;

    ssize_t got = dos_read(fd, buf, count);
    if (got <= 0)
        return got;
    size_t n = got;
    int device = -1;
    char *p = buf;
    size_t out = 0;
    for (size_t i = 0; i < n; i++) {
        char c = p[i];
        if (c == 0x1a) {
            if (device < 0)
                device = __msdos_isdevice(fd) > 0;
            if (!device) {
                /* Leave the file at the Ctrl-Z, and read nothing more. */
                seek_back(fd, n - i);
                __msdos_set_fdflags(fd, flags | __MSDOS_FD_EOF);
            }
            break;
        }
        if (c == '\r') {
            if (i + 1 < n) {
                if (p[i + 1] == '\n')
                    continue;
            } else {
                if (device < 0)
                    device = __msdos_isdevice(fd) > 0;
                if (device)
                    continue;
                /* Look at the next byte for the LF. */
                char next;
                if (dos_read(fd, &next, 1) == 1) {
                    if (next == '\n')
                        c = '\n';
                    else
                        seek_back(fd, 1);
                }
            }
        }
        p[out++] = c;
    }
    return out;
}
