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
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int
unlink(const char *path)
{
    if (DeleteFileA(path))
        return 0;
    __win32_dword error = GetLastError();
    __win32_dword attr = GetFileAttributesA(path);
    if (attr != __WIN32_INVALID_FILE_ATTRIBUTES && (attr & __WIN32_FILE_ATTRIBUTE_DIR)) {
        errno = EISDIR;
        return -1;
    }
    return __win32_error(error);
}

typedef __win32_bool (__attribute__((__stdcall__)) *move_file_ex_t)(const char *, const char *,
                                                                    __win32_dword);

/* rename() replaces an existing file, as in POSIX. MoveFileExA does that
 * on Windows NT; Windows 9x has only MoveFileA, so the old file is
 * deleted first there. */
int
rename(const char *from, const char *to)
{
    static move_file_ex_t move_file_ex;
    static int            looked;
    if (!looked) {
        move_file_ex = (move_file_ex_t)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                                      "MoveFileExA");
        looked = 1;
    }
    /* MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED */
    if (move_file_ex && move_file_ex(from, to, 3))
        return 0;
    if (MoveFileA(from, to))
        return 0;
    __win32_dword error = GetLastError();
    if (error == 80 || error == __WIN32_ERROR_ALREADY_EXISTS) {
        __win32_dword from_attr = GetFileAttributesA(from);
        __win32_dword to_attr = GetFileAttributesA(to);
        if (from_attr == __WIN32_INVALID_FILE_ATTRIBUTES)
            return __win32_error(0);
        if (to_attr & __WIN32_FILE_ATTRIBUTE_DIR) {
            if (!(from_attr & __WIN32_FILE_ATTRIBUTE_DIR)) {
                errno = EISDIR;
                return -1;
            }
            if (!RemoveDirectoryA(to))
                return __win32_error(0);
        } else {
            if (from_attr & __WIN32_FILE_ATTRIBUTE_DIR) {
                errno = ENOTDIR;
                return -1;
            }
            if (!DeleteFileA(to))
                return __win32_error(0);
        }
        if (MoveFileA(from, to))
            return 0;
        error = GetLastError();
    }
    return __win32_error(error);
}

int
mkdir(const char *path, mode_t mode)
{
    (void)mode;
    if (!CreateDirectoryA(path, NULL))
        return __win32_error(0);
    return 0;
}

int
rmdir(const char *path)
{
    if (!RemoveDirectoryA(path))
        return __win32_error(0);
    return 0;
}

int
chdir(const char *path)
{
    if (!SetCurrentDirectoryA(path))
        return __win32_error(0);
    return 0;
}

char *
getcwd(char *buf, size_t size)
{
    char           tmp[PATH_MAX];
    __win32_dword  n = GetCurrentDirectoryA(sizeof(tmp), tmp);
    if (n == 0) {
        __win32_error(0);
        return NULL;
    }
    if (n >= sizeof(tmp)) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    if (buf == NULL) {
        if (size == 0)
            size = n + 1;
        if (size < n + 1) {
            errno = ERANGE;
            return NULL;
        }
        buf = malloc(size);
        if (!buf)
            return NULL;
    } else if (size < n + 1) {
        errno = size ? ERANGE : EINVAL;
        return NULL;
    }
    memcpy(buf, tmp, n + 1);
    return buf;
}

int
access(const char *path, int mode)
{
    __win32_dword attr = GetFileAttributesA(path);
    if (attr == __WIN32_INVALID_FILE_ATTRIBUTES)
        return __win32_error(0);
    if ((mode & W_OK) && (attr & __WIN32_FILE_ATTRIBUTE_READONLY)
        && !(attr & __WIN32_FILE_ATTRIBUTE_DIR)) {
        errno = EACCES;
        return -1;
    }
    return 0;
}

int
chmod(const char *path, mode_t mode)
{
    __win32_dword attr = GetFileAttributesA(path);
    if (attr == __WIN32_INVALID_FILE_ATTRIBUTES)
        return __win32_error(0);
    if (mode & 0200)
        attr &= ~__WIN32_FILE_ATTRIBUTE_READONLY;
    else
        attr |= __WIN32_FILE_ATTRIBUTE_READONLY;
    /* FILE_ATTRIBUTE_DIRECTORY can't be set; leave it to Windows. */
    attr &= ~__WIN32_FILE_ATTRIBUTE_DIR;
    if (!SetFileAttributesA(path, attr ? attr : __WIN32_FILE_ATTRIBUTE_NORMAL))
        return __win32_error(0);
    return 0;
}

int
ftruncate(int fd, off_t length)
{
    __win32_handle h = __win32_handle_of(fd);
    if (!h)
        return -1;
    if (length < 0) {
        errno = EINVAL;
        return -1;
    }
    /* Keep the file position. */
    long          high = 0;
    __win32_dword low = SetFilePointer(h, 0, &high, __WIN32_FILE_CURRENT);
    long          new_high = (long)((int64_t)length >> 32);
    SetFilePointer(h, (long)(uint32_t)length, &new_high, __WIN32_FILE_BEGIN);
    int ok = SetEndOfFile(h);
    __win32_dword error = GetLastError();
    SetFilePointer(h, (long)low, &high, __WIN32_FILE_BEGIN);
    if (!ok)
        return __win32_error(error);
    return 0;
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

int
fsync(int fd)
{
    __win32_handle h = __win32_handle_of(fd);
    if (!h)
        return -1;
    if (!FlushFileBuffers(h) && !(__win32_fds[fd].flags & (__WIN32_FD_DEVICE | __WIN32_FD_PIPE)))
        return __win32_error(0);
    return 0;
}

int
fdatasync(int fd)
{
    return fsync(fd);
}

char *
realpath(const char * restrict path, char * restrict resolved)
{
    char          tmp[PATH_MAX];
    char         *file;
    __win32_dword n = GetFullPathNameA(path, sizeof(tmp), tmp, &file);
    if (n == 0) {
        __win32_error(0);
        return NULL;
    }
    if (n >= sizeof(tmp)) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    if (GetFileAttributesA(tmp) == __WIN32_INVALID_FILE_ATTRIBUTES) {
        __win32_error(0);
        return NULL;
    }
    if (!resolved) {
        resolved = malloc(n + 1);
        if (!resolved)
            return NULL;
    }
    memcpy(resolved, tmp, n + 1);
    return resolved;
}

int
link(const char *from, const char *to)
{
    (void)from;
    (void)to;
    errno = ENOSYS;
    return -1;
}

int
symlink(const char *from, const char *to)
{
    (void)from;
    (void)to;
    errno = ENOSYS;
    return -1;
}

ssize_t
readlink(const char * restrict path, char * restrict buf, size_t size)
{
    (void)buf;
    (void)size;
    if (GetFileAttributesA(path) == __WIN32_INVALID_FILE_ATTRIBUTES)
        return __win32_error(0);
    errno = EINVAL;
    return -1;
}

int _unlink(const char *path) __attribute__((__alias__("unlink")));
int _rmdir(const char *path) __attribute__((__alias__("rmdir")));
int _chdir(const char *path) __attribute__((__alias__("chdir")));
char *_getcwd(char *buf, size_t size) __attribute__((__alias__("getcwd")));
int _access(const char *path, int mode) __attribute__((__alias__("access")));
int _chmod(const char *path, mode_t mode) __attribute__((__alias__("chmod")));
