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

int
setmode(int fd, int mode)
{
    if (fd < 0 || fd >= __MSDOS_MAX_FD || (mode != O_TEXT && mode != O_BINARY)) {
        errno = EINVAL;
        return -1;
    }
    /* Check that the handle is open. */
    struct __msdos_regs r = { .ax = 0x4400, .bx = (uint16_t)fd };
    if (__msdos_int21(&r))
        return __msdos_error(r.ax);
    int flags = __msdos_fd_flags[fd];
    int old = (flags & __MSDOS_FD_TEXT) ? O_TEXT : O_BINARY;
    flags &= ~(__MSDOS_FD_TEXT | __MSDOS_FD_EOF);
    if (mode == O_TEXT)
        flags |= __MSDOS_FD_TEXT;
    __msdos_fd_flags[fd] = flags;
    return old;
}

int _setmode(int fd, int mode) __attribute__((alias("setmode")));
