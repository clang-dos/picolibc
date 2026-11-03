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
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

void
__amiga_fib_stat(const struct __amiga_fib *fib, struct stat *st)
{
    memset(st, 0, sizeof(*st));
    /* The owner's RWED bits are set when the access is not allowed. */
    long   prot = fib->fib_Protection;
    mode_t mode = 0;
    if (!(prot & 8))
        mode |= 0444;
    if (!(prot & 4))
        mode |= 0222;
    if (!(prot & 2))
        mode |= 0111;
    if (fib->fib_DirEntryType > 0)
        mode |= S_IFDIR | 0111;
    else
        mode |= S_IFREG;
    st->st_mode = mode;
    st->st_nlink = 1;
    st->st_ino = fib->fib_DiskKey;
    st->st_size = fib->fib_Size;
    st->st_blksize = 512;
    st->st_blocks = fib->fib_NumBlocks;
    time_t t = ((time_t)fib->ds_Days * 1440 + fib->ds_Minute) * 60 + fib->ds_Tick / __AMIGA_TICKS
        + __AMIGA_EPOCH_OFFSET;
    st->st_atime = st->st_mtime = st->st_ctime = t;
}

int
stat(const char *path, struct stat *st)
{
    struct __amiga_fib fib;
    __amiga_bptr       lock = __amiga_Lock(path, __AMIGA_SHARED_LOCK);
    if (!lock)
        return __amiga_error(0);
    long ok = __amiga_Examine(lock, &fib);
    long err = ok ? 0 : __amiga_IoErr();
    __amiga_UnLock(lock);
    if (!ok)
        return __amiga_error(err);
    __amiga_fib_stat(&fib, st);
    return 0;
}

int
lstat(const char *path, struct stat *st)
{
    return stat(path, st);
}

int
fstat(int fd, struct stat *st)
{
    __amiga_bptr fh = __amiga_fh(fd);
    if (!fh)
        return -1;
    if (__amiga_IsInteractive(fh)) {
        memset(st, 0, sizeof(*st));
        st->st_mode = S_IFCHR | 0666;
        st->st_nlink = 1;
        return 0;
    }
    struct __amiga_fib fib;
    if (__amiga_libversion(DOSBase) >= 36 && __amiga_ExamineFH(fh, &fib)) {
        __amiga_fib_stat(&fib, st);
        return 0;
    }
    /* Version 34: the size from seeking to the end. */
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFREG | 0666;
    st->st_nlink = 1;
    long pos = __amiga_Seek(fh, 0, __AMIGA_OFFSET_END);
    if (pos >= 0) {
        st->st_size = __amiga_Seek(fh, pos, __AMIGA_OFFSET_BEGINNING);
    }
    return 0;
}
