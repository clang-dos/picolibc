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

/* Sets errno for an OS/2 error code and returns -1. */
int
__os2_error(unsigned code)
{
    int e;
    switch (code) {
    case 2:   /* ERROR_FILE_NOT_FOUND */
    case 3:   /* ERROR_PATH_NOT_FOUND */
    case 18:  /* ERROR_NO_MORE_FILES */
    case 110: /* ERROR_OPEN_FAILED */
        e = ENOENT;
        break;
    case 4: /* ERROR_TOO_MANY_OPEN_FILES */
        e = EMFILE;
        break;
    case 5:  /* ERROR_ACCESS_DENIED */
    case 32: /* ERROR_SHARING_VIOLATION */
    case 33: /* ERROR_LOCK_VIOLATION */
        e = EACCES;
        break;
    case 6: /* ERROR_INVALID_HANDLE */
        e = EBADF;
        break;
    case 8: /* ERROR_NOT_ENOUGH_MEMORY */
        e = ENOMEM;
        break;
    case 15: /* ERROR_INVALID_DRIVE */
        e = ENODEV;
        break;
    case 16:  /* ERROR_CURRENT_DIRECTORY */
    case 108: /* ERROR_DRIVE_LOCKED */
        e = EBUSY;
        break;
    case 17: /* ERROR_NOT_SAME_DEVICE */
        e = EXDEV;
        break;
    case 19: /* ERROR_WRITE_PROTECT */
        e = EROFS;
        break;
    case 80: /* ERROR_FILE_EXISTS */
        e = EEXIST;
        break;
    case 1:   /* ERROR_INVALID_FUNCTION */
    case 87:  /* ERROR_INVALID_PARAMETER */
    case 131: /* ERROR_NEGATIVE_SEEK */
        e = EINVAL;
        break;
    case 109: /* ERROR_BROKEN_PIPE */
        e = EPIPE;
        break;
    case 112: /* ERROR_DISK_FULL */
        e = ENOSPC;
        break;
    case 123: /* ERROR_INVALID_NAME */
    case 206: /* ERROR_FILENAME_EXCED_RANGE */
        e = ENAMETOOLONG;
        break;
    default:
        e = EIO;
        break;
    }
    errno = e;
    return -1;
}
