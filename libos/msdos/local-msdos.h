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
 * pointers the segment is DGROUP, given as 0. */
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
#define __MSDOS_SEG(p) ((uint16_t)((uint32_t)(const void *)(p) >> 16))
#else
#define __MSDOS_SEG(p) ((uint16_t)0)
#endif
#define __MSDOS_OFF(p) ((uint16_t)(uintptr_t)(const void *)(p))

/* Call INT 21h with *r and store the result registers back. Returns the
 * carry flag: nonzero when DOS reports an error, whose code is then in ax. */
int __msdos_int21(struct __msdos_regs *r);

/* Copy n bytes from seg:off to dst. */
void __msdos_fromfar(void *dst, uint16_t seg, uint16_t off, size_t n);

/* Segment of the program segment prefix. */
extern uint16_t __msdos_psp;

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

/* Heap bounds. crt0 defines __heap_start after the stack; startup sets the
 * end from the memory DOS gave the program. */
extern char __msdos_heap_start[] __asm__("___heap_start");
extern char *__msdos_brk;
extern char *__msdos_heap_end;

void __msdos_start(void);

#endif /* _LOCAL_MSDOS_H_ */
