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


#ifndef _LOCAL_MSDOS_H_
#define _LOCAL_MSDOS_H_

#define _GNU_SOURCE

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

/* Registers for a DOS call (INT 21h). A ds or es of 0 means the data
 * segment (DGROUP). On return es holds the ES that DOS returned. */
struct __msdos_regs {
    uint16_t ax, bx, cx, dx, si, di, ds, es;
};

/* The offset and segment of a pointer to pass to DOS. With near data
 * pointers the segment is DGROUP, given as 0. A far pointer as an integer
 * is segment:offset; a huge pointer as an integer is its linear address,
 * given to DOS normalized (offset 0 to 15). */
#if defined(__HUGE__)
#define __MSDOS_SEG(p) ((uint16_t)((uint32_t)(const void *)(p) >> 4))
#define __MSDOS_OFF(p) ((uint16_t)((uint32_t)(const void *)(p) & 15))
#elif defined(__COMPACT__) || defined(__LARGE__)
#define __MSDOS_SEG(p) ((uint16_t)((uint32_t)(const void *)(p) >> 16))
#define __MSDOS_OFF(p) ((uint16_t)(uint32_t)(const void *)(p))
#else
#define __MSDOS_SEG(p) ((uint16_t)0)
#define __MSDOS_OFF(p) ((uint16_t)(uintptr_t)(const void *)(p))
#endif

/* The most bytes one DOS read or write moves: a normalized huge pointer's
 * offset leaves this much room in its segment. */
#define __MSDOS_MAX_IO 0xfff0

/* Call INT 21h with *r and store the result registers back. Returns the
 * carry flag: nonzero when DOS reports an error, whose code is then in ax. */
int __msdos_int21(struct __msdos_regs *r);

/* Copy n bytes from seg:off to dst. */
void __msdos_fromfar(void *dst, uint16_t seg, uint16_t off, size_t n);

/* Segment of the program segment prefix. */
extern uint16_t __msdos_psp;
/* The stack top that crt0 found, in the small and medium models. */
extern uint16_t __msdos_stack_top;

/* Set errno from a DOS error code and return -1. */
int __msdos_error(uint16_t code);

/* Whether a handle is a character device (console, printer, NUL). */
int __msdos_isdevice(int fd);

/* Per-handle state that DOS does not keep. DOS has 20 handles per
 * program by default. */
#define __MSDOS_MAX_FD 32
#define __MSDOS_FD_APPEND 0x01 /* O_APPEND: seek to the end before writes. */
#define __MSDOS_FD_TEXT   0x02 /* O_TEXT: CR LF line ends, Ctrl-Z ends. */
#define __MSDOS_FD_EOF    0x04 /* A text read reached Ctrl-Z. */
extern uint8_t __msdos_fd_flags[__MSDOS_MAX_FD];
static inline int
__msdos_fdflags(int fd)
{
    return fd >= 0 && fd < __MSDOS_MAX_FD ? __msdos_fd_flags[fd] : 0;
}
static inline void
__msdos_set_fdflags(int fd, int flags)
{
    if (fd >= 0 && fd < __MSDOS_MAX_FD)
        __msdos_fd_flags[fd] = flags;
}

/* The DOS clock as seconds since 1970 (local time taken as UTC) and
 * hundredths of a second. */
void __msdos_time(int64_t *sec, uint16_t *centi);
/* __msdos_time at startup, in hundredths of a second. */
extern int64_t __msdos_start_centi;

#include <time.h>
time_t __msdos_mktime(const struct tm *tm);

/* Heap bounds. crt0 defines __heap_start after the stack, or after the
 * program with far data pointers; startup sets the heap's base from it, and
 * its end from the memory DOS gave the program. */
extern char __msdos_heap_start[] __asm__("___heap_start");
extern char *__msdos_heap_base;
extern char *__msdos_brk;
extern char *__msdos_heap_end;

void __msdos_start(void);

#ifdef __WINDOWS__
/* 16-bit Windows: the task's handles from InitTask, and the KERNEL
 * functions the C library uses. INT 21h works for files and the clock as
 * in DOS; memory comes from Windows. */
extern uint16_t __win16_hinstance;
extern uint16_t __win16_hprevinstance;
extern uint16_t __win16_cmdshow;
/* The command line (the PSP's command tail, NUL-terminated) for WinMain. */
extern char __win16_cmdline[128];

#define __WIN16_API __far __pascal
uint16_t __WIN16_API LocalAlloc(uint16_t flags, uint16_t bytes);
uint16_t __WIN16_API GlobalAlloc(uint16_t flags, uint32_t bytes);
void __far *__WIN16_API GlobalLock(uint16_t handle);
int __WIN16_API GetModuleFileName(uint16_t module, char __far *name, int size);

/* A DLL's local heap size and command line, from libentry.obj. */
extern uint16_t __win16_heapsize;
extern char __far *__win16_libcmdline;
int __win16_libstart(void);
/* Take the heap from Windows. */
void __win16_init_heap(void);
#endif

#endif /* _LOCAL_MSDOS_H_ */
