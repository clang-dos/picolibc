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

/* Win32 error codes and the errno values they become. */
static const struct {
    uint16_t code;
    uint8_t  err;
} errors[] = {
    { 1,    EINVAL       }, /* ERROR_INVALID_FUNCTION */
    { 2,    ENOENT       }, /* ERROR_FILE_NOT_FOUND */
    { 3,    ENOENT       }, /* ERROR_PATH_NOT_FOUND */
    { 4,    EMFILE       }, /* ERROR_TOO_MANY_OPEN_FILES */
    { 5,    EACCES       }, /* ERROR_ACCESS_DENIED */
    { 6,    EBADF        }, /* ERROR_INVALID_HANDLE */
    { 7,    ENOMEM       }, /* ERROR_ARENA_TRASHED */
    { 8,    ENOMEM       }, /* ERROR_NOT_ENOUGH_MEMORY */
    { 9,    ENOMEM       }, /* ERROR_INVALID_BLOCK */
    { 10,   E2BIG        }, /* ERROR_BAD_ENVIRONMENT */
    { 11,   ENOEXEC      }, /* ERROR_BAD_FORMAT */
    { 12,   EINVAL       }, /* ERROR_INVALID_ACCESS */
    { 13,   EINVAL       }, /* ERROR_INVALID_DATA */
    { 14,   ENOMEM       }, /* ERROR_OUTOFMEMORY */
    { 15,   ENOENT       }, /* ERROR_INVALID_DRIVE */
    { 16,   EACCES       }, /* ERROR_CURRENT_DIRECTORY */
    { 17,   EXDEV        }, /* ERROR_NOT_SAME_DEVICE */
    { 18,   ENOENT       }, /* ERROR_NO_MORE_FILES */
    { 19,   EROFS        }, /* ERROR_WRITE_PROTECT */
    { 21,   EAGAIN       }, /* ERROR_NOT_READY */
    { 32,   EACCES       }, /* ERROR_SHARING_VIOLATION */
    { 33,   EACCES       }, /* ERROR_LOCK_VIOLATION */
    { 38,   EIO          }, /* ERROR_HANDLE_EOF */
    { 39,   ENOSPC       }, /* ERROR_HANDLE_DISK_FULL */
    { 50,   ENOSYS       }, /* ERROR_NOT_SUPPORTED */
    { 53,   ENOENT       }, /* ERROR_BAD_NETPATH */
    { 65,   EACCES       }, /* ERROR_NETWORK_ACCESS_DENIED */
    { 67,   ENOENT       }, /* ERROR_BAD_NET_NAME */
    { 80,   EEXIST       }, /* ERROR_FILE_EXISTS */
    { 82,   EACCES       }, /* ERROR_CANNOT_MAKE */
    { 83,   EACCES       }, /* ERROR_FAIL_I24 */
    { 87,   EINVAL       }, /* ERROR_INVALID_PARAMETER */
    { 89,   EAGAIN       }, /* ERROR_NO_PROC_SLOTS */
    { 108,  EACCES       }, /* ERROR_DRIVE_LOCKED */
    { 109,  EPIPE        }, /* ERROR_BROKEN_PIPE */
    { 111,  ENAMETOOLONG }, /* ERROR_BUFFER_OVERFLOW */
    { 112,  ENOSPC       }, /* ERROR_DISK_FULL */
    { 114,  EBADF        }, /* ERROR_INVALID_TARGET_HANDLE */
    { 122,  ERANGE       }, /* ERROR_INSUFFICIENT_BUFFER */
    { 123,  ENOENT       }, /* ERROR_INVALID_NAME */
    { 124,  EINVAL       }, /* ERROR_INVALID_LEVEL */
    { 128,  ECHILD       }, /* ERROR_WAIT_NO_CHILDREN */
    { 129,  ECHILD       }, /* ERROR_CHILD_NOT_COMPLETE */
    { 130,  EBADF        }, /* ERROR_DIRECT_ACCESS_HANDLE */
    { 131,  EINVAL       }, /* ERROR_NEGATIVE_SEEK */
    { 132,  EACCES       }, /* ERROR_SEEK_ON_DEVICE */
    { 145,  ENOTEMPTY    }, /* ERROR_DIR_NOT_EMPTY */
    { 158,  EACCES       }, /* ERROR_NOT_LOCKED */
    { 161,  ENOENT       }, /* ERROR_BAD_PATHNAME */
    { 164,  EAGAIN       }, /* ERROR_MAX_THRDS_REACHED */
    { 167,  EACCES       }, /* ERROR_LOCK_FAILED */
    { 183,  EEXIST       }, /* ERROR_ALREADY_EXISTS */
    { 193,  ENOEXEC      }, /* ERROR_BAD_EXE_FORMAT */
    { 206,  ENAMETOOLONG }, /* ERROR_FILENAME_EXCED_RANGE */
    { 215,  EAGAIN       }, /* ERROR_NESTING_NOT_ALLOWED */
    { 232,  EPIPE        }, /* ERROR_NO_DATA */
    { 267,  ENOTDIR      }, /* ERROR_DIRECTORY */
    { 1816, ENOMEM       }, /* ERROR_NOT_ENOUGH_QUOTA */
};

int
__win32_error(__win32_dword code)
{
    if (code == 0)
        code = GetLastError();
    int err = EINVAL;
    if (code >= 19 && code <= 36)
        err = EACCES; /* The disk and sharing errors. */
    for (size_t i = 0; i < sizeof(errors) / sizeof(errors[0]); i++) {
        if (errors[i].code == code) {
            err = errors[i].err;
            break;
        }
    }
    errno = err;
    return -1;
}
