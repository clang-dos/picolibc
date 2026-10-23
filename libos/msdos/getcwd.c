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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *
getcwd(char *buf, size_t size)
{
    char path[64 + 3];
    struct __msdos_regs r = { .ax = 0x1900 };
    __msdos_int21(&r);
    path[0] = 'A' + (r.ax & 0xff);
    path[1] = ':';
    path[2] = '\\';
    r = (struct __msdos_regs){ .ax = 0x4700, .dx = 0, .si = (uint16_t)(path + 3) };
    if (__msdos_int21(&r)) {
        __msdos_error(r.ax);
        return NULL;
    }
    size_t len = strlen(path) + 1;
    if (buf == NULL) {
        /* As in glibc and BSD, a NULL buffer is allocated. */
        if (size < len)
            size = len;
        buf = malloc(size);
        if (buf == NULL)
            return NULL;
    }
    if (size == 0) {
        errno = EINVAL;
        return NULL;
    }
    if (size < len) {
        errno = ERANGE;
        return NULL;
    }
    memcpy(buf, path, len);
    return buf;
}
