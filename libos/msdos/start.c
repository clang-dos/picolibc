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
#include <stdlib.h>
#include <string.h>
#include <sys/_initfini.h>

uint16_t __msdos_psp;
int64_t __msdos_start_centi;
char **environ;

extern int main(int, char **, char **);

/* Bytes free between the heap and the stack in a .COM program. */
#define COM_STACK 2048
/* Most arguments a command line (at most 126 bytes) can hold. */
#define MAX_ARGS 64

static uint8_t
peek(uint16_t seg, uint16_t off)
{
    uint8_t b;
    __msdos_fromfar(&b, seg, off, 1);
    return b;
}

static uint16_t
peekw(uint16_t seg, uint16_t off)
{
    uint16_t w;
    __msdos_fromfar(&w, seg, off, 2);
    return w;
}

static void
init_heap(void)
{
    __msdos_heap_base = __msdos_brk = __msdos_heap_start;
#ifdef __HUGE__
    /* Huge pointers reach all of the program's memory: the heap runs to
     * its end, the paragraph at PSP:2. A huge pointer as an integer is its
     * linear address. */
    __msdos_heap_end = (char *)((uint32_t)peekw(__msdos_psp, 2) << 4);
    if (__msdos_heap_end < __msdos_heap_start)
        __msdos_heap_end = __msdos_heap_start;
#elif defined(__COMPACT__) || defined(__LARGE__)
    /* The heap is a segment of its own after the program, up to 64 KiB or
     * the end of the program's memory, which is at PSP:2. */
    uint16_t para = __MSDOS_SEG(__msdos_heap_start) +
                    ((__MSDOS_OFF(__msdos_heap_start) + 15u) >> 4);
    uint16_t top = peekw(__msdos_psp, 2);
    uint16_t paras = top > para ? top - para : 0;
    __msdos_heap_base = __msdos_brk = (char *)((uint32_t)para << 16);
    __msdos_heap_end = __msdos_brk + (paras >= 0x1000 ? 0xfffe : paras << 4);
#else
    /* The offset in DGROUP where the heap ends. */
    uint16_t end;
#ifdef __TINY__
    /* The stack starts at the top of the segment, or of the memory DOS
     * gave the program if that is less than 64 KiB. */
    uint16_t sp;
    __asm__("movw %%sp, %0" : "=r"(sp));
    end = sp - COM_STACK;
#else
    /* The heap is the rest of DGROUP, up to 64 KiB or the end of the
     * program's memory, which is at PSP:2. */
    uint16_t ds;
    __asm__("movw %%ds, %0" : "=r"(ds));
    uint16_t top = peekw(__msdos_psp, 2);
    uint16_t paras = top > ds ? top - ds : 0;
    end = paras >= 0x1000 ? 0xfffe : paras << 4;
#endif
    /* Pointer arithmetic keeps the segment of far data pointers. */
    uint16_t start = __MSDOS_OFF(__msdos_heap_start);
    __msdos_heap_end = __msdos_heap_start + (end > start ? end - start : 0);
#endif
}

/* Copy the environment block at PSP:2Ch. It holds NUL-terminated
 * NAME=value strings and an empty string, then (DOS 3 and later) a word
 * count and the program's full path, which becomes argv[0]. */
static char *
init_environ(void)
{
    uint16_t seg = peekw(__msdos_psp, 0x2c);
    static char *empty[1];
    environ = empty;
    if (seg == 0)
        return NULL;
    uint16_t off = 0, count = 0;
    while (peek(seg, off) != 0 && off < 0x7fff) {
        while (peek(seg, off) != 0)
            off++;
        off++;
        count++;
    }
    uint16_t end = off + 1;
    uint16_t path = 0;
    if (peekw(seg, end) >= 1) {
        path = end + 2;
        end = path;
        while (peek(seg, end) != 0)
            end++;
        end++;
    }
    char *block = malloc(end);
    char **env = malloc((count + 1) * sizeof(char *));
    if (block == NULL || env == NULL)
        return NULL;
    __msdos_fromfar(block, seg, 0, end);
    char *p = block;
    for (uint16_t i = 0; i < count; i++) {
        env[i] = p;
        p += strlen(p) + 1;
    }
    env[count] = NULL;
    environ = env;
    return path ? block + path : NULL;
}

/* Split the command tail at PSP:80h into words. Double quotes group
 * words with spaces, and \" is a literal quote, as in the Microsoft C
 * runtime. */
static int
init_args(char **argv, char *argv0)
{
    static char tail[128];
    uint8_t len = peek(__msdos_psp, 0x80);
    if (len > 126)
        len = 126;
    __msdos_fromfar(tail, __msdos_psp, 0x81, len);
    tail[len] = '\0';

    int argc = 0;
    argv[argc++] = argv0 ? argv0 : (char *)"";
    char *in = tail, *out = tail;
    while (argc < MAX_ARGS) {
        while (*in == ' ' || *in == '\t' || *in == '\r')
            in++;
        if (*in == '\0')
            break;
        argv[argc++] = out;
        int quoted = 0;
        while (*in && *in != '\r' && (quoted || (*in != ' ' && *in != '\t'))) {
            if (in[0] == '\\' && in[1] == '"') {
                *out++ = '"';
                in += 2;
            } else if (*in == '"') {
                quoted = !quoted;
                in++;
            } else {
                *out++ = *in++;
            }
        }
        if (*in)
            in++;
        *out++ = '\0';
    }
    argv[argc] = NULL;
    return argc;
}

/* Called by crt0 with the segment registers and the stack set up, BSS
 * cleared and the PSP segment saved. */
void
__msdos_start(void)
{
    static char *argv[MAX_ARGS + 1];
    int64_t s;
    uint16_t c;

    __msdos_time(&s, &c);
    __msdos_start_centi = s * 100 + c;
    init_heap();
    /* Read the command line before anything changes the DTA at PSP:80h. */
    int argc = init_args(argv, NULL);
    char *argv0 = init_environ();
    if (argv0)
        argv[0] = argv0;
    else
        argv[0] = (char *)"";
#ifdef __INIT_FINI_ARRAY
    __libc_init_array();
#endif
    exit(main(argc, argv, environ));
}
