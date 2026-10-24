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
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

/* A directory is read with find first and find next (INT 21h functions 4Eh
 * and 4Fh). The find data (the DTA) is kept in the DIR, and is made the
 * current DTA again before each find next, so other calls that search
 * (stat) can come in between. The search pattern follows it. */
struct dta {
    uint8_t reserved[21];
    uint8_t attr;
    uint16_t time, date;
    uint32_t size;
    char name[13];
} __attribute__((packed));

#define DIR_DTA(d) ((struct dta *)(d)->buf)
#define DIR_PATTERN(d) ((d)->buf + 64)
#define DIR_PATTERN_MAX (sizeof(((DIR *)0)->buf) - 64)

static void
set_dta(DIR *d)
{
    struct __msdos_regs r = { .ax = 0x1a00, .dx = __MSDOS_OFF(DIR_DTA(d)),
                              .ds = __MSDOS_SEG(DIR_DTA(d)) };
    __msdos_int21(&r);
}

/* Start the search; d->count is 1 while an entry is waiting. */
static int
find_first(DIR *d)
{
    set_dta(d);
    /* Files, hidden and system files, and directories. */
    struct __msdos_regs r = { .ax = 0x4e00, .cx = 0x16,
                              .dx = __MSDOS_OFF(DIR_PATTERN(d)),
                              .ds = __MSDOS_SEG(DIR_PATTERN(d)) };
    if (__msdos_int21(&r)) {
        d->count = 0;
        /* No files (12h) is an empty directory; no path (3) is not. */
        return r.ax == 0x12 ? 0 : r.ax;
    }
    d->count = 1;
    return 0;
}

/* The attributes of a file or directory (INT 21h function 4300h), or -1. */
static int
attributes(const char *path)
{
    struct __msdos_regs r = { .ax = 0x4300, .dx = __MSDOS_OFF(path),
                              .ds = __MSDOS_SEG(path) };
    if (__msdos_int21(&r))
        return -1;
    return r.cx;
}

/* "X:\", "X:/", "\" or "/", which have no directory entry of their own. */
static int
is_root(const char *p)
{
    if (p[0] && p[1] == ':')
        p += 2;
    return (p[0] == '\\' || p[0] == '/') && p[1] == '\0';
}

DIR *
opendir(const char *path)
{
    size_t n = strlen(path);
    if (n == 0) {
        errno = ENOENT;
        return NULL;
    }
    if (n + 5 > DIR_PATTERN_MAX) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    DIR *d = malloc(sizeof(DIR));
    if (d == NULL)
        return NULL;
    d->fd = -1;
    d->offset = 0;
    char *pat = DIR_PATTERN(d);
    memcpy(pat, path, n);
    if (pat[n - 1] != '/' && pat[n - 1] != '\\' && pat[n - 1] != ':')
        pat[n++] = '\\';
    memcpy(pat + n, "*.*", 4);
    int err = find_first(d);
    if (err == 0 && d->count == 0 && !is_root(path)) {
        /* No entries at all, not even "." and "..": make sure that the
         * path is a directory. */
        int a = attributes(path);
        if (a < 0 || !(a & 0x10))
            err = 3;
    }
    if (err) {
        free(d);
        if (err == 2 || err == 3) {
            /* A path that names a file is not a directory. */
            int a = attributes(path);
            errno = (a >= 0 && !(a & 0x10)) ? ENOTDIR : ENOENT;
        } else {
            __msdos_error(err);
        }
        return NULL;
    }
    return d;
}

struct dirent *
readdir(DIR *d)
{
    if (d->count == 0)
        return NULL;
    struct dta *dta = DIR_DTA(d);
    struct dirent *e = &d->dirent;
    e->d_ino = 0;
    e->d_type = (dta->attr & 0x10) ? DT_DIR : DT_REG;
    memcpy(e->d_name, dta->name, sizeof(dta->name));
    e->d_name[sizeof(dta->name)] = '\0';
    d->offset++;
    /* Find the next entry now, while the DTA is ours. */
    set_dta(d);
    struct __msdos_regs r = { .ax = 0x4f00 };
    d->count = !__msdos_int21(&r);
    return e;
}

void
rewinddir(DIR *d)
{
    d->offset = 0;
    find_first(d);
}

int
closedir(DIR *d)
{
    free(d);
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
