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
#include <string.h>

/* errno for a GEMDOS or BIOS error code. */
int
__tos_error(long code)
{
    int e;
    switch (code) {
    case -2:  /* EDRVNR: drive not ready */
    case -14: /* E_CHNG: media changed */
        e = EIO;
        break;
    case -7:  /* EMEDIA */
    case -15: /* EUNDEV: unknown device */
    case -46: /* EDRIVE: invalid drive */
        e = ENXIO;
        break;
    case -13: /* EWRPRO: write protected */
        e = EROFS;
        break;
    case -32: /* EINVFN: invalid function */
        e = ENOSYS;
        break;
    case -33: /* EFILNF: file not found */
    case -34: /* EPTHNF: path not found */
        e = ENOENT;
        break;
    case -35: /* ENHNDL: no more handles */
        e = EMFILE;
        break;
    case -36: /* EACCDN: access denied */
        e = EACCES;
        break;
    case -37: /* EIHNDL: invalid handle */
        e = EBADF;
        break;
    case -39: /* ENSMEM: out of memory */
        e = ENOMEM;
        break;
    case -40: /* EIMBA: invalid memory block */
        e = EFAULT;
        break;
    case -48: /* ENSAME: not the same drive */
        e = EXDEV;
        break;
    case -49: /* ENMFIL: no more files */
        e = ENOENT;
        break;
    case -64: /* ERANGE: seek out of range */
        e = EINVAL;
        break;
    case -66: /* EPLFMT: invalid program format */
        e = ENOEXEC;
        break;
    default:
        e = EIO;
        break;
    }
    errno = e;
    return -1;
}

char *
__tos_path(const char *path, char *buf)
{
    size_t n = strlen(path);
    if (n >= __TOS_PATH_MAX) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    for (size_t i = 0; i <= n; i++)
        buf[i] = path[i] == '/' ? '\\' : path[i];
    return buf;
}
