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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int
unlink(const char *path)
{
    char  buf[__TOS_PATH_MAX];
    char *name = __tos_path(path, buf);
    if (!name)
        return -1;
    long r = __tos_Fdelete(name);
    return r < 0 ? __tos_error(r) : 0;
}

int
rmdir(const char *path)
{
    char  buf[__TOS_PATH_MAX];
    char *name = __tos_path(path, buf);
    if (!name)
        return -1;
    long r = __tos_Ddelete(name);
    if (r == __TOS_EACCDN) {
        /* Not empty, or not a directory. */
        struct stat st;
        if (stat(path, &st) == 0 && !S_ISDIR(st.st_mode))
            errno = ENOTDIR;
        else
            errno = ENOTEMPTY;
        return -1;
    }
    return r < 0 ? __tos_error(r) : 0;
}

int
rename(const char *from, const char *to)
{
    char  fbuf[__TOS_PATH_MAX], tbuf[__TOS_PATH_MAX];
    char *f = __tos_path(from, fbuf), *t = __tos_path(to, tbuf);
    if (!f || !t)
        return -1;
    long r = __tos_Frename(f, t);
    /* GEMDOS won't rename over an existing file; POSIX replaces it. */
    if (r == __TOS_EACCDN && __tos_Fattrib(t, 0, 0) >= 0 && strcmp(f, t)
        && __tos_Fattrib(f, 0, 0) >= 0 && __tos_Fdelete(t) == 0)
        r = __tos_Frename(f, t);
    return r < 0 ? __tos_error(r) : 0;
}

int
mkdir(const char *path, mode_t mode)
{
    (void)mode;
    char  buf[__TOS_PATH_MAX];
    char *name = __tos_path(path, buf);
    if (!name)
        return -1;
    if (__tos_Fattrib(name, 0, 0) >= 0) {
        errno = EEXIST;
        return -1;
    }
    long r = __tos_Dcreate(name);
    return r < 0 ? __tos_error(r) : 0;
}

/* chdir() with a drive letter also makes that drive the current one. */
int
chdir(const char *path)
{
    char  buf[__TOS_PATH_MAX];
    char *name = __tos_path(path, buf);
    if (!name)
        return -1;
    int old = -1;
    if (name[0] && name[1] == ':') {
        int drv = (name[0] | 0x20) - 'a';
        if (drv < 0 || drv >= 32) {
            errno = ENOENT;
            return -1;
        }
        old = (int)__tos_Dgetdrv();
        __tos_Dsetdrv(drv);
        name += 2;
        if (!*name)
            return 0;
    }
    long r = __tos_Dsetpath(name);
    if (r < 0) {
        if (old >= 0)
            __tos_Dsetdrv(old);
        return __tos_error(r);
    }
    return 0;
}

char *
getcwd(char *buf, size_t size)
{
    char path[__TOS_PATH_MAX];
    int  drv = (int)__tos_Dgetdrv();
    long r = __tos_Dgetpath(path, 0);
    if (r < 0) {
        __tos_error(r);
        return NULL;
    }
    size_t n = strlen(path);
    size_t need = 2 + (n ? n : 1) + 1;
    char  *alloc = NULL;
    if (!buf) {
        if (size < need)
            size = need;
        buf = alloc = malloc(size);
        if (!buf)
            return NULL;
    }
    if (size < need) {
        errno = ERANGE;
        return NULL;
    }
    buf[0] = 'A' + drv;
    buf[1] = ':';
    if (n)
        memcpy(buf + 2, path, n + 1);
    else
        strcpy(buf + 2, "\\");
    return buf;
}

int
access(const char *path, int mode)
{
    struct stat st;
    if (stat(path, &st) < 0)
        return -1;
    if ((mode & W_OK) && !(st.st_mode & 0200)) {
        errno = EACCES;
        return -1;
    }
    return 0;
}
