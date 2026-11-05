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

/*
 * Destructors for C++ thread_local objects. clang calls
 * __cxa_thread_atexit for each one, and on Windows the C library provides
 * it, as the mingw-w64 runtime does. Each thread keeps its own list; it is
 * run when the thread ends, from a TLS callback, and for the thread that
 * calls exit, from an atexit handler, so before the destructors of
 * objects with static storage duration made earlier.
 */

struct thread_dtor {
    void               (*dtor)(void *);
    void                *obj;
    struct thread_dtor *next;
};

static __thread struct thread_dtor *thread_dtors;

static void
run_thread_dtors(void)
{
    struct thread_dtor *d;
    /* A destructor may add more. */
    while ((d = thread_dtors) != NULL) {
        thread_dtors = d->next;
        d->dtor(d->obj);
        free(d);
    }
}

int __cxa_thread_atexit(void (*dtor)(void *), void *obj, void *dso);

int
__cxa_thread_atexit(void (*dtor)(void *), void *obj, void *dso)
{
    static volatile long registered;
    (void)dso;
    if (!__atomic_exchange_n(&registered, 1, __ATOMIC_ACQ_REL))
        atexit(run_thread_dtors);
    struct thread_dtor *d = malloc(sizeof(*d));
    if (!d)
        return -1;
    d->dtor = dtor;
    d->obj = obj;
    d->next = thread_dtors;
    thread_dtors = d;
    return 0;
}

#define DLL_THREAD_DETACH 3

static void __attribute__((__stdcall__))
thread_detach(void *module, __win32_dword reason, void *reserved)
{
    (void)module;
    (void)reserved;
    if (reason == DLL_THREAD_DETACH)
        run_thread_dtors();
}

/* Before the TlsAlloc key destructors in .CRT$XLD, which std::
 * notify_all_at_thread_exit and the like use. */
static void(__attribute__((__stdcall__)) * const thread_dtor_callback)(void *, __win32_dword, void *)
    __attribute__((__section__(".CRT$XLC"), __used__))
    = thread_detach;
