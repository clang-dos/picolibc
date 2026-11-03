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
#include <stdlib.h>
#include <string.h>
#include <sys/_initfini.h>
#include <unistd.h>

void               *SysBase;
void               *DOSBase;
void               *_WBenchMsg;

extern char       **environ;
extern int          main(int, char **, char **);
void                __amiga_start(const char *args, long len) __attribute__((noreturn));
void                __amiga_return(int status) __attribute__((noreturn));
void                __amiga_init_fds(void);

static __amiga_bptr wb_old_dir;
static int          wb_dir_set;

/* The command name from the CLI structure: cli_CommandName, a BSTR at
 * offset 16. */
static char *
command_name(void *process)
{
    long cli = __amiga_field(process, __AMIGA_PR_CLI);
    if (!cli)
        return NULL;
    long bstr = __amiga_field((void *)(cli << 2), 16);
    if (!bstr)
        return NULL;
    unsigned char *b = (unsigned char *)(bstr << 2);
    char          *name = malloc(b[0] + 1);
    if (name) {
        memcpy(name, b + 1, b[0]);
        name[b[0]] = '\0';
    }
    return name;
}

/* Split the CLI argument string into words, the way the CLI and
 * ReadArgs() do: spaces separate words, double quotes group them, and in
 * quotes *" is a quote, *N a newline and *E an escape. With argv NULL,
 * only count the words. */
static int
split_args(char **argv, char *out, const char *s, long n)
{
    int  argc = 0;
    long i = 0;
    while (i < n) {
        while (i < n && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r'))
            i++;
        if (i >= n)
            break;
        if (argv)
            argv[argc] = out;
        if (s[i] == '"') {
            i++;
            while (i < n && s[i] != '"' && s[i] != '\n') {
                char c = s[i++];
                if (c == '*' && i < n) {
                    c = s[i++];
                    if (c == 'n' || c == 'N')
                        c = '\n';
                    else if (c == 'e' || c == 'E')
                        c = 0x1b;
                }
                if (argv)
                    *out++ = c;
            }
            i++;
        } else {
            while (i < n && s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r') {
                if (argv)
                    *out++ = s[i];
                i++;
            }
        }
        if (argv)
            *out++ = '\0';
        argc++;
    }
    return argc;
}

void
__amiga_restore_dir(void)
{
    if (wb_dir_set) {
        __amiga_CurrentDir(wb_old_dir);
        wb_dir_set = 0;
    }
}

void
__amiga_start(const char *args, long len)
{
    DOSBase = __amiga_OpenLibrary("dos.library", 33);
    if (!DOSBase)
        _exit(20);
    __amiga_init_fds();

    if (_WBenchMsg) {
        /* Workbench start: run in the program's directory (the lock of
         * sm_ArgList[0]), with argc 0 and argv the WBStartup message, as
         * other Amiga compilers do. */
        long arglist = __amiga_field(_WBenchMsg, 36);
        if (arglist) {
            long lock = __amiga_field((void *)arglist, 0);
            if (lock) {
                wb_old_dir = __amiga_CurrentDir(lock);
                wb_dir_set = 1;
            }
        }
        __libc_init_array();
        exit(main(0, (char **)_WBenchMsg, environ));
    }

    int    argc = 1 + split_args(NULL, NULL, args, len);
    char **argv = malloc((argc + 1) * sizeof(char *) + len + 1);
    if (!argv)
        _exit(20);
    split_args(argv + 1, (char *)(argv + argc + 1), args, len);
    argv[0] = command_name(__amiga_FindTask(NULL));
    if (!argv[0])
        argv[0] = (char *)"";
    argv[argc] = NULL;

    __libc_init_array();
    exit(main(argc, argv, environ));
}
