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

#ifndef _LOCAL_TOS_H_
#define _LOCAL_TOS_H_

#define _GNU_SOURCE

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/*
 * Atari TOS calls for the C library. GEMDOS is trap #1, the BIOS trap #13
 * and the XBIOS trap #14. Each takes a function number and its arguments
 * on the stack, pushed last argument first, as 16-bit words or 32-bit
 * longs, and returns a long in d0. The caller pops the arguments. TOS may
 * change d0-d2 and a0-a2 and keeps the other registers.
 *
 * The macros are named after the argument sizes after the function
 * number: __TOS_L for one long, __TOS_WLL for a word and two longs.
 */

#define __TOS_CLOBBERS "d1", "d2", "a0", "a1", "a2", "cc", "memory"

#define __TOS_V(t, n)                                                     \
    ({                                                                    \
        register long __r __asm__("d0");                                  \
        __asm__ volatile("move.w #" #n ",-(%%sp)\n\t"                     \
                         "trap #" #t "\n\t"                               \
                         "lea (2,%%sp),%%sp"                                 \
                         : "=r"(__r)                                      \
                         :                                                \
                         : __TOS_CLOBBERS);                               \
        __r;                                                              \
    })

#define __TOS_W(t, n, a)                                                  \
    ({                                                                    \
        register long __r __asm__("d0");                                  \
        short         __a = (short)(a);                                   \
        __asm__ volatile("move.w %1,-(%%sp)\n\t"                          \
                         "move.w #" #n ",-(%%sp)\n\t"                     \
                         "trap #" #t "\n\t"                               \
                         "lea (4,%%sp),%%sp"                                 \
                         : "=r"(__r)                                      \
                         : "d"(__a)                                       \
                         : __TOS_CLOBBERS);                               \
        __r;                                                              \
    })

#define __TOS_L(t, n, a)                                                  \
    ({                                                                    \
        register long __r __asm__("d0");                                  \
        long          __a = (long)(a);                                    \
        __asm__ volatile("move.l %1,-(%%sp)\n\t"                          \
                         "move.w #" #n ",-(%%sp)\n\t"                     \
                         "trap #" #t "\n\t"                               \
                         "lea (6,%%sp),%%sp"                                 \
                         : "=r"(__r)                                      \
                         : "r"(__a)                                       \
                         : __TOS_CLOBBERS);                               \
        __r;                                                              \
    })

#define __TOS_WW(t, n, a, b)                                              \
    ({                                                                    \
        register long __r __asm__("d0");                                  \
        short         __a = (short)(a);                                   \
        short         __b = (short)(b);                                   \
        __asm__ volatile("move.w %2,-(%%sp)\n\t"                          \
                         "move.w %1,-(%%sp)\n\t"                          \
                         "move.w #" #n ",-(%%sp)\n\t"                     \
                         "trap #" #t "\n\t"                               \
                         "lea (6,%%sp),%%sp"                                 \
                         : "=r"(__r)                                      \
                         : "d"(__a), "d"(__b)                             \
                         : __TOS_CLOBBERS);                               \
        __r;                                                              \
    })

#define __TOS_LW(t, n, a, b)                                              \
    ({                                                                    \
        register long __r __asm__("d0");                                  \
        long          __a = (long)(a);                                    \
        short         __b = (short)(b);                                   \
        __asm__ volatile("move.w %2,-(%%sp)\n\t"                          \
                         "move.l %1,-(%%sp)\n\t"                          \
                         "move.w #" #n ",-(%%sp)\n\t"                     \
                         "trap #" #t "\n\t"                               \
                         "lea (8,%%sp),%%sp"                                 \
                         : "=r"(__r)                                      \
                         : "r"(__a), "d"(__b)                             \
                         : __TOS_CLOBBERS);                               \
        __r;                                                              \
    })

#define __TOS_LWW(t, n, a, b, c)                                          \
    ({                                                                    \
        register long __r __asm__("d0");                                  \
        long          __a = (long)(a);                                    \
        short         __b = (short)(b);                                   \
        short         __c = (short)(c);                                   \
        __asm__ volatile("move.w %3,-(%%sp)\n\t"                          \
                         "move.w %2,-(%%sp)\n\t"                          \
                         "move.l %1,-(%%sp)\n\t"                          \
                         "move.w #" #n ",-(%%sp)\n\t"                     \
                         "trap #" #t "\n\t"                               \
                         "lea (10,%%sp),%%sp"                             \
                         : "=r"(__r)                                      \
                         : "r"(__a), "d"(__b), "d"(__c)                   \
                         : __TOS_CLOBBERS);                               \
        __r;                                                              \
    })

#define __TOS_WLL(t, n, a, b, c)                                          \
    ({                                                                    \
        register long __r __asm__("d0");                                  \
        short         __a = (short)(a);                                   \
        long          __b = (long)(b);                                    \
        long          __c = (long)(c);                                    \
        __asm__ volatile("move.l %3,-(%%sp)\n\t"                          \
                         "move.l %2,-(%%sp)\n\t"                          \
                         "move.w %1,-(%%sp)\n\t"                          \
                         "move.w #" #n ",-(%%sp)\n\t"                     \
                         "trap #" #t "\n\t"                               \
                         "lea (12,%%sp),%%sp"                             \
                         : "=r"(__r)                                      \
                         : "d"(__a), "r"(__b), "r"(__c)                   \
                         : __TOS_CLOBBERS);                               \
        __r;                                                              \
    })

#define __TOS_LL(t, n, a, b)                                              \
    ({                                                                    \
        register long __r __asm__("d0");                                  \
        long          __a = (long)(a);                                    \
        long          __b = (long)(b);                                    \
        __asm__ volatile("move.l %2,-(%%sp)\n\t"                          \
                         "move.l %1,-(%%sp)\n\t"                          \
                         "move.w #" #n ",-(%%sp)\n\t"                     \
                         "trap #" #t "\n\t"                               \
                         "lea (10,%%sp),%%sp"                             \
                         : "=r"(__r)                                      \
                         : "r"(__a), "r"(__b)                             \
                         : __TOS_CLOBBERS);                               \
        __r;                                                              \
    })

#define __TOS_WLW(t, n, a, b, c)                                          \
    ({                                                                    \
        register long __r __asm__("d0");                                  \
        short         __a = (short)(a);                                   \
        long          __b = (long)(b);                                    \
        short         __c = (short)(c);                                   \
        __asm__ volatile("move.w %3,-(%%sp)\n\t"                          \
                         "move.l %2,-(%%sp)\n\t"                          \
                         "move.w %1,-(%%sp)\n\t"                          \
                         "move.w #" #n ",-(%%sp)\n\t"                     \
                         "trap #" #t "\n\t"                               \
                         "lea (10,%%sp),%%sp"                             \
                         : "=r"(__r)                                      \
                         : "d"(__a), "r"(__b), "d"(__c)                   \
                         : __TOS_CLOBBERS);                               \
        __r;                                                              \
    })


/* GEMDOS */
#define __tos_Pterm0()               __TOS_V(1, 0x00)
#define __tos_Cconws(s)              __TOS_L(1, 0x09, s)
#define __tos_Dsetdrv(d)             __TOS_W(1, 0x0e, d)
#define __tos_Dgetdrv()              __TOS_V(1, 0x19)
#define __tos_Fsetdta(dta)           __TOS_L(1, 0x1a, dta)
#define __tos_Tgetdate()             __TOS_V(1, 0x2a)
#define __tos_Tgettime()             __TOS_V(1, 0x2c)
#define __tos_Fgetdta()              __TOS_V(1, 0x2f)
#define __tos_Sversion()             __TOS_V(1, 0x30)
#define __tos_Dcreate(path)          __TOS_L(1, 0x39, path)
#define __tos_Ddelete(path)          __TOS_L(1, 0x3a, path)
#define __tos_Dsetpath(path)         __TOS_L(1, 0x3b, path)
#define __tos_Fcreate(name, attr)    __TOS_LW(1, 0x3c, name, attr)
#define __tos_Fopen(name, mode)      __TOS_LW(1, 0x3d, name, mode)
#define __tos_Fclose(h)              __TOS_W(1, 0x3e, h)
#define __tos_Fread(h, n, buf)       __TOS_WLL(1, 0x3f, h, n, buf)
#define __tos_Fwrite(h, n, buf)      __TOS_WLL(1, 0x40, h, n, buf)
#define __tos_Fdelete(name)          __TOS_L(1, 0x41, name)
#define __tos_Fseek(off, h, mode)    __TOS_LWW(1, 0x42, off, h, mode)
#define __tos_Fattrib(name, w, attr) __TOS_LWW(1, 0x43, name, w, attr)
#define __tos_Fdup(h)                __TOS_W(1, 0x45, h)
#define __tos_Fforce(h, nh)          __TOS_WW(1, 0x46, h, nh)
#define __tos_Dgetpath(buf, drv)     __TOS_LW(1, 0x47, buf, drv)
#define __tos_Malloc(n)              __TOS_L(1, 0x48, n)
#define __tos_Mfree(p)               __TOS_L(1, 0x49, p)
#define __tos_Pterm(code)            __TOS_W(1, 0x4c, code)
#define __tos_Fsfirst(name, attr)    __TOS_LW(1, 0x4e, name, attr)
#define __tos_Fsnext()               __TOS_V(1, 0x4f)
#define __tos_Frename(old, new)      __TOS_WLL(1, 0x56, 0, old, new)
#define __tos_Fdatime(t, h, set)     __TOS_LWW(1, 0x57, t, h, set)

/* XBIOS */
#define __tos_Supexec(fn)            __TOS_L(14, 38, fn)

/* GEMDOS error codes. */
#define __TOS_EFILNF  (-33) /* file not found */
#define __TOS_EPTHNF  (-34) /* path not found */
#define __TOS_EACCDN  (-36) /* access denied */
#define __TOS_ENMFIL  (-49) /* no more files */

/* File attributes. */
#define __TOS_FA_RDONLY 0x01
#define __TOS_FA_HIDDEN 0x02
#define __TOS_FA_SYSTEM 0x04
#define __TOS_FA_LABEL  0x08
#define __TOS_FA_DIR    0x10
#define __TOS_FA_ARCH   0x20

/* The disk transfer area that Fsfirst() and Fsnext() fill in. */
struct __tos_dta {
    char           d_reserved[21];
    unsigned char  d_attrib;
    unsigned short d_time;
    unsigned short d_date;
    unsigned long  d_length;
    char           d_fname[14];
};

/* The basepage GEMDOS builds in front of each program. */
struct __tos_basepage {
    char          *p_lowtpa;
    char          *p_hitpa;
    char          *p_tbase;
    long           p_tlen;
    char          *p_dbase;
    long           p_dlen;
    char          *p_bbase;
    long           p_blen;
    char          *p_dta;
    struct __tos_basepage *p_parent;
    long           p_reserved;
    char          *p_env;
    char           p_undef[80];
    unsigned char  p_cmdlin[128];
};

extern struct __tos_basepage *_base;

/* Set errno from a negative GEMDOS error code and return -1. */
int  __tos_error(long code);

/* A GEMDOS path for a C path: forward slashes become backslashes. Returns
 * NULL with errno ENAMETOOLONG when it doesn't fit. */
#define __TOS_PATH_MAX 256
char *__tos_path(const char *path, char *buf);

/* File descriptors. */
#define __TOS_MAX_FD    32
#define __TOS_FD_OPEN   0x01
#define __TOS_FD_OWNED  0x02 /* closed by close() */
#define __TOS_FD_APPEND 0x04 /* O_APPEND */
#define __TOS_FD_TTY    0x08 /* a character device: \n is written as \r\n */
#define __TOS_FD_NOTTY  0x10 /* checked, and not one */

struct __tos_fd {
    short         handle;
    unsigned char flags;
};
extern struct __tos_fd __tos_fds[__TOS_MAX_FD];

/* The fd's entry, or NULL with errno EBADF. */
struct __tos_fd *__tos_fd(int fd);
int              __tos_new_fd(short handle, int flags);
int              __tos_is_tty(struct __tos_fd *f);
void             __tos_close_fds(void);

/* Seconds since 1970 for a GEMDOS date and time (local time, taken as
 * UTC). */
int64_t __tos_unix_time(unsigned date, unsigned time);
/* The 200 Hz system timer. */
unsigned long __tos_hz200(void);

#endif /* _LOCAL_TOS_H_ */
