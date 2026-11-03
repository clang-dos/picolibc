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

#include "local-amigaos.h"

/* errno for a dos.library error code (IoErr()). */
int
__amiga_error(long code)
{
    if (!code)
        code = __amiga_IoErr();
    int e;
    switch (code) {
    case 103: /* ERROR_NO_FREE_STORE */
        e = ENOMEM;
        break;
    case 202: /* ERROR_OBJECT_IN_USE */
        e = EBUSY;
        break;
    case 203: /* ERROR_OBJECT_EXISTS */
        e = EEXIST;
        break;
    case 204: /* ERROR_DIR_NOT_FOUND */
    case 205: /* ERROR_OBJECT_NOT_FOUND */
    case 218: /* ERROR_DEVICE_NOT_MOUNTED */
        e = ENOENT;
        break;
    case 209: /* ERROR_ACTION_NOT_KNOWN */
        e = ENOSYS;
        break;
    case 210: /* ERROR_INVALID_COMPONENT_NAME */
        e = EINVAL;
        break;
    case 211: /* ERROR_INVALID_LOCK */
        e = EBADF;
        break;
    case 212: /* ERROR_OBJECT_WRONG_TYPE */
        e = ENOTDIR;
        break;
    case 214: /* ERROR_DISK_WRITE_PROTECTED */
        e = EROFS;
        break;
    case 215: /* ERROR_RENAME_ACROSS_DEVICES */
        e = EXDEV;
        break;
    case 216: /* ERROR_DIRECTORY_NOT_EMPTY */
        e = ENOTEMPTY;
        break;
    case 219: /* ERROR_SEEK_ERROR */
        e = EINVAL;
        break;
    case 221: /* ERROR_DISK_FULL */
        e = ENOSPC;
        break;
    case 222: /* ERROR_DELETE_PROTECTED */
    case 223: /* ERROR_WRITE_PROTECTED */
    case 224: /* ERROR_READ_PROTECTED */
        e = EACCES;
        break;
    case 226: /* ERROR_NO_DISK */
        e = ENXIO;
        break;
    case 233: /* ERROR_IS_SOFT_LINK */
        e = ELOOP;
        break;
    case 303: /* ERROR_BUFFER_OVERFLOW */
        e = ERANGE;
        break;
    default:
        e = EIO;
        break;
    }
    errno = e;
    return -1;
}
