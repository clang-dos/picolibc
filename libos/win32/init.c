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

/* Setup shared by programs (start.c) and DLLs (dllstart.c). */

extern char **environ;

/* The environment block holds NAME=value strings and an empty string.
 * Entries that start with '=' (the current directory of each drive, on
 * Windows NT) are left out, as the Microsoft C library does. */
static void
init_environ(void)
{
    static char *empty[1];
    environ = empty;
    const char *block = GetEnvironmentStrings();
    if (!block)
        return;
    size_t count = 0, bytes = 0;
    for (const char *p = block; *p; p += strlen(p) + 1) {
        size_t len = strlen(p) + 1;
        bytes += len;
        if (*p != '=')
            count++;
    }
    char **env = malloc((count + 1) * sizeof(char *) + bytes);
    if (!env)
        return;
    char  *out = (char *)(env + count + 1);
    size_t i = 0;
    for (const char *p = block; *p; p += strlen(p) + 1) {
        size_t len = strlen(p) + 1;
        if (*p == '=')
            continue;
        memcpy(out, p, len);
        env[i++] = out;
        out += len;
    }
    env[i] = NULL;
    environ = env;
}

void
__win32_init(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    __win32_heap = GetProcessHeap();
    __win32_init_fds();
    init_environ();
}
