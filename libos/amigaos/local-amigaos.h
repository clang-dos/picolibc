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

#ifndef _LOCAL_AMIGAOS_H_
#define _LOCAL_AMIGAOS_H_

#define _GNU_SOURCE

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/*
 * AmigaOS calls for the C library. The NDK headers can't be shipped, so
 * the few structure offsets and library vector offsets (LVOs) used here
 * are written out. A call puts its arguments in the registers the library
 * expects, the library base in a6, and jumps to base + LVO. d0, d1, a0
 * and a1 are scratch; the library keeps the other registers.
 */

typedef long __amiga_bptr; /* BPTR: a BCPL pointer, the address / 4 */

extern void *SysBase;
extern void *DOSBase;
extern void *_WBenchMsg;

#define __AMIGA_CALL(base, lvo, d0v, d1v, a0v, a1v, d2v, d3v, d4v)        \
    ({                                                                    \
        register long  __d0 __asm__("d0") = (long)(d0v);                  \
        register long  __d1 __asm__("d1") = (long)(d1v);                  \
        register long  __a0 __asm__("a0") = (long)(a0v);                  \
        register long  __a1 __asm__("a1") = (long)(a1v);                  \
        register long  __d2 __asm__("d2") = (long)(d2v);                  \
        register long  __d3 __asm__("d3") = (long)(d3v);                  \
        register long  __d4 __asm__("d4") = (long)(d4v);                  \
        register void *__a6 __asm__("a6") = (base);                       \
        __asm__ volatile("jsr (" #lvo ",%%a6)"                            \
                         : "+r"(__d0), "+r"(__d1), "+r"(__a0), "+r"(__a1) \
                         : "r"(__d2), "r"(__d3), "r"(__d4), "r"(__a6)     \
                         : "cc", "memory");                               \
        __d0;                                                             \
    })

/* exec.library */
#define __amiga_Forbid()           ((void)__AMIGA_CALL(SysBase, -132, 0, 0, 0, 0, 0, 0, 0))
#define __amiga_AllocMem(size, fl) ((void *)__AMIGA_CALL(SysBase, -198, size, fl, 0, 0, 0, 0, 0))
#define __amiga_FreeMem(p, size)   ((void)__AMIGA_CALL(SysBase, -210, size, 0, 0, p, 0, 0, 0))
#define __amiga_FindTask(name)     ((void *)__AMIGA_CALL(SysBase, -294, 0, 0, 0, name, 0, 0, 0))
#define __amiga_SetSignal(n, m)    ((unsigned long)__AMIGA_CALL(SysBase, -306, n, m, 0, 0, 0, 0, 0))
#define __amiga_GetMsg(port)       ((void *)__AMIGA_CALL(SysBase, -372, 0, 0, port, 0, 0, 0, 0))
#define __amiga_ReplyMsg(msg)      ((void)__AMIGA_CALL(SysBase, -378, 0, 0, 0, msg, 0, 0, 0))
#define __amiga_WaitPort(port)     ((void)__AMIGA_CALL(SysBase, -384, 0, 0, port, 0, 0, 0, 0))
#define __amiga_CloseLibrary(lib)  ((void)__AMIGA_CALL(SysBase, -414, 0, 0, 0, lib, 0, 0, 0))
#define __amiga_OpenLibrary(n, v)  ((void *)__AMIGA_CALL(SysBase, -552, v, 0, 0, n, 0, 0, 0))

/* dos.library */
#define __DOS(lvo, d1, d2, d3, d4)  __AMIGA_CALL(DOSBase, lvo, 0, d1, 0, 0, d2, d3, d4)
#define __amiga_Open(name, mode)    ((__amiga_bptr)__DOS(-30, name, mode, 0, 0))
#define __amiga_Close(fh)           ((long)__DOS(-36, fh, 0, 0, 0))
#define __amiga_Read(fh, buf, n)    ((long)__DOS(-42, fh, buf, n, 0))
#define __amiga_Write(fh, buf, n)   ((long)__DOS(-48, fh, buf, n, 0))
#define __amiga_Input()             ((__amiga_bptr)__DOS(-54, 0, 0, 0, 0))
#define __amiga_Output()            ((__amiga_bptr)__DOS(-60, 0, 0, 0, 0))
#define __amiga_Seek(fh, pos, mode) ((long)__DOS(-66, fh, pos, mode, 0))
#define __amiga_DeleteFile(name)    ((long)__DOS(-72, name, 0, 0, 0))
#define __amiga_Rename(from, to)    ((long)__DOS(-78, from, to, 0, 0))
#define __amiga_Lock(name, mode)    ((__amiga_bptr)__DOS(-84, name, mode, 0, 0))
#define __amiga_UnLock(lock)        ((void)__DOS(-90, lock, 0, 0, 0))
#define __amiga_Examine(lock, fib)  ((long)__DOS(-102, lock, fib, 0, 0))
#define __amiga_CreateDir(name)     ((__amiga_bptr)__DOS(-120, name, 0, 0, 0))
#define __amiga_CurrentDir(lock)    ((__amiga_bptr)__DOS(-126, lock, 0, 0, 0))
#define __amiga_IoErr()             ((long)__DOS(-132, 0, 0, 0, 0))
#define __amiga_DateStamp(ds)       ((void)__DOS(-192, ds, 0, 0, 0))
#define __amiga_Delay(ticks)        ((void)__DOS(-198, ticks, 0, 0, 0))
#define __amiga_IsInteractive(fh)   ((long)__DOS(-216, fh, 0, 0, 0))
/* Version 36 (Kickstart 2.0) and later. */
#define __amiga_ExamineFH(fh, fib)    ((long)__DOS(-390, fh, fib, 0, 0))
#define __amiga_NameFromLock(l, b, n) ((long)__DOS(-402, l, b, n, 0))

#define __AMIGA_MODE_READWRITE        1004
#define __AMIGA_MODE_OLDFILE          1005
#define __AMIGA_MODE_NEWFILE          1006
#define __AMIGA_SHARED_LOCK           (-2)
#define __AMIGA_OFFSET_BEGINNING      (-1)
#define __AMIGA_OFFSET_CURRENT        0
#define __AMIGA_OFFSET_END            1
#define __AMIGA_MEMF_CLEAR            0x10000

/* The version of a library: lib_Version, a UWORD at offset 20. */
static inline unsigned
__amiga_libversion(void *lib)
{
    return *(unsigned short *)((char *)lib + 20);
}

/* struct Process fields. */
#define __AMIGA_PR_MSGPORT    92
#define __AMIGA_PR_CURRENTDIR 152
#define __AMIGA_PR_CLI        172
#define __AMIGA_PR_CES        224 /* version 36 and later */

static inline long
__amiga_field(void *p, int off)
{
    return *(long *)((char *)p + off);
}

/* A FileInfoBlock is 260 bytes and must be longword aligned. */
struct __amiga_fib {
    long fib_DiskKey;
    long fib_DirEntryType; /* > 0 for a directory */
    char fib_FileName[108];
    long fib_Protection; /* RWED bits 3..0, set when not allowed */
    long fib_EntryType;
    long fib_Size;
    long fib_NumBlocks;
    long ds_Days, ds_Minute, ds_Tick;
    char fib_Comment[80];
    char fib_Reserved[36];
} __attribute__((__aligned__(4)));

/* The Amiga clock counts from 1 January 1978; local time is taken as UTC. */
#define __AMIGA_EPOCH_OFFSET 252460800L
#define __AMIGA_TICKS        50

/* Set errno from IoErr() (or from code when it is not 0) and return -1. */
int __amiga_error(long code);

/* File descriptors. */
#define __AMIGA_MAX_FD    64
#define __AMIGA_FD_OPEN   0x01
#define __AMIGA_FD_OWNED  0x02 /* closed by close() */
#define __AMIGA_FD_APPEND 0x04 /* O_APPEND */
#define __AMIGA_FD_CON    0x08 /* the console window of a Workbench start */

struct __amiga_fd {
    __amiga_bptr  fh;
    unsigned char flags;
};
extern struct __amiga_fd __amiga_fds[__AMIGA_MAX_FD];

/* The file handle of fd, or 0 with errno EBADF. Opens the console window
 * of a program started from Workbench the first time it is used. */
__amiga_bptr             __amiga_fh(int fd);
int                      __amiga_new_fd(__amiga_bptr fh, int flags);
void                     __amiga_close_fds(void);

/* Time from DateStamp(), as seconds since 1970 and ticks (1/50 s). */
void                     __amiga_time(int64_t *sec, long *tick);
/* Fill in the fields of struct stat from a FileInfoBlock. */
struct stat;
void __amiga_fib_stat(const struct __amiga_fib *fib, struct stat *st);

void __amiga_free_all(void);
void __amiga_restore_dir(void);
void __amiga_restore_cwd(void);

#endif /* _LOCAL_AMIGAOS_H_ */
