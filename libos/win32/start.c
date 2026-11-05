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

#include "local-win32.h"
#include <stdlib.h>
#include <string.h>

extern char **environ;

extern int    main(int, char **, char **);
extern int __attribute__((__stdcall__, __weak__))
     WinMain(__win32_handle instance, __win32_handle prev, char *cmdline, int show);
void __main(void);
void mainCRTStartup(void);
void WinMainCRTStartup(void);

/*
 * Split a command line into words the way the Microsoft C library does.
 * The program name ends at the first space or tab, or within double quotes
 * at the next quote, and backslashes in it are kept. In the other words:
 *  - 2n backslashes and a quote are n backslashes, and the quote starts or
 *    ends a quoted part; 2n+1 backslashes and a quote are n backslashes
 *    and a quote;
 *  - backslashes not before a quote are kept;
 *  - "" in a quoted part is a quote.
 * With argv NULL, only counts the words and the bytes they need.
 */
static int
split(const char *cmd, char **argv, char *buf, size_t *bytes)
{
    int    argc = 0;
    size_t n = 0;
    /* The program name. */
    if (argv)
        argv[argc] = buf + n;
    argc++;
    if (*cmd == '"') {
        cmd++;
        while (*cmd && *cmd != '"') {
            if (argv)
                buf[n] = *cmd;
            n++;
            cmd++;
        }
        if (*cmd)
            cmd++;
    } else {
        while (*cmd && *cmd != ' ' && *cmd != '\t') {
            if (argv)
                buf[n] = *cmd;
            n++;
            cmd++;
        }
    }
    if (argv)
        buf[n] = '\0';
    n++;

    for (;;) {
        while (*cmd == ' ' || *cmd == '\t')
            cmd++;
        if (!*cmd)
            break;
        if (argv)
            argv[argc] = buf + n;
        argc++;
        int quoted = 0;
        while (*cmd && (quoted || (*cmd != ' ' && *cmd != '\t'))) {
            if (*cmd == '\\') {
                size_t slashes = 0;
                while (*cmd == '\\') {
                    slashes++;
                    cmd++;
                }
                size_t keep = *cmd == '"' ? slashes / 2 : slashes;
                for (size_t i = 0; i < keep; i++) {
                    if (argv)
                        buf[n] = '\\';
                    n++;
                }
                if (*cmd == '"' && (slashes & 1)) {
                    if (argv)
                        buf[n] = '"';
                    n++;
                    cmd++;
                }
                continue;
            }
            if (*cmd == '"') {
                if (quoted && cmd[1] == '"') {
                    if (argv)
                        buf[n] = '"';
                    n++;
                    cmd += 2;
                    continue;
                }
                quoted = !quoted;
                cmd++;
                continue;
            }
            if (argv)
                buf[n] = *cmd;
            n++;
            cmd++;
        }
        if (argv)
            buf[n] = '\0';
        n++;
    }
    if (argv)
        argv[argc] = NULL;
    *bytes = n;
    return argc;
}

void
__win32_init_args(int *argcp, char ***argvp)
{
    const char *cmd = GetCommandLineA();
    size_t      bytes;
    int         argc = split(cmd, NULL, NULL, &bytes);
    char      **argv = malloc((argc + 1) * sizeof(char *) + bytes);
    if (!argv) {
        static char *none[2] = { (char *)"", NULL };
        *argcp = 1;
        *argvp = none;
        return;
    }
    split(cmd, argv, (char *)(argv + argc + 1), &bytes);
    *argcp = argc;
    *argvp = argv;
}

#ifdef __i386__
#define ENTRY __attribute__((__force_align_arg_pointer__, __used__))
#else
#define ENTRY __attribute__((__used__))
#endif

/* The entry point of console programs. */
ENTRY void
mainCRTStartup(void)
{
    int    argc;
    char **argv;
    __win32_init();
    __win32_init_args(&argc, &argv);
    __main();
    exit(main(argc, argv, environ));
}

/* The entry point of GUI programs (-mwindows), which call WinMain. A
 * program that has main instead gets main. */
ENTRY void
WinMainCRTStartup(void)
{
    if (!WinMain) {
        mainCRTStartup();
        return;
    }
    __win32_init();
    char *cmd = GetCommandLineA();
    /* Skip the program name. */
    if (*cmd == '"') {
        cmd++;
        while (*cmd && *cmd != '"')
            cmd++;
        if (*cmd)
            cmd++;
    } else {
        while (*cmd && *cmd != ' ' && *cmd != '\t')
            cmd++;
    }
    while (*cmd == ' ' || *cmd == '\t')
        cmd++;
    struct __win32_startup_info si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    GetStartupInfoA(&si);
    int show = (si.flags & __WIN32_STARTF_USESHOWWINDOW) ? si.show_window : __WIN32_SW_SHOWDEFAULT;
    __main();
    exit(WinMain(GetModuleHandleA(NULL), NULL, cmd, show));
}
