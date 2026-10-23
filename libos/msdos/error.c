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

/* DOS error codes (INT 21h function 59h table) to errno values. */
int
__msdos_error(uint16_t code)
{
    int e;
    switch (code) {
    case 0x01: e = ENOSYS; break;  /* invalid function */
    case 0x02:                     /* file not found */
    case 0x03:                     /* path not found */
    case 0x0f:                     /* invalid drive */
    case 0x12: e = ENOENT; break;  /* no more files */
    case 0x04: e = EMFILE; break;  /* too many open files */
    case 0x05: e = EACCES; break;  /* access denied */
    case 0x06: e = EBADF; break;   /* invalid handle */
    case 0x07:                     /* memory control blocks destroyed */
    case 0x08: e = ENOMEM; break;  /* insufficient memory */
    case 0x0c: e = EINVAL; break;  /* invalid access code */
    case 0x10: e = EBUSY; break;   /* attempt to remove current directory */
    case 0x11: e = EXDEV; break;   /* not same device */
    case 0x13: e = EROFS; break;   /* write-protected disk */
    case 0x15: e = EAGAIN; break;  /* drive not ready */
    case 0x20:                     /* sharing violation */
    case 0x21: e = EBUSY; break;   /* lock violation */
    case 0x27: e = ENOSPC; break;  /* disk full */
    case 0x50: e = EEXIST; break;  /* file exists */
    default: e = EIO; break;
    }
    errno = e;
    return -1;
}
