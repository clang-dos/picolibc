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
#include "include/io.h"
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>
#include <unistd.h>

/*
 * Opens a file by its narrow or wide name. A wide name is opened with
 * CreateFileW where the system has it (Windows NT); Windows 9x has only
 * the ANSI functions, so there the name is converted to the ANSI code
 * page first.
 */
/* From libc/stdio/sflags.c */
int __stdio_flags(const char *mode, int *optr);

typedef __win32_handle(__stdcall *create_file_w_t)(const wchar_t *, __win32_dword, __win32_dword,
                                                   void *, __win32_dword, __win32_dword,
                                                   __win32_handle);
typedef __win32_dword(__stdcall *get_file_attributes_w_t)(const wchar_t *);

static create_file_w_t         create_file_w;
static get_file_attributes_w_t get_file_attributes_w;

/* The wide functions, found at run time: Windows 9x has only stubs for
 * them, and programs for it should not import them. */
static int
have_wide(void)
{
    static volatile int looked;
    if (!looked) {
        if (!(GetVersion() & 0x80000000)) {
            __win32_handle k = GetModuleHandleA("kernel32.dll");
            get_file_attributes_w = (get_file_attributes_w_t)GetProcAddress(k, "GetFileAttributesW");
            create_file_w = (create_file_w_t)GetProcAddress(k, "CreateFileW");
        }
        looked = 1;
    }
    return create_file_w && get_file_attributes_w;
}

static __win32_handle
create(const char *path, const wchar_t *wpath, __win32_dword access,
       struct __win32_security_attributes *sa, __win32_dword disposition,
       __win32_dword attributes)
{
    __win32_dword share = __WIN32_FILE_SHARE_READ | __WIN32_FILE_SHARE_WRITE;
    if (wpath)
        return create_file_w(wpath, access, share, sa, disposition, attributes, NULL);
    return CreateFileA(path, access, share, sa, disposition, attributes, NULL);
}

static int
win32_open(const char *path, const wchar_t *wpath, int flags, int mode)
{
    __win32_dword access;
    switch (flags & O_ACCMODE) {
    case O_RDONLY:
        access = __WIN32_GENERIC_READ;
        break;
    case O_WRONLY:
        access = __WIN32_GENERIC_WRITE;
        break;
    default:
        access = __WIN32_GENERIC_READ | __WIN32_GENERIC_WRITE;
        break;
    }

    __win32_dword disposition;
    if ((flags & O_CREAT) && (flags & O_EXCL))
        disposition = __WIN32_CREATE_NEW;
    else if ((flags & O_CREAT) && (flags & O_TRUNC))
        disposition = __WIN32_CREATE_ALWAYS;
    else if (flags & O_CREAT)
        disposition = __WIN32_OPEN_ALWAYS;
    else if (flags & O_TRUNC)
        disposition = __WIN32_TRUNCATE_EXISTING;
    else
        disposition = __WIN32_OPEN_EXISTING;

    __win32_dword attributes = __WIN32_FILE_ATTRIBUTE_NORMAL;
    if ((flags & O_CREAT) && !(mode & 0222))
        attributes = __WIN32_FILE_ATTRIBUTE_READONLY;

    /* Descriptors are inherited by programs this one starts, as in POSIX,
     * unless opened with O_CLOEXEC (O_NOINHERIT). Windows 9x does not
     * know FILE_SHARE_DELETE. */
    struct __win32_security_attributes sa = { sizeof(sa), NULL, !(flags & O_CLOEXEC) };
    __win32_handle h = create(path, wpath, access, &sa, disposition, attributes);
    if (h == __WIN32_INVALID_HANDLE) {
        __win32_dword error = GetLastError();
        __win32_dword attr = wpath ? get_file_attributes_w(wpath) : GetFileAttributesA(path);
        if (attr != __WIN32_INVALID_FILE_ATTRIBUTES && (attr & __WIN32_FILE_ATTRIBUTE_DIR)) {
            /* Windows NT opens a directory with backup semantics, for
             * fstat and fsync; Windows 9x can't. */
            if ((flags & O_ACCMODE) == O_RDONLY && !(flags & O_CREAT)) {
                h = create(path, wpath, 0, &sa, __WIN32_OPEN_EXISTING,
                           __WIN32_FILE_FLAG_BACKUP_SEMANTICS);
                if (h != __WIN32_INVALID_HANDLE)
                    goto opened;
            }
            errno = EISDIR;
            return -1;
        }
        return __win32_error(error);
    }
opened:;
    int text = (flags & O_TEXT) || (!(flags & O_BINARY) && _fmode != O_BINARY);
    int fd = __win32_new_fd(h, __win32_type_flags(h) | ((flags & O_APPEND) ? __WIN32_FD_APPEND : 0)
                                   | (text ? __WIN32_FD_TEXT : 0));
    if (fd < 0)
        CloseHandle(h);
    return fd;
}

int
open(const char *path, int flags, ...)
{
    int mode = 0666;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
    }
    return win32_open(path, NULL, flags, mode);
}

int _open(const char *path, int flags, ...) __attribute__((__alias__("open")));

int
_wopen(const wchar_t *path, int flags, ...)
{
    int mode = 0666;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
    }
    if (have_wide())
        return win32_open(NULL, path, flags, mode);

    /* Windows 9x: use the ANSI name. */
    char name[PATH_MAX];
    if (!WideCharToMultiByte(__WIN32_CP_ACP, 0, path, -1, name, sizeof(name), NULL, NULL)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    return win32_open(name, NULL, flags, mode);
}

FILE *
_wfopen(const wchar_t *path, const wchar_t *wmode)
{
    char mode[8];
    size_t i;
    for (i = 0; wmode[i] && i < sizeof(mode) - 1; i++)
        mode[i] = (char)wmode[i];
    mode[i] = '\0';

    int open_flags;
    if (__stdio_flags(mode, &open_flags) == 0)
        return NULL;

    int fd = _wopen(path, open_flags, 0666);
    if (fd < 0)
        return NULL;

    FILE *f = fdopen(fd, mode);
    if (f == NULL)
        close(fd);
    return f;
}
