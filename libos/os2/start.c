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

#include "local-os2.h"
#include <stdlib.h>
#include <string.h>
#include <sys/_initfini.h>
#include <unistd.h>

/*
 * The startup code (crt0.S) keeps where OS/2 put the environment and the
 * command line, then calls __os2_start.
 *
 * The environment is NAME=value strings, each ending with a NUL, and an
 * empty string. The command line is the program name, a NUL, the
 * arguments, and two NULs.
 */

typedef const char __OS2_FAR *fstr;

fstr   __os2_env;
fstr   __os2_cmdline;
char **environ;

extern int main(int, char **, char **);

#ifndef __i386__
extern char     __heap_start[];
extern uint16_t __os2_stack_top;
#endif

static size_t
fstrlen(fstr s)
{
    size_t n = 0;
    while (s[n])
        n++;
    return n;
}

/* A copy of a far string, in the C library's memory. */
static char *
fdup(fstr s, size_t n)
{
    char *d = malloc(n + 1);
    if (!d)
        _exit(255);
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
    d[n] = '\0';
    return d;
}

static void
init_environ(void)
{
    static char *empty[1];
    environ = empty;
    if (!__os2_env)
        return;
    size_t count = 0;
    for (fstr p = __os2_env; *p; p += fstrlen(p) + 1)
        count++;
    char **env = malloc((count + 1) * sizeof(char *));
    if (!env)
        return;
    size_t i = 0;
    for (fstr p = __os2_env; *p; p += fstrlen(p) + 1) {
        size_t n = fstrlen(p);
        env[i++] = fdup(p, n);
    }
    env[i] = NULL;
    environ = env;
}

/* Splits the arguments into words, as the OS/2 C compilers do: blanks
 * separate them, double quotes group them, and \" is a quote. With argv
 * NULL, only counts them. */
static int
split(char **argv, char *out, const char *s)
{
    int argc = 0;
    for (;;) {
        while (*s == ' ' || *s == '\t')
            s++;
        if (!*s)
            break;
        if (argv)
            argv[argc] = out;
        int quoted = 0;
        while (*s && (quoted || (*s != ' ' && *s != '\t'))) {
            if (*s == '\\' && s[1] == '"') {
                s++;
                if (argv)
                    *out++ = *s;
                s++;
            } else if (*s == '"') {
                quoted = !quoted;
                s++;
            } else {
                if (argv)
                    *out++ = *s;
                s++;
            }
        }
        if (argv)
            *out++ = '\0';
        argc++;
    }
    return argc;
}

void __os2_start(void) __attribute__((noreturn));

void
__os2_start(void)
{
#ifndef __i386__
    /* 1.x: the heap. */
#if defined(__COMPACT__) || defined(__LARGE__)
    __os2_word sel;
    if (DosAllocSeg(0, &sel, 0) == 0) {
        __os2_heap_base = __os2_brk = (char *)((uint32_t)sel << 16);
        __os2_heap_end = __os2_brk + 0xfff0;
    }
#else
    /* DGROUP grows to 64 KiB; the heap is its end. */
    unsigned short ds;
    __asm__("movw %%ds, %0" : "=r"(ds));
    /* The heap starts after the stack, whose size the linker's option
     * stack sets. */
    char *start = __heap_start;
    if (__os2_stack_top > (uint16_t)(uintptr_t)start)
        start = (char *)(uintptr_t)__os2_stack_top;
    __os2_heap_base = __os2_brk = start;
    if (DosReallocSeg(0, ds) == 0)
        __os2_heap_end = (char *)0xfff0;
    else
        __os2_heap_end = start;
#endif
#endif

    init_environ();

    int    argc = 0;
    char **argv;
    char  *args = NULL;
    char  *prog = (char *)"";
    if (__os2_cmdline) {
        size_t n = fstrlen(__os2_cmdline);
        prog = fdup(__os2_cmdline, n);
        fstr a = __os2_cmdline + n + 1;
        args = fdup(a, fstrlen(a));
    }
    argc = 1 + (args ? split(NULL, NULL, args) : 0);
    argv = malloc((argc + 1) * sizeof(char *) + (args ? strlen(args) + 1 : 0));
    if (!argv)
        _exit(255);
    if (args)
        split(argv + 1, (char *)(argv + argc + 1), args);
    argv[0] = prog;
    argv[argc] = NULL;

    __libc_init_array();
    exit(main(argc, argv, environ));
}
