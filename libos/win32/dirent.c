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
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* A directory is read with FindFirstFileA and FindNextFileA. The DIR is
 * the first member of a larger block that keeps the search. */
struct win32_dir {
    DIR                      dir;
    __win32_handle           find;
    int                      have; /* data holds an entry not yet returned */
    struct __win32_find_data data;
    char                     pattern[PATH_MAX + 3];
};

static int
find_first(struct win32_dir *w)
{
    if (w->find != __WIN32_INVALID_HANDLE)
        FindClose(w->find);
    w->find = FindFirstFileA(w->pattern, &w->data);
    if (w->find == __WIN32_INVALID_HANDLE) {
        w->have = 0;
        __win32_dword error = GetLastError();
        return error == __WIN32_ERROR_FILE_NOT_FOUND || error == __WIN32_ERROR_NO_MORE_FILES
            ? 0
            : (int)error;
    }
    w->have = 1;
    return 0;
}

DIR *
opendir(const char *path)
{
    size_t n = strlen(path);
    if (n == 0) {
        errno = ENOENT;
        return NULL;
    }
    if (n + 3 > PATH_MAX) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    __win32_dword attr = GetFileAttributesA(path);
    if (attr == __WIN32_INVALID_FILE_ATTRIBUTES) {
        __win32_error(0);
        return NULL;
    }
    if (!(attr & __WIN32_FILE_ATTRIBUTE_DIR)) {
        errno = ENOTDIR;
        return NULL;
    }
    struct win32_dir *w = malloc(sizeof(*w));
    if (w == NULL)
        return NULL;
    memset(&w->dir, 0, sizeof(w->dir));
    w->dir.fd = -1;
    w->find = __WIN32_INVALID_HANDLE;
    memcpy(w->pattern, path, n);
    if (w->pattern[n - 1] != '/' && w->pattern[n - 1] != '\\' && w->pattern[n - 1] != ':')
        w->pattern[n++] = '\\';
    memcpy(w->pattern + n, "*", 2);
    int err = find_first(w);
    if (err) {
        free(w);
        __win32_error((__win32_dword)err);
        return NULL;
    }
    return &w->dir;
}

struct dirent *
readdir(DIR *d)
{
    struct win32_dir *w = (struct win32_dir *)d;
    if (!w->have)
        return NULL;
    struct dirent *e = &d->dirent;
    e->d_ino = 0;
    e->d_type = (w->data.attributes & __WIN32_FILE_ATTRIBUTE_DIR) ? DT_DIR : DT_REG;
    strncpy(e->d_name, w->data.name, sizeof(e->d_name) - 1);
    e->d_name[sizeof(e->d_name) - 1] = '\0';
    d->offset++;
    w->have = FindNextFileA(w->find, &w->data) != 0;
    return e;
}

void
rewinddir(DIR *d)
{
    d->offset = 0;
    find_first((struct win32_dir *)d);
}

int
closedir(DIR *d)
{
    struct win32_dir *w = (struct win32_dir *)d;
    if (w->find != __WIN32_INVALID_HANDLE)
        FindClose(w->find);
    free(w);
    return 0;
}

int
dirfd(DIR *d)
{
    (void)d;
    errno = ENOTSUP;
    return -1;
}

DIR *
fdopendir(int fd)
{
    (void)fd;
    errno = ENOSYS;
    return NULL;
}
