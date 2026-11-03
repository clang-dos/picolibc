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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int
unlink(const char *path)
{
    return __amiga_DeleteFile(path) ? 0 : __amiga_error(0);
}

int
rmdir(const char *path)
{
    struct stat st;
    if (stat(path, &st) < 0)
        return -1;
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }
    return __amiga_DeleteFile(path) ? 0 : __amiga_error(0);
}

int
rename(const char *from, const char *to)
{
    if (__amiga_Rename(from, to))
        return 0;
    /* AmigaOS won't rename over an existing file; POSIX replaces it. */
    if (__amiga_IoErr() == 203 /* object exists */ && __amiga_DeleteFile(to)
        && __amiga_Rename(from, to))
        return 0;
    return __amiga_error(0);
}

int
mkdir(const char *path, mode_t mode)
{
    (void)mode;
    __amiga_bptr lock = __amiga_CreateDir(path);
    if (!lock)
        return __amiga_error(0);
    __amiga_UnLock(lock);
    return 0;
}

/* The directory the program started in. chdir() changes the process's
 * current directory, and _exit puts the first one back. */
static __amiga_bptr start_dir;
static int          changed;

int
chdir(const char *path)
{
    __amiga_bptr lock = __amiga_Lock(path, __AMIGA_SHARED_LOCK);
    if (!lock)
        return __amiga_error(0);
    struct __amiga_fib fib;
    if (!__amiga_Examine(lock, &fib) || fib.fib_DirEntryType <= 0) {
        __amiga_UnLock(lock);
        errno = ENOTDIR;
        return -1;
    }
    __amiga_bptr old = __amiga_CurrentDir(lock);
    if (!changed) {
        start_dir = old;
        changed = 1;
    } else {
        __amiga_UnLock(old);
    }
    return 0;
}

void
__amiga_restore_cwd(void)
{
    if (changed) {
        __amiga_UnLock(__amiga_CurrentDir(start_dir));
        changed = 0;
    }
}

char *
getcwd(char *buf, size_t size)
{
    if (__amiga_libversion(DOSBase) < 36) {
        errno = ENOSYS;
        return NULL;
    }
    __amiga_bptr dir = __amiga_field(__amiga_FindTask(NULL), __AMIGA_PR_CURRENTDIR);
    char        *alloc = NULL;
    if (!buf) {
        if (!size)
            size = 256;
        buf = alloc = malloc(size);
        if (!buf)
            return NULL;
    }
    if (!dir) {
        /* No lock is the root of the boot volume. */
        if (size < 5) {
            errno = ERANGE;
            free(alloc);
            return NULL;
        }
        strcpy(buf, "SYS:");
        return buf;
    }
    if (!__amiga_NameFromLock(dir, buf, (long)size)) {
        __amiga_error(0);
        free(alloc);
        return NULL;
    }
    return buf;
}

int
access(const char *path, int mode)
{
    struct stat st;
    if (stat(path, &st) < 0)
        return -1;
    if (((mode & R_OK) && !(st.st_mode & 0400)) || ((mode & W_OK) && !(st.st_mode & 0200))
        || ((mode & X_OK) && !(st.st_mode & 0100))) {
        errno = EACCES;
        return -1;
    }
    return 0;
}
