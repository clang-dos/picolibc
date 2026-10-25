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
#include <unistd.h>

/* Writing 0 bytes (INT 21h function 40h) at an offset makes it the end of
 * the file: DOS cuts the file there, or extends it. */
int
ftruncate(int fd, off_t length)
{
    if (length < 0) {
        errno = EINVAL;
        return -1;
    }
    off_t cur = lseek(fd, 0, SEEK_CUR);
    if (cur < 0 || lseek(fd, length, SEEK_SET) < 0)
        return -1;
    struct __msdos_regs r = { .ax = 0x4000, .bx = (uint16_t)fd };
    int ret = 0;
    if (__msdos_int21(&r))
        ret = __msdos_error(r.ax);
    lseek(fd, cur, SEEK_SET);
    return ret;
}

int
truncate(const char *path, off_t length)
{
    int fd = open(path, O_WRONLY | O_BINARY);
    if (fd < 0)
        return -1;
    int ret = ftruncate(fd, length);
    close(fd);
    return ret;
}
