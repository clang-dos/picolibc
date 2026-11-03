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

#include "local-tos.h"
#include <stdlib.h>
#include <string.h>
#include <sys/_initfini.h>
#include <unistd.h>

extern char **environ;
extern int    main(int, char **, char **);
void          __tos_start(void) __attribute__((noreturn));

static char  *empty_env[1];

/* Count the strings of a GEMDOS environment, "NAME=value" strings each
 * ended by a 0 and the list by an empty one. With the ARGV convention
 * (Atari's extended arguments), a string "ARGV=..." is followed by the
 * arguments, argv[0] first; *args points there and the environment ends
 * before it. */
static int
scan_env(char *env, char **args)
{
    int n = 0;
    *args = NULL;
    for (char *p = env; p && *p; p += strlen(p) + 1) {
        if (!strncmp(p, "ARGV=", 5)) {
            *args = p + strlen(p) + 1;
            break;
        }
        n++;
    }
    return n;
}

/* Split the basepage command line into words: spaces separate them,
 * quotes (" or ') group them. With argv NULL, only count them. */
static int
split_cmdline(char **argv, char *out, const unsigned char *s, int n)
{
    int argc = 0, i = 0;
    while (i < n) {
        while (i < n && (s[i] == ' ' || s[i] == '\t'))
            i++;
        if (i >= n)
            break;
        if (argv)
            argv[argc] = out;
        char quote = 0;
        while (i < n && (quote || (s[i] != ' ' && s[i] != '\t'))) {
            char c = s[i++];
            if (quote && c == quote)
                quote = 0;
            else if (!quote && (c == '"' || c == '\''))
                quote = c;
            else if (argv)
                *out++ = c;
        }
        if (argv)
            *out++ = '\0';
        argc++;
    }
    return argc;
}

void
__tos_start(void)
{
    char *args;
    int   nenv = scan_env(_base->p_env, &args);

    environ = empty_env;
    char **env = malloc((nenv + 1) * sizeof(char *));
    if (env) {
        char *p = _base->p_env;
        for (int i = 0; i < nenv; i++, p += strlen(p) + 1)
            env[i] = p;
        env[nenv] = NULL;
        environ = env;
    }

    int    argc;
    char **argv;
    /* The ARGV convention is in use when the command line length is 127. */
    if (args && _base->p_cmdlin[0] == 127) {
        argc = 0;
        for (char *p = args; *p; p += strlen(p) + 1)
            argc++;
        argv = malloc((argc + 1) * sizeof(char *));
        if (!argv)
            _exit(1);
        int i = 0;
        for (char *p = args; *p; p += strlen(p) + 1)
            argv[i++] = p;
        argv[argc] = NULL;
        if (!argc) {
            static char *noargs[] = { "", NULL };
            argc = 1;
            argv = noargs;
        }
    } else {
        int                  len = _base->p_cmdlin[0];
        const unsigned char *s = _base->p_cmdlin + 1;
        if (len > 126)
            len = 126;
        for (int i = 0; i < len; i++)
            if (s[i] == '\0' || s[i] == '\r' || s[i] == '\n') {
                len = i;
                break;
            }
        argc = 1 + split_cmdline(NULL, NULL, s, len);
        argv = malloc((argc + 1) * sizeof(char *) + len + 1);
        if (!argv)
            _exit(1);
        split_cmdline(argv + 1, (char *)(argv + argc + 1), s, len);
        argv[0] = (char *)"";
        argv[argc] = NULL;
    }

    __libc_init_array();
    exit(main(argc, argv, environ));
}
